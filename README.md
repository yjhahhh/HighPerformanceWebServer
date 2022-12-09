# A C++ High Performance Web Server
## 概述   
该WebServer基于muoduo网络库实现，在实现细节上很大程度上学习了陈硕的muduo网络库，同样遵循了one loop pre thread 的思想。   
本项目为C++编写的多线程Web服务器，能解析简单的GET/POST请求，支持HTTP长连接，支持管线化请求，能踢掉空闲的连接，实现了高性能的异步日志功能。   
> |Ⅰ|Ⅱ|Ⅲ|Ⅳ|Ⅴ|Ⅵ|
> |:--:|:--:|:--:|:--:|:--:|:--:|
> |[并发模型](./%E5%B9%B6%E5%8F%91%E6%A8%A1%E5%9E%8B.md)|[事件驱动模型](./%E4%BA%8B%E4%BB%B6%E9%A9%B1%E5%8A%A8%E6%A8%A1%E5%9E%8B.md)|[TCP连接的维护](./TCP%E8%BF%9E%E6%8E%A5%E7%9A%84%E7%BB%B4%E6%8A%A4.md)|[应用层缓冲Buffer模型](./%E5%BA%94%E7%94%A8%E5%B1%82%E7%BC%93%E5%86%B2Buffer%E6%A8%A1%E5%9E%8B.md)|[日志模型](./%E6%97%A5%E5%BF%97%E6%A8%A1%E5%9E%8B.md)|[压力测试](./%E5%8E%8B%E5%8A%9B%E6%B5%8B%E8%AF%95.md)|

## 环境   
* Ubuntu 18.04.06
* gcc 7.5.0

## 使用
    ./server [-(0-5)(日志级别)] [-h (使用帮助)] [-t threadNum] [-p port] [-s seconds] [-l logfile_path]

## Technical points
* 使用epoll水平触发的IO多路复用技术，非阻塞式IO，使用Reactor模式
* 基于多线程，并使用线程池技术，避免程序运行过程中频繁创建销毁的开销
* 主线程只accept连接，并以Round Robin的方式分发给其它IO线程，有单独的日志线程，很大程度上避免了锁的争用
* 使用eventfd实现了线程异步唤醒
* 实现了基于红黑树的TimerQueue定时器
* 实现了用timing wheel来踢掉空闲的连接
* 使用双缓冲技术实现了高效的异步日志系统
* 为了避免内存泄漏，使用了智能指针等RALL技术
* 使用状态机解析HTTP请求，支持管线化请求
* 实现了应用层缓冲Buffer
* 支持优雅地断开连接

## Model
并发模型为one loop per thread+ thread pool，新连接使用Round Robin的方式分配，详情见[并发模型](./%E5%B9%B6%E5%8F%91%E6%A8%A1%E5%9E%8B.md)   
![并发模型](./datum//Reactor%E6%A8%A1%E5%9E%8B.png)

## 代码统计
![代码统计](./datum//%E4%BB%A3%E7%A0%81%E7%BB%9F%E8%AE%A1.png)
