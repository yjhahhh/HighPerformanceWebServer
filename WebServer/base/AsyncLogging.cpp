#include"AsyncLogging.h"
#include"LogFile.h"
#include"FileUtil.h"
#include"Timestamp.h"
using namespace std;


AsyncLogging::AsyncLogging(const std::string& basename, size_t rollSize, int flushInterval)
    : flushInterval_(flushInterval), rollSize_(rollSize),
    basename_(basename), running_(false), thread_(bind(&AsyncLogging::threadFunc, this), "Logging"),
    mutex_(), cond_(mutex_), latch_(1),
    curBuffer_(make_unique<Buffer>()), nextBuffer_(make_unique<Buffer>())
{
   
}

AsyncLogging::~AsyncLogging()
{
    if(running_)
        stop();
}

void AsyncLogging::start()
{
    assert(!running_);
    running_ = true;
    thread_.start();
    latch_.wait();  //调用线程等待线程函数通知
}

void AsyncLogging::stop()
{
    assert(running_);
    running_ = false;
    cond_.notify(); //唤醒后端线程
    thread_.join();
}

void AsyncLogging::append(const char* logline, size_t len)
{
    MutexLockGuard lock(mutex_);    //可能有多个前端线程调用
    //当前缓冲区还有空间则添加
    if(curBuffer_->avail() > len)
    {
        curBuffer_->append(logline, len);
    }
    else    //当前缓冲区已满，放入队列，改用下一块缓冲区
    {
        buffers_.push_back(move(curBuffer_));
        if(nextBuffer_)
        {
            curBuffer_ = move(nextBuffer_);
        }
        else
        {
            curBuffer_.reset(new Buffer);
            curBuffer_->bzero();
        }
        curBuffer_->append(logline, len);
        cond_.notify(); //唤醒后端线程
    }
}

void AsyncLogging::threadFunc()
{
    assert(running_);
    BufferPtr newBuffer1 = make_unique<Buffer>();
    BufferPtr newBuffer2 = make_unique<Buffer>();
    newBuffer1->bzero();
    newBuffer2->bzero();
    BufferVector buffersToWrite;
    LogFile logFile(basename_, rollSize_);
    latch_.countDown(); //通知调用线程 后端线程已启动
    while(running_)
    {
        assert(newBuffer1 && newBuffer1->length() == 0);
        assert(newBuffer2 && newBuffer2->length() == 0);
        assert(buffersToWrite.empty());

        {
            MutexLockGuard lock(mutex_);  //加锁
            if(buffers_.empty())
            {
                //暂时没太多数据，等待flushInterval_秒
                cond_.waitForSeconds(flushInterval_);
            }
            buffers_.push_back(move(curBuffer_));
            curBuffer_ = move(newBuffer1);  //把新的缓冲区交换给curBuffer_
            if(!nextBuffer_)
                nextBuffer_ = move(newBuffer2);
            buffersToWrite.swap(buffers_);  //交换等待队列的指针 交给后端线程处理buffersTowrite,避免前端线程阻塞在buffers_
        }

        //若上一轮loop堆积了超过128MB的日志消息，则丢弃
        if(buffersToWrite.size() > MaxBuffersToWriteSize)
        {
            char msg[256] {0};
            sprintf(msg, "Dropped log message at %s, %zd larger buffers\n",
                Timestamp::now().toFormattedString(true).c_str(),
                buffersToWrite.size() - 2);
            fputs(msg, stderr); //输出到标准错误
            logFile.append(msg, strlen(msg));   //输出到日志

            buffersToWrite.erase(buffersToWrite.begin() + 2, buffersToWrite.end()); //丢弃多余的日志消息
        }

        for(const BufferPtr& ptr : buffersToWrite)
        {
            logFile.append(ptr->data(), ptr->length()); //交给logFile处理
        }

        //回收两块缓冲区
        if(!newBuffer1)
        {
            newBuffer1 = move(buffersToWrite.back());
            newBuffer1->reset();
            buffersToWrite.pop_back();
        }
        if(!newBuffer2)
        {
            newBuffer2 = move(buffersToWrite.back());
            newBuffer2->reset();
        }
        buffersToWrite.clear(); //剩下的都是无用的缓冲区，直接clear即可

        logFile.flush();    //刷新缓冲
    }
    logFile.flush();
    
}
