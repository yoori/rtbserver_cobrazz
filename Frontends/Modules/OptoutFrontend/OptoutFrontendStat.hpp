/// @file AdFrontend/OptOutFrontendStat.hpp
#ifndef OPT_OUT_FRONTENDSTAT_HPP
#define OPT_OUT_FRONTENDSTAT_HPP

#include <eh/Exception.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <CORBACommons/CorbaAdapters.hpp>
#include <Frontends/FrontendCommons/FrontendStatistic.hpp>

namespace AdServer
{
  enum OptOperation
  {
    OO_NOT_DEFINED = 0,
    OO_OUT = 1,
    OO_IN  = 2,
    OO_STATUS
  };

  class OptOutFrontendStat: public FrontendStat
  {
    struct StatData
    {
      unsigned long opt_in_user;
      unsigned long non_opt_on_user;
      unsigned long opt_status;

      StatData&
      operator +=(const StatData& rhs) noexcept;
    };

  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    OptOutFrontendStat(
      Logging::Logger* logger,
      const CORBACommons::CorbaObjectRef& stats_collector_ref,
      Generics::Planner* shep_ptr,
      const Generics::Time& dump_period,
      Generics::ActiveObjectCallback* callback,
      const char* host_name = 0)
      /*throw(Exception)*/;

    void consider_request(OptOperation op) noexcept;

    virtual Generics::Values_var
    extract_stats_values();

  private:
    StatData stat_data_;
    Sync::PosixMutex mutex_;
  };

  typedef ReferenceCounting::SmartPtr<OptOutFrontendStat>
    OptOutFrontendStat_var;


}

#endif
