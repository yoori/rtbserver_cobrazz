#include <Logger/StreamLogger.hpp>
#include <HTTP/HTTPCookie.hpp>

#include <utility>
#include <Generics/Uuid.hpp>

#include <Commons/UserInfoManip.hpp>
#include <Commons/Algs.hpp>

#include <Commons/ConfigUtils.hpp>
#include <Commons/ErrorHandler.hpp>
#include <Commons/ExternalUserIdUtils.hpp>
#include <Commons/Base32.hpp>
#include <Commons/GrpcAlgs.hpp>
#include <Commons/JsonFormatter.hpp>

#include <Frontends/CommonModule/CommonModule.hpp>
#include <Frontends/FrontendCommons/HTTPUtils.hpp>
#include <Frontends/FrontendCommons/RequestParamProcessor.hpp>
#include <Frontends/FrontendCommons/add_UID_cookie.hpp>

#include <Frontends/FrontendCommons/UserInfoClientConfig.hpp>

#include "UserBindFrontend.hpp"
#include "BindRequestState.hpp"
#include "UserBindMatchRequestState.hpp"

namespace Config
{
  const char ENABLE[] = "UserBindFrontend_Enable";
  const char CONFIG_FILES[] = "UserBindFrontend_Config";
  const char CONFIG_FILE[] = "UserBindFrontend_ConfigFile";
}

namespace Aspect
{
  const char USER_BIND_FRONTEND[] = "UserBindFrontend";
}

namespace UrlPath
{
  const String::SubString kGetUserId("/get_user_id");
  const String::SubString kGetSegments("/segments");
}

namespace Response::Type
{
    const String::SubString JSON("application/json");
  }

namespace TemplateParams
{
  const String::SubString MARKER("##");
  const String::SubString SIGNEDUID("SIGNEDUID");
  const String::SubString SIGNEDCOOKIEUID("SIGNEDCOOKIEUID");
  const String::SubString SSPUID("SSPUID");
  const String::SubString UNSIGNEDSSPUID("UNSIGNEDSSPUID");
  const String::SubString UNSIGNEDUID("UNSIGNEDUID");
  const String::SubString EXTERNALID("EXTERNALID");
  const String::SubString SHORTEXTERNALID("SHORTEXTERNALID");
  const String::SubString YANDEXSIGN("YANDEXSIGN");
  const String::SubString PASSBACK_URL("PASSBACK_URL");
  const String::SubString RANDOM("RANDOM");
  const String::SubString PUSH_DATA("PUSH_DATA");
  const String::SubString BINDREQUESTID("BINDREQUESTID");
  const String::SubString LOCATION("LOCATION");
}

namespace
{
  const String::SubString HTTP_PREFIX("http:");
  const String::SubString HTTPS_PREFIX("https:");

  const String::SubString GOOGLE_ERRORS[] = {
    String::SubString(
      "User has a Google cookie, but has opted "
      "out of any tracking using this cookie"),
    String::SubString(
      "No valid operations specified. e.g., a "
      "no-op request was received"),
    String::SubString(
      "User does not have a Google cookie. "
      "Google will not set the cookie via the "
      "Cookie Matching Service"),
    String::SubString(
      "Conflicting operations specified. You are "
      "not allowed to specify both the google_push "
      "and google_cm flags on the same request since "
      "they have conflicting purposes"),
    String::SubString(
      "An invalid google_push parameter was passed in "
      "a redirect to a Google server as part of a Pixel "
      "Matching Service request. Your redirect must set "
      "google_push to the same value passed to you in "
      "the initial pixel request")
  };

  /*
  void google_error(
    Stream::Error& error_strm,
    unsigned long error_code)
  {
    error_strm << "'" << error_code << "': ";
    if (error_code <= sizeof(GOOGLE_ERRORS) / sizeof(GOOGLE_ERRORS[0]) &&
        error_code > 0)
    {
      error_strm << GOOGLE_ERRORS[error_code - 1];
    }
    else
    {
      error_strm << "unknown";
    }
  }
  */

  namespace WebStats
  {
    const String::SubString APPLICATION("adserver");
    const String::SubString SOURCE("userbind");
    const String::SubString OPERATION("mapping");
    const String::SubString INVALID_MAPPING_OPERATION("invalid-mapping");
  }
}

namespace AdServer
{
  class KeyArgsCallback: public String::TextTemplate::ArgsCallback
  {
    virtual bool
    get_argument(const String::SubString& key, std::string& result,
      bool /*value*/) const /*throw(eh::Exception)*/
    {
      key.assign_to(result);
      return true;
    }
  };

  UserBindFrontend::ProcessRequestTask
  UserBindFrontend::co_process_request_(
    UserBind::RequestInfo_var request_info,
    std::string dns_bind_request_id)
    noexcept
  {
    auto state = std::make_shared<BindRequestState>(
      this,
      std::move(request_info),
      std::move(dns_bind_request_id));
    auto result = co_await state->co_process_();
    co_return result;
  }

  bool
  UserBindFrontend::has_user_bind_client_() const noexcept
  {
    return static_cast<bool>(user_bind_client_coro_);
  }

