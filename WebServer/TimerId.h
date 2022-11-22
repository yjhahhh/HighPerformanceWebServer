#ifndef _TIMERID_
#define _TIMERID_
#include"Timer.h"
#include<memory>
#include"base/noncopyable.h"

class TimerId
{
public:
    TimerId(Timer* timer)
        : timer_(timer), sequence_(timer->sequence())
    {

    }
    friend class TimerQueue;
private:
    Timer* timer_;   //定时器
    int64_t sequence_;  //定时器编号
};

#endif