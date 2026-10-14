#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <list>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <Plain/Base.hpp>

namespace PlainTypes
{
  // Context must outlive writers, their copies and their container elements. Context-dependent
  // allocators must retain the context on copy and propagate it on assignment and swap.
  // CustomTypes supplies Context, String, Buffer, Vector<T>, List<T> and make<T>(Context&).
  // A string_view policy also supplies allocate_string(Context&, size) for string mutations.
  // Borrowed input buffers must remain alive and unchanged while any writer refers to them.
  // View strings check terminators lazily, so sizing and saving may report corrupted input.
  struct DefaultWritingTypes
  {
    struct Context {};
    using String = std::string;
    using Buffer = std::vector<unsigned char>;

    template<typename T>
    using Vector = std::vector<T>;

    template<typename T>
    using List = std::list<T>;

    template<typename T>
    static T make(Context& context);
  };

  template<typename CustomTypes, typename String = typename CustomTypes::String>
  class CustomString: public String
  {
  public:
    using Base = String;
    using Context = typename CustomTypes::Context;
    using Base::operator=;
    static constexpr unsigned long FIXED_SIZE = 4;

    explicit CustomString(Context& context);
    CustomString(const CustomString& other);
    CustomString(CustomString&& other) noexcept(std::is_nothrow_move_constructible_v<Base>);
    CustomString& operator=(const CustomString& other);
    CustomString& operator=(CustomString&& other) noexcept(std::is_nothrow_move_assignable_v<Base>);
    void init_default() noexcept;
    void init(const void* buf, unsigned long size);
    unsigned long dyn_size_() const noexcept;
    void save_(void* fixed_buf, void* dyn_buf) const noexcept;
  };

  template<typename CustomTypes>
  class CustomBuffer: public CustomTypes::Buffer
  {
  public:
    using Base = typename CustomTypes::Buffer;
    using Context = typename CustomTypes::Context;
    using Base::assign;
    static constexpr unsigned long FIXED_SIZE = 8;

    explicit CustomBuffer(Context& context);
    void assign(const void* buf, unsigned long size);
    void init_default() noexcept;
    void init(const void* buf, unsigned long size);
    unsigned long dyn_size_() const noexcept;
    void save_(void* fixed_buf, void* dyn_buf) const noexcept;
  };

  template<typename Element>
  struct CustomDynamicElement
  {
    static constexpr unsigned long FIXED_SIZE = Element::FIXED_SIZE;

    static void init(Element& element, const void* buf, unsigned long size);
    static unsigned long dyn_size(const Element& element);
    static void save(const Element& element, void* fixed_buf, void* dyn_buf);
  };

  template<typename Element, Element Read(const void*), Element& Write(void*), unsigned long Step>
  struct CustomSimpleElement
  {
    static constexpr unsigned long FIXED_SIZE = Step;

    static void init(Element& element, const void* buf, unsigned long size);
    static unsigned long dyn_size(const Element& element) noexcept;
    static void save(const Element& element, void* fixed_buf, void* dyn_buf) noexcept;
  };

  template<typename CustomTypes, typename Storage, typename Codec, unsigned long HeaderSize = 8>
  class CustomContainer: public Storage
  {
  public:
    using Context = typename CustomTypes::Context;
    using Element = typename Storage::value_type;
    using typename Storage::iterator;
    using typename Storage::const_iterator;
    using Storage::emplace;
    using Storage::emplace_back;
    static constexpr unsigned long FIXED_SIZE = HeaderSize;
    static_assert(Codec::FIXED_SIZE > 0, "Array elements must have a nonempty fixed part");

    explicit CustomContainer(Context& context);
    Context& context() const noexcept;
    Element& emplace_back();
    iterator emplace(const_iterator position);
    void resize(std::size_t size);
    void swap(CustomContainer& other);
    void init_default() noexcept;
    void init(const void* buf, unsigned long size);
    unsigned long dyn_size_() const;
    void save_(void* fixed_buf, void* dyn_buf) const;

  private:
    Context* context_;
  };

  template<typename Types, typename Element, unsigned long HeaderSize = 8>
  using CustomVector = CustomContainer<Types, typename Types::template Vector<Element>,
    CustomDynamicElement<Element>, HeaderSize>;

  template<typename Types, typename Element, unsigned long HeaderSize = 8>
  using CustomList = CustomContainer<Types, typename Types::template List<Element>,
    CustomDynamicElement<Element>, HeaderSize>;

  template<typename Types, typename Element, Element Read(const void*), Element& Write(void*),
    unsigned long Step, unsigned long HeaderSize = 8>
  using CustomSimpleVector = CustomContainer<Types, typename Types::template Vector<Element>,
    CustomSimpleElement<Element, Read, Write, Step>, HeaderSize>;

