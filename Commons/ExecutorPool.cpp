#include "ExecutorPool.hpp"

#include <Commons/ThreadName.hpp>

namespace AdServer::Commons
{
  ExecutorPool::ExecutorPool(
    Generics::ActiveObjectCallback* callback,
    unsigned long threads)
    : DelegateActiveObject(callback, threads, 1024 * 1024),
      io_service_(std::make_shared<IoService>()),
      io_work_(std::make_unique<Work>(*io_service_))
  {}

  void
  ExecutorPool::post(std::function<void()> task)
  {
    io_service_->post(std::move(task));
  }

  void
  ExecutorPool::schedule(
    const Generics::Time& timeout,
    std::function<void()> task)
  {
    const auto timeout_us = timeout.microseconds();
    auto timer = std::make_shared<SteadyTimer>(*io_service_);
    timer->expires_after(
      std::chrono::microseconds(timeout_us > 0 ? timeout_us : 0));
    timer->async_wait(
      [timer, task = std::move(task)](const boost::system::error_code& error)
      {
        if(!error)
        {
          task();
        }
      });
  }

  ExecutorPool::YieldAwaiter::YieldAwaiter(
    std::shared_ptr<ExecutorPool> executor_pool)
    : executor_pool_(std::move(executor_pool))
  {}

  bool
  ExecutorPool::YieldAwaiter::await_ready() const noexcept
  {
    return false;
  }

  void
  ExecutorPool::YieldAwaiter::await_suspend(
    std::coroutine_handle<> handle) noexcept
  {
    executor_pool_->post([handle]() mutable { handle.resume(); });
  }

  void
  ExecutorPool::YieldAwaiter::await_resume() const noexcept
  {}

  ExecutorPool::YieldAwaiter
  ExecutorPool::yield(std::shared_ptr<ExecutorPool> executor_pool)
  {
    return YieldAwaiter(std::move(executor_pool));
  }

  void
  ExecutorPool::work_() noexcept
  {
    set_current_thread_name("asio-pool");

    while(active())
    {
      try
      {
        io_service_->run();
      }
      catch(...)
      {}
    }
  }

  void
  ExecutorPool::terminate_() noexcept
  {
    io_work_.reset();
    io_service_->stop();
  }
}
