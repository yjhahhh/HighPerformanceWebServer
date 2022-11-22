#ifndef _TIMESTAMP_
#define _TIMESTAMP_
#include<unistd.h>
#include<sys/time.h>
#include<string>
constexpr int MicroSecondsPerSecond = 1000 * 1000; //一秒的微妙数

class Timestamp
{
public:
    typedef long long ll;

    Timestamp()
        : microSecondsSinceEpoch_(0) { }
    Timestamp(ll microSecondsSinceEpoch)
        : microSecondsSinceEpoch_(microSecondsSinceEpoch) { }

    //提供一个无效的Timestamp
    static Timestamp invalid()
    {
        return Timestamp();
    }
    //提供当前时间的时间戳
    static Timestamp now();

    bool valid() const
    { 
        return microSecondsSinceEpoch_ > 0; 
    }

    //给Timer用
    static Timestamp addTime(const Timestamp& timeStamp, double seconds);
    
    ////获得的自Epoch时间以来的秒数，微秒数默认为0，转化为Timestamp类型对象
    static Timestamp fromUnixTime(time_t t, int microseconds = 0);

    void swap(Timestamp& ts);

    ll microSecondsSinceEpoch() const 
    { 
        return microSecondsSinceEpoch_;
    }; 
    time_t secondsSinceEpoch() const
    {
        return static_cast<time_t>(microSecondsSinceEpoch_ / MicroSecondsPerSecond);
    }
    //获取字符串格式时间戳
    std::string toString() const;
    //获得固定格式字符串时间戳
    std::string toFormattedString(bool showMicroseconds) const;

    bool operator<(const Timestamp& ts) const;
private:
     ll microSecondsSinceEpoch_;    //表示从 Epoch时间到目前为止的微妙数
};



#endif