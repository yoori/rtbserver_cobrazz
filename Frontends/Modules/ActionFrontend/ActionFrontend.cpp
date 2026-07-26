
#include <sstream>
#include <memory>
#include <string_view>
#include <utility>

#include <HTTP/HTTPCookie.hpp>
#include <HTTP/UrlAddress.hpp>
#include <Logger/StreamLogger.hpp>
#include <Logger/DistributorLogger.hpp>

#include <Commons/ErrorHandler.hpp>
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

namespace Request::Cookie
{
    const String::AsciiStringManip::Caseless OPTOUT("OPTED_OUT");
    const String::AsciiStringManip::Caseless OPTOUT_TRUE_VALUE("YES");
    const String::AsciiStringManip::Caseless USER_ID("uid");
  }

namespace Request::Param
  {
    const char CAMPAIGN_ID[] = "cid";
    const char CONVERSION_ID_OLD[] = "actionid";
    const char CONVERSION_ID[] = "convid";
    const char COUNTRY[] = "country";
    const char TEST_REQUEST[] = "testrequest";
    const char DEBUG_CURRENT_TIME[] = "debug-time";
  }

namespace Request::Header
  {
    const String::AsciiStringManip::Caseless REM_HOST(".RemoteHost");
    const String::AsciiStringManip::Caseless REFERER("Referer");
  }

