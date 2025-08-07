#include <Frontends/FrontendCommons/HTTPUtils.hpp>

#include "RequestInfoFiller.hpp"

namespace AdServer
{
namespace UserBind
{
  namespace Aspect
  {
    const char USER_BIND_FRONTEND[] = "UserBindFrontend";
  }

  namespace Request
  {
    namespace Cookies
    {
      const String::SubString COHORT("ct");
      const String::SubString GA_USER_ID("_ga");
      const String::SubString GCLU_USER_ID("_gcl_au");
      const String::SubString YM_USER_ID("_ym_uid");
    }

    namespace Context
    {
      const String::SubString AMP("*amp*");
      const String::SubString EQL("*eql*");

      const String::SubString EXTERNAL_IDS[] = {
        String::SubString("id"),
        String::SubString("tid"),
        String::SubString("ssp_user_id"),
        String::SubString("google_gid"),
        String::SubString("tanx_tid")
      };

      const String::SubString CLID_ID("clid");
      const String::SubString SOURCE_ID("src");
      const String::SubString OPTOUT("OPTED_OUT");
      const String::SubString LOCATION_NAME("loc.name");
      const String::SubString COLOCATION_ID("colo");
      const String::SubString IP_ADDRESS("debug.ip");
      const String::SubString DO_PASSBACK("pbf");
      const String::SubString GENERATE_EXTERNAL_ID("gi");
      const String::SubString BIDSWITCH_SSP_ID("bidswitch_ssp_id");
      const String::SubString DEBUG_CURRENT_TIME("debug-time");
      const String::SubString PASSBACK_URL("passback");
      const String::SubString PASSBACK_URL2("url");
      const String::SubString DO_DELETE("delete");
      const String::SubString ADD_USER_ID("au");
      const String::SubString DISABLE_SECURE_REDIRECT("nosecure");
      const String::SubString CHANNELS_WL("sg_wl");
      const String::SubString CL_ID("cl_id");
      const String::SubString SESSION_ID("session_id");

      // Used to return back somedata to a sender
      const String::SubString PUSH_DATA[] = {
        String::SubString("fp"),
        String::SubString("google_push")
      };

      const String::SubString EXTERNAL_ID("fid");
      const String::SubString GOOGLE_ERROR("google_error");
    }

    namespace Header
    {
      const String::SubString REM_HOST(".remotehost");
      const String::AsciiStringManip::Caseless X_FORWARDED_FOR("x-forwarded-for");
      const String::AsciiStringManip::Caseless FCGI_X_FORWARDED_FOR("x_forwarded_for");
      const String::AsciiStringManip::Caseless USER_AGENT("user-agent");
      const String::AsciiStringManip::Caseless FCGI_USER_AGENT("user_agent");
      const String::AsciiStringManip::Caseless REFERER("referer");
      const String::SubString SECURE("secure");
      const String::SubString HOST("host");
    }
  }

  namespace
  {
    class UuidParamProcessor:
      public FrontendCommons::SignedUuidParamProcessor<RequestInfo>
    {
    public:
      UuidParamProcessor(
        AdServer::UserIdController* user_id_controller,
        AdServer::UserIdController::KeyType user_id_type,
        bool allow_rewrite = true)
        : FrontendCommons::SignedUuidParamProcessor<RequestInfo>(
            &RequestInfo::user_id,
            user_id_controller,
            user_id_type,
            allow_rewrite)
      {}

      virtual void
      process(
        RequestInfo& request_info,
        const String::SubString& value) const
        noexcept
      {
        try
        {
          FrontendCommons::SignedUuidParamProcessor<RequestInfo>::process(
            request_info, value);

          if (request_info.user_status == AdServer::CampaignSvcs::US_UNDEFINED &&
            request_info.user_id == AdServer::Commons::PROBE_USER_ID)
          {
            request_info.user_status = AdServer::CampaignSvcs::US_PROBE;
          }
        }
        catch(const eh::Exception&)
        {}
      }
    };

    const String::SubString EQL("*eql*");
    const String::SubString AMP("*amp*");
  }

