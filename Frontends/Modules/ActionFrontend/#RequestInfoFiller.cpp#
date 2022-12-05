#include <String/AsciiStringManip.hpp>
#include <String/UTF8Case.hpp>
#include <String/StringManip.hpp>
#include <Generics/Rand.hpp>
#include <HTTP/UrlAddress.hpp>

#include <Commons/ErrorHandler.hpp>
#include <Commons/UserInfoManip.hpp>

#include <Frontends/FrontendCommons/RequestParamProcessor.hpp>
#include <Frontends/FrontendCommons/Cookies.hpp>

#include "RequestInfoFiller.hpp"

namespace AdServer
{
namespace Action
{
  namespace Aspect
  {
    const char ACTION_FRONTEND[] = "ActionFrontend";
  }

  namespace Request
  {
    namespace Cookie
    {
      const String::AsciiStringManip::Caseless OPTOUT("OPTED_OUT");
      const String::AsciiStringManip::Caseless OPTOUT_TRUE_VALUE("YES");
      //const String::AsciiStringManip::Caseless USER_ID("uid");
    }

    namespace Header
    {
      const String::SubString REM_HOST(".remotehost");
      const String::SubString USER_AGENT("user-agent");
      const String::SubString FCGI_USER_AGENT("user_agent");
      const String::AsciiStringManip::Caseless REFERER("referer");
      const String::SubString SECURE("secure");
    }

    namespace Context
    {
      const String::SubString CLID_ID("clid");
      const String::SubString CAMPAIGN_ID("cid");
      const String::SubString ACTION_ID("actionid");
      const String::SubString CONVERSION_ID("convid");
      const String::SubString COUNTRY("country");
      const String::SubString TEST_REQUEST("testrequest");
      const String::SubString LOCATION_NAME("loc.name");
      const String::SubString ORDER_ID("orderid");
      const String::SubString ACTION_VALUE("value");
      const String::SubString EXTERNAL_ID("fid");

      /* debug params */
      const String::SubString IP_ADDRESS("debug.ip");
      const String::SubString DEBUG_CURRENT_TIME("debug-time");
    }
  }

  class UuidParamProcessor: public RequestInfoParamProcessor
  {
  public:
    UuidParamProcessor(
      const RequestInfoFiller* filler,
      bool allow_rewrite = true)
      : filler_(filler),
        allow_rewrite_(allow_rewrite)
    {}

    virtual void process(
      RequestInfo& request_info,
      const String::SubString& value) const
      /*throw(RequestInfoFiller::InvalidParamException)*/
    {
      try
      {
        filler_->adapt_client_id_(
          value, request_info, allow_rewrite_);
      }
      catch(...)
      {}
    }

  private:
    const RequestInfoFiller* filler_;
    const bool allow_rewrite_;
  };

  class OptedOutParamProcessor: public RequestInfoParamProcessor
  {
  public:
    virtual void process(
      RequestInfo& request_info,
      const String::SubString& /*value*/) const
      /*throw(RequestInfoFiller::InvalidParamException)*/
    {
      request_info.user_status = AdServer::CampaignSvcs::US_OPTOUT;
    }

  protected:
    virtual ~OptedOutParamProcessor() noexcept {}
  };

  namespace
  {
    const String::SubString OLD_AMP("_amp_");
    const String::SubString OLD_EQL("_eql_");

    const String::SubString AMP("*amp*");
    const String::SubString EQL("*eql*");

    const String::SubString UTM_TERM_1("?utm_term=");
    const String::SubString UTM_TERM_2("&utm_term=");
    const String::SubString UTM_TERM_3(".utm_term=");
  }

