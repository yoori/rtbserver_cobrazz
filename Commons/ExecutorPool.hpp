#pragma once

#include <coroutine>
#include <cstdint>
#include <exception>
#include <functional>
#include <memory>
#include <atomic>
#include <optional>
#include <string>
#include <vector>

#include <boost/asio.hpp>

#include <Commons/Coro/Utils.hpp>
#include <Commons/DelegateActiveObject.hpp>
#include <Generics/Time.hpp>

namespace AdServer::Commons
{
  class ExecutorPool:
    public DelegateActiveObject
  {
  public:
    using ContextIndex = std::size_t;

    struct Stats
    {
      std::uint64_t resumes_scheduled = 0;
      std::uint64_t resumes_executed = 0;
      std::uint64_t resume_schedule_failures = 0;
    };

    enum class ResumeStrategy
    {
      CurrentContext,
      AnyContext
    };

    ExecutorPool(
      Generics::ActiveObjectCallback* callback,
      unsigned long threads,
      ResumeStrategy resume_strategy,
      std::string thread_name = "asio-pool",
      unsigned long threads_per_context = 4);

    void post(std::function<void()> task, std::optional<ContextIndex> context_index = std::nullopt);

    std::optional<ContextIndex> current_context_index() const noexcept;

    void
    dispatch(std::function<void()> task, std::optional<ContextIndex> context_index = std::nullopt);

    ContextIndex
    get_next_context_index() noexcept;

    bool
    running_in_this_thread() const noexcept;

    Stats
    stats() const noexcept;

    void
    schedule(const Generics::Time& timeout, std::function<void()> task);

    class YieldAwaiter
    {
    public:
      explicit YieldAwaiter(std::shared_ptr<ExecutorPool> executor_pool);

      bool
      await_ready() const noexcept;

      void
      await_suspend(std::coroutine_handle<> handle);

      void
      await_resume() const noexcept;

    private:
      std::shared_ptr<ExecutorPool> executor_pool_;
    };

    static YieldAwaiter
    yield(std::shared_ptr<ExecutorPool> executor_pool);

    class RescheduleAwaiter
    {
    public:
      explicit RescheduleAwaiter(std::shared_ptr<ExecutorPool> executor_pool);

      bool
      await_ready() const noexcept;

      void
      await_suspend(std::coroutine_handle<> handle);

      void
      await_resume() const noexcept;

    private:
      std::shared_ptr<ExecutorPool> executor_pool_;
    };

    static RescheduleAwaiter
    reschedule(std::shared_ptr<ExecutorPool> executor_pool);

    ~ExecutorPool() noexcept override = default;

  private:
    void
    work_() noexcept override;

    void
    terminate_() noexcept override;

  private:
    using IoService = boost::asio::io_service;
    using Work = IoService::work;
    using SteadyTimer = boost::asio::steady_timer;

    struct Context
    {
      std::shared_ptr<IoService> io_service;
      std::unique_ptr<Work> io_work;
    };

    IoService&
    next_io_service() noexcept;

    IoService&
    io_service(ContextIndex context_index) noexcept;

    template<bool Detached = false>
    void resume_(std::coroutine_handle<> handle) noexcept;

    template<bool Detached = false>
    void schedule_resume_(
      std::coroutine_handle<> handle,
      std::optional<ContextIndex> context_index = std::nullopt) noexcept;

    void
    report_exception_(std::exception_ptr exception) noexcept;

    std::vector<Context> contexts_;
    std::atomic_size_t post_index_{0};
    std::atomic_size_t work_index_{0};
    std::atomic<std::uint64_t> resumes_scheduled_{0};
    std::atomic<std::uint64_t> resumes_executed_{0};
    std::atomic<std::uint64_t> resume_schedule_failures_{0};
    ResumeStrategy resume_strategy_;
    std::string thread_name_;

    static thread_local const ExecutorPool* current_executor_pool_;
    static thread_local IoService* current_io_service_;
  };

  template<bool Detached>
  inline void ExecutorPool::resume_(std::coroutine_handle<> handle) noexcept
  {
    try
    {
      if constexpr (Detached)
      {
        resume_detached_coroutine(handle);
      }
      else
      {
        resume_coroutine(handle);
      }
    }
    catch (...)
    {
      report_exception_(std::current_exception());
    }
  }

  template<bool Detached>
  inline void ExecutorPool::schedule_resume_(
    std::coroutine_handle<> handle, std::optional<ContextIndex> context_index) noexcept
  {
    resumes_scheduled_.fetch_add(1, std::memory_order_relaxed);
    try
    {
      dispatch(
        [this, handle]() mutable
        {
          resumes_executed_.fetch_add(1, std::memory_order_relaxed);
          resume_<Detached>(handle);
        },
        context_index);
    }
    catch (...)
    {
      resume_schedule_failures_.fetch_add(1, std::memory_order_relaxed);
      report_exception_(std::current_exception());
      resumes_executed_.fetch_add(1, std::memory_order_relaxed);
      resume_<Detached>(handle);
    }
  }
}
