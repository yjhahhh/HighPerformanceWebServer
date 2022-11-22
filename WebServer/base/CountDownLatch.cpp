#include"CountDownLatch.h"

void CountDownLatch::wait()
{
    MutexLockGuard lock(mutex);
    while(count > 0)
        cond.wait();
}

void CountDownLatch::countDown()
{
    MutexLockGuard lock(mutex);
    --count;
    if(count == 0)
        cond.notifyAll();
}