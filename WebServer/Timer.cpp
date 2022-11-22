#include"Timer.h"

using namespace std;

atomic<int64_t> Timer::numCreated_;

Timer::Timer(const TimerCallback& cb, const Timestamp& when, double interval)
    : repeat_(interval > 0.0), sequence_(++numCreated_),
    interval_(interval), expration_(when), cb_(cb)
{

}

void Timer::run()
{
    cb_();
}

bool Timer::repeat() const
{
    return repeat_;
}

int64_t Timer::sequence() const
{
    return sequence_;
}

Timestamp Timer::expration() const
{
    return expration_;
}

void Timer::restart(const Timestamp& now)
{
    if(repeat_)
        expration_ = Timestamp::addTime(now, interval_);
    else
        expration_ = Timestamp::invalid();  //提供一个无用的时间戳
}

