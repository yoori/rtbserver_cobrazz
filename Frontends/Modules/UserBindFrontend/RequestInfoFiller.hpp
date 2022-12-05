#ifndef USERBINDFRONTEND_REQUESTINFOFILLER_HPP_
#define USERBINDFRONTEND_REQUESTINFOFILLER_HPP_

#include <Generics/GnuHashTable.hpp>
#include <GeoIP/IPMap.hpp>
#include <HTTP/Http.hpp>
#include <Logger/Logger.hpp>

#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>
#include <Frontends/FrontendCommons/FCGI.hpp>
#include <Frontends/FrontendCommons/RequestParamProcessor.hpp>
#include <Frontends/CommonModule/CommonModule.hpp>

namespace AdServer
{
namespace UserBind
{
  struct RequestInfo: public ReferenceCounting::AtomicImpl
  {
    RequestInfo()
    :
      user_status(AdServer::CampaignSvcs::US_UNDEFINED),
      passback(false),
      colo_id(0),
      generate_external_id(false),
      secure(false),
      google_error(0),
      delete_op(false),
      disable_secure_redirect(false)
    {}

    void
    dump(std::ostream& os) const noexcept;

    AdServer::CampaignSvcs::UserStatus user_status;

    Generics::Time time;
    bool passback;
    AdServer::Commons::UserId user_id;
    std::string cohort;
    unsigned long colo_id;
    std::string source_id;
    FrontendCommons::Location_var location;
    std::string peer_ip;
    std::string x_peer_ip;//IP from header x-forwarded-for
    std::string passback_url;

    std::string short_external_id;
    std::string user_agent;
    std::string referer;
    bool generate_external_id;
    bool secure;
    std::string ssp_id;

    std::string external_id;

    std::string push_data;
    unsigned long google_error;

    AdServer::Commons::UserId add_user_id;
    std::string ga_user_id;
    std::string gclu_user_id;
    std::string ym_user_id;

    bool delete_op;

    std::string server_host;

    bool disable_secure_redirect;

  protected:
    virtual ~RequestInfo() noexcept
    {}
  };

  typedef ReferenceCounting::SmartPtr<RequestInfo> RequestInfo_var;

  typedef ReferenceCounting::ConstPtr<RequestInfo> CRequestInfo_var;

  typedef FrontendCommons::RequestParamProcessor<RequestInfo>
    RequestInfoParamProcessor;

  typedef ReferenceCounting::SmartPtr<RequestInfoParamProcessor>
    RequestInfoParamProcessor_var;

  class RequestInfoFiller: public FrontendCommons::HTTPExceptions
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    typedef std::set<std::string> ExternalUserIdSet;
    typedef std::vector<std::string> AllowedPassbackDomainArray;

  public:
    RequestInfoFiller(
      Logging::Logger* logger,
      CommonModule* common_module,
      const char* geo_ip_path,
      const ExternalUserIdSet& skip_external_ids,
      const AllowedPassbackDomainArray& allowed_passback_domains,
      unsigned long colo_id)
      /*throw(eh::Exception)*/;

    void
    fill(
      RequestInfo& request_info,
      const FCGI::HttpRequest& request,
      const String::SubString& path_args)
      /*throw(InvalidParamException, Exception)*/;

    const Logging::Logger_var&
    logger() const noexcept
    {
      return logger_;
    }

  private:
    typedef Generics::GnuHashTable<
      Generics::SubStringHashAdapter, RequestInfoParamProcessor_var>
      ParamProcessorMap;

  private:
    Logging::Logger_var logger_;
    CommonModule_var common_module_;
    const ExternalUserIdSet skip_external_ids_;
    const AllowedPassbackDomainArray allowed_passback_domains_;
    const unsigned long colo_id_;

    std::unique_ptr<GeoIPMapping::IPMapCity2> ip_map_;

    ParamProcessorMap header_processors_;
    ParamProcessorMap param_processors_;
    ParamProcessorMap cookie_processors_;

  private:
    void
    add_processor_(
      bool headers,
      bool parameters,
      const String::SubString& name,
      RequestInfoParamProcessor* processor)
      noexcept;

    void
    cookies_processing_(
      RequestInfo& request_info,
      const FCGI::HttpRequest& request)
      /*throw(InvalidParamException, Exception)*/;

    void
    params_processing_(
      RequestInfo& request_info,
      const FCGI::HttpRequest& request,
      const String::SubString& path_args)
      /*throw(InvalidParamException, Exception)*/;

    void
    headers_processing_(
      RequestInfo& request_info,
      const FCGI::HttpRequest& request)
      /*throw(InvalidParamException, Exception)*/;
  };
}
}

#endif /*USERBINDFRONTEND_REQUESTINFOFILLER_HPP_*/
