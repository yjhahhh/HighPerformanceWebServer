#include"HttpServer.h"
#include"base/AsyncLogging.h"
#include"EventLoop.h"
#include"base/Logging.h"

#define LogBaseName "/home/yjh/HighPerformanceWebServer/WebServer/logfiles/HttpServer"

AsyncLogging asyncLogging(LogBaseName, 1024*1024*1024);

void outputLog(const char* data, size_t len)
{
    asyncLogging.append(data, len);
}


int main(int argc, char* argv[])
{
    Logger::setOutputFunc(outputLog);   //设置异步日志输出
    InetAddress addr(80);   //http服务器绑定80端口
    EventLoop mainLoop;
    HttpServer server(&mainLoop, addr);
    server.setThreadNum(8);
    server.start(); //开启监听
    mainLoop.loop();


}


