#include"HttpParser.h"
#include"string.h"
#include<sys/stat.h>
#include<sys/types.h>
#include<unistd.h>
#include<sys/mman.h>
#include<fcntl.h>

using namespace std;

#define CRLF "\r\n"
#define SPACE " "
#define COLON ": "
//资源根目录
#define RESOURCE_ROOT "/home/yjh/www/html"
#define ContentLength "Content-Length: "
#define Connection "Connection: "
//保持连接
#define KeepAlive "keep-alive\r\n"
//关闭连接
#define CLOSE "close\r\n"
#define HTTP10 "HTTP/1.0 "
#define HTTP11 "HTTP/1.1 "
#define OK "200 OK"
#define BADREQUEST "400 Bad Request"
#define NOTFOUND "404 Not Found"
#define INTERNALERROR "500 Internal Server Error"
#define NotFound404_Form "The requested file was not found on this server.\n"
#define BadRequest400_Form "Your request has bad syntax or this inherently impossiable to satisfy.\n"
#define INTERNALERROR500_Form "There was an unusual problem serving the requested file.\n"

HttpParser::HttpParser(const TcpConnectionPtr& conn)
    : connecting_(false), pos_(0), checkState_(Check_RequestLine),
    state_(NoRequest), fileAddress_(nullptr), filelen_(0),
    fileName_(RESOURCE_ROOT), conn_(conn)
{

}


void HttpParser::init(bool clear)
{
    checkState_ = Check_RequestLine;
    state_ = NoRequest;
    fileAddress_ = nullptr;
    filelen_ = 0;
    contentLength_ = 0;
    if(clear)
    {
        input_.clear();
        pos_ = 0;
    }
    output_.clear();
    fileName_.clear();
}

void HttpParser::send()
{
    if(conn_)
    {
        conn_->send(output_);
    }
}

void HttpParser::process()
{
    while(processRead() != NoRequest)
    {
        processWrite();
        send();
        init(input_.size() == pos_);    //当前所有请求报文都已解析     
    } 
    
}

HttpParser::HttpState HttpParser::processRead()
{
    LineState lineState = Line_OK;
    HttpState ret = NoRequest;
    while(((checkState_ == Check_Content) && lineState == Line_OK)
        || ((lineState = parseLine()) == Line_OK))
    {

        switch(checkState_)
        {
            case Check_RequestLine:
            {
                ret = parseRequestLine(input_.data() + pos_);
                if(ret == Invalid)
                {
                    state_ = Invalid;
                    return Invalid;
                }
                    
                break;
            }
            case Check_Header:
            {
                ret = parseHeaders(input_.data() + pos_);
                if(ret == Invalid)
                    return Invalid;
                if(ret == GetRequest)
                    return doRequest();
            }
            case Check_Content :
            {
                ret = parseContent();
                if(ret == GetRequest)
                    return doRequest();
                lineState = Line_Open;
                break;
            }
            default:
                state_ = InternalError;
                return InternalError;
        }
    }
    if(lineState == Line_Bad)
    {
        state_ = Invalid;
        return Invalid;
    }
    state_ = NoRequest;
    return NoRequest;
}

HttpParser::LineState HttpParser::parseLine()
{
    size_t n = input_.size();
    while(pos_ < n)
    {
        if(input_[pos_] == '\r')
        {//遇到\r则可能接受完整的行
            if(pos_ == n - 1)
            {
                //没读完，不是完整的行
                return Line_Open;
            }
            if(input_[pos_+1] == '\n')
            {
                input_[pos_] = '\0';
                input_[pos_+1] = '\0';
                pos_ += 2;
                return Line_OK;
            }
            return Line_Bad;    //格式出错
        }
        if(input_[pos_] == '\n')
        {
            if(input_[pos_-1] == '\r')
            {
                input_[pos_-1] = '\0';
                input_[pos_++] = '\0';
                return Line_OK;
            }
            return Line_Bad;
        }
        ++pos_;
    }
    return Line_Open;
}

HttpParser::HttpState HttpParser::doRequest()
{
    if(request_.getMethod() == Method::HEAD)
    {
        OnlyHead;
        return OnlyHead;
    }
        
    bool ret = getResource();
    if(!ret)
    {
        state_ = NotFound;
        return NotFound;
    }
    state_ = GetRequest;
    return GetResource;
}

void HttpParser::processWrite()
{
    appendResponseLine();  //填充响应行
    appendHeaders();    //填充首部字段
    if(state_ != OnlyHead)
        appendContent();
}

void HttpParser::appendResponseLine()
{
    //填充
    if(response_.getVersion() == Version::Http10)
    {
        output_.append(HTTP10);
    }
    else
    {
        output_.append(HTTP11);
    }
    switch (state_)
    {
    case InternalError:
        output_.append(INTERNALERROR);
        break;
    case NotFound:
        output_.append(NOTFOUND);
        break;
    case OnlyHead:
    case GetResource:
        output_.append(OK);
        break;
    default:
        output_.append(BADREQUEST);
        break;
    }
    output_.append(CRLF);
}

