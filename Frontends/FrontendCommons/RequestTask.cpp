#include "RequestTask.hpp"

#include <Commons/Coro/Utils.hpp>

namespace FrontendCommons
{
  RequestResult
  RequestResult::written() noexcept
  {
    RequestResult result;
    result.already_written = true;
    return result;
  }

  RequestTask::RequestTask(Handle handle) noexcept
    : handle_(handle)
  {}

  RequestTask::RequestTask(RequestTask&& rhs) noexcept
    : handle_(std::exchange(rhs.handle_, nullptr))
  {}

  RequestTask&
  RequestTask::operator=(RequestTask&& rhs) noexcept
  {
    if (this != &rhs)
    {
      if (handle_)
      {
        handle_.destroy();
      }

      handle_ = std::exchange(rhs.handle_, nullptr);
    }

    return *this;
  }

  RequestTask::~RequestTask() noexcept
  {
    if (handle_)
    {
      handle_.destroy();
    }
  }

  bool
  RequestTask::await_ready() const noexcept
  {
    return !handle_ || handle_.done();
  }

  std::coroutine_handle<>
  RequestTask::await_suspend(std::coroutine_handle<> continuation) noexcept
  {
    handle_.promise().continuation = continuation;
    return handle_;
  }

  RequestResult
  RequestTask::await_resume()
  {
    if (handle_ && handle_.promise().exception)
    {
      std::rethrow_exception(handle_.promise().exception);
    }

    if (handle_)
    {
      return std::move(handle_.promise().result);
    }

    return {};
  }

  void
  RequestTask::start_detached(
    FCGI::BaseHttpResponseWriter_var response_writer) noexcept
  {
    if (handle_)
    {
      handle_.promise().response_writer = std::move(response_writer);
      handle_.promise().detached = true;
      auto handle = std::exchange(handle_, nullptr);
      AdServer::Commons::resume_coroutine(handle);
    }
  }

  RequestTask
  RequestTask::promise_type::get_return_object() noexcept
  {
    return RequestTask(Handle::from_promise(*this));
  }

  std::suspend_always
  RequestTask::promise_type::initial_suspend() noexcept
  {
    return {};
  }

  bool
  RequestTask::promise_type::FinalAwaiter::await_ready() noexcept
  {
    return false;
  }

  void
  RequestTask::promise_type::FinalAwaiter::await_resume() noexcept
  {}

  std::coroutine_handle<>
  RequestTask::promise_type::FinalAwaiter::await_suspend(Handle handle) noexcept
  {
    auto& promise = handle.promise();
    if (promise.detached)
    {
      auto response_writer = std::move(promise.response_writer);
      auto result = std::move(promise.result);
      auto exception = std::move(promise.exception);

      try
      {
        if (!result.already_written && response_writer)
        {
          if (exception)
          {
            FCGI::HttpResponse_var response(new FCGI::HttpResponse());
            response_writer->write(500, response);
          }
          else
          {
            FCGI::HttpResponse_var response = result.response;
            if (!response)
            {
              response = new FCGI::HttpResponse();
            }

            response_writer->write(result.status, response);
          }
        }
      }
      catch(...)
      {}

      handle.destroy();
      return std::noop_coroutine();
    }

    if (promise.continuation)
    {
      return promise.continuation;
    }

    return std::noop_coroutine();
  }

  RequestTask::promise_type::FinalAwaiter
  RequestTask::promise_type::final_suspend() noexcept
  {
    return FinalAwaiter{this};
  }

  void
  RequestTask::promise_type::return_value(RequestResult value) noexcept
  {
    result = std::move(value);
  }

  void
  RequestTask::promise_type::unhandled_exception() noexcept
  {
    exception = std::current_exception();
  }
}
