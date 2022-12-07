#include"http/HttpServer.h"
#include"base/AsyncLogging.h"
#include"EventLoop.h"
#include"base/Logging.h"

const char* LogBaseName = "/home/yjh/HighPerformanceWebServer/HighPerformanceWebServer/WebServer/logfiles/HttpServer";


int main(int argc, char* argv[])
{
    Logger::LogLevel logLevel = Logger::INFO;
    int threadNum = 8;
    int port = 8080;
    int idleSeconds = 8;
    const char* basename = nullptr;

    int opt;
    while((opt = getopt(argc, argv, "012345ht:p:s:l:")) != -1)
    {
        switch (opt)
        {
        case '0':
            logLevel = Logger::TRACE;
            break;
        case '1':
            logLevel = Logger::DEBUG;
            break;
        case '2':
            logLevel = Logger::INFO;
            break;
        case '3':
            logLevel = Logger::WARN;
            break;
        case '4':
            logLevel = Logger::ERROR;
            break;
        case '5':
            logLevel = Logger::FATAL;
            break;
        case 'h':
            std::cout << "-0            日志级别为TREAC\n";
            std::cout << "-1            日志级别为DEBUG\n";
            std::cout << "-2            日志级别为INFO\n";
            std::cout << "-3            日志级别为WARN\n";
            std::cout << "-4            日志级别为ERROR\n";
            std::cout << "-5            日志级别为FATAL\n";
            std::cout << "-h            help\n";
            std::cout << "-t [n]        线程数量\n";
            std::cout << "-p [n]        监听端口\n";
            std::cout << "-s [n]        连接最大空闲秒数\n";
            std::cout << "-l [path]     日志文件目录" << std::endl;
            exit(0);
        case 't':
            threadNum = atoi(optarg);
            break;
        case 'p':
            port = atoi(optarg);
            break;
        case 's':
            idleSeconds = atoi(optarg);
            break;
        case 'l':
            basename = optarg;
            break;
        default:
            break;
        }
    }

    Logger::setLevel(logLevel);
    if(!basename)
        basename = LogBaseName;
    AsyncLogging asyncLogging(basename, 1024 * 1024 *1024);
    auto outputLog = [&asyncLogging] (const char* data, size_t len) -> void { asyncLogging.append(data, len); };
    Logger::setOutputFunc(outputLog);   //设置异步日志输出
    asyncLogging.start();
    InetAddress addr(port); //http服务器绑定端口
    EventLoop mainLoop;
    HttpServer server(&mainLoop, addr, idleSeconds);
    server.setThreadNum(threadNum);
    server.start(); //开启监听
    mainLoop.loop();

}


