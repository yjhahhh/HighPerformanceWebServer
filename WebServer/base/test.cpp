#include"Logging.h"
#include"Thread.h"
int main()
{
    for(int i = 0; i < 1000; ++i)
    {
        Logger(__FILE__, __LINE__).stream() << "我是 :" << i ;
    }
}