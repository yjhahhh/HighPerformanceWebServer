#ifndef _HTTPPARSER_
#define _HTTPPARSER_
#include"HttpResponse.h"
#include"../TcpConnection.h"

/*支持GET、POST、HEAD*/

class HttpParser
{
public:
    typedef HttpRequest::Method Method;
    typedef HttpRequest::Version Version;

    //主状态机
    enum CheckState
    {
        Check_RequestLine,    //正在分析请求行
        Check_Header,   //正在分析首部字段
        Check_Content   //正在分析主体
    };
    //从状态机  分析一行
    enum LineState
    {
        Line_OK,    //读取完整的行
        Line_Bad,   //格式错误
        Line_Open   //不完整的行，需要继续读取
    };
    //处理http请求可能的结果
    enum HttpState
    {
        Invalid,    //请求报文格式错误
        NotFound,   //没有请求的资源
        NoRequest,  //请求不完整，需要继续读数据
        GetRequest, //获得了一个完整的http请求
        GetResource,    //获取了资源
        OnlyHead,   //不发送消息体，HEAD方法
        InternalError   //服务器内部错误
    };

    HttpParser(const TcpConnectionPtr& conn);
    ~HttpParser()
    {
        if(fileContent_)
            delete fileContent_;
        fileContent_ = nullptr;
    }

    //处理请求
    bool process();


    bool hasData() const
    {
        if(input_.empty() && pos_ == 0)
            return true;
        return false;
    }

    //读取新数据
    void newInput(const std::string& input)
    {
        input_ = input;
        pos_ = 0;
    }

    void appendInput(const std::string& input)
    {
        input_.append(input);
    }



private:
    //主状态机
    HttpState processRead();
    //从状态机
    LineState parseLine();

    void init(bool clear);

    //发送Http响应报文
    void send();

    //解析请求行
    HttpState parseRequestLine(const char* start);
    //解析首部
    HttpState parseHeaders(const char* start);
    //解析主体，实际上并不解析，只判断是否完整
    HttpState parseContent();
    //已接受完整的Http请求
    HttpState doRequest();
    //处理请求，填充response_
    void processWrite();
    //在output_添加版本状态码
    void appendResponseLine();
    //在output_添加首部字段
    void appendHeaders();
    //在output_添加消息体
    void appendContent();

    //接受主体
    void recvBody(const char* start, const char* end);
    //设置方法
    bool setMethod(const std::string& method);
    //设置URI
    bool setURI(const std::string& uri);
    //设置版本
    bool setVersion(const std::string& version);
    //设置首部字段
    bool setHeader(const std::string& line);
    //获取请求的资源
    HttpState getResource();

    bool connecting_;   //是否保持连接
    size_t pos_;    //当前读取的位置
    CheckState checkState_; //主状态机状态
    HttpState state_;
    char* fileContent_;   //请求的文件
    off_t filelen_; //文件大小
    size_t contentLength_;
    std::string input_; //HTTP请求报文
    std::string output_;    //HTTP响应报文
    std::string fileName_;  //文件名
    HttpRequest request_;
    HttpResponse response_;
    WeakTcpConnectionPtr conn_; //tcp连接
};


#endif