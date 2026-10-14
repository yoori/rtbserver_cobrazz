#pragma once

#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>

#include <Plain/Base.hpp>

namespace PlainTypes
{
  template<typename CustomTypes, typename String>
  class CustomString;

  template<typename CustomTypes>
  class CustomString<CustomTypes, std::string_view>
  {
  public:
    using Base = std::string_view;
    using Context = typename CustomTypes::Context;
    static constexpr unsigned long FIXED_SIZE = 4;

    explicit CustomString(Context& context) noexcept;
    CustomString& operator=(std::string_view value) noexcept;
    operator std::string_view() const;
    explicit operator std::string() const;

    Context& context() const noexcept;
    std::string_view view() const;
    const char* data() const noexcept;
    std::size_t size() const;
    bool empty() const;
    CustomString& assign(const char* data, std::size_t size) noexcept;
    CustomString& assign(std::string_view value) noexcept;
    CustomString& assign(std::size_t count, char value);
    CustomString& append(std::string_view suffix);
    void clear() noexcept;
    void init_default() noexcept;
    void init(const void* buf, unsigned long size);
    unsigned long dyn_size_() const;
    void save_(void* fixed_buf, void* dyn_buf) const;

  private:
    Context* context_;
    // init keeps the remaining buffer; view narrows it only on the first read.
    mutable std::string_view value_;
    mutable bool length_known_ = true;
  };

  template<typename T>
  CustomString<T, std::string_view>::CustomString(Context& context) noexcept
    : context_(&context)
  {}

  template<typename T>
  CustomString<T, std::string_view>&
  CustomString<T, std::string_view>::operator=(std::string_view value) noexcept
  {
    return assign(value);
  }

  template<typename T>
  CustomString<T, std::string_view>::operator std::string_view() const
  {
    return view();
  }

  template<typename T>
  CustomString<T, std::string_view>::operator std::string() const
  {
    return std::string(view());
  }

  template<typename T>
  typename CustomString<T, std::string_view>::Context&
  CustomString<T, std::string_view>::context() const noexcept
  {
    return *context_;
  }

  template<typename T>
  std::string_view CustomString<T, std::string_view>::view() const
  {
    if (!length_known_)
    {
      const char* end = static_cast<const char*>(std::memchr(value_.data(), 0, value_.size()));
      if (!end)
      {
        throw CorruptedStruct("CustomString: missing terminator");
      }

      value_ = value_.substr(0, end - value_.data());
      length_known_ = true;
    }
    return value_;
  }

  template<typename T>
  const char* CustomString<T, std::string_view>::data() const noexcept
  {
    return value_.data();
  }

  template<typename T>
  std::size_t CustomString<T, std::string_view>::size() const
  {
    return view().size();
  }

  template<typename T>
  bool CustomString<T, std::string_view>::empty() const
  {
    return view().empty();
  }

  template<typename T>
  CustomString<T, std::string_view>&
  CustomString<T, std::string_view>::assign(const char* data, std::size_t size) noexcept
  {
    value_ = std::string_view(data, size);
    length_known_ = true;
    return *this;
  }

  template<typename T>
  CustomString<T, std::string_view>&
  CustomString<T, std::string_view>::assign(std::string_view value) noexcept
  {
    value_ = value;
    length_known_ = true;
    return *this;
  }

  template<typename T>
  CustomString<T, std::string_view>&
  CustomString<T, std::string_view>::assign(std::size_t count, char value)
  {
    if (!count)
    {
      clear();
      return *this;
    }

    char* data = T::allocate_string(*context_, count);
    std::memset(data, value, count);
    return assign(data, count);
  }

  template<typename T>
  CustomString<T, std::string_view>&
  CustomString<T, std::string_view>::append(std::string_view suffix)
  {
    const auto prefix = view();
    if (suffix.empty())
    {
      return *this;
    }

    if (suffix.size() > std::numeric_limits<std::size_t>::max() - prefix.size())
    {
      throw std::length_error("CustomString: appended string is too long");
    }

    char* data = T::allocate_string(*context_, prefix.size() + suffix.size());
    if (!prefix.empty())
    {
      std::memcpy(data, prefix.data(), prefix.size());
    }

    std::memcpy(data + prefix.size(), suffix.data(), suffix.size());
    return assign(data, prefix.size() + suffix.size());
  }

  template<typename T>
  void CustomString<T, std::string_view>::clear() noexcept
  {
    value_ = {};
    length_known_ = true;
  }

  template<typename T>
  void CustomString<T, std::string_view>::init_default() noexcept
  {
    clear();
  }

  template<typename T>
  void CustomString<T, std::string_view>::init(const void* buf, unsigned long size)
  {
    if (size < FIXED_SIZE)
    {
      throw CorruptedStruct("CustomString: truncated header");
    }

    std::uint32_t offset;
    std::memcpy(&offset, buf, sizeof(offset));
    if (offset < FIXED_SIZE || offset >= size)
    {
      throw CorruptedStruct("CustomString: invalid offset");
    }

    value_ = std::string_view(static_cast<const char*>(buf) + offset, size - offset);
    length_known_ = false;
  }

  template<typename T>
  unsigned long CustomString<T, std::string_view>::dyn_size_() const
  {
    return size() + 1;
  }

  template<typename T>
  void CustomString<T, std::string_view>::save_(void* fixed_buf, void* dyn_buf) const
  {
    const auto value = view();
    const std::uint32_t offset = static_cast<char*>(dyn_buf) - static_cast<char*>(fixed_buf);
    std::memcpy(fixed_buf, &offset, sizeof(offset));
    if (!value.empty())
    {
      std::memcpy(dyn_buf, value.data(), value.size());
    }

    static_cast<char*>(dyn_buf)[value.size()] = '\0';
  }
}