  void
  UserBindFrontend::schedule_user_match_(
    const Commons::UserId& result_user_id,
    const Commons::UserId& merge_user_id,
    bool create_user_profile,
    const String::SubString& keywords,
    const String::SubString& cohort,
    const String::SubString& referer,
    unsigned long colo_id,
    FrontendCommons::Location_var location,
    const String::SubString& source)
    noexcept
  {
    if(!config_->enable_profiling() ||
      !match_workers_ ||
      result_user_id.is_null() ||
      (referer.empty() &&
        cohort.empty() &&
        merge_user_id.is_null() &&
        !create_user_profile))
    {
      return;
    }

    unsigned long cur_task_count =
      match_task_count_.exchange_and_add(1) + 1;

    if(cur_task_count > config_->match_pending_task_limit() +
      config_->match_threads())
    {
      match_task_count_ += -1;
      return;
    }

    match_workers_->post(
      [this,
        result_user_id,
        merge_user_id,
        create_user_profile,
        keywords = keywords.str(),
        cohort = cohort.str(),
        referer = referer.str(),
        colo_id,
        location = std::move(location),
        source = source.str()]()
      {
        match_task_count_ += -1;
        user_match_(
          result_user_id,
          merge_user_id,
          create_user_profile,
          keywords,
          cohort,
          referer,
          colo_id,
          location,
          source);
      });
  }

