#include"Thread.h"
#include<sys/prctl.h>
using namespace std;

__thread pid_t CurrentThread::t_cachedTid = 0;
__thread char CurrentThread::t_tidString[32] {0};
__thread const char* CurrentThread::t_threadName = "unknow";

Thread::Thread(ThreadFunc&& func, const string& name)
    : started_(false), joined_(false),
    pthreadId_(0), tid_(0),
    func_(func), name_(name), 
    latch_(1)
{
    setDefaultNmae();
}

Thread::~Thread()
{
    if(started_ && !joined_)
        pthread_detach(pthreadId_);
}

atomic<int> Thread::numCreated_(0);

void Thread::setDefaultNmae()
{
    int num = numCreated_ += 1;
    if(name_.empty())
    {
        name_ = "thread_" + to_string(num);
    }
}

int Thread::join()
{
    assert(started_);
    assert(!joined_);
    joined_ = true;
    return pthread_join(pthreadId_, nullptr);
}

void* startThread(void* arg);
void Thread::start()
{
    assert(!started_);
    started_ = true;
    ThreadData* data = new ThreadData(move(func_), name_, &tid_, &latch_);
    //成功返回0
    if(pthread_create(&pthreadId_, nullptr, startThread, data))
    {
        started_ = false;
        delete data;
        //记录日志
    }
    else
    {
        latch_.wait();  //主线程等待子线程初始化完毕
        assert(tid_ > 0);
    }
}

void ThreadData::runInThread()
{
    *tid_ = CurrentThread::tid();   //初始化tid
    tid_ = nullptr; //初始化完成，悬空指针
    latch_->countDown();    //通知主线程初始化完成
    latch_ = nullptr;

    CurrentThread::setName(name_.empty() ? "unknow" : name_.c_str());
    prctl(PR_SET_NAME, CurrentThread::name());    //设置调用进程的经常名字
    //执行传入的回调函数
    try
    {
        func_();
        CurrentThread::setName("finished");
    }
    catch (const std::exception& ex){
        CurrentThread::setName("crashed");
        fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
        fprintf(stderr, "reason: %s\n", ex.what());
        abort();
    }
    catch (...){
        CurrentThread::setName("crashed");
        fprintf(stderr, "unknown exception caught in Thread %s\n", name_.c_str());
        throw; 
    }
}


//传入pthread_create的回调函数
void* startThread(void* arg)
{
    auto data = static_cast<ThreadData*>(arg);
    data->runInThread();
    delete data;
    return nullptr;
}