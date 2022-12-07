#ifndef _EPOLLPOLLER_
#define _EPOLLPOLLER_
#include"Poller.h"
#include<vector>
#include<map>

class EpollPoller : public Poller
{
public:

    EpollPoller(EventLoop* loop);
    virtual ~EpollPoller();

    virtual Timestamp poll(int timeoutMs, ChannelList& activeChannels) override;
    virtual void updateChannel(Channel* channel) override;
    //在channels_中移除
    virtual void removeChannel(Channel* channel) override;

    virtual bool hasChannel(Channel* channel) const override;


private:
    typedef std::vector<struct epoll_event> EventList;
    typedef std::map<int, Channel*> ChannelMap;

    //poll()调用fillActiveChannels()，在activeChannels中放入numEvents个活动通道
    void fillActiveChanneds(int numEvents, ChannelList& activeChannels );
    //updateChannel()调用update()注册或者更新通道所关注的事件
    void update(int operation, Channel* channel);

    int epollfd_;
    EventList events_;  //事件列表
    ChannelMap channels_;   //关注的通道列表

    static const int InitEventListSize;    //事件列表初始大小为16，会自动增长

};

#endif