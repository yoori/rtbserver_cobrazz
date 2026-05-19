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
    if (gate_)
    {
      gate_->leave_();
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
    if (previous == 1 && closed_.load(std::memory_order_acquire))
    {
      Sync::PosixGuard guard(cond_);
      cond_.broadcast();
    }
  }
}
