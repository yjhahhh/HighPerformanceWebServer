#include"Buffer.h"
#include<assert.h>
#include<endian.h>
#include<sys/uio.h>

using namespace std;


Buffer::Buffer(size_t initialSize)
    : buffer_(CheapPrepend + initialSize),
    readIndex_(CheapPrepend), writeIndex_(CheapPrepend)
{
    assert(readableBytes() == 0);
    assert(writeableBytes() == initialSize);
    assert(prependableBytes() == CheapPrepend);
}

size_t Buffer::readableBytes() const
{
    return writeIndex_ - readIndex_;
}

size_t Buffer::writeableBytes() const
{
    return buffer_.size() - writeIndex_;
}

size_t Buffer::prependableBytes() const
{
    return readIndex_;
}

const char* Buffer::peek() const
{
    return begin() + readIndex_;
}

const char* Buffer::beginWrite() const
{
    return begin() + writeIndex_;
}

char* Buffer::beginWrite()
{
    return begin() + writeIndex_;
}

void Buffer::hasWritten(size_t len)
{
    assert(writeableBytes() >= len);
    writeIndex_ += len;
}

void Buffer::unWrites(size_t len)
{
    assert(readableBytes() >= len);
    writeIndex_ -= len;
}

void Buffer::swap(Buffer& rhs)
{
    buffer_.swap(rhs.buffer_);
    std::swap(readIndex_, rhs.readIndex_);
    std::swap(writeIndex_, rhs.writeIndex_);
}

void Buffer::retrieve(size_t len)
{
    assert(len <= readableBytes());

    if(len < readableBytes())
    {
        //数据充足
        readIndex_ += len;
    }
    else
    {
        retrieveAll();
    }
}

void Buffer::retrieveUntil(const char* end)
{
    assert(peek() < end);
    assert(end <= beginWrite());
    retrieve(end - peek());
}

void Buffer::retrieveInt64()
{
    retrieve(sizeof(int64_t));
}

void Buffer::retrieveInt32()
{
    retrieve(sizeof(int32_t));
}

void Buffer::retrieveInt16()
{
    retrieve(sizeof(int16_t));
}

void Buffer::retrieveInt8()
{
    retrieve(sizeof(int8_t));
}

void Buffer::retrieveAll()
{
    readIndex_ = CheapPrepend;
    writeIndex_ = CheapPrepend;
}

string Buffer::retrieveAllAsString()
{
    return retrieveAsString(readableBytes());
}

string Buffer::retrieveAsString(size_t len)
{
    assert(len <= readableBytes());
    string result(peek(),len);
    retrieve(len);;
    return result;
}

StringPiece Buffer::toStringPiece() const
{
    return StringPiece(peek(), readableBytes());
}

int64_t Buffer::readInt64()
{
    int64_t x = peekInt16();;
    retrieveInt64();
    return x;;
}

int32_t Buffer::readInt32()
{
    int32_t x = peekInt32();
    retrieveInt32();
    return x;
}

int16_t Buffer::readInt16()
{
    int16_t x = peekInt16();
    retrieveInt16();
    return x;
}

int8_t Buffer::readInt8()
{
    int8_t x = peekInt8();
    retrieveInt8();
    return x;
}

int64_t Buffer::peekInt64() const
{
    assert(readableBytes() >= sizeof(int64_t));
    int64_t be64 = 0;
    memcpy(&be64, peek(), sizeof(be64));
    return be64toh(be64);

}

int32_t Buffer::peekInt32() const
{
    assert(readableBytes() > sizeof(int32_t));
    int32_t be32 = 0;
    memcpy(&be32, peek(), sizeof(int32_t));
    return be32toh(be32);
}

int16_t Buffer::peekInt16() const
{
    assert(readableBytes() > sizeof(int16_t));
    int16_t be16 = 0;
    memcpy(&be16, peek(), sizeof(int16_t));
    return be16toh(be16);
}

