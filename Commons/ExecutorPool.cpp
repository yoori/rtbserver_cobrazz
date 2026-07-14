#include "ExecutorPool.hpp"

#include <Commons/Coro/ScopedCoroutineResumeScheduler.hpp>
#include <Commons/Coro/Utils.hpp>
#include <Commons/ThreadName.hpp>

namespace AdServer::Commons
{
  thread_local const ExecutorPool* ExecutorPool::current_executor_pool_ = nullptr;
  thread_local ExecutorPool::IoService* ExecutorPool::current_io_service_ = nullptr;

  ExecutorPool::ExecutorPool(
    Generics::ActiveObjectCallback* callback,
    unsigned long threads,
    ResumeStrategy resume_strategy,
    std::string thread_name)
    : DelegateActiveObject(callback, threads ? threads : 1, 1024 * 1024)
    , resume_strategy_(resume_strategy)
    , thread_name_(std::move(thread_name))
  {
    const auto context_count = threads ? threads : 1;
    contexts_.reserve(context_count);
    for(unsigned long i = 0; i < context_count; ++i)
    {
      auto io_service = std::make_shared<IoService>();
      contexts_.emplace_back(Context{
        io_service,
        std::make_unique<Work>(*io_service)});
    }
  }

  void
  ExecutorPool::post(std::function<void()> task)
  {
    next_io_service().post(std::move(task));
  }

  void
  ExecutorPool::dispatch(
    std::function<void()> task,
    std::optional<ContextIndex> context_index)
  {
    if(context_index)
    {
      boost::asio::dispatch(
        io_service(*context_index),
        std::move(task));
    }
    else if(current_executor_pool_ == this && current_io_service_)
    {
      boost::asio::dispatch(*current_io_service_, std::move(task));
    }
    else
    {
      boost::asio::dispatch(next_io_service(), std::move(task));
    }
  }

  bool
  ExecutorPool::running_in_this_thread() const noexcept
  {
    return current_executor_pool_ == this && current_io_service_;
  }

  ExecutorPool::ContextIndex
  ExecutorPool::get_next_context_index() noexcept
  {
    return post_index_.fetch_add(
      1,
      std::memory_order_relaxed) % contexts_.size();
  }

  void
  ExecutorPool::schedule(
    const Generics::Time& timeout,
    std::function<void()> task)
  {
    const auto timeout_us = timeout.microseconds();
    auto timer = std::make_shared<SteadyTimer>(next_io_service());
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
    return executor_pool_->running_in_this_thread();
  }

  void
  ExecutorPool::YieldAwaiter::await_suspend(std::coroutine_handle<> handle) noexcept
  {
    const auto context_index = executor_pool_->get_next_context_index();
    executor_pool_->dispatch(
      [handle]() mutable
      {
        resume_coroutine(handle);
      },
      context_index);
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
    set_current_thread_name(thread_name_);

    const auto context_index = work_index_.fetch_add(
      1,
      std::memory_order_relaxed) % contexts_.size();
    auto& io_service = *contexts_[context_index].io_service;
    current_executor_pool_ = this;
    current_io_service_ = &io_service;

    CoroutineResumeScheduler resume_scheduler;
    if(resume_strategy_ == ResumeStrategy::CurrentContext)
    {
      resume_scheduler =
        [this, context_index](std::coroutine_handle<> handle)
        {
          dispatch(
            [handle]() mutable
            {
              resume_coroutine(handle);
            },
            context_index);
        };
    }
    else
    {
      resume_scheduler =
        [this](std::coroutine_handle<> handle)
        {
          dispatch(
            [handle]() mutable
            {
              resume_coroutine(handle);
            });
        };
    }

    ScopedCoroutineResumeScheduler scheduler_scope(resume_scheduler);

    while(active())
    {
      try
      {
        io_service.run();
      }
      catch(...)
      {}
    }

    current_executor_pool_ = nullptr;
    current_io_service_ = nullptr;
  }

  void
  ExecutorPool::terminate_() noexcept
  {
    for(auto& context : contexts_)
    {
      context.io_work.reset();
      context.io_service->stop();
    }
  }

  ExecutorPool::IoService&
  ExecutorPool::next_io_service() noexcept
  {
    return io_service(get_next_context_index());
  }

  ExecutorPool::IoService&
  ExecutorPool::io_service(ContextIndex context_index) noexcept
  {
    return *contexts_[context_index].io_service;
  }
}
