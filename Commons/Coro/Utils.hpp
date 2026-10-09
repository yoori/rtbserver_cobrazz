#pragma once

#include <coroutine>
#include <Commons/Coro/ScopedCoroutineResumeScheduler.hpp>

namespace AdServer::Commons
{
  // Redirect nested starts/continuations into a cooperative runner, not the thread trampoline.
  class ScopedCoroutineResumeQueue final
  {
  public:
    explicit ScopedCoroutineResumeQueue(const CoroutineResumeScheduler& scheduler) noexcept;
    ~ScopedCoroutineResumeQueue() noexcept;
    ScopedCoroutineResumeQueue(const ScopedCoroutineResumeQueue&) = delete;
    ScopedCoroutineResumeQueue& operator=(const ScopedCoroutineResumeQueue&) = delete;

  private:
    const CoroutineResumeScheduler* previous_;
  };

  void
  resume_coroutine(std::coroutine_handle<> handle);
}
