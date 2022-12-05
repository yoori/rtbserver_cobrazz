// @file SyncLogs/RouteProcessor.hpp

#ifndef SYNCLOGS_ROUTEPROCESSOR_HPP
#define SYNCLOGS_ROUTEPROCESSOR_HPP

#include <ReferenceCounting/Interface.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

namespace AdServer
{
namespace LogProcessing
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
}

#endif /*SYNCLOGS_ROUTEPROCESSOR_HPP*/
