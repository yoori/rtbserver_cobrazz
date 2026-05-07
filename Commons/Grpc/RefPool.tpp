#pragma once

namespace AdServer::Grpc
{
  template<typename T>
  RefPool<T>::RefHolder::RefHolder(
    std::shared_ptr<T> object_val,
    std::weak_ptr<RefPool> pool_val)
    : object(std::move(object_val)),
      pool(std::move(pool_val))
  {}

  template<typename T>
  bool
  RefPool<T>::BadRefKey::operator<(const BadRefKey& rhs) const noexcept
  {
    return
      bad_before_time < rhs.bad_before_time ||
      (bad_before_time == rhs.bad_before_time &&
        ref_holder < rhs.ref_holder);
  }

  template<typename T>
  T*
  RefPool<T>::Ref::operator->() noexcept
  {
    return ref_holder_->object.get();
  }

  template<typename T>
  const T*
  RefPool<T>::Ref::operator->() const noexcept
  {
    return ref_holder_->object.get();
  }

  template<typename T>
  T&
  RefPool<T>::Ref::operator*() noexcept
  {
    return *ref_holder_->object;
  }

  template<typename T>
  const T&
  RefPool<T>::Ref::operator*() const noexcept
  {
    return *ref_holder_->object;
  }

  template<typename T>
  void
  RefPool<T>::Ref::mark_as_bad(const Generics::Time& bad_before_time)
  {
    if (auto pool = ref_holder_->pool.lock())
    {
      pool->mark_as_bad_(ref_holder_, bad_before_time);
    }
  }

  template<typename T>
  bool
  RefPool<T>::Ref::is_bad() const noexcept
  {
    return ref_holder_->bad.load(std::memory_order_acquire);
  }

  template<typename T>
  RefPool<T>::Ref::Ref(std::shared_ptr<RefHolder> ref_holder)
    : ref_holder_(std::move(ref_holder))
  {}

  template<typename T>
  RefPool<T>::RefPool() = default;

  template<typename T>
  RefPool<T>::~RefPool() noexcept
  {
    try
    {
      deactivate_object();
      wait_object();
    }
    catch (...)
    {
    }
  }

  template<typename T>
  void
  RefPool<T>::set_refs(const std::vector<std::shared_ptr<T>>& refs)
  {
    {
      std::unique_lock<std::shared_mutex> lock(lock_);
      available_ref_holders_.clear();
      bad_ref_holders_.clear();
      available_ref_holders_.reserve(refs.size());
      for (const auto& ref : refs)
      {
        available_ref_holders_.push_back(
          std::make_shared<RefHolder>(ref, this->weak_from_this()));
      }
    }

    cond_.notify_one();
  }

  template<typename T>
  std::optional<typename RefPool<T>::Ref>
  RefPool<T>::get_object()
  {
    std::shared_lock<std::shared_mutex> lock(lock_);
    if (available_ref_holders_.empty())
    {
      return std::nullopt;
    }

    const auto index =
      next_ref_index_.fetch_add(1, std::memory_order_relaxed) %
      available_ref_holders_.size();
    return Ref(available_ref_holders_[index]);
  }

  template<typename T>
  void
  RefPool<T>::activate_object_()
  {
    thread_.emplace([this]() {
      move_bad_refs_loop_();
    });
  }

  template<typename T>
  void
  RefPool<T>::deactivate_object_()
  {
    cond_.notify_one();
  }

  template<typename T>
  void
  RefPool<T>::wait_object_()
  {
    if (thread_)
    {
      thread_->join();
      thread_.reset();
    }
  }

  template<typename T>
  void
  RefPool<T>::mark_as_bad_(
    const std::shared_ptr<RefHolder>& ref_holder,
    const Generics::Time& bad_before_time)
  {
    bool wake_worker = false;

    {
      std::unique_lock<std::shared_mutex> lock(lock_);
      const auto previous_first_bad_time = bad_ref_holders_.empty() ?
        std::optional<Generics::Time>() :
        std::optional<Generics::Time>(
          bad_ref_holders_.begin()->first.bad_before_time);

      auto available_it = std::find(
        available_ref_holders_.begin(),
        available_ref_holders_.end(),
        ref_holder);
      if (available_it != available_ref_holders_.end())
      {
        available_ref_holders_.erase(available_it);
      }
      else if (ref_holder->bad_before_time)
      {
        bad_ref_holders_.erase(BadRefKey{
          *ref_holder->bad_before_time,
          ref_holder.get()});
      }

      const auto new_bad_before_time = ref_holder->bad_before_time ?
        std::max(*ref_holder->bad_before_time, bad_before_time) :
        bad_before_time;

      ref_holder->bad_before_time = new_bad_before_time;
      ref_holder->bad.store(true, std::memory_order_release);
      bad_ref_holders_.emplace(
        BadRefKey{new_bad_before_time, ref_holder.get()},
        ref_holder);

      wake_worker =
        !previous_first_bad_time ||
        new_bad_before_time < *previous_first_bad_time;
    }

    if (wake_worker)
    {
      cond_.notify_one();
    }
  }

  template<typename T>
  void
  RefPool<T>::move_bad_refs_loop_()
  {
    std::unique_lock<std::shared_mutex> lock(lock_);
    while (active())
    {
      if (bad_ref_holders_.empty())
      {
        cond_.wait(lock, [this]() {
          return !active() || !bad_ref_holders_.empty();
        });
        continue;
      }

      const auto now = Generics::Time::get_time_of_day();
      const auto first_bad_time =
        bad_ref_holders_.begin()->first.bad_before_time;
      if (first_bad_time > now)
      {
        const auto timeout = first_bad_time - now;
        cond_.wait_for(
          lock,
          std::chrono::microseconds(timeout.microseconds()),
          [this, first_bad_time]() {
            return
              !active() ||
              bad_ref_holders_.empty() ||
              bad_ref_holders_.begin()->first.bad_before_time < first_bad_time;
          });
        continue;
      }

      std::vector<std::shared_ptr<RefHolder>> ready_refs;
      for (auto it = bad_ref_holders_.begin();
        it != bad_ref_holders_.end() && it->first.bad_before_time <= now;)
      {
        ready_refs.emplace_back(std::move(it->second));
        it = bad_ref_holders_.erase(it);
      }

      for (auto& ref_holder : ready_refs)
      {
        ref_holder->bad_before_time.reset();
        ref_holder->bad.store(false, std::memory_order_release);
        available_ref_holders_.emplace_back(std::move(ref_holder));
      }
    }
  }
}
