#pragma once

#include <coroutine>
#include <exception>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

#include <Commons/ExecutorPool.hpp>
#include <Commons/Coro.hpp>
#include <Commons/TextTemplateCache.hpp>

namespace FrontendCommons
{
  struct TextTemplateRequest
  {
    TextTemplateRequest(
      AdServer::Commons::TextTemplateCache_var cache_val,
      std::shared_ptr<AdServer::Commons::ExecutorPool> workers_val,
      std::string file_val,
      std::string service_index_val)
      : cache(std::move(cache_val)),
        workers(std::move(workers_val)),
        file(std::move(file_val)),
        service_index(std::move(service_index_val))
    {}

    AdServer::Commons::TextTemplateCache_var cache;
    std::shared_ptr<AdServer::Commons::ExecutorPool> workers;
    std::string file;
    std::string service_index;
  };

  class TextTemplateTask
  {
  public:
    explicit
    TextTemplateTask(std::shared_ptr<TextTemplateRequest> request)
      : state_(std::make_shared<State>()),
        request_(std::move(request))
    {}

    TextTemplateTask(const TextTemplateTask&) = delete;

    TextTemplateTask&
    operator=(const TextTemplateTask&) = delete;

    TextTemplateTask(TextTemplateTask&&) noexcept = default;

    TextTemplateTask&
    operator=(TextTemplateTask&&) noexcept = default;

    ~TextTemplateTask() noexcept
    {
      if(state_)
      {
        std::lock_guard<std::mutex> guard(state_->lock);
        state_->cancelled = true;
        state_->handle = {};
      }
    }

    bool
    await_ready() const noexcept
    {
      return false;
    }

    bool
    await_suspend(std::coroutine_handle<> handle)
    {
      {
        std::lock_guard<std::mutex> guard(state_->lock);
        state_->handle = handle;
      }

      try
      {
        auto request = request_;
        auto cache = request->cache;
        const auto& file = request->file;
        const auto* const service_index = request->service_index.c_str();

        cache->get_async(
          file,
          service_index,
          [
            state = state_,
            request = std::move(request)
          ](AdServer::Commons::TextTemplate_var result) mutable
          {
            auto workers = request->workers;
            request.reset();

            auto complete =
              [
                state = std::move(state),
                result = std::move(result)
              ]() mutable
              {
                std::coroutine_handle<> handle;
                {
                  std::lock_guard<std::mutex> guard(state->lock);
                  if(state->cancelled)
                  {
                    return;
                  }
                  state->result = std::move(result);
                  state->completed = true;
                  if(state->suspended)
                  {
                    handle = state->handle;
                  }
                }

                if(handle)
                {
                  AdServer::Commons::resume_coroutine(handle);
                }
              };

            if(workers)
            {
              workers->post(std::move(complete));
            }
            else
            {
              complete();
            }
          });
      }
      catch(...)
      {
        std::lock_guard<std::mutex> guard(state_->lock);
        state_->exception = std::current_exception();
        state_->completed = true;
      }

      std::lock_guard<std::mutex> guard(state_->lock);
      if(state_->completed)
      {
        return false;
      }

      state_->suspended = true;
      return true;
    }

    AdServer::Commons::TextTemplate_var
    await_resume()
    {
      if(state_->exception)
      {
        std::rethrow_exception(state_->exception);
      }
      return std::move(state_->result);
    }

  private:
    struct State
    {
      std::mutex lock;
      std::coroutine_handle<> handle;
      AdServer::Commons::TextTemplate_var result;
      std::exception_ptr exception;
      bool completed = false;
      bool suspended = false;
      bool cancelled = false;
    };

    std::shared_ptr<State> state_;
    std::shared_ptr<TextTemplateRequest> request_;
  };

  inline TextTemplateTask
  co_get_text_template(
    std::shared_ptr<TextTemplateRequest> request)
    noexcept
  {
    return TextTemplateTask(std::move(request));
  }

  inline TextTemplateTask
  co_get_text_template(
    AdServer::Commons::TextTemplateCache_var cache,
    std::shared_ptr<AdServer::Commons::ExecutorPool> workers,
    std::string file,
    std::string service_index)
    noexcept
  {
    return co_get_text_template(
      std::make_shared<TextTemplateRequest>(
        std::move(cache),
        std::move(workers),
        std::move(file),
        std::move(service_index)));
  }
}
