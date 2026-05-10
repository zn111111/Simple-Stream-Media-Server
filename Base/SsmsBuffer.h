// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef MUDUO_NET_BUFFER_H
#define MUDUO_NET_BUFFER_H

#include <algorithm>
#include <vector>
#include <string>
#include <memory>
#include <assert.h>
#include <string.h>
#include <sys/socket.h>
#include "Base/NonCopyable.h"

using std::string;

namespace ssms
{
  namespace base
  {

    namespace sockets
    {

      // the inline assembler code makes type blur,
      // so we disable warnings for a while.
      #pragma GCC diagnostic push
      #pragma GCC diagnostic ignored "-Wconversion"
      #pragma GCC diagnostic ignored "-Wold-style-cast"
      inline uint64_t hostToNetwork64(uint64_t host64);

      inline uint32_t hostToNetwork32(uint32_t host32);

      inline uint16_t hostToNetwork16(uint16_t host16);

      inline uint64_t networkToHost64(uint64_t net64);

      inline uint32_t networkToHost32(uint32_t net32);

      inline uint16_t networkToHost16(uint16_t net16);

      ssize_t readv(int sockfd, const struct iovec *iov, int iovcnt);

      #pragma GCC diagnostic pop
    }

    template<typename To, typename From>
    inline To implicit_cast(From const &f)
    {
      return f;
    }

    class StringPiece {
      private:
      const char*   ptr_;
      int           length_;
    
      public:
      // We provide non-explicit singleton constructors so users can pass
      // in a "const char*" or a "string" wherever a "StringPiece" is
      // expected.
      StringPiece()
        : ptr_(NULL), length_(0) { }
      StringPiece(const char* str)
        : ptr_(str), length_(static_cast<int>(strlen(ptr_))) { }
      StringPiece(const unsigned char* str)
        : ptr_(reinterpret_cast<const char*>(str)),
          length_(static_cast<int>(strlen(ptr_))) { }
      StringPiece(const string& str)
        : ptr_(str.data()), length_(static_cast<int>(str.size())) { }
      StringPiece(const char* offset, int len)
        : ptr_(offset), length_(len) { }
    
      // data() may return a pointer to a buffer with embedded NULs, and the
      // returned buffer may or may not be null terminated.  Therefore it is
      // typically a mistake to pass data() to a routine that expects a NUL
      // terminated string.  Use "as_string().c_str()" if you really need to do
      // this.  Or better yet, change your routine so it does not rely on NUL
      // termination.
      const char* data() const { return ptr_; }
      int size() const { return length_; }
      bool empty() const { return length_ == 0; }
      const char* begin() const { return ptr_; }
      const char* end() const { return ptr_ + length_; }
    
      void clear() { ptr_ = NULL; length_ = 0; }
      void set(const char* buffer, int len) { ptr_ = buffer; length_ = len; }
      void set(const char* str) {
        ptr_ = str;
        length_ = static_cast<int>(strlen(str));
      }
      void set(const void* buffer, int len) {
        ptr_ = reinterpret_cast<const char*>(buffer);
        length_ = len;
      }
    
      char operator[](int i) const { return ptr_[i]; }
    
      void remove_prefix(int n) {
        ptr_ += n;
        length_ -= n;
      }
    
      void remove_suffix(int n) {
        length_ -= n;
      }
    
      bool operator==(const StringPiece& x) const {
        return ((length_ == x.length_) &&
                (memcmp(ptr_, x.ptr_, length_) == 0));
      }
      bool operator!=(const StringPiece& x) const {
        return !(*this == x);
      }
    
    #define STRINGPIECE_BINARY_PREDICATE(cmp,auxcmp)                             \
      bool operator cmp (const StringPiece& x) const {                           \
        int r = memcmp(ptr_, x.ptr_, length_ < x.length_ ? length_ : x.length_); \
        return ((r auxcmp 0) || ((r == 0) && (length_ cmp x.length_)));          \
      }
      STRINGPIECE_BINARY_PREDICATE(<,  <);
      STRINGPIECE_BINARY_PREDICATE(<=, <);
      STRINGPIECE_BINARY_PREDICATE(>=, >);
      STRINGPIECE_BINARY_PREDICATE(>,  >);
    #undef STRINGPIECE_BINARY_PREDICATE
    
