#pragma once

#include <memory>
#include <string>
#include <utility>

#include <Commons/Coro.hpp>
#include <Commons/ExecutorPool.hpp>
#include <Commons/TextTemplateCache.hpp>
#include <Frontends/FrontendCommons/ValueTask.hpp>

namespace FrontendCommons
{
  using TextTemplateTask =
    ValueTask<AdServer::Commons::TextTemplate_var>;

  inline TextTemplateTask
  co_get_text_template(
    AdServer::Commons::TextTemplateCache_var cache,
    std::shared_ptr<AdServer::Commons::ExecutorPool> workers,
    std::string file,
    std::string service_index)
    noexcept
  {
    co_return co_await AdServer::Commons::async_callback<
      AdServer::Commons::TextTemplate_var>(
        [
          cache = std::move(cache),
          workers = std::move(workers),
          file = std::move(file),
          service_index = std::move(service_index)
        ](auto callback) mutable
        {
          cache->get_async(
            file,
            service_index.c_str(),
            [
              workers = std::move(workers),
              callback = std::move(callback)
            ](AdServer::Commons::TextTemplate_var result) mutable
            {
              if(workers)
              {
                workers->post(
                  [
                    callback = std::move(callback),
                    result = std::move(result)
                  ]() mutable
                  {
                    callback(std::move(result));
                  });
              }
              else
              {
                callback(std::move(result));
              }
            });
        });
  }
}
