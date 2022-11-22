#ifndef _POLLER_
#define _POLLER_
#include<vector>
#include"base/noncopyable.h"
#include<map>
#include"EventLoop.h"

class Channel;
class Timestamp;

class Poller : noncopyable
{
public:
    typedef std::vector<Channel*> ChannelList;

    explicit Poller(EventLoop* loop)
        : ownerLoop_(loop)
    {

    }
    virtual ~Poller()
    {

    }
    //返回调用完epoll_wait()的时间
    virtual Timestamp loop(int timeoutMs, ChannelList& activeChannels) = 0;
    //更新监听通道的事件
    virtual void updateChannel(Channel* channel) = 0;
    //删除监听通道
    virtual void removeChannel(Channel* channel) = 0;
    //判定当前Poller是否持有指定通道
    virtual bool hasChannel(Channel* channel) const = 0;
    //断言所属EventLoop为当前线程
    void assertInLoopTread() const
    {
        ownerLoop_->assertInLoopThread();
    }

    static Poller* newDefaultPoller(EventLoop* loop);
protected:
    //该类型保存fd和需要监听的events，以及各种事件回调函数
    typedef std::map<int, Channel*> ChannelMap;
    ChannelMap channels_;   //保存所有事件的Channel，一个Channel绑定一个fd

private:

    EventLoop* ownerLoop_;  //所属EventLoop

};

#endif