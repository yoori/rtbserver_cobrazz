#ifndef ACTIONFRONTEND_REQUESTINFOFILLER_HPP
#define ACTIONFRONTEND_REQUESTINFOFILLER_HPP

#include <string>
#include <set>
#include <optional>

#include <GeoIP/IPMap.hpp>
#include <Logger/Logger.hpp>
#include <Generics/Time.hpp>

#include <Commons/UserInfoManip.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>
#include <Frontends/FrontendCommons/HTTPUtils.hpp>
#include <Frontends/FrontendCommons/FCGI.hpp>
#include <Frontends/FrontendCommons/RequestMatchers.hpp>
#include <Frontends/FrontendCommons/RequestParamProcessor.hpp>
#include <Frontends/FrontendCommons/FCGI.hpp>
#include <Frontends/CommonModule/CommonModule.hpp>
#include <Commons/LogReferrerUtils.hpp>

namespace AdServer
{
namespace Action
{
  struct RequestInfo
  {
    RequestInfo()
      : value(AdServer::CampaignSvcs::RevenueDecimal::ZERO),
        user_status(AdServer::CampaignSvcs::US_UNDEFINED),
        test_request(false),
        log_as_test(false),
        secure(false),
        redirect(true)
    {}

    Generics::Time time;
    AdServer::Commons::UserId user_id;
    AdServer::Commons::UserId utm_resolved_user_id;
    AdServer::Commons::UserId utm_cookie_user_id;
    std::string signed_client_id;
    GeoIPMapping::IPMapCity2::CityLocation location;
    std::string req_country;

    std::optional<unsigned long> campaign_id;
    std::optional<unsigned long> action_id;
    std::string order_id;
    std::optional<AdServer::CampaignSvcs::RevenueDecimal> value;
    AdServer::CampaignSvcs::UserStatus user_status;
    std::string referer;
    bool test_request;
    bool log_as_test;
    std::string peer_ip;
    std::set<unsigned long> platform_ids;
    std::string platform;
    std::string full_platform;
    std::string user_agent;

    std::string external_user_id;
    std::string source_id;
    std::string short_external_id;

    bool secure;
    bool redirect;

    std::string ifa;
  };

  typedef FrontendCommons::RequestParamProcessor<RequestInfo>
    RequestInfoParamProcessor;

  typedef ReferenceCounting::SmartPtr<RequestInfoParamProcessor>
    RequestInfoParamProcessor_var;

  class RequestInfoFiller: public FrontendCommons::HTTPExceptions
  {
  public:
    RequestInfoFiller(
      Logging::Logger* logger,
      CommonModule* common_module,
      const char* geo_ip_path,
      Commons::LogReferrer::Setting use_referer,
      bool set_uid)
      /*throw(eh::Exception)*/;

    void
    fill(RequestInfo& request_info,
      const FrontendCommons::HttpRequest& request,
      const String::SubString& service_prefix) const
      /*throw(InvalidParamException, ForbiddenException, Exception)*/;

    const Logging::Logger_var&
    logger() const noexcept;

    void
    adapt_client_id_(
      const String::SubString& in,
      RequestInfo& request_info,
      bool allow_rewrite)
      const
      /*throw(InvalidParamException)*/;

  private:
    typedef std::unique_ptr<GeoIPMapping::IPMapCity2> IPMapPtr;

    typedef Generics::GnuHashTable<
      Generics::SubStringHashAdapter, RequestInfoParamProcessor_var>
      ParamProcessorMap;

  private:
    void
    add_processor_(
      bool headers,
      bool parameters,
      const String::SubString& name,
      RequestInfoParamProcessor* processor)
      noexcept;

    static bool
    parse_utm_term_(
      RequestInfo& request_info,
      const String::SubString& utm_term)
      noexcept;

  private:
    Logging::Logger_var logger_;

    CommonModule_var common_module_;
    IPMapPtr ip_map_;
    const Commons::LogReferrer::Setting use_referrer_;
    const bool set_uid_;

    ParamProcessorMap header_processors_;
    ParamProcessorMap param_processors_;
    ParamProcessorMap cookie_processors_;
  };
} // Action
} // AdServer

namespace AdServer
{
namespace Action
{
  inline
  const Logging::Logger_var&
  RequestInfoFiller::logger() const noexcept
  {
    return logger_;
  }
}
}

#endif /*ACTIONFRONTEND_REQUESTINFOFILLER_HPP*/