namespace AdServer::Action
{
  Frontend::Frontend(
    Configuration* frontend_config,
    Logging::Logger* logger,
    std::shared_ptr<AdServer::Commons::ExecutorPool> request_workers,
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
      frontend_config_(ReferenceCounting::add_ref(frontend_config)),
      common_module_(ReferenceCounting::add_ref(common_module)),
      match_task_count_(0),
      workers_(std::move(request_workers))
  {}

  void
  Frontend::parse_config_() /*throw(Exception)*/
  {
    static const char* FUN = "Frontend::parse_config_()";

    try
    {
      typedef Configuration::FeConfig Config;
      const Config& fe_config = frontend_config_->get();

      if (!fe_config.CommonFeConfiguration().present())
      {
        throw Exception("CommonFeConfiguration not presented.");
      }

      common_config_ = CommonConfigPtr(
        new CommonFeConfiguration(*fe_config.CommonFeConfiguration()));

      if (!fe_config.ActionFeConfiguration().present())
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

    if (logger()->log_level() >= TraceLevel::MIDDLE)
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

    if (true) // module_used())
    {
      try
      {
        parse_config_();

        if (config_->PathUriList().present())
        {
          for(xsd::AdServer::Configuration::UriListType::Uri_sequence::const_iterator
                it = config_->PathUriList()->Uri().begin();
              it != config_->PathUriList()->Uri().end(); ++it)
          {
            if (!it->path().empty())
            {
              derived_config_.advertiser_service_uri.push_back(
                xsd::AdServer::Configuration::UriType(it->path()));
            }
          }
        }
        grpc_executor_ = common_module_->grpc_executor();

        auto user_bind_client =
          AdServer::UserInfoSvcs::create_distributed_user_bind_client(
            *common_config_,
            grpc_executor_,
            common_module_->grpc_coalesce_runner(),
            logger());
        if (user_bind_client)
        {
          user_bind_client_coro_ = std::make_shared<
            AdServer::UserInfoSvcs::UserBindServerGrpcCoroClient>(
              user_bind_client,
              workers_);
          add_child_object(user_bind_client);
        }

        auto campaign_manager = std::make_shared<
          AdServer::CampaignSvcs::CampaignManagerDistributedGrpcClient>(
            FrontendCommons::read_campaign_manager_grpc_refs(*common_config_),
            FrontendCommons::read_campaign_manager_grpc_batching_options(
              *common_config_),
            grpc_executor_,
            common_module_->grpc_coalesce_runner());
        campaign_manager_coro_ = std::make_shared<
          AdServer::CampaignSvcs::CampaignManagerGrpcCoroClient>(
            campaign_manager,
            workers_);
        add_child_object(campaign_manager);

        auto user_info_client =
          AdServer::UserInfoSvcs::create_distributed_user_info_client(
            *common_config_,
            grpc_executor_,
            common_module_->grpc_coalesce_runner(),
            logger());
        user_info_client_coro_ = std::make_shared<
          AdServer::UserInfoSvcs::UserInfoManagerGrpcCoroClient>(
            user_info_client,
            workers_);
        add_child_object(user_info_client);

        auto channel_client =
          AdServer::ChannelSvcs::create_distributed_channel_client(
            *common_config_,
            grpc_executor_,
            common_module_->grpc_coalesce_runner(),
            logger());
        channel_client_coro_ = std::make_shared<
          AdServer::ChannelSvcs::ChannelServerGrpcCoroClient>(
            channel_client,
            workers_);
        add_child_object(channel_client);

        request_info_filler_.reset(
          new RequestInfoFiller(
            logger(),
            common_module_.in(),
            common_module_->ip_mapper(),
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

        stats_ = new AcFrontendStat();

        match_workers_ = workers_;

        derived_config_.use_referrer = Commons::LogReferrer::read_log_referrer_settings(
          config_->use_referrer_action_stats());

        // init redirect rules
        for(auto redirect_it = config_->Redirect().begin();
          redirect_it != config_->Redirect().end(); ++redirect_it)
        {
          RedirectRule_var redirect_rule = new RedirectRule();
          const auto& template_text = redirect_it->template_();
          redirect_rule->url_template =
            std::make_shared<Commons::TextTemplate>(
              std::string_view(template_text.c_str(), template_text.size()));
          redirect_rule->use_keywords = redirect_it->use_keywords();

          if (redirect_it->use_keywords())
          {
            String::StringManip::Splitter<String::AsciiStringManip::SepNL> splitter(
              redirect_it->keywords());
            String::SubString token;
            while(splitter.get_token(token))
            {
              String::StringManip::trim(token);
              if (!token.empty())
              {
                redirect_rule->keywords.insert(Generics::StringHashAdapter(token));
              }
            }
          }

          redirect_rules_.push_back(redirect_rule);
        }
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

  FrontendCommons::RequestTask
  Frontend::process_advertiser_request_(
    FCGI::HttpRequestHolder_var request_holder,
    FCGI::HttpResponse_var response,
    RequestInfo request_info,
    bool return_html)
    noexcept
  {
    static const char* FUN = "Action::Frontend::process_advertiser_request_()";

    int http_status = 500;
    try
    {
      http_status = fill_advertiser_response_(
        *response,
        request_holder->request(),
        request_info,
        return_html,
        request_info.user_id,
        request_info.utm_cookie_user_id);
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

    co_return FrontendCommons::RequestResult{http_status, response, false};
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

    if (!request_info.utm_resolved_user_id.is_null())
    {
      result_user_id = request_info.utm_resolved_user_id;
    }
    else if (!cookie_resolved_user_id.is_null())
    {
      result_user_id = cookie_resolved_user_id;
    }
    else if (!request_info.user_id.is_null())
    {
      result_user_id = request_info.user_id;
    }
    else if (!utm_cookie_resolved_user_id.is_null())
    {
      result_user_id = utm_cookie_resolved_user_id;
    }

    // relink user ids block
    // process utm user id (have priority over other identifiers)
    // result_user_id is user id from cookies
    //
    // result user id = <utm user id> (set into cookie), that garantee that
    // all actions will be linked to user for that done bid request
    //
    if (request_info.user_status != AdServer::CampaignSvcs::US_OPTOUT &&
      !result_user_id.is_null())
    {
      relink_user_id_all_(
        request_info,
        utm_cookie_resolved_user_id,
        result_user_id);
    }

    // fill response
    FrontendCommons::CORS::set_headers(request, response);

    if (request_info.user_status != AdServer::CampaignSvcs::US_OPTOUT && (
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

    if (common_config_->ResponseHeaders().present())
    {
      FrontendCommons::add_headers(
        *(common_config_->ResponseHeaders()),
        response);
    }

    // do redirect by rules
    bool redirected = false;
    int http_status = 200;

    if (request_info.redirect)
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
          if ((*redirect_rule_it)->use_keywords)
          {
            for(auto keyword_it = keywords.begin(); keyword_it != keywords.end(); ++keyword_it)
            {
              if ((*redirect_rule_it)->keywords.find(*keyword_it) != (*redirect_rule_it)->keywords.end())
              {
                keyword_redirect_rules.push_back(*redirect_rule_it);
                break;
              }
            }
          }
          else if (keyword_redirect_rules.empty())
          {
            redirect_rules.push_back(*redirect_rule_it);
          }
        }
      }

      RedirectRule_var result_redirect_rule;

      if (!keyword_redirect_rules.empty())
      {
        result_redirect_rule = keyword_redirect_rules[
          Generics::safe_rand(keyword_redirect_rules.size())];
      }
      else if (!redirect_rules.empty())
      {
        result_redirect_rule = redirect_rules[
          Generics::safe_rand(redirect_rules.size())];
      }

      if (result_redirect_rule.in())
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

          if (!redirect.empty())
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

    if (!redirected)
    {
      if (return_html)
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
    co_trigger_match_(
      conv_id,
      now,
      user_id,
      referer.str()).start_detached(nullptr);
  }

  FrontendCommons::RequestTask
  Frontend::co_trigger_match_(
    unsigned long conv_id,
    Generics::Time now,
    AdServer::Commons::UserId user_id,
    std::string referer)
    noexcept
  {
    try
    {
      adserver::channel_svcs::channel_server::MatchRequest channel_request;
      channel_request.set_non_strict_word_match(false);
      channel_request.set_non_strict_url_match(false);
      channel_request.set_return_negative(false);
      channel_request.set_simplify_page(false);
      channel_request.set_fill_content(false);
      channel_request.set_statuses("A", 2);
      std::ostringstream keywords_ostr;
      keywords_ostr << "poadcp";
      if (conv_id)
      {
        keywords_ostr << ", poadcp" << conv_id;
      }

      channel_request.set_pwords(keywords_ostr.str());
      channel_request.set_first_url(referer);

      auto channel_result = co_await channel_client_coro_->co_match(
        std::move(channel_request));
      if (!channel_result.status.ok())
      {
        Stream::Error ostr;
        ostr << "ChannelServer::match(): gRPC call failed: code=" <<
          static_cast<int>(channel_result.status.error_code()) <<
          ", message=" << channel_result.status.error_message();
        logger()->log(
          ostr.str(),
          Logging::Logger::EMERGENCY,
          Aspect::ACTION_FRONTEND,
          "ADS-IMPL-117");
        co_return FrontendCommons::RequestResult{};
      }

      const auto& trigger_match_result = channel_result.response;
      const auto& matched_channels = trigger_match_result.matched_channels();
      if (matched_channels.page_channels_size() == 0 &&
        matched_channels.url_channels_size() == 0 &&
        matched_channels.url_keyword_channels_size() == 0)
      {
        co_return FrontendCommons::RequestResult{};
      }

      adserver::user_info_svcs::user_info_manager::MatchRequest
        history_match_request;
      auto* match_params = history_match_request.mutable_match_params();
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

      auto* user_info = history_match_request.mutable_user_info();
      user_info->set_user_id(GrpcAlgs::pack_user_id(user_id));
      user_info->set_last_colo_id(-1);
      user_info->set_request_colo_id(common_config_->colo_id());
      user_info->set_current_colo_id(-1);
      user_info->set_temporary(false);
      user_info->set_time(now.tv_sec);

      auto user_info_result = co_await user_info_client_coro_->co_match(
        std::move(history_match_request));
      if (!user_info_result.status.ok())
      {
        Stream::Error ostr;
        ostr << "UserInfoManager::match(): gRPC call failed: code=" <<
          static_cast<int>(user_info_result.status.error_code()) <<
          ", message=" << user_info_result.status.error_message();

        const unsigned long log_level =
          user_info_result.status.error_code() == grpc::StatusCode::UNAVAILABLE ?
            static_cast<unsigned long>(TraceLevel::MIDDLE) :
            static_cast<unsigned long>(Logging::Logger::EMERGENCY);
        logger()->log(
          ostr.str(),
          log_level,
          Aspect::ACTION_FRONTEND,
          user_info_result.status.error_code() == grpc::StatusCode::UNAVAILABLE ?
            "" : "ADS-IMPL-112");
      }

      adserver::campaign_svcs::campaign_manager::ProcessMatchRequestRequest
        process_match_request;
      fill_match_request_info_(
        *process_match_request.mutable_match_request_info(),
        user_id,
        now,
        trigger_match_result);

      auto process_result =
        co_await campaign_manager_coro_->co_process_match_request(
          std::move(process_match_request));
      if (!process_result.status.ok())
      {
        Stream::Error ostr;
        ostr << "CampaignManager::process_match_request(): "
          "gRPC call failed: code=" <<
          static_cast<int>(process_result.status.error_code()) <<
          ", message=" << process_result.status.error_message();
        logger()->log(
          ostr.str(),
          Logging::Logger::EMERGENCY,
          Aspect::ACTION_FRONTEND,
          "ADS-ICON-4");
      }
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

    co_return FrontendCommons::RequestResult{};
  }

  FrontendCommons::RequestTask
  Frontend::co_handle_request(
    FCGI::HttpRequestHolder_var request_holder)
    noexcept
  {
    static const char* FUN = "Action::Frontend::handle_request()";

    co_await AdServer::Commons::ExecutorPool::yield(workers_);

    const FCGI::HttpRequest& request = request_holder->request();

    FCGI::HttpResponse_var response_ptr(new FCGI::HttpResponse());

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

      if (logger()->log_level() >= TraceLevel::MIDDLE)
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

      auto result = co_await process_advertiser_request_(
        std::move(request_holder),
        std::move(response_ptr),
        std::move(request_info),
        return_html);
      co_return std::move(result);
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

    co_return FrontendCommons::RequestResult{
      http_status,
      response_ptr,
      false};
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

      if (!relink_user_id.is_null() &&
        relink_user_id != AdServer::Commons::PROBE_USER_ID &&
        processed_user_ids.find(relink_user_id) == processed_user_ids.end() &&
        relink_user_id != link_user_id)
      {
        processed_user_ids.insert(relink_user_id);
      }
    }

    // link ifa
    if (user_bind_client_coro_ && !request_info.ifa.empty())
    {
      (void)FUN;
      co_add_user_id_(
        std::string("ifa/") + request_info.ifa,
        link_user_id,
        request_info.time).start_detached(nullptr);
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

      if (!match_user_id.is_null() &&
        match_user_id != AdServer::Commons::PROBE_USER_ID &&
        processed_user_ids.find(match_user_id) == processed_user_ids.end())
      {
        const unsigned long current_task_count =
          match_task_count_.exchange_and_add(1) + 1;

        if (config_->match_task_limit() == 0 ||
          current_task_count <= config_->match_task_limit() + config_->threads())
        {
          match_workers_->post(
            [this,
              now = request_info.time,
              user_id = match_user_id,
              conv_id = request_info.action_id.present() ?
                *request_info.action_id : 0,
              referer = request_info.referer]()
            {
              trigger_match_(conv_id, now, user_id, referer);
              match_task_count_ += -1;
            });
        }
        else
        {
          match_task_count_ += -1;
        }

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
      if (request_info.value.present())
      {
        verify_action_info.mutable_action_value()->set_value(
          GrpcAlgs::pack_decimal(*request_info.value));
      }

      verify_action_info.set_order_id(request_info.order_id);

      if (request_info.campaign_id.present())
      {
        auto* campaign_id = verify_action_info.mutable_campaign_id();
        campaign_id->set_defined(true);
        campaign_id->set_value(*request_info.campaign_id);
      }
      else
      {
        verify_action_info.mutable_campaign_id()->set_defined(false);
      }

      if (request_info.action_id.present())
      {
        auto* action_id = verify_action_info.mutable_action_id();
        action_id->set_defined(true);
        action_id->set_value(*request_info.action_id);
      }
      else
      {
        verify_action_info.mutable_action_id()->set_defined(false);
      }

      if (common_config_->ip_logging_enabled())
      {
        std::string ip_hash;
        FrontendCommons::ip_hash(ip_hash, request_info.peer_ip, common_config_->ip_salt());
        verify_action_info.set_ip_hash(ip_hash);
        verify_action_info.set_peer_ip(request_info.peer_ip);
      }

      verify_action_info.mutable_platform_ids()->Add(
        request_info.platform_ids.begin(),
        request_info.platform_ids.end());

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

        if (!verify_user_id.is_null() &&
           verify_user_id != AdServer::Commons::PROBE_USER_ID &&
           processed_user_ids.find(verify_user_id) == processed_user_ids.end())
        {
          verify_action_info.set_user_id(GrpcAlgs::pack_user_id(verify_user_id));
          verify_action_info.set_user_status(AdServer::CampaignSvcs::US_OPTIN);

          adserver::campaign_svcs::campaign_manager::ActionTakenRequest
            action_taken_request;
          *action_taken_request.mutable_action_info() = verify_action_info;
          co_action_taken_(std::move(action_taken_request)).
            start_detached(nullptr);

          processed_user_ids.insert(verify_user_id);
        }
      }

      if (processed_user_ids.empty())
      {
        // verify action without user id
        verify_action_info.set_user_id(GrpcAlgs::pack_user_id(Commons::UserId()));
        verify_action_info.set_user_status(
          request_info.user_status != AdServer::CampaignSvcs::US_OPTOUT ?
          AdServer::CampaignSvcs::US_UNDEFINED :
          AdServer::CampaignSvcs::US_OPTOUT);

        adserver::campaign_svcs::campaign_manager::ActionTakenRequest
          action_taken_request;
        *action_taken_request.mutable_action_info() = verify_action_info;
        co_action_taken_(std::move(action_taken_request)).
          start_detached(nullptr);
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

  Frontend::ResolveUserIdTask
  Frontend::co_resolve_user_id_(
    std::string external_id_str,
    Commons::UserId current_user_id,
    Generics::Time time)
    noexcept
  {
    static const char* FUN = "Action::Frontend::co_resolve_user_id_()";

    // don't add resolving for generated external ids
    if (user_bind_client_coro_)
    {
      try
      {
        // get user id by external id
        adserver::user_info_svcs::user_bind::GetUserIdRequest
          get_request_info;
        get_request_info.set_id(std::move(external_id_str));
        get_request_info.set_timestamp(GrpcAlgs::pack_time(time));
        get_request_info.set_silent(true);
        get_request_info.set_generate_user_id(false);
        get_request_info.set_for_set_cookie(false);
        get_request_info.set_create_timestamp(
          GrpcAlgs::pack_time(Generics::Time::ZERO));
        get_request_info.set_current_user_id(
          GrpcAlgs::pack_user_id(current_user_id));

        auto get_result = co_await user_bind_client_coro_->co_get_user_id(
          std::move(get_request_info));
        if (!get_result.status.ok())
        {
          Stream::Error ostr;
          ostr << FUN << ": UserBindServer::get_user_id(): "
            "gRPC call failed: code=" <<
            static_cast<int>(get_result.status.error_code()) <<
            ", message=" << get_result.status.error_message();
          logger()->log(
            ostr.str(),
            Logging::Logger::ERROR,
            Aspect::ACTION_FRONTEND,
            "ADS-IMPL-109");
          co_return ResolveUserIdResult{};
        }

        co_return ResolveUserIdResult{
          true,
          GrpcAlgs::unpack_user_id(get_result.response.user_id())};
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": UserBindServer::get_user_id() scheduling failed: " <<
          ex.what();
        logger()->log(ostr.str(),
          Logging::Logger::ERROR,
          Aspect::ACTION_FRONTEND,
          "ADS-IMPL-109");
      }
      catch(...)
      {
        assert(0);
      }
    }

    co_return ResolveUserIdResult{};
  }

  FrontendCommons::RequestTask
  Frontend::co_add_user_id_(
    std::string external_user_id,
    Commons::UserId user_id,
    Generics::Time time)
    noexcept
  {
    static const char* FUN = "Action::Frontend::co_add_user_id_()";

    try
    {
      adserver::user_info_svcs::user_bind::AddUserIdRequest add_request;
      add_request.set_id(std::move(external_user_id));
      add_request.set_user_id(GrpcAlgs::pack_user_id(user_id));
      add_request.set_timestamp(GrpcAlgs::pack_time(time));

      auto add_result = co_await user_bind_client_coro_->co_add_user_id(
        std::move(add_request));
      if (!add_result.status.ok())
      {
        Stream::Error ostr;
        ostr << FUN << ": UserBindServer::add_user_id(): "
          "gRPC call failed: code=" <<
          static_cast<int>(add_result.status.error_code()) <<
          ", message=" << add_result.status.error_message();
        logger()->log(
          ostr.str(),
          Logging::Logger::ERROR,
          Aspect::ACTION_FRONTEND,
          "ADS-IMPL-109");
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": UserBindServer::add_user_id() scheduling failed: " << ex.what();
      logger()->log(
        ostr.str(),
        Logging::Logger::ERROR,
        Aspect::ACTION_FRONTEND,
        "ADS-IMPL-109");
    }

    co_return FrontendCommons::RequestResult{};
  }

  FrontendCommons::RequestTask
  Frontend::co_action_taken_(
    adserver::campaign_svcs::campaign_manager::ActionTakenRequest request)
    noexcept
  {
    try
    {
      const bool test_request = request.action_info().test_request();
      const auto user_status = request.action_info().user_status();
      auto action_result = co_await campaign_manager_coro_->co_action_taken(
        std::move(request));
      if (!action_result.status.ok())
      {
        Stream::Error ostr;
        ostr << "CampaignManager::action_taken(): "
          "gRPC call failed: code=" <<
          static_cast<int>(action_result.status.error_code()) <<
          ", message=" << action_result.status.error_message();
        logger()->log(
          ostr.str(),
          Logging::Logger::EMERGENCY,
          Aspect::ACTION_FRONTEND,
          "ADS-ICON-4");
        co_return FrontendCommons::RequestResult{};
      }

      if (stats_)
      {
        stats_->consider_request(test_request, user_status);
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "Action::Frontend::co_action_taken_(): "
        "CampaignManager::action_taken() scheduling failed: " << ex.what();
      logger()->log(
        ostr.str(),
        Logging::Logger::EMERGENCY,
        Aspect::ACTION_FRONTEND,
        "ADS-ICON-4");
    }

    co_return FrontendCommons::RequestResult{};
  }
}
