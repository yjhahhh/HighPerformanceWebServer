#ifndef _HTTPRESPONSE_
#define _HTTPRESPONSE_
#include"HttpRequest.h"


class HttpResponse
{
public:
    typedef HttpRequest::Version Version;
    typedef HttpRequest::Header Header;

    //状态码
    enum StateCode
    {
        OK200,
        BadRequest400,
        NotFound404,
        InternalServerError500
    };

    void setStateCode(StateCode stateCode)
    {
        stateCode_ = stateCode;
    }

    void setVersion(Version version)
    {
        version_ = version;
    }
    
    void setBody(const char* file)
    {
        body_.assign(file);
    }

    Version getVersion() const
    {
        return version_;
    }

    void clear()
    {
        version_ = Version::Unknow;
        stateCode_ = BadRequest400;
        body_.clear();
    }

private:

    Version version_;   //http版本
    StateCode stateCode_;   //状态码
    std::string body_;  //请求体
};

#endif