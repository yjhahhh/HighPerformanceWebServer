#ifndef _THREAD_
#define _THREAD_
#include<pthread.h>
#include<unistd.h>
#include<sys/syscall.h>
#include<assert.h>
#include<stdio.h>
#include"noncopyable.h"
#include<functional>
#include<memory>
#include"CountDownLatch.h"
#include<atomic>
class CurrentThread
{
public:
    //获取当前进程的tid
    static pid_t tid()
    {
        if(t_cachedTid == 0)
            cacheTid();
        return t_cachedTid;
    }

    static const char* tidString()
    {
        return t_tidString;
    }
    
    static const char* name()
    {
        return t_threadName;
    }

    static void setName(const char* name)
    {
        t_threadName = name;
    }

    static void cacheTid()
    {
        t_cachedTid = static_cast<pid_t>(syscall(SYS_gettid));
        snprintf(t_tidString, sizeof(t_tidString), "%5d", t_cachedTid);
    }
private:
    static __thread pid_t t_cachedTid;    //每个线程都有一份实体
    static __thread char t_tidString[32];   //线程id预先格式化为字符串
    static __thread const char* t_threadName; //线程名称
};


class Thread : noncopyable
{
public:
    typedef std::function<void()> ThreadFunc;
    Thread(ThreadFunc&& func, const std::string& name = "");
    ~Thread();
    //将数据封装在ThreadData类，执行pthread_create创建线程。实际执行过程在ThreadData类的runInThread()成员函数中
    void start();
    int join();

    bool started() const
    {
        return started_;
    }
    std::string name() const 
    {
        return name_;
    }
    static int numCreated()
    {
        return numCreated_.load();
    }

private:
    bool started_;  //线程是否开始运行
    bool joined_;   //线程是否可join
    pthread_t pthreadId_;   //线程变量
    pid_t tid_; //线程id
    ThreadFunc func_;   //线程回调函数
    std::string name_;     //线程名称
    CountDownLatch latch_;  // 用于等待线程函数执行完毕
    static std::atomic<int> numCreated_; // 原子操作，当前已经创建线程的数量

    void setDefaultNmae();
};

class ThreadData : noncopyable
{
public:
    typedef Thread::ThreadFunc ThreadFunc;
    ThreadData(ThreadFunc&& func, const std::string& name, pid_t* tid, CountDownLatch* latch)
        : tid_(tid), latch_(latch), func_(func), name_(name) {}
        
    void runInThread();
private:
    pid_t* tid_;
    CountDownLatch* latch_;
    ThreadFunc func_;
    std::string name_;
};

#endif