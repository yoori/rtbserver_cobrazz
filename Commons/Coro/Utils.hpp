#pragma once

#include <coroutine>
#include <Commons/Coro/ScopedCoroutineResumeScheduler.hpp>

namespace AdServer::Commons
{
  inline bool& current_coroutine_symmetric_transfer_ref() noexcept
  {
    thread_local bool enabled = false;
    return enabled;
  }

  // Forked starts use the runner; nested awaits may transfer directly without recursive resume().
  class ScopedCoroutineResumeQueue final
  {
  public:
    explicit ScopedCoroutineResumeQueue(
      const CoroutineResumeScheduler& scheduler, bool symmetric_transfer = false) noexcept;
    ~ScopedCoroutineResumeQueue() noexcept;
    ScopedCoroutineResumeQueue(const ScopedCoroutineResumeQueue&) = delete;
    ScopedCoroutineResumeQueue& operator=(const ScopedCoroutineResumeQueue&) = delete;

  private:
    const CoroutineResumeScheduler* previous_;
    bool previous_transfer_;
  };

  void
  resume_coroutine(std::coroutine_handle<> handle);
}
