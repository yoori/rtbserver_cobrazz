#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
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
    DECLARE_EXCEPTION(InvalidArgument, eh::DescriptiveException);

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
      bool available = true;
      bool probing = false;
      std::optional<Generics::Time> bad_before_time;
      mutable std::mutex unavailable_error_lock;
      Generics::Time unavailable_error_time = Generics::Time::ZERO;
      std::string unavailable_error;
    };

    class Ref
    {
    public:
      T* operator->() noexcept;
      const T* operator->() const noexcept;

      T& operator*() noexcept;
      const T& operator*() const noexcept;

      void mark_as_bad(const Generics::Time& bad_before_time);
      void mark_as_bad(
        const Generics::Time& bad_before_time,
        std::string unavailable_error);
      bool is_bad() const noexcept;

    private:
      friend class RefPool;

      struct ProbeGuard
      {
        explicit ProbeGuard(std::shared_ptr<RefHolder> ref_holder_val);
        ~ProbeGuard() noexcept;

        std::shared_ptr<RefHolder> ref_holder;
      };

      explicit Ref(
        std::shared_ptr<RefHolder> ref_holder,
        bool probe_ref = false);

      std::shared_ptr<RefHolder> ref_holder_;
      std::shared_ptr<ProbeGuard> probe_guard_;
    };

  public:
    explicit RefPool(std::vector<std::shared_ptr<T>> refs);
    ~RefPool() noexcept;

    std::optional<Ref> get_object();
    std::string unavailable_description() const;

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

    mutable std::shared_mutex lock_;
    std::condition_variable_any cond_;
    std::vector<std::shared_ptr<RefHolder>> available_ref_holders_;
    std::vector<std::shared_ptr<RefHolder>> try_ref_holders_;
    std::map<BadRefKey, std::shared_ptr<RefHolder>> bad_ref_holders_;

  private:
    void init_refs_();
    void mark_as_bad_(
      const std::shared_ptr<RefHolder>& ref_holder,
      const Generics::Time& bad_before_time,
      std::string unavailable_error);

    void finish_probe_(
      const std::shared_ptr<RefHolder>& ref_holder) noexcept;

    void move_bad_refs_loop_();

  private:
    std::vector<std::shared_ptr<T>> refs_;
    std::vector<std::shared_ptr<RefHolder>> ref_holders_;
    std::atomic<std::size_t> next_ref_index_{0};
    std::atomic<std::size_t> try_ref_count_{0};
    std::optional<std::thread> thread_;
  };
}

#include "RefPool.tpp"