int8_t Buffer::peekInt8() const
{
    assert(readableBytes() > sizeof(int8_t));
    int8_t x = *peek();
    return x;
}

void Buffer::appendInt64(int64_t x)
{
    int64_t be64 = htobe64(x);
    append(&be64, sizeof(be64));
}

void Buffer::appendInt32(int32_t x)
{
    int32_t be32 = htobe32(x);
    append(&be32, sizeof(be32));
}

void Buffer::appendInt16(int16_t x)
{
    int16_t be16 = htobe16(x);
    append(&be16, sizeof(be16)); 
}

void Buffer::appendInt8(int8_t x)
{
    append(&x, sizeof(x));
}

void Buffer::append(const string& str)
{
    append(str.data(), str.size());
}

void Buffer::append(const void* data, size_t len)
{
    append(static_cast<const char*>(data), len);
}

void Buffer::append(const char* data, size_t len)
{
    ensureWritableBytes(len);
    memcpy(beginWrite(), data, len);
    hasWritten(len);
}

void Buffer::append(const StringPiece& str)
{
    append(str.data(), str.size());
}

void Buffer::ensureWritableBytes(size_t len)
{
    if(writeableBytes() < len)
        makeSpace(len);
    assert(writeableBytes() >= len);
}

void Buffer::prependInt64(int64_t x)
{
    int64_t be64 = htobe64(x);
    prepend(&be64, sizeof(be64));
}

void Buffer::prependInt32(int32_t x)
{
    int32_t be32 = htobe32(x);
    prepend(&be32, sizeof(be32));
}

void Buffer::prependInt16(int16_t x)
{
    int16_t be16 = htobe16(x);
    prepend(&be16, sizeof(be16));
}

void Buffer::prependInt8(int8_t x)
{
    prepend(&x, sizeof(x));
}

void Buffer::prepend(const void* data, size_t len)
{
    assert(prependableBytes() >= len);
    readIndex_ -= len;
    memcpy(begin() + readIndex_, data, len);

}

void Buffer::shrink(size_t reserve)
{
    Buffer other;
    other.ensureWritableBytes(readableBytes() + reserve);
    other.append(toStringPiece());
    swap(other);
}

// 结合栈上的空间，避免内存使用过大，提高内存使用率
// 如果有5K个连接，每个连接就分配64K+64K的缓冲区的话，将占用640M内存，
// 而大多数时候，这些缓冲区的使用率很低
ssize_t Buffer::readFd(int fd, int* saveErrno)
{
    char extrabuf[65535];   //栈上额外的缓冲
    size_t writeable = writeableBytes();
    iovec vec[2];
    //先填满原缓冲区
    vec[0].iov_base = begin() + writeIndex_;
    vec[0].iov_len = writeable;
    //如果原缓冲区不够，则放入extabuf
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);

    const ssize_t n = readv(fd, vec, 2);

    if(n < 0)
    {
        //发生错误
        *saveErrno = errno;;
    }
    else if(static_cast<size_t>(n) <= writeable)
    {
        //原缓冲区足够容纳
        writeIndex_ += n;;
    }
    else
    {
        //原缓冲区不够容纳
        writeIndex_ = buffer_.size();
        append(extrabuf, n - writeable);    //将其append到buffer_，vector会自动扩容
    }
    return n;
}

size_t Buffer::internalCapacity() const
{
    return buffer_.capacity();
}

void Buffer::makeSpace(size_t len)
{
    if(writeableBytes() + prependableBytes() < len + CheapPrepend)
    {
        //可写空间加上预留空间不足的话，扩充容量
        buffer_.resize(writeIndex_ + len);
    }
    else
    {
        size_t readable = readableBytes();
        //把prependable空间合并到writeable
        copy(begin() + readIndex_, begin() + writeIndex_, begin() + CheapPrepend);
        readIndex_ -= prependableBytes();
        writeIndex_ -= prependableBytes();
        assert(readableBytes() == readable);
    }
}