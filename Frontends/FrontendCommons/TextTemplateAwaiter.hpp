#pragma once

#include <memory>
#include <string>
#include <utility>

#include <Commons/TextTemplateAsyncCache.hpp>

namespace FrontendCommons
{
  using TextTemplateAwaiter =
    AdServer::Commons::TextTemplateCache::CacheAwaiter;

  inline TextTemplateAwaiter
  co_get_text_template(
    AdServer::Commons::TextTemplateCachePtr cache,
    std::shared_ptr<AdServer::Commons::ExecutorPool> workers,
    std::string file,
    std::string service_index)
    noexcept
  {
    return cache->co_get(
      std::move(workers),
      std::move(file),
      std::move(service_index));
  }
}
