#include "ActivityGate.hpp"

#include <cassert>
#include <Sync/Condition.hpp>

namespace AdServer::Commons
{
  ActivityGate::Guard::Guard(ActivityGate* gate) noexcept
    : gate_(gate)
  {}

  ActivityGate::Guard::~Guard() noexcept
  {
    reset();
  }

  void
  ActivityGate::Guard::reset() noexcept
  {
    if (gate_)
    {
      auto* gate = gate_;
      gate_ = nullptr;
      gate->leave_();
    }
  }

  ActivityGate::Guard::Guard(Guard&& rhs) noexcept
    : gate_(rhs.gate_)
  {
    rhs.gate_ = nullptr;
  }

  ActivityGate::Guard::operator bool() const noexcept
  {
    return gate_ != nullptr;
  }

  ActivityGate::Guard
  ActivityGate::enter() noexcept
  {
    if (closed_.load(std::memory_order_acquire))
    {
      return Guard();
    }

    running_.fetch_add(1, std::memory_order_acq_rel);
    if (closed_.load(std::memory_order_acquire))
    {
      leave_();
      return Guard();
    }

    return Guard(this);
  }

  void
  ActivityGate::wait_for_activities()
  {
    Sync::ConditionalGuard guard(cond_);
    waiters_.fetch_add(1, std::memory_order_acq_rel);
    while (running_.load(std::memory_order_acquire) != 0)
    {
      guard.wait();
    }
    waiters_.fetch_sub(1, std::memory_order_acq_rel);
  }

  bool
  ActivityGate::has_activities() const noexcept
  {
    return running_.load(std::memory_order_acquire) != 0;
  }

  void
  ActivityGate::activate_object_()
  {
    assert(running_.load(std::memory_order_acquire) == 0);
    closed_.store(false, std::memory_order_release);
  }

  void
  ActivityGate::deactivate_object_()
  {
    closed_.store(true, std::memory_order_release);
  }

  bool
  ActivityGate::wait_more_()
  {
    return running_.load(std::memory_order_acquire) != 0;
  }

  void
  ActivityGate::leave_() noexcept
  {
    const auto previous = running_.fetch_sub(1, std::memory_order_acq_rel);
    assert(previous > 0);
    if (previous == 1 && (closed_.load(std::memory_order_acquire) ||
      waiters_.load(std::memory_order_acquire) != 0))
    {
      Sync::PosixGuard guard(cond_);
      cond_.broadcast();
    }
  }
}
