#include"EventLoop.h"
#include<sys/eventfd.h>
#include"Poller.h"
#include"base/Logging.h"
#include"Channel.h"
#include"TimerQueue.h"

using namespace std;

__thread EventLoop* t_loopInThisThread = 0;
const int PollTImeMs = 10000;

static int createEventfd()
{
  int evtfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  if (evtfd < 0)
  {
    LOG_SYSERR << "Failed in eventfd";
    abort();
  }
  return evtfd;
}

EventLoop::EventLoop()
    : looping_(false), quit_(false),
    eventHandling_(false), callingPendingFunctor_(false),
    threadId_(CurrentThread::tid()), weakupFd_(createEventfd()),
    currentActiveChannel_(nullptr), iteration_(0),
    poller_(Poller::newDefaultPoller(this)),  timerQueue_(make_unique<TimerQueue>(this)),
    weakupChannel_(make_unique<Channel>(this, weakupFd_))
{
    LOG_TRACE << "EventLoop created " << this << " in thread " << threadId_;    //记录日志
    if(t_loopInThisThread)
    {
        LOG_FATAL << "Another EventLoop " << t_loopInThisThread << " exists in this thread " << threadId_;
    }
    else
        t_loopInThisThread = this;
    weakupChannel_->setReadCallback(bind(&EventLoop::handleRead, this));
    weakupChannel_->enableReading();

}

EventLoop::~EventLoop()
{
    LOG_DEBUG << "EventLoop " << this << " of thread " << threadId_ << " destructs in thread " << CurrentThread::tid();
    close(weakupFd_);
    t_loopInThisThread = NULL;
}

void EventLoop::loop()
{
    assert(!looping_);
    assertInLoopThread();
    looping_ = true;
    quit_ = false;
    LOG_TRACE << "EventLoop " << this << " start looping ";
    while(!quit_)
    {
        activeChannels_.clear();    //清除激活事件的通道列表
        epollReturnTime_ = poller_->loop(PollTImeMs, activeChannels_);
        if(Logger::logLevel_ <= Logger::TRACE)
        {
            printActiveChannels();
        }
        eventHandling_ = true;
        for(Channel* channel : activeChannels_)
        {
            currentActiveChannel_ = channel;
            channel->handleEvent(epollReturnTime_); //
        }
        currentActiveChannel_ = nullptr;
        eventHandling_ = false;

        doPendingFunctors();    //处理pending函数，由其他线程请求的用户任务
    }

    LOG_TRACE << "EventLoop " << this << " stop looping ";
    looping_ = false;
}

void EventLoop::quit()
{
    quit_ = true;
    if(!isInLoopThread())
        weakup();
}

Timestamp EventLoop::epollReturnTime() const
{
    return epollReturnTime_;
}

void EventLoop::runInLoop(const Functor& cb)
{
    if(isInLoopThread())
        cb();
    else
        queueInLoop(cb);
}

void EventLoop::queueInLoop(const Functor& cb)
{
    {
        MutexLockGuard lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }
    if(!isInLoopThread() || callingPendingFunctor_)
        weakup();
}

size_t EventLoop::queueSize() const
{
    return pendingFunctors_.size();
}

void EventLoop::runAt(const Timestamp& when, const Functor& cb)
{
    timerQueue_->addTimer(cb, when, 0);
}

void EventLoop::runAfter(double delay, const Functor& cb)
{

    timerQueue_->addTimer(cb, Timestamp::addTime(Timestamp::now(), delay), 0);
}

void EventLoop::runEvery(double interval, const Functor& cb)
{
    timerQueue_->addTimer(cb, Timestamp::addTime(Timestamp::now(), interval), interval);
}

void EventLoop::cancel(const TimerId& timerId)
{
    timerQueue_->cancel(timerId);
}

void EventLoop::weakup()
{
    uint64_t one = 1;
    ssize_t size = write(weakupFd_, &one, sizeof(one));
    if(size != sizeof(one))
        LOG_ERROR << "EventLoop::weakuo() writes " << size << "bytes instead of " << sizeof(one);
}

void EventLoop::updateChannel(Channel* channel)
{
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel)
{
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel* channel) const
{
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    return poller_->hasChannel(channel);

}

void EventLoop::assertInLoopThread() const
{
    assert(CurrentThread::tid() == threadId_);
}

bool EventLoop::isInLoopThread() const
{
    return CurrentThread::tid() == threadId_;
}

bool EventLoop::callingPendingFunctor() const
{
    return callingPendingFunctor_;
}

bool EventLoop::eventHandling() const
{
    return eventHandling_;
}

EventLoop* EventLoop::getEventLoopOfCurrentThread()
{
    return t_loopInThisThread;
}

void EventLoop::abortNotInLoopThread() const
{
    LOG_FATAL << "EventLoop::abortNotInLoopThread - EventLoop " << this
            << " was created in threadId_ = " << threadId_
            << ", current thread id = " <<  CurrentThread::tid();
}

void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t size = read(weakupFd_, &one, sizeof(one));
    if(size != sizeof(one))
    {
        LOG_ERROR << "EventLoop::handleread() reads " << size << " bytes instead of " << sizeof(one);
    }
}

void EventLoop::printActiveChannels() const
{
    for(const Channel* channel : activeChannels_)
    {
        LOG_TRACE << "{ " << channel->reventsToString() << " }";
    }
}

void EventLoop::doPendingFunctors()
{
    vector<Functor> list;
    callingPendingFunctor_ = true;
    {
        MutexLockGuard lock(mutex_);
        list.swap(pendingFunctors_);
    }
    for(const Functor& cb : list)
    {
        cb();
    }
    callingPendingFunctor_ = false;

}