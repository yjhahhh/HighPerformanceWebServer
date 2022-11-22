#include"Timestamp.h"
using namespace std;
Timestamp Timestamp::now()
{
    timeval tv;
    gettimeofday(&tv, nullptr);
    return Timestamp(static_cast<ll>(tv.tv_sec * MicroSecondsPerSecond + tv.tv_usec));
}

Timestamp Timestamp::addTime(const Timestamp& timeStamp, double seconds)
{
    ll delta = static_cast<ll>(seconds * MicroSecondsPerSecond);
    return Timestamp(timeStamp.microSecondsSinceEpoch() + delta);
}

Timestamp Timestamp::fromUnixTime(time_t t, int microseconds)
{
    return Timestamp(static_cast<ll>(t * MicroSecondsPerSecond + microseconds));
}

void Timestamp::swap(Timestamp& ts)
{
    std::swap(microSecondsSinceEpoch_, ts.microSecondsSinceEpoch_);
}

string Timestamp::toString() const
{
    char buf[32]{0};
    ll seconds = microSecondsSinceEpoch_ / MicroSecondsPerSecond;
    ll microseconds = microSecondsSinceEpoch_ % MicroSecondsPerSecond;
    sprintf(buf, "%lld.%06lld", seconds, microseconds);
    return buf;
}

string Timestamp::toFormattedString(bool showMicroseconds) const
{
    char buf[64]{0};
    time_t seconds = static_cast<time_t>(microSecondsSinceEpoch_ / MicroSecondsPerSecond);
    tm time;
    gmtime_r(&seconds, &time);  //转换为tm类型
    // 显示微妙
    if(showMicroseconds)
    {
        int microseconds = static_cast<int>(microSecondsSinceEpoch_ % MicroSecondsPerSecond);
        sprintf(buf, "%4d%02d%02d %02d:%02d:%02d.%06dZ",
        time.tm_year + 1990, time.tm_mon + 1, time.tm_mday,
        time.tm_hour, time.tm_min, time.tm_sec, microseconds);
    }
    else
    {
        sprintf(buf, "%4d%02d%02d %02d:%02d:%02d",
        time.tm_year + 1990, time.tm_mon + 1, time.tm_mday,
        time.tm_hour, time.tm_min, time.tm_sec);
    }
    return buf;
}

bool Timestamp::operator<(const Timestamp& ts) const
{
    return microSecondsSinceEpoch_ < ts.microSecondsSinceEpoch_;
}