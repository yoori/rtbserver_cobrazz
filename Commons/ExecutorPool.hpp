#pragma once

#include <coroutine>
#include <functional>
#include <memory>

#include <boost/asio.hpp>

#include <Commons/DelegateActiveObject.hpp>
#include <Generics/Time.hpp>

namespace AdServer::Commons
{
  class ExecutorPool:
    public DelegateActiveObject
  {
  public:
    ExecutorPool(
      Generics::ActiveObjectCallback* callback,
      unsigned long threads);

    void
    post(std::function<void()> task);

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

    std::shared_ptr<IoService> io_service_;
    std::unique_ptr<Work> io_work_;
  };
}
