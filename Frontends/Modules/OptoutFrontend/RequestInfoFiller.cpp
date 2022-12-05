#include <String/AsciiStringManip.hpp>
#include <String/UTF8Case.hpp>
#include <Generics/Rand.hpp>
#include <HTTP/UrlAddress.hpp>

#include <Commons/ErrorHandler.hpp>
#include <Commons/UserInfoManip.hpp>

#include <Frontends/CommonModule/CommonModule.hpp>
#include <Frontends/FrontendCommons/RequestParamProcessor.hpp>
#include "RequestInfoFiller.hpp"

namespace
{
  const String::AsciiStringManip::CharCategory COHORT_CHARS(
    String::AsciiStringManip::ALPHA_NUM,
    String::AsciiStringManip::CharCategory("-."));

  namespace Aspect
  {
    const char OPTOUT_FRONTEND[] = "OptoutFrontend";
    const char OPTIN[] = "Opt-In";

    const char* LOCAL_ASPECTS[] =
    {
      OPTOUT_FRONTEND, "Opt-Out", OPTIN, "Status"
    };
  }

  namespace Cookie
  {
    const Generics::SubStringHashAdapter OPTOUT(String::SubString("OPTED_OUT"));
    const String::AsciiStringManip::Caseless OPTOUT_TRUE_VALUE("YES");
    const String::SubString COHORT("ct");
  }

  namespace Header
  {
    const String::SubString REM_HOST(".remotehost");
    const String::SubString USER_AGENT("user-agent");
    const String::SubString FCGI_USER_AGENT("user_agent");
  }

  namespace Context
  {
    const Generics::SubStringHashAdapter CLIENT_ID(String::SubString("uid"));
    const String::AsciiStringManip::Caseless COHORT("ct");

    const String::AsciiStringManip::Caseless OO_OUT_OPERATION("out");
    const char OO_IN_OPERATION[] = "in";
    const char OO_STATUS_OPERATION[] = "status";

    const String::SubString OO_OPERATION("op");
    const String::SubString OO_DEBUG("oo_debug");
    const String::SubString DEBUG_CURRENT_TIME("debug-time");
    const String::SubString COLOCATION_ID("colo");

    const String::SubString OO_SUCCESS_REDIRECT("success_url");
    const String::SubString OO_FAILURE_REDIRECT("fail_url");
    const String::SubString OO_ALREADY_REDIRECT("already_url");

    const String::SubString OO_IN_REDIRECT("opted_in_url");
    const String::SubString OO_OUT_REDIRECT("opted_out_url");
    const String::SubString OO_UNDEF_REDIRECT("opt_undef_url");

    const String::SubString OO_CE("ce");

    const String::SubString TEST_REQUEST("testrequest");
  }
}

namespace AdServer
{
namespace OptOut
{

  namespace
  {
    class OldOOProcessor : public RequestInfoParamProcessor
    {
    public:
      virtual void
      process(
        RequestInfo& request_info,
        const String::SubString& value) const
      {
        if (value == Cookie::OPTOUT_TRUE_VALUE)
        {
          request_info.old_oo_type = OO_OUT;
          request_info.user_status = CampaignSvcs::US_OPTOUT;
        }
      }

    private:
      virtual
      ~OldOOProcessor() noexcept
      {}
    };

    class UuidParamProcessor: public RequestInfoParamProcessor
    {
    public:
      UuidParamProcessor(
        Logging::Logger* logger,
        CommonModule* common_module)
        noexcept
        : logger_(ReferenceCounting::add_ref(logger)),
          common_module_(ReferenceCounting::add_ref(common_module))
      {}

      virtual void
      process(
        RequestInfo& request_info,
        const String::SubString& value) const
        noexcept
      {
        try
        {
          if(value == AdServer::Commons::PROBE_USER_ID.to_string())
          {
            request_info.user_id = AdServer::Commons::PROBE_USER_ID;
            request_info.user_status = AdServer::CampaignSvcs::US_PROBE;
          }
          else
          {
            Generics::SignedUuid uid =
              common_module_->user_id_controller()->verify(value);
            if (!uid.uuid().is_null())
            {
              request_info.user_id = uid.uuid();
              request_info.user_status = AdServer::CampaignSvcs::US_OPTIN;
            }
          }
        }
        catch (const eh::Exception& e)
        {
          Stream::Error ostr;
          ostr << FNS << ": Invalid uid value=\"" << value << "): " << e.what();

          logger_->log(
            ostr.str(),
            Logging::Logger::NOTICE,
            Aspect::OPTOUT_FRONTEND);
        }
      }

    private:
      virtual
      ~UuidParamProcessor() noexcept {}

      Logging::Logger_var logger_;
      CommonModule_var common_module_;
    };

    class UserAgentProcessor: public RequestInfoParamProcessor
    {
    public:
      UserAgentProcessor(CommonModule* common_module) noexcept
        : common_module_(ReferenceCounting::add_ref(common_module))
      {}

