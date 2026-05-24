/// @file AdFrontend/OptOutFrontendStat.hpp
#pragma once

#include <eh/Exception.hpp>
#include <Generics/Values.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>

namespace AdServer
{
  enum OptOperation
  {
    OO_NOT_DEFINED = 0,
    OO_OUT = 1,
    OO_IN  = 2,
    OO_STATUS
  };

  class OptOutFrontendStat: public ReferenceCounting::AtomicImpl
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

    OptOutFrontendStat()
      /*throw(Exception)*/;

    void consider_request(OptOperation op) noexcept;

    Generics::Values_var
    extract_stats_values();

  private:
    StatData stat_data_;
    Sync::PosixMutex mutex_;
  };

  typedef ReferenceCounting::SmartPtr<OptOutFrontendStat>
    OptOutFrontendStat_var;


}
