#pragma once

#include <coroutine>
#include <functional>
#include <memory>
#include <atomic>
#include <optional>
#include <string>
#include <vector>

#include <boost/asio.hpp>

#include <Commons/DelegateActiveObject.hpp>
#include <Generics/Time.hpp>

namespace AdServer::Commons
{
  class ExecutorPool:
    public DelegateActiveObject
  {
  public:
    using ContextIndex = std::size_t;

    enum class ResumeStrategy
    {
      CurrentContext,
      AnyContext
    };

    ExecutorPool(
      Generics::ActiveObjectCallback* callback,
      unsigned long threads,
      ResumeStrategy resume_strategy,
      std::string thread_name = "asio-pool");

    void
    post(std::function<void()> task);

    void
    dispatch(
      std::function<void()> task,
      std::optional<ContextIndex> context_index = std::nullopt);

    ContextIndex
    get_next_context_index() noexcept;

    bool
    running_in_this_thread() const noexcept;

    void
    schedule(
      const Generics::Time& timeout,
      std::function<void()> task);

    class YieldAwaiter
    {
    public:
      explicit YieldAwaiter(std::shared_ptr<ExecutorPool> executor_pool);

      bool
      await_ready() const noexcept;

      void
      await_suspend(std::coroutine_handle<> handle) noexcept;

      void
      await_resume() const noexcept;

    private:
      std::shared_ptr<ExecutorPool> executor_pool_;
    };

    static YieldAwaiter
    yield(std::shared_ptr<ExecutorPool> executor_pool);

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

    std::vector<Context> contexts_;
    std::atomic_size_t post_index_{0};
    std::atomic_size_t work_index_{0};
    ResumeStrategy resume_strategy_;
    std::string thread_name_;

    static thread_local const ExecutorPool* current_executor_pool_;
    static thread_local IoService* current_io_service_;
  };
}