      virtual void
      process(
        RequestInfo& request_info,
        const String::SubString& value) const
        /*throw(eh::Exception)*/
      {
        std::string& user_agent = request_info.user_agent;
        value.assign_to(user_agent);
        if (!user_agent.empty())
        {
          FrontendCommons::WebBrowserMatcher_var web_browser_matcher =
            common_module_->web_browser_matcher();
          FrontendCommons::PlatformMatcher_var platform_matcher =
            common_module_->platform_matcher();

          if (web_browser_matcher.in())
          {
            web_browser_matcher->match(request_info.browser, user_agent);
          }

          if (platform_matcher.in())
          {
            std::string short_os;
            platform_matcher->match(0, short_os, request_info.os, user_agent);
          }
        }
      }

    private:
      virtual
      ~UserAgentProcessor() noexcept {}

    private:
      CommonModule_var common_module_;
    };

    class OperationProcessor: public RequestInfoParamProcessor
    {
    public:
      virtual void
      process(
        RequestInfo& request_info,
        const String::SubString& value) const
        /*throw(RequestInfoFiller::BadParameter)*/
      {
        if (value == Context::OO_OUT_OPERATION)
        {
          request_info.oo_operation = OO_OUT;
        }
        else if (value == Context::OO_IN_OPERATION)
        {
          request_info.oo_operation = OO_IN;
        }
        else if (value == Context::OO_STATUS_OPERATION)
        {
          request_info.oo_operation = OO_STATUS;
        }
        else
        {
          Stream::Error ostr;
          ostr << FNS << ": unknown type of OptOut operation: '" <<
            value << "'";

          throw RequestInfoFiller::BadParameter(ostr);
        }
        request_info.local_aspect =
          Aspect::LOCAL_ASPECTS[request_info.oo_operation];
      }

    private:
      virtual
      ~OperationProcessor() noexcept {}
    };

    class CeProcessor: public RequestInfoParamProcessor
    {
    public:
      virtual void
      process(
        RequestInfo& request_info,
        const String::SubString& value) const
        /*throw(RequestInfoFiller::BadParameter)*/
      {
        if (!value.empty())
        {
          unsigned long expire_value;
          Generics::Time expire_value_multiplier;

          String::SubString num_value;

          if(*value.rbegin() == 'm')
          {
            expire_value_multiplier = Generics::Time::ONE_MINUTE;
            num_value.assign(value.begin(), value.end() - 1);
          }
          else
          {
            expire_value_multiplier = Generics::Time::ONE_DAY;
            num_value.assign(
              value.begin(),
              *value.rbegin() == 'd' ? value.end() - 1 : value.end());
          }

          if (!String::StringManip::str_to_int(num_value, expire_value))
          {
            Stream::Error ostr;
            ostr << "Non correct " << Context::OO_CE <<
              " value '" << value << "'.";
            throw RequestInfoFiller::BadParameter(ostr);
          }

          request_info.cookie_expire_time = expire_value_multiplier * expire_value;
        }
      }

    private:
      virtual
      ~CeProcessor() noexcept {}
    };

  } // namespace

   RequestInfo::RequestInfo() noexcept
    : oo_operation(OO_NOT_DEFINED),
      old_oo_type(OO_NOT_DEFINED),
      user_status(CampaignSvcs::US_UNDEFINED),
      debug_time(Generics::Time::get_time_of_day()),
      colo_id(-1),
      local_aspect(Aspect::OPTIN),
      log_as_test(false)
  {}

