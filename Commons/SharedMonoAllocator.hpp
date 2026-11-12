#pragma once

#include <memory>
#include <Generics/MonoAllocator.hpp>

namespace AdServer::Commons
{
  template<typename T>
  class SharedMonoAllocator : public Generics::MonoAllocator<T>
  {
  public:
    template<typename U>
    struct rebind
    {
      using other = SharedMonoAllocator<U>;
    };

    explicit SharedMonoAllocator(std::shared_ptr<Generics::MonoAllocatorArena> arena);
    SharedMonoAllocator(const SharedMonoAllocator& other) noexcept;
    SharedMonoAllocator& operator=(const SharedMonoAllocator& other) noexcept;

    template<typename U>
    SharedMonoAllocator(const SharedMonoAllocator<U>& other) noexcept;

    const std::shared_ptr<Generics::MonoAllocatorArena>& arena_owner() const noexcept;

    template<typename U>
    bool operator==(const SharedMonoAllocator<U>& other) const noexcept;

  private:
    std::shared_ptr<Generics::MonoAllocatorArena> arena_;
  };

  template<typename T>
  SharedMonoAllocator<T>::SharedMonoAllocator(std::shared_ptr<Generics::MonoAllocatorArena> arena)
    : Generics::MonoAllocator<T>(arena.get()),
      arena_(std::move(arena))
  {}

  template<typename T>
  SharedMonoAllocator<T>::SharedMonoAllocator(const SharedMonoAllocator& other) noexcept = default;

  template<typename T>
  SharedMonoAllocator<T>&
  SharedMonoAllocator<T>::operator=(const SharedMonoAllocator& other) noexcept = default;

  template<typename T>
  template<typename U>
  SharedMonoAllocator<T>::SharedMonoAllocator(const SharedMonoAllocator<U>& other) noexcept
    : Generics::MonoAllocator<T>(other.arena_owner().get()),
      arena_(other.arena_owner())
  {}

  template<typename T>
  const std::shared_ptr<Generics::MonoAllocatorArena>&
  SharedMonoAllocator<T>::arena_owner() const noexcept
  {
    return arena_;
  }

  template<typename T>
  template<typename U>
  bool SharedMonoAllocator<T>::operator==(const SharedMonoAllocator<U>& other) const noexcept
  {
    return arena_ == other.arena_owner();
  }
}
