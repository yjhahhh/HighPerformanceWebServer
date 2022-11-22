#ifndef _EVENTLOOPTHREAD_
#define _EVENTLOOPTHREAD_
#include"base/noncopyable.h"
#include<functional>
#include<string>
#include"base/Thread.h"
#include"base/CountDownLatch.h"

/*一个EventLoopThread对象对应一个IO线程，而IO线程函数负责创建局部EventLoop对象，并启动EventLoop的loop循环*/

class EventLoop;
class EventLoopThreadPool;

class EventLoopThread : noncopyable
{
public:
    typedef std::function<void(EventLoop*)> ThreadInitCallback;

    EventLoopThread(const ThreadInitCallback& cb, const std::string& name = "");
    ~EventLoopThread();

    //启动IO线程函数中的loop循环，返回IO线程中创建的EventLoop对象地址（栈上创建的）
    EventLoop* startLoop();

private:
    void threadFunc();  //IO线程函数

    friend class EventLoopThreadPool;

    bool exiting_;
    EventLoop* loop_;   //绑定的EventLoop
    Thread thread_;
    CountDownLatch latch_;
    ThreadInitCallback callback_;
};

#endif