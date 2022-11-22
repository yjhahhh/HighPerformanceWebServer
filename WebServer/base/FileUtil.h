#ifndef _FILEUTIL_
#define _FILEUTIL_

#include<string>
#include"noncopyable.h"
class AppendFile : noncopyable
{
public:
    AppendFile()
        : file_(nullptr)
    { }
    explicit AppendFile(const std::string& name);
    ~AppendFile();
    //向文件写
    void append(const char* data, const size_t len);
    //刷新缓冲区
    void flush();
    size_t writtenBytes() const
    {
        return writtenBytes_;
    }
private:
    size_t write_(const void *ptr, size_t nmemb);
    size_t writtenBytes_;    //当前日志文件已写多少字节
    FILE* file_;
    char buf_[1024 * 64];    //缓冲区
};
#endif