  template<typename Types, typename Element, Element Read(const void*), Element& Write(void*),
    unsigned long Step, unsigned long HeaderSize = 8>
  using CustomSimpleList = CustomContainer<Types, typename Types::template List<Element>,
    CustomSimpleElement<Element, Read, Write, Step>, HeaderSize>;

  template<typename T>
  T DefaultWritingTypes::make(Context&)
  {
    return T();
  }

  template<typename CustomTypes, typename String>
  CustomString<CustomTypes, String>::CustomString(Context& context)
    : Base(CustomTypes::template make<Base>(context))
  {}

  template<typename CustomTypes, typename String>
  CustomString<CustomTypes, String>::CustomString(const CustomString&) = default;

  template<typename CustomTypes, typename String>
  CustomString<CustomTypes, String>::CustomString(CustomString&&)
    noexcept(std::is_nothrow_move_constructible_v<Base>) = default;

  template<typename CustomTypes, typename String>
  CustomString<CustomTypes, String>&
  CustomString<CustomTypes, String>::operator=(const CustomString&) = default;

  template<typename CustomTypes, typename String>
  CustomString<CustomTypes, String>& CustomString<CustomTypes, String>::operator=(CustomString&&)
    noexcept(std::is_nothrow_move_assignable_v<Base>) = default;

  template<typename CustomTypes, typename String>
  void CustomString<CustomTypes, String>::init_default() noexcept
  {
    this->clear();
  }

  template<typename CustomTypes, typename String>
  void CustomString<CustomTypes, String>::init(const void* buf, unsigned long size)
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

    const char* begin = static_cast<const char*>(buf) + offset;
    const char* end = static_cast<const char*>(std::memchr(begin, 0, size - offset));
    if (!end)
    {
      throw CorruptedStruct("CustomString: missing terminator");
    }

