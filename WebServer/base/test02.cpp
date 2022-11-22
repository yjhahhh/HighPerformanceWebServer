#include"AsyncLogging.h"
#include"Logging.h"
using namespace std;
AsyncLogging asyncLog("log_test", 1024 * 1024 * 1024);

void outputLog(const char* data, size_t len)
{
    asyncLog.append(data, len);
}

int main()
{
    
    Logger::setOutputFunc(outputLog);
    asyncLog.start();
    for(int i = 0; i < 3; ++i)
    {
        LOG_INFO << "我是 : " << i;
    }
}