  void
  RequestInfo::dump(std::ostream& os) const noexcept
  {
    os << "user_status = " <<
      (user_status == AdServer::CampaignSvcs::US_OPTOUT
        ? "OPTOUT"
        : user_status == AdServer::CampaignSvcs::US_OPTIN
          ? "OPTIN"
          : user_status == AdServer::CampaignSvcs::US_PROBE
            ? "PROBE"
            : "UNDEFINED")  << "\n" <<
      "user_id = " << user_id << "\n" <<
      "cohort = " << cohort << "\n" <<
      "colo_id = " << colo_id << "\n" <<
      "external_id = " << external_id << "\n" <<
      "source_id = " << source_id << "\n" <<
      "peer_ip = " << peer_ip << "\n"
      "location = ";

    if (location)
    {
      os <<
        location->country << '/' <<
        location->region << '/' <<
        location->city;
    }

    os << std::endl;
  }

  RequestInfoFiller::RequestInfoFiller(
    Logging::Logger* logger,
    CommonModule* common_module,
    const char* geo_ip_path,
    const ExternalUserIdSet& skip_external_ids,
    const AllowedPassbackDomainArray& allowed_passback_domains,
    unsigned long colo_id)
    /*throw(eh::Exception)*/
    : logger_(ReferenceCounting::add_ref(logger)),
      common_module_(ReferenceCounting::add_ref(common_module)),
      skip_external_ids_(skip_external_ids),
      allowed_passback_domains_(allowed_passback_domains),
      colo_id_(colo_id)
  {
    static const char* FUN = "RequestInfoFiller::RequestInfoFiller()";

    const String::AsciiStringManip::CharCategory IPCat(".0-9");

    if(geo_ip_path)
    {
      try
      {
        ip_map_.reset(new GeoIPMapping::IPMapCity2(geo_ip_path));
      }
      catch (const GeoIPMapping::IPMap::Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": GeoIPMapping::IPMap::Exception caught: " << e.what();

        logger->log(ostr.str(),
          Logging::Logger::CRITICAL,
          Aspect::USER_BIND_FRONTEND,
          "ADS-IMPL-102");
      }
    }

    cookie_processors_.insert(std::make_pair(
      FrontendCommons::Cookies::CLIENT_ID,
      RequestInfoParamProcessor_var(
        new UuidParamProcessor(
          common_module->user_id_controller(),
          AdServer::UserIdController::PERSISTENT))));
    cookie_processors_.insert(std::make_pair(
      FrontendCommons::Cookies::CLIENT_ID2,
      RequestInfoParamProcessor_var(
        new UuidParamProcessor(
          common_module->user_id_controller(),
          AdServer::UserIdController::PERSISTENT,
          false))));

    cookie_processors_.insert(std::make_pair(
      FrontendCommons::Cookies::OPTOUT,
      RequestInfoParamProcessor_var(
        new FrontendCommons::OptOutParamProcessor<RequestInfo>(
          &RequestInfo::user_status))));
    cookie_processors_.insert(std::make_pair(
      Request::Cookies::COHORT,
      RequestInfoParamProcessor_var(
        new FrontendCommons::StringParamProcessor<RequestInfo>(
          &RequestInfo::cohort))));
    cookie_processors_.insert(std::make_pair(
      Request::Cookies::GA_USER_ID,
      RequestInfoParamProcessor_var(
        new FrontendCommons::StringParamProcessor<RequestInfo>(
          &RequestInfo::ga_user_id))));
    cookie_processors_.insert(std::make_pair(
      Request::Cookies::GCLU_USER_ID,
      RequestInfoParamProcessor_var(
        new FrontendCommons::StringParamProcessor<RequestInfo>(
          &RequestInfo::gclu_user_id))));
    cookie_processors_.insert(std::make_pair(
      Request::Cookies::YM_USER_ID,
      RequestInfoParamProcessor_var(
        new FrontendCommons::StringParamProcessor<RequestInfo>(
          &RequestInfo::ym_user_id))));

    add_processor_(true, true, Request::Context::CLID_ID,
      new UuidParamProcessor(
        common_module->user_id_controller(),
        AdServer::UserIdController::PERSISTENT,
        true // clid override cookies now !!!
        ));

    for(unsigned long i = 0;
        i < sizeof(Request::Context::EXTERNAL_IDS) /
          sizeof(Request::Context::EXTERNAL_IDS[0]); ++i)
    {
      add_processor_(false, true, Request::Context::EXTERNAL_IDS[i],
        new FrontendCommons::StringParamProcessor<RequestInfo>(
          &RequestInfo::short_external_id));
    }

    add_processor_(false, true, Request::Context::SOURCE_ID,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::source_id));
    add_processor_(false, true, Request::Context::LOCATION_NAME,
      new FrontendCommons::LocationNameParamProcessor<RequestInfo>(
        &RequestInfo::location));
    add_processor_(false, true, Request::Context::COLOCATION_ID,
      new FrontendCommons::NumberParamProcessor<RequestInfo, unsigned long>(
        &RequestInfo::colo_id));
    add_processor_(false, true, Request::Context::DO_PASSBACK,
      new FrontendCommons::BoolParamProcessor<RequestInfo>(
        &RequestInfo::passback));
    add_processor_(false, true, Request::Context::GENERATE_EXTERNAL_ID,
      new FrontendCommons::BoolParamProcessor<RequestInfo>(
        &RequestInfo::generate_external_id));
    add_processor_(false, true, Request::Context::BIDSWITCH_SSP_ID,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::ssp_id));
    add_processor_(false, true, Request::Context::DEBUG_CURRENT_TIME,
      new FrontendCommons::TimeParamProcessor<RequestInfo>(&RequestInfo::time));
    add_processor_(false, true, Request::Context::PASSBACK_URL,
      new FrontendCommons::UrlParamProcessor<RequestInfo>(
        &RequestInfo::passback_url));
    add_processor_(false, true, Request::Context::PASSBACK_URL2,
      new FrontendCommons::UrlParamProcessor<RequestInfo>(
        &RequestInfo::passback_url));
    add_processor_(false, true, Request::Context::EXTERNAL_ID,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::external_id));
    add_processor_(true, false, Request::Header::REM_HOST,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::peer_ip));
    add_processor_(true, false, Request::Header::HOST,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::server_host));
    add_processor_(false, true, Request::Context::IP_ADDRESS,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::peer_ip));
    add_processor_(true, false, Request::Header::X_FORWARDED_FOR.str,
      new FrontendCommons::StringCheckParamProcessor<
        RequestInfo, String::AsciiStringManip::CharCategory>(
        &RequestInfo::x_peer_ip, IPCat, 30, false, true));
    add_processor_(true, false, Request::Header::FCGI_X_FORWARDED_FOR.str,
      new FrontendCommons::StringCheckParamProcessor<
        RequestInfo, String::AsciiStringManip::CharCategory>(
        &RequestInfo::x_peer_ip, IPCat, 30, false, true));
    add_processor_(true, false, Request::Header::USER_AGENT.str,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::user_agent));
    add_processor_(true, false, Request::Header::FCGI_USER_AGENT.str,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::user_agent));
    // referer must be saved as is (without normalization) - this required for yandex sign calc
    add_processor_(true, false, Request::Header::REFERER.str,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::referer));
    add_processor_(true, false, Request::Header::SECURE,
      new FrontendCommons::BoolParamProcessor<RequestInfo>(
        &RequestInfo::secure));
    add_processor_(false, true, Request::Context::DO_DELETE,
      new FrontendCommons::BoolParamProcessor<RequestInfo>(
        &RequestInfo::delete_op));
    add_processor_(false, true, Request::Context::ADD_USER_ID,
      new FrontendCommons::UuidParamProcessor<RequestInfo>(
        &RequestInfo::add_user_id));
    add_processor_(false, true, Request::Context::DISABLE_SECURE_REDIRECT,
      new FrontendCommons::BoolParamProcessor<RequestInfo>(
        &RequestInfo::disable_secure_redirect));
    add_processor_(false, true, FrontendCommons::Cookies::CLIENT_ID,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::param_user_id));
    add_processor_(false, true, Request::Context::CL_ID,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::cl_id));
    add_processor_(false, true, Request::Context::SESSION_ID,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::session_id));
    add_processor_(false, true, Request::Context::CHANNELS_WL,
      new FrontendCommons::NumberContainerParamProcessor<
        RequestInfo,
        RequestInfo::ChannelsWl,
        String::AsciiStringManip::SepComma>(&RequestInfo::channels_wl));

    for(unsigned long i = 0;
        i < sizeof(Request::Context::PUSH_DATA) /
          sizeof(Request::Context::PUSH_DATA[0]); ++i)
    {
      add_processor_(false, true, Request::Context::PUSH_DATA[i],
        new FrontendCommons::StringParamProcessor<RequestInfo>(
          &RequestInfo::push_data));
    }
    add_processor_(false, true, Request::Context::GOOGLE_ERROR,
        new FrontendCommons::NumberParamProcessor<RequestInfo, unsigned long>(
          &RequestInfo::google_error));
  }

  void
  RequestInfoFiller::fill(
    RequestInfo& request_info,
    const FCGI::HttpRequest& request,
    const String::SubString& path_args)
    /*throw(InvalidParamException, Exception)*/
  {
    request_info.time = Generics::Time::get_time_of_day();

    cookies_processing_(request_info, request);
    headers_processing_(request_info, request);
    params_processing_(request_info, request, path_args);

    if(!request_info.x_peer_ip.empty())
    {
      //x-forwarded-for must override other sources
      request_info.peer_ip = request_info.x_peer_ip;
    }

    if(!request_info.location.in() &&
       !request_info.peer_ip.empty() &&
       ip_map_.get())
    {
      try
      {
        GeoIPMapping::IPMapCity2::CityLocation geo_location;

        if(ip_map_->city_location_by_addr(
             request_info.peer_ip.c_str(),
             geo_location,
             false))
        {
          request_info.location = new FrontendCommons::Location();
          request_info.location->country = geo_location.country_code.str();
          geo_location.region.assign_to(request_info.location->region);
          request_info.location->city = geo_location.city.str();
          request_info.location->normalize();
        }
      }
      catch(const eh::Exception& e)
      {}
    }

    if(request_info.source_id.empty() && request_info.server_host == String::SubString("ad-blast.ru"))
    {
      // special processing of ad-blast.ru?id=... request
      request_info.source_id = "adblast0";
      if(!request_info.short_external_id.empty())
      {
        try
        {
          request_info.user_id = common_module_->user_id_controller()->verify(request_info.short_external_id).uuid();
          request_info.short_external_id.clear();
          request_info.generate_external_id = true;
        }
        catch(const eh::Exception&)
        {}
      }
    }

    if(request_info.external_id.empty() && !request_info.short_external_id.empty())
    {
      request_info.external_id = request_info.source_id + "/" +
        request_info.short_external_id;
    }

    if(!request_info.external_id.empty())
    {
      std::string short_external_id;

      // get short external id by external_id
      std::size_t pos = request_info.external_id.find('/');
      if(pos != std::string::npos)
      {
        short_external_id.assign(
          request_info.external_id,
          pos + 1,
          request_info.external_id.size() - pos - 1);
      }
      else
      {
        short_external_id = request_info.external_id;
      }

      // if external_id defined in skip list and gi is true - generate user id
      if(skip_external_ids_.find(short_external_id) !=
        skip_external_ids_.end())
      {
        request_info.external_id.clear();
      }
      else
      {
        request_info.short_external_id = short_external_id;
      }
    }

    if(!request_info.external_id.empty())
    {
      request_info.generate_external_id = false;
    }

    if (request_info.user_status == AdServer::CampaignSvcs::US_OPTOUT)
    {
      request_info.user_id = AdServer::Commons::UserId();
    }
    else if (!request_info.user_id.is_null())
    {
      request_info.user_status = AdServer::CampaignSvcs::US_OPTIN;
    }

    if(!allowed_passback_domains_.empty() && !request_info.passback_url.empty())
    {
      bool passback_allowed = false;

      try
      {
        HTTP::BrowserAddress addr(request_info.passback_url);
        String::SubString host = addr.host();

        for(auto it = allowed_passback_domains_.begin();
          it != allowed_passback_domains_.end(); ++it)
        {
          if(*it->begin() == '*')
          {
            const std::string& cmp = *it;
            if(host.size() >= cmp.size() - 1 &&
               host.compare(host.size() - (cmp.size() - 1), cmp.size() - 1, cmp.data() + 1) == 0)
            {
              passback_allowed = true;
            }
          }
          else if(host.compare(*it) == 0)
          {
            passback_allowed = true;
          }
        }
      }
      catch(...)
      {}

      if(!passback_allowed)
      {
        request_info.passback_url.clear();
      }
    }
    else
    {
      request_info.passback_url.clear();
    }

    if(!request_info.ga_user_id.empty())
    {
      request_info.ga_user_id = std::string("ga/") + request_info.ga_user_id;
    }

    if(!request_info.gclu_user_id.empty())
    {
      request_info.gclu_user_id = std::string("gu/") + request_info.gclu_user_id;
    }

    if(!request_info.ym_user_id.empty())
    {
      request_info.ym_user_id = std::string("ym/") + request_info.ym_user_id;
    }

    if(request_info.colo_id == 0)
    {
      // resolve colo by IP + cohort
      FrontendCommons::IPMatcher_var ip_matcher =
        common_module_->ip_matcher();

      try
      {
        FrontendCommons::IPMatcher::MatchResult ip_match_result;
        if(ip_matcher.in() && !request_info.peer_ip.empty() &&
          ip_matcher->match(
            ip_match_result,
            request_info.peer_ip,
            request_info.cohort))
        {
          request_info.colo_id = ip_match_result.colo_id;
        }
      }
      catch(const FrontendCommons::IPMatcher::InvalidParameter&)
      {}
    }

    if(request_info.colo_id == 0)
    {
      request_info.colo_id = colo_id_;
    }
  }

  void
  RequestInfoFiller::params_processing_(
    RequestInfo& request_info,
    const FCGI::HttpRequest& request,
    const String::SubString& path_args)
    /*throw(InvalidParamException, Exception)*/
  {
    const HTTP::ParamList& params = request.params();

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

    FrontendCommons::ParsedParamsMap parsed_params;

    FrontendCommons::parse_args(
      parsed_params,
      path_args,
      AMP,
      EQL);

    for(auto it = parsed_params.begin(); it != parsed_params.end(); ++it)
    {
      ParamProcessorMap::const_iterator param_it =
        param_processors_.find(it->first);

      if(param_it != param_processors_.end())
      {
        param_it->second->process(request_info, it->second);
      }
    }
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
  RequestInfoFiller::headers_processing_(
    RequestInfo& request_info,
    const FCGI::HttpRequest& request)
    /*throw(InvalidParamException, Exception)*/
  {
    const HTTP::SubHeaderList& headers = request.headers();

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
  }

  void
  RequestInfoFiller::cookies_processing_(
    RequestInfo& request_info,
    const FCGI::HttpRequest& request)
    /*throw(InvalidParamException, Exception)*/
  {
    static const char* FUN = "RequestInfoFiller::cookies_processing_()";

    HTTP::CookieList cookies;

    try
    {
      cookies.load_from_headers(request.headers());
    }
    catch(HTTP::CookieList::InvalidArgument& ex)
    {
      if(logger()->log_level() >= Logging::Logger::TRACE)
      {
        Stream::Error ostr;
        ostr << FUN << ": caught HTTP::CookieList::InvalidArgument: " <<
          ex.what();
        logger()->log(ostr.str(),
          Logging::Logger::TRACE,
          Aspect::USER_BIND_FRONTEND);
      }

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
  }
}
}