    this->assign(begin, end - begin);
  }

  template<typename CustomTypes, typename String>
  unsigned long CustomString<CustomTypes, String>::dyn_size_() const noexcept
  {
    return this->size() + 1;
  }

  template<typename CustomTypes, typename String>
  void CustomString<CustomTypes, String>::save_(void* fixed_buf, void* dyn_buf) const noexcept
  {
    const std::uint32_t offset = static_cast<char*>(dyn_buf) - static_cast<char*>(fixed_buf);
    std::memcpy(fixed_buf, &offset, sizeof(offset));
    std::memcpy(dyn_buf, this->c_str(), this->size() + 1);
  }

  template<typename CustomTypes>
  CustomBuffer<CustomTypes>::CustomBuffer(Context& context)
    : Base(CustomTypes::template make<Base>(context))
  {}

  template<typename CustomTypes>
  void CustomBuffer<CustomTypes>::assign(const void* buf, unsigned long size)
  {
    this->resize(size);
    if (size)
    {
      std::memcpy(this->data(), buf, size);
    }
  }

  template<typename CustomTypes>
  void CustomBuffer<CustomTypes>::init_default() noexcept
  {
    this->clear();
  }

  template<typename CustomTypes>
  void CustomBuffer<CustomTypes>::init(const void* buf, unsigned long size)
  {
    if (size < FIXED_SIZE)
    {
      throw CorruptedStruct("CustomBuffer: truncated header");
    }

    std::uint32_t header[2];
    std::memcpy(header, buf, sizeof(header));
    if (header[0] < FIXED_SIZE || header[0] > size || header[1] > size - header[0])
    {
      throw CorruptedStruct("CustomBuffer: invalid range");
    }

    assign(static_cast<const char*>(buf) + header[0], header[1]);
  }

  template<typename CustomTypes>
  unsigned long CustomBuffer<CustomTypes>::dyn_size_() const noexcept
  {
    return this->size();
  }

  template<typename CustomTypes>
  void CustomBuffer<CustomTypes>::save_(void* fixed_buf, void* dyn_buf) const noexcept
  {
    const std::uint32_t header[] = {
      static_cast<std::uint32_t>(static_cast<char*>(dyn_buf) - static_cast<char*>(fixed_buf)),
      static_cast<std::uint32_t>(this->size())};
    std::memcpy(fixed_buf, header, sizeof(header));
    if (!this->empty())
    {
      std::memcpy(dyn_buf, this->data(), this->size());
    }
  }

  template<typename Element>
  void CustomDynamicElement<Element>::init(Element& element, const void* buf, unsigned long size)
  {
    element.init(buf, size);
  }

  template<typename Element>
  unsigned long CustomDynamicElement<Element>::dyn_size(const Element& element)
  {
    return element.dyn_size_();
  }

  template<typename Element>
  void CustomDynamicElement<Element>::save(const Element& element, void* fixed_buf, void* dyn_buf)
  {
    element.save_(fixed_buf, dyn_buf);
  }

  template<typename E, E Read(const void*), E& Write(void*), unsigned long Step>
  void CustomSimpleElement<E, Read, Write, Step>::init(E& element, const void* buf, unsigned long)
  {
    if constexpr (std::is_same_v<E, char> && Step == 1)
    {
      element = *static_cast<const char*>(buf);
    }
    else
    {
      alignas(std::max_align_t) unsigned char value[Step];
      std::memcpy(value, buf, Step);
      element = Read(value);
    }
  }

  template<typename E, E Read(const void*), E& Write(void*), unsigned long Step>
  unsigned long CustomSimpleElement<E, Read, Write, Step>::dyn_size(const E&) noexcept
  {
    return 0;
  }

  template<typename E, E Read(const void*), E& Write(void*), unsigned long Step>
  void CustomSimpleElement<E, Read, Write, Step>::save(const E& element, void* fixed_buf, void*)
    noexcept
  {
    alignas(std::max_align_t) unsigned char value[Step]{};
    Write(value) = element;
    std::memcpy(fixed_buf, value, Step);
  }

  template<typename T, typename S, typename C, unsigned long H>
  CustomContainer<T, S, C, H>::CustomContainer(Context& context)
    : S(T::template make<S>(context)), context_(&context)
  {}

  template<typename T, typename S, typename C, unsigned long H>
  typename CustomContainer<T, S, C, H>::Context& CustomContainer<T, S, C, H>::context() const
    noexcept
  {
    return *context_;
  }

  template<typename T, typename S, typename C, unsigned long H>
  typename CustomContainer<T, S, C, H>::Element& CustomContainer<T, S, C, H>::emplace_back()
  {
    if constexpr (std::is_constructible_v<Element, Context&>)
    {
      return S::emplace_back(*context_);
    }
    else
    {
      return S::emplace_back();
    }
  }

  template<typename T, typename S, typename C, unsigned long H>
  typename CustomContainer<T, S, C, H>::iterator
  CustomContainer<T, S, C, H>::emplace(const_iterator position)
  {
    if constexpr (std::is_constructible_v<Element, Context&>)
    {
      return S::emplace(position, *context_);
    }
    else
    {
      return S::emplace(position);
    }
  }

  template<typename T, typename S, typename C, unsigned long H>
  void CustomContainer<T, S, C, H>::resize(std::size_t size)
  {
    if (size < this->size())
    {
      this->erase(std::next(this->begin(), size), this->end());
    }

    if constexpr (std::is_same_v<S, typename T::template Vector<Element>>)
    {
      this->reserve(size);
    }

    while (this->size() < size)
    {
      emplace_back();
    }
  }

  template<typename T, typename S, typename C, unsigned long H>
  void CustomContainer<T, S, C, H>::swap(CustomContainer& other)
  {
    S::swap(other);
    std::swap(context_, other.context_);
  }

  template<typename T, typename S, typename C, unsigned long H>
  void CustomContainer<T, S, C, H>::init_default() noexcept
  {
    this->clear();
  }

  template<typename T, typename S, typename C, unsigned long H>
  void CustomContainer<T, S, C, H>::init(const void* buf, unsigned long size)
  {
    if (size < FIXED_SIZE)
    {
      throw CorruptedStruct("CustomContainer: truncated header");
    }

    std::uint32_t header[2];
    std::memcpy(header, buf, sizeof(header));
    if (header[0] < FIXED_SIZE || header[0] > header[1] || header[1] > size ||
      (header[1] - header[0]) % C::FIXED_SIZE != 0)
    {
      throw CorruptedStruct("CustomContainer: invalid range");
    }

    resize((header[1] - header[0]) / C::FIXED_SIZE);
    auto offset = header[0];
    for (auto& element : *this)
    {
      C::init(element, static_cast<const char*>(buf) + offset, size - offset);
      offset += C::FIXED_SIZE;
    }
  }

  template<typename T, typename S, typename C, unsigned long H>
  unsigned long CustomContainer<T, S, C, H>::dyn_size_() const
  {
    unsigned long result = this->size() * C::FIXED_SIZE;
    for (const auto& element : *this)
    {
      result += C::dyn_size(element);
    }
    return result;
  }

  template<typename T, typename S, typename C, unsigned long H>
  void CustomContainer<T, S, C, H>::save_(void* fixed_buf, void* dyn_buf) const
  {
    char* fixed = static_cast<char*>(dyn_buf);
    char* dynamic = fixed + this->size() * C::FIXED_SIZE;
    const std::uint32_t header[] = {
      static_cast<std::uint32_t>(fixed - static_cast<char*>(fixed_buf)),
      static_cast<std::uint32_t>(dynamic - static_cast<char*>(fixed_buf))};
    std::memcpy(fixed_buf, header, sizeof(header));
    for (const auto& element : *this)
    {
      C::save(element, fixed, dynamic);
      fixed += C::FIXED_SIZE;
      dynamic += C::dyn_size(element);
    }
  }
}

#include <Plain/CustomStringView.hpp>