  // RequestInfoFiller
  RequestInfoFiller::RequestInfoFiller(
    Logging::Logger* logger,
    CommonModule* common_module,
    const char* geo_ip_path,
    Commons::LogReferrer::Setting use_referrer,
    bool set_uid)
    /*throw(eh::Exception)*/
    : logger_(ReferenceCounting::add_ref(logger)),
      common_module_(ReferenceCounting::add_ref(common_module)),
      use_referrer_(use_referrer),
      set_uid_(set_uid)
  {
    static const char* FUN = "RequestInfoFiller::RequestInfoFiller()";

    if(geo_ip_path)
    {
      try
      {
        ip_map_ = IPMapPtr(new GeoIPMapping::IPMapCity2(geo_ip_path));
      }
      catch (const GeoIPMapping::IPMap::Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": GeoIPMapping::IPMap::Exception caught: " << e.what();

        logger->log(ostr.str(),
          Logging::Logger::CRITICAL,
          Aspect::ACTION_FRONTEND,
          "ADS-IMPL-102");
      }
    }

    cookie_processors_.insert(std::make_pair(
      FrontendCommons::Cookies::CLIENT_ID,
      RequestInfoParamProcessor_var(new UuidParamProcessor(this, set_uid))));
    cookie_processors_.insert(std::make_pair(
      FrontendCommons::Cookies::CLIENT_ID2,
      RequestInfoParamProcessor_var(new UuidParamProcessor(this, false))));

    cookie_processors_.insert(std::make_pair(
      FrontendCommons::Cookies::OPTOUT,
      RequestInfoParamProcessor_var(
        new FrontendCommons::OptOutParamProcessor<RequestInfo>(
          &RequestInfo::user_status))));

    add_processor_(true, true, FrontendCommons::Cookies::CLIENT_ID,
      new UuidParamProcessor(this));
    add_processor_(true, true, Request::Context::CLID_ID,
      new UuidParamProcessor(this, false));
    add_processor_(true, true, Request::Context::LOCATION_NAME,
      new FrontendCommons::LocationCountryParamProcessor<RequestInfo>(
        &RequestInfo::req_country));
    add_processor_(true, true, Request::Context::COUNTRY,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::req_country,
        2, // max length
        true // to lower
        ));
    add_processor_(true, true, Request::Context::CAMPAIGN_ID,
      new FrontendCommons::NumberParamProcessor<
        RequestInfo,
        AdServer::Commons::Optional<unsigned long>,
        unsigned long>(
        &RequestInfo::campaign_id));
    add_processor_(true, true, Request::Context::ACTION_ID,
      new FrontendCommons::NumberParamProcessor<
        RequestInfo,
        AdServer::Commons::Optional<unsigned long>,
        unsigned long>(
        &RequestInfo::action_id));
    add_processor_(true, true, Request::Context::CONVERSION_ID,
      new FrontendCommons::NumberParamProcessor<
        RequestInfo,
        AdServer::Commons::Optional<unsigned long>,
        unsigned long>(
        &RequestInfo::action_id));
    add_processor_(true, true, Request::Context::ORDER_ID,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::order_id,
        100,
        false,
        true));
    add_processor_(true, true, Request::Context::ACTION_VALUE,
      new FrontendCommons::DecimalParamProcessor<
        RequestInfo,
        AdServer::Commons::Optional<AdServer::CampaignSvcs::RevenueDecimal>,
        AdServer::CampaignSvcs::RevenueDecimal>(
          &RequestInfo::value, false));
    add_processor_(false, true, Request::Context::EXTERNAL_ID,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::external_user_id));
    
    add_processor_(true, true, Request::Context::TEST_REQUEST,
      new FrontendCommons::TestRequestParamProcessor<RequestInfo>());
    add_processor_(false, true, Request::Context::IP_ADDRESS,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::peer_ip));
    add_processor_(false, true, Request::Context::DEBUG_CURRENT_TIME,
      new FrontendCommons::TimeParamProcessor<RequestInfo>(
        &RequestInfo::time, Generics::Time::ONE_DAY));

    add_processor_(true, false, Request::Header::REFERER.str,
      new FrontendCommons::UrlParamProcessor<RequestInfo>(
        &RequestInfo::referer));

    add_processor_(true, false, Request::Header::REM_HOST,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::peer_ip));

    add_processor_(true, false, Request::Header::USER_AGENT,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::user_agent));
    add_processor_(true, false, Request::Header::FCGI_USER_AGENT,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::user_agent));
    add_processor_(true, false, Request::Header::SECURE,
      new FrontendCommons::BoolParamProcessor<RequestInfo>(
        &RequestInfo::secure));
  }

  void
  RequestInfoFiller::add_processor_(
    bool headers,
    bool parameters,
    const String::SubString& name,
    RequestInfoParamProcessor* processor)
    noexcept
  {
    RequestInfoParamProcessor_var processor_ptr(processor);

    if(headers)
    {
      header_processors_.insert(
        std::make_pair(name, processor_ptr));
    }

    if(parameters)
    {
      param_processors_.insert(
        std::make_pair(name, processor_ptr));
    }
  }

  void
  RequestInfoFiller::fill(
    RequestInfo& request_info,
    const FCGI::HttpRequest& request,
    const String::SubString& path_params_str) const
    /*throw(InvalidParamException, ForbiddenException, Exception)*/
  {
    static const char* FUN = "RequestInfoFiller::fill()";

    request_info.time = Generics::Time::get_time_of_day();

    const HTTP::SubHeaderList& headers = request.headers();
    const HTTP::ParamList& params = request.params();
    std::map<std::string, std::string> path_params;

    if(!path_params_str.empty())
    {
      if(path_params_str.find(OLD_EQL) != std::string::npos)
      {
        FrontendCommons::parse_args(
          path_params,
          path_params_str,
          OLD_AMP,
          OLD_EQL);
      }
      else
      {
        FrontendCommons::parse_args(
          path_params,
          path_params_str,
          AMP,
          EQL);
      }
    }

    try
    {
      for (HTTP::SubHeaderList::const_iterator it = headers.begin();
        it != headers.end(); ++it)
      {
        std::string header_name = it->name.str();
        String::AsciiStringManip::to_lower(header_name);

        ParamProcessorMap::const_iterator param_it =
          header_processors_.find(header_name);

        if(param_it != header_processors_.end())
        {
          param_it->second->process(request_info, it->value);
        }
      }

      for(std::map<std::string, std::string>::const_iterator it =
            path_params.begin();
          it != path_params.end(); ++it)
      {
        ParamProcessorMap::const_iterator param_it =
          param_processors_.find(it->first);

        if(param_it != param_processors_.end())
        {
          param_it->second->process(request_info, it->second);
        }
      }

      for(HTTP::ParamList::const_iterator it = params.begin();
          it != params.end(); ++it)
      {
        ParamProcessorMap::const_iterator param_it =
          param_processors_.find(it->name);

        if(param_it != param_processors_.end())
        {
          param_it->second->process(request_info, it->value);
        }
      }

      HTTP::CookieList cookies;

      try
      {
        cookies.load_from_headers(headers);
      }
      catch(HTTP::CookieList::InvalidArgument&)
      {
        throw InvalidParamException("");
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
      }

      if(request_info.value.present() &&
        *request_info.value == AdServer::CampaignSvcs::RevenueDecimal::ZERO)
      {
        request_info.value.clear();
      }

      if(!request_info.req_country.empty())
      {
        request_info.location.country_code = request_info.req_country;
      }
      else if (!request_info.peer_ip.empty() && ip_map_.get())
      {
        try
        {
          ip_map_->city_location_by_addr(
             request_info.peer_ip.c_str(),
             request_info.location,
             false);
        }
        catch(const eh::Exception&)
        {}
      }

      if(request_info.user_status != AdServer::CampaignSvcs::US_OPTOUT)
      {
        if(!request_info.user_id.is_null())
        {
          if(request_info.user_id == AdServer::Commons::PROBE_USER_ID)
          {
            request_info.user_status = AdServer::CampaignSvcs::US_PROBE;
            request_info.user_id = AdServer::Commons::UserId();
          }
          else
          {
            request_info.user_status = AdServer::CampaignSvcs::US_OPTIN;
          }
        }
        else if(set_uid_)
        {
          FrontendCommons::CountryFilter_var country_filter =
            common_module_->country_filter();

          bool pass_by_country =
            (!country_filter.in() ||
             country_filter->enabled(request_info.location.country_code)) ?
            true : false;

          if(pass_by_country)
          {
            Generics::SignedUuid uid =
              common_module_->user_id_controller()->generate();
            request_info.user_id = uid.uuid();
            request_info.signed_client_id = uid.str();
            request_info.user_status = AdServer::CampaignSvcs::US_OPTIN;
          }
        }
      }


      FrontendCommons::PlatformMatcher_var platform_matcher =
        common_module_->platform_matcher();

      if (platform_matcher.in())
      {
        try
        {
          platform_matcher->match(
            &request_info.platform_ids,
            request_info.platform,
            request_info.full_platform,
            request_info.user_agent);
        }
        catch(const eh::Exception& ex)
        {
          Stream::Error ostr;
          ostr << FUN << ": Web browser matching exception: " << ex.what();
          logger()->log(
            ostr.str(),
            Logging::Logger::EMERGENCY,
            Aspect::ACTION_FRONTEND,
            "ADS-IMPL-109");
        }
      }

    }
    catch(const InvalidParamException&)
    {
      throw;
    }
    catch(const ForbiddenException&)
    {
      throw;
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": "
        "Can't fill request info. Caught eh::Exception: " <<
        ex.what();
      throw Exception(ostr);
    }

    if(!request_info.action_id.present() &&
       !request_info.campaign_id.present())
    {
      throw InvalidParamException("");
    }

    // process case when external_user_id is:
    // 1) UNSIGNEDUID
    // 2) UNSIGNEDUID/UNSIGNEDCOOKIEUID
    if((request_info.external_user_id.size() == 24 &&
        request_info.external_user_id[22] == '.' && request_info.external_user_id[23] == '.') ||
      (request_info.external_user_id.size() == 49 &&
        request_info.external_user_id[22] == '.' && request_info.external_user_id[23] == '.' &&
        request_info.external_user_id[24] == '/' &&
        request_info.external_user_id[47] == '.' && request_info.external_user_id[48] == '.') ||
      // UNSIGNEDUID or UNSIGNEDCOOKIEUID can be empty
      (request_info.external_user_id.size() == 25 &&
        request_info.external_user_id[23] == '.' && request_info.external_user_id[24] == '.' &&
        request_info.external_user_id[0] == '/') ||
      (request_info.external_user_id.size() == 25 &&
        request_info.external_user_id[22] == '.' && request_info.external_user_id[23] == '.' &&
        request_info.external_user_id[24] == '/'))
    {
      if(parse_utm_term_(request_info, request_info.external_user_id))
      {
        request_info.external_user_id.clear();
      }
    }

    if(request_info.external_user_id.compare(0, 4, "ifa/") == 0)
    {
      String::AsciiStringManip::to_lower(request_info.external_user_id);
    }

    if(!request_info.external_user_id.empty())
    {
      std::string short_external_id;

      // get short external id by external_id
      std::size_t pos = request_info.external_user_id.find('/');
      if(pos != std::string::npos)
      {
        short_external_id.assign(
          request_info.external_user_id,
          pos + 1,
          request_info.external_user_id.size() - pos - 1);

        request_info.source_id.assign(
          request_info.external_user_id,
          0,
          pos);
      }
      else
      {
        short_external_id = request_info.external_user_id;
      }

      request_info.short_external_id.swap(short_external_id);
    }

    // parse referer for utm_term
    {
      String::SubString ref(request_info.referer);
      String::SubString::SizeType utm_term_pos1 = ref.find(UTM_TERM_1);
      String::SubString::SizeType utm_term_pos2 = ref.find(UTM_TERM_2);
      String::SubString::SizeType utm_term_pos3 = ref.find(UTM_TERM_3);
      String::SubString::SizeType start_pos = String::SubString::NPOS;
      if(utm_term_pos1 != String::SubString::NPOS)
      {
        start_pos = utm_term_pos1 + UTM_TERM_1.size();
      }
      else if(utm_term_pos2 != String::SubString::NPOS)
      {
        start_pos = utm_term_pos2 + UTM_TERM_2.size();
      }
      else if(utm_term_pos3 != String::SubString::NPOS)
      {
        start_pos = utm_term_pos3 + UTM_TERM_3.size();
      }

      if(start_pos != String::SubString::NPOS)
      {
        String::SubString::SizeType end_pos = ref.find('&', start_pos);

        String::SubString utm_term = ref.substr(
          start_pos,
          end_pos == String::SubString::NPOS ? String::SubString::NPOS :
            end_pos - start_pos);

        std::string decoded_utm_term;
        String::StringManip::mime_url_decode(
          utm_term,
          decoded_utm_term);

        parse_utm_term_(request_info, decoded_utm_term);
      }
    }

    // shrink referer for precision from config
    if(use_referrer_ == Commons::LogReferrer::LR_EMPTY)
    {
      request_info.referer.clear();
    }
    else
    {
      try
      {
        std::string new_referer;
        HTTP::BrowserAddress addr(request_info.referer);
        addr.get_view(
          use_referrer_ == Commons::LogReferrer::LR_HOST ?
            (HTTP::HTTPAddress::VW_PROTOCOL | HTTP::HTTPAddress::VW_HOSTNAME |
             HTTP::HTTPAddress::VW_NDEF_PORT) :
            (HTTP::HTTPAddress::VW_PROTOCOL | HTTP::HTTPAddress::VW_HOSTNAME |
             HTTP::HTTPAddress::VW_NDEF_PORT | HTTP::HTTPAddress::VW_PATH),
          new_referer);
        request_info.referer.swap(new_referer);
      }
      catch(const eh::Exception&)
      {
        request_info.referer.clear();
      }
    }
  }

  bool
  RequestInfoFiller::parse_utm_term_(
    RequestInfo& request_info,
    const String::SubString& utm_term)
    noexcept
  {
    // fill utm_resolved_user_id, utm_cookie_user_id
    String::SubString::SizeType slash_pos = utm_term.find('/');

    if(slash_pos == String::SubString::NPOS)
    {
      try
      {
        request_info.utm_resolved_user_id = Commons::UserId(utm_term);
      }
      catch(const eh::Exception&)
      {
        // ignore invalid user ids in utm_term
        return false;
      }
    }
    else
    {
      String::SubString u1 = utm_term.substr(0, slash_pos);
      String::SubString u2 = utm_term.substr(slash_pos + 1);

      if(!u1.empty())
      {
        try
        {
          request_info.utm_resolved_user_id = Commons::UserId(u1);
        }
        catch(const eh::Exception&)
        {
          // ignore invalid user ids in utm_term
          return false;
        }
      }

      if(!u2.empty())
      {
        String::SubString::SizeType slash2_pos = u2.find('/');
        String::SubString utm_cookie_user_id_str;
        String::SubString ifa_str;
        if(slash2_pos == String::SubString::NPOS)
        {
          utm_cookie_user_id_str = u2;
        }
        else
        {
          utm_cookie_user_id_str = u2.substr(0, slash2_pos);
          ifa_str = u2.substr(slash2_pos + 1);
        }

        if(!utm_cookie_user_id_str.empty())
        {
          try
          {
            request_info.utm_cookie_user_id = Commons::UserId(utm_cookie_user_id_str);
          }
          catch(const eh::Exception&)
          {
            // ignore invalid user ids in utm_term
            return false;
          }
        }

        if(!ifa_str.empty())
        {
          request_info.ifa = FrontendCommons::normalize_ifa(ifa_str);
        }
      }
    }

    return true;
  }

  void
  RequestInfoFiller::adapt_client_id_(
    const String::SubString& in,
    RequestInfo& request_info,
    bool allow_rewrite)
    const
    /*throw(InvalidParamException)*/
  {
    try
    {
      if(in == AdServer::Commons::PROBE_USER_ID.to_string())
      {
        request_info.user_id = AdServer::Commons::PROBE_USER_ID;
      }
      else if(allow_rewrite || request_info.user_id.is_null())
      {
        Generics::SignedUuid uid = common_module_->user_id_controller()->verify(in);
        if (!uid.uuid().is_null())
        {
          request_info.user_id = uid.uuid();
          request_info.signed_client_id = uid.str();
        }
      }
    }
    catch(const eh::Exception& ex)
    {
      throw InvalidParamException(ex.what());
    }
  }
} // Action
} // AdServer
