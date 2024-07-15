#ifndef PLAIN_BUFFER_HPP
#define PLAIN_BUFFER_HPP

#include "Base.hpp"
#include <Generics/MemBuf.hpp>

namespace PlainTypes
{
  struct Buffer: public Generics::MemBuf
  {
  public:
    static const unsigned long FIXED_SIZE = 8;

    void init_default() noexcept;

    void init(const void* buf, unsigned long size)
      /*throw(CorruptedStruct)*/;

    unsigned long dyn_size_() const noexcept;

    void save_(void* fixed_buf, void* dyn_buf) const
      noexcept;

    static ConstBuf read_cast(const void* fixed_buf);

    Buffer&
    operator=(const Buffer& right) noexcept;

    Buffer();
    Buffer(const Buffer& right);

  };
}

namespace PlainTypes
{
  // Buffer
  inline
  void
  Buffer::init_default() noexcept
  {}

  inline
  void
  Buffer::init(const void* buf, unsigned long size)
    /*throw(CorruptedStruct)*/
  {
    static const char* FUN = "Buffer::init()";

    uint32_t buf_offset = *static_cast<const uint32_t*>(buf);
    uint32_t buf_size = *(static_cast<const uint32_t*>(buf) + 1);

    if(buf_offset + buf_size > size)
    {
      Stream::Error ostr;
      ostr << FUN << ": buffer end position great than size: " <<
        (buf_offset + buf_size) << " > " << size;
      throw CorruptedStruct(ostr);
    }

    assign(static_cast<const char*>(buf) + buf_offset, buf_size);
  }

  inline
  unsigned long
  Buffer::dyn_size_() const noexcept
  {
    return size();
  }

  inline
  void
  Buffer::save_(void* fixed_buf, void* dyn_buf) const
    noexcept
  {
    *static_cast<uint32_t*>(fixed_buf) =
      static_cast<unsigned char*>(dyn_buf) -
      static_cast<unsigned char*>(fixed_buf);
    *(static_cast<uint32_t*>(fixed_buf) + 1) = size();
    ::memcpy(dyn_buf, data(), size());
  }

  inline
  ConstBuf
  Buffer::read_cast(const void* fixed_buf)
  {
    return ConstBuf(
      static_cast<const char*>(fixed_buf) +
        *static_cast<const uint32_t*>(fixed_buf),
      *(static_cast<const uint32_t*>(fixed_buf) + 1));
  }

  inline
  Buffer&
  Buffer::operator=(const Buffer& right) noexcept
  {
    assign(right.data(), right.size());
    return *this;
  }

  inline
  Buffer::Buffer() : Generics::MemBuf() {}

  inline
  Buffer::Buffer(const Buffer& right): Generics::MemBuf(right){}
}

#endif /*PLAIN_BUFFER_HPP*/
