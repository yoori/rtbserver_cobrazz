#pragma once

#include <cstddef>
#include <type_traits>

#include <Commons/Coro/StartableAwaitable.hpp>
#include <Generics/MonoAllocator.hpp>

namespace AdServer::Commons
{
  // The arena must outlive every frame; only frame allocation accesses its mutable state.
  struct MonoCoroutineFrameAllocator
  {
    template<typename... Args>
    static void* operator new(std::size_t size, Generics::MonoAllocatorArena& arena, Args&&...)
    {
      constexpr auto element_size = sizeof(Unit);
      return Generics::MonoAllocator<Unit>(arena).allocate(
        size / element_size + (size % element_size != 0));
    }

    template<typename Owner, typename... Args>
      requires (!std::is_same_v<std::remove_cvref_t<Owner>, Generics::MonoAllocatorArena>)
    static void* operator new(
      std::size_t size, Owner&, Generics::MonoAllocatorArena& arena, Args&&...)
    {
      return operator new(size, arena);
    }

    static void operator delete(void*, std::size_t) noexcept
    {}

  private:
    struct alignas(std::max_align_t) Unit
    {};
  };

  template<typename ResultType>
  using MonoStartableAwaitable = StartableAwaitable<ResultType, MonoCoroutineFrameAllocator>;
}