      int compare(const StringPiece& x) const {
        int r = memcmp(ptr_, x.ptr_, length_ < x.length_ ? length_ : x.length_);
        if (r == 0) {
          if (length_ < x.length_) r = -1;
          else if (length_ > x.length_) r = +1;
        }
        return r;
      }
    
      string as_string() const {
        return string(data(), size());
      }
    
      void CopyToString(string* target) const {
        target->assign(ptr_, length_);
      }
    
      // Does "this" start with "x"
      bool starts_with(const StringPiece& x) const {
        return ((length_ >= x.length_) && (memcmp(ptr_, x.ptr_, x.length_) == 0));
      }
    };

    /// A buffer class modeled after org.jboss.netty.buffer.ChannelBuffer
    ///
    /// @code
    /// +-------------------+------------------+------------------+
    /// | prependable bytes |  readable bytes  |  writable bytes  |
    /// |                   |     (CONTENT)    |                  |
    /// +-------------------+------------------+------------------+
    /// |                   |                  |                  |
    /// 0      <=      readerIndex   <=   writerIndex    <=     size
    /// @endcode
    class SsmsBuffer;
    using SsmsBufferPtr = std::shared_ptr<SsmsBuffer>;
    class SsmsBuffer : public NonCopyable
    {
    public:
      static const size_t kCheapPrepend = 8;
      static const size_t kInitialSize = 1024;

      explicit SsmsBuffer(size_t initialSize = kInitialSize);

      // implicit copy-ctor, move-ctor, dtor and assignment are fine
      // NOTE: implicit move-ctor is added in g++ 4.6

      void swap(SsmsBuffer& rhs);

      size_t readableBytes() const;

      size_t writableBytes() const;

      size_t prependableBytes() const;

      const char* peek() const;

      const char* findCRLF() const;

      const char* findCRLF(const char* start) const;

      const char* findEOL() const;

      const char* findEOL(const char* start) const;

      // retrieve returns void, to prevent
      // string str(retrieve(readableBytes()), readableBytes());
      // the evaluation of two functions are unspecified
      void retrieve(size_t len);

      void retrieveUntil(const char* end);

      void retrieveInt64();

      void retrieveInt32();

      void retrieveInt16();

      void retrieveInt8();

      void retrieveAll();

      string retrieveAllAsString();

      string retrieveAsString(size_t len);

      StringPiece toStringPiece() const;

      void append(const StringPiece& str);

      void append(const char* /*restrict*/ data, size_t len);

      void append(const void* /*restrict*/ data, size_t len);

      void ensureWritableBytes(size_t len);

      char* beginWrite();

      const char* beginWrite() const;

      void hasWritten(size_t len);

      void unwrite(size_t len);

      void appendInt64(int64_t x);

      void appendInt32(int32_t x);

      void appendInt16(int16_t x);

      void appendInt8(int8_t x);

      ///
      /// Read int64_t from network endian
      ///
      /// Require: buf->readableBytes() >= sizeof(int32_t)
      int64_t readInt64();

      ///
      /// Read int32_t from network endian
      ///
      /// Require: buf->readableBytes() >= sizeof(int32_t)
      int32_t readInt32();

      int16_t readInt16();

      int8_t readInt8();

      ///
      /// Peek int64_t from network endian
      ///
      /// Require: buf->readableBytes() >= sizeof(int64_t)
      int64_t peekInt64() const;

      ///
      /// Peek int32_t from network endian
      ///
      /// Require: buf->readableBytes() >= sizeof(int32_t)
      int32_t peekInt32() const;
  
      int16_t peekInt16() const;

      int8_t peekInt8() const;

      ///
      /// Prepend int64_t using network endian
      ///
      void prependInt64(int64_t x);

      ///
      /// Prepend int32_t using network endian
      ///
      void prependInt32(int32_t x);

      void prependInt16(int16_t x);

      void prependInt8(int8_t x);

      void prepend(const void* /*restrict*/ data, size_t len);

      void shrink(size_t reserve);

      size_t internalCapacity() const;

      /// Read data directly into buffer.
      ///
      /// It may implement with readv(2)
      /// @return result of read(2), @c errno is saved
      ssize_t readFd(int fd, int* savedErrno);

    private:

      char* begin();

      const char* begin() const;

      void makeSpace(size_t len);

    private:
      std::vector<char> buffer_;
      size_t readerIndex_;
      size_t writerIndex_;

      static const char kCRLF[];
    };

  }
}

#endif  // MUDUO_NET_BUFFER_H