  //
  // UserBindFrontend implementation
  //
  UserBindFrontend::UserBindFrontend(
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
            frontend_config->get().ContentFeConfiguration()->Logger().log_level())),
        "UserBindFrontend",
        Aspect::USER_BIND_FRONTEND,
        0),
      frontend_config_(ReferenceCounting::add_ref(frontend_config)),
      common_module_(ReferenceCounting::add_ref(common_module)),
      workers_(std::move(request_workers)),
      match_task_count_(0)
  {}

  bool
  UserBindFrontend::will_handle(const String::SubString& uri) noexcept
  {
    std::string found_uri;

    bool result =
      FrontendCommons::find_uri(
        config_->PathUriList().Uri(), uri, found_uri, 0, false) ||
      FrontendCommons::find_uri(
        config_->UriList().Uri(), uri, found_uri) ||
      FrontendCommons::find_uri(
        config_->PixelUriList().Uri(), uri, found_uri);

    if(logger()->log_level() >= TraceLevel::MIDDLE)
    {
      Stream::Error ostr;
      ostr << "UserBindFrontend::will_handle(" << uri <<
        "), service: '" << found_uri << "'";

      logger()->log(ostr.str());
    }

    return result;
  }

  void
  UserBindFrontend::parse_configs_() /*throw(Exception)*/
  {
    static const char* FUN = "UserBindFrontend::parse_configs_()";

    /* load common configuration */
    Config::ErrorHandler error_handler;

    try
    {
      typedef Configuration::FeConfig Config;
      const Config& fe_config = frontend_config_->get();

      if(!fe_config.CommonFeConfiguration().present())
      {
        throw Exception("CommonFeConfiguration not presented.");
      }

      common_config_.reset(
        new CommonFeConfiguration(*fe_config.CommonFeConfiguration()));

      if(!fe_config.UserBindFeConfiguration().present())
      {
        throw Exception("BiddingFeConfiguration isn't present");
      }

      config_.reset(
        new UserBindFeConfiguration(*fe_config.UserBindFeConfiguration()));

    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't parse config file '" << fe_config_path_ << "': " <<
        e.what();
      throw Exception(ostr);
    }
  }

  void
  UserBindFrontend::init() /*throw(eh::Exception)*/
  {
    static const char* FUN = "UserBindFrontend::init()";

    if(true) // module_used())
    {
      try
      {
        parse_configs_();

        grpc_executor_ = std::make_shared<AdServer::Grpc::GrpcExecutor>(
          common_config_->grpc_executor_threads());
        add_child_object(grpc_executor_);

        auto user_bind_client =
          AdServer::UserInfoSvcs::create_distributed_user_bind_client(
            *common_config_,
            grpc_executor_,
            common_module_->grpc_coalesce_runner(),
            logger());
        if(user_bind_client)
        {
          user_bind_client_coro_ = std::make_shared<
            AdServer::UserInfoSvcs::UserBindServerGrpcCoroClient>(
              user_bind_client,
              workers_);
          add_child_object(user_bind_client);
        }

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
            common_module_->grpc_coalesce_runner());
        channel_client_coro_ = std::make_shared<
          AdServer::ChannelSvcs::ChannelServerGrpcCoroClient>(
            channel_client,
            workers_);
        add_child_object(channel_client);

        auto campaign_manager = std::make_shared<
          AdServer::CampaignSvcs::CampaignManagerDistributedGrpcClient>(
            FrontendCommons::read_campaign_manager_grpc_refs(*common_config_),
            AdServer::Grpc::BatchingOptions(),
            grpc_executor_,
            common_module_->grpc_coalesce_runner());
        campaign_manager_coro_ = std::make_shared<
          AdServer::CampaignSvcs::CampaignManagerGrpcCoroClient>(
            campaign_manager,
            workers_);
        add_child_object(campaign_manager);

        cookie_manager_.reset(
          new FrontendCommons::CookieManager<
            FCGI::HttpRequest, FCGI::HttpResponse>(
              common_config_->Cookies()));

        for (UserBindFeConfiguration::Source_sequence::const_iterator
             it = config_->Source().begin(); it != config_->Source().end(); ++it)
        {
          SourceEntity_var& source = sources_[it->id()];
          if(!source.in())
          {
            source = new SourceEntity();
          }

          if(!it->redirect().empty())
          {
            std::string keywords = it->keywords();
            String::SubString keywords_ss(keywords);

            RedirectRule::AllowedParams allowed_params;
            auto it_redirect_param = it->redirectUrlParam().begin();
            const auto it_redirect_param_end = it->redirectUrlParam().end();
            for (; it_redirect_param != it_redirect_param_end; ++it_redirect_param)
            {
              allowed_params.try_emplace(
                it_redirect_param->name(),
                it_redirect_param->token());
            }

            source->rules.push_back(
              init_redirect_rule_(
                it->redirect(),
                !keywords.empty() ? &keywords_ss : 0,
                it->passback(),
                it->weight(),
                it->location(),
                it->redirect_empty_uid(),
                std::move(allowed_params)));
          }
        }

        const char* geo_ip_path = 0;

        if(common_config_->GeoIP().present())
        {
          geo_ip_path = common_config_->GeoIP()->path().c_str();
        }

        UserBind::RequestInfoFiller::ExternalUserIdSet skip_external_ids;

        if (common_config_->SkipExternalIds().present())
        {
          for(CommonFeConfiguration::SkipExternalIds_type::Id_sequence::const_iterator
                it = common_config_->SkipExternalIds()->Id().begin();
              it != common_config_->SkipExternalIds()->Id().end(); ++it)
          {
            skip_external_ids.insert(it->value());
          }

          String::SubString skip_ids =
            common_config_->SkipExternalIds()->skip_external_ids();

          if (!skip_ids.empty())
          {
            String::StringManip::SplitNL tokenizer(skip_ids);
            for (String::SubString skip_id; tokenizer.get_token(skip_id);)
            {
              skip_external_ids.insert(skip_id.str());
            }
          }
        }

        UserBind::RequestInfoFiller::AllowedPassbackDomainArray
          allowed_passback_domains;

        for(auto it = config_->AllowedPassback().begin();
          it != config_->AllowedPassback().end(); ++it)
        {
          if(!it->domain().empty())
          {
            allowed_passback_domains.push_back(it->domain());
          }
        }

        request_info_filler_.reset(
          new UserBind::RequestInfoFiller(
            logger(),
            common_module_,
            geo_ip_path,
            skip_external_ids,
            allowed_passback_domains,
            common_config_->colo_id()));

        if (config_->pixel_path().present())
        {
          pixel_ = FileCachePtr(new FileCache((*config_->pixel_path()).c_str()));
        }

        pixel_content_type_ = config_->pixel_content_type();

        if(config_->match_threads() > 0)
        {
          match_workers_ = new FrontendCommons::FrontendWorkers(
            callback(),
            config_->match_threads());
          add_child_object(match_workers_);
        }

        activate_object();
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << ex.what();
        throw Exception(ostr);
      }

      logger()->log(String::SubString(
          "UserBindFrontend::init(): frontend is running ..."),
        Logging::Logger::INFO,
        Aspect::USER_BIND_FRONTEND);
    }
  }

  void
  UserBindFrontend::shutdown() noexcept
  {
    try
    {
      deactivate_object();
      wait_object();

      Stream::Error ostr;
      ostr << "UserBindFrontend::shutdown(): frontend terminated (pid = " <<
        ::getpid() << ").";

      logger()->log(ostr.str(),
        Logging::Logger::INFO,
        Aspect::USER_BIND_FRONTEND);
    }
    catch(...)
    {}
  }

  uint32_t
  UserBindFrontend::calc_yandex_sign_(
    const UserBind::RequestInfo& request_info,
    const std::string& user_data,
    const std::string* location,
    const std::string& secure_key)
    noexcept
  {
    std::string res;
    size_t pos = static_cast<size_t>(-1);
    for(auto i = 0; i < 3; ++i)
    {
      pos = request_info.peer_ip.find('.', pos + 1);
      if(pos == std::string::npos)
      {//not valid ip
        return 0;
      }
    }
    res = request_info.peer_ip.substr(0, pos);
    res += request_info.referer;
    res += request_info.user_agent;
    if(location)
    {
      res += *location;
    }
    res += user_data;
    res += secure_key;

    if(logger()->log_level() >= Logging::Logger::TRACE)
    {
      logger()->sstream(Logging::Logger::TRACE, Aspect::USER_BIND_FRONTEND) <<
        "string for calculation of yandex sum '" << res << "'";
    }

    return Generics::CRC::reversed(0, res.data(), res.size());
  }

  FrontendCommons::RequestTask
  UserBindFrontend::co_handle_request(
    FCGI::HttpRequestHolder_var request_holder)
    noexcept
  {
    static const char* FUN = "UserBindFrontend::co_handle_request()";
    co_await AdServer::Commons::ExecutorPool::yield(workers_);

    const FCGI::HttpRequest& request = request_holder->request();

    FCGI::HttpResponse_var response_ptr(new FCGI::HttpResponse());
    FCGI::HttpResponse& response = *response_ptr;

    auto finish_response =
      [this,
        request_holder,
        response_ptr](int http_status) mutable
      {
        const FCGI::HttpRequest& request = request_holder->request();
        FCGI::HttpResponse& response = *response_ptr;
        std::string found_uri;
        if(http_status == 204 &&
          FrontendCommons::find_uri(
            config_->PixelUriList().Uri(),
            request.uri(),
            found_uri) &&
          pixel_.in())
        {
          http_status = 200;
          response.set_content_type_nocopy(pixel_content_type_);

          FileCache::BufferHolder_var buffer = pixel_->get();
          response.get_output_stream().write((*buffer)->data(), (*buffer)->size());
        }

        return http_status;
      };

    try
    {
      std::string path_args;
      std::string found_uri;
      if(FrontendCommons::find_uri(
           config_->PathUriList().Uri(), request.uri(), found_uri, 0, false))
      {
        path_args = request.uri().substr(found_uri.length()).str();
      }

      UserBind::RequestInfo_var request_info_holder =
        new UserBind::RequestInfo();
      request_info_filler_->fill(
        *request_info_holder,
        request,
        path_args);

      const UserBind::RequestInfo& request_info = *request_info_holder;

      FrontendCommons::CORS::set_headers(request, response);

      if (request.uri().substr(0, UrlPath::kGetUserId.size()) ==
          UrlPath::kGetUserId &&
        (request.uri().size() == UrlPath::kGetUserId.size() ||
          (request.uri().size() > UrlPath::kGetUserId.size()
          && request.uri()[UrlPath::kGetUserId.size()] == '?')))
      {
        std::ostringstream stream;
        if (!request_info.user_id.is_null())
        {
          stream << "{\"uid\":\""
                 << request_info.user_id.to_string()
                 << "\"}";
        }
        else
        {
          stream << "{}";
        }

        response.set_content_type_nocopy(Response::Type::JSON);
        response.write(stream.str());
        const int http_status = finish_response(200);
        co_return FrontendCommons::RequestResult{
          http_status,
          response_ptr,
          false};
      }

      if (request.uri().substr(0, UrlPath::kGetSegments.size()) ==
          UrlPath::kGetSegments &&
        (request.uri().size() == UrlPath::kGetSegments.size() ||
          (request.uri().size() > UrlPath::kGetSegments.size()
          && request.uri()[UrlPath::kGetSegments.size()] == '?')))
      {
        auto channels_result = co_await co_handle_user_channels_request_(
          request_info_holder,
          response_ptr);
        const int channels_status = channels_result.status;
        const int http_status = finish_response(channels_status);
        co_return FrontendCommons::RequestResult{
          http_status,
          response_ptr,
          false};
      }

      if(!request.secure() &&
         config_->nosecure_redirect().present() &&
         !config_->nosecure_redirect()->empty() &&
         !request_info.disable_secure_redirect)
      {
        std::string result_url = *(config_->nosecure_redirect()) + request.uri();
        const int http_status = finish_response(
          FrontendCommons::redirect(result_url, response));
        co_return FrontendCommons::RequestResult{
          http_status,
          response_ptr,
          false};
      }

      if(request_info.google_error != 0)
      {
        const int http_status = finish_response(204);
        co_return FrontendCommons::RequestResult{
          http_status,
          response_ptr,
          false};
      }

      const bool opted_out =
        request_info.user_status == AdServer::CampaignSvcs::US_OPTOUT;
      if(opted_out)
      {
        if(!request_info.user_id.is_null())
        {
          throw InvalidParamException("");
        }

        const int http_status = finish_response(204);
        co_return FrontendCommons::RequestResult{
          http_status,
          response_ptr,
          false};
      }

      if(request_info.external_id.empty() &&
        !request_info.generate_external_id)
      {
        throw InvalidParamException("");
      }

      std::vector<UserBindFrontend::RedirectRule_var> redirect_rules;
      std::vector<UserBindFrontend::RedirectRule_var> keyword_redirect_rules;

      const SourceMap::iterator source_it =
        sources_.find(request_info.source_id);
      if(source_it != sources_.end())
      {
        const SourceEntity& source = *source_it->second;

        std::vector<std::string> keywords;
        FrontendCommons::get_ip_keywords(keywords, request_info.peer_ip);

        for(auto bind_rule_it = source.rules.begin();
          bind_rule_it != source.rules.end();
          ++bind_rule_it)
        {
          if((*bind_rule_it)->use_keywords)
          {
            if(!(*bind_rule_it)->passback ||
              ((*bind_rule_it)->passback && request_info.passback))
            {
              if((*bind_rule_it)->keywords.empty())
              {
                redirect_rules.push_back(*bind_rule_it);
              }
              else
              {
                for(auto keyword_it = keywords.begin();
                  keyword_it != keywords.end();
                  ++keyword_it)
                {
                  if((*bind_rule_it)->keywords.find(*keyword_it) !=
                    (*bind_rule_it)->keywords.end())
                  {
                    keyword_redirect_rules.push_back(*bind_rule_it);
                  }
                }
              }
            }
          }
          else if(!(*bind_rule_it)->passback ||
            ((*bind_rule_it)->passback && request_info.passback))
          {
            redirect_rules.push_back(*bind_rule_it);
          }
        }
      }

      const std::vector<UserBindFrontend::RedirectRule_var>& select_rules =
        !keyword_redirect_rules.empty() ? keyword_redirect_rules : redirect_rules;

      unsigned long sum_weight = 0;
      for(auto rule_it = select_rules.begin();
        rule_it != select_rules.end();
        ++rule_it)
      {
        sum_weight += (*rule_it)->weight;
      }

      UserBindFrontend::RedirectRule_var redirect_rule;
      if(!select_rules.empty())
      {
        redirect_rule = *select_rules.begin();

        unsigned long sel_weight = Generics::safe_rand(sum_weight);
        sum_weight = 0;
        for(auto rule_it = select_rules.begin();
          rule_it != select_rules.end();
          ++rule_it)
        {
          if(sum_weight <= sel_weight &&
            sel_weight < sum_weight + (*rule_it)->weight)
          {
            redirect_rule = *rule_it;
            break;
          }
          sum_weight += (*rule_it)->weight;
        }
      }

      std::string dns_bind_request_id;
      if(redirect_rule && redirect_rule->init_bind_request)
      {
        AdServer::Commons::RequestId gen_id =
          AdServer::Commons::RequestId::create_random_based();
        AdServer::Commons::base32_encode(
          dns_bind_request_id,
          String::SubString(reinterpret_cast<const char*>(&*gen_id.begin()), 16));
        dns_bind_request_id = std::string("r") + dns_bind_request_id;
      }

      ProcessRequestResult process_result = co_await co_process_request_(
        request_info_holder,
        dns_bind_request_id);

      {
          const FCGI::HttpRequest& request = request_holder->request();
          FCGI::HttpResponse& response = *response_ptr;
          const UserBind::RequestInfo& request_info = *request_info_holder;
          BindResult& bind_result = process_result.bind_result;
          int http_status =
            process_result.status == 200 ? 204 : process_result.status;
          std::string redirect_url;

          try
          {
            if(redirect_rule.in() && (
              !request_info.user_id.is_null() ||
              redirect_rule->redirect_empty_uid))
            {
              String::TextTemplate::Args templ_args;

              const auto& params = request.params();
              const auto& allowed_params = redirect_rule->allowed_params;
              if(!allowed_params.empty())
              {
                for(const auto& param : params)
                {
                  const auto it = allowed_params.find(param.name);
                  if(it != std::end(allowed_params))
                  {
                    templ_args[it->second] = param.value;
                  }
                }
              }

              templ_args[TemplateParams::EXTERNALID] = request_info.external_id;
              templ_args[TemplateParams::SHORTEXTERNALID] =
                request_info.short_external_id;
              templ_args[TemplateParams::RANDOM] =
                String::StringManip::IntToStr(
                  Generics::safe_rand()).str().str();

              if(!request_info.passback_url.empty())
              {
                templ_args[TemplateParams::PASSBACK_URL] =
                  request_info.passback_url;
              }

              if(!bind_result.result_user_id.is_null())
              {
                templ_args[TemplateParams::UNSIGNEDUID] =
                  bind_result.result_user_id.to_string();
                templ_args[TemplateParams::SIGNEDUID] =
                  common_module_->user_id_controller()->sign(
                    bind_result.result_user_id).str();
              }

              if(!request_info.user_id.is_null())
              {
                templ_args[TemplateParams::SIGNEDCOOKIEUID] =
                  common_module_->user_id_controller()->sign(
                    request_info.user_id).str();
              }

              std::string ssp_user_id_str;
              if(!bind_result.ssp_user_id.is_null() ||
                !request_info.user_id.is_null())
              {
                Commons::UserId result_ssp_user_id = bind_result.ssp_user_id;
                if(result_ssp_user_id.is_null())
                {
                  result_ssp_user_id =
                    common_module_->user_id_controller()->ssp_uuid(
                      request_info.user_id,
                      request_info.source_id);
                }

                ssp_user_id_str =
                  common_module_->user_id_controller()->ssp_uuid_string(
                    result_ssp_user_id);

                templ_args[TemplateParams::SSPUID] =
                  common_module_->user_id_controller()->sign(
                    bind_result.ssp_user_id, UserIdController::SSP).str();

                templ_args[TemplateParams::UNSIGNEDSSPUID] = ssp_user_id_str;
              }

              if(!request_info.push_data.empty())
              {
                templ_args[TemplateParams::PUSH_DATA] = request_info.push_data;
              }

              if(!dns_bind_request_id.empty())
              {
                templ_args[TemplateParams::BINDREQUESTID] = dns_bind_request_id;
              }

              String::TextTemplate::DefaultValue args_with_default(&templ_args);
              String::TextTemplate::ArgsEncoder args_with_encoding(
                &args_with_default);

              std::string location;
              if(redirect_rule->location)
              {
                location = redirect_rule->redirect.instantiate(args_with_encoding);
                templ_args[TemplateParams::LOCATION] = location;
              }

              uint32_t yandex_sign = 0;
              if(request_info.external_id.empty() && config_->Keys().present())
              {
                yandex_sign = calc_yandex_sign_(
                  request_info,
                  ssp_user_id_str,
                  !location.empty() ? &location : 0,
                  config_->Keys().get().yandex_key().present() ?
                    config_->Keys().get().yandex_key().get() : "" );
              }

              templ_args[TemplateParams::YANDEXSIGN] =
                String::StringManip::IntToStr(yandex_sign).str().str();

              std::string redirect =
                redirect_rule->redirect.instantiate(args_with_encoding);

              if(!redirect.empty())
              {
                if(!request_info.ssp_id.empty())
                {
                  std::string mimed_ssp_id;
                  String::StringManip::mime_url_encode(
                    request_info.ssp_id,
                    mimed_ssp_id);

                  redirect += redirect.find('?') == std::string::npos ?
                    '?' : '&';
                  redirect += "ssp=";
                  redirect += mimed_ssp_id;
                }

                redirect_url = FrontendCommons::normalize_abs_url(
                  HTTP::BrowserAddress(redirect),
                  HTTP::HTTPAddress::VW_FULL,
                  FrontendCommons::is_secure_request(request) ||
                    request_info.secure ?
                      HTTPS_PREFIX : HTTP_PREFIX);
              }
            }
          }
          catch(const eh::Exception& e)
          {
            Stream::Error ostr;
            ostr << FUN << ": source processing failed for external_id = '" <<
              request_info.external_id << "'; " << e.what();
            logger()->log(ostr.str(),
              Logging::Logger::ERROR,
              Aspect::USER_BIND_FRONTEND);
          }

          if(!bind_result.result_user_id.is_null())
          {
            const Generics::SignedUuid signed_uid =
              common_module_->user_id_controller()->sign(
                bind_result.result_user_id);
            FrontendCommons::add_UID_cookie(
              response,
              request,
              *cookie_manager_,
              signed_uid.str());
          }

          if(!redirect_url.empty())
          {
            http_status = FrontendCommons::redirect(redirect_url, response);
          }

          if(http_status == 204 && !request_info.passback_url.empty())
          {
            http_status = FrontendCommons::redirect(
              request_info.passback_url,
              response);
          }

          http_status = finish_response(http_status);
          co_return FrontendCommons::RequestResult{
            http_status,
            response_ptr,
            false};
      }
    }
    catch(const InvalidParamException&)
    {
      response.add_header_nocopy(
        String::SubString("X-Status"),
        String::SubString("Bad request"));
      const int http_status = finish_response(204);
      co_return FrontendCommons::RequestResult{
        http_status,
        response_ptr,
        false};
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": eh::Exception caught: " << e.what();
      logger()->log(ostr.str(),
        Logging::Logger::EMERGENCY,
        Aspect::USER_BIND_FRONTEND,
        "ADS-IMPL-109");
      const int http_status = finish_response(500);
      co_return FrontendCommons::RequestResult{
        http_status,
        response_ptr,
        false};
    }
  }

  void
  UserBindFrontend::report_bad_user_(
    const UserBind::RequestInfo& /*request_info*/)
    noexcept
  {
    try
    {
    }
    catch (const Exception&)
    {}
    catch(const eh::Exception&)
    {}
  }

  void
  UserBindFrontend::log_cookie_mapping_(
    const UserBind::RequestInfo& request_info)
    noexcept
  {
    if(config_->cookie_mapping_log() && campaign_manager_coro_)
    {
      adserver::campaign_svcs::campaign_manager::ConsiderWebOperationRequest
        web_op_info;
      web_op_info.set_time(GrpcAlgs::pack_time(request_info.time));
      web_op_info.set_colo_id(request_info.colo_id);
      web_op_info.set_tag_id(0);
      web_op_info.set_cc_id(0);
      web_op_info.set_app(WebStats::APPLICATION.str());
      web_op_info.set_source(WebStats::SOURCE.str());
      web_op_info.set_operation(WebStats::OPERATION.str());
      web_op_info.set_result('U');
      web_op_info.set_user_status(
        static_cast<unsigned long>(request_info.user_status));
      web_op_info.set_test_request(false);
      web_op_info.set_user_bind_src(request_info.source_id);

      co_consider_web_operation_(std::move(web_op_info)).start_detached(nullptr);
    }
  }

  FrontendCommons::RequestTask
  UserBindFrontend::co_consider_web_operation_(
    adserver::campaign_svcs::campaign_manager::ConsiderWebOperationRequest
      request)
    noexcept
  {
    static const char* FUN = "UserBindFrontend::co_consider_web_operation_()";

    try
    {
      auto result = co_await campaign_manager_coro_->consider_web_operation(
        std::move(request));
      if(!result.status.ok() &&
        result.status.error_code() != grpc::StatusCode::INVALID_ARGUMENT)
      {
        logger()->sstream(
          Logging::Logger::ERROR,
          Aspect::USER_BIND_FRONTEND,
          "ADS-IMPL-7805") <<
          FUN << ": CampaignManager::consider_web_operation() "
          "gRPC call failed: code=" <<
          static_cast<int>(result.status.error_code()) <<
          ", message=" << result.status.error_message();
      }
    }
    catch(const eh::Exception& ex)
    {
      logger()->sstream(
        Logging::Logger::ERROR,
        Aspect::USER_BIND_FRONTEND,
        "ADS-IMPL-7805") <<
        FUN << ": CampaignManager::consider_web_operation(): " << ex.what();
    }

    co_return FrontendCommons::RequestResult::written();
  }

  void
  UserBindFrontend::user_match_(
    const Commons::UserId& result_user_id,
    const Commons::UserId& merge_user_id,
    bool create_user_profile,
    const String::SubString& keywords,
    const String::SubString& cohort,
    const String::SubString& referer,
    unsigned long colo_id,
    FrontendCommons::Location_var location,
    const String::SubString& source)
    noexcept
  {
    std::make_shared<UserBindMatchRequestState>(
      this,
      result_user_id,
      merge_user_id,
      create_user_profile,
      keywords,
      cohort,
      referer,
      colo_id,
      std::move(location),
      source)->start();
  }

  void
  UserBindFrontend::fill_match_request_info_(
    adserver::campaign_svcs::campaign_manager::MatchRequestInfo& mri,
    const AdServer::Commons::UserId& user_id,
    const Generics::Time& now,
    const adserver::channel_svcs::channel_server::MatchResponse*
      trigger_match_result,
    const adserver::user_info_svcs::user_info_manager::MatchResponse*
      history_match_result,
    const FrontendCommons::Location* location,
    const String::SubString& referer,
    const String::SubString& source)
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
    match_info->set_full_referer(referer.str());
    mri.set_source(source.str());

    if(trigger_match_result)
    {
      const auto& page_channels =
        trigger_match_result->matched_channels().page_channels();
      const int result_len = page_channels.size();
      for(int i = 0; i < result_len; ++i)
      {
        auto* pkw_channel = match_info->add_pkw_channels();
        pkw_channel->set_channel_id(page_channels[i].id());
        pkw_channel->set_channel_trigger_id(page_channels[i].trigger_channel_id());
      }
    }

    if(history_match_result)
    {
      const int result_len = history_match_result->match_result().channels_size();
      for(int i = 0; i < result_len; ++i)
      {
        match_info->add_channels(
          history_match_result->match_result().channels(i).channel_id());
      }
    }

    if (location)
    {
      auto* geo_info = match_info->add_location();
      geo_info->set_country(location->country);
      geo_info->set_region(location->region);
      geo_info->set_city(location->city);
    }
  }

  UserBindFrontend::RedirectRule_var
  UserBindFrontend::init_redirect_rule_(
    const String::SubString& redirect,
    const String::SubString* keywords,
    const bool passback,
    const unsigned long weight,
    const String::SubString& location,
    const bool redirect_empty_uid,
    RedirectRule::AllowedParams&& allowed_params)
    /*throw(UserBindFrontend::InvalidSource)*/
  {
    static const char* FUN = "UserBindFrontend::init_redirect_rule_";

    try
    {
      UserBindFrontend::RedirectRule_var redirect_rule = new UserBindFrontend::RedirectRule();

      init_redirect_template_(redirect_rule->redirect, redirect);

      if(!location.empty())
      {
        redirect_rule->location.reset(new String::TextTemplate::IStream());
        init_redirect_template_(*(redirect_rule->location), location);
      }

      if(keywords)
      {
        String::StringManip::Splitter<String::AsciiStringManip::SepNL> splitter(
          *keywords);
        String::SubString token;
        while(splitter.get_token(token))
        {
          String::StringManip::trim(token);
          if(!token.empty())
          {
            redirect_rule->keywords.insert(Generics::StringHashAdapter(token));
          }
        }

        redirect_rule->use_keywords = true;
      }
      else
      {
        redirect_rule->use_keywords = false;
      }

      redirect_rule->passback = passback;
      redirect_rule->init_bind_request = false;
      redirect_rule->weight = weight;
      redirect_rule->redirect_empty_uid = redirect_empty_uid;
      redirect_rule->allowed_params = std::move(allowed_params);

      {
        String::TextTemplate::Keys keys;

        KeyArgsCallback null_args;
        String::TextTemplate::DefaultValue default_cont(&null_args);
        String::TextTemplate::ArgsEncoder encoder(&default_cont);
        redirect_rule->redirect.keys(encoder, keys);

        if(keys.find(TemplateParams::BINDREQUESTID.str()) != keys.end())
        {
          redirect_rule->init_bind_request = true;
        }
      }

      return redirect_rule;
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": eh::Exception caught: " << ex.what();
      throw InvalidSource(ostr);
    }
  }

  void
  UserBindFrontend::init_redirect_template_(
    String::TextTemplate::IStream& templ,
    const String::SubString& text)
    /*throw(UserBindFrontend::InvalidSource)*/
  {
    static const char* FUN = "UserBindFrontend::init_redirect_template_()";

    Stream::Parser istr(text);
    templ.init(istr, TemplateParams::MARKER, TemplateParams::MARKER);

    AdServer::Commons::UserId test_uid;
    std::string test_uid_str = test_uid.to_string(false);

    String::TextTemplate::Args templ_args;
    templ_args[TemplateParams::SSPUID] = "";
    templ_args[TemplateParams::UNSIGNEDSSPUID] = test_uid_str;
    templ_args[TemplateParams::EXTERNALID] = "";
    templ_args[TemplateParams::SHORTEXTERNALID] = "";
    templ_args[TemplateParams::YANDEXSIGN] = "0";
    templ_args[TemplateParams::RANDOM] = "0";
    templ_args[TemplateParams::PUSH_DATA] = "";
    templ_args[TemplateParams::BINDREQUESTID] = "r1";

    String::TextTemplate::DefaultValue args_with_default(&templ_args);
    String::TextTemplate::ArgsEncoder args_with_encoding(&args_with_default);

    // check template
    const std::string str = templ.instantiate(args_with_encoding);
    if(!str.empty())
    {
      HTTP::HTTPChecker checker;
      std::string error;

      if (!checker(str, &error, false))
      {
        Stream::Error ostr;
        ostr << FUN << ": invalid redirect template = '" << text <<
          "', error = " << error;
        throw InvalidSource(ostr);
      }
    }
  }

  FrontendCommons::RequestTask
  UserBindFrontend::co_handle_user_channels_request_(
    UserBind::RequestInfo_var request_info,
    FCGI::HttpResponse_var response)
    noexcept
  {
    static const char* FUN = "UserBindFrontend::handle_user_channels_request_()";

    if (request_info->user_id.is_null())
    {
      co_return FrontendCommons::RequestResult{400, response, false};
    }
    if (!user_info_client_coro_)
    {
      co_return FrontendCommons::RequestResult{500, response, false};
    }

    adserver::user_info_svcs::user_info_manager::MatchRequest
      history_match_request;
    auto* match_params = history_match_request.mutable_match_params();
    match_params->set_use_empty_profile(false);
    match_params->set_silent_match(false);
    match_params->set_no_match(false);
    match_params->set_no_result(false);
    match_params->set_ret_freq_caps(false);
    match_params->set_provide_channel_count(false);
    match_params->set_provide_persistent_channels(false);
    match_params->set_change_last_request(false);
    match_params->set_publishers_optin_timeout(
      GrpcAlgs::pack_time(Generics::Time::ZERO));
    auto* user_info = history_match_request.mutable_user_info();
    user_info->set_user_id(GrpcAlgs::pack_user_id(request_info->user_id));
    user_info->set_last_colo_id(request_info->colo_id);
    user_info->set_request_colo_id(request_info->colo_id);
    user_info->set_current_colo_id(-1);
    user_info->set_temporary(false);
    user_info->set_time(Generics::Time::get_time_of_day().tv_sec);

    auto match_result = co_await user_info_client_coro_->match(
      std::move(history_match_request));

    static const String::SubString JSON_SESSION_ID_NAME("session_id");
    static const String::SubString JSON_CL_ID_NAME("cl_id");
    static const String::SubString JSON_SEGMENTS_NAME("segments");

    if(!match_result.status.ok())
    {
      logger()->sstream(
        Logging::Logger::EMERGENCY,
        Aspect::USER_BIND_FRONTEND,
        match_result.status.error_code() == grpc::StatusCode::UNAVAILABLE ?
          "ADS-IMPL-7804" : "ADS-IMPL-7803") <<
        FUN << ": UserInfoManager::match() gRPC call failed: "
        "user_id = '" << request_info->user_id.to_string() <<
        "'; code=" << static_cast<int>(match_result.status.error_code()) <<
        ", message=" << match_result.status.error_message();
      co_return FrontendCommons::RequestResult{500, response, false};
    }

    std::vector<unsigned long> return_channel_ids;

    const auto& history_matched_channels =
      match_result.response.match_result().channels();
    for(const auto& history_matched_channel : history_matched_channels)
    {
      if(request_info->channels_wl.empty() ||
        request_info->channels_wl.find(
          history_matched_channel.channel_id()) !=
          request_info->channels_wl.end())
      {
        return_channel_ids.emplace_back(history_matched_channel.channel_id());
      }
    }

    std::ostringstream response_string;
    {
      AdServer::Commons::JsonFormatter root_json(response_string);
      if(!request_info->session_id.empty())
      {
        root_json.add(JSON_SESSION_ID_NAME, request_info->session_id);
      }

      if(!request_info->cl_id.empty())
      {
        root_json.add(JSON_CL_ID_NAME, request_info->cl_id);
      }

      AdServer::Commons::JsonObject segment_array(
        root_json.add_array(JSON_SEGMENTS_NAME));
      for(const auto& user_channel : return_channel_ids)
      {
        segment_array.add_number(user_channel);
      }
    }

    response->set_content_type_nocopy(Response::Type::JSON);
    response->write(response_string.str());
    co_return FrontendCommons::RequestResult{200, response, false};
  }
}
