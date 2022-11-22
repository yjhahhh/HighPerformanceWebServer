#include"Channel.h"
#include"EventLoop.h"
#include<sys/epoll.h>
#include<assert.h>
#include<poll.h>
#include"base/Logging.h"
using namespace std;

const int Channel::NoneEvent = 0;
const int Channel::ReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::WriteEvent = EPOLLOUT;

Channel::Channel(EventLoop* loop, int fd)
    : logHup_(true), tied_(false), eventHandling_(false),
    fd_(fd), events_(0), revents_(0), index_(-1),
    loop_(loop)
{

}

Channel::~Channel()
{
    assert(!eventHandling_);
}

//Poller中监听到激活事件的Channel后, 将其加入激活Channel列表
// EventLoop::loop根据激活Channel回调对应事件处理函数
void Channel::handleEvent(Timestamp receiveTime)
{
    if(tied_)
    {
        shared_ptr<void> guard = tie_.lock();   //将weak_ptr提升为share_ptr并引用计数加一,确保在执行事件处理动作时, 所需的对象不会被释放
        if(guard)
            handleEventWithGuard(receiveTime);
    }
    else
        handleEventWithGuard(receiveTime);
}

void Channel::handleEventWithGuard(Timestamp receiveTime)
{
    eventHandling_ = true;  //正在处理事件
    LOG_TRACE << reventsToString(); //日志输出fd及就绪事件
    if(revents_ & EPOLLHUP && !(revents_ & EPOLLIN))
    {
        //fd已断开连接且无数据可读
        if(logHup_)
            LOG_WARN << "fd = " << fd_ << " Channel::handleEvent() EPOLLHUP";
        if(closeCallBack_)
            closeCallBack_();   //调用关闭连接回调
    }
    if(revents_ & EPOLLERR)
    {
        //对读端或写端已关闭的连接进行读或写，关闭连接即可
        if(logHup_)
            LOG_WARN << "fd = " << fd_ << " Channel::handleEvent() EPOLLERR";
        if(closeCallBack_)
            closeCallBack_();
    }
    if(revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP))
    {
        if(readCallback_)
            readCallback_(receiveTime);
    }
    if(revents_ & EPOLLOUT)
    {
        if(writeCallback_)
            writeCallback_();
    }
    eventHandling_ = false;
}

void Channel::tie(const weak_ptr<void>& obj)
{
    tie_ = obj;
    tied_ = true;
}

string Channel::reventsToString() const
{
    string str(to_string(fd_));
    str += " : ";
    if(revents_ & EPOLLHUP)
        str += "EPOLLHUP ";
    if(revents_ & EPOLLERR)
        str += "EPOLLERR ";
    if(revents_ & EPOLLIN)
        str += "EPOLLIN ";
    if(revents_ & EPOLLPRI)
        str += "EPOLLPRI ";
    if(revents_ & EPOLLRDHUP)
        str += "EPOLLRDHUP ";
    if(revents_ & EPOLLOUT)
        str += "EPOLLOUT ";
    return str;
}

void Channel::remove()
{
    loop_->removeChannel(this);
}

void Channel::update()
{
    loop_->updateChannel(this);
}
