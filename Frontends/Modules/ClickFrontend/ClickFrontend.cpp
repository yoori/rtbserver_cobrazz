
#include <sstream>
#include <functional>

#include <Generics/Rand.hpp>
#include <Logger/StreamLogger.hpp>
#include <String/StringManip.hpp>
#include <String/AsciiStringManip.hpp>
#include <HTTP/HTTPCookie.hpp>
#include <HTTP/UrlAddress.hpp>

#include <CORBACommons/CorbaAdapters.hpp>

#include <Commons/ErrorHandler.hpp>
#include <Commons/CorbaConfig.hpp>
#include <Commons/GrpcAlgs.hpp>
#include <Commons/UserInfoManip.hpp>
#include <UserInfoSvcs/UserInfoClient/UserInfoGrpcAlgs.hpp>

#include <Frontends/FrontendCommons/Cookies.hpp>
#include <Frontends/FrontendCommons/HTTPUtils.hpp>
#include <Frontends/FrontendCommons/add_UID_cookie.hpp>
#include <Frontends/FrontendCommons/Location.hpp>

#include <Frontends/CommonModule/CommonModule.hpp>

#include <Frontends/FrontendCommons/UserInfoClientConfig.hpp>

#include "ClickFrontend.hpp"

namespace
{
  DECLARE_EXCEPTION(GrpcCallException, eh::DescriptiveException);

  void
  throw_grpc_exception_(
    const char* name,
    const grpc::Status& status)
  {
    Stream::Error ostr;
    ostr << name << ": gRPC call failed: code=" <<
      static_cast<int>(status.error_code()) <<
      ", message=" << status.error_message();
    throw GrpcCallException(ostr);
  }

  void
  throw_user_bind_exception_(const grpc::Status& status)
  {
    const std::string message = status.error_message();
    switch(status.error_code())
    {
    case grpc::StatusCode::UNAVAILABLE:
      throw AdServer::UserInfoSvcs::UserBindClient::NotReady(
        message.c_str());
    case grpc::StatusCode::NOT_FOUND:
      throw AdServer::UserInfoSvcs::UserBindClient::ChunkNotFound(
        message.c_str());
    default:
      throw AdServer::UserInfoSvcs::UserBindClient::ImplementationException(
        message.c_str());
    }
  }

  void
  throw_user_info_exception_(
    const char* name,
    const grpc::Status& status)
  {
    Stream::Error ostr;
    ostr << name << ": gRPC call failed: code=" <<
      static_cast<int>(status.error_code()) <<
      ", message=" << status.error_message();
    const std::string message = ostr.str().str();

    if(status.error_code() == grpc::StatusCode::UNAVAILABLE)
    {
      throw AdServer::UserInfoSvcs::UserInfoMatcher::NotReady(
        message.c_str());
    }

    throw AdServer::UserInfoSvcs::UserInfoMatcher::ImplementationException(
      message.c_str());
  }

  struct ClickFrontendConstrainTraits
  {
    static const unsigned long MAX_NUMBER_PARAMS = 30;
    static const unsigned long MAX_LENGTH_PARAM_NAME = 50;
    static const unsigned long MAX_LENGTH_PARAM_VALUE = 12 * 1024;
  };

  const String::AsciiStringManip::Caseless
    MIME_ENCODED_HTTP_PREFIX("http%3a%2f%2f");
  const String::AsciiStringManip::Caseless
    MIME_ENCODED_HTTPS_PREFIX("https%3a%2f%2f");
  const String::AsciiStringManip::Caseless
    MIME_ENCODED_SHEME_RELATIVE_PREFIX("%2f%2f");

  const String::SubString EQL("*eql*");
  const String::SubString AMP("*amp*");

  namespace Tokens
  {
    const String::SubString PRECLICK("PUBPRECLICK");
    const String::SubString CLICK_URL("CRCLICK");
  }

  namespace Aspect
  {
    const char CLICK_FRONTEND[] = "ClickFrontend";
  }

  namespace Request
  {
    namespace Param
    {
      const char RELOCATE[] = "relocate";
    }
  }

  namespace Response
  {
    namespace Header
    {
      const String::SubString LOCATION("Location");
    }
  }

  struct ChannelMatch
  {
    ChannelMatch(unsigned long channel_id_val,
                 unsigned long channel_trigger_id_val)
      :
      channel_id(channel_id_val),
      channel_trigger_id(channel_trigger_id_val)
    {}

    bool operator<(const ChannelMatch& right) const
    {
      return
        (channel_id < right.channel_id ||
         (channel_id == right.channel_id &&
          channel_trigger_id < right.channel_trigger_id));
    }

    unsigned long channel_id;
    unsigned long channel_trigger_id;
  };

  struct GetChannelTriggerId
  {
    ChannelMatch
    operator() (
      const adserver::channel_svcs::channel_server::ChannelAtom& atom)
      noexcept
    {
      return ChannelMatch(atom.id(), atom.trigger_channel_id());
    }
  };
}

namespace Config
{
  namespace
  {
    const char ENABLE[] = "ClickFrontend_Enable";
    const char CONFIG_FILE[] = "ClickFrontend_Config";
  }
}