 /** RequestInfoFiller */
  RequestInfoFiller::RequestInfoFiller(
    xsd::AdServer::Configuration::OptOutFeConfigurationType* config,
    Logging::Logger* logger,
    CommonModule* common_module)
    /*throw(eh::Exception)*/
    : common_module_(ReferenceCounting::add_ref(common_module))
  {
    // Get oo type and partner id for partner based OO
    // Params
    add_processor_(false, true, Context::OO_SUCCESS_REDIRECT,
      new FrontendCommons::UrlParamProcessor<RequestInfo>(
        &RequestInfo::oo_success_redirect_url));
    add_processor_(false, true, Context::OO_FAILURE_REDIRECT,
      new FrontendCommons::UrlParamProcessor<RequestInfo>(
        &RequestInfo::oo_failure_redirect_url));
    add_processor_(false, true, Context::OO_ALREADY_REDIRECT,
      new FrontendCommons::UrlParamProcessor<RequestInfo>(
        &RequestInfo::oo_already_redirect_url));
    add_processor_(false, true, Context::OO_IN_REDIRECT,
      new FrontendCommons::UrlParamProcessor<RequestInfo>(
        &RequestInfo::oo_status_in_redirect_url));
    add_processor_(false, true, Context::OO_OUT_REDIRECT,
      new FrontendCommons::UrlParamProcessor<RequestInfo>(
        &RequestInfo::oo_status_out_redirect_url));
    add_processor_(false, true, Context::OO_UNDEF_REDIRECT,
      new FrontendCommons::UrlParamProcessor<RequestInfo>(
        &RequestInfo::oo_status_undef_redirect_url));

    add_processor_(false, true, Context::COLOCATION_ID,
      new FrontendCommons::NumberParamProcessor<RequestInfo, long>(
        &RequestInfo::colo_id));
    add_processor_(false, true, Context::OO_CE,
      new CeProcessor);

    add_processor_(false, true, Context::OO_OPERATION,
      new OperationProcessor);
    add_processor_(false, true, Cookie::COHORT,
      new FrontendCommons::StringCheckParamProcessor<
      RequestInfo, String::AsciiStringManip::CharCategory>(
        &RequestInfo::ct, COHORT_CHARS, 50));

    // Get peer IP address and user id
    // Headers
    if (config->log_ip())
    {
      add_processor_(true, false, Header::REM_HOST,
        new FrontendCommons::StringParamProcessor<RequestInfo>(
          &RequestInfo::peer_ip));
    }

    add_processor_(true, false, Header::USER_AGENT, new UserAgentProcessor(common_module_));
    add_processor_(true, false, Header::FCGI_USER_AGENT, new UserAgentProcessor(common_module_));

    // Get OO value and user id from client's cookie
    // Cookies
    cookie_processors_.insert(std::make_pair(
      Context::CLIENT_ID,
      RequestInfoParamProcessor_var(new UuidParamProcessor(logger, common_module_))));
    cookie_processors_.insert(std::make_pair(
      Cookie::OPTOUT,
      RequestInfoParamProcessor_var(new OldOOProcessor)));
    cookie_processors_.insert(std::make_pair(
      Cookie::COHORT,
      RequestInfoParamProcessor_var(
        new FrontendCommons::StringCheckParamProcessor<
        RequestInfo, String::AsciiStringManip::CharCategory>(
          &RequestInfo::curct, COHORT_CHARS, 50))));

    // debug parameters
    add_processor_(false, true, Context::OO_DEBUG,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::oo_debug));
    add_processor_(false, true, Context::DEBUG_CURRENT_TIME,
      new FrontendCommons::TimeParamProcessor<RequestInfo>(
        &RequestInfo::debug_time, Generics::Time::ONE_DAY));
    add_processor_(false, true, Context::TEST_REQUEST,
      new FrontendCommons::BoolParamProcessor<RequestInfo>(
        &RequestInfo::log_as_test));

  }

  void
  RequestInfoFiller::add_processor_(
    bool headers,
    bool parameters,
    const String::SubString& name,
    RequestInfoParamProcessor* processor)
    noexcept
  {
    ParamProcessorMap::value_type value(name, processor);

    if (headers)
    {
      header_processors_.insert(value);
    }

    if (parameters)
    {
      param_processors_.insert(value);
    }
  }

  void
  RequestInfoFiller::fill(
    RequestInfo& request_info,
    const HTTP::SubHeaderList& headers,
    const HTTP::ParamList& params,
    HTTP::CookieList& cookies) const
    /*throw(BadParameter, FillerException)*/
  {
    static const char* FUN = "RequestInfoFiller::fill()";

    /* fill opt-out request parameters */
    try
    {
      for (HTTP::SubHeaderList::const_iterator it = headers.begin();
        it != headers.end(); ++it)
      {
        std::string header_name(it->name.str());
        String::AsciiStringManip::to_lower(header_name);

        ParamProcessorMap::const_iterator param_it =
          header_processors_.find(header_name);

        if(param_it != header_processors_.end())
        {
          param_it->second->process(request_info, it->value);
        }
      } /* headers processing */

      for(HTTP::ParamList::const_iterator it = params.begin();
          it != params.end(); ++it)
      {
        ParamProcessorMap::const_iterator param_it =
          param_processors_.find(it->name);

        if(param_it != param_processors_.end())
        {
          param_it->second->process(request_info, it->value);
        }
      } /* url parameters processing */

      try
      {
        cookies.load_from_headers(headers);
      }
      catch (const HTTP::CookieList::InvalidArgument& e)
      {
        Stream::Error ostr;
        ostr << __func__ << ": " << e.what();
        throw RequestInfoFiller::BadParameter(ostr);
      }

      for(HTTP::CookieList::const_iterator it = cookies.begin();
          it != cookies.end(); ++it)
      {
        ParamProcessorMap::const_iterator param_it =
          cookie_processors_.find(it->name);

        if(param_it != cookie_processors_.end())
        {
          param_it->second->process(request_info, it->value);
        }
      } /* cookies processing */

      if (request_info.oo_operation == OO_NOT_DEFINED)
      {
        Stream::Error ostr;
        ostr << FNS << ": empty operation parameter. OO cookie will not be set";
        throw BadParameter(ostr);
      }

      if (request_info.old_oo_type == OO_NOT_DEFINED &&
        !(request_info.user_id.is_null() ||
          request_info.user_id == AdServer::Commons::PROBE_USER_ID))
      {
        request_info.old_oo_type = OO_IN;
      }
    }
    catch (const BadParameter&)
    {
      throw;
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": "
        "Can't fill request info. Caught eh::Exception: " <<
        ex.what();
      throw FillerException(ostr);
    }
  }
} // OptOut
} /*AdServer*/
