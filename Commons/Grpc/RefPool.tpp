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
  RefPool<T>::Ref::ProbeGuard::ProbeGuard(
    std::shared_ptr<RefHolder> ref_holder_val)
    : ref_holder(std::move(ref_holder_val))
  {}

  template<typename T>
  RefPool<T>::Ref::ProbeGuard::~ProbeGuard() noexcept
  {
    if (ref_holder)
    {
      if (auto pool = ref_holder->pool.lock())
      {
        pool->finish_probe_(ref_holder);
      }
    }
  }

  template<typename T>
  RefPool<T>::Ref::Ref(
    std::shared_ptr<RefHolder> ref_holder,
    const bool probe_ref)
    : ref_holder_(std::move(ref_holder)),
      probe_guard_(probe_ref ?
        std::make_shared<ProbeGuard>(ref_holder_) :
        nullptr)
  {}

  template<typename T>
  RefPool<T>::RefPool(std::vector<std::shared_ptr<T>> refs)
    : refs_(std::move(refs))
  {
    if (refs_.empty())
    {
      throw InvalidArgument("RefPool refs list is empty");
    }
  }

  template<typename T>
  RefPool<T>::~RefPool() noexcept
  {}

  template<typename T>
  void
  RefPool<T>::init_refs_()
  {
    std::unique_lock<std::shared_mutex> lock(lock_);
    if (refs_.empty())
    {
      return;
    }

    const auto pool = this->weak_from_this();
    available_ref_holders_.reserve(refs_.size());
    for (const auto& ref : refs_)
    {
      available_ref_holders_.emplace_back(
        std::make_shared<RefHolder>(ref, pool));
    }
    refs_.clear();
    refs_.shrink_to_fit();
  }

  template<typename T>
  std::optional<typename RefPool<T>::Ref>
  RefPool<T>::get_object()
  {
    // optimization using double-checked locking to reduce lock contention.
    if (try_ref_count_.load(std::memory_order_acquire) != 0)
    {
      std::unique_lock<std::shared_mutex> lock(lock_);
      if (!try_ref_holders_.empty())
      {
        auto ref_holder = std::move(try_ref_holders_.back());
        try_ref_holders_.pop_back();
        try_ref_count_.store(
          try_ref_holders_.size(),
          std::memory_order_release);
        ref_holder->probing = true;
        return Ref(ref_holder, true);
      }
    }

    const auto ref_index =
      next_ref_index_.fetch_add(1, std::memory_order_relaxed);

    std::shared_lock<std::shared_mutex> lock(lock_);
    if (available_ref_holders_.empty())
    {
      return std::nullopt;
    }

    return Ref(
      available_ref_holders_[ref_index % available_ref_holders_.size()],
      false);
  }

  template<typename T>
  void
  RefPool<T>::activate_object_()
  {
    init_refs_();
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
      std::optional<Generics::Time> previous_first_bad_time;
      if (!bad_ref_holders_.empty())
      {
        previous_first_bad_time =
          bad_ref_holders_.begin()->first.bad_before_time;
      }

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
      else if (!ref_holder->probing)
      {
        auto try_it = std::find(
          try_ref_holders_.begin(),
          try_ref_holders_.end(),
          ref_holder);
        if (try_it != try_ref_holders_.end())
        {
          try_ref_holders_.erase(try_it);
          try_ref_count_.store(
            try_ref_holders_.size(),
            std::memory_order_release);
        }
      }

      const auto new_bad_before_time = ref_holder->bad_before_time ?
        std::max(*ref_holder->bad_before_time, bad_before_time) :
        bad_before_time;

      ref_holder->bad_before_time = new_bad_before_time;
      ref_holder->probing = false;
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
  RefPool<T>::finish_probe_(
    const std::shared_ptr<RefHolder>& ref_holder) noexcept
  {
    {
      std::unique_lock<std::shared_mutex> lock(lock_);
      if (!ref_holder->probing ||
        ref_holder->bad.load(std::memory_order_acquire))
      {
        return;
      }

      ref_holder->probing = false;
      ref_holder->bad_before_time.reset();
      ref_holder->bad.store(false, std::memory_order_release);
      available_ref_holders_.emplace_back(ref_holder);
    }

    cond_.notify_one();
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
        ref_holder->probing = false;
        ref_holder->bad.store(false, std::memory_order_release);
        try_ref_holders_.emplace_back(std::move(ref_holder));
      }
      try_ref_count_.store(
        try_ref_holders_.size(),
        std::memory_order_release);
    }
  }
}
