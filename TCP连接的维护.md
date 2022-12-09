# TCP连接的维护
**TcpConnection是较为核心的类，它是一个Tcp连接的抽象，默认是使用share_ptr来管理其生命周期，因为当Tcp连接断开时，可能还有其他地方持有TcpConnection对象，使用share_ptr来管理能安全地销毁对象。TcpConnection不提供接口给用户，它所提供的接口供TcpServer和TcpClient使用。TcpConnection还拥有应用层的读写缓冲区inputBuffer_和outputBuffet_。**
## 新建连接
>TcpConnection没有发起连接的功能，构造函数参数是已经建立好连接的socket fd，初始状态是Connecting。连接可以是TcpServer或TcpClient发起。
![新建连接](./datum/%E6%96%B0%E5%BB%BA%E8%BF%9E%E6%8E%A5%E6%97%B6%E5%BA%8F%E5%9B%BE.png)

## 连接的断开
>TcpConnection有2种关闭连接的方式：
>1. 被动关闭：即对端先关闭连接，本地read返回0，触发关闭逻辑，调用handleClose。
>2. 主动关闭：利用forceClose()或forceCloseWithDelay()成员函数调用handleClose，强制关闭或强制延时关闭连接。
>>在WebServer中并没有使用强制主动关闭连接，而是等待对端先断开连接，若是HTTP短连接，会优雅的连接，会调用shutdown()关闭写连接，然后等待对端close()连接。
>
>![连接的断开](./datum/TcpConnection%E6%96%AD%E5%BC%80%E8%BF%9E%E6%8E%A5.png)