namespace AdServer
{
  ClickFrontend::ClickFrontend(
    Configuration* frontend_config,
    Logging::Logger* logger,
    CommonModule* common_module)
    /*throw(eh::Exception)*/
    : Logging::LoggerCallbackHolder(
        Logging::Logger_var(
          new Logging::SeveritySelectorLogger(
            logger,
            0,
            frontend_config->get().ClickFeConfiguration()->Logger().log_level())),
        "ClickFrontend",
        Aspect::CLICK_FRONTEND,
        0),
      frontend_config_(ReferenceCounting::add_ref(frontend_config)),
      common_module_(ReferenceCounting::add_ref(common_module)),
      match_task_count_(0)
  {}

  bool
  ClickFrontend::will_handle(const String::SubString& uri) noexcept
  {
    std::string found_uri;

    bool result =
      FrontendCommons::find_uri(
        config_->PathUriList().Uri(), uri, found_uri, 0, false) ||
      FrontendCommons::find_uri(
        config_->UriList().Uri(), uri, found_uri);

    if(log_level() >= TraceLevel::MIDDLE)
    {
      Stream::Error ostr;
      ostr
        << "ClickFrontend::will_handle(" << uri
        << "), result " << result << ", for service: '"
        << found_uri << "'";

      log(ostr.str());
    }

    return result;
  }

  void
  ClickFrontend::parse_config_() /*throw(Exception)*/
  {
    static const char* FUN = "ClickFrontend::parse_config_()";

    try
    {
      typedef Configuration::FeConfig Config;
      const Config& fe_config = frontend_config_->get();

      if(!fe_config.CommonFeConfiguration().present())
      {
        throw Exception("CommonFeConfiguration not presented.");
      }

      common_config_ = CommonConfigPtr(
        new CommonFeConfiguration(*fe_config.CommonFeConfiguration()));

      if(!fe_config.ClickFeConfiguration().present())
      {
        throw Exception("ClickFeConfiguration not presented.");
      }

      config_ = ConfigPtr(
        new ClickFeConfiguration(*fe_config.ClickFeConfiguration()));

      cookie_manager_.reset(
        new FrontendCommons::CookieManager<
          FCGI::HttpRequest, FCGI::HttpResponse>(
            common_config_->Cookies()));
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't parse config file '" << config_file_ <<
        "': " << e.what();
      throw Exception(ostr);
    }
  }

  void
  ClickFrontend::handle_request_noparams(
    FCGI::HttpRequestHolder_var request_holder,
    FCGI::BaseHttpResponseWriter_var response_writer)
    noexcept
  {
    workers_->post(
      [this,
        request_holder = std::move(request_holder),
        response_writer = std::move(response_writer)]() mutable
      {
        handle_request_noparams_(
          std::move(request_holder),
          std::move(response_writer));
      });
  }

  void
  ClickFrontend::handle_request_noparams_(
    FCGI::HttpRequestHolder_var request_holder,
    FCGI::BaseHttpResponseWriter_var response_writer)
    noexcept
  {
    FCGI::HttpRequest& request = request_holder->request();

    HTTP::ParamList params;

    if (!request.args().empty())
    {
      String::StringManip::SplitAmp tokenizer(request.args());
      String::SubString token;

      while (tokenizer.get_token(token))
      {
        String::SubString enc_name;
        String::SubString enc_value;
        String::SubString::SizeType pos = token.find('=');

        if (pos == String::SubString::NPOS)
        {
          enc_name = token;
        }
        else
        {
          enc_name = token.substr(0, pos);
          enc_value = token.substr(pos + 1);
        }

        try
        {
          HTTP::Param param;
          String::StringManip::mime_url_decode(enc_name, param.name);

          if (param.name == Request::Param::RELOCATE)
          {
            // relocate is a last parameter
            // we supported preclick isn't last in encoded case only
            if (!MIME_ENCODED_HTTP_PREFIX.start(enc_value) &&
              !MIME_ENCODED_HTTPS_PREFIX.start(enc_value) &&
              !MIME_ENCODED_SHEME_RELATIVE_PREFIX.start(enc_value))
            {
              enc_value.assign_to(param.value);
              params.push_back(std::move(param));
              break;
            }
          }

          String::StringManip::mime_url_decode(enc_value, param.value);
          params.push_back(std::move(param));
        }
        catch (const String::StringManip::InvalidFormatException&)
        {}
      }
    }

    if (!request.body().empty())
    {
      FCGI::HttpRequest::parse_params(request.body(), params);
    }

    request.set_params(std::move(params));
    handle_request_(std::move(request_holder), std::move(response_writer));
  }

