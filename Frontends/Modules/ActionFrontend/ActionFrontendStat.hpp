/// @file AdFrontend/AcFrontendStat.hpp
#ifndef ACFRONTENDSTAT_HPP
#define ACFRONTENDSTAT_HPP

#include <eh/Exception.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <CORBACommons/CorbaAdapters.hpp>
#include <Frontends/FrontendCommons/FrontendStatistic.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignCommons.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignSvcsVersionAdapter.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>
#include <CampaignSvcs/CampaignManager/CampaignManager.hpp>

namespace AdServer
{
  class AcFrontendStat: public FrontendStat
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

    AcFrontendStat(
      Logging::Logger* logger,
      const CORBACommons::CorbaObjectRef& stats_collector_ref,
      Generics::Planner* shep_ptr,
      const Generics::Time& dump_period,
      Generics::ActiveObjectCallback* callback,
      const char* host_name = 0)
      /*throw(Exception)*/;

    void consider_request(
      const bool test_request,
      const AdServer::CampaignSvcs::UserStatus user_status) noexcept;

    void consider_request(
      const AdServer::CampaignSvcs::CampaignManager::ActionInfo& verify_action_info)
      noexcept;

    virtual Generics::Values_var
    extract_stats_values();

  private:
    StatData stat_data_;
    Sync::PosixMutex mutex_;
  };

  typedef ReferenceCounting::SmartPtr<AcFrontendStat>
    AcFrontendStat_var;


}

#endif
