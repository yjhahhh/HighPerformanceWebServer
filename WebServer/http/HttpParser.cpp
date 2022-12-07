#include"HttpParser.h"
#include"string.h"
#include<sys/stat.h>
#include<sys/types.h>
#include<unistd.h>
#include<sys/mman.h>
#include<fcntl.h>
#include"../base/Logging.h"
#include<map>

using namespace std;

#define CRLF "\r\n"
#define SPACE " "
#define COLON ": "
//资源根目录
#define RESOURCE_ROOT "/home/yjh/www/html"
#define ContentLength "Content-Length: "
#define Connection "Connection: "
#define ContentType "Content-Type: "
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

static map<string, string> mime {
    {".html", "text/html\r\n"},
    {".avi", "video/x-msvideo\r\n"},
    {".bmp", "image/bmp\r\n"},
    {".c", "text/plain"},
    {".doc", "application/msword\r\n"},
    {".gif", "image/gif\r\n"},
    {".gz","application/x-gzip\r\n"},
    {".htm", "text/html\r\n"},
    {".ico", "image/x-icon\r\n"},
    {".jpg", "image/jpeg\r\n"},
    {".png", "image/png\r\n"},
    {".txt", "text/plain\r\n"},
    {".mp3", "audio/mp3\r\n"},
    {"default", "text/html\r\n"}
};

HttpParser::HttpParser(const TcpConnectionPtr& conn)
    : connecting_(false), pos_(0), checkState_(Check_RequestLine),
    state_(NoRequest), fileContent_(nullptr), filelen_(0),
    contentLength_(0), fileName_(RESOURCE_ROOT), conn_(conn)
{

}


void HttpParser::init(bool clear)
{
    checkState_ = Check_RequestLine;
    state_ = NoRequest;
    if(fileContent_)
        delete fileContent_;
    fileContent_ = nullptr;
    filelen_ = 0;
    contentLength_ = 0;
    if(clear)
    {
        input_.clear();
        pos_ = 0;
    }
    output_.clear();
    fileName_.assign(RESOURCE_ROOT);
}

void HttpParser::send()
{
    TcpConnectionPtr conn = conn_.lock();
    if(conn)
    {
        conn->send(output_);
    }
}

bool HttpParser::process()
{
    while(processRead() != NoRequest)
    {
        processWrite();
        send();
        if(!connecting_)
        {
            return false;
        }    
        init(input_.size() == pos_);    //当前所有请求报文都已解析     
    } 
    return connecting_;
}

HttpParser::HttpState HttpParser::processRead()
{
    LineState lineState = Line_OK;
    HttpState ret = NoRequest;
    size_t begin = pos_;
    while(((checkState_ == Check_Content) && lineState == Line_OK)
        || ((lineState = parseLine()) == Line_OK))
    {

        switch(checkState_)
        {
            case Check_RequestLine:
            {
                ret = parseRequestLine(input_.data() + begin);
                if(ret == Invalid)
                {
                    state_ = Invalid;
                    return Invalid;
                }
                    
                break;
            }
            case Check_Header:
            {
                ret = parseHeaders(input_.data() + begin);
                if(ret == Invalid)
                {
                    return Invalid;
                }    
                if(ret == GetRequest)
                    return doRequest();
                break;
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
        begin = pos_;
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
        state_ = OnlyHead;
        return OnlyHead;
    }
        
    HttpState ret = getResource();
    if(InternalError == ret)
    {
        state_ = InternalError;
        return InternalError;
    }
    else if(NotFound == ret)
    {
        state_ = NotFound;
        return NotFound;
    }
    state_ = GetResource;
    return GetResource;
}

void HttpParser::processWrite()
{
    appendResponseLine();  //填充响应行
    appendHeaders();    //填充首部字段
    TcpConnectionPtr conn = conn_.lock();
    if(conn)
        LOG_INFO << conn->peerAddr().toIpPort() << "\n" << output_.c_str();
    if(state_ != OnlyHead || state_ != NoRequest)
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
    if(filelen_!= 0)
    {
        output_.append(ContentLength);
        output_.append(to_string(filelen_));
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
    output_.append(ContentType);
    size_t pos = request_.getURI().find_last_of('.');
    if(pos != string::npos)
    {
        string type = request_.getURI().substr(pos);
        if(mime.find(type) != mime.end())
        {
            output_.append(mime[type]);
        }
        else
        {
            output_.append(mime["default"]);
        }
    }
    else
    {
        output_.append(mime["default"]);
    }
    output_.append(CRLF);
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
        output_.append(fileContent_);
        break;
    default:
        output_.append(BadRequest400_Form);
        break;
    }
    
}


HttpParser::HttpState HttpParser::parseRequestLine(const char* start)
{
    string line(start);
    size_t begin = 0;
    size_t pos = line.find(SPACE, begin); //获取请求方法
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
    TcpConnectionPtr conn = conn_.lock();
    if(conn)
        LOG_INFO << conn->peerAddr().toIpPort() << '\n' << line;
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
    size_t begin = 0;
    if(uri.compare(begin, 7, "http://") == 0)
        begin +=  7;
    begin = uri.find('/', begin);
    if(begin == string::npos)
        return false;
    request_.setUri(uri.substr(begin));
    return true;
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
    size_t pos = line.find(COLON); //查找冒号
    if(pos == string::npos)
        return Invalid;
    const string& key = line.substr(0, pos);
    pos += 2;   //跳过冒号空格
    if(line.size() <= pos)
        return Invalid;   //没有值
    const string& value = line.substr(pos);
    if(key == "Content-Length")
        contentLength_ = atoi(value.c_str());
    else if(key == "Connection" && (value == "keep-alive" || value == "Keep-Alive"))
        connecting_ = true;
    request_.addHeader(key, value);  //记录首部字段
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

HttpParser::HttpState HttpParser::getResource()
{
    if(request_.getURI() == "/hello")
    {
        fileContent_ = new char[14];
        strcpy(fileContent_, "Hello World!\n");
        return GetResource;
    }
    fileName_.append(request_.getURI());    //RESOURCE_ROOT + URI
    struct stat fileStat;
    if(stat(fileName_.c_str(), &fileStat) < 0)
    {
        return NotFound;
    }    
    if(!(fileStat.st_mode & S_IROTH))   //对用户不具可读取权限
    {
        return NotFound;
    }
        
    if(S_ISDIR(fileStat.st_mode))   //是目录
    {
        return NotFound;
    }
    FILE* file = fopen(fileName_.c_str(), "r");
    if(!file)
    {
        return InternalError;
    }
    size_t filelen_ = fileStat.st_size;
    fileContent_ = new char[filelen_ + 1];
    size_t n = fread(fileContent_, 1, filelen_, file);
    fclose(file);
    if(n < filelen_ || n == 0)
    {
        delete fileContent_;
        fileContent_ = nullptr;
        return InternalError;
    }
    
    return GetResource;
}

void HttpParser::recvBody(const char* start, const char* end)
{
    request_.setBody(start, end);
}