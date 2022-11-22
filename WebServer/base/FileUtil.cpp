#include"FileUtil.h"
#include<stdio.h>
#include<unistd.h>
#include<assert.h>
using namespace std;

AppendFile::AppendFile(const std::string& name)
    : writtenBytes_(0), file_(fopen(name.c_str(), "a+"))
{
    assert(file_);
    //设置缓冲区
    setbuffer(file_, buf_, sizeof(buf_));
}

AppendFile::~AppendFile()
{
    if(file_)
        fclose(file_);
}

void AppendFile::append(const char* data, const size_t len)
{
    size_t size = write_(data, len);
    size_t remain = len - size;
    while(remain > 0)
    {
        size_t n = write_(data + size, remain);
        if(n == 0)
        {
            int ferr = ferror(file_);
            if(ferr)
            {
                writtenBytes_ += size;
                fprintf(stderr, "AppendFile::append() failed !\n");
                break;
            }
        }
        size += n;
        remain = len - size;
    }
    writtenBytes_ += size;
}

void AppendFile::flush()
{
    fflush(file_);
}

size_t AppendFile::write_(const void *ptr, size_t nmemb)
{
    return fwrite_unlocked(ptr, 1, nmemb, file_);
}