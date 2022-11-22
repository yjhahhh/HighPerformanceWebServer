#ifndef _STRINGARG_
#define _STRINGARG_
#include<string>

class StringArg
{
public:
    StringArg()
        : ptr(nullptr){ }
    StringArg(const char* str)
        : ptr(str){ }
    StringArg(const std::string& str)
        : ptr(str.data()){ }

    const char* c_str() const
    {
        return ptr;
    }

private:
    const char* ptr;
};

#endif