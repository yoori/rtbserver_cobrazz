#include "Utils.hpp"

#include <deque>
#include <exception>
#include <utility>

namespace AdServer::Commons
{
  namespace
  {
    thread_local const CoroutineResumeScheduler* resume_queue = nullptr;

    struct ResumeState final
    {
      std::deque<std::coroutine_handle<>> pending;
      bool draining = false;
    };

    ResumeState&
    resume_state() noexcept
    {
      thread_local ResumeState state;
      return state;
    }

    class DrainGuard final
    {
    public:
      explicit DrainGuard(bool& draining) noexcept;

      ~DrainGuard() noexcept;

      DrainGuard(const DrainGuard&) = delete;
      DrainGuard& operator=(const DrainGuard&) = delete;

    private:
      bool& draining_;
    };

    DrainGuard::DrainGuard(bool& draining) noexcept : draining_(draining)
    {
      draining_ = true;
    }

    DrainGuard::~DrainGuard() noexcept
    {
      draining_ = false;
    }
  }

  ScopedCoroutineResumeQueue::ScopedCoroutineResumeQueue(
    const CoroutineResumeScheduler& scheduler, bool symmetric_transfer) noexcept
    : previous_(std::exchange(resume_queue, &scheduler)),
      previous_transfer_(
        std::exchange(current_coroutine_symmetric_transfer_ref(), symmetric_transfer))
  {}

  ScopedCoroutineResumeQueue::~ScopedCoroutineResumeQueue() noexcept
  {
    resume_queue = previous_;
    current_coroutine_symmetric_transfer_ref() = previous_transfer_;
  }

  void
  resume_coroutine(std::coroutine_handle<> handle)
  {
    if (!handle)
    {
      return;
    }

    if (resume_queue)
    {
      (*resume_queue)(handle);
      return;
    }

    auto& state = resume_state();
    state.pending.push_back(handle);
    if (state.draining)
    {
      return;
    }

    std::exception_ptr exception;
    DrainGuard guard(state.draining);
    // A failed handle must not strand continuations queued by earlier nested resumes.
    while (!state.pending.empty())
    {
      const auto next = state.pending.front();
      state.pending.pop_front();

      try
      {
        next.resume();
      }
      catch (...)
      {
        if (!exception)
        {
          exception = std::current_exception();
        }
      }
    }

    if (exception)
    {
      std::rethrow_exception(exception);
    }
  }
}
