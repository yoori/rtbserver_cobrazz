#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <map>
#include <memory>
#include <optional>
#include <shared_mutex>
#include <thread>
#include <utility>
#include <vector>

#include <Generics/ActiveObject.hpp>
#include <Generics/Time.hpp>

namespace AdServer::Grpc
{
  template<typename T>
  class RefPool:
    public Generics::SimpleActiveObject,
    public std::enable_shared_from_this<RefPool<T>>
  {
  public:
    class Ref;

    struct RefHolder:
      public std::enable_shared_from_this<RefHolder>
    {
      RefHolder(
        std::shared_ptr<T> object_val,
        std::weak_ptr<RefPool> pool_val);

      std::shared_ptr<T> object;
      std::weak_ptr<RefPool> pool;
      std::atomic_bool bad{false};
      std::optional<Generics::Time> bad_before_time;
    };

    class Ref
    {
    public:
      T* operator->() noexcept;
      const T* operator->() const noexcept;

      T& operator*() noexcept;
      const T& operator*() const noexcept;

      void mark_as_bad(const Generics::Time& bad_before_time);
      bool is_bad() const noexcept;

    private:
      friend class RefPool;

      explicit Ref(std::shared_ptr<RefHolder> ref_holder);

      std::shared_ptr<RefHolder> ref_holder_;
    };

  public:
    RefPool();
    ~RefPool() noexcept;

    void set_refs(const std::vector<std::shared_ptr<T>>& refs);
    std::optional<Ref> get_object();

  protected:
    struct BadRefKey
    {
      Generics::Time bad_before_time;
      RefHolder* ref_holder;

      bool operator<(const BadRefKey& rhs) const noexcept;
    };

    void activate_object_() override;
    void deactivate_object_() override;
    void wait_object_() override;

    std::shared_mutex lock_;
    std::condition_variable_any cond_;
    std::vector<std::shared_ptr<RefHolder>> available_ref_holders_;
    std::map<BadRefKey, std::shared_ptr<RefHolder>> bad_ref_holders_;

  private:
    void mark_as_bad_(
      const std::shared_ptr<RefHolder>& ref_holder,
      const Generics::Time& bad_before_time);

    void move_bad_refs_loop_();

  private:
    std::atomic<std::size_t> next_ref_index_{0};
    std::optional<std::thread> thread_;
  };
}

#include "RefPool.tpp"
