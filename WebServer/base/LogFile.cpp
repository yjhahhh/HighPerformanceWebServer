#include"LogFile.h"
#include<assert.h>
#include<unistd.h>
#include<string.h>
using namespace std;

bool LogFile::threadSafe_ = false;

LogFile::LogFile(const string& baseName, int rollSize, int checkEveryN, bool threadSafe)
    : checkEveryN_(checkEveryN), count_(0),
    rollSize_(rollSize), baseName_(baseName),
    mutex_(threadSafe ? new MutexLock : nullptr),
    file_(make_unique<AppendFile>())
{
    if(!threadSafe_)
        threadSafe_ = threadSafe;
    
    rollFile(); //这里面初始化file_
}

LogFile::~LogFile()
{
    if(threadSafe_)
    {
        MutexLockGuard lock(*mutex_);
        
    }
    else if(count_ > 0)
        flush();

}

void LogFile::append(const char* data, const size_t len)
{
    if(threadSafe_)
    {
        MutexLockGuard lock(*mutex_);
        append_unlocked(data, len);
    }
    else
        append_unlocked(data, len);
}

void LogFile::flush()
{
    if(threadSafe_)
    {
        MutexLockGuard lock(*mutex_);
        file_->flush();
    }
    else
        file_->flush();
}

void LogFile::append_unlocked(const char* data, const size_t len)
{
    time_t now = time(nullptr); //获取当前时间
    if(now / RollPerSeconds != startOfPeriod_ / RollPerSeconds)
    {
        count_ = 0;
        file_->flush();
        rollFile();
    }
    file_->append(data, len);
    if(checkEveryN_ <= ++count_ || file_->writtenBytes() >= rollSize_)
    {    
        count_ = 0;
        file_->flush();
        //日志文件超过滚动限制
        if(file_->writtenBytes() >= rollSize_)
        {
            rollFile();
        }
    }
}

void LogFile::rollFile()
{
    string filename = nextLogFileName(baseName_, &startOfPeriod_); //获得日志文件名并更新写日志时间
    file_.reset(new AppendFile(filename));  //重置file_并创建新文件

}

string getHostName()
{
    char buf[256];
    memset(buf, 0, sizeof(buf));
    if (gethostname(buf, sizeof(buf)) == 0)
    {
        buf[sizeof(buf)-1] = '\0';
        return buf;
    }
    else
    {
        return "unknownhost";
    }
}

string LogFile::nextLogFileName(string baseName, time_t* now)
{
    //日志名格式：basename + now + hostname + pid + ".log"
    string filename;
    filename.reserve(baseName.size() + 64);
    filename = baseName;
    
    char timebuf[32];
    memset(timebuf, 0, sizeof(timebuf));
    tm tmbuf;
    *now = time(nullptr);
    gmtime_r(now, &tmbuf);
    strftime(timebuf, sizeof(timebuf), ".%Y%M%d-%T.", &tmbuf);
    filename += timebuf;

    filename += getHostName();

    char pidbuf[32];
    memset(pidbuf, 0, sizeof(pidbuf));
    snprintf(pidbuf, sizeof(pidbuf), ".%d", getpid());
    filename += pidbuf;
    filename += ".log";

    return filename;

}