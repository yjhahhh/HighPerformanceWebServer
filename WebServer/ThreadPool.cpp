#include"ThreadPool.h"

using namespace std;

ThreadPool::ThreadPool(const string& name)
    : running_(false), maxQueueSize_(0),
    name_(name), mutex_(), notEmpty_(mutex_)
{

}

ThreadPool::~ThreadPool()
{
    if(running_)
        stop();
}

void ThreadPool::start(size_t num)
{
    assert(!running_);
    assert(pool_.empty());
    running_ = true;
    pool_.reserve(num);

    for(size_t i = 0; i < num; ++i)
    {
        pool_.emplace_back(make_unique<Thread>(bind(&ThreadPool::runInThread, this), name_ + to_string(i)));  //Thread构造传的是右值引用
        pool_[i]->start();
    }
}

void ThreadPool::stop()
{
    {
        MutexLockGuard lock(mutex_);
        running_ = false;
        notEmpty_.notifyAll();
    }
    for(auto& thread : pool_)
        thread->join();
}

void ThreadPool::setMaxQueueSize(size_t size)
{
    maxQueueSize_ = size;
}

void ThreadPool::run(const Task& task)
{
    {
        MutexLockGuard lock(mutex_);
        if(queue_.size() < maxQueueSize_)
            queue_.push_back(task);
    }
    notEmpty_.notify();
}

ThreadPool::Task ThreadPool::take()
{
    while(queue_.empty() && running_)
    {
        notEmpty_.wait();   //等待run函数唤醒
    }
    Task task;
    {
        MutexLockGuard lock(mutex_);
        if(!queue_.empty())
        {
            task = queue_.front();
            queue_.pop_front();
        }
    }
    return task;
}

void ThreadPool::runInThread()
{
    try
    {
        while (running_)
        {
            //阻塞在此函数，消费者线程中的runInThread()函数调用take()获取任务task
            Task task(take());
            //任务task不为空，而执行任务task
            if (task)
            {
                task();
            }
        }
    }
    catch (const std::exception& ex)
    {
        fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
        fprintf(stderr, "reason: %s\n", ex.what());
        abort();
    }
    catch (...)
    {
        fprintf(stderr, "unknown exception caught in ThreadPool %s\n", name_.c_str());
        throw; // rethrow
    }
    
}