#pragma once

#include <coroutine>
#include <functional>

namespace AdServer::Commons
{
  using CoroutineResumeScheduler =
    std::function<void(std::coroutine_handle<>)>;

  inline const CoroutineResumeScheduler*&
  current_coroutine_resume_scheduler_ref() noexcept
  {
    thread_local const CoroutineResumeScheduler* scheduler = nullptr;
    return scheduler;
  }

  inline const CoroutineResumeScheduler*
  current_coroutine_resume_scheduler() noexcept
  {
    return current_coroutine_resume_scheduler_ref();
  }

  class ScopedCoroutineResumeScheduler
  {
  public:
    explicit
    ScopedCoroutineResumeScheduler(const CoroutineResumeScheduler& scheduler)
      : previous_(current_coroutine_resume_scheduler_ref())
    {
      current_coroutine_resume_scheduler_ref() = &scheduler;
    }

    ~ScopedCoroutineResumeScheduler()
    {
      current_coroutine_resume_scheduler_ref() = previous_;
    }

    ScopedCoroutineResumeScheduler(const ScopedCoroutineResumeScheduler&) =
      delete;

    ScopedCoroutineResumeScheduler&
    operator=(const ScopedCoroutineResumeScheduler&) = delete;

  private:
    const CoroutineResumeScheduler* previous_;
  };
}
