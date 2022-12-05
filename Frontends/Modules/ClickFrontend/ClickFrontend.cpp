
#include <sstream>

#include <Generics/Rand.hpp>
#include <Logger/StreamLogger.hpp>
#include <String/StringManip.hpp>
#include <String/AsciiStringManip.hpp>
#include <HTTP/HTTPCookie.hpp>
#include <HTTP/UrlAddress.hpp>

#include <CORBACommons/CorbaAdapters.hpp>

#include <Commons/ErrorHandler.hpp>
#include <Commons/CorbaConfig.hpp>
#include <Commons/CorbaAlgs.hpp>
#include <Commons/UserInfoManip.hpp>

#include <Frontends/FrontendCommons/Cookies.hpp>
#include <Frontends/FrontendCommons/HTTPUtils.hpp>
#include <Frontends/FrontendCommons/add_UID_cookie.hpp>
#include <Frontends/FrontendCommons/Location.hpp>
#include <Frontends/FrontendCommons/GeoInfoUtils.hpp>

#include <Frontends/CommonModule/CommonModule.hpp>

#include "ClickFrontend.hpp"

namespace
{
  struct ClickFrontendConstrainTraits
  {
    static const unsigned long MAX_NUMBER_PARAMS = 30;
    static const unsigned long MAX_LENGTH_PARAM_NAME = 50;
    static const unsigned long MAX_LENGTH_PARAM_VALUE = 1024;
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
      const AdServer::ChannelSvcs::ChannelServerBase::ChannelAtom& atom)
      noexcept
    {
      return ChannelMatch(atom.id, atom.trigger_channel_id);
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
      FrontendCommons::FrontendTaskPool(
        this->callback(),
        frontend_config->get().ClickFeConfiguration()->threads(),
        0), // max pending tasks
      frontend_config_(ReferenceCounting::add_ref(frontend_config)),
      common_module_(ReferenceCounting::add_ref(common_module)),
      campaign_managers_(this->logger(), Aspect::CLICK_FRONTEND)
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
  ClickFrontend::handle_request_noparams_(
    FCGI::HttpRequestHolder_var request_holder,
    FCGI::HttpResponseWriter_var response_writer)
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

        corba_client_adapter_ = new CORBACommons::CorbaClientAdapter();

        campaign_managers_.resolve(
          *common_config_, corba_client_adapter_);

        user_bind_client_ = new FrontendCommons::UserBindClient(
          common_config_->UserBindControllerGroup(),
          corba_client_adapter_.in(),
          logger());
        add_child_object(user_bind_client_);

        user_info_client_ = new FrontendCommons::UserInfoClient(
          common_config_->UserInfoManagerControllerGroup(),
          corba_client_adapter_.in(),
          logger());
        add_child_object(user_info_client_);

        CORBACommons::CorbaObjectRefList channel_manager_controller_refs;

        Config::CorbaConfigReader::read_multi_corba_ref(
          common_config_->ChannelManagerControllerRefs().get(),
          channel_manager_controller_refs);

        channel_servers_.reset(
          new FrontendCommons::ChannelServerSessionPool(
            channel_manager_controller_refs,
            corba_client_adapter_,
            callback()));

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

        task_runner_ = new Generics::TaskRunner(
          callback(), config_->threads(), 0, config_->match_task_limit());
        add_child_object(task_runner_);

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
  ClickFrontend::shutdown() noexcept
  {
    try
    {
      deactivate_object();
      wait_object();
      clear();

      corba_client_adapter_.reset();

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
    AdServer::CampaignSvcs::CampaignManager::MatchRequestInfo& mri,
    const AdServer::Commons::UserId& user_id,
    const Generics::Time& now,
    const AdServer::ChannelSvcs::ChannelServerBase::MatchResult* trigger_match_result,
    const AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult* history_match_result,
    const String::SubString& peer_ip_val)
    const noexcept
  {
    /*
      Don't fill:
        mri.household_id
        mri.match_info.hid_channels
        mri.match_info.coord_location
    */

    mri.match_info.colo_id = common_config_->colo_id();
    mri.user_id = CorbaAlgs::pack_user_id(user_id);
    mri.request_time = CorbaAlgs::pack_time(now);

    {
      CORBA::ULong result_len =
        trigger_match_result->matched_channels.page_channels.length();
      mri.match_info.pkw_channels.length(result_len);
      for(CORBA::ULong i = 0; i < result_len; ++i)
      {
        mri.match_info.pkw_channels[i].channel_id =
          trigger_match_result->matched_channels.page_channels[i].id;
        mri.match_info.pkw_channels[i].channel_trigger_id =
          trigger_match_result->matched_channels.page_channels[i].trigger_channel_id;
      }
    }

    if(history_match_result)
    {
      CORBA::ULong result_len =
        history_match_result->channels.length();
      mri.match_info.channels.length(result_len);
      for(CORBA::ULong i = 0; i < result_len; ++i)
      {
        mri.match_info.channels[i] =
          history_match_result->channels[i].channel_id;
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

          FrontendCommons::fill_geo_location_info(
            mri.match_info.location,
            location);
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

  class ClickFrontend::MatchClickChannelsTask:
    public Generics::Task,
    public ReferenceCounting::AtomicImpl
  {
  public:
    MatchClickChannelsTask(
      ClickFrontend* click_frontend,
      const AdServer::Commons::UserId& user_id,
      const AdServer::Commons::UserId& cookie_user_id,
      const Generics::Time& now,
      const AdServer::CampaignSvcs::CampaignManager::ClickResultInfo& click_result_info,
      const String::SubString& peer_ip,
      const std::list<std::string>& markers)
      noexcept
      : click_frontend_(click_frontend),
        user_id_(user_id),
        cookie_user_id_(cookie_user_id),
        now_(now),
        campaign_id_(click_result_info.campaign_id),
        advertiser_id_(click_result_info.advertiser_id),
        peer_ip_(peer_ip.str()),
        markers_(markers)
    {}

    virtual
    void
    execute() noexcept
    {
      click_frontend_->match_click_channels_(
        user_id_,
        cookie_user_id_,
        now_,
        campaign_id_,
        advertiser_id_,
        peer_ip_,
        markers_);
    }

  protected:
    virtual
    ~MatchClickChannelsTask() noexcept
    {}

  private:
    ClickFrontend* click_frontend_;
    AdServer::Commons::UserId user_id_;
    AdServer::Commons::UserId cookie_user_id_;
    Generics::Time now_;
    ::CORBA::ULong campaign_id_;
    ::CORBA::ULong advertiser_id_;
    const std::string peer_ip_;
    const std::list<std::string> markers_;
  };

  void
  ClickFrontend::handle_request_(
    FCGI::HttpRequestHolder_var request_holder,
    FCGI::HttpResponseWriter_var response_writer)
    noexcept
  {
    static const char* FUN = "ClickFrontend::handle_request()";

    const FCGI::HttpRequest& request = request_holder->request();

    FCGI::HttpResponse_var response_ptr(new FCGI::HttpResponse());
    FCGI::HttpResponse& response = *response_ptr;

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
          // use last defined parameter (actual for baidu(static)
          // when requestid defined for both frontends
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

      AdServer::CampaignSvcs::CampaignManager::ClickInfo click_info;
      ClickFE::RequestInfo request_info;
      request_info_filler_->fill(request_info, request, parsed_params);

      {
        click_info.colo_id = request_info.colo_id;
        click_info.tag_id = request_info.tag_id;
        click_info.tag_size_id = request_info.tag_size_id;
        click_info.ccid = request_info.ccid;
        click_info.creative_id = request_info.creative_id;
        click_info.ccg_keyword_id = request_info.ccg_keyword_id;
        click_info.user_id_hash_mod.defined = request_info.user_id_hash_mod_defined;
        click_info.user_id_hash_mod.value = request_info.user_id_hash_mod_value;
        click_info.relocate << request_info.relocate;
        click_info.time = CorbaAlgs::pack_time(request_info.request_time);
        click_info.bid_time = CorbaAlgs::pack_time(request_info.bid_time);
        click_info.request_id = CorbaAlgs::pack_request_id(request_info.request_id);
        click_info.referer << request_info.referer;
        click_info.log_click = true;
        click_info.ctr = CorbaAlgs::pack_decimal(request_info.ctr);
        click_info.match_user_id = CorbaAlgs::pack_user_id(request_info.match_user_id);
        click_info.cookie_user_id = CorbaAlgs::pack_user_id(request_info.cookie_user_id);
        click_info.tokens.length(request_info.tokens.size() + 2);

        CORBA::ULong tok_i = 0;

        if(!request_info.match_user_id.is_null())
        {
          click_info.tokens[tok_i].name = "UNSIGNEDUID";
          click_info.tokens[tok_i].value << request_info.match_user_id.to_string();
          ++tok_i;
        }

        if(!request_info.cookie_user_id.is_null())
        {
          click_info.tokens[tok_i].name = "UNSIGNEDCOOKIEUID";
          click_info.tokens[tok_i].value << request_info.cookie_user_id.to_string();
          ++tok_i;
        }

        for(auto it = request_info.tokens.begin();
          it != request_info.tokens.end(); ++it, ++tok_i)
        {
          click_info.tokens[tok_i].name << it->first;
          click_info.tokens[tok_i].value << it->second;
        }

        click_info.tokens.length(tok_i);
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

      AdServer::CampaignSvcs::CampaignManager::ClickResultInfo_var click_result_info;
      CORBA::Boolean got_click_url = false;

      AdServer::SetUidController::SetUidPtr set_uid;

      if (click_info.ccid != 0 || click_info.creative_id != 0)
      {
        got_click_url = campaign_managers_.get_click_url(
          click_info,
          click_result_info,
          request_info.campaign_manager_index.c_str());

        set_uid = set_uid_controller_->generate_if_allowed(
          request_info.user_status,
          request_info.cookie_user_id,
          false);

        if((!request_info.match_user_id.is_null() && !(
              request_info.match_user_id == AdServer::Commons::PROBE_USER_ID)) ||
           (!request_info.cookie_user_id.is_null() && !(
              request_info.cookie_user_id == AdServer::Commons::PROBE_USER_ID)) ||
           set_uid)
        {
          try
          {
            // delay match click channels
            task_runner_->enqueue_task(new MatchClickChannelsTask(
              this,
              set_uid ? set_uid->client_id.uuid() : request_info.match_user_id,
              request_info.cookie_user_id,
              CorbaAlgs::unpack_time(click_info.time),
              click_result_info.in(),
              request_info.peer_ip,
              request_info.markers));
          }
          catch (const Generics::TaskRunner::Overflow& ex)
          {
            logger()->sstream(
              Logging::Logger::ERROR,
              Aspect::CLICK_FRONTEND,
              "ADS-IMPL-198") << FUN <<
              ": the limit of simultaneous matching tasks has been reached: " <<
              ex.what();
          }
        }
      }
      else
      {
        set_uid = set_uid_controller_->generate_if_allowed(
          request_info.user_status,
          request_info.cookie_user_id,
          false);
      }

      if (set_uid)
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

      if (request_info.use_click_template)
      {
        try
        {
          // instantiate click template
          Commons::TextTemplate_var templ = template_files_->get(click_template_file_);

          typedef std::map<String::SubString, std::string> ArgMap;
          ArgMap args_cont;
          if(!request_info.preclick_url.empty())
          {
            args_cont[Tokens::PRECLICK] = request_info.preclick_url;
          }

          if(got_click_url)
          {
            args_cont[Tokens::CLICK_URL] = request_info.click_prefix +
              click_result_info->url.in();
          }

          String::TextTemplate::ArgsContainer<ArgMap> args(&args_cont);
          String::TextTemplate::DefaultValue args_with_default(&args);
          String::TextTemplate::ArgsEncoder args_with_encoding(
            &args_with_default);
          std::string response_content = templ->instantiate(
            args_with_encoding);

          response.set_content_type(FrontendCommons::ContentType::TEXT_HTML);
          response.get_output_stream().write(
            response_content.data(), response_content.size());
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
          // do redirect to click_url
          http_status = 302;
          response.add_header(
            Response::Header::LOCATION,
            request_info.click_prefix + click_result_info->url.in());

          if(log_level() >= TraceLevel::MIDDLE)
          {
            Stream::Error ostr;
            ostr << FUN << ": redirecting to " << click_result_info->url.in();

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

    // do trigger match
    AdServer::ChannelSvcs::ChannelServerBase::MatchResult_var trigger_match_result;

    try
    {
      AdServer::ChannelSvcs::ChannelServerBase::MatchQuery query;
      query.non_strict_word_match = false;
      query.non_strict_url_match = false;
      query.return_negative = false;
      query.simplify_page = false;
      query.statuses[0] = 'A';
      query.statuses[1] = '\0';
      query.fill_content = false;
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

      query.pwords << keywords_ostr.str();

      channel_servers_->match(query, trigger_match_result);
    }
    catch(const FrontendCommons::ChannelServerSessionPool::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN <<
        ": caught ChannelServerSessionPool::Exception: " <<
        ex.what();
      logger()->log(ostr.str(),
        Logging::Logger::EMERGENCY,
        Aspect::CLICK_FRONTEND,
        "ADS-IMPL-117");
    }

    // resolve actual user id (cookies)
    AdServer::Commons::UserId resolved_cookie_user_id = cookie_user_id;

    assert(user_bind_client_.in());

    AdServer::UserInfoSvcs::UserBindMapper_var user_bind_mapper =
      user_bind_client_->user_bind_mapper();

    // resolve cookie user id
    try
    {
      // resolve cookie user id only if user id in params not equal to cookie user id
      if(!cookie_user_id.is_null() && user_id != cookie_user_id)
      {
        const std::string cookie_external_id_str =
          std::string("c/") + cookie_user_id.to_string();

        AdServer::UserInfoSvcs::UserBindMapper::GetUserRequestInfo get_request_info;
        get_request_info.id << cookie_external_id_str;
        get_request_info.timestamp = CorbaAlgs::pack_time(now);
        get_request_info.silent = true;
        get_request_info.generate_user_id = false;
        get_request_info.for_set_cookie = false;
        get_request_info.create_timestamp = CorbaAlgs::pack_time(Generics::Time::ZERO);
        get_request_info.current_user_id = CorbaAlgs::pack_user_id(cookie_user_id);

        AdServer::UserInfoSvcs::UserBindServer::GetUserResponseInfo_var prev_user_bind_info =
          user_bind_mapper->get_user_id(get_request_info);

        resolved_cookie_user_id = CorbaAlgs::unpack_user_id(prev_user_bind_info->user_id);
      }
    }
    catch(const AdServer::UserInfoSvcs::UserBindMapper::NotReady&)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught UserBindServer::NotReady";
      logger()->log(ostr.str(),
        Logging::Logger::EMERGENCY,
        Aspect::CLICK_FRONTEND,
        "ADS-IMPL-109");
    }
    catch(const AdServer::UserInfoSvcs::UserBindMapper::ChunkNotFound& )
    {
      Stream::Error ostr;
      ostr << FUN << ": caught UserBindMapper::ChunkNotFound";
      logger()->log(ostr.str(),
        Logging::Logger::ERROR,
        Aspect::CLICK_FRONTEND,
        "ADS-IMPL-109");
    }
    catch(const AdServer::UserInfoSvcs::UserBindMapper::ImplementationException& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught UserBindMapper::ImplementationException: " <<
        ex.description;
      logger()->log(ostr.str(),
        Logging::Logger::ERROR,
        Aspect::CLICK_FRONTEND,
        "ADS-IMPL-109");
    }
    catch(const CORBA::SystemException& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught CORBA::SystemException: " << e;
      logger()->log(ostr.str(),
        Logging::Logger::ERROR,
        Aspect::CLICK_FRONTEND,
        "ADS-ICON-6");
    }

    // do history match
    AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult_var history_match_result;

    if(trigger_match_result.ptr() != 0 &&
       trigger_match_result->matched_channels.page_channels.length() != 0)
    {
      AdServer::UserInfoSvcs::UserInfoMatcher_var
        uim_session = user_info_client_->user_info_session();

      try
      {
        // call UIM only if any page channel matched
        AdServer::UserInfoSvcs::UserInfoMatcher::MatchParams match_params;
        match_params.use_empty_profile = false;
        match_params.silent_match = false;
        match_params.no_match = false;
        match_params.no_result = true;
        match_params.ret_freq_caps = false;
        match_params.provide_channel_count = false;
        match_params.provide_persistent_channels = false;
        match_params.change_last_request = false;
        match_params.filter_contextual_triggers = false;
        match_params.publishers_optin_timeout =
          CorbaAlgs::pack_time(Generics::Time::ZERO);

        typedef std::set<ChannelMatch> ChannelMatchSet;
        ChannelMatchSet page_channels;

        std::transform(
          trigger_match_result->matched_channels.page_channels.get_buffer(),
          trigger_match_result->matched_channels.page_channels.get_buffer() +
          trigger_match_result->matched_channels.page_channels.length(),
          std::inserter(page_channels, page_channels.end()),
          GetChannelTriggerId());

        match_params.page_channel_ids.length(page_channels.size());
        CORBA::ULong res_ch_i = 0;
        for (ChannelMatchSet::const_iterator ch_it = page_channels.begin();
             ch_it != page_channels.end(); ++ch_it, ++res_ch_i)
        {
          match_params.page_channel_ids[res_ch_i].channel_id = ch_it->channel_id;
          match_params.page_channel_ids[res_ch_i].channel_trigger_id =
            ch_it->channel_trigger_id;
        }

        AdServer::UserInfoSvcs::UserInfo user_info;
        user_info.user_id = CorbaAlgs::pack_user_id(user_id);
        user_info.last_colo_id = -1;
        user_info.request_colo_id = common_config_->colo_id();
        user_info.current_colo_id = -1;
        user_info.temporary = false;
        user_info.time = now.tv_sec;

        if (user_id != AdServer::Commons::PROBE_USER_ID)
        {
          user_info.user_id = CorbaAlgs::pack_user_id(user_id);
          uim_session->match(
            user_info,
            match_params,
            history_match_result.out());
        }

        if (user_id != resolved_cookie_user_id && !resolved_cookie_user_id.is_null())
        {
          user_info.user_id = CorbaAlgs::pack_user_id(resolved_cookie_user_id);
          AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult_var local_history_match_result;
          uim_session->match(
            user_info,
            match_params,
            local_history_match_result.out());
        }
      }
      catch(const UserInfoSvcs::UserInfoMatcher::ImplementationException& e)
      {
        Stream::Error ostr;
        ostr << FUN <<
          ": UserInfoSvcs::UserInfoMatcher::ImplementationException caught: " <<
          e.description;

        logger()->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          Aspect::CLICK_FRONTEND,
          "ADS-IMPL-112");
      }
      catch(const UserInfoSvcs::UserInfoMatcher::NotReady& e)
      {
        logger()->log(
          String::SubString("UserInfoManager not ready for matching."),
          TraceLevel::MIDDLE,
          Aspect::CLICK_FRONTEND);
      }
      catch(const CORBA::SystemException& ex)
      {
        Stream::Error ostr;
        ostr << FUN <<
          ": Can't match history channels. Caught CORBA::SystemException: " <<
          ex;

        logger()->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          Aspect::CLICK_FRONTEND,
          "ADS-ICON-2");
      }

      try
      {
        AdServer::CampaignSvcs::CampaignManager::MatchRequestInfo request_info;
        fill_match_request_info_(
          request_info,
          user_id,
          now,
          trigger_match_result,
          history_match_result,
          peer_ip);

        campaign_managers_.process_match_request(request_info);
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": Can't process match request. "
          "Possible problem with Campaignmanager. Caught Exception: " <<
          ex.what();
        logger()->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          Aspect::CLICK_FRONTEND,
          "ADS-ICON-4");
      }
    } // trigger_match_result.ptr() != 0 && trigger_match_result->matched_channels.page_channels.length() != 0
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
