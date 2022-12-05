/// @file FrontendCommons/FrontendStatistic.hpp
#ifndef FRONTEND_COMMONS_FRONTENDSTAT_HPP
#define FRONTEND_COMMONS_FRONTENDSTAT_HPP

#include <eh/Exception.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Generics/TaskRunner.hpp>
#include <CORBACommons/CorbaAdapters.hpp>
#include <Controlling/StatsDumper/StatsDumper.hpp>

namespace AdServer
{
  class FrontendStat:
    public Generics::CompositeActiveObject,
    public ReferenceCounting::AtomicImpl
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    FrontendStat(
      Logging::Logger* logger,
      const CORBACommons::CorbaObjectRef& stats_collector_ref,
      Generics::Planner* shep_ptr,
      const Generics::Time& dump_period,
      Generics::ActiveObjectCallback* callback,
      const char* host_name = 0)
      /*throw(Exception)*/;

    virtual Generics::Values_var
    extract_stats_values() = 0;

  protected:
    virtual
    ~FrontendStat() noexcept {};

    Logging::Logger_var logger_;
    Controlling::StatsDumper stats_dumper_;
  };

}

#endif
