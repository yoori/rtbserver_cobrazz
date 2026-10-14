#pragma once

#include <string_view>
#include <type_traits>

#include <Generics/MonoAllocator.hpp>

namespace AdServer::Commons
{
  struct PlainMonoAllocatorTypes
  {
    using Context = Generics::MonoAllocatorArena;
    using String = std::string_view;
    using Buffer = Generics::MonoVector<unsigned char>;

    template<typename T>
    using Vector = Generics::MonoVector<T>;

    template<typename T>
    using List = Generics::MonoList<T>;

    template<typename T>
    static T make(Context& context);

    static char* allocate_string(Context& context, std::size_t size);
  };

  template<typename T>
  T PlainMonoAllocatorTypes::make(Context& context)
  {
    if constexpr (std::is_same_v<T, String>)
    {
      return T();
    }
    else
    {
      return T(typename T::allocator_type(context));
    }
  }

  inline char* PlainMonoAllocatorTypes::allocate_string(Context& context, std::size_t size)
  {
    return Generics::MonoAllocator<char>(context).allocate(size);
  }
}