  void
  ClickFrontend::init() /*throw(eh::Exception)*/
  {
    static const char* FUN = "ClickFrontend::init()";

    if(true) // module_used())
    {
      try
      {
        parse_config_();

        request_info_filler_.reset(
          new ClickFE::RequestInfoFiller(
            logger(),
            common_module_,
            common_config_->GeoIP().present() ?
              common_config_->GeoIP()->path().c_str() : 0));
        grpc_executor_ = std::make_shared<AdServer::Grpc::GrpcExecutor>(
          common_config_->grpc_executor_threads());
        add_child_object(grpc_executor_);

        workers_ = new FrontendCommons::FrontendWorkers(
          callback(),
          config_->threads());
        add_child_object(workers_);

        auto campaign_manager = std::make_shared<
          AdServer::CampaignSvcs::CampaignManagerDistributedGrpcClient>(
            FrontendCommons::read_campaign_manager_grpc_refs(*common_config_),
            AdServer::Grpc::BatchingOptions(),
            grpc_executor_);
        campaign_manager_ = campaign_manager;
        add_child_object(campaign_manager);

        auto user_bind_objects =
          AdServer::UserInfoSvcs::create_distributed_user_bind_client(
            *common_config_,
            grpc_executor_,
            logger());
        user_bind_client_ = user_bind_objects.client;
        if(user_bind_client_)
        {
          add_child_object(user_bind_objects.active_object);
        }

        user_info_client_ =
          AdServer::UserInfoSvcs::create_distributed_user_info_client(
            *common_config_,
            grpc_executor_,
            logger(),
            this);

        auto channel_client_objects =
          AdServer::ChannelSvcs::create_distributed_channel_client(
            *common_config_,
            grpc_executor_);
        channel_client_ = channel_client_objects.client;
        add_child_object(channel_client_objects.active_object);

        template_files_ = new Commons::TextTemplateCache(
          static_cast<unsigned long>(-1),
          Generics::Time::ONE_MINUTE,
          Commons::TextTemplateCacheConfiguration<Commons::TextTemplate>(
            Generics::Time::ONE_SECOND));

        click_template_file_ = config_->template_file();

        try
        {
          if (common_config_->GeoIP().present())
          {
            ip_map_ = IPMapPtr(new GeoIPMapping::IPMapCity2(
              common_config_->GeoIP()->path().c_str()));
          }
        }
        catch (const GeoIPMapping::IPMap::Exception& e)
        {
          Stream::Error ostr;
          ostr << FUN << ": GeoIPMapping::IPMap::Exception caught: " << e.what();

          logger()->log(ostr.str(),
            Logging::Logger::CRITICAL,
            Aspect::CLICK_FRONTEND,
            "ADS-IMPL-102");
        }

        match_workers_ = new FrontendCommons::FrontendWorkers(
          callback(),
          config_->threads());
        add_child_object(match_workers_);

        set_uid_controller_ = new SetUidController(
          common_module_->user_id_controller(),
          config_->set_uid(),
          config_->probe_uid());

        activate_object();
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << ex.what();
        throw Exception(ostr);
      }
    }
  }

  void
  ClickFrontend::handle_request(
    FCGI::HttpRequestHolder_var request_holder,
    FCGI::BaseHttpResponseWriter_var response_writer)
    noexcept
  {
    workers_->post(
      [this,
        request_holder = std::move(request_holder),
        response_writer = std::move(response_writer)]() mutable
      {
        handle_request_(
          std::move(request_holder),
          std::move(response_writer));
      });
  }

  void
  ClickFrontend::shutdown() noexcept
  {
    try
    {
      deactivate_object();
      wait_object();
      clear();

      log(String::SubString(
          "ClickFrontend::shutdown: frontend terminated"),
        Logging::Logger::INFO,
        Aspect::CLICK_FRONTEND);
    }
    catch(...)
    {
    }
  }

  void
  ClickFrontend::fill_match_request_info_(
    adserver::campaign_svcs::campaign_manager::MatchRequestInfo& mri,
    const AdServer::Commons::UserId& user_id,
    const Generics::Time& now,
    const adserver::channel_svcs::channel_server::MatchResponse* trigger_match_result,
    const AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult* history_match_result,
    const String::SubString& peer_ip_val)
    const noexcept
  {
    /*
      Don't fill:
        mri.match_info.coord_location
    */

    auto* match_info = mri.mutable_match_info();
    match_info->set_colo_id(common_config_->colo_id());
    mri.set_user_id(GrpcAlgs::pack_user_id(user_id));
    mri.set_request_time(GrpcAlgs::pack_time(now));

    {
      const int result_len =
        trigger_match_result->matched_channels().page_channels_size();
      for(int i = 0; i < result_len; ++i)
      {
        auto* pkw_channel = match_info->add_pkw_channels();
        pkw_channel->set_channel_id(
          trigger_match_result->matched_channels().page_channels(i).id());
        pkw_channel->set_channel_trigger_id(
          trigger_match_result->matched_channels().
            page_channels(i).trigger_channel_id());
      }
    }

    if(history_match_result)
    {
      CORBA::ULong result_len =
        history_match_result->channels.length();
      for(CORBA::ULong i = 0; i < result_len; ++i)
      {
        match_info->add_channels(history_match_result->channels[i].channel_id);
      }
    }

    if (!peer_ip_val.empty() && ip_map_.get())
    {
      try
      {
        GeoIPMapping::IPMapCity2::CityLocation geo_location;

        if(ip_map_->city_location_by_addr(
             peer_ip_val.str().c_str(),
             geo_location,
             false))
        {
          FrontendCommons::Location_var location = new FrontendCommons::Location();
          location->country = geo_location.country_code.str();
          geo_location.region.assign_to(location->region);
          location->city = geo_location.city.str();
          location->normalize();

          auto* geo_info = match_info->add_location();
          geo_info->set_country(location->country);
          geo_info->set_region(location->region);
          geo_info->set_city(location->city);
        }
      }
      catch(const eh::Exception&)
      {}
    }
  }

