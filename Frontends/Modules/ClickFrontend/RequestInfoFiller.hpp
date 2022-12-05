#ifndef CLICKFRONTEND_REQUESTINFIFILLER_HPP
#define CLICKFRONTEND_REQUESTINFIFILLER_HPP

#include <string>

#include <GeoIP/IPMap.hpp>
#include <Logger/Logger.hpp>
#include <Generics/Time.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>
#include <Commons/CorbaTypes.hpp>

#include <Frontends/FrontendCommons/RequestParamProcessor.hpp>
#include <Frontends/FrontendCommons/HTTPUtils.hpp>
#include <Frontends/FrontendCommons/FCGI.hpp>
#include <Frontends/CommonModule/CommonModule.hpp>

namespace AdServer
{
namespace ClickFE
{
  struct RequestInfo
  {
    RequestInfo()
      : user_id_hash_mod_value(0),
        user_id_hash_mod_defined(false),
        colo_id(0),
        tag_id(0),
        tag_size_id(0),
        ccid(0),
        ccg_keyword_id(0),
        creative_id(0),
        f_flag_value(0),
        user_status(AdServer::CampaignSvcs::US_UNDEFINED)
    {}

    // from parameters
    std::string relocate;
    std::string preclick_url;
    std::string click_prefix;
    AdServer::Commons::RequestId request_id;
    unsigned long user_id_hash_mod_value;
    bool user_id_hash_mod_defined;
    unsigned long colo_id;
    unsigned long tag_id;
    unsigned long tag_size_id;
    unsigned long ccid;
    unsigned long ccg_keyword_id;
    unsigned long creative_id;
    CampaignSvcs::RevenueDecimal ctr;
    bool creative_id_error_flag;
    std::string campaign_manager_index;
    Generics::Time request_time;
    Generics::Time bid_time;
    AdServer::Commons::UserId match_user_id;
    unsigned long f_flag_value;
    bool use_click_template;
    std::list<std::string> markers;
    std::map<std::string, std::string> tokens;

    // headers
    std::string referer;
    std::string peer_ip;

    AdServer::CampaignSvcs::UserStatus user_status;
    AdServer::Commons::UserId cookie_user_id;

    AdServer::Commons::Optional<bool> set_uid_param;
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
      const char* geo_ip_path)
      /*throw(eh::Exception)*/;

    void
    fill(RequestInfo& request_info,
      const FCGI::HttpRequest& request,
      const FrontendCommons::ParsedParamsMap& parsed_params) const
      /*throw(InvalidParamException, ForbiddenException, Exception)*/;

    const Logging::Logger_var&
    logger() const noexcept;

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
    unsigned long colo_id_;
    const CommonModule_var common_module_;

    IPMapPtr ip_map_;

    ReferenceCounting::SmartPtr<
      FrontendCommons::ExtRequestParamProcessor<RequestInfo> > tokens_processor_;
    ParamProcessorMap header_processors_;
    ParamProcessorMap param_processors_;
  };
}
}

namespace AdServer
{
namespace ClickFE
{
  inline
  const Logging::Logger_var&
  RequestInfoFiller::logger() const noexcept
  {
    return logger_;
  }
}
}
#endif //CLICKFRONTEND_REQUESTINFIFILLER_HPP

