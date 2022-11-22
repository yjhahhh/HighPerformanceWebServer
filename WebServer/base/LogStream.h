#ifndef _LOGSTREAM_
#define _LOGSTREAM_
//LogStream主要用来格式化输出，重载了<<运算符，同时也有自己的一块缓冲区
#include<iostream>
#include"noncopyable.h"
#include<string.h>
constexpr int SmallBuffer = 4096;
constexpr int LargeBuffer = 4096 * 1024;

template <int SIZE>
class FixedBuffer : noncopyable
{
public:
    FixedBuffer()
        : buf_(new char[SIZE]), cur_(buf_){ }
    ~FixedBuffer()
    {
        if(buf_)
            delete buf_;
    }
    void append(const char* data, size_t len)
    {
        if(avail() > len)
        {
            memcpy(cur_, data, len);
            cur_ += len;
        }
    }
    const char* data() const
    {
        return buf_;
    }
    //缓冲区剩余空间
    size_t avail() const
    {
        return static_cast<size_t>(end() - cur_);
    }
    size_t length() const
    {
        return static_cast<size_t>(cur_ - buf_);
    }
    char* current()
    {
        return cur_;
    }
    //移动cur_指针
    void add(size_t n)
    {
        cur_ += n;
    }
    void reset()
    {
        cur_ = buf_;
    }
    void bzero()
    {
        memset(buf_, 0, SIZE);
        cur_ = buf_;
    }
private:
    //指向数组末尾 不能解引用end()
    const char* end() const
    {
        return buf_ + SIZE;
    }

    char* buf_;
    char* cur_;
};

class LogStream : noncopyable
{
public:
    typedef FixedBuffer<SmallBuffer> Buffer;
    
    LogStream& operator<<(bool);

    LogStream& operator<<(short);
    LogStream& operator<<(unsigned short);
    LogStream& operator<<(int);
    LogStream& operator<<(unsigned int);
    LogStream& operator<<(long);
    LogStream& operator<<(unsigned long);
    LogStream& operator<<(long long);
    LogStream& operator<<(unsigned long long);

    LogStream& operator<<(float);
    LogStream& operator<<(double);

    LogStream& operator<<(const void*);
    LogStream& operator<<(const char*);
    LogStream& operator<<(const std::string&);

    LogStream& operator<<(char);
    LogStream& operator<<(unsigned char);

    void append(const char* data, size_t len);
    const Buffer& buffer() const
    {
        return buffer_;
    }
    void resetBuffer()
    {
        buffer_.reset();
    }

private:
    //格式化输出整数
    template <typename T>
    void formatInteger(T);

    Buffer buffer_;

    static const int MaxNumericSize = 32;   //最大的数字长度
};

#endif