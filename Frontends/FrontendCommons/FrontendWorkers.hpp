#pragma once

#include <Commons/ExecutorPool.hpp>

namespace FrontendCommons
{
  class FrontendWorkers:
    public AdServer::Commons::ExecutorPool
  {
  public:
    FrontendWorkers(
      Generics::ActiveObjectCallback* callback,
      unsigned long threads);

  protected:
    ~FrontendWorkers() noexcept override = default;
  };

  using FrontendWorkers_var =
    ReferenceCounting::SmartPtr<FrontendWorkers>;
}
