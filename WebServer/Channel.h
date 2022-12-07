#ifndef _CHANNEL_
#define _CHANNEL_
#include"base/noncopyable.h"
#include<functional>
#include"base/Timestamp.h"
#include<memory>
#include"base/Timestamp.h"

class EventLoop;

//一个Channel对象只能属于一个EventLoop对象
//一个EventLoop对象可以拥有多个Channel对象

class Channel : noncopyable
{
public:
    typedef std::function<void()> EventCallback;
    typedef std::function<void(Timestamp)> ReadEventCallback;

    Channel(EventLoop* loop, int fd);
    ~Channel();

    //事件到来时会调用handleEvent处理
    void handleEvent(Timestamp receiveTime);
    //回调函数注册，对相应的IO事件进行处理
    void setReadCallback(const ReadEventCallback& cb)
    {
        readCallback_ = cb;
    }
    void setWriteCallback(const EventCallback& cb)
    {
        writeCallback_ = cb;
    }
    void setCloseCallback(const EventCallback& cb)
    {
        closeCallBack_ = cb;
    }
    void setErrorCallback(const EventCallback& cb)
    {
        errorCallback_ = cb;
    }

    //绑定对象
    void tie(const std::weak_ptr<void>& obj);
    //返回Channel对应的文件描述符
    int fd() const 
    {
        return fd_;
    }
    //返回Channel注册的事件
    int events() const
    {
        return events_;
    }
    //设置返回的事件，由Poller调用
    void setRevrnts(int events)
    {
        revents_ = events;
    }
    //返回poll/epoll返回的事件的数目
    int revents() const { return revents_; }
    //判断有无事件发生
    bool isNoneEvent() const
    {
        return NoneEvent == events_;
    }
    //通道关注可读事件，调用update()把这个通道注册到EventLoop所持有的poller_对象中
    void enableReading()
    {
        events_ |= ReadEvent;
        update();
    }
    //通道不关注可读事件
    void disableReading()
    {
        events_ &= ~ReadEvent;
        update();
    }
    //通道关注可写事件
    void enableWriting()
    {
        events_ |= WriteEvent;
        update();
    }
    //通道不关注可写事件
    void disableWriting()
    {
        events_ &= ~WriteEvent;
        update();
    }
    //通道不关注所有事件
    void disableAll()
    {
        events_ = NoneEvent;
        update();
    }
    //判断通道是否有可读事件
    bool isReading() const
    {
        return ReadEvent & events_;
    }
    //判断通道是否有可写事件
    bool isWriting() const
    {
        return WriteEvent & events_;
    }
    //返回index_
    int index() const
    {
        return index_;
    }
    //令poll的事件数组中的序号index_=idx
    void set_index(int idx)
    { 
        index_ = idx; 
    }

    void doNotLogHup()
    { 
        logHup_ = false; 
    }
    //返回Channel所属的EventLoop，即loop_
    EventLoop* ownerLoop()
    { 
        return loop_;
    }
    //负责移除I/O的可读或可写等事件
    void remove();
    //把活动事件及fd转换为字符串
    std::string reventsToString() const;

private:
    //更新或注册IO可读事件或可写等事件
    void update();
    //handleEvent()会调用handleEventWithGuard()对事件进行处理
    void handleEventWithGuard(Timestamp receiveTime);

    bool logHup_;   //是否生成某些日志
    bool tied_; //tie_绑定对象的标志
    bool eventHandling_; //是否处于事件处理状态
    const int fd_;  //channel负责的文件描述符，但并不负责关闭它
    int events_;    //关注的事件
    int revents_;  //返回的事件
    int index_; //表示通道的状态
    EventLoop* loop_;   //所属EventLoop
    std::weak_ptr<void> tie_;   //绑定对象
    ReadEventCallback readCallback_;    //可读事件回调函数
    EventCallback writeCallback_;   //可写事件回调函数
    EventCallback closeCallBack_;   //关闭连接回调函数
    EventCallback errorCallback_;   //发生错误回调函数
    
    static const int NoneEvent;   //没有事件
    static const int ReadEvent;   //读事件
    static const int WriteEvent;    //写事件
};


#endif