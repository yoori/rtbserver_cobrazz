/// @file AdFrontend/AcFrontendStat.hpp
#pragma once

#include <eh/Exception.hpp>
#include <Generics/Values.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignCommons.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>

namespace AdServer
{
  class AcFrontendStat: public ReferenceCounting::AtomicImpl
  {
    struct StatData
    {
      unsigned long opt_in_user;
      unsigned long non_opt_on_user;

      StatData&
      operator +=(const StatData& rhs) noexcept;
    };

  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    AcFrontendStat()
      /*throw(Exception)*/;

    void consider_request(bool test_request, unsigned long user_status)
      noexcept;

    Generics::Values_var
    extract_stats_values();

  private:
    StatData stat_data_;
    Sync::PosixMutex mutex_;
  };

  typedef ReferenceCounting::SmartPtr<AcFrontendStat>
    AcFrontendStat_var;


}
