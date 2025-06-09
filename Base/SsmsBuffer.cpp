// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
#include <errno.h>
#include <sys/uio.h>
#include "SsmsBuffer.h"

using namespace ssms::base;

const char SsmsBuffer::kCRLF[] = "\r\n";

const size_t SsmsBuffer::kCheapPrepend;
const size_t SsmsBuffer::kInitialSize;

inline uint64_t sockets::hostToNetwork64(uint64_t host64)
{
  return htobe64(host64);
}

inline uint32_t sockets::hostToNetwork32(uint32_t host32)
{
  return htobe32(host32);
}

inline uint16_t sockets::hostToNetwork16(uint16_t host16)
{
  return htobe16(host16);
}

inline uint64_t sockets::networkToHost64(uint64_t net64)
{
  return be64toh(net64);
}

inline uint32_t sockets::networkToHost32(uint32_t net32)
{
  return be32toh(net32);
}

inline uint16_t sockets::networkToHost16(uint16_t net16)
{
  return be16toh(net16);
}

ssize_t sockets::readv(int sockfd, const struct iovec *iov, int iovcnt)
{
  return ::readv(sockfd, iov, iovcnt);
}

ssize_t SsmsBuffer::readFd(int fd, int* savedErrno)
{
  // saved an ioctl()/FIONREAD call to tell how much to read
  char extrabuf[65536];
  struct iovec vec[2];
  const size_t writable = writableBytes();
  vec[0].iov_base = begin()+writerIndex_;
  vec[0].iov_len = writable;
  vec[1].iov_base = extrabuf;
  vec[1].iov_len = sizeof extrabuf;
  // when there is enough space in this buffer, don't read into extrabuf.
  // when extrabuf is used, we read 128k-1 bytes at most.
  const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;
  const ssize_t n = sockets::readv(fd, vec, iovcnt);
  if (n < 0)
  {
    *savedErrno = errno;
  }
  else if (implicit_cast<size_t>(n) <= writable)
  {
    writerIndex_ += n;
  }
  else
  {
    writerIndex_ = buffer_.size();
    append(extrabuf, n - writable);
  }
  // if (n == writable + sizeof extrabuf)
  // {
  //   goto line_30;
  // }
  return n;
}

SsmsBuffer::SsmsBuffer(size_t initialSize)
: buffer_(kCheapPrepend + initialSize),
  readerIndex_(kCheapPrepend),
  writerIndex_(kCheapPrepend)
{
  assert(readableBytes() == 0);
  assert(writableBytes() == initialSize);
  assert(prependableBytes() == kCheapPrepend);
}

// implicit copy-ctor, move-ctor, dtor and assignment are fine
// NOTE: implicit move-ctor is added in g++ 4.6

void SsmsBuffer::swap(SsmsBuffer& rhs)
{
  buffer_.swap(rhs.buffer_);
  std::swap(readerIndex_, rhs.readerIndex_);
  std::swap(writerIndex_, rhs.writerIndex_);
}

size_t SsmsBuffer::readableBytes() const
{ return writerIndex_ - readerIndex_; }

size_t SsmsBuffer::writableBytes() const
{ return buffer_.size() - writerIndex_; }

size_t SsmsBuffer::prependableBytes() const
{ return readerIndex_; }

const char* SsmsBuffer::peek() const
{ return begin() + readerIndex_; }

const char* SsmsBuffer::findCRLF() const
{
// FIXME: replace with memmem()?
  const char* crlf = std::search(peek(), beginWrite(), kCRLF, kCRLF+2);
  return crlf == beginWrite() ? NULL : crlf;
}

const char* SsmsBuffer::findCRLF(const char* start) const
{
  assert(peek() <= start);
  assert(start <= beginWrite());
  // FIXME: replace with memmem()?
  const char* crlf = std::search(start, beginWrite(), kCRLF, kCRLF+2);
  return crlf == beginWrite() ? NULL : crlf;
}

const char* SsmsBuffer::findEOL() const
{
  const void* eol = memchr(peek(), '\n', readableBytes());
  return static_cast<const char*>(eol);
}

const char* SsmsBuffer::findEOL(const char* start) const
{
  assert(peek() <= start);
  assert(start <= beginWrite());
  const void* eol = memchr(start, '\n', beginWrite() - start);
  return static_cast<const char*>(eol);
}

void SsmsBuffer::retrieve(size_t len)
{
  assert(len <= readableBytes());
  if (len < readableBytes())
  {
    readerIndex_ += len;
  }
  else
  {
    retrieveAll();
  }
}

void SsmsBuffer::retrieveUntil(const char* end)
{
  assert(peek() <= end);
  assert(end <= beginWrite());
  retrieve(end - peek());
}

void SsmsBuffer::retrieveInt64()
{
  retrieve(sizeof(int64_t));
}

void SsmsBuffer::retrieveInt32()
{
  retrieve(sizeof(int32_t));
}

void SsmsBuffer::retrieveInt16()
{
  retrieve(sizeof(int16_t));
}

void SsmsBuffer::retrieveInt8()
{
  retrieve(sizeof(int8_t));
}

void SsmsBuffer::retrieveAll()
{
  readerIndex_ = kCheapPrepend;
  writerIndex_ = kCheapPrepend;
}

string SsmsBuffer::retrieveAllAsString()
{
  return retrieveAsString(readableBytes());
}

