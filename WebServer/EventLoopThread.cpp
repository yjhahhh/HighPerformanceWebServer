#include"EventLoopThread.h"
#include"EventLoop.h"

using namespace std;

EventLoopThread::EventLoopThread(const ThreadInitCallback& cb, const string& name)
    : exiting_(false), loop_(nullptr),
    thread_(bind(&EventLoopThread::threadFunc, this), name), latch_(1), callback_(cb)
{

}

EventLoopThread::~EventLoopThread()
{
    exiting_ = true;

    if(loop_)
    {
        loop_->quit();
        thread_.join();
    }
}

EventLoop* EventLoopThread::startLoop()
{
    assert(!thread_.started());
    thread_.start();

    latch_.wait();

    return loop_;

}

void EventLoopThread::threadFunc()
{
    EventLoop loop;
    if(callback_)
        callback_(&loop);

    loop_ = &loop;
    latch_.countDown(); //通知startLoop()

    loop.loop();    //开始循环

    loop_ = nullptr;

}