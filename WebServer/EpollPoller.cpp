#include"EpollPoller.h"
#include<sys/epoll.h>
#include"base/Logging.h"
#include"Channel.h"

using namespace std;

const int kNew = -1;
const int kAdded = 1;
const int kDeleted = 2;

const int EpollPoller::InitEventListSize = 16;

EpollPoller::EpollPoller(EventLoop* loop)
    : Poller(loop), epollfd_(epoll_create1(EPOLL_CLOEXEC)),
    events_(InitEventListSize)
{

}

Poller* Poller::newDefaultPoller(EventLoop* loop)
{
    return new EpollPoller(loop);
}

EpollPoller::~EpollPoller()
{
    close(epollfd_);
}

Timestamp EpollPoller::loop(int timeoutMs, ChannelList& activeChannels)
{
    int numEvents = epoll_wait(epollfd_, events_.data(), static_cast<int>(events_.size()), timeoutMs);
    Timestamp now = Timestamp::now();
    if(numEvents > 0)
    {
        LOG_TRACE << numEvents << " events happended ";
        fillActiveChanneds(numEvents, activeChannels);
        if(static_cast<size_t>(numEvents) == events_.size())
            events_.resize(events_.size() * 2);
    }
    else if(numEvents == 0)
    {
        LOG_TRACE << "nothing happended ";
    }
    else
    {
        LOG_SYSERR << "EPollPoller::poll()";
    }
    return now;
}

void EpollPoller::fillActiveChanneds(int numEvents, ChannelList& activeChannels)
{
    for(int i = 0; i < numEvents; ++i)
    {
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
    #ifndef NDEBUG
        int fd = channel->fd();
        ChannelMap::const_iterator it = channels_.find(fd);
        assert(it != channels_.end());
        assert(it->second == channel);
    #endif
        channel->setRevrnts(events_[i].events); //设置返回的事件
        activeChannels.push_back(channel);  //添加到活动通道队列
    }
}

void EpollPoller::updateChannel(Channel* channel)
{
    assertInLoopTread();
    LOG_TRACE << "fd = " << channel->fd() << " events = " << channel->events();
    int fd = channel->fd();
    int index = channel->index();
    if(kNew == index || kDeleted == index)
    {
        if(kNew == index)
        {
            assert(channels_.find(fd) == channels_.end());
            channels_[fd] = channel;
        }
        else
        {
            //已在epoll描述符表中删除的，但仍在channels_
            assert(channels_.find(fd) != channels_.end());
            assert(channels_[fd] == channel);
        }
        channel->set_index(kAdded); //已添加
        update(EPOLL_CTL_ADD, channel);
    }
    else
    {
        //更改
        assert(channels_.find(fd) != channels_.end());
        assert(channels_[fd] == channel);
        assert(kAdded == index);
        if(channel->isNoneEvent())
        {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);   //已在epoll描述符列表删除
        }    
        else
            update(EPOLL_CTL_MOD, channel);

    }
    
}

void EpollPoller::removeChannel(Channel* channel)
{
    assertInLoopTread();
    int fd = channel->fd();
    int index = channel->index();
    assert(channels_.find(fd) != channels_.end());
    assert(channels_[fd] == channel);
    //assert(channel->isNoneEvent());

    assert(kAdded == index || kDeleted == index);
    size_t res = channels_.erase(fd);
    assert(1 == res);

    if(kAdded == index)
    {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);

}

bool EpollPoller::hasChannel(Channel* Channel) const
{
    return channels_.find(Channel->fd()) != channels_.end();
}

void EpollPoller::update(int operation, Channel* channel)
{
    epoll_event event;
    event.events = channel->events();
    event.data.ptr = channel;
    int fd = channel->fd();
    if(epoll_ctl(epollfd_, operation, fd, &event) < 0)
    {
        if (operation == EPOLL_CTL_DEL)
        {
            LOG_SYSERR << "epoll_ctl op = " << operation << " fd = " << fd;
        }
        else
        {
            LOG_SYSFATAL << "epoll_ctl op = " << operation << " fd = " << fd;
        }
    }
}
