
#include <sstream>

#include <HTTP/HTTPCookie.hpp>
#include <HTTP/UrlAddress.hpp>
#include <Logger/StreamLogger.hpp>
#include <Logger/DistributorLogger.hpp>

#include <Commons/ErrorHandler.hpp>
#include <Commons/CorbaConfig.hpp>
#include <Commons/CorbaAlgs.hpp>
#include <Commons/GrpcAlgs.hpp>
#include <Commons/UserInfoManip.hpp>
#include <Commons/ExternalUserIdUtils.hpp>

#include <Frontends/FrontendCommons/add_UID_cookie.hpp>

#include "ActionFrontendStat.hpp"
#include <Frontends/FrontendCommons/UserInfoClientConfig.hpp>

#include "ActionFrontend.hpp"

namespace
{
  struct ActionFrontendConstrainTraits
  {
    static const unsigned long MAX_NUMBER_PARAMS = 30;
    static const unsigned long MAX_LENGTH_PARAM_NAME = 20;
    static const unsigned long MAX_LENGTH_PARAM_VALUE = 1024;
  };

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

  typedef FrontendCommons::DefaultConstrain<
    FrontendCommons::OnlyGetAllowed,
    FrontendCommons::ParamConstrainDefault,
    ActionFrontendConstrainTraits>
      ActionFrontendHTTPConstrain;

  namespace TemplateParams
  {
    const String::SubString MARKER("##");
    const String::SubString RANDOM("RANDOM");
  }

  const String::SubString HTTP_PREFIX("http:");
  const String::SubString HTTPS_PREFIX("https:");
}

namespace Config
{
  const char CONFIG_FILE[] = "ActionFrontend_Config";
  const char ENABLE[] = "ActionFrontend_Enable";
}

namespace Aspect
{
  extern const char ACTION_FRONTEND[] = "AcFrontend";
}

namespace Request
{
  namespace Cookie
  {
    const String::AsciiStringManip::Caseless OPTOUT("OPTED_OUT");
    const String::AsciiStringManip::Caseless OPTOUT_TRUE_VALUE("YES");
    const String::AsciiStringManip::Caseless USER_ID("uid");
  }

  namespace Param
  {
    const char CAMPAIGN_ID[] = "cid";
    const char CONVERSION_ID_OLD[] = "actionid";
    const char CONVERSION_ID[] = "convid";
    const char COUNTRY[] = "country";
    const char TEST_REQUEST[] = "testrequest";
    const char DEBUG_CURRENT_TIME[] = "debug-time";
  }

  namespace Header
  {
    const String::AsciiStringManip::Caseless REM_HOST(".RemoteHost");
    const String::AsciiStringManip::Caseless REFERER("Referer");
  }
}

namespace AdServer::Action
{
  struct Frontend::AdvertiserRequestState
  {
    FCGI::HttpRequestHolder_var request_holder;
    FCGI::BaseHttpResponseWriter_var response_writer;
    FCGI::HttpResponse_var response;
    RequestInfo request_info;
    bool return_html = false;
    Commons::UserId cookie_resolved_user_id;
    Commons::UserId utm_cookie_resolved_user_id;
  };

  class Frontend::MatchActionChannelsTask : public Generics::Task,
    public ReferenceCounting::AtomicImpl
  {
  public:
    MatchActionChannelsTask(
      Frontend* action_frontend,
      const Generics::Time& now,
      const AdServer::Commons::UserId& user_id,
      unsigned long conv_id,
      const String::SubString& referer)
      noexcept
      : user_id_(user_id),
        now_(now),
        conv_id_(conv_id),
        referer_(referer.str()),
        action_frontend_(action_frontend)
    {}

    virtual
    void
    execute() noexcept
    {
      action_frontend_->trigger_match_(
        conv_id_,
        now_,
        user_id_,
        referer_);
    }

  protected:
    virtual
    ~MatchActionChannelsTask() noexcept {};

  private:
    const AdServer::Commons::UserId user_id_;
    const Generics::Time now_;
    const unsigned long conv_id_;
    const std::string referer_;
    Frontend* action_frontend_;
  };

  Frontend::Frontend(
    Configuration* frontend_config,
    Logging::Logger* logger,
    CommonModule* common_module)
    /*throw(eh::Exception)*/
    : Logging::LoggerCallbackHolder(
        Logging::Logger_var(
          new Logging::SeveritySelectorLogger(
            logger,
            0,
            frontend_config->get().ActionFeConfiguration()->Logger().log_level())),
         "Action::Frontend",
        Aspect::ACTION_FRONTEND,
        0),
      FrontendCommons::FrontendTaskPool(
        this->callback(),
        frontend_config->get().ActionFeConfiguration()->threads(),
        0), // max pending tasks
      frontend_config_(ReferenceCounting::add_ref(frontend_config)),
      common_module_(ReferenceCounting::add_ref(common_module))
  {}

