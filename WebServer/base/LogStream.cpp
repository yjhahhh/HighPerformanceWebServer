#include"LogStream.h"
#include<algorithm>
using namespace std;

const char digits[] = "9876543210123456789";
const char* zero = digits + 9;
const char digitsHex[] = "0123456789ABCDEF";

//把无符号整数转化为16进制字符串
size_t convertHex(char buf[], uintptr_t value)
{
  uintptr_t i = value;
  char* p = buf;

  do
  {
    int lsd = static_cast<int>(i % 16);
    i /= 16;
    *p++ = digitsHex[lsd];
  } while (i != 0);

  *p = '\0';
  std::reverse(buf, p);

  return p - buf;
}

//把带符号整数转换为字符串,返回长度
template <typename T>
size_t convert(char buf[], T value) 
{
    size_t loc = 0;
    do
    {
        int i = value % 10;
        value /= 10;
        buf[loc++] = zero[i];
    }  while(value > 0);
    if(value < 0)
        buf[loc++] = '-';
    buf[loc] = '\0';
    reverse(buf, buf + loc);
    return loc;
}

template <typename T>
void LogStream::formatInteger(T t)
{
    if(buffer_.avail() > MaxNumericSize)
    {
        size_t len = convert(buffer_.current(), t);
        buffer_.add(len);
    }
}

LogStream& LogStream::operator<<(bool v)
{
    buffer_.append(v ? "1" : "0", 1);
    return *this;
}

LogStream& LogStream::operator<<(short v)
{
    *this << static_cast<int>(v);
    return *this;
}
LogStream& LogStream::operator<<(unsigned short v)
{
    *this << static_cast<int>(v);
    return *this;
}
LogStream& LogStream::operator<<(int v)
{
    formatInteger(v);
    return *this;
}
LogStream& LogStream::operator<<(unsigned int v)
{
    formatInteger(v);
    return *this;
}
LogStream& LogStream::operator<<(long v)
{
    formatInteger(v);
    return *this;
}
LogStream& LogStream::operator<<(unsigned long v)
{
    formatInteger(v);
    return *this;
}
LogStream& LogStream::operator<<(long long v)
{
    formatInteger(v);
    return *this;
}
LogStream& LogStream::operator<<(unsigned long long v)
{
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<<(float v)
{
    *this << static_cast<double>(v);
    return *this;
}
LogStream& LogStream::operator<<(double v)
{
    if(buffer_.avail() > MaxNumericSize)
    {
        int len = snprintf(buffer_.current(), MaxNumericSize, "%.12g", v);
        buffer_.add(len);
    }
    return *this;
}

LogStream& LogStream::operator<<(const void* v)
{
    uintptr_t p = reinterpret_cast<const uintptr_t>(v);
    if(buffer_.avail() >= MaxNumericSize)
    {
        char* buf = buffer_.current();
        buf[0] = '0';
        buf[1] = 'x';
        size_t len = convertHex(buf + 2, p);
        buffer_.add(len + 2);
    }
    return *this;
}
LogStream& LogStream::operator<<(const char* v)
{
    if(v)
        buffer_.append(v, strlen(v));
    else
        buffer_.append("(null)", 6);
    return *this;
}
LogStream& LogStream::operator<<(const std::string& v)
{
    *this << v.c_str();
    return *this;
}

LogStream& LogStream::operator<<(char v)
{
    buffer_.append(&v, 1);
    return *this;
}
LogStream& LogStream::operator<<(unsigned char v)
{
    *this << static_cast<char>(v);
    return *this;
}