string SsmsBuffer::retrieveAsString(size_t len)
{
  assert(len <= readableBytes());
  string result(peek(), len);
  retrieve(len);
  return result;
}

StringPiece SsmsBuffer::toStringPiece() const
{
  return StringPiece(peek(), static_cast<int>(readableBytes()));
}

void SsmsBuffer::append(const StringPiece& str)
{
  append(str.data(), str.size());
}

void SsmsBuffer::append(const char* /*restrict*/ data, size_t len)
{
  ensureWritableBytes(len);
  std::copy(data, data+len, beginWrite());
  hasWritten(len);
}

void SsmsBuffer::append(const void* /*restrict*/ data, size_t len)
{
  append(static_cast<const char*>(data), len);
}

void SsmsBuffer::ensureWritableBytes(size_t len)
{
  if (writableBytes() < len)
  {
    makeSpace(len);
  }
  assert(writableBytes() >= len);
}

char* SsmsBuffer::beginWrite()
{ return begin() + writerIndex_; }

const char* SsmsBuffer::beginWrite() const
{ return begin() + writerIndex_; }

void SsmsBuffer::hasWritten(size_t len)
{
  assert(len <= writableBytes());
  writerIndex_ += len;
}

void SsmsBuffer::unwrite(size_t len)
{
  assert(len <= readableBytes());
  writerIndex_ -= len;
}

void SsmsBuffer::appendInt64(int64_t x)
{
  int64_t be64 = sockets::hostToNetwork64(x);
  append(&be64, sizeof be64);
}

void SsmsBuffer::appendInt32(int32_t x)
{
  int32_t be32 = sockets::hostToNetwork32(x);
  append(&be32, sizeof be32);
}

void SsmsBuffer::appendInt16(int16_t x)
{
  int16_t be16 = sockets::hostToNetwork16(x);
  append(&be16, sizeof be16);
}

void SsmsBuffer::appendInt8(int8_t x)
{
  append(&x, sizeof x);
}

int64_t SsmsBuffer::readInt64()
{
  int64_t result = peekInt64();
  retrieveInt64();
  return result;
}

int32_t SsmsBuffer::readInt32()
{
  int32_t result = peekInt32();
  retrieveInt32();
  return result;
}

int16_t SsmsBuffer::readInt16()
{
  int16_t result = peekInt16();
  retrieveInt16();
  return result;
}

int8_t SsmsBuffer::readInt8()
{
  int8_t result = peekInt8();
  retrieveInt8();
  return result;
}

int64_t SsmsBuffer::peekInt64() const
{
  assert(readableBytes() >= sizeof(int64_t));
  int64_t be64 = 0;
  ::memcpy(&be64, peek(), sizeof be64);
  return sockets::networkToHost64(be64);
}

int32_t SsmsBuffer::peekInt32() const
{
  assert(readableBytes() >= sizeof(int32_t));
  int32_t be32 = 0;
  ::memcpy(&be32, peek(), sizeof be32);
  return sockets::networkToHost32(be32);
}

int16_t SsmsBuffer::peekInt16() const
{
  assert(readableBytes() >= sizeof(int16_t));
  int16_t be16 = 0;
  ::memcpy(&be16, peek(), sizeof be16);
  return sockets::networkToHost16(be16);
}

int8_t SsmsBuffer::peekInt8() const
{
  assert(readableBytes() >= sizeof(int8_t));
  int8_t x = *peek();
  return x;
}

void SsmsBuffer::prependInt64(int64_t x)
{
  int64_t be64 = sockets::hostToNetwork64(x);
  prepend(&be64, sizeof be64);
}

void SsmsBuffer::prependInt32(int32_t x)
{
  int32_t be32 = sockets::hostToNetwork32(x);
  prepend(&be32, sizeof be32);
}

void SsmsBuffer::prependInt16(int16_t x)
{
  int16_t be16 = sockets::hostToNetwork16(x);
  prepend(&be16, sizeof be16);
}

void SsmsBuffer::prependInt8(int8_t x)
{
  prepend(&x, sizeof x);
}

void SsmsBuffer::prepend(const void* /*restrict*/ data, size_t len)
{
  assert(len <= prependableBytes());
  readerIndex_ -= len;
  const char* d = static_cast<const char*>(data);
  std::copy(d, d+len, begin()+readerIndex_);
}

void SsmsBuffer::shrink(size_t reserve)
{
  // FIXME: use vector::shrink_to_fit() in C++ 11 if possible.
  SsmsBuffer other;
  other.ensureWritableBytes(readableBytes()+reserve);
  other.append(toStringPiece());
  swap(other);
}

size_t SsmsBuffer::internalCapacity() const
{
  return buffer_.capacity();
}

char* SsmsBuffer::begin()
{ return &*buffer_.begin(); }

const char* SsmsBuffer::begin() const
{ return &*buffer_.begin(); }

void SsmsBuffer::makeSpace(size_t len)
{
  if (writableBytes() + prependableBytes() < len + kCheapPrepend)
  {
    // FIXME: move readable data
    buffer_.resize(writerIndex_+len);
  }
  else
  {
    // move readable data to the front, make space inside buffer
    assert(kCheapPrepend < readerIndex_);
    size_t readable = readableBytes();
    std::copy(begin()+readerIndex_,
              begin()+writerIndex_,
              begin()+kCheapPrepend);
    readerIndex_ = kCheapPrepend;
    writerIndex_ = readerIndex_ + readable;
    assert(readable == readableBytes());
  }
}