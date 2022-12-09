#ifndef _TIMERQUEUE_
#define _TIMERQUEUE_
#include"base/noncopyable.h"
#include<vector>
#include<map>
#include<set>
#include"Timer.h"
#include"TimerId.h"
#include"Channel.h"

class EventLoop;

class TimerQueue : noncopyable
{
public:
    typedef Timer::TimerCallback TimerCallback;

    explicit TimerQueue(EventLoop* loop);
    ~TimerQueue();

    //增加一个定时器,是线程安全的，可以跨线程调用
    TimerId addTimer(const TimerCallback& cb, const Timestamp& when, double interval);
    //取消一个定时器
    void cancel(const TimerId& timerId);

private:
    typedef long long ll;
    typedef std::pair<Timestamp, Timer*> Entry;
    typedef std::set<Entry> TimerList;
    typedef std::map<Timer*, ll> ActiveTimerSet;

    //以下成员函数只可能在其所属的I/O线程中调用，因而不必加锁

    //在addTimer()中被调用，添加一个计时器
    void addTimerInLoop(Timer* timer);
    //在cancel()中被调用，取消一个计时器
    void cancelInLoop(const TimerId& timerId);
    //定时器事件到来的时候，可读事件产生，会回调handleRead()函数
    void handleRead();
    //返回所有超时的定时器列表
    std::vector<Entry> getExpired(const Timestamp& now);
    //重置超时的定时器
    void reset(const std::vector<Entry>& expired, const Timestamp& now);
    //插入定时器
    bool insert(Timer* timer);



    bool callingExpiredTimers_; //是否在处理超时的定时器
    EventLoop* loop_;   //所属的EventLoop
    const int timerfd_; //createTimerfd()所创建的定时器文件描述符
    Channel timerfdChannel_;    //定时器通道
    TimerList timers_;  //包含用户添加的所有Timer对象，按到期时间排序
    ActiveTimerSet cancelingTimers_;    //保存取消的定时器集合
};

#endif