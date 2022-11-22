#ifndef _LOGGING_
#define _LOGGING_
#include"noncopyable.h"
#include"LogStream.h"
#include"Timestamp.h"
#include<unistd.h>




//提供给前端的接口
class Logger : noncopyable
{
public:
    typedef void (*OutputFunc)(const char*, size_t);
    typedef void (*FlushFunc)();
    enum LogLevel
    {
        TRACE = 0,  //指出比DEBUG粒度更细的一些信息事件（开发过程中使用）
        DEBUG,  //指出细粒度信息事件对调试应用程序是非常有帮助的（开发过程中使用）
        INFO,   //表明消息在粗粒度级别上突出强调应用程序的运行过程
        WARN,   //系统能正常运行，但可能会出现潜在错误的情形
        ERROR,  //指出虽然发生错误事件，但仍然不影响系统的继续运行
        FATAL,  //指出每个严重的错误事件将会导致应用程序的退出
        NUM_LOG_LEVELS
    };
    static LogLevel logLevel_;  //日志级别

    static OutputFunc g_output; //输出位置
    static FlushFunc g_flush;   //刷新方式

    static void setOutputFunc(OutputFunc func)
    {
        g_output = func;
    }
    static void setFlushFunc(FlushFunc func)
    {
        g_flush = func;
    }

    static LogLevel getLevel()
    {
        return logLevel_;
    }
    static void setLevel(LogLevel level)
    {
        logLevel_ = level;
    }
    
    Logger(const char* file, size_t line, LogLevel level, const char* funcname);
    Logger(const char* file, size_t line, bool toAbort);
    Logger(const char* file, size_t line);
    Logger(const char* file, size_t line, LogLevel level);
    ~Logger();
    LogStream& stream()
    {
        return impl_.stream_;
    }


private:
    class Impl
    {
    public:
        typedef Logger::LogLevel LogLevel;
        friend class Logger;

        Impl(LogLevel level, int savedErrno, const char* file, int line);

    private:
        //格式化时间字符串，也是一条log消息的开头
        void formatTime();
        //一条log消息的结尾 
        void finish();

        const char* file_;  //当前代码所在文件名
        size_t line_;   //行号
        LogLevel level_;    //日志等级
        Timestamp time_;    //当前时间
        LogStream stream_;
    };

    Impl impl_;
};

#define LOG_TRACE if(Logger::logLevel_ <= Logger::LogLevel::TRACE) \
    Logger(__FILE__, __LINE__, Logger::LogLevel::TRACE, __func__).stream()
#define LOG_DEBUG if(Logger::logLevel_ <= Logger::LogLevel::DEBUG) \
    Logger(__FILE__, __LINE__, Logger::LogLevel::DEBUG, __func__).stream()
#define LOG_INFO if(Logger::logLevel_ <= Logger::LogLevel::INFO) \
    Logger(__FILE__, __LINE__).stream()
#define LOG_WARN Logger(__FILE__, __LINE__, Logger::LogLevel::WARN).stream()
#define LOG_ERROR Logger(__FILE__, __LINE__, Logger::LogLevel::ERROR).stream()
#define LOG_FATAL Logger(__FILE__, __LINE__, Logger::LogLevel::FATAL).stream()
#define LOG_SYSERR Logger(__FILE__, __LINE__, false).stream()
#define LOG_SYSFATAL Logger(__FILE__, __LINE__, true).stream()
#endif