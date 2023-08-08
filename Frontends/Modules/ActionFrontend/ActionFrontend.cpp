
#include <sstream>

#include <HTTP/HTTPCookie.hpp>
#include <HTTP/UrlAddress.hpp>
#include <Logger/StreamLogger.hpp>
#include <Logger/DistributorLogger.hpp>

#include <Commons/ErrorHandler.hpp>
#include <Commons/CorbaConfig.hpp>
#include <Commons/CorbaAlgs.hpp>
#include <Commons/UserInfoManip.hpp>
#include <Commons/ExternalUserIdUtils.hpp>
#include <Commons/GrpcAlgs.hpp>
#include <Commons/UserverConfigUtils.hpp>

#include <Frontends/FrontendCommons/add_UID_cookie.hpp>

#include "ActionFrontendStat.hpp"
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
      const AdServer::ChannelSvcs::ChannelServerBase::ChannelAtom& atom)
      noexcept
    {
      return ChannelMatch(atom.id, atom.trigger_channel_id);
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

namespace AdServer
{
namespace Action
{
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
      AdServer::ChannelSvcs::ChannelServerBase::MatchResult_var trigger_match_result;

      action_frontend_->trigger_match_(
        trigger_match_result,
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
      common_module_(ReferenceCounting::add_ref(common_module)),
      campaign_managers_(this->logger(), Aspect::ACTION_FRONTEND)
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

        // Coroutine
        auto task_processor_container_builder =
          Config::create_task_processor_container_builder(
            logger(),
            common_config_->Coroutine());
        auto init_func = [] (
          TaskProcessorContainer& task_processor_container) {
            return std::make_unique<ComponentsBuilder>();
        };

        manager_coro_ = ManagerCoro_var(
          new ManagerCoro(
            std::move(task_processor_container_builder),
            std::move(init_func),
            logger()));

        add_child_object(manager_coro_);

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

        corba_client_adapter_ = new CORBACommons::CorbaClientAdapter();

        const auto& config_grpc_client = common_config_->GrpcClientPool();
        const auto config_grpc_data = Config::create_pool_client_config(
          config_grpc_client);

        if(!common_config_->UserBindControllerGroup().empty())
        {
          user_bind_client_ = new FrontendCommons::UserBindClient(
            common_config_->UserBindControllerGroup(),
            corba_client_adapter_.in(),
            logger(),
            manager_coro_.in(),
            config_grpc_data.first,
            config_grpc_data.second,
            config_grpc_client.enable());
          add_child_object(user_bind_client_);
        }

        campaign_managers_.resolve(
          *common_config_, corba_client_adapter_);

        user_info_client_ = new FrontendCommons::UserInfoClient(
          common_config_->UserInfoManagerControllerGroup(),
          corba_client_adapter_.in(),
          logger(),
          manager_coro_.in(),
          config_grpc_data.first,
          config_grpc_data.second,
          config_grpc_client.enable());
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

          add_child_object(stats_);
        }

        task_runner_ = new Generics::TaskRunner(
          callback(), config_->threads(), 0, config_->match_task_limit());
        add_child_object(task_runner_);

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
      corba_client_adapter_.reset();

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
    response.set_content_type(String::SubString("image/gif"));

    FileCache::BufferHolder_var buffer = track_pixel_->get();
    response.get_output_stream().write((*buffer)->data(), (*buffer)->size());
  }

  void
  Frontend::write_html(FCGI::HttpResponse& response) /*throw(eh::Exception)*/
  {
    response.set_content_type(String::SubString("text/html"));

    FileCache::BufferHolder_var buffer = track_html_->get();
    response.get_output_stream().write((*buffer)->data(), (*buffer)->size());
  }

  int
  Frontend::process_advertiser_request(
    FCGI::HttpResponse& response,
    const FCGI::HttpRequest& request,
    const RequestInfo& request_info,
    bool return_html)
    /*throw(Exception, eh::Exception)*/
  {
    static const char* FUN = "Frontend::process_advertiser_request()";

    AdServer::Commons::UserId cookie_resolved_user_id = request_info.user_id;
    //AdServer::CampaignSvcs::UserStatus result_user_status = request_info.user_status;

    if(request_info.user_status != AdServer::CampaignSvcs::US_OPTOUT && (
         !request_info.external_user_id.empty() ||
         !cookie_resolved_user_id.is_null()))
      // primary use external_id if it defined
    {
      std::string external_id_str;

      if(!request_info.external_user_id.empty())
      {
        external_id_str = request_info.external_user_id;
      }
      else
      {
        external_id_str = std::string("c/") + cookie_resolved_user_id.to_string();
      }

      Commons::UserId resolved_user_id;

      bool resolve_res = resolve_user_id_(
        resolved_user_id,
        external_id_str,
        cookie_resolved_user_id,
        request_info.time);

      if(resolve_res)
      {
        if(!resolved_user_id.is_null())
        {
          //result_user_status = AdServer::CampaignSvcs::US_OPTIN;
          cookie_resolved_user_id = resolved_user_id;
        }
      }
    }

    Commons::UserId utm_cookie_resolved_user_id;

    if(request_info.user_status != AdServer::CampaignSvcs::US_OPTOUT &&
        !request_info.utm_cookie_user_id.is_null())
      // primary use external_id if it defined
    {
      std::string external_id_str = std::string("c/") + request_info.utm_cookie_user_id.to_string();

      Commons::UserId resolved_user_id;

      bool resolve_res = resolve_user_id_(
        resolved_user_id,
        external_id_str,
        cookie_resolved_user_id,
        request_info.time);

      if(resolve_res)
      {
        if(!resolved_user_id.is_null())
        {
          utm_cookie_resolved_user_id = resolved_user_id;
        }
        else
        {
          utm_cookie_resolved_user_id = request_info.utm_cookie_user_id;
        }
      }
    }

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
    AdServer::CampaignSvcs::CampaignManager::MatchRequestInfo& mri,
    const AdServer::Commons::UserId& user_id,
    const Generics::Time& now,
    const AdServer::ChannelSvcs::ChannelServerBase::MatchResult_var& trigger_match_result) const
    noexcept
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

  void
  Frontend::trigger_match_(
    AdServer::ChannelSvcs::ChannelServerBase::MatchResult_var& trigger_match_result,
    unsigned long conv_id,
    const Generics::Time& now,
    const AdServer::Commons::UserId& user_id,
    const String::SubString& referer)
    noexcept
  {
    try
    {
      //CORBA::String_var hostname;
      AdServer::ChannelSvcs::ChannelServerBase::MatchQuery query;
      query.non_strict_word_match = false;
      query.non_strict_url_match = false;
      query.return_negative = false;
      query.simplify_page = false;
      query.fill_content = false;
      query.statuses[0] = 'A';
      query.statuses[1] = '\0';
      std::ostringstream keywords_ostr;
      keywords_ostr << "poadcp";
      if(conv_id)
      {
        keywords_ostr << ", poadcp" << conv_id;
      }

      query.pwords << keywords_ostr.str();
      query.first_url << referer;

      channel_servers_->match(query, trigger_match_result);

      if(trigger_match_result.ptr() != 0 && (
        trigger_match_result->matched_channels.page_channels.length() != 0 ||
        trigger_match_result->matched_channels.url_channels.length() != 0 ||
        trigger_match_result->matched_channels.url_keyword_channels.length() != 0))
      {
        AdServer::UserInfoSvcs::UserInfoMatcher_var
          uim_session = user_info_client_->user_info_session();

        try
        {
          // call UIM only if any channel matched
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

          ChannelMatchSet url_channels;

          std::transform(
            trigger_match_result->matched_channels.url_channels.get_buffer(),
            trigger_match_result->matched_channels.url_channels.get_buffer() +
            trigger_match_result->matched_channels.url_channels.length(),
            std::inserter(url_channels, url_channels.end()),
            GetChannelTriggerId());

          match_params.url_channel_ids.length(url_channels.size());
          res_ch_i = 0;
          for (ChannelMatchSet::const_iterator ch_it = url_channels.begin();
               ch_it != url_channels.end(); ++ch_it, ++res_ch_i)
          {
            match_params.url_channel_ids[res_ch_i].channel_id = ch_it->channel_id;
            match_params.url_channel_ids[res_ch_i].channel_trigger_id =
              ch_it->channel_trigger_id;
          }

          ChannelMatchSet url_keyword_channels;

          std::transform(
            trigger_match_result->matched_channels.url_keyword_channels.get_buffer(),
            trigger_match_result->matched_channels.url_keyword_channels.get_buffer() +
            trigger_match_result->matched_channels.url_keyword_channels.length(),
            std::inserter(url_keyword_channels, url_keyword_channels.end()),
            GetChannelTriggerId());

          match_params.url_keyword_channel_ids.length(url_keyword_channels.size());
          res_ch_i = 0;
          for (ChannelMatchSet::const_iterator ch_it = url_keyword_channels.begin();
               ch_it != url_keyword_channels.end(); ++ch_it, ++res_ch_i)
          {
            match_params.url_keyword_channel_ids[res_ch_i].channel_id = ch_it->channel_id;
            match_params.url_keyword_channel_ids[res_ch_i].channel_trigger_id =
              ch_it->channel_trigger_id;
          }

          AdServer::UserInfoSvcs::UserInfo user_info;
          user_info.user_id = CorbaAlgs::pack_user_id(user_id);
          user_info.last_colo_id = -1;
          user_info.request_colo_id = common_config_->colo_id();
          user_info.current_colo_id = -1;
          user_info.temporary = false;
          user_info.time = now.tv_sec;

          AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult_var match_result;
          uim_session->match(
            user_info,
            match_params,
            match_result.out());
        }
        catch(const UserInfoSvcs::UserInfoMatcher::ImplementationException& e)
        {
          Stream::Error ostr;
          ostr << __func__ <<
            ": UserInfoSvcs::UserInfoMatcher::ImplementationException caught: " <<
            e.description;

          logger()->log(ostr.str(),
            Logging::Logger::EMERGENCY,
            Aspect::ACTION_FRONTEND,
            "ADS-IMPL-112");
        }
        catch(const UserInfoSvcs::UserInfoMatcher::NotReady& e)
        {
          logger()->log(
            String::SubString("UserInfoManager not ready for matching."),
            TraceLevel::MIDDLE,
            Aspect::ACTION_FRONTEND);
        }
        catch(const CORBA::SystemException& ex)
        {
          Stream::Error ostr;
          ostr << __func__ <<
            ": Can't match history channels: CORBA::SystemException: " << ex;

          logger()->log(ostr.str(),
            Logging::Logger::EMERGENCY,
            Aspect::ACTION_FRONTEND,
            "ADS-ICON-2");
        }

        try
        {
          AdServer::CampaignSvcs::CampaignManager::MatchRequestInfo request_info;
          fill_match_request_info_(
            request_info,
            user_id,
            now,
            trigger_match_result);

          campaign_managers_.process_match_request(request_info);
        }
        catch(const eh::Exception& ex)
        {
          Stream::Error ostr;
          ostr << __func__ << ": Can't process match request. "
            "Possible problem with Campaignmanager: " <<
            ex.what();
          logger()->log(ostr.str(),
            Logging::Logger::EMERGENCY,
            Aspect::ACTION_FRONTEND,
            "ADS-ICON-4");
        }

      }
    }
    catch(const FrontendCommons::ChannelServerSessionPool::Exception& ex)
    {
      Stream::Error ostr;
      ostr << __func__ <<
        ":  ChannelServerSessionPool::Exception: " << ex.what();
      logger()->log(ostr.str(),
        Logging::Logger::EMERGENCY,
        Aspect::ACTION_FRONTEND,
        "ADS-IMPL-117");
    }
  }

  void
  Frontend::handle_request_(
    FCGI::HttpRequestHolder_var request_holder,
    FCGI::HttpResponseWriter_var response_writer)
    noexcept
  {
    static const char* FUN = "Action::Frontend::handle_request()";

    const FCGI::HttpRequest& request = request_holder->request();

    FCGI::HttpResponse_var response_ptr(new FCGI::HttpResponse());
    FCGI::HttpResponse& response = *response_ptr;

    logger()->log(String::SubString(
        "Action::Frontend::handle_request(): entered"),
      TraceLevel::MIDDLE,
      Aspect::ACTION_FRONTEND);
    int http_status = 500;
    bool return_html = false;

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

      http_status = process_advertiser_request(response, request, request_info, return_html);
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

    response_writer->write(http_status, response_ptr);
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
            AdServer::UserInfoSvcs::UserBindMapper_var user_bind_mapper =
              user_bind_client_->user_bind_mapper();

            AdServer::UserInfoSvcs::UserBindMapper::AddUserRequestInfo
              add_user_request_info;
            const std::string external_id_str =
              std::string("c/") + relink_user_id.to_string();
            add_user_request_info.id << external_id_str;
            add_user_request_info.user_id = CorbaAlgs::pack_user_id(link_user_id);
            add_user_request_info.timestamp = CorbaAlgs::pack_time(request_info.time);

            AdServer::UserInfoSvcs::UserBindServer::AddUserResponseInfo_var
              prev_user_bind_info =
                user_bind_mapper->add_user_id(add_user_request_info);

            (void)prev_user_bind_info;
          }
          catch(const AdServer::UserInfoSvcs::UserBindMapper::NotReady&)
          {
            Stream::Error ostr;
            ostr << FUN << ": caught UserBindServer::NotReady";
            logger()->log(ostr.str(),
              Logging::Logger::EMERGENCY,
              Aspect::ACTION_FRONTEND,
              "ADS-IMPL-109");
          }
          catch(const AdServer::UserInfoSvcs::UserBindMapper::ChunkNotFound& )
          {
            Stream::Error ostr;
            ostr << FUN << ": caught UserBindMapper::ChunkNotFound";
            logger()->log(ostr.str(),
              Logging::Logger::ERROR,
              Aspect::ACTION_FRONTEND,
              "ADS-IMPL-109");
          }
          catch(const AdServer::UserInfoSvcs::UserBindMapper::ImplementationException& ex)
          {
            Stream::Error ostr;
            ostr << FUN << ": caught UserBindMapper::ImplementationException: " <<
              ex.description;
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
        AdServer::UserInfoSvcs::UserBindMapper_var user_bind_mapper =
          user_bind_client_->user_bind_mapper();

        AdServer::UserInfoSvcs::UserBindMapper::AddUserRequestInfo
          add_user_request_info;
        const std::string external_id_str = std::string("ifa/") + request_info.ifa;
        add_user_request_info.id << external_id_str;
        add_user_request_info.user_id = CorbaAlgs::pack_user_id(link_user_id);
        add_user_request_info.timestamp = CorbaAlgs::pack_time(request_info.time);

        AdServer::UserInfoSvcs::UserBindServer::AddUserResponseInfo_var
          prev_user_bind_info =
            user_bind_mapper->add_user_id(add_user_request_info);

        (void)prev_user_bind_info;
      }
      catch(const AdServer::UserInfoSvcs::UserBindMapper::NotReady&)
      {
        Stream::Error ostr;
        ostr << FUN << ": caught UserBindServer::NotReady";
        logger()->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          Aspect::ACTION_FRONTEND,
          "ADS-IMPL-109");
      }
      catch(const AdServer::UserInfoSvcs::UserBindMapper::ChunkNotFound& )
      {
        Stream::Error ostr;
        ostr << FUN << ": caught UserBindMapper::ChunkNotFound";
        logger()->log(ostr.str(),
          Logging::Logger::ERROR,
          Aspect::ACTION_FRONTEND,
          "ADS-IMPL-109");
      }
      catch(const AdServer::UserInfoSvcs::UserBindMapper::ImplementationException& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": caught UserBindMapper::ImplementationException: " <<
          ex.description;
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
      AdServer::CampaignSvcs::CampaignManager::ActionInfo verify_action_info;

      // verify_action_info.user_id, verify_action_info.user_status must be initialized in loop

      verify_action_info.time = CorbaAlgs::pack_time(request_info.time);
      verify_action_info.test_request = request_info.test_request;
      verify_action_info.log_as_test = request_info.log_as_test;

      {
        verify_action_info.location.length(1);
        verify_action_info.location[0].country << request_info.location.country_code;
        verify_action_info.location[0].region << request_info.location.region;
        verify_action_info.location[0].city << request_info.location.city;
      }

      verify_action_info.referer << request_info.referer;
      verify_action_info.action_value_defined = request_info.value.present();
      if(request_info.value.present())
      {
        verify_action_info.action_value = CorbaAlgs::pack_decimal(*request_info.value);
      }

      verify_action_info.order_id << request_info.order_id;
    
      if(request_info.campaign_id.present())
      {
        verify_action_info.campaign_id_defined = true;
        verify_action_info.campaign_id = *request_info.campaign_id;
      }
      else
      {
        verify_action_info.campaign_id_defined = false;
      }

      if(request_info.action_id.present())
      {
        verify_action_info.action_id_defined = true;
        verify_action_info.action_id = *request_info.action_id;
      }
      else
      {
        verify_action_info.action_id_defined = false;
      }

      if(common_config_->ip_logging_enabled())
      {
        std::string ip_hash;
        FrontendCommons::ip_hash(ip_hash, request_info.peer_ip, common_config_->ip_salt());
        verify_action_info.ip_hash << ip_hash;
        verify_action_info.peer_ip << request_info.peer_ip;
      }

      CorbaAlgs::fill_sequence(
        request_info.platform_ids.begin(),
        request_info.platform_ids.end(),
        verify_action_info.platform_ids);

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
          verify_action_info.user_id = CorbaAlgs::pack_user_id(verify_user_id);
          verify_action_info.user_status = AdServer::CampaignSvcs::US_OPTIN;

          campaign_managers_.action_taken(verify_action_info);

          if(stats_)
          {
            stats_->consider_request(verify_action_info);
          }

          processed_user_ids.insert(verify_user_id);
        }
      }

      if(processed_user_ids.empty())
      {
        // verify action without user id
        verify_action_info.user_id = CorbaAlgs::pack_user_id(Commons::UserId());
        verify_action_info.user_status =
          request_info.user_status != AdServer::CampaignSvcs::US_OPTOUT ?
          AdServer::CampaignSvcs::US_UNDEFINED :
          AdServer::CampaignSvcs::US_OPTOUT;

        campaign_managers_.action_taken(verify_action_info);

        if(stats_) // ?
        {
          stats_->consider_request(verify_action_info);
        }
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

  bool
  Frontend::resolve_user_id_(
    Commons::UserId& result_user_id,
    const String::SubString& external_id_str,
    const Commons::UserId& current_user_id,
    const Generics::Time& time)
    noexcept
  {
    static const char* FUN = "Action::Frontend::resolve_user_id_()";

    // don't add resolving for generated external ids
    if(user_bind_client_)
    {
      try
      {
        AdServer::UserInfoSvcs::UserBindMapper_var user_bind_mapper =
          user_bind_client_->user_bind_mapper();

        // get user id by external id
        AdServer::UserInfoSvcs::UserBindMapper::GetUserRequestInfo get_request_info;
        get_request_info.id << external_id_str;
        get_request_info.timestamp = CorbaAlgs::pack_time(time);
        get_request_info.silent = true;
        get_request_info.generate_user_id = false;
          get_request_info.for_set_cookie = false;
        get_request_info.create_timestamp = CorbaAlgs::pack_time(Generics::Time::ZERO);
        get_request_info.current_user_id = CorbaAlgs::pack_user_id(current_user_id);

        AdServer::UserInfoSvcs::UserBindServer::GetUserResponseInfo_var
          prev_user_bind_info =
            user_bind_mapper->get_user_id(get_request_info);

        const AdServer::Commons::UserId resolved_user_id =
          CorbaAlgs::unpack_user_id(prev_user_bind_info->user_id);

        if(!resolved_user_id.is_null())
        {
          result_user_id = resolved_user_id;
        }

        return true;
      }
      catch(const AdServer::UserInfoSvcs::UserBindMapper::NotReady&)
      {
        Stream::Error ostr;
        ostr << FUN << ": caught UserBindServer::NotReady";
        logger()->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          Aspect::ACTION_FRONTEND,
          "ADS-IMPL-109");
      }
      catch(const AdServer::UserInfoSvcs::UserBindMapper::ChunkNotFound& )
      {
        Stream::Error ostr;
        ostr << FUN << ": caught UserBindMapper::ChunkNotFound";
          logger()->log(ostr.str(),
          Logging::Logger::ERROR,
          Aspect::ACTION_FRONTEND,
          "ADS-IMPL-109");
      }
      catch(const AdServer::UserInfoSvcs::UserBindMapper::ImplementationException& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": caught UserBindMapper::ImplementationException: " <<
          ex.description;
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

    return false;
  }
}
}
