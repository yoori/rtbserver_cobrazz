/// @file FrontendCommons/FrontendStatistic.hpp
#pragma once

#include <eh/Exception.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Generics/TaskRunner.hpp>
#include <CORBACommons/CorbaAdapters.hpp>
#include <Commons/CorbaConfig.hpp>
#include <Controlling/StatsDumper/StatsDumper.hpp>

namespace AdServer
{
  class FrontendStat:
    public Generics::RefCountableCompositeActiveObject
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    typedef CORBACommons::CorbaObjectRef StatsCollectorRef;

    FrontendStat(
      Logging::Logger* logger,
      const StatsCollectorRef& stats_collector_ref,
      Generics::Planner* shep_ptr,
      const Generics::Time& dump_period,
      Generics::ActiveObjectCallback* callback,
      const char* host_name = 0)
      /*throw(Exception)*/;

    virtual Generics::Values_var
    extract_stats_values() = 0;

    template<typename CorbaRefConfig>
    static void
    read_stats_collector_ref(
      StatsCollectorRef& stats_collector_ref,
      const CorbaRefConfig& corba_ref_config)
      /*throw(eh::Exception)*/
    {
      Config::CorbaConfigReader::read_corba_ref(
        corba_ref_config,
        stats_collector_ref);
    }

  protected:
    virtual
    ~FrontendStat() noexcept {};

    Logging::Logger_var logger_;
    Controlling::StatsDumper stats_dumper_;
  };

}
