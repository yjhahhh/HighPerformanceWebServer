#include"MutexLock.h"
#include"Thread.h"

MutexLock::MutexLock()
    : holder(0)
{
    pthread_mutex_init(&mutex, nullptr);
}

MutexLock::~MutexLock()
{
    assert(0 == holder);
    pthread_mutex_destroy(&mutex);
}

void MutexLock::lock()
{
    pthread_mutex_lock(&mutex);
    assignHolder();
}

void MutexLock::unlock()
{
    unassignHolder();
    pthread_mutex_unlock(&mutex);
}

void MutexLock::assertLocked() const
{
    assert(isLockedByThisThread());
}

bool MutexLock::isLockedByThisThread() const
{
    return CurrentThread::tid() == holder;
}

void MutexLock::assignHolder()
{
    holder = CurrentThread::tid();
}