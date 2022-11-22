#ifndef _NONCOPYABLE_
#define _NONCOPYABLE_

class noncopyable
{
protected:
    noncopyable() = default;
    ~noncopyable() = default;
    noncopyable(const noncopyable&) = delete;
    noncopyable operator=(const noncopyable&) = delete;
};

#endif