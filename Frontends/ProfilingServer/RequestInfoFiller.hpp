#ifndef PROFILINGSERVER_REQUESTINFOFILLER_HPP_
#define PROFILINGSERVER_REQUESTINFOFILLER_HPP_

#include <string>

#include <GeoIP/IPMap.hpp>
#include <Logger/Logger.hpp>
#include <Generics/Time.hpp>
#include <Generics/Uuid.hpp>
#include <Generics/GnuHashTable.hpp>

#include <Commons/Containers.hpp>
#include <Commons/UserInfoManip.hpp>
#include <Commons/ExternalUserIdUtils.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>
#include <Frontends/FrontendCommons/RequestMatchers.hpp>
#include <Frontends/FrontendCommons/UserAgentMatcher.hpp>
#include <Frontends/FrontendCommons/Location.hpp>

#include <ChannelSvcs/ChannelManagerController/ChannelManagerController.hpp>

#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>
#include <CampaignSvcs/CampaignManager/CampaignManager.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignSvcsVersionAdapter.hpp>
#include <Frontends/ProfilingServer/DMPKafkaStreamer.hpp>

namespace AdServer
{
namespace Profiling
{
  struct RequestInfo
  {
    RequestInfo()
      : filter_request(false)
    {}

    bool filter_request;

    std::string url;
    std::string keywords;
    std::string search_words;
    AdServer::Commons::ExternalUserIdArray bind_user_ids;
    FrontendCommons::CoordLocation_var coord_location;
    std::string bind_request_id;

    AdServer::Commons::UserId merge_persistent_client_id;

    std::string cohort2;
  };

  class RequestInfoFiller
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    typedef std::set<std::string> ExternalUserIdSet;

  public:
    RequestInfoFiller(
      Logging::Logger* logger,
      unsigned long colo_id,
      const ExternalUserIdSet& skip_external_ids,
      const String::SubString& bind_url_suffix,
      bool debug_on)
      /*throw(eh::Exception)*/;

    void
    fill_by_dmp_profiling_info(
      AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
      RequestInfo& request_info,
      const DMPProfilingInfoReader& dmp_profiling_info,
      const Generics::Time& now)
      noexcept;

  protected:
    void
    init_request_param_(
      AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
      RequestInfo& request_info) const
      noexcept;

    void
    fill_by_referer_(
      AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
      std::string& search_words,
      const HTTP::HTTPAddress& referer,
      bool fill_search_words = true)
      const
      noexcept;

    void
    verify_user_id_(
      const std::string& signed_user_id,
      const std::string& source_id,
      AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params)
      const
      noexcept;

    void
    split_external_user_id_(
      AdServer::Commons::ExternalUserIdArray& bind_user_ids,
      std::string& cohort2,
      const String::SubString& external_user_id,
      const String::SubString& primary_source,
      const String::SubString& secondary_source)
      noexcept;

  private:
    Logging::Logger_var logger_;
    unsigned long colo_id_;
    const ExternalUserIdSet skip_external_ids_;
    const std::string bind_url_suffix_;
    bool debug_on_;
  };
}
}

#endif /*PROFILINGSERVER_REQUESTINFOFILLER_HPP_*/
