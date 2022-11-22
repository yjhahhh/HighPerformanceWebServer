#ifndef _EVENTLOOPTHREADPOOL_
#define _EVENTLOOPTHREADPOOL_
#include"base/noncopyable.h"
#include<functional>
#include<string>
#include<vector>
#include"EventLoopThread.h"
#include<memory>


class EventLoop;

class EventLoopThreadPool : noncopyable
{
public:
    typedef std::function<void(EventLoop*)> ThreadInitCallback;

    EventLoopThreadPool(EventLoop* baseLoop, const std::string& name = "");
    ~EventLoopThreadPool();

    //设置线程数量，在start()前调用
    void setThreadNum(int num)
    {
        numThreads_ = num;
    }
    //开启线程池
    void start(const ThreadInitCallback& cb = ThreadInitCallback());
    //轮询
    EventLoop* getNextLoop();

    //获取线程池启动状态
    bool started() const
    {
        return started_;
    }
    //获取线程池名称
    std::string name() const
    {
        return name_;
    }


private:
    bool started_;  //线程池是否开启
    int numThreads_;    //线程个数
    int next_;  //新连接到来时所选择的线程下标
    EventLoop* baseLoop_;   
    std::string name_;  //线程池名称
    std::vector<std::unique_ptr<EventLoopThread>> threads_; //IO线程列表

};


#endif