#ifndef _CONDITION_
#define _CONDITION_
#include"noncopyable.h"
#include"MutexLock.h"
#include"errno.h"
class Condition : noncopyable
{
public:
    explicit Condition(MutexLock& mutex)
        : mutex_(mutex)
    {
        pthread_cond_init(&cond_, nullptr);
    }
    ~Condition()
    {
        pthread_cond_destroy(&cond_);
    }
    void wait()
    {
        pthread_cond_wait(&cond_, &mutex_.mutex);
    }
    bool waitForSeconds(int seconds)
    {
        struct timespec abstime;
        clock_gettime(CLOCK_REALTIME, &abstime);
        abstime.tv_sec += static_cast<time_t>(seconds);
        return ETIMEDOUT == pthread_cond_timedwait(&cond_, &mutex_.mutex, &abstime);
    }
    void notify()
    {
        pthread_cond_signal(&cond_);
    }
    void notifyAll()
    {
        pthread_cond_broadcast(&cond_);
    }
private:
    MutexLock& mutex_;
    pthread_cond_t cond_;
};

#endif