#include"EventLoopThreadPool.h"
#include"EventLoop.h"
using namespace std;

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop, const string& name)
    : started_(false), numThreads_(0), next_(0),
    baseLoop_(baseLoop), name_(name)
{

}

EventLoopThreadPool::~EventLoopThreadPool()
{

}

void EventLoopThreadPool::start(const ThreadInitCallback& cb)
{
    assert(!started_);
    baseLoop_->assertInLoopThread();

    started_ = true;

    //创建线程
    for(int i = 1; i <= numThreads_; ++i)
    {
        string name = name_ + to_string(i);
        threads_.emplace_back(make_unique<EventLoopThread>(cb, name));
        threads_.back()->startLoop();   //启动IO线程
    }
    if(numThreads_ == 0 && cb)
        cb(baseLoop_);

}

EventLoop* EventLoopThreadPool::getNextLoop()
{
    assert(started_);
    baseLoop_->assertInLoopThread();

    EventLoop* loop = baseLoop_;
    if(!threads_.empty())
    {
        loop = threads_[next_]->loop_;
        next_ = (++next_) % threads_.size();
    }
    return loop;
}