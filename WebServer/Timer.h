#ifndef _TIMER_
#define _TIMER_
#include<functional>
#include"base/Timestamp.h"
#include<atomic>

class Timer 
{
public:
    typedef std::function<void()> TimerCallback;

    Timer(const TimerCallback& cb, const Timestamp& when, double interval);

    //执行回调函数
    void run();
    //重启定时器
    void restart(const Timestamp& now);
    //获取定时器序号
    int64_t sequence() const;
    //返回下一次超时时刻
    Timestamp expration() const;
    //返回是否是一次性定时器
    bool repeat() const;

    static int64_t numCreated();
private:
    const bool repeat_; //是否重复
    int64_t sequence_;  //定时器序号，全局唯一
    const double interval_; //超时时间间隔
    Timestamp expration_;   //下一次超时时刻
    TimerCallback cb_;  //超时回调函数
    static std::atomic<int64_t> numCreated_;    //已创建的定时器数
    
};

#endif