#ifndef ADINSTFRONTEND_REQUESTINFOFILLER_HPP
#define ADINSTFRONTEND_REQUESTINFOFILLER_HPP

#include <string>

#include <GeoIP/IPMap.hpp>
#include <Logger/Logger.hpp>
#include <Generics/Time.hpp>
#include <Generics/Uuid.hpp>
#include <Generics/GnuHashTable.hpp>

#include <Commons/IPCrypter.hpp>
#include <Commons/UserInfoManip.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>
#include <Frontends/FrontendCommons/HTTPUtils.hpp>
#include <Frontends/FrontendCommons/RequestMatchers.hpp>
#include <Frontends/FrontendCommons/RequestParamProcessor.hpp>
#include <Frontends/FrontendCommons/FCGI.hpp>
#include <Frontends/CommonModule/CommonModule.hpp>

namespace AdServer
{
namespace Instantiate
{
  struct RequestInfo
  {
    struct CreativeInfo
    {
      CreativeInfo()
        : ccid(0),
          ccg_keyword_id(0),
          ctr(CampaignSvcs::RevenueDecimal::ZERO)
      {}

      unsigned long ccid;
      unsigned long ccg_keyword_id;
      CampaignSvcs::RevenueDecimal ctr;
    };

    typedef std::list<CreativeInfo> CreativeList;
    typedef std::list<AdServer::Commons::RequestId> RequestIdList;
    typedef std::list<unsigned long> AccountIdList;

    RequestInfo()
      : request_type(),
        random(CampaignSvcs::RANDOM_PARAM_MAX),
        format("html"),
        secure(false),
        test_request(false),
        log_as_test(false),
        colo_id(0),
        publisher_account_id(0),
        publisher_site_id(0),
        user_status(AdServer::CampaignSvcs::US_UNDEFINED),
        tag_id(0),
        tag_size_id(0),
        creative_id(0),
        remove_merged_uid(false),
        hpos(CampaignSvcs::UNDEFINED_PUB_POSITION_BOTTOM),
        video_width(0),
        video_height(0),
        emulate_click(false),
        consider_request(false),
        enabled_notice(false),
        set_uid(false),
        full_referer_hash(0),
        short_referer_hash(0),
        set_cookie(true)
    {}

    Generics::Time time;
    Generics::Time bid_time;
    AdServer::Commons::RequestId global_request_id;
    unsigned long request_type;
    unsigned long random;
    std::string format;
    bool secure;
    bool test_request;
    bool log_as_test;
    unsigned long colo_id;
    unsigned long publisher_account_id;
    unsigned long publisher_site_id;
    std::string external_user_id;
    std::string source_id;
    FrontendCommons::Location_var location;
    FrontendCommons::CoordLocation_var coord_location;
    AdServer::Commons::Optional<unsigned long> user_id_hash_mod;
    std::string referer;
    std::string security_token;
    std::string pub_impr_track_url;
    std::string preclick_url;
    std::string click_prefix_url;
    std::string original_url;
    AdServer::Commons::UserId track_user_id;
    AdServer::Commons::UserId user_id;
    AdServer::CampaignSvcs::UserStatus user_status;
    AdServer::Commons::UserId temp_client_id;
    std::string signed_user_id;
    std::string peer_ip;
    std::string cohort;

    std::string passback_type;
    std::string passback_url;

    unsigned long tag_id;
    unsigned long tag_size_id;
    CreativeList creatives;
    unsigned long creative_id;
    std::string ext_tag_id;
    RequestIdList request_ids;
    AccountIdList pubpixel_accounts;
    std::string encrypted_user_ip;

    AdServer::Commons::UserId temp_user_id;
    bool remove_merged_uid;

    std::string tanx_price;
    std::string open_price; // price in open view
    std::string openx_price;
    std::string liverail_price;
    std::string baidu_price;
    std::string google_price;

    std::string campaign_manager_index;
    unsigned long hpos;
    unsigned long video_width;
    unsigned long video_height;
    std::string ext_track_params;
    bool emulate_click;

    bool consider_request;
    // next fields actual only if consider_request is true
    bool enabled_notice;
    bool set_uid;

    std::string client_app;
    std::string client_app_version;
    std::string user_agent;
    std::string web_browser;
    std::set<unsigned long> platform_ids;
    std::string platform;
    std::string full_platform;
    unsigned long full_referer_hash;
    unsigned long short_referer_hash;
    bool set_cookie;
    Commons::Optional<AdServer::CampaignSvcs::RevenueDecimal> pub_imp_revenue;

    std::map<std::string, std::string> tokens;
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
      const String::SubString& ip_key)
      /*throw(eh::Exception)*/;

    void
    fill(RequestInfo& request_info,
      const FCGI::HttpRequest& request) const
      /*throw(InvalidParamException, ForbiddenException, Exception)*/;

    const Logging::Logger_var&
    logger() const noexcept;

    void
    adapt_client_id_(
      const String::SubString& in,
      RequestInfo& request_info,
      bool persistent)
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

  private:
    Logging::Logger_var logger_;

    CommonModule_var common_module_;
    IPMapPtr ip_map_;

    ParamProcessorMap header_processors_;
    ParamProcessorMap param_processors_;
    ParamProcessorMap cookie_processors_;

    Commons::IPCrypter_var ip_crypter_;
  };
} // Instantiate
} // AdServer

namespace AdServer
{
namespace Instantiate
{
  inline
  const Logging::Logger_var&
  RequestInfoFiller::logger() const noexcept
  {
    return logger_;
  }
}
}

#endif /*ADINSTFRONTEND_REQUESTINFOFILLER_HPP*/
