// @file SyncLogs/RouteProcessor.hpp

#pragma once

#include <ReferenceCounting/Interface.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

namespace AdServer::LogProcessing
{
  /**
   * RouteProcessor interface
   */
  class RouteProcessor: public virtual ReferenceCounting::Interface
  {
  public:
    virtual void process() noexcept = 0;

  protected:
    virtual ~RouteProcessor() noexcept {}
  };

  typedef ReferenceCounting::QualPtr<RouteProcessor>
    RouteProcessor_var;
}
