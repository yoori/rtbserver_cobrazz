#pragma once

#include <memory>
#include <vector>

#include <Commons/AsyncMutex.hpp>

namespace AdServer::Commons
{
  template<typename KeyType>
  class AsyncNoAllocLockMap
  {
  public:
    explicit AsyncNoAllocLockMap(unsigned long size = 100);

    AsyncMutex::ScopedLockAwaiter
    scoped_lock_async(const KeyType& key) noexcept;

  private:
    std::vector<std::unique_ptr<AsyncMutex> > locks_;
  };

  template<typename KeyType>
  AsyncNoAllocLockMap<KeyType>::AsyncNoAllocLockMap(unsigned long size)
  {
    locks_.reserve(size);
    for(unsigned long i = 0; i < size; ++i)
    {
      locks_.emplace_back(std::make_unique<AsyncMutex>());
    }
  }

  template<typename KeyType>
  AsyncMutex::ScopedLockAwaiter
  AsyncNoAllocLockMap<KeyType>::scoped_lock_async(const KeyType& key) noexcept
  {
    return locks_[key.hash() % locks_.size()]->scoped_lock_async();
  }
}