  void
  Frontend::parse_config_() /*throw(Exception)*/
  {
    static const char* FUN = "Frontend::parse_config_()";

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

      if(!fe_config.ActionFeConfiguration().present())
      {
        throw Exception("ActionFeConfiguration not presented.");
      }

      config_ = ConfigPtr(
        new ActionFeConfiguration(*fe_config.ActionFeConfiguration()));
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't parse config: " << e.what();
      throw Exception(ostr);
    }
  }

  bool
  Frontend::will_handle(const String::SubString& uri) noexcept
  {
    std::string found_uri;

    bool result =
      FrontendCommons::find_uri(
        derived_config_.advertiser_service_uri, uri, found_uri, 0, false) ||
      (config_->PixelUriList().present() &&
        FrontendCommons::find_uri(
          config_->PixelUriList()->Uri(), uri, found_uri)) ||
      (config_->UriList().present() &&
        FrontendCommons::find_uri(
          config_->UriList()->Uri(), uri, found_uri));

    if(logger()->log_level() >= TraceLevel::MIDDLE)
    {
      Stream::Error ostr;
      ostr << "Frontend::will_handle(" << uri <<
        "), service: '" << found_uri << "'";

      logger()->log(ostr.str());
    }

    return result;
  }

  void
  Frontend::init() /*throw(eh::Exception)*/
  {
    static const char* FUN = "Frontend::init()";

    if(true) // module_used())
    {
      try
      {
        parse_config_();

        if(config_->PathUriList().present())
        {
          for(xsd::AdServer::Configuration::UriListType::Uri_sequence::const_iterator
                it = config_->PathUriList()->Uri().begin();
              it != config_->PathUriList()->Uri().end(); ++it)
          {
            if(!it->path().empty())
            {
              derived_config_.advertiser_service_uri.push_back(
                xsd::AdServer::Configuration::UriType(it->path()));
            }
          }
        }
        grpc_executor_ = std::make_shared<AdServer::Grpc::GrpcExecutor>(
          common_config_->grpc_executor_threads());
        add_child_object(grpc_executor_);

        auto user_bind_objects =
          AdServer::UserInfoSvcs::create_distributed_user_bind_client(
            *common_config_,
            grpc_executor_,
            logger());
        if(user_bind_objects.client)
        {
          user_bind_client_ = user_bind_objects.client;
          add_child_object(user_bind_objects.active_object);
        }

        auto campaign_manager = std::make_shared<
          AdServer::CampaignSvcs::CampaignManagerDistributedGrpcClient>(
            FrontendCommons::read_campaign_manager_grpc_refs(*common_config_),
            AdServer::Grpc::BatchingOptions(),
            grpc_executor_);
        campaign_manager_ = campaign_manager;
        add_child_object(campaign_manager);

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

        request_info_filler_.reset(
          new RequestInfoFiller(
            logger(),
            common_module_.in(),
            common_config_->GeoIP().present() ?
              common_config_->GeoIP()->path().c_str() : 0,
            Commons::LogReferrer::read_log_referrer_settings(
            config_->use_referrer_action_stats()),
            config_->set_uid()));

        track_pixel_ = FileCachePtr(
          new FileCache(config_->track_pixel_path().c_str()));

        track_html_ = FileCachePtr(
          new FileCache(config_->track_html_path().c_str()));

        cookie_manager_.reset(
          new FrontendCommons::CookieManager<
            FCGI::HttpRequest, FCGI::HttpResponse>(
              common_config_->Cookies()));

        if(common_config_->StatsDumper().present())
        {
          CORBACommons::CorbaObjectRef dumper_ref;

          Config::CorbaConfigReader::read_corba_ref(
            common_config_->StatsDumper().get().StatsDumperRef(),
            dumper_ref);

          stats_ = new AcFrontendStat(
            logger(),
            dumper_ref,
            0,
            Generics::Time(common_config_->StatsDumper().get().period()),
            callback());

          add_child_object(stats_.in());
        }

        task_runner_ = new Generics::TaskRunner(
          callback(), config_->threads(), 0, config_->match_task_limit());
        add_child_object(task_runner_.in());

        derived_config_.use_referrer = Commons::LogReferrer::read_log_referrer_settings(
          config_->use_referrer_action_stats());

        // init redirect rules
        for(auto redirect_it = config_->Redirect().begin();
          redirect_it != config_->Redirect().end(); ++redirect_it)
        {
          RedirectRule_var redirect_rule = new RedirectRule();
          redirect_rule->url_template = new Commons::TextTemplate(
            redirect_it->template_());
          redirect_rule->use_keywords = redirect_it->use_keywords();

          if(redirect_it->use_keywords())
          {
            String::StringManip::Splitter<String::AsciiStringManip::SepNL> splitter(
              redirect_it->keywords());
            String::SubString token;
            while(splitter.get_token(token))
            {
              String::StringManip::trim(token);
              if(!token.empty())
              {
                redirect_rule->keywords.insert(Generics::StringHashAdapter(token));
              }
            }
          }

          redirect_rules_.push_back(redirect_rule);
        }

        activate_object();
      }
      catch (const eh::Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << e.what();
        throw Exception(ostr);
      }

      logger()->log(String::SubString(
          "Frontend::init(): frontend is running ..."),
        Logging::Logger::INFO,
        Aspect::ACTION_FRONTEND);
    }
  }

  void
  Frontend::shutdown() noexcept
  {
    try
    {
      task_runner_->wait_for_queue_exhausting();
      deactivate_object();
      wait_object();

      Stream::Error ostr;
      ostr << "Frontend::shutdown(): frontend terminated (pid = " <<
        ::getpid() << ").";

      logger()->log(ostr.str(),
        Logging::Logger::INFO,
        Aspect::ACTION_FRONTEND);
    }
    catch(...)
    {}
  }

  void
  Frontend::write_pixel(FCGI::HttpResponse& response) /*throw(eh::Exception)*/
  {
    response.set_content_type_nocopy(String::SubString("image/gif"));

    FileCache::BufferHolder_var buffer = track_pixel_->get();
    response.get_output_stream().write((*buffer)->data(), (*buffer)->size());
  }

  void
  Frontend::write_html(FCGI::HttpResponse& response) /*throw(eh::Exception)*/
  {
    response.set_content_type_nocopy(String::SubString("text/html"));

    FileCache::BufferHolder_var buffer = track_html_->get();
    response.get_output_stream().write((*buffer)->data(), (*buffer)->size());
  }

  void
  Frontend::process_advertiser_request_(
    const std::shared_ptr<AdvertiserRequestState>& state)
    noexcept
  {
    state->cookie_resolved_user_id = state->request_info.user_id;

    if(state->request_info.user_status != AdServer::CampaignSvcs::US_OPTOUT &&
      (!state->request_info.external_user_id.empty() ||
        !state->cookie_resolved_user_id.is_null()))
    {
      const std::string external_id_str =
        !state->request_info.external_user_id.empty() ?
          state->request_info.external_user_id :
          std::string("c/") + state->cookie_resolved_user_id.to_string();

      resolve_user_id_(
        external_id_str,
        state->cookie_resolved_user_id,
        state->request_info.time,
        [this, state](bool resolve_res, Commons::UserId resolved_user_id)
        {
          if(resolve_res && !resolved_user_id.is_null())
          {
            state->cookie_resolved_user_id = resolved_user_id;
          }

          resolve_utm_user_id_(state);
        });
    }
    else
    {
      resolve_utm_user_id_(state);
    }
  }

  void
  Frontend::resolve_utm_user_id_(
    const std::shared_ptr<AdvertiserRequestState>& state)
    noexcept
  {
    if(state->request_info.user_status != AdServer::CampaignSvcs::US_OPTOUT &&
      !state->request_info.utm_cookie_user_id.is_null())
    {
      const std::string external_id_str =
        std::string("c/") + state->request_info.utm_cookie_user_id.to_string();

      resolve_user_id_(
        external_id_str,
        state->cookie_resolved_user_id,
        state->request_info.time,
        [this, state](bool resolve_res, Commons::UserId resolved_user_id)
        {
          if(resolve_res)
          {
            state->utm_cookie_resolved_user_id =
              !resolved_user_id.is_null() ?
                resolved_user_id :
                state->request_info.utm_cookie_user_id;
          }

          finish_advertiser_request_(state);
        });
    }
    else
    {
      finish_advertiser_request_(state);
    }
  }

  void
  Frontend::finish_advertiser_request_(
    const std::shared_ptr<AdvertiserRequestState>& state)
    noexcept
  {
    static const char* FUN = "Action::Frontend::finish_advertiser_request_()";

    int http_status = 500;
    try
    {
      http_status = fill_advertiser_response_(
        *state->response,
        state->request_holder->request(),
        state->request_info,
        state->return_html,
        state->cookie_resolved_user_id,
        state->utm_cookie_resolved_user_id);
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": eh::Exception caught: " << e.what();

      logger()->log(
        ostr.str(),
        Logging::Logger::EMERGENCY,
        Aspect::ACTION_FRONTEND,
        "ADS-IMPL-128");
    }

    state->response_writer->write(http_status, state->response);
  }

  int
  Frontend::fill_advertiser_response_(
    FCGI::HttpResponse& response,
    const FCGI::HttpRequest& request,
    const RequestInfo& request_info,
    bool return_html,
    const Commons::UserId& cookie_resolved_user_id,
    const Commons::UserId& utm_cookie_resolved_user_id)
    /*throw(Exception, eh::Exception)*/
  {
    static const char* FUN = "Frontend::fill_advertiser_response_()";
    //AdServer::CampaignSvcs::UserStatus result_user_status = request_info.user_status;

    // verify actions for all uids:
    //   request_info.user_id
    //   request_info.utm_resolved_user_id
    //   utm_cookie_resolved_user_id
    //
    action_taken_all_(
      request_info,
      utm_cookie_resolved_user_id);

    // enqueue match channels
    trigger_match_all_(
      request_info,
      utm_cookie_resolved_user_id);

    // choose user id, that will be used for set into cookie and relink other ids
    Commons::UserId result_user_id;

    if(!request_info.utm_resolved_user_id.is_null())
    {
      result_user_id = request_info.utm_resolved_user_id;
    }
    else if(!cookie_resolved_user_id.is_null())
    {
      result_user_id = cookie_resolved_user_id;
    }
    else if(!request_info.user_id.is_null())
    {
      result_user_id = request_info.user_id;
    }
    else if(!utm_cookie_resolved_user_id.is_null())
    {
      result_user_id = utm_cookie_resolved_user_id;
    }

    // relink user ids block
    // TO CHECK !!!
    // process utm user id (have priority over other identifiers)
    // result_user_id is user id from cookies or result of resolving c/<cookie user id>
    // add mapping c/<result_user_id> => <utm user id>
    //
    // result user id = <utm user id> (set into cookie), that garantee that
    // all actions will be linked to user for that done bid request
    //
    if(request_info.user_status != AdServer::CampaignSvcs::US_OPTOUT &&
      !result_user_id.is_null())
    {
      relink_user_id_all_(
        request_info,
        utm_cookie_resolved_user_id,
        result_user_id);
    }

    // fill response
    FrontendCommons::CORS::set_headers(request, response);

    if(request_info.user_status != AdServer::CampaignSvcs::US_OPTOUT && (
         !result_user_id.is_null() || config_->set_uid()))
    {
      const Generics::SignedUuid signed_uid =
        common_module_->user_id_controller()->sign(
          !result_user_id.is_null() ?
          result_user_id : Generics::Uuid::create_random_based());

      FrontendCommons::add_UID_cookie(
        response,
        request,
        *cookie_manager_,
        signed_uid.str());
    }

    if(common_config_->ResponseHeaders().present())
    {
      FrontendCommons::add_headers(
        *(common_config_->ResponseHeaders()),
        response);
    }

    // do redirect by rules
    bool redirected = false;
    int http_status = 200;

    if(request_info.redirect)
    {
      // find redirect rules that match traits
      RedirectRuleArray redirect_rules;
      RedirectRuleArray keyword_redirect_rules;

      {
        std::vector<std::string> keywords;
        FrontendCommons::get_ip_keywords(keywords, request_info.peer_ip);

        for(auto redirect_rule_it = redirect_rules_.begin();
          redirect_rule_it != redirect_rules_.end(); ++redirect_rule_it)
        {
          if((*redirect_rule_it)->use_keywords)
          {
            for(auto keyword_it = keywords.begin(); keyword_it != keywords.end(); ++keyword_it)
            {
              if((*redirect_rule_it)->keywords.find(*keyword_it) != (*redirect_rule_it)->keywords.end())
              {
                keyword_redirect_rules.push_back(*redirect_rule_it);
                break;
              }
            }
          }
          else if(keyword_redirect_rules.empty())
          {
            redirect_rules.push_back(*redirect_rule_it);
          }
        }
      }

      RedirectRule_var result_redirect_rule;

      if(!keyword_redirect_rules.empty())
      {
        result_redirect_rule = keyword_redirect_rules[
          Generics::safe_rand(keyword_redirect_rules.size())];
      }
      else if(!redirect_rules.empty())
      {
        result_redirect_rule = redirect_rules[
          Generics::safe_rand(redirect_rules.size())];
      }

      if(result_redirect_rule.in())
      {
        try
        {
          String::TextTemplate::Args templ_args;

          templ_args[TemplateParams::RANDOM] =
            String::StringManip::IntToStr(
              Generics::safe_rand()).str().str();

          // instantiate redirect template
          String::TextTemplate::DefaultValue args_with_default(&templ_args);
          String::TextTemplate::ArgsEncoder args_with_encoding(
            &args_with_default);

          std::string redirect =
            result_redirect_rule->url_template->instantiate(args_with_encoding);

          if(!redirect.empty())
          {
            const std::string str = FrontendCommons::normalize_abs_url(
              HTTP::BrowserAddress(redirect),
              HTTP::HTTPAddress::VW_FULL,
              FrontendCommons::is_secure_request(request) || request_info.secure ?
                HTTPS_PREFIX : HTTP_PREFIX);

            http_status = FrontendCommons::redirect(
              redirect,
              response);

            redirected = true;
          }
        }
        catch(const eh::Exception& ex)
        {
          logger()->sstream(
            Logging::Logger::EMERGENCY,
            Aspect::ACTION_FRONTEND,
            "ADS-IMPL-?") <<
            FUN << ": eh::Exception has been caught: " << ex.what();
        }
      } // result_redirect_rule.in()
    }

    if(!redirected)
    {
      if(return_html)
      {
        write_html(response);
      }
      else
      {
        write_pixel(response);
      }

      http_status = 200;
    }

    return http_status;
  }

  void
  Frontend::fill_match_request_info_(
    adserver::campaign_svcs::campaign_manager::MatchRequestInfo& mri,
    const AdServer::Commons::UserId& user_id,
    const Generics::Time& now,
    const adserver::channel_svcs::channel_server::MatchResponse&
      trigger_match_result) const
    noexcept
  {
    /*
      Don't fill:
        mri.match_info.coord_location
    */

    auto* match_info = mri.mutable_match_info();
    match_info->set_colo_id(common_config_->colo_id());
    mri.set_user_id(GrpcAlgs::pack_user_id(user_id));
    mri.set_request_time(GrpcAlgs::pack_time(now));
    const auto& page_channels =
      trigger_match_result.matched_channels().page_channels();
    const int result_len = page_channels.size();
    for(int i = 0; i < result_len; ++i)
    {
      auto* pkw_channel = match_info->add_pkw_channels();
      pkw_channel->set_channel_id(page_channels[i].id());
      pkw_channel->set_channel_trigger_id(page_channels[i].trigger_channel_id());
    }
  }

  void
  Frontend::trigger_match_(
    unsigned long conv_id,
    const Generics::Time& now,
    const AdServer::Commons::UserId& user_id,
    const String::SubString& referer)
    noexcept
  {
    try
    {
      //CORBA::String_var hostname;
      auto channel_request = std::make_shared<
        adserver::channel_svcs::channel_server::MatchRequest>();
      channel_request->set_non_strict_word_match(false);
      channel_request->set_non_strict_url_match(false);
      channel_request->set_return_negative(false);
      channel_request->set_simplify_page(false);
      channel_request->set_fill_content(false);
      channel_request->set_statuses("A", 2);
      std::ostringstream keywords_ostr;
      keywords_ostr << "poadcp";
      if(conv_id)
      {
        keywords_ostr << ", poadcp" << conv_id;
      }

      channel_request->set_pwords(keywords_ostr.str());
      channel_request->set_first_url(referer.str());

      channel_client_->match(
        *channel_request,
        [this, channel_request, user_id, now](
          const grpc::Status& status,
          const adserver::channel_svcs::channel_server::MatchResponse&
            trigger_match_result)
        {
          if(!status.ok())
          {
            Stream::Error ostr;
            ostr << "ChannelServer::match(): gRPC call failed: code=" <<
              static_cast<int>(status.error_code()) <<
              ", message=" << status.error_message();
            logger()->log(
              ostr.str(),
              Logging::Logger::EMERGENCY,
              Aspect::ACTION_FRONTEND,
              "ADS-IMPL-117");
            return;
          }

          const auto& matched_channels = trigger_match_result.matched_channels();
          if(matched_channels.page_channels_size() == 0 &&
            matched_channels.url_channels_size() == 0 &&
            matched_channels.url_keyword_channels_size() == 0)
          {
            return;
          }

          // call UIM only if any channel matched
          auto history_match_request = std::make_shared<
            adserver::user_info_svcs::user_info_manager::MatchRequest>();
          auto* match_params = history_match_request->mutable_match_params();
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

          const auto fill_channel_matches =
            [](
              auto* out,
              const auto& in)
          {
            for(const auto& channel_match : in)
            {
              auto* result = out->Add();
              result->set_channel_id(channel_match.channel_id);
              result->set_channel_trigger_id(
                channel_match.channel_trigger_id);
            }
          };

          typedef std::set<ChannelMatch> ChannelMatchSet;
          ChannelMatchSet page_channels;

          std::transform(
            matched_channels.page_channels().begin(),
            matched_channels.page_channels().end(),
            std::inserter(page_channels, page_channels.end()),
            GetChannelTriggerId());

          fill_channel_matches(
            match_params->mutable_page_channel_ids(),
            page_channels);

          ChannelMatchSet url_channels;

          std::transform(
            matched_channels.url_channels().begin(),
            matched_channels.url_channels().end(),
            std::inserter(url_channels, url_channels.end()),
            GetChannelTriggerId());

          fill_channel_matches(
            match_params->mutable_url_channel_ids(),
            url_channels);

          ChannelMatchSet url_keyword_channels;

          std::transform(
            matched_channels.url_keyword_channels().begin(),
            matched_channels.url_keyword_channels().end(),
            std::inserter(url_keyword_channels, url_keyword_channels.end()),
            GetChannelTriggerId());

          fill_channel_matches(
            match_params->mutable_url_keyword_channel_ids(),
            url_keyword_channels);

          auto* user_info = history_match_request->mutable_user_info();
          user_info->set_user_id(GrpcAlgs::pack_user_id(user_id));
          user_info->set_last_colo_id(-1);
          user_info->set_request_colo_id(common_config_->colo_id());
          user_info->set_current_colo_id(-1);
          user_info->set_temporary(false);
          user_info->set_time(now.tv_sec);

          user_info_client_->match(
            *history_match_request,
            [this, history_match_request](
              const grpc::Status& status,
              const adserver::user_info_svcs::user_info_manager::MatchResponse&)
            {
              if(!status.ok())
              {
                Stream::Error ostr;
                ostr << "UserInfoManager::match(): gRPC call failed: code=" <<
                  static_cast<int>(status.error_code()) <<
                  ", message=" << status.error_message();

                const unsigned long log_level =
                  status.error_code() == grpc::StatusCode::UNAVAILABLE ?
                    static_cast<unsigned long>(TraceLevel::MIDDLE) :
                    static_cast<unsigned long>(Logging::Logger::EMERGENCY);
                logger()->log(
                  ostr.str(),
                  log_level,
                  Aspect::ACTION_FRONTEND,
                  status.error_code() == grpc::StatusCode::UNAVAILABLE ?
                    "" : "ADS-IMPL-112");
              }
            });

          auto process_match_request = std::make_shared<
            adserver::campaign_svcs::campaign_manager::
              ProcessMatchRequestRequest>();
          fill_match_request_info_(
            *process_match_request->mutable_match_request_info(),
            user_id,
            now,
            trigger_match_result);

          campaign_manager_->process_match_request(
            *process_match_request,
            [this, process_match_request](
              const grpc::Status& status,
              const adserver::campaign_svcs::campaign_manager::
                ProcessMatchRequestResponse&)
            {
              if(!status.ok())
              {
                Stream::Error ostr;
                ostr << "CampaignManager::process_match_request(): "
                  "gRPC call failed: code=" <<
                  static_cast<int>(status.error_code()) <<
                  ", message=" << status.error_message();
                logger()->log(
                  ostr.str(),
                  Logging::Logger::EMERGENCY,
                  Aspect::ACTION_FRONTEND,
                  "ADS-ICON-4");
              }
            });
        });
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << __func__ <<
        ":  ChannelServerGrpcAsyncClient error: " << ex.what();
      logger()->log(ostr.str(),
        Logging::Logger::EMERGENCY,
        Aspect::ACTION_FRONTEND,
        "ADS-IMPL-117");
    }
  }

  void
  Frontend::handle_request_(
    FCGI::HttpRequestHolder_var request_holder,
    FCGI::BaseHttpResponseWriter_var response_writer)
    noexcept
  {
    static const char* FUN = "Action::Frontend::handle_request()";

    const FCGI::HttpRequest& request = request_holder->request();

    FCGI::HttpResponse_var response_ptr(new FCGI::HttpResponse());

    logger()->log(String::SubString(
        "Action::Frontend::handle_request(): entered"),
      TraceLevel::MIDDLE,
      Aspect::ACTION_FRONTEND);
    int http_status = 500;
    bool return_html = false;
    bool response_deferred = false;

    try
    {
      std::string found_uri;

      bool params_in_path = FrontendCommons::find_uri(
        derived_config_.advertiser_service_uri,
        request.uri(),
        found_uri,
        0,
        false);

      return_html =
        config_->UriList().present() &&
        FrontendCommons::find_uri(
          config_->UriList()->Uri(), request.uri(), found_uri);

      ActionFrontendHTTPConstrain::apply(request);

      if(logger()->log_level() >= TraceLevel::MIDDLE)
      {
        std::ostringstream ostr;
        ostr << FUN << ":" << std::endl;
        FrontendCommons::print_request(ostr, request);

        logger()->log(ostr.str(),
          TraceLevel::MIDDLE,
          Aspect::ACTION_FRONTEND);
      }

      RequestInfo request_info;

      request_info_filler_->fill(
        request_info,
        request,
        params_in_path ?
          String::SubString(request.uri().begin(), request.uri().begin() + found_uri.length()) :
          String::SubString());

      auto state = std::make_shared<AdvertiserRequestState>();
      state->request_holder = request_holder;
      state->response_writer = response_writer;
      state->response = response_ptr;
      state->request_info = request_info;
      state->return_html = return_html;
      response_deferred = true;
      process_advertiser_request_(state);
    }
    catch (const ForbiddenException& ex)
    {
      http_status = 403;
      logger()->sstream(TraceLevel::LOW, Aspect::ACTION_FRONTEND) <<
        FUN << ": ForbiddenException caught: " << ex.what();
    }
    catch (const InvalidParamException& ex)
    {
      http_status = 400;
      logger()->sstream(TraceLevel::MIDDLE, Aspect::ACTION_FRONTEND) <<
        FUN << ": InvalidParamException caught: " << ex.what();
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": eh::Exception caught: " << e.what();

      logger()->log(ostr.str(),
        Logging::Logger::EMERGENCY,
        Aspect::ACTION_FRONTEND,
        "ADS-IMPL-128");
    }

    if(!response_deferred)
    {
      response_writer->write(http_status, response_ptr);
    }
  }

  void
  Frontend::relink_user_id_all_(
    const RequestInfo& request_info,
    const Commons::UserId& utm_cookie_resolved_user_id,
    const Commons::UserId& link_user_id)
    noexcept
  {
    static const char* FUN = "Frontend::relink_user_id_all_()";

    const Commons::UserId* relink_user_ids[] = {
      &request_info.user_id,
      &request_info.utm_resolved_user_id,
      &utm_cookie_resolved_user_id
    };

    std::set<Commons::UserId> processed_user_ids;

    for(unsigned long user_i = 0;
      user_i < sizeof(relink_user_ids) / sizeof(relink_user_ids[0]);
      ++user_i)
    {
      const Commons::UserId& relink_user_id = *relink_user_ids[user_i];

      if(!relink_user_id.is_null() &&
        relink_user_id != AdServer::Commons::PROBE_USER_ID &&
        processed_user_ids.find(relink_user_id) == processed_user_ids.end() &&
        relink_user_id != link_user_id)
      {
        processed_user_ids.insert(relink_user_id);

        if(user_bind_client_)
        {
          try
          {

            auto add_user_request_info = std::make_shared<
              adserver::user_info_svcs::user_bind::AddUserIdRequest>();
            const std::string external_id_str =
              std::string("c/") + relink_user_id.to_string();
            add_user_request_info->set_id(external_id_str);
            add_user_request_info->set_user_id(
              GrpcAlgs::pack_user_id(link_user_id));
            add_user_request_info->set_timestamp(
              GrpcAlgs::pack_time(request_info.time));

            user_bind_client_->add_user_id(
              *add_user_request_info,
              [this, add_user_request_info](
                const grpc::Status& status,
                const adserver::user_info_svcs::user_bind::AddUserIdResponse&)
              {
                if(!status.ok())
                {
                  Stream::Error ostr;
                  ostr << FUN << ": UserBindServer::add_user_id(): "
                    "gRPC call failed: code=" <<
                    static_cast<int>(status.error_code()) <<
                    ", message=" << status.error_message();
                  logger()->log(
                    ostr.str(),
                    Logging::Logger::ERROR,
                    Aspect::ACTION_FRONTEND,
                    "ADS-IMPL-109");
                }
              });
          }
          catch(const AdServer::UserInfoSvcs::UserBindClient::NotReady&)
          {
            Stream::Error ostr;
            ostr << FUN << ": caught UserBindServer::NotReady";
            logger()->log(ostr.str(),
              Logging::Logger::EMERGENCY,
              Aspect::ACTION_FRONTEND,
              "ADS-IMPL-109");
          }
          catch(const AdServer::UserInfoSvcs::UserBindClient::ChunkNotFound& )
          {
            Stream::Error ostr;
            ostr << FUN << ": caught UserBindClient::ChunkNotFound";
            logger()->log(ostr.str(),
              Logging::Logger::ERROR,
              Aspect::ACTION_FRONTEND,
              "ADS-IMPL-109");
          }
          catch(const AdServer::UserInfoSvcs::UserBindClient::ImplementationException& ex)
          {
            Stream::Error ostr;
            ostr << FUN << ": caught UserBindClient::ImplementationException: " <<
              ex.what();
            logger()->log(ostr.str(),
              Logging::Logger::ERROR,
              Aspect::ACTION_FRONTEND,
              "ADS-IMPL-109");
          }
          catch(const CORBA::SystemException& e)
          {
            Stream::Error ostr;
            ostr << FUN << ": caught CORBA::SystemException: " << e;
            logger()->log(ostr.str(),
              Logging::Logger::ERROR,
              Aspect::ACTION_FRONTEND,
              "ADS-ICON-6");
          }
          catch(...)
          {
            assert(0);
          }
        }
      }
    }

    // link ifa
    if(user_bind_client_ && !request_info.ifa.empty())
    {
      try
      {

        auto add_user_request_info = std::make_shared<
          adserver::user_info_svcs::user_bind::AddUserIdRequest>();
        const std::string external_id_str = std::string("ifa/") + request_info.ifa;
        add_user_request_info->set_id(external_id_str);
        add_user_request_info->set_user_id(
          GrpcAlgs::pack_user_id(link_user_id));
        add_user_request_info->set_timestamp(
          GrpcAlgs::pack_time(request_info.time));

        user_bind_client_->add_user_id(
          *add_user_request_info,
          [this, add_user_request_info](
            const grpc::Status& status,
            const adserver::user_info_svcs::user_bind::AddUserIdResponse&)
          {
            if(!status.ok())
            {
              Stream::Error ostr;
              ostr << FUN << ": UserBindServer::add_user_id(): "
                "gRPC call failed: code=" <<
                static_cast<int>(status.error_code()) <<
                ", message=" << status.error_message();
              logger()->log(
                ostr.str(),
                Logging::Logger::ERROR,
                Aspect::ACTION_FRONTEND,
                "ADS-IMPL-109");
            }
          });
      }
      catch(const AdServer::UserInfoSvcs::UserBindClient::NotReady&)
      {
        Stream::Error ostr;
        ostr << FUN << ": caught UserBindServer::NotReady";
        logger()->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          Aspect::ACTION_FRONTEND,
          "ADS-IMPL-109");
      }
      catch(const AdServer::UserInfoSvcs::UserBindClient::ChunkNotFound& )
      {
        Stream::Error ostr;
        ostr << FUN << ": caught UserBindClient::ChunkNotFound";
        logger()->log(ostr.str(),
          Logging::Logger::ERROR,
          Aspect::ACTION_FRONTEND,
          "ADS-IMPL-109");
      }
      catch(const AdServer::UserInfoSvcs::UserBindClient::ImplementationException& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": caught UserBindClient::ImplementationException: " <<
          ex.what();
        logger()->log(ostr.str(),
          Logging::Logger::ERROR,
          Aspect::ACTION_FRONTEND,
          "ADS-IMPL-109");
      }
      catch(const CORBA::SystemException& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": caught CORBA::SystemException: " << e;
        logger()->log(ostr.str(),
          Logging::Logger::ERROR,
          Aspect::ACTION_FRONTEND,
          "ADS-ICON-6");
      }
      catch(...)
      {
        assert(0);
      }
    }
  }

  void
  Frontend::trigger_match_all_(
    const RequestInfo& request_info,
    const Commons::UserId& utm_cookie_resolved_user_id)
    noexcept
  {
    const Commons::UserId* match_user_ids[] = {
      &request_info.user_id,
      &request_info.utm_resolved_user_id,
      &utm_cookie_resolved_user_id
    };

    std::set<Commons::UserId> processed_user_ids;

    for(unsigned long user_i = 0;
      user_i < sizeof(match_user_ids) / sizeof(match_user_ids[0]);
      ++user_i)
    {
      const Commons::UserId& match_user_id = *match_user_ids[user_i];

      if(!match_user_id.is_null() &&
        match_user_id != AdServer::Commons::PROBE_USER_ID &&
        processed_user_ids.find(match_user_id) == processed_user_ids.end())
      {
        try
        {
          // delay match click channels
          task_runner_->enqueue_task(new MatchActionChannelsTask(
            this,
            request_info.time,
            match_user_id,
            (request_info.action_id.present() ? *request_info.action_id : 0),
              request_info.referer));
        }
        catch (const Generics::TaskRunner::Overflow&)
        {}

        processed_user_ids.insert(match_user_id);
      }
    }
  }

  void
  Frontend::action_taken_all_(
    const RequestInfo& request_info,
    const Commons::UserId& utm_cookie_resolved_user_id)
    noexcept
  {
    static const char* FUN = "Frontend::action_taken_all_()";

    try
    {
      adserver::campaign_svcs::campaign_manager::ActionInfo
        verify_action_info;

      // verify_action_info.user_id, verify_action_info.user_status must be initialized in loop

      verify_action_info.set_time(GrpcAlgs::pack_time(request_info.time));
      verify_action_info.set_test_request(request_info.test_request);
      verify_action_info.set_log_as_test(request_info.log_as_test);

      {
        auto* location = verify_action_info.add_location();
        location->set_country(request_info.location.country_code.str());
        location->set_region(request_info.location.region.str());
        location->set_city(request_info.location.city.str());
      }

      verify_action_info.set_referer(request_info.referer);
      if(request_info.value.present())
      {
        verify_action_info.mutable_action_value()->set_value(
          GrpcAlgs::pack_decimal(*request_info.value));
      }

      verify_action_info.set_order_id(request_info.order_id);

      if(request_info.campaign_id.present())
      {
        auto* campaign_id = verify_action_info.mutable_campaign_id();
        campaign_id->set_defined(true);
        campaign_id->set_value(*request_info.campaign_id);
      }
      else
      {
        verify_action_info.mutable_campaign_id()->set_defined(false);
      }

      if(request_info.action_id.present())
      {
        auto* action_id = verify_action_info.mutable_action_id();
        action_id->set_defined(true);
        action_id->set_value(*request_info.action_id);
      }
      else
      {
        verify_action_info.mutable_action_id()->set_defined(false);
      }

      if(common_config_->ip_logging_enabled())
      {
        std::string ip_hash;
        FrontendCommons::ip_hash(ip_hash, request_info.peer_ip, common_config_->ip_salt());
        verify_action_info.set_ip_hash(ip_hash);
        verify_action_info.set_peer_ip(request_info.peer_ip);
      }

      for(const auto platform_id : request_info.platform_ids)
      {
        verify_action_info.add_platform_ids(platform_id);
      }

      const Commons::UserId* verify_user_ids[] = {
        &request_info.user_id,
        &request_info.utm_resolved_user_id,
        &utm_cookie_resolved_user_id
      };

      std::set<Commons::UserId> processed_user_ids;

      for(unsigned long user_i = 0;
          user_i < sizeof(verify_user_ids) / sizeof(verify_user_ids[0]);
          ++user_i)
      {
        const Commons::UserId& verify_user_id = *verify_user_ids[user_i];

        if(!verify_user_id.is_null() &&
           verify_user_id != AdServer::Commons::PROBE_USER_ID &&
           processed_user_ids.find(verify_user_id) == processed_user_ids.end())
        {
          verify_action_info.set_user_id(GrpcAlgs::pack_user_id(verify_user_id));
          verify_action_info.set_user_status(AdServer::CampaignSvcs::US_OPTIN);

          auto action_taken_request = std::make_shared<
            adserver::campaign_svcs::campaign_manager::ActionTakenRequest>();
          *action_taken_request->mutable_action_info() = verify_action_info;
          campaign_manager_->action_taken(
            *action_taken_request,
            [this, action_taken_request](
              const grpc::Status& status,
              const adserver::campaign_svcs::campaign_manager::
                ActionTakenResponse&)
            {
              if(!status.ok())
              {
                Stream::Error ostr;
                ostr << "CampaignManager::action_taken(): "
                  "gRPC call failed: code=" <<
                  static_cast<int>(status.error_code()) <<
                  ", message=" << status.error_message();
                logger()->log(
                  ostr.str(),
                  Logging::Logger::EMERGENCY,
                  Aspect::ACTION_FRONTEND,
                  "ADS-ICON-4");
                return;
              }

              if(stats_)
              {
                const auto& action_info =
                  action_taken_request->action_info();
                stats_->consider_request(
                  action_info.test_request(),
                  action_info.user_status());
              }
            });

          processed_user_ids.insert(verify_user_id);
        }
      }

      if(processed_user_ids.empty())
      {
        // verify action without user id
        verify_action_info.set_user_id(GrpcAlgs::pack_user_id(Commons::UserId()));
        verify_action_info.set_user_status(
          request_info.user_status != AdServer::CampaignSvcs::US_OPTOUT ?
          AdServer::CampaignSvcs::US_UNDEFINED :
          AdServer::CampaignSvcs::US_OPTOUT);

        auto action_taken_request = std::make_shared<
          adserver::campaign_svcs::campaign_manager::ActionTakenRequest>();
        *action_taken_request->mutable_action_info() = verify_action_info;
        campaign_manager_->action_taken(
          *action_taken_request,
          [this, action_taken_request](
            const grpc::Status& status,
            const adserver::campaign_svcs::campaign_manager::
              ActionTakenResponse&)
          {
            if(!status.ok())
            {
              Stream::Error ostr;
              ostr << "CampaignManager::action_taken(): "
                "gRPC call failed: code=" <<
                static_cast<int>(status.error_code()) <<
                ", message=" << status.error_message();
              logger()->log(
                ostr.str(),
                Logging::Logger::EMERGENCY,
                Aspect::ACTION_FRONTEND,
                "ADS-ICON-4");
              return;
            }

            if(stats_)
            {
              const auto& action_info = action_taken_request->action_info();
              stats_->consider_request(
                action_info.test_request(),
                action_info.user_status());
            }
          });
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't process request: " << ex.what();
      logger()->log(ostr.str(),
        Logging::Logger::EMERGENCY,
        Aspect::ACTION_FRONTEND,
        "ADS-ICON-4");
    }
  }

  void
  Frontend::resolve_user_id_(
    const String::SubString& external_id_str,
    const Commons::UserId& current_user_id,
    const Generics::Time& time,
    std::function<void(bool, Commons::UserId)> callback)
    noexcept
  {
    static const char* FUN = "Action::Frontend::resolve_user_id_()";
    auto callback_holder = std::make_shared<
      std::function<void(bool, Commons::UserId)>>(std::move(callback));

    // don't add resolving for generated external ids
    if(user_bind_client_)
    {
      try
      {
        // get user id by external id
        auto get_request_info = std::make_shared<
          adserver::user_info_svcs::user_bind::GetUserIdRequest>();
        get_request_info->set_id(external_id_str.str());
        get_request_info->set_timestamp(GrpcAlgs::pack_time(time));
        get_request_info->set_silent(true);
        get_request_info->set_generate_user_id(false);
        get_request_info->set_for_set_cookie(false);
        get_request_info->set_create_timestamp(
          GrpcAlgs::pack_time(Generics::Time::ZERO));
        get_request_info->set_current_user_id(
          GrpcAlgs::pack_user_id(current_user_id));

        user_bind_client_->get_user_id(
          *get_request_info,
          [
            this,
            get_request_info,
            callback_holder
          ](
            const grpc::Status& status,
            const adserver::user_info_svcs::user_bind::GetUserIdResponse&
              response) mutable
          {
            if(!status.ok())
            {
              Stream::Error ostr;
              ostr << FUN << ": UserBindServer::get_user_id(): "
                "gRPC call failed: code=" <<
                static_cast<int>(status.error_code()) <<
                ", message=" << status.error_message();
              logger()->log(
                ostr.str(),
                Logging::Logger::ERROR,
                Aspect::ACTION_FRONTEND,
                "ADS-IMPL-109");
              (*callback_holder)(false, Commons::UserId());
              return;
            }

            (*callback_holder)(
              true,
              GrpcAlgs::unpack_user_id(response.user_id()));
          });
        return;
      }
      catch(const AdServer::UserInfoSvcs::UserBindClient::NotReady&)
      {
        Stream::Error ostr;
        ostr << FUN << ": caught UserBindServer::NotReady";
        logger()->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          Aspect::ACTION_FRONTEND,
          "ADS-IMPL-109");
      }
      catch(const AdServer::UserInfoSvcs::UserBindClient::ChunkNotFound& )
      {
        Stream::Error ostr;
        ostr << FUN << ": caught UserBindClient::ChunkNotFound";
          logger()->log(ostr.str(),
          Logging::Logger::ERROR,
          Aspect::ACTION_FRONTEND,
          "ADS-IMPL-109");
      }
      catch(const AdServer::UserInfoSvcs::UserBindClient::ImplementationException& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": caught UserBindClient::ImplementationException: " <<
          ex.what();
        logger()->log(ostr.str(),
          Logging::Logger::ERROR,
          Aspect::ACTION_FRONTEND,
          "ADS-IMPL-109");
      }
      catch(const CORBA::SystemException& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": caught CORBA::SystemException: " << e;
        logger()->log(ostr.str(),
          Logging::Logger::ERROR,
          Aspect::ACTION_FRONTEND,
          "ADS-ICON-6");
      }
      catch(...)
      {
        assert(0);
      }
    }

    (*callback_holder)(false, Commons::UserId());
  }
}