void HttpParser::appendHeaders()
{
    if(contentLength_ != 0)
    {
        output_.append(ContentLength);
        output_.append(to_string(contentLength_));
        output_.append(CRLF);
    }
    output_.append(Connection);
    if(connecting_)
    {
        output_.append(KeepAlive);
    }
    else
    {
        output_.append(CLOSE);
    }
}

void HttpParser::appendContent()
{
    switch (state_)
    {
    case InternalError:
        output_.append(INTERNALERROR500_Form);
        break;
    case NotFound:
        output_.append(NotFound404_Form);
        break;
    case GetResource:
        output_.append(fileAddress_);
    default:
        output_.append(BadRequest400_Form);
        break;
    }
    unmap();
}


HttpParser::HttpState HttpParser::parseRequestLine(const char* start)
{
    string line(start);
    int begin = 0;
    int pos = line.find(SPACE, begin); //获取请求方法
    if(pos == string::npos)
        return Invalid;
    bool ret = setMethod(line.substr(begin, pos - begin));
    if(!ret)
        return Invalid;   //仅支持GET、POST、HEAD
    begin = pos +1;  //跳过空格
    pos = line.find(SPACE, begin);    //获取URI
    if(pos == string::npos || pos == begin)
        return Invalid;
    setURI(line.substr(begin, pos - begin));    //设置URI
    begin = pos + 1;    //跳过空格
    if(line.size() == begin)
        return Invalid;
    ret = setVersion(line.substr(begin));   //设置版本
    if(!ret)
        return Invalid;
    checkState_ = Check_Header; //解析完请求行
    return NoRequest;
}

bool HttpParser::setMethod(const string& method)
{
    if(method == "GET")
    {
        request_.setMethod(Method::GET);
    }
    else if(method == "POST")
    {
        request_.setMethod(Method::POST);
    }
    else if(method == "HEAD")
    {
        request_.setMethod(Method::HEAD);
    }
    else
    {
        request_.setMethod(Method::Invalid);
        return false;
    }
    return true;
}

bool HttpParser::setURI(const string& uri)
{
    int begin = 0;
    if(uri.compare(begin, 7, "http://") == 0)
        begin +=  7;
    begin = uri.find('/', begin);
    if(begin == string::npos)
        return false;
    request_.setUri(uri.substr(begin));

}

bool HttpParser::setVersion(const string& version)
{
    if(version == "HTTP/1.0")
    {
        request_.setVersion(Version::Http10);
        response_.setVersion(Version::Http10);
    }
    else if(version == "HTTP/1.1")
    {
        request_.setVersion(Version::Http11);
        response_.setVersion(Version::Http11);
    }
    else
    {
        request_.setVersion(Version::Unknow);
        response_.setVersion(Version::Unknow);
        return false;
    }    
    return true;
}

HttpParser::HttpState HttpParser::parseHeaders(const char* start)
{
    if(start[0] == '\0')
    {
        //头部字段解析完毕
        //如果有消息体
        if(contentLength_ != 0)
        {
            checkState_ = Check_Content;
            return NoRequest;
        }
        return GetRequest;
    }
    string line(start);
    int pos = line.find(COLON); //查找冒号
    if(pos == string::npos)
        return Invalid;
    const string& key = line.substr(0, pos);
    pos += 2;   //跳过冒号空格
    if(line.size() <= pos)
        return Invalid;   //没有值
    request_.addHeader(key, line.substr(pos));  //记录首部字段
    return NoRequest;
}

HttpParser::HttpState HttpParser::parseContent()
{
    if(input_.size() - pos_ >= contentLength_)
    {
        recvBody(input_.data() + pos_, input_.data() + pos_ + contentLength_);
        pos_ += contentLength_;
        return GetRequest;
    }
    return NoRequest;
}

bool HttpParser::getResource()
{
    fileName_.append(request_.getURI());    //RESOURCE_ROOT + URI
    struct stat fileStat;
    if(stat(fileName_.c_str(), &fileStat) < 0)
        return false;
    if(!(fileStat.st_mode & S_IROTH))   //对用户不具可读取权限
        return false;
    if(S_ISDIR(fileStat.st_mode))   //是目录
    {
        return false;
    }
    
    int fd = open(fileName_.c_str(), O_RDONLY);
    if(fd < 0)
        return false;
    filelen_ = fileStat.st_size;
    fileAddress_ = static_cast<char*>(mmap(0, filelen_, PROT_READ, MAP_PRIVATE, fd, 0));
    close(fd);
    return true;
}

void HttpParser::unmap()
{
    if(fileAddress_)
    {
        munmap(fileAddress_, filelen_);
        fileAddress_ = nullptr;
        filelen_ = 0;
    }
}
