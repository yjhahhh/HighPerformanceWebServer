#ifndef _MUTEXLOCK_
#define _MUTEXLOCK_

#include"noncopyable.h"
#include<pthread.h>
class CurrentThread;

class MutexLock
{
public:
    MutexLock();
    ~MutexLock();
    
    void lock();
    
    void unlock();
    
    bool isLockedByThisThread() const;
    void assertLocked() const;
    
    pthread_mutex_t* getMutex()
    {
        return &mutex;
    }

private:
    pid_t holder;
    pthread_mutex_t mutex;

    void assignHolder();
    void unassignHolder()
    {
        holder = 0;
    }
    friend class Condition;
};

class MutexLockGuard : noncopyable
{
public:
    MutexLockGuard(MutexLock& mutex)
        : mutex_(mutex)
    {
        mutex_.lock();
    }
    ~MutexLockGuard()
    {
        mutex_.unlock();
    }
private:
    MutexLock& mutex_;
};
#define MutexLockGuard(x) static_assert(fasle, "missing mutex guard var name")
#endif