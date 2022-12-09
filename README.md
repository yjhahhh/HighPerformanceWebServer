# A C++ High Performance Web Server
## 概述   
该WebServer基于muoduo网络库实现，在实现细节上很大程度上学习了陈硕的muduo网络库，同样遵循了one loop pre thread 的思想。   
本项目为C++编写的多线程Web服务器，能解析简单的GET/POST请求，支持HTTP长连接，支持管线化请求，能踢掉空闲的连接，实现了高性能的异步日志功能。   
> |Ⅰ|Ⅱ|Ⅲ|Ⅳ|Ⅴ|Ⅵ|
> |:--:|:--:|:--:|:--:|:--:|:--:|
> |[并发模型]()|[事件驱动模型]()|[TCP连接的维护]()|[应用层缓冲Buffer模型]()|[日志模型]()|[压力测试]()|

## 环境   
*Ubuntu 18.04.06
*gcc 7.5.0

## 使用
    ./server [-(0-5)(日志级别)] [-h (使用帮助)] [-t threadNum] [-p port] [-s seconds] [-l logfile_path]

## Technical points
*使用epoll水平触发的IO多路复用技术，非阻塞式IO，使用Reactor模式
*基于多线程，并使用线程池技术，避免程序运行过程中频繁创建销毁的开销
*主线程只accept连接，并以Round Robin的方式分发给其它IO线程，有单独的日志线程，很大程度上避免了锁的争用
*使用eventfd实现了线程异步唤醒
*实现了基于红黑树的TimerQueue定时器
*实现了用timing wheel来踢掉空闲的连接
*使用双缓冲技术实现了高效的异步日志系统
*为了避免内存泄漏，使用了智能指针等RALL技术
*使用状态机解析HTTP请求，支持管线化请求
*实现了应用层缓冲Buffer
*支持优雅地断开连接

## Model
并发模型为one loop per thread+ thread pool，新连接使用Round Robin的方式分配，详情见[并发模型]()   
![并发模型]()

## 代码统计
![代码统计]()
