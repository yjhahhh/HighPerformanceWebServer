#ifndef _BUFFER_
#define _BUFFER_
#include"base/noncopyable.h"
#include<vector>
#include<string>
#include"base/StringPiece.h"

class Buffer
{
public:

    Buffer(size_t initialSize = InitialSize);
    //可读缓冲大小
    size_t readableBytes() const;
    //可写缓冲大小
    size_t writeableBytes() const;
    //prependable的大小
    size_t prependableBytes() const;

    //readIndex_对应的地址
    const char* peek() const;
    //可写缓冲的首地址
    const char* beginWrite() const;
    char* beginWrite();
    //将writeIndex_往后移动len,确保可写空间够大
    void hasWritten(size_t len);
    //将writeIndex_向前移动len，确保可读空间较大
    void unWrites(size_t len);

    //交换两个缓冲区
    void swap(Buffer& rhs);

    /* 这里取走只是移动readerIndex_, writerIndex_, 并不会直接读取或清除readable, writable空间数据　*/
    //取走(读入)长度为len的数据
    void retrieve(size_t len);
    //从readable空间取走 [peek(), end)这段区间数据, peek()是readable空间首地址
    void retrieveUntil(const char* end);
    // 从readable空间取走一个int64_t数据, 长度8byte
    void retrieveInt64();
    // 从readable空间取走一个int32_t数据, 长度4byte
    void retrieveInt32();
    //从readable空间取走一个int16_t数据, 长度2byte
    void retrieveInt16();
    //从readable空间取走一个int8_t数据, 长度1byte
    void retrieveInt8();
    //从readable空间取走所有数据, 直接移动readerIndex_, writerIndex_指示器即可
    void retrieveAll();
    //从readable空间取走所有数据, 转换为字符串返回
    std::string retrieveAllAsString();
    //从readable空间头部取走长度len byte的数据, 转换为字符串返回
    std::string retrieveAsString(size_t len);

    //把所有数据取回(读入)，并返回StringPiece
    StringPiece toStringPiece() const;

    /*
    readInt系列函数从readable空间读取指定长度（类型）的数据
    不仅从readable空间读取数据，还会利用相应的retrieve函数把数据从中取走，导致readable空间变小
    */
    //从readable空间头部读取一个int64_t类型数，由网络字节序转换为本地字节序
    int64_t readInt64();
    //从readable空间头部读取一个int32_t类型数，由网络字节序转换为本地字节序
    int32_t readInt32();
    //从readable空间头部读取一个int16_t类型数，由网络字节序转换为本地字节序
    int16_t readInt16();
    //从readable空间头部读取一个int8_t类型数，由网络字节序转换为本地字节序
    int8_t readInt8();

    /*peek系列函数只从readable空间头部（peek()）读取数据，而不取走数据，不会导致readable空间变化*/
    //从readable的头部peek()读取一个int64_t数据
    int64_t peekInt64() const;
    //从readable的头部peek()读取一个int32_t数据
    int32_t peekInt32() const;
    //从readable的头部peek()读取一个int16_t数据
    int16_t peekInt16() const;
    //从readable的头部peek()读取一个int8_t数据
    int8_t peekInt8() const;

    /*添加整数转换为网络字节序*/
    //向writeable空间添加一个int64_t类型数，转换为网络字节序
    void appendInt64(int64_t x);
    //向writeable空间添加一个int32_t类型数，转换为网络字节序
    void appendInt32(int32_t x);
    //向writeable空间添加一个int16_t类型数，转换为网络字节序
    void appendInt16(int16_t x);
    //向writeable空间添加一个int8_t类型数，转换为网络字节序
    void appendInt8(int8_t x);
    
    //将str加到缓冲区
    void append(const std::string& str);
    //将长度为len的data添加到缓冲区中
    void append(const char* data, size_t len);
    //将长度为len的data添加到缓冲区中
    void append(const void* data, size_t len);
    //将str加入缓冲区
    void append(const StringPiece& str);

    // 确保缓冲区可写空间>=len，如果不足则扩充
    void ensureWritableBytes(size_t len);


    /*prepend系列函数将预置指定长度数据到prependable空间，但不会改变prependable空间大小*/
    //在prependable空间末尾预置int64_t类型网络字节序的数x, 预置数会被转化为本地字节序
    void prependInt64(int64_t x);
    //在prependable空间末尾预置int32_t类型网络字节序的数x, 预置数会被转化为本地字节序
    void prependInt32(int32_t x);
    //在prependable空间末尾预置int16_t类型网络字节序的数x, 预置数会被转化为本地字节序
    void prependInt16(int16_t x);
    //在prependable空间末尾预置int8_t类型网络字节序的数x, 预置数会被转化为本地字节序
    void prependInt8(int8_t x);
    //在prependable空间添加长度为len的data
    void prepend(const void* data, size_t len);
    
    //收缩缓冲区空间, 将缓冲区中数据拷贝到新缓冲区, 确保writable空间最终大小为reserve
    void shrink(size_t reserve);

    //从fd读取数据到内部缓冲区, 将系统调用错误保存至savedErrno
    ssize_t readFd(int fd, int* saveErrno);

    static const size_t CheapPrepend = 8;   //初始预留的prependable空间大小
    static const size_t InitialSize = 1024; //初始大小

private:

    //缓冲区首地址，及prependable的首地址
    char* begin() const
    {
        return const_cast<char*>(buffer_.data());
    }

    

    //返回buffer_的容量capacity()
    size_t internalCapacity() const;

    /* writable空间不足以写入len byte数据时,
     * 1)如果writable空间 + prependable空间不足以存放数据, 就resize 申请新的更大的内部缓冲区buffer_
     * 2)如果足以存放数据, 就将prependable多余空间腾挪出来, 合并到writable空间 */
    void makeSpace(size_t len);

    std::vector<char> buffer_;  //线性缓冲区
    size_t readIndex_;  //可读数据首地址
    size_t writeIndex_; //可写数据首地址

    
    
};


#endif