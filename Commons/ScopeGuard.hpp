#pragma once

#include <utility>

namespace AdServer::Commons
{
  template<typename Functor>
  class ScopeGuard
  {
  public:
    explicit ScopeGuard(Functor&& functor) noexcept(
      noexcept(Functor(std::forward<Functor>(functor))))
      : functor_(std::forward<Functor>(functor)),
        active_(true)
    {}

    ScopeGuard(ScopeGuard&& other) noexcept(
      noexcept(Functor(std::move(other.functor_))))
      : functor_(std::move(other.functor_)),
        active_(other.active_)
    {
      other.release();
    }

    ~ScopeGuard() noexcept
    {
      if(active_)
      {
        functor_();
      }
    }

    ScopeGuard(const ScopeGuard&) = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;
    ScopeGuard& operator=(ScopeGuard&&) = delete;

    void release() noexcept
    {
      active_ = false;
    }

  private:
    Functor functor_;
    bool active_;
  };

  template<typename Functor>
  ScopeGuard<std::decay_t<Functor>>
  make_scope_guard(Functor&& functor) noexcept(
    noexcept(ScopeGuard<std::decay_t<Functor>>(std::forward<Functor>(functor))))
  {
    return ScopeGuard<std::decay_t<Functor>>(
      std::forward<Functor>(functor));
  }
}
