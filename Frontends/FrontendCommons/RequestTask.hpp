#pragma once

#include <coroutine>
#include <exception>
#include <utility>

#include "HttpResponse.hpp"

namespace FrontendCommons
{
  struct RequestResult
  {
    int status = 200;
    FCGI::HttpResponse_var response;
    bool already_written = false;

    static RequestResult
    written() noexcept;
  };

  class RequestTask
  {
  public:
    struct promise_type;

    using Handle = std::coroutine_handle<promise_type>;

    explicit RequestTask(Handle handle) noexcept;

    RequestTask(const RequestTask&) = delete;
    RequestTask& operator=(const RequestTask&) = delete;

    RequestTask(RequestTask&& rhs) noexcept;
    RequestTask& operator=(RequestTask&& rhs) noexcept;

    ~RequestTask() noexcept;

    bool
    await_ready() const noexcept;

    std::coroutine_handle<>
    await_suspend(std::coroutine_handle<> continuation) noexcept;

    RequestResult
    await_resume();

    void
    start_detached(FCGI::BaseHttpResponseWriter_var response_writer);

    struct promise_type
    {
      struct FinalAwaiter
      {
        promise_type* promise;

        bool
        await_ready() noexcept;

        void
        await_resume() noexcept;

        std::coroutine_handle<>
        await_suspend(Handle handle) noexcept;
      };

      RequestTask
      get_return_object() noexcept;

      std::suspend_always
      initial_suspend() noexcept;

      FinalAwaiter
      final_suspend() noexcept;

      void
      return_value(RequestResult result) noexcept;

      void
      unhandled_exception() noexcept;

      std::exception_ptr exception;
      RequestResult result;
      FCGI::BaseHttpResponseWriter_var response_writer;
      std::coroutine_handle<> continuation;
      bool detached = false;
    };

  private:
    Handle handle_;
  };
}
