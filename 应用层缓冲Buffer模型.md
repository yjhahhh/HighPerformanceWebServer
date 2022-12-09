# 应用层缓冲Buffer
**由于epollr使用的是LT水平触发模式，所以需要提供应用层缓冲给TcpConnection。至于使用LT模式的原因是LT模式不会发生漏掉事件的BUG、读写的时候不必等候EAGAIN，可以节省系统调用次数，降低延迟。**

>Buffer对外表现为一块连续的内存（char*, len），以便程序代码的编写。size()可以自动增长，以适应不同大小的消息。不是一个fixed size array（不是固定大小数组）。内部以vector of char来保存数据，并提供相应的访问函数。TcpConnection会从socket读取数据，然后写入input buffer（由Buffer::readFd()完成）；程序代码在onMessage回调中，从input buffer读取数据。程序代码把数据写入output buffer（用Connection::send()完成）；TcpConnection从output buffer读取数据并写入socket。
![模型](./datum/Buffer%E6%95%B0%E6%8D%AE%E7%BB%93%E6%9E%84.png)