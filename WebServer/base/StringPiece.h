#ifndef _STRINGPIECE_
#define _STRINGPIECE_
#include<string.h>
#include<string>

class StringPiece
{
public:

    StringPiece()
        : ptr_(nullptr), length_(0) { }
    StringPiece(const char* str)
        : ptr_(str), length_(strlen(ptr_)) { }
    StringPiece(const unsigned char* str)
        : ptr_(reinterpret_cast<const char*>(str)),
        length_(strlen(ptr_)) { }
    StringPiece(const std::string& str)
        : ptr_(str.data()), length_(str.size()) { }
    StringPiece(const char* offset, size_t len)
        : ptr_(offset), length_(len) { }

    const char* data() const
    {
        return ptr_;
    }
    const char* end() const
    {
        return ptr_ + length_;
    }
    bool empty() const
    {
        return length_ == 0;
    }
    void clear()
    {
        ptr_ = nullptr;
        length_ = 0;
    }
    size_t size() const
    {
        return length_;
    }
    const char* begin() const
    {
        return ptr_;
    }
    void set(const char* buffer, size_t len)
    {
         ptr_ = buffer; length_ = len; 
    }
    void set(const char* str)
    {
        ptr_ = str;
        length_ = strlen(str);
    }
    void set(const void* buffer, size_t len) 
    {
        ptr_ = reinterpret_cast<const char*>(buffer);
        length_ = len;
    }
    char operator[](size_t i) const 
    { 
        return ptr_[i]; 
    }
    //移除ptr_的前n个字节
    void remove_prefix(size_t n) 
    {
        ptr_ += n;
        length_ -= n;
    }
    //移除ptr_的后n个字节
    void remove_suffix(size_t n) 
    {
        length_ -= n;
    }

    bool operator==(const StringPiece& x) const 
    {
        return ((length_ == x.length_) &&
                (memcmp(ptr_, x.ptr_, length_) == 0));
    }
    bool operator!=(const StringPiece& x) const {
        return !(*this == x);
    }


private:
    const char* ptr_;   //起始地址
    size_t length_;    //长度
};


#endif