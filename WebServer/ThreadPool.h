#ifndef _THREADPOOL_
#define _THREADPOOL_

#include"base/noncopyable.h"
#include<string>
#include<functional>
#include<deque>
#include"base/Condition.h"
#include<memory>
#include"base/Thread.h"
#include<vector>
class ThreadPool : noncopyable
{
public:
    typedef std::function<void()> Task; //回调函数

    explicit ThreadPool(const std::string& name = "ThreadPool");
    ~ThreadPool();
    //创建指定个数线程并启动
    void start(size_t num);
    //停止各线程
    void stop();
    //设置任务队列最大任务个数
    void setMaxQueueSize(size_t size);
    //将任务task添加到线程中的任务队列queue_
    void run(const Task& task);
    
private:
    //线程执行函数
    void runInThread();
    //获取任务
    Task take();

    bool running_;  //线程池运行标志
    size_t maxQueueSize_;    //任务队列最大大小
    std::string name_;  //线程池名称
    mutable MutexLock mutex_;
    Condition notEmpty_;    //任务队列非空条件
    std::deque<Task> queue_;    //任务队列
    std::vector<std::unique_ptr<Thread>> pool_; //线程池 线程指针
};

#endif