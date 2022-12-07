#ifndef _HTTPREQUEST_
#define _HTTPREQUEST_
#include<string>
#include<unordered_map>


class HttpRequest
{
public:
    typedef std::unordered_map<std::string, std::string> Header;

    enum Method
    {
        Invalid, GET, POST, HEAD
    };

    enum Version
    {
        Unknow, Http10, Http11
    };

    void setMethod(Method method)
    {
        method_ = method;
    }
    void setVersion(Version version)
    {
        version_ = version;
    }
    void setUri(const std::string& uri)
    {
        uri_.assign(uri);
    }
    void setBody(const char* start, const char* end)
    {
        body_.assign(start, end);
    }
    void addHeader(const std::string& key, const std::string& value)
    {
        headers_[key] = value;
    }
    const std::string& getURI() const
    {
        return uri_;
    }
    bool findHeader(const std::string& key)
    {
        return headers_.find(key) != headers_.end();
    }
    const std::string& getValue(const std::string& key)
    {
        return headers_[key];
    }
    Method getMethod()const
    {
        return method_;
    }
    void clear()
    {
        method_ = Invalid;
        version_ = Unknow;
        uri_.clear();
        body_.clear();
        headers_.clear();
    }

private:

    Method method_; //请求方法
    Version version_;   //http版本
    std::string uri_;   //请求路径
    std::string body_;  //请求体
    Header headers_;    //头部字段

};


#endif