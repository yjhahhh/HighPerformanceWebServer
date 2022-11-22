#ifndef _EVENTLOOP_
#define _EVENTLOOP_
#include"base/noncopyable.h"
#include"base/Thread.h"
#include<atomic>
#include<any>
#include<vector>
#include"TimerId.h"


class Channel;
class Poller;
class TimerQueue;

class EventLoop : noncopyable
{
public:
    typedef std::function<void()> Functor;

    EventLoop();
    ~EventLoop();
    //loop循环, 运行一个死循环,必须在当前对象的创建线程中运行
    void loop();
    //退出loop循环
    void quit();
    //epoll_wait()返回事件
    Timestamp epollReturnTime() const;
    //在loop线程中, 立即运行回调cb,如果没在loop线程, 就会唤醒loop, (排队)运行回调cb.
    void runInLoop(const Functor& cb);
    //排队回调cb进loop线程,回调cb在loop中完成polling后运行
    void queueInLoop(const Functor& cb);
    //排队回调cb的个数
    size_t queueSize() const;
    //在指定时间点运行回调cb
    void runAt(const Timestamp& when, const Functor& timerCallback);
    //在当前时间点延后delay运行回调cb
    void runAfter(double delay, const Functor& timerCallback);
    //每隔interval sec周期执行回调
    void runEvery(double interval, const Functor& timerCallbak);
    //取消定时器
    void cancel(const TimerId& timerId);
    //唤醒loop线程
    void weakup();
    //更新Poller监听的channel, 只能在channel所属loop线程中调用
    void updateChannel(Channel* channel);
    //移除Poller监听的channel, 只能在channel所属loop线程中调用
    void removeChannel(Channel* channel);
    //判断Poller是否正在监听channel, 只能在channel所属loop线程中调用
    bool hasChannel(Channel* channel) const;

    //断言当前线程是创建当前对象的线程
    void assertInLoopThread() const;
    //判断前线程是否创建当前对象的线程
    bool isInLoopThread() const;
    //判断是否有待调用的回调函数
    bool callingPendingFunctor() const;
    //判断loop线程是否正在处理事件, 执行事件回调
    bool eventHandling() const;

    //获取当前线程的EventLoop对象指针
    static EventLoop* getEventLoopOfCurrentThread();

private:
    typedef std::vector<Channel*> ChannelList;

    //终止程序(LOG_FATAL), 当前线程不是创建当前EventLoop对象的线程时,由assertInLoopThread()调用
    void abortNotInLoopThread() const;
    //唤醒所属loop线程, 也是wakeupFd_的事件回调
    void handleRead();
    //打印激活通道的事件信息
    void printActiveChannels() const;
    //处理pending函数
    void doPendingFunctors();

    bool looping_;  //是否正在循环
    bool quit_; //是否退出循环状态
    bool eventHandling_;   //是否处于事件处理状态
    bool callingPendingFunctor_;   //是否处于调用pending函数的状态
    const pid_t threadId_;  //所属线程id
    int weakupFd_;
    Channel* currentActiveChannel_; //当前正在处理的活动通道
    int64_t iteration_; //loop迭代次数
    Timestamp epollReturnTime_;  //epoll_wait()返回时间点
    mutable MutexLock mutex_;   //保护pending函数队列
    std::unique_ptr<Poller> poller_;    //轮询器，用于监听事件
    std::unique_ptr<TimerQueue> timerQueue_;    //定时器队列
    std::unique_ptr<Channel> weakupChannel_;    //用于唤醒loop线程的Channel

    ChannelList activeChannels_;    //激活事件的通道列表
    std::vector<Functor> pendingFunctors_;  //待调用函数列表

};


#endif