#ifndef _ASYNCLOGGING_
#define _ASYNCLOGGING
//提供后端线程，定时将log缓冲写到磁盘，维护缓冲及缓冲队列
#include"noncopyable.h"
#include"LogStream.h"
#include<vector>
#include<memory>
#include<atomic>
#include"Thread.h"
#include"MutexLock.h"
#include"Condition.h"
#include"CountDownLatch.h"

constexpr int MaxBuffersToWriteSize = 32; //128MB为堆积门限 超过则丢弃

class AsyncLogging : noncopyable
{
public:

    AsyncLogging(const std::string& basename, size_t rollSize, int flushInterval = 3);
    ~AsyncLogging();
    //添加至缓冲
    void append(const char* logline, size_t len);
    
    /*线程函数
    构建1个LogFile对象，用于控制log文件创建、写日志数据，创建2个空闲缓冲区buffer1、buffer2，
    和一个待写缓冲队列buffersToWrite，分别用于替换当前缓冲currentBuffer_、空闲缓冲nextBuffer_、
    已满缓冲队列buffers_，避免在写文件过程中，锁住缓冲和队列，导致前端无法写数据到后端缓冲
    */
    void threadFunc();
    //主线程调用
    void start();

private:
    typedef FixedBuffer<LargeBuffer> Buffer;
    typedef std::vector<std::unique_ptr<Buffer>> BufferVector;
    typedef BufferVector::value_type BufferPtr;

    //析构函数调用
    void stop();

    int flushInterval_;  //多久刷新缓冲区 默认3秒
    const int rollSize_;    //日志文件滚动大小
    std::string basename_;  //日志文件的基本名称
    std::atomic<bool> running_; //后端线程loop是否运行
    Thread thread_; //后端线程
    MutexLock mutex_;   //互斥锁
    Condition cond_;    //条件变量
    CountDownLatch latch_;  //用于同步调用线程与后端线程
    BufferPtr curBuffer_;   //当前缓冲区
    BufferPtr nextBuffer_;  //下一个缓冲区
    BufferVector buffers_;  //已满缓冲队列

};

#endif