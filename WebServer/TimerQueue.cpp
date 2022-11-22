#include"TimerQueue.h"
#include<sys/timerfd.h>
#include"base/Logging.h"
#include"Channel.h"
#include"TimerId.h"
#include"EventLoop.h"

using namespace std;

int createTimerfd()
{
    int timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (timerfd < 0)
    {
        LOG_SYSFATAL << "Failed in timerfd_create";
    }
    return timerfd;
}

//计算超时时刻与当前时间的时间差
timespec howMuchTImeFromNow(const Timestamp& when)
{
    auto microseconds = when.microSecondsSinceEpoch() - Timestamp::now().microSecondsSinceEpoch();
    if(microseconds < 100)
        microseconds = 100;
    timespec ts;
    ts.tv_sec = static_cast<time_t>(microseconds / MicroSecondsPerSecond);
    ts.tv_nsec = static_cast<long>((microseconds % MicroSecondsPerSecond) * 1000);  //纳秒
    return ts;
}

//清除定时器，避免一直触发
void readTimerfd(int timerfd, const Timestamp& now)
{
    uint64_t howmany = 0;   //获取超时次数
    ssize_t size = read(timerfd, &howmany, sizeof(howmany));    
    LOG_TRACE << "TimerQueue::handleRead() " << howmany << " at " << now.toString();
    if (size != sizeof(howmany))
    {
        LOG_ERROR << "TimerQueue::handleRead() reads " << size << " bytes instead of " << sizeof(howmany);
    }
}

//重置定时器的超时时间
void resetTimerfd(int timerfd, const Timestamp& expiration)
{
    itimerspec newValue;
    itimerspec oldValue;
    bzero(&newValue, sizeof(newValue));
    bzero(&oldValue, sizeof(oldValue));
    newValue.it_value = howMuchTImeFromNow(expiration);
    int ret = timerfd_settime(timerfd, 0, &newValue, &oldValue);
    if (ret)
    {
        LOG_SYSERR << "timerfd_settime()";
    }

}

TimerQueue::TimerQueue(EventLoop* loop)
    : callingExpiredTimers_(false), loop_(loop),
    timerfd_(createTimerfd()),
    timerfdChannel_(loop, timerfd_)
{
    timerfdChannel_.setReadCallback(bind(&TimerQueue::handleRead, this));
    timerfdChannel_.enableReading();
}

TimerQueue::~TimerQueue()
{
    timerfdChannel_.disableAll();   //关闭所有(通道)事件, Poller不再监听该通道
    timerfdChannel_.remove();   //从激活的通道列表中移除
    close(timerfd_);
    for(auto& entry : timers_)
    {
        delete entry.second;
    }
}

TimerId TimerQueue::addTimer(const TimerCallback& cb, const Timestamp& when, double interval)
{
    Timer* timer = new Timer(cb, when, interval);
    addTimerInLoop(timer);
    return TimerId(timer);
}

void TimerQueue::addTimerInLoop(Timer* timer)
{
    loop_->assertInLoopThread();
    // 插入一个定时器，有可能会使得最早到期的定时器发生改变
    bool earliestChanged = insert(timer);
    if(earliestChanged)
    {
        resetTimerfd(timerfd_, timer->expration());
    }
}   

void TimerQueue::cancel(const TimerId& timerId)
{
    cancelInLoop(timerId);
}

void TimerQueue::cancelInLoop(const TimerId& timerId)
{
    loop_->assertInLoopThread();

    auto itor = timers_.find(timerId.timer_->expration());    //查找该dingshiq
    if(itor != timers_.end())
    {
        timers_.erase(itor);
        delete itor->second;
    }
    else if(callingExpiredTimers_)
    {
        //已到期，并且正在调用回调函数的定时器
        cancelingTimers_.insert(make_pair(timerId.timer_, timerId.sequence_));
    }

}

void TimerQueue::handleRead()
{
    loop_->assertInLoopThread();
    Timestamp now = Timestamp::now();
   
    readTimerfd(timerfd_, now); //清除该事件，避免一直触发

    vector<Entry> expired = getExpired(now);    //获取该时刻之前的所有定时器
    callingExpiredTimers_ = true;
    cancelingTimers_.clear();
    for(Entry& entry : expired)
    {
        entry.second->run();    //执行回调函数
    }
    callingExpiredTimers_ = false;

    reset(expired, now);    //重启计时器

}

vector<TimerQueue::Entry> TimerQueue::getExpired(const Timestamp& now)
{
    loop_->assertInLoopThread();
    auto itor = timers_.lower_bound(now);   //返回第一个不到期的定时器

    vector<Entry> expired(timers_.begin(), itor);
    timers_.erase(timers_.begin(), itor);   //在timers_中删除到期的定时器
    
    return expired;
    
}

void TimerQueue::reset(const std::vector<Entry>& expired, const Timestamp& now)
{
    bool earliestChanged = false;
    for(const Entry& timer : expired)
    {
        //如果是重复定时器且是未被取消的，则重置定时器
        if(timer.second->repeat() && cancelingTimers_.find(timer.second) == cancelingTimers_.end())
        {
            timer.second->restart(now);
            earliestChanged |= insert(timer.second);
        }
        else
        {
            //一次性定时器或者已被取消的定时器，不能重置
            delete timer.second;
        }
    }

    if(earliestChanged && !timers_.empty())
    {
        //获取下一次超时时间
        Timestamp nextExpired = timers_.begin()->first;
        if(nextExpired.valid())
            resetTimerfd(timerfd_, nextExpired);    //重置下一次超时时间
    }
}

bool TimerQueue::insert(Timer* timer)
{
    loop_->assertInLoopThread();

    //最早到期时间是否改变
    bool earliestChanged = false;
    if(timers_.empty() || timer->expration() < timers_.begin()->first)
    {
        earliestChanged = true;
    }
    //插入timers_列表
    timers_.emplace(timer->expration(), timer); 

    return earliestChanged;
}