  void
  ClickFrontend::check_constraints_(
    const FrontendCommons::ParsedParamsMap& params,
    const FCGI::HttpRequest& request) const
    /*throw(ForbiddenException, InvalidParamException)*/
  {
    static const char* FUN = "ClickFrontend::check_constraints_()";

    FrontendCommons::OnlyGetAllowed::apply(request);

    if (params.size() > ClickFrontendConstrainTraits::MAX_NUMBER_PARAMS)
    {
      Stream::Error ostr;
      ostr << FUN << ": Params number(" << params.size() << ") exceed";

      throw InvalidParamException(ostr);
    }

    for (auto it = params.begin(); it != params.end(); ++it)
    {
      if (it->first.size() >
        ClickFrontendConstrainTraits::MAX_LENGTH_PARAM_NAME)
      {
        Stream::Error ostr;
        ostr << FUN << ": Param name length(" << it->first.size() <<
          ") exceed";

        throw InvalidParamException(ostr);
      }
      else if (it->second.size() >
        ClickFrontendConstrainTraits::MAX_LENGTH_PARAM_VALUE)
      {
        Stream::Error ostr;
        ostr << FUN << ": Param value length(" << it->second.size() <<
          ") exceed";

        throw InvalidParamException(ostr);
      }
    }
  }

  void
  ClickFrontend::handle_request_(
    FCGI::HttpRequestHolder_var request_holder,
    FCGI::BaseHttpResponseWriter_var response_writer)
    noexcept
  {
    static const char* FUN = "ClickFrontend::handle_request()";

    const FCGI::HttpRequest& request = request_holder->request();

    FCGI::HttpResponse_var response_ptr(new FCGI::HttpResponse());

    log(String::SubString("ClickFrontend::handle_request: entered"),
      TraceLevel::MIDDLE,
      Aspect::CLICK_FRONTEND);

    int http_status = 200;

    try
    {
      std::string found_uri;
      bool params_in_path = FrontendCommons::find_uri(
        config_->PathUriList().Uri(), request.uri(), found_uri, 0, false);
      FrontendCommons::ParsedParamsMap parsed_params;

      if(params_in_path)
      {
        std::string arguments = request.uri().substr(found_uri.length()).str();

        if(!request.args().empty())
        {
          arguments += std::string("?") + request.args().str();
        }

        FrontendCommons::parse_args(
          parsed_params,
          arguments,
          AMP,
          EQL);
      }
      else // !params_in_path
      {
        const HTTP::ParamList& params = request.params();
        for(HTTP::ParamList::const_iterator it = params.begin();
            it != params.end(); ++it)
        {
          // use last defined parameter when requestid defined for both frontends
          parsed_params[it->name] = it->value;
        }
      }

      check_constraints_(parsed_params, request);

      if(log_level() >= TraceLevel::MIDDLE)
      {
        std::ostringstream ostr;
        ostr << FUN << ": " << std::endl <<
          "Uri: " << request.uri() << std::endl <<
          "Arg: " << request.args() << std::endl <<
          "Params ("<< parsed_params.size() << "):"  << std::endl;

        for(std::map<std::string, std::string>::const_iterator it =
              parsed_params.begin();
            it != parsed_params.end(); ++it)
        {
          ostr << "    " << it->first << " : " << it->second << std::endl;
        }

        ostr << "Headers ("<< request.headers().size() << "):"  << std::endl;

        for (HTTP::SubHeaderList::const_iterator it =
          request.headers().begin(); it != request.headers().end(); ++it)
        {
          ostr << "    " << it->name << " : " << it->value << std::endl;
        }

        log(ostr.str(),
          TraceLevel::MIDDLE,
          Aspect::CLICK_FRONTEND);
      }

      adserver::campaign_svcs::campaign_manager::ClickInfo click_info;
      ClickFE::RequestInfo request_info;
      request_info_filler_->fill(request_info, request, parsed_params);

      {
        click_info.set_colo_id(request_info.colo_id);
        click_info.set_tag_id(request_info.tag_id);
        click_info.set_tag_size_id(request_info.tag_size_id);
        click_info.set_ccid(request_info.ccid);
        click_info.set_creative_id(request_info.creative_id);
        click_info.set_ccg_keyword_id(request_info.ccg_keyword_id);
        auto* user_id_hash_mod = click_info.mutable_user_id_hash_mod();
        user_id_hash_mod->set_defined(request_info.user_id_hash_mod_defined);
        user_id_hash_mod->set_value(request_info.user_id_hash_mod_value);
        click_info.set_relocate(request_info.relocate);
        click_info.set_time(GrpcAlgs::pack_time(request_info.request_time));
        click_info.set_bid_time(GrpcAlgs::pack_time(request_info.bid_time));
        click_info.set_request_id(
          GrpcAlgs::pack_request_id(request_info.request_id));
        click_info.set_referer(request_info.referer);
        click_info.set_log_click(true);
        click_info.mutable_ctr()->set_value(
          GrpcAlgs::pack_decimal(request_info.ctr));
        click_info.set_match_user_id(
          GrpcAlgs::pack_user_id(request_info.match_user_id));
        click_info.set_cookie_user_id(
          GrpcAlgs::pack_user_id(request_info.cookie_user_id));

        if(!request_info.match_user_id.is_null())
        {
          auto* token = click_info.add_tokens();
          token->set_name("UNSIGNEDUID");
          token->set_value(request_info.match_user_id.to_string());
        }

        if(!request_info.cookie_user_id.is_null())
        {
          auto* token = click_info.add_tokens();
          token->set_name("UNSIGNEDCOOKIEUID");
          token->set_value(request_info.cookie_user_id.to_string());
        }

        for(auto it = request_info.tokens.begin();
          it != request_info.tokens.end(); ++it)
        {
          auto* token = click_info.add_tokens();
          token->set_name(it->first);
          token->set_value(it->second);
        }
      }

      /*
      {
        logger()->sstream(Logging::Logger::CRITICAL, Aspect::CLICK_FRONTEND) <<
          FUN << ": click_info: " <<
          "colo_id: " << click_info.colo_id <<
          ", tag_id: " << click_info.tag_id <<
          ", tag_size_id: " << click_info.tag_size_id <<
          ", ccid: " << click_info.ccid <<
          ", creative_id: " << click_info.creative_id <<
          ", ccg_keyword_id: " << click_info.ccg_keyword_id <<
          ", user_id_hash_mod.defined: " << click_info.user_id_hash_mod.defined <<
          ", user_id_hash_mod.value: " << click_info.user_id_hash_mod.value <<
          ", relocate: " << click_info.relocate <<
          ", time (before packing): " << request_info.request_time <<
          ", request_id (before packing): " << request_info.request_id <<
          ", referer: " << click_info.referer <<
          ". request_info: " <<
          "relocate: " << request_info.relocate <<
          ", preclick_url: " << request_info.preclick_url <<
          ", click_prefix: " << request_info.click_prefix <<
          ", campaign_manager_index: " << request_info.campaign_manager_index <<
          ", use_click_template: " << request_info.use_click_template;
      }
      */

      adserver::campaign_svcs::campaign_manager::ClickResultInfo
        click_result_info;
      auto finish_response =
        [this,
          request_holder,
          response_writer,
          response_ptr,
          click_info,
          request_info,
          FUN](
            bool got_click_url,
            const adserver::campaign_svcs::campaign_manager::ClickResultInfo&
              click_result_info) mutable
        {
          int http_status = 200;
          FCGI::HttpResponse& response = *response_ptr;
          const FCGI::HttpRequest& request = request_holder->request();

          try
          {
            AdServer::SetUidController::SetUidPtr set_uid =
              set_uid_controller_->generate_if_allowed(
                request_info.user_status,
                request_info.cookie_user_id,
                false);

            if(got_click_url && (
              (!request_info.match_user_id.is_null() && !(
                request_info.match_user_id ==
                  AdServer::Commons::PROBE_USER_ID)) ||
              (!request_info.cookie_user_id.is_null() && !(
                request_info.cookie_user_id ==
                  AdServer::Commons::PROBE_USER_ID)) ||
              set_uid))
            {
              const unsigned long current_task_count =
                match_task_count_.exchange_and_add(1) + 1;

              if(config_->match_task_limit() == 0 ||
                current_task_count <=
                  config_->match_task_limit() + config_->threads())
              {
                match_workers_->post(
                  [this,
                    user_id = set_uid ?
                    set_uid->client_id.uuid() :
                    request_info.match_user_id,
                    cookie_user_id = request_info.cookie_user_id,
                    now = GrpcAlgs::unpack_time(click_info.time()),
                    campaign_id = click_result_info.campaign_id(),
                    advertiser_id = click_result_info.advertiser_id(),
                    peer_ip = request_info.peer_ip,
                    markers = request_info.markers]()
                  {
                    match_click_channels_(
                      user_id,
                      cookie_user_id,
                      now,
                      campaign_id,
                      advertiser_id,
                      peer_ip,
                      markers);
                    match_task_count_ += -1;
                  });
              }
              else
              {
                match_task_count_ += -1;
                logger()->sstream(
                  Logging::Logger::ERROR,
                  Aspect::CLICK_FRONTEND,
                  "ADS-IMPL-198") << FUN <<
                  ": the limit of simultaneous matching tasks has been "
                  "reached";
              }
            }

            if(set_uid)
            {
              FrontendCommons::add_UID_cookie(
                response,
                request,
                *cookie_manager_,
                set_uid->client_id.str());
            }

            if(common_config_->ResponseHeaders().present())
            {
              FrontendCommons::add_headers(
                *(common_config_->ResponseHeaders()),
                response);
            }

            FrontendCommons::CORS::set_headers(request, response);

            bool instantiated = false;

            if(request_info.use_click_template)
            {
              try
              {
                Commons::TextTemplate_var templ =
                  template_files_->get(click_template_file_);

                typedef std::map<String::SubString, std::string> ArgMap;
                ArgMap args_cont;
                if(!request_info.preclick_url.empty())
                {
                  args_cont[Tokens::PRECLICK] = request_info.preclick_url;
                }

                if(got_click_url)
                {
                  args_cont[Tokens::CLICK_URL] = request_info.click_prefix +
                    click_result_info.url();
                }

                String::TextTemplate::ArgsContainer<ArgMap> args(&args_cont);
                String::TextTemplate::DefaultValue args_with_default(&args);
                String::TextTemplate::ArgsEncoder args_with_encoding(
                  &args_with_default);
                std::string response_content = templ->instantiate(
                  args_with_encoding);

                response.set_content_type_nocopy(
                  FrontendCommons::ContentType::TEXT_HTML);
                response.get_output_stream().write(
                  response_content.data(),
                  response_content.size());
                http_status = 200;

                instantiated = true;
              }
              catch(const eh::Exception& ex)
              {
                logger()->sstream(
                  Logging::Logger::EMERGENCY,
                  Aspect::CLICK_FRONTEND,
                  "ADS-IMPL-?") <<
                  FUN << ": eh::Exception has been caught: " << ex.what();
              }
            }

            if(!instantiated)
            {
              if(got_click_url)
              {
                http_status = 302;
                response.add_header_nocopy_name(
                  Response::Header::LOCATION,
                  request_info.click_prefix + click_result_info.url());

                if(log_level() >= TraceLevel::MIDDLE)
                {
                  Stream::Error ostr;
                  ostr << FUN << ": redirecting to " <<
                    click_result_info.url();

                  log(ostr.str(),
                    TraceLevel::MIDDLE,
                    Aspect::CLICK_FRONTEND);
                }
              }
              else
              {
                log(String::SubString("DO NOT redirecting !"),
                  TraceLevel::MIDDLE,
                  Aspect::CLICK_FRONTEND);

                http_status = 204;
              }
            }
          }
          catch(const eh::Exception& e)
          {
            http_status = 500;
            Stream::Error ostr;
            ostr << FUN << ": eh::Exception caught:" << e.what();

            log(ostr.str(),
              Logging::Logger::EMERGENCY,
              Aspect::CLICK_FRONTEND);
          }

          response_writer->write(http_status, response_ptr);
        };

      if(click_info.ccid() != 0 || click_info.creative_id() != 0)
      {
        adserver::campaign_svcs::campaign_manager::GetClickUrlRequest
          click_url_request;
        *click_url_request.mutable_click_info() = click_info;
        click_url_request.set_service_index(request_info.campaign_manager_index);

        campaign_manager_->get_click_url(
          click_url_request,
          [this,
            finish_response = std::move(finish_response),
            FUN](
              const grpc::Status& status,
              const adserver::campaign_svcs::campaign_manager::
                GetClickUrlResponse& click_url_response) mutable
          {
            workers_->post(
              [this,
                status,
                click_url_response,
                finish_response = std::move(finish_response),
                FUN]() mutable
              {
                try
                {
                  if(!status.ok())
                  {
                    throw_grpc_exception_(
                      "CampaignManager::get_click_url()",
                      status);
                  }

                  finish_response(
                    click_url_response.found(),
                    click_url_response.click_result_info());
                }
                catch(const eh::Exception& e)
                {
                  Stream::Error ostr;
                  ostr << FUN << ": eh::Exception caught:" << e.what();

                  log(ostr.str(),
                    Logging::Logger::EMERGENCY,
                    Aspect::CLICK_FRONTEND);

                  adserver::campaign_svcs::campaign_manager::ClickResultInfo
                    empty_click_result;
                  finish_response(false, empty_click_result);
                }
              });
          });
      }
      else
      {
        finish_response(false, click_result_info);
      }

      return;
    }
    catch (const ForbiddenException& ex)
    {
      http_status = 403;
      logger()->sstream(TraceLevel::LOW, Aspect::CLICK_FRONTEND) <<
        FUN << ": ForbiddenException caught: " << ex.what();
    }
    catch (const InvalidParamException& ex)
    {
      http_status = 400;
      logger()->sstream(TraceLevel::MIDDLE, Aspect::CLICK_FRONTEND) <<
        FUN << ": InvalidParamException caught: " << ex.what();
    }
    catch (const Exception&)
    {
      http_status = 500;
    }
    catch(const eh::Exception& e)
    {
      http_status = 500;
      Stream::Error ostr;
      ostr << FUN << ": eh::Exception caught:" << e.what();

      log(ostr.str(),
        Logging::Logger::EMERGENCY,
        Aspect::CLICK_FRONTEND);
    }

    response_writer->write(http_status, response_ptr);
  }

  void
  ClickFrontend::match_click_channels_(
    const AdServer::Commons::UserId& user_id,
    const AdServer::Commons::UserId& cookie_user_id,
    const Generics::Time& now,
    ::CORBA::ULong campaign_id,
    ::CORBA::ULong advertiser_id,
    const String::SubString& peer_ip,
    const std::list<std::string>& markers)
    noexcept
  {
    static const char* FUN = "ClickFrontend::match_click_channels_()";

    struct State: public std::enable_shared_from_this<State>
    {
      ClickFrontend* self;
      AdServer::Commons::UserId user_id;
      AdServer::Commons::UserId cookie_user_id;
      AdServer::Commons::UserId resolved_cookie_user_id;
      Generics::Time now;
      ::CORBA::ULong campaign_id;
      ::CORBA::ULong advertiser_id;
      std::string peer_ip;
      std::list<std::string> markers;
      adserver::channel_svcs::channel_server::MatchResponse trigger_match_result;
      bool trigger_match_result_present = false;
      AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult_var
        history_match_result;

      adserver::user_info_svcs::user_info_manager::MatchRequest
      make_history_match_request(const AdServer::Commons::UserId& match_user_id)
      {
        adserver::user_info_svcs::user_info_manager::MatchRequest request;
        auto* match_params = request.mutable_match_params();
        match_params->set_use_empty_profile(false);
        match_params->set_silent_match(false);
        match_params->set_no_match(false);
        match_params->set_no_result(true);
        match_params->set_ret_freq_caps(false);
        match_params->set_provide_channel_count(false);
        match_params->set_provide_persistent_channels(false);
        match_params->set_change_last_request(false);
        match_params->set_filter_contextual_triggers(false);
        match_params->set_publishers_optin_timeout(
          GrpcAlgs::pack_time(Generics::Time::ZERO));

        auto* user_info = request.mutable_user_info();
        user_info->set_user_id(GrpcAlgs::pack_user_id(match_user_id));
        user_info->set_last_colo_id(-1);
        user_info->set_request_colo_id(self->common_config_->colo_id());
        user_info->set_current_colo_id(-1);
        user_info->set_temporary(false);
        user_info->set_time(now.tv_sec);

        typedef std::set<ChannelMatch> ChannelMatchSet;
        ChannelMatchSet page_channels;
        std::transform(
          trigger_match_result.matched_channels().page_channels().begin(),
          trigger_match_result.matched_channels().page_channels().end(),
          std::inserter(page_channels, page_channels.end()),
          GetChannelTriggerId());

        auto* page_channel_ids =
          request.mutable_match_params()->mutable_page_channel_ids();
        for(const auto& channel_match : page_channels)
        {
          auto* result = page_channel_ids->Add();
          result->set_channel_id(channel_match.channel_id);
          result->set_channel_trigger_id(channel_match.channel_trigger_id);
        }

        return request;
      }

      void log_user_bind_error(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": caught UserBindClient exception: " << ex.what();
        self->logger()->log(
          ostr.str(),
          Logging::Logger::ERROR,
          Aspect::CLICK_FRONTEND,
          "ADS-IMPL-109");
      }

      void resolve_cookie()
      {
        if(cookie_user_id.is_null() || user_id == cookie_user_id)
        {
          match_history_user();
          return;
        }

        const std::string cookie_external_id_str =
          std::string("c/") + cookie_user_id.to_string();

        adserver::user_info_svcs::user_bind::GetUserIdRequest get_request_info;
        get_request_info.set_id(cookie_external_id_str);
        get_request_info.set_timestamp(GrpcAlgs::pack_time(now));
        get_request_info.set_silent(true);
        get_request_info.set_generate_user_id(false);
        get_request_info.set_for_set_cookie(false);
        get_request_info.set_create_timestamp(
          GrpcAlgs::pack_time(Generics::Time::ZERO));
        get_request_info.set_current_user_id(
          GrpcAlgs::pack_user_id(cookie_user_id));

        auto self_state = shared_from_this();
        self->user_bind_client_->get_user_id(
          get_request_info,
          [self_state](
            const grpc::Status& status,
            const adserver::user_info_svcs::user_bind::GetUserIdResponse&
              response)
          {
            self_state->self->workers_->post(
              [self_state, status, response]()
              {
                try
                {
                  if(!status.ok())
                  {
                    throw_user_bind_exception_(status);
                  }
                  self_state->resolved_cookie_user_id =
                    GrpcAlgs::unpack_user_id(response.user_id());
                }
                catch(const eh::Exception& ex)
                {
                  self_state->log_user_bind_error(ex);
                }
                self_state->match_history_user();
              });
          });
      }

      void match_history_user()
      {
        if(!trigger_match_result_present ||
          trigger_match_result.matched_channels().page_channels_size() == 0)
        {
          return;
        }

        if(user_id == AdServer::Commons::PROBE_USER_ID)
        {
          match_history_cookie();
          return;
        }

        auto request = make_history_match_request(user_id);
        auto self_state = shared_from_this();
        self->user_info_client_->match(
          request,
          [self_state](
            const grpc::Status& status,
            const adserver::user_info_svcs::user_info_manager::MatchResponse&
              response)
          {
            self_state->self->workers_->post(
              [self_state, status, response]()
              {
                try
                {
                  if(!status.ok())
                  {
                    throw_user_info_exception_(
                      "UserInfoManager::match()",
                      status);
                  }
                  self_state->history_match_result =
                    AdServer::UserInfoSvcs::GrpcAlgs::
                      make_history_match_result(response);
                }
                catch(const UserInfoSvcs::UserInfoMatcher::
                  ImplementationException& e)
                {
                  Stream::Error ostr;
                  ostr << FUN << ": UserInfoSvcs::UserInfoMatcher::"
                    "ImplementationException caught: " << e.description;

                  self_state->self->logger()->log(
                    ostr.str(),
                    Logging::Logger::EMERGENCY,
                    Aspect::CLICK_FRONTEND,
                    "ADS-IMPL-112");
                }
                catch(const UserInfoSvcs::UserInfoMatcher::NotReady&)
                {
                  self_state->self->logger()->log(
                    String::SubString(
                      "UserInfoManager not ready for matching."),
                    TraceLevel::MIDDLE,
                    Aspect::CLICK_FRONTEND);
                }
                self_state->match_history_cookie();
              });
          });
      }

      void match_history_cookie()
      {
        if(user_id == resolved_cookie_user_id ||
          resolved_cookie_user_id.is_null())
        {
          process_match();
          return;
        }

        auto request = make_history_match_request(resolved_cookie_user_id);
        auto self_state = shared_from_this();
        self->user_info_client_->match(
          request,
          [self_state](const grpc::Status& status, const auto&)
          {
            self_state->self->workers_->post(
              [self_state, status]()
              {
                if(!status.ok())
                {
                  try
                  {
                    throw_user_info_exception_(
                      "UserInfoManager::match()",
                      status);
                  }
                  catch(const eh::Exception& ex)
                  {
                    Stream::Error ostr;
                    ostr << FUN << ": cookie history match failed: " <<
                      ex.what();
                    self_state->self->logger()->log(
                      ostr.str(),
                      Logging::Logger::EMERGENCY,
                      Aspect::CLICK_FRONTEND,
                      "ADS-IMPL-112");
                  }
                }
                self_state->process_match();
              });
          });
      }

      void process_match()
      {
        adserver::campaign_svcs::campaign_manager::ProcessMatchRequestRequest
          process_match_request;
        self->fill_match_request_info_(
          *process_match_request.mutable_match_request_info(),
          user_id,
          now,
          &trigger_match_result,
          history_match_result,
          peer_ip);

        auto self_state = shared_from_this();
        self->campaign_manager_->process_match_request(
          process_match_request,
          [self_state](const grpc::Status& status, const auto&)
          {
            self_state->self->workers_->post(
              [self_state, status]()
              {
                if(!status.ok())
                {
                  Stream::Error ostr;
                  ostr << FUN << ": Can't process match request. "
                    "Possible problem with Campaignmanager. gRPC code=" <<
                    static_cast<int>(status.error_code()) <<
                    ", message=" << status.error_message();
                  self_state->self->logger()->log(
                    ostr.str(),
                    Logging::Logger::EMERGENCY,
                    Aspect::CLICK_FRONTEND,
                    "ADS-ICON-4");
                }
              });
          });
      }
    };

    auto state = std::make_shared<State>();
    state->self = this;
    state->user_id = user_id;
    state->cookie_user_id = cookie_user_id;
    state->resolved_cookie_user_id = cookie_user_id;
    state->now = now;
    state->campaign_id = campaign_id;
    state->advertiser_id = advertiser_id;
    state->peer_ip = peer_ip.str();
    state->markers = markers;

    adserver::channel_svcs::channel_server::MatchRequest channel_request;
    channel_request.set_non_strict_word_match(false);
    channel_request.set_non_strict_url_match(false);
    channel_request.set_return_negative(false);
    channel_request.set_simplify_page(false);
    channel_request.set_fill_content(false);
    channel_request.set_statuses("A", 2);
    std::ostringstream keywords_ostr;
    keywords_ostr << "poadclick poadclicka" << advertiser_id <<
      " poadclickc" << campaign_id;
    for(auto mit = markers.begin(); mit != markers.end(); ++mit)
    {
      std::string base_trigger = std::string("poad") + *mit + "click";
      keywords_ostr << " " << base_trigger <<
        " " << base_trigger << "a" << advertiser_id <<
        " " << base_trigger << "c" << campaign_id;
    }
    channel_request.set_pwords(keywords_ostr.str());

    channel_client_->match(
      channel_request,
      [state](
        const grpc::Status& status,
        const adserver::channel_svcs::channel_server::MatchResponse& response)
      {
        state->self->workers_->post(
          [state, status, response]()
          {
            try
            {
              if(!status.ok())
              {
                throw_grpc_exception_("ChannelServer::match()", status);
              }
              state->trigger_match_result = response;
              state->trigger_match_result_present = true;
            }
            catch(const eh::Exception& ex)
            {
              Stream::Error ostr;
              ostr << FUN <<
                ": caught ChannelServerGrpcAsyncClient error: " <<
                ex.what();
              state->self->logger()->log(
                ostr.str(),
                Logging::Logger::EMERGENCY,
                Aspect::CLICK_FRONTEND,
                "ADS-IMPL-117");
            }
            state->resolve_cookie();
          });
      });
  }

  bool
  ClickFrontend::log(
    const String::SubString& text,
    unsigned long severity,
    const char* aspect,
    const char* error_code) const
    noexcept
  {
    if (!logger())
    {
      try
      {
        std::cerr << text << std::endl;
        return true;
      }
      catch(...)
      {
        return false;
      }
    }
    else
    {
      return logger()->log(text, severity, aspect, error_code);
    }
  }

}
