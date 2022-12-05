#ifndef FRONTENDS_OPTOUTFRONTEND_REQUESTINFOFILLER_HPP
#define FRONTENDS_OPTOUTFRONTEND_REQUESTINFOFILLER_HPP

#include <string>

#include <Logger/Logger.hpp>
#include <Generics/Time.hpp>
#include <Generics/Uuid.hpp>
#include <Generics/GnuHashTable.hpp>

#include <HTTP/Http.hpp>

#include <Language/SegmentorCommons/SegmentorInterface.hpp>

#include <Commons/Containers.hpp>
#include <Commons/UserInfoManip.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>
#include <Frontends/FrontendCommons/HTTPUtils.hpp>
#include <Frontends/FrontendCommons/RequestMatchers.hpp>
#include <Frontends/FrontendCommons/UserAgentMatcher.hpp>
#include <Frontends/FrontendCommons/RequestParamProcessor.hpp>
#include <Frontends/CommonModule/CommonModule.hpp>

#include <xsd/Frontends/FeConfig.hpp>

#include "OptoutFrontendStat.hpp"

namespace AdServer
{
namespace OptOut
{
  struct RequestInfo
  {
    RequestInfo() noexcept;

    std::string oo_success_redirect_url;
    std::string oo_failure_redirect_url;
    std::string oo_already_redirect_url;
    std::string oo_status_in_redirect_url;
    std::string oo_status_out_redirect_url;
    std::string oo_status_undef_redirect_url;

    OptOperation oo_operation;
    OptOperation old_oo_type;

    std::string peer_ip;
    AdServer::Commons::UserId user_id;
    unsigned long user_status;
    std::string user_agent;
    std::string oo_debug;
    Generics::Time debug_time;
    long colo_id;

    Generics::Time cookie_expire_time;

    const char* local_aspect;
    bool log_as_test;

    std::string browser;
    std::string os;
    std::string ct;
    std::string curct;
  };

  typedef FrontendCommons::RequestParamProcessor<RequestInfo>
    RequestInfoParamProcessor;

  typedef ReferenceCounting::SmartPtr<RequestInfoParamProcessor>
    RequestInfoParamProcessor_var;

  class RequestInfoFiller: private FrontendCommons::HTTPExceptions
  {
  public:
    typedef FrontendCommons::HTTPExceptions::Exception Exception;
    typedef FrontendCommons::HTTPExceptions::BadParameter BadParameter;
    DECLARE_EXCEPTION(FillerException, Exception);

    RequestInfoFiller(
      xsd::AdServer::Configuration::OptOutFeConfigurationType* config,
      Logging::Logger* logger,
      CommonModule* common_module)
      /*throw(eh::Exception)*/;

    void
    fill(
      RequestInfo& request_info,
      const HTTP::SubHeaderList& headers,
      const HTTP::ParamList& params,
      HTTP::CookieList& cookies) const
      /*throw(BadParameter, FillerException)*/;

  private:
    typedef Generics::GnuHashTable<
      Generics::SubStringHashAdapter, RequestInfoParamProcessor_var>
      ParamProcessorMap;

    void
    add_processor_(
      bool headers,
      bool parameters,
      const String::SubString& name,
      RequestInfoParamProcessor* processor)
      noexcept;

  private:
    CommonModule_var common_module_;

    ParamProcessorMap header_processors_;
    ParamProcessorMap param_processors_;
    ParamProcessorMap cookie_processors_;
  };
} // OptOut
} /*AdServer*/

#endif /*FRONTENDS_WEBSTATFRONTEND_REQUESTINFOFILLER_HPP*/
