#ifndef _LOGFILE_
#define _LOGFILE_
#include"FileUtil.h"
#include<memory>
#include"MutexLock.h"
constexpr int ROLLSIZE = 1024 * 1024 * 1024;
constexpr int CHECKEVERY = 1024;
constexpr int RollPerSeconds = 60 * 60 * 24;
class LogFile
{
public:
    LogFile(const std::string& baseName, int rollSize = ROLLSIZE, int checkEveryN = CHECKEVERY, bool threadSfe = false);
    ~LogFile();
    //当日志文件接近指定的滚动限值时,需要换一个新文件写数据,便于后续归档、查看
    void rollFile();
    void flush();
    void append(const char* data, const size_t len);
    
private:
    void append_unlocked(const char* data, const size_t len);
    static std::string nextLogFileName(std::string basename, time_t* now);

    static bool threadSafe_;    //是否线程安全
    const int checkEveryN_;    //写数据次数限值，默认1024
    int count_;  //写数据次数
    const int rollSize_;    //滚动文件大小，默认1GB
    time_t startOfPeriod_;  //本次写日志的起始时间
    std::string baseName_;  //基础文件名, 用于新log文件命名
    std::unique_ptr<MutexLock> mutex_; //互斥锁
    std::unique_ptr<AppendFile> file_;
};
#endif
