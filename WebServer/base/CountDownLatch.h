#ifndef _COUNTDOWNLATCH_
#define _COUNTDOWNLATCH_
#include"noncopyable.h"
#include"MutexLock.h"
#include"Condition.h"

//确保Thread中传进去的func真的启动了以后 外层的start才返回
class CountDownLatch : noncopyable
{
public:
    explicit CountDownLatch(int num)
        : count(num), mutex(), cond(mutex){ }
    //等待计数值变为0
    void wait();
    //计数值-1
    void countDown();
private:
    int count;
    mutable MutexLock mutex;
    Condition cond;
};

#endif