#include"Logging.h"
#include"Thread.h"
using namespace std;


__thread char t_errnobuf[512];
const char* strerror_tl(int saveErrno)
{
    return strerror_r(saveErrno, t_errnobuf, sizeof(t_errnobuf));
}

const char* logLevelName[] = {
  "TRACE ",
  "DEBUG ",
  "INFO  ",
  "WARN  ",
  "ERROR ",
  "FATAL ",
};

void defaultOutput(const char* data, size_t len)
{
    fwrite(data, 1, len, stdout);
}
void defaultFlush()
{
    fflush(stdout);
}

Logger::OutputFunc Logger::g_output = defaultOutput;
Logger::FlushFunc Logger::g_flush = defaultFlush;
Logger::LogLevel Logger::logLevel_ = Logger::INFO;

Logger::Logger(const char* file, size_t line, LogLevel level, const char* funcname)
    : impl_(level, 0, file, line)
{
    impl_.stream_ << funcname << ' ';
}

Logger::Logger(const char* file, size_t line, bool toAbort)
    : impl_(toAbort ? FATAL : ERROR, errno, file, line)
{
}

Logger::Logger(const char* file, size_t line)
    : impl_(INFO, 0, file, line)
{
}

Logger::Logger(const char* file, size_t line, LogLevel level)
    : impl_(level, 0, file, line)
{
}

Logger::~Logger()
{
    impl_.finish(); //加上日志尾
    const LogStream::Buffer& buf = impl_.stream_.buffer();
    g_output(buf.data(), buf.length());
    if(impl_.level_ == FATAL)
    {
        g_flush();  //发生错误关闭程序前 要刷新缓冲区
        abort();
    }
}

//消息格式：20221031 09:15:44.681220Z  4013 INFO  正文 - Logging_test.cpp:10
Logger::Impl::Impl(LogLevel level, int savedErrno, const char* file, int line)
    : file_(file), line_(line), level_(level), 
    time_(Timestamp::now()), stream_()
{
    formatTime();   //log消息头
    CurrentThread::tid();
    stream_ << CurrentThread::tidString() << ' ' << logLevelName[level_] << ' ';
    //如果有错误添加错误信息
    if(savedErrno)
    {
        stream_ << strerror_tl(savedErrno) << " (errno = " << savedErrno << ") ";
    }

}

void Logger::Impl::formatTime()
{
    stream_ << time_.toFormattedString(true) << ' ';
}
void Logger::Impl::finish()
{
    stream_ << " - " << file_ << '.' << line_ << '\n';
}