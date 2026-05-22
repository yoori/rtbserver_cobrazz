
#include <coroutine>
#include <sstream>

#include <Logger/StreamLogger.hpp>
#include <String/StringManip.hpp>
#include <String/AsciiStringManip.hpp>
#include <HTTP/UrlAddress.hpp>

#include <Commons/ErrorHandler.hpp>
#include <Commons/GrpcAlgs.hpp>
#include <Commons/UserInfoManip.hpp>
#include <Commons/ExternalUserIdUtils.hpp>

#include <Frontends/FrontendCommons/HTTPUtils.hpp>
#include <Frontends/FrontendCommons/add_UID_cookie.hpp>
#include <Frontends/FrontendCommons/UserInfoClientConfig.hpp>

#include <unistd.h>

#include "ImprTrackFrontend.hpp"
#include "ImprTrackRequestState.hpp"
#include "ImprTrackMatchRequestState.hpp"

namespace
{
  struct ImprTrackFrontendConstrainTraits
  {
    static const unsigned long MAX_NUMBER_PARAMS = 30;
    static const unsigned long MAX_LENGTH_PARAM_NAME = 20;
    static const unsigned long MAX_LENGTH_PARAM_VALUE = 1024;
  };

  typedef const String::AsciiStringManip::Char2Category<',', ' '> ListParameterSepCategory;

  typedef FrontendCommons::DefaultConstrain<
    FrontendCommons::OnlyGetAllowed,
    FrontendCommons::ParamConstrainDefault,
    ImprTrackFrontendConstrainTraits>
      ImprTrackFrontendHTTPConstrain;

  namespace Tokens
  {
    const String::SubString BIND_URL_TOKEN("BINDURL");
    const String::SubString DNS_ENCODED_UIDS_TOKEN("DNSUIDS");
    const String::SubString RANDOM("RANDOM");
    const String::SubString SOURCE_ID("SOURCEID");
    const String::SubString EXTERNAL_USER_ID("EXTERNALID");
    const String::SubString ADD_USER_ID("ADDUSERID");
  }

  const String::SubString HTTPS_PREFIX("https:");
  const String::SubString HTTP_PREFIX("http:");
}

namespace Config
{
  const char ENABLE[] = "ImprTrackFrontend_Enable";
  const char CONFIG_FILE[] = "ImprTrackFrontend_Config";
}

namespace Aspect
{
  const char IMPR_TRACK_FRONTEND[] = "ImprTrackFrontend";
}

namespace AdServer::ImprTrack
{
  namespace Request::Cookie
  {
      const Generics::SubStringHashAdapter USER_ID(String::SubString("uid"));
    }

  namespace WebStats
  {
    const String::SubString APPLICATION("adserver");
    const String::SubString SOURCE("imprtrack");
    const String::SubString INVALID_MAPPING_OPERATION("invalid-mapping");
  }

  struct Frontend::MatchScheduleState
  {
    RequestInfo request_info;
    AdServer::Commons::UserId result_user_id;
    bool request_finished = false;
    bool verify_finished = false;
    bool scheduled = false;
    adserver::campaign_svcs::campaign_manager::ImpressionResultInfo
      impression_result_info;
  };

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
            frontend_config->get().ImprTrackFeConfiguration()->Logger().log_level())),
        "ImprTrackFrontend",
        Aspect::IMPR_TRACK_FRONTEND,
        0),
      frontend_config_(ReferenceCounting::add_ref(frontend_config)),
      common_module_(ReferenceCounting::add_ref(common_module)),
      workers_(std::move(request_workers))
  {
  }

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

      if(!fe_config.ImprTrackFeConfiguration().present())
      {
        throw Exception("ImprTrackFeConfiguration not presented.");
      }

      config_ = ConfigPtr(
        new ImprTrackFeConfiguration(*fe_config.ImprTrackFeConfiguration()));
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't parse config file: " << e.what();
      throw Exception(ostr);
    }
  }

  bool
  Frontend::will_handle(const String::SubString& uri) noexcept
  {
    std::string found_uri;
    bool result = FrontendCommons::find_uri(
      config_->UriList().Uri(), uri, found_uri);

    if(logger()->log_level() >= TraceLevel::MIDDLE)
    {
      Stream::Error ostr;
      ostr << "ImprTrack::Frontend::will_handle(" << uri << "), service: '" <<
        found_uri << "'";

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

        ip_map_ = common_module_->ip_mapper();
        cookie_manager_.reset(
          new FrontendCommons::CookieManager<
            FCGI::HttpRequest, FCGI::HttpResponse>(common_config_->Cookies()));
        if(config_->match_threads() > 0)
        {
          match_workers_ = workers_;
        }
        grpc_executor_ = common_module_->grpc_executor();
        auto campaign_manager = std::make_shared<
          AdServer::CampaignSvcs::CampaignManagerDistributedGrpcClient>(
            FrontendCommons::read_campaign_manager_grpc_refs(*common_config_),
            AdServer::Grpc::BatchingOptions(),
            grpc_executor_,
            common_module_->grpc_coalesce_runner());
        campaign_manager_coro_ =
          std::make_shared<AdServer::CampaignSvcs::CampaignManagerGrpcCoroClient>(
            campaign_manager,
            workers_);
        add_child_object(campaign_manager);
        auto user_bind_client =
          AdServer::UserInfoSvcs::create_distributed_user_bind_client(
            *common_config_,
            grpc_executor_,
            common_module_->grpc_coalesce_runner(),
            logger());
        user_bind_client_coro_ =
          std::make_shared<AdServer::UserInfoSvcs::UserBindServerGrpcCoroClient>(
            user_bind_client,
            workers_);
        if(user_bind_client)
        {
          add_child_object(user_bind_client);
        }
        auto user_info_client =
          AdServer::UserInfoSvcs::create_distributed_user_info_client(
            *common_config_,
            grpc_executor_,
            common_module_->grpc_coalesce_runner(),
            logger());
        user_info_client_coro_ =
          std::make_shared<AdServer::UserInfoSvcs::UserInfoManagerGrpcCoroClient>(
            user_info_client,
            workers_);
        add_child_object(user_info_client);
        auto channel_client =
          AdServer::ChannelSvcs::create_distributed_channel_client(
            *common_config_,
            grpc_executor_,
            common_module_->grpc_coalesce_runner());
        channel_client_coro_ =
          std::make_shared<AdServer::ChannelSvcs::ChannelServerGrpcCoroClient>(
            channel_client,
            workers_);
        add_child_object(channel_client);
        track_pixel_ = FileCachePtr(
          new FileCache(config_->track_pixel_path().c_str()));
        track_pixel_content_type_ = config_->track_pixel_content_type();
        RequestInfoFiller::EncryptionKeys_var default_keys =
          read_keys_(config_->DefaultKeys());
        RequestInfoFiller::EncryptionKeysMap account_keys;
        RequestInfoFiller::EncryptionKeysMap site_keys;

        // fill account and site keys
        for (ImprTrackFeConfiguration::AccountTraits_sequence::
             const_iterator it = config_->AccountTraits().begin();
           it != config_->AccountTraits().end(); ++it)
        {
          account_keys[it->account_id()] = read_keys_(*it);
        }

        for (ImprTrackFeConfiguration::SiteTraits_sequence::
            const_iterator it = config_->SiteTraits().begin();
          it != config_->SiteTraits().end(); ++it)
        {
          site_keys[it->site_id()] = read_keys_(*it);
        }
        template_files_ = new Commons::TextTemplateCache(
          static_cast<unsigned long>(-1),
          Generics::Time::ONE_MINUTE,
          Commons::TextTemplateCacheConfiguration<Commons::TextTemplate>(Generics::Time::ONE_SECOND));

        for(auto bind_url_it = config_->BindURL().begin();
          bind_url_it != config_->BindURL().end(); ++bind_url_it)
        {
          BindURLRule_var bind_url_rule = new BindURLRule();
          bind_url_rule->url_template = new Commons::TextTemplate(
            bind_url_it->template_());
          bind_url_rule->use_keywords = bind_url_it->use_keywords();

          if(bind_url_it->use_keywords())
          {
            String::StringManip::Splitter<String::AsciiStringManip::SepNL> splitter(
              bind_url_it->keywords());
            String::SubString token;
            while(splitter.get_token(token))
            {
              String::StringManip::trim(token);
              if(!token.empty())
              {
                bind_url_rule->keywords.insert(Generics::StringHashAdapter(token));
              }
            }
          }

          bind_url_rules_.push_back(bind_url_rule);
        }

        track_template_file_ = config_->template_file();
        request_info_filler_.reset(
          new RequestInfoFiller(
            logger(),
            ip_map_,
            common_module_,
            common_config_->colo_id(),
            default_keys,
            account_keys,
            site_keys));
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << ex.what();
        throw Exception(ostr);
      }

      logger()->log(String::SubString(
          "Frontend::init(): frontend is running ..."),
        Logging::Logger::INFO,
        Aspect::IMPR_TRACK_FRONTEND);
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
      ostr << "Frontend::shutdown: frontend terminated (pid = "
        << ::getpid() << ").";

      logger()->log(ostr.str(),
        Logging::Logger::INFO,
        Aspect::IMPR_TRACK_FRONTEND);
    }
    catch(...)
    {
    }
  }

  FrontendCommons::RequestTask
  Frontend::co_handle_request(
    FCGI::HttpRequestHolder_var request_holder)
    noexcept
  {
    static const char* FUN = "ImprTrack::Frontend::handle_request()";
    co_await AdServer::Commons::ExecutorPool::yield(workers_);

    const FCGI::HttpRequest& request = request_holder->request();
    FCGI::HttpResponse_var response_ptr(new FCGI::HttpResponse());
    FCGI::HttpResponse& response = *response_ptr;

    logger()->log(String::SubString(
        "ImprTrack::Frontend::handle_request(): entered"),
      TraceLevel::MIDDLE,
      Aspect::IMPR_TRACK_FRONTEND);

    int http_status = 204;

    try
    {
      ImprTrackFrontendHTTPConstrain::apply(request);

      RequestInfo request_info;
      request_info_filler_->fill(request_info, request);

      if(logger()->log_level() >= TraceLevel::MIDDLE)
      {
        std::ostringstream ostr;
        ostr << FUN << ": " << std::endl <<
          "Uri: " << request.uri() << std::endl <<
          "Params ("<< request.params().size() << "):"  << std::endl;

        for(HTTP::ParamList::const_iterator it =
              request.params().begin(); it != request.params().end(); ++it)
        {
          ostr << "    " << it->name << " : " << it->value << std::endl;
        }

        ostr << "Headers ("<< request.headers().size() << "):"  << std::endl;

        for (HTTP::SubHeaderList::const_iterator it =
          request.headers().begin(); it != request.headers().end(); ++it)
        {
          ostr << "    " << it->name << " : " << it->value << std::endl;
        }

        logger()->log(ostr.str(),
          TraceLevel::MIDDLE,
          Aspect::IMPR_TRACK_FRONTEND);
      }

      AdServer::Commons::UserId result_user_id = request_info.actual_user_id;
      auto match_schedule_state = std::make_shared<MatchScheduleState>();
      match_schedule_state->request_info = request_info;

      if (!request_info.skip)
      {
        if (request_info.user_status == AdServer::CampaignSvcs::US_OPTOUT &&
          !request_info.actual_user_id.is_null())
        {
          throw InvalidParamException("");
        }

        // confirm impression for stats (CampaignManager)
        if(!request_info.request_ids.empty())
        {
          adserver::campaign_svcs::campaign_manager::ImpressionInfo
            verify_impression_info;
          verify_impression_info.set_verify_type(request_info.verify_type);
          verify_impression_info.set_time(GrpcAlgs::pack_time(request_info.time));
          verify_impression_info.set_bid_time(GrpcAlgs::pack_time(request_info.bid_time));
          verify_impression_info.set_pub_imp_revenue_type(
            request_info.pub_imp_revenue_type);
          verify_impression_info.mutable_pub_imp_revenue()->set_value(
            GrpcAlgs::pack_decimal(request_info.pub_imp_revenue));
          verify_impression_info.set_request_type(request_info.request_type);
          verify_impression_info.set_user_id(GrpcAlgs::pack_user_id(
            request_info.actual_user_id));
          verify_impression_info.set_referer(request_info.referer);
          verify_impression_info.set_viewability(request_info.viewability);
          verify_impression_info.set_action_name(request_info.action_name);

          if(request_info.user_id_hash_mod.present())
          {
            auto* user_id_hash_mod =
              verify_impression_info.mutable_user_id_hash_mod();
            user_id_hash_mod->set_defined(true);
            user_id_hash_mod->set_value(*request_info.user_id_hash_mod);
          }
          else
          {
            verify_impression_info.mutable_user_id_hash_mod()->set_defined(false);
          }

          RequestInfo::CreativeList::const_iterator cr_it =
            request_info.creatives.begin();
          for(RequestIdList::const_iterator rit = request_info.request_ids.begin();
            rit != request_info.request_ids.end();
            ++rit)
          {
            auto* cr_info = verify_impression_info.add_creatives();
            cr_info->set_request_id(GrpcAlgs::pack_request_id(*rit));
            // ccg_keyword_id non used now,
            // it can't define specific impression cost (only click)
            cr_info->set_ccg_keyword_id(0);
            if(cr_it != request_info.creatives.end())
            {
              cr_info->set_ccid(cr_it->ccid);
              cr_info->mutable_ctr()->set_value(
                GrpcAlgs::pack_decimal(cr_it->ctr));
              ++cr_it;
            }
            else
            {
              cr_info->set_ccid(0);
              cr_info->mutable_ctr()->set_value(
                GrpcAlgs::pack_decimal(CampaignSvcs::RevenueDecimal::ZERO));
            }
          }

          adserver::campaign_svcs::campaign_manager::VerifyImpressionRequest
            verify_impression_request;
          *verify_impression_request.mutable_impression_info() =
            verify_impression_info;
          co_verify_impression_(
            std::move(verify_impression_request),
            match_schedule_state).start_detached(nullptr);
        }
        else
        {
          match_schedule_state->verify_finished = true;
        }
        const ResolveUserBindResult bind_result =
          co_await co_resolve_user_bind_(request_info, result_user_id);

        http_status = finish_request_(
          request,
          response,
          request_info,
          bind_result.user_id,
          bind_result.invalid_bind_operation);

        match_schedule_state->result_user_id = bind_result.user_id;
        match_schedule_state->request_finished = true;
        try_schedule_match_channels_(match_schedule_state);

        co_return FrontendCommons::RequestResult{
          http_status,
          response_ptr,
          false};
      } // request_info.skip

      FrontendCommons::CORS::set_headers(request, response);

      if(!request_info.redirect_url.empty())
      {
        const std::string redirect_url_str = FrontendCommons::normalize_abs_url(
          HTTP::BrowserAddress(request_info.redirect_url),
          HTTP::HTTPAddress::VW_FULL,
          FrontendCommons::is_secure_request(request) || request_info.secure ?
            HTTPS_PREFIX : HTTP_PREFIX);

        http_status = FrontendCommons::redirect(redirect_url_str, response);
      }
      else if(request_info.verify_type != AdServer::CampaignSvcs::RVT_NOTICE)
        // don't try to make bind redirect on notice calls
      {
        // find templates that match traits
        TextTemplateArray inst_templates;

        {
          std::vector<std::string> keywords;
          FrontendCommons::get_ip_keywords(keywords, request_info.peer_ip);

          for(auto bind_rule_it = bind_url_rules_.begin(); bind_rule_it != bind_url_rules_.end(); ++bind_rule_it)
          {
            if((*bind_rule_it)->use_keywords)
            {
              for(auto keyword_it = keywords.begin(); keyword_it != keywords.end(); ++keyword_it)
              {
                if((*bind_rule_it)->keywords.find(*keyword_it) != (*bind_rule_it)->keywords.end())
                {
                  inst_templates.push_back((*bind_rule_it)->url_template);
                  break;
                }
              }
            }
            else
            {
              inst_templates.push_back((*bind_rule_it)->url_template);
            }
          }
        }

        std::vector<std::string> bind_urls;

        // prepare BINDURL tokens
        if(!inst_templates.empty())
        {
          char random_str[40];
          unsigned long random = Generics::safe_rand();
          String::StringManip::int_to_str(random, random_str, sizeof(random_str));

          typedef std::map<String::SubString, std::string> ArgMap;
          ArgMap sub_args_cont;
          sub_args_cont[Tokens::RANDOM] = random_str;
          sub_args_cont[Tokens::EXTERNAL_USER_ID] = request_info.external_user_id;
          sub_args_cont[Tokens::SOURCE_ID] = request_info.source_id;
          if(!request_info.current_user_id.is_null())
          {
            sub_args_cont[Tokens::ADD_USER_ID] = request_info.current_user_id.to_string();
          }

          String::TextTemplate::ArgsContainer<ArgMap> args(&sub_args_cont);
          String::TextTemplate::DefaultValue args_with_default(&args);
          String::TextTemplate::ArgsEncoder args_with_encoding(&args_with_default);

          for(auto bind_url_templ_it = inst_templates.begin();
            bind_url_templ_it != inst_templates.end();
            ++bind_url_templ_it)
          {
            bind_urls.push_back((*bind_url_templ_it)->instantiate(args_with_encoding));
          }
        }

        if(request_info.use_template_file)
        {
          try
          {
            // instantiate imp template
            Commons::TextTemplate_var templ = template_files_->get(track_template_file_);

            typedef std::map<String::SubString, std::string> ArgMap;

            ArgMap args_cont;

            String::TextTemplate::ArgsContainer<ArgMap> args(&args_cont);
            String::TextTemplate::DefaultValue args_with_default(&args);
            String::TextTemplate::ArgsEncoder args_with_encoding(&args_with_default);

            unsigned long i = 1;
            char i_str[40];
            for(auto bind_url_it = bind_urls.begin(); bind_url_it != bind_urls.end(); ++bind_url_it, ++i)
            {
              String::StringManip::int_to_str(i, i_str, sizeof(i_str));
              std::string token = Tokens::BIND_URL_TOKEN.str();
              token += i_str;
              args_cont[token] = *bind_url_it;
            }

            std::string response_content = templ->instantiate(args_with_encoding);

            response.set_content_type_nocopy(FrontendCommons::ContentType::TEXT_HTML);

            response.get_output_stream().write(
              response_content.data(), response_content.size());

            http_status = 200;
          }
          catch(const eh::Exception& ex)
          {
            logger()->sstream(
              Logging::Logger::EMERGENCY,
              Aspect::IMPR_TRACK_FRONTEND,
              "ADS-IMPL-?") <<
              FUN << ": eh::Exception has been caught: " << ex.what();

            http_status = 204;
          }
        }
        else if(!bind_urls.empty())
        {
          const std::string redirect_url_str = FrontendCommons::normalize_abs_url(
            HTTP::BrowserAddress(*bind_urls.begin()),
            HTTP::HTTPAddress::VW_FULL,
            FrontendCommons::is_secure_request(request) || request_info.secure ?
            HTTPS_PREFIX : HTTP_PREFIX);

          http_status = FrontendCommons::redirect(redirect_url_str, response);
        }
        else
        {
          response.set_content_type_nocopy(track_pixel_content_type_);

          if(common_config_->ResponseHeaders().present())
          {
            FrontendCommons::add_headers(
              *(common_config_->ResponseHeaders()),
              response);
          }

          FileCache::BufferHolder_var buffer = track_pixel_->get();
          response.get_output_stream().write((*buffer)->data(), (*buffer)->size());
        } // request_info.redirect_url.empty()
      }
    }
    catch (const ForbiddenException& ex)
    {
      http_status = 403;
      logger()->sstream(TraceLevel::LOW, Aspect::IMPR_TRACK_FRONTEND) <<
        FUN << ": ForbiddenException caught: " << ex.what();
    }
    catch (const InvalidParamException& ex)
    {
      http_status = 400;
      logger()->sstream(TraceLevel::MIDDLE, Aspect::IMPR_TRACK_FRONTEND) <<
        FUN << ": InvalidParamException caught: " << ex.what();
    }
    catch(const eh::Exception& e)
    {
      http_status = 500;
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << e.what();

      logger()->log(ostr.str(),
        Logging::Logger::EMERGENCY,
        Aspect::IMPR_TRACK_FRONTEND,
        "ADS-IMPL-134");
    }
    co_return FrontendCommons::RequestResult{
      http_status,
      response_ptr,
      false};
  }

  int
  Frontend::finish_request_(
    const FCGI::HttpRequest& request,
    FCGI::HttpResponse& response,
    const RequestInfo& request_info,
    const AdServer::Commons::UserId& result_user_id,
    bool invalid_bind_operation)
    noexcept
  {
    static const char* FUN = "ImprTrack::Frontend::finish_request_()";
    int http_status = 204;

    try
    {
      const AdServer::Commons::UserId freq_cap_user_id =
        !invalid_bind_operation &&
        !result_user_id.is_null() &&
          !(result_user_id == AdServer::Commons::PROBE_USER_ID) ?
        result_user_id :
        request_info.current_user_id;

      if((!request_info.common_request_id.is_null() ||
          !request_info.pubpixel_accounts.empty()) &&
         !freq_cap_user_id.is_null() &&
         user_info_client_coro_)
      {
        try
        {
          adserver::user_info_svcs::user_info_manager::
            ConfirmUserFreqCapsRequest confirm_request;
          confirm_request.set_user_id(GrpcAlgs::pack_user_id(
            freq_cap_user_id));
          confirm_request.set_time(GrpcAlgs::pack_time(request_info.time));
          confirm_request.set_request_id(GrpcAlgs::pack_request_id(
            request_info.common_request_id));
          for(const auto account_id : request_info.pubpixel_accounts)
          {
            confirm_request.add_exclude_pubpixel_accounts(account_id);
          }

          co_confirm_user_freq_caps_(std::move(confirm_request)).
            start_detached(nullptr);
        }
        catch(const eh::Exception& e)
        {
          Stream::Error ostr;
          ostr << FUN << ": confirm_user_freq_caps preparation failed: " <<
            e.what();

          logger()->log(ostr.str(),
            Logging::Logger::EMERGENCY,
            Aspect::IMPR_TRACK_FRONTEND,
            "ADS-IMPL-123");
        }
      }

      if(request_info.set_cookie &&
        !invalid_bind_operation &&
        !result_user_id.is_null())
      {
        const Generics::SignedUuid signed_uid =
          common_module_->user_id_controller()->sign(result_user_id);
        FrontendCommons::add_UID_cookie(
          response,
          request,
          *cookie_manager_,
          signed_uid.str());
      }
      FrontendCommons::CORS::set_headers(request, response);

      if(!request_info.redirect_url.empty())
      {
        const std::string redirect_url_str = FrontendCommons::normalize_abs_url(
          HTTP::BrowserAddress(request_info.redirect_url),
          HTTP::HTTPAddress::VW_FULL,
          FrontendCommons::is_secure_request(request) || request_info.secure ?
            HTTPS_PREFIX : HTTP_PREFIX);

        http_status = FrontendCommons::redirect(redirect_url_str, response);
      }
      else if(request_info.verify_type != AdServer::CampaignSvcs::RVT_NOTICE)
      {
        TextTemplateArray inst_templates;

        {
          std::vector<std::string> keywords;
          FrontendCommons::get_ip_keywords(keywords, request_info.peer_ip);

          for(auto bind_rule_it = bind_url_rules_.begin();
            bind_rule_it != bind_url_rules_.end();
            ++bind_rule_it)
          {
            if((*bind_rule_it)->use_keywords)
            {
              for(auto keyword_it = keywords.begin();
                keyword_it != keywords.end();
                ++keyword_it)
              {
                if((*bind_rule_it)->keywords.find(*keyword_it) !=
                  (*bind_rule_it)->keywords.end())
                {
                  inst_templates.push_back((*bind_rule_it)->url_template);
                  break;
                }
              }
            }
            else
            {
              inst_templates.push_back((*bind_rule_it)->url_template);
            }
          }
        }

        std::vector<std::string> bind_urls;

        if(!inst_templates.empty())
        {
          char random_str[40];
          const unsigned long random = Generics::safe_rand();
          String::StringManip::int_to_str(
            random,
            random_str,
            sizeof(random_str));

          typedef std::map<String::SubString, std::string> ArgMap;
          ArgMap sub_args_cont;
          sub_args_cont[Tokens::RANDOM] = random_str;
          sub_args_cont[Tokens::EXTERNAL_USER_ID] =
            request_info.external_user_id;
          sub_args_cont[Tokens::SOURCE_ID] = request_info.source_id;
          if(!request_info.current_user_id.is_null())
          {
            sub_args_cont[Tokens::ADD_USER_ID] =
              request_info.current_user_id.to_string();
          }

          String::TextTemplate::ArgsContainer<ArgMap> args(&sub_args_cont);
          String::TextTemplate::DefaultValue args_with_default(&args);
          String::TextTemplate::ArgsEncoder args_with_encoding(
            &args_with_default);

          for(auto bind_url_templ_it = inst_templates.begin();
            bind_url_templ_it != inst_templates.end();
            ++bind_url_templ_it)
          {
            bind_urls.push_back(
              (*bind_url_templ_it)->instantiate(args_with_encoding));
          }
        }

        if(request_info.use_template_file)
        {
          try
          {
            Commons::TextTemplate_var templ =
              template_files_->get(track_template_file_);

            typedef std::map<String::SubString, std::string> ArgMap;
            ArgMap args_cont;

            String::TextTemplate::ArgsContainer<ArgMap> args(&args_cont);
            String::TextTemplate::DefaultValue args_with_default(&args);
            String::TextTemplate::ArgsEncoder args_with_encoding(
              &args_with_default);

            unsigned long i = 1;
            char i_str[40];
            for(auto bind_url_it = bind_urls.begin();
              bind_url_it != bind_urls.end();
              ++bind_url_it, ++i)
            {
              String::StringManip::int_to_str(i, i_str, sizeof(i_str));
              std::string token = Tokens::BIND_URL_TOKEN.str();
              token += i_str;
              args_cont[token] = *bind_url_it;
            }

            const std::string response_content =
              templ->instantiate(args_with_encoding);

            response.set_content_type_nocopy(
              FrontendCommons::ContentType::TEXT_HTML);
            response.get_output_stream().write(
              response_content.data(),
              response_content.size());

            http_status = 200;
          }
          catch(const eh::Exception& ex)
          {
            logger()->sstream(
              Logging::Logger::EMERGENCY,
              Aspect::IMPR_TRACK_FRONTEND,
              "ADS-IMPL-?") <<
              FUN << ": eh::Exception has been caught: " << ex.what();

            http_status = 204;
          }
        }
        else if(!bind_urls.empty())
        {
          const std::string redirect_url_str = FrontendCommons::normalize_abs_url(
            HTTP::BrowserAddress(*bind_urls.begin()),
            HTTP::HTTPAddress::VW_FULL,
            FrontendCommons::is_secure_request(request) || request_info.secure ?
              HTTPS_PREFIX : HTTP_PREFIX);

          http_status = FrontendCommons::redirect(redirect_url_str, response);
        }
        else
        {
          response.set_content_type_nocopy(track_pixel_content_type_);

          if(common_config_->ResponseHeaders().present())
          {
            FrontendCommons::add_headers(
              *(common_config_->ResponseHeaders()),
              response);
          }

          FileCache::BufferHolder_var buffer = track_pixel_->get();
          response.get_output_stream().write(
            (*buffer)->data(),
            (*buffer)->size());
        }
      }
    }
    catch(const eh::Exception& e)
    {
      http_status = 500;
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << e.what();

      logger()->log(ostr.str(),
        Logging::Logger::EMERGENCY,
        Aspect::IMPR_TRACK_FRONTEND,
        "ADS-IMPL-134");
    }
    return http_status;
  }

  Frontend::ResolveUserBindTask
  Frontend::co_resolve_user_bind_(
    const RequestInfo& request_info,
    const AdServer::Commons::UserId& input_user_id)
    noexcept
  {
    ResolveUserBindResult result;
    result.user_id = input_user_id;

    if(request_info.user_status == AdServer::CampaignSvcs::US_OPTOUT)
    {
      result.user_id = AdServer::Commons::UserId();
      co_return result;
    }

    if(!user_bind_client_coro_)
    {
      co_return result;
    }

    if(!result.user_id.is_null())
    {
      adserver::user_info_svcs::user_bind::GetUserIdRequest get_request;
      get_request.set_id(std::string("c/") + result.user_id.to_string());
      get_request.set_timestamp(GrpcAlgs::pack_time(request_info.time));
      get_request.set_silent(true);
      get_request.set_generate_user_id(false);
      get_request.set_for_set_cookie(request_info.set_cookie);
      get_request.set_create_timestamp(
        GrpcAlgs::pack_time(Generics::Time::ZERO));
      get_request.set_current_user_id(GrpcAlgs::pack_user_id(result.user_id));

      auto get_result = co_await user_bind_client_coro_->get_user_id(
        std::move(get_request));
      if(get_result.status.ok())
      {
        if(get_result.response.invalid_operation())
        {
          result.invalid_bind_operation = true;
          report_bad_user_(request_info);
          co_return result;
        }

        const AdServer::Commons::UserId resolved_user_id =
          GrpcAlgs::unpack_user_id(get_result.response.user_id());
        if(!resolved_user_id.is_null())
        {
          result.user_id = resolved_user_id;
        }
      }

      if(request_info.current_user_id == result.user_id ||
        request_info.external_user_id.empty())
      {
        co_return result;
      }
    }
    else if(request_info.external_user_id.empty())
    {
      co_return result;
    }

    if(!result.user_id.is_null())
    {
      adserver::user_info_svcs::user_bind::AddUserIdRequest add_user_request;
      add_user_request.set_id(request_info.external_user_id);
      add_user_request.set_user_id(GrpcAlgs::pack_user_id(result.user_id));
      add_user_request.set_timestamp(GrpcAlgs::pack_time(request_info.time));

      auto add_result = co_await user_bind_client_coro_->add_user_id(
        std::move(add_user_request));
      if(!add_result.status.ok())
      {
        co_return result;
      }

      if(add_result.response.invalid_operation())
      {
        result.invalid_bind_operation = true;
        report_bad_user_(request_info);
      }

      co_return result;
    }

    adserver::user_info_svcs::user_bind::GetUserIdRequest get_request;
    get_request.set_id(request_info.external_user_id);
    get_request.set_timestamp(GrpcAlgs::pack_time(request_info.time));
    get_request.set_silent(true);
    get_request.set_generate_user_id(false);
    get_request.set_for_set_cookie(request_info.set_cookie);
    get_request.set_create_timestamp(GrpcAlgs::pack_time(Generics::Time::ZERO));

    auto get_result = co_await user_bind_client_coro_->get_user_id(
      std::move(get_request));
    if(!get_result.status.ok())
    {
      co_return result;
    }

    if(get_result.response.invalid_operation())
    {
      result.invalid_bind_operation = true;
      report_bad_user_(request_info);
    }
    else
    {
      const AdServer::Commons::UserId resolved_user_id =
        GrpcAlgs::unpack_user_id(get_result.response.user_id());
      if(!resolved_user_id.is_null())
      {
        result.user_id = resolved_user_id;
        common_module_->user_id_controller()->null_blacklisted(result.user_id);
      }
    }

    co_return result;
  }

  FrontendCommons::RequestTask
  Frontend::co_verify_impression_(
    adserver::campaign_svcs::campaign_manager::VerifyImpressionRequest request,
    std::shared_ptr<MatchScheduleState> match_schedule_state)
    noexcept
  {
    if(campaign_manager_coro_)
    {
      auto result = co_await campaign_manager_coro_->verify_impression(
        std::move(request));
      if(!result.status.ok())
      {
        Stream::Error ostr;
        ostr << "CampaignManager::verify_impression(): "
          "gRPC call failed: code=" <<
          static_cast<int>(result.status.error_code()) <<
          ", message=" << result.status.error_message();
        logger()->log(
          ostr.str(),
          Logging::Logger::ERROR,
          Aspect::IMPR_TRACK_FRONTEND,
          "ADS-IMPL-134");
      }
      else
      {
        match_schedule_state->impression_result_info =
          std::move(result.response.impression_result_info());
      }
    }

    match_schedule_state->verify_finished = true;
    try_schedule_match_channels_(match_schedule_state);
    co_return FrontendCommons::RequestResult{};
  }

  FrontendCommons::RequestTask
  Frontend::co_confirm_user_freq_caps_(
    adserver::user_info_svcs::user_info_manager::ConfirmUserFreqCapsRequest
      request)
    noexcept
  {
    if(user_info_client_coro_)
    {
      auto result = co_await user_info_client_coro_->confirm_user_freq_caps(
        std::move(request));
      if(!result.status.ok())
      {
        Stream::Error ostr;
        ostr << "UserInfoManagerGrpc::confirm_user_freq_caps(): "
          "gRPC call failed: code=" <<
          static_cast<int>(result.status.error_code()) <<
          ", message=" << result.status.error_message();
        logger()->log(
          ostr.str(),
          result.status.error_code() == grpc::StatusCode::UNAVAILABLE ?
            Logging::Logger::WARNING :
            Logging::Logger::EMERGENCY,
          Aspect::IMPR_TRACK_FRONTEND,
          result.status.error_code() == grpc::StatusCode::UNAVAILABLE ?
            "" : "ADS-IMPL-123");
      }
    }

    co_return FrontendCommons::RequestResult{};
  }

  FrontendCommons::RequestTask
  Frontend::co_consider_web_operation_(
    adserver::campaign_svcs::campaign_manager::ConsiderWebOperationRequest
      request)
    noexcept
  {
    if(campaign_manager_coro_)
    {
      auto result = co_await campaign_manager_coro_->consider_web_operation(
        std::move(request));
      if(!result.status.ok() &&
        result.status.error_code() != grpc::StatusCode::INVALID_ARGUMENT)
      {
        Stream::Error ostr;
        ostr << "CampaignManager::consider_web_operation(): "
          "gRPC call failed: code=" <<
          static_cast<int>(result.status.error_code()) <<
          ", message=" << result.status.error_message();
        logger()->log(
          ostr.str(),
          Logging::Logger::ERROR,
          Aspect::IMPR_TRACK_FRONTEND,
          "ADS-IMPL-134");
      }
    }

    co_return FrontendCommons::RequestResult{};
  }

  void
  Frontend::try_schedule_match_channels_(
    const std::shared_ptr<MatchScheduleState>& state)
    noexcept
  {
    if(!state->request_finished ||
      !state->verify_finished ||
      state->scheduled)
    {
      return;
    }

    if(!match_workers_ ||
      state->impression_result_info.creatives().empty())
    {
      state->scheduled = true;
      return;
    }

    const AdServer::Commons::UserId match_user_id =
      !state->result_user_id.is_null() &&
        !(state->result_user_id == AdServer::Commons::PROBE_USER_ID) ?
      state->result_user_id :
      state->request_info.current_user_id;

    if(match_user_id.is_null() ||
      match_user_id == AdServer::Commons::PROBE_USER_ID ||
      !channel_client_coro_ ||
      !campaign_manager_coro_)
    {
      state->scheduled = true;
      return;
    }

    const unsigned long limit = config_->match_task_limit();
    unsigned long current = match_tasks_count_.load(std::memory_order_relaxed);
    while(limit == 0 || current < limit)
    {
      if(match_tasks_count_.compare_exchange_weak(
          current,
          current + 1,
          std::memory_order_relaxed,
          std::memory_order_relaxed))
      {
        break;
      }
    }

    if(limit != 0 && current >= limit)
    {
      state->scheduled = true;
      logger()->log(
        String::SubString("ImprTrack match task limit reached."),
        Logging::Logger::WARNING,
        Aspect::IMPR_TRACK_FRONTEND,
        "ADS-IMPL-198");
      return;
    }

    state->scheduled = true;
    auto task_state = std::make_shared<ImprTrackMatchRequestState>(this);
    task_state->request_info = state->request_info;
    task_state->user_id = match_user_id;
    task_state->cookie_user_id = state->request_info.current_user_id;
    task_state->resolved_cookie_user_id = state->request_info.current_user_id;

    for(const auto& creative :
      state->impression_result_info.creatives())
    {
      if(creative.campaign_id())
      {
        task_state->campaign_ids.push_back(creative.campaign_id());
      }

      if(creative.advertiser_id())
      {
        task_state->advertiser_ids.push_back(creative.advertiser_id());
      }
    }

    co_match_request_(task_state).start_detached(nullptr);
  }

  FrontendCommons::RequestTask
  Frontend::co_match_request_(
    std::shared_ptr<ImprTrackMatchRequestState> state)
    noexcept
  {
    adserver::channel_svcs::channel_server::MatchRequest channel_request;
    channel_request.set_non_strict_word_match(false);
    channel_request.set_non_strict_url_match(false);
    channel_request.set_return_negative(false);
    channel_request.set_simplify_page(false);
    channel_request.set_fill_content(false);
    channel_request.set_statuses("A", 2);

    std::ostringstream keywords_ostr;
    keywords_ostr << "poadimp";
    for(const auto campaign_id : state->campaign_ids)
    {
      keywords_ostr << " poadimpc" << campaign_id;
    }

    for(const auto advertiser_id : state->advertiser_ids)
    {
      keywords_ostr << " poadimpa" << advertiser_id;
    }

    channel_request.set_pwords(keywords_ostr.str());

    auto channel_result = co_await channel_client_coro_->match(
      std::move(channel_request));
    if(!channel_result.status.ok())
    {
      Stream::Error ostr;
      ostr << "ChannelServer::match(): gRPC call failed: code=" <<
        static_cast<int>(channel_result.status.error_code()) <<
        ", message=" << channel_result.status.error_message();
      logger()->log(
        ostr.str(),
        Logging::Logger::EMERGENCY,
        Aspect::IMPR_TRACK_FRONTEND,
        "ADS-IMPL-117");
      finish_match_channels_request_();
      co_return FrontendCommons::RequestResult{};
    }

    state->trigger_match_result = std::move(channel_result.response);

    if(user_bind_client_coro_ &&
      !state->cookie_user_id.is_null() &&
      state->user_id != state->cookie_user_id)
    {
      adserver::user_info_svcs::user_bind::GetUserIdRequest get_request;
      get_request.set_id(std::string("c/") + state->cookie_user_id.to_string());
      get_request.set_timestamp(GrpcAlgs::pack_time(state->request_info.time));
      get_request.set_silent(true);
      get_request.set_generate_user_id(false);
      get_request.set_for_set_cookie(false);
      get_request.set_create_timestamp(
        GrpcAlgs::pack_time(Generics::Time::ZERO));
      get_request.set_current_user_id(
        GrpcAlgs::pack_user_id(state->cookie_user_id));

      auto get_result = co_await user_bind_client_coro_->get_user_id(
        std::move(get_request));
      if(get_result.status.ok() && !get_result.response.invalid_operation())
      {
        const AdServer::Commons::UserId resolved_user_id =
          GrpcAlgs::unpack_user_id(get_result.response.user_id());
        if(!resolved_user_id.is_null())
        {
          state->resolved_cookie_user_id = resolved_user_id;
        }
      }
    }

    const auto& matched_channels =
      state->trigger_match_result.matched_channels();
    if(matched_channels.page_channels_size() == 0)
    {
      finish_match_channels_request_();
      co_return FrontendCommons::RequestResult{};
    }

    using ChannelMatch = std::pair<unsigned long, unsigned long>;
    std::set<ChannelMatch> page_channels;
    for(const auto& channel : matched_channels.page_channels())
    {
      page_channels.emplace(channel.id(), channel.trigger_channel_id());
    }

    auto build_history_match_request = [this, &page_channels, state](
      const AdServer::Commons::UserId& match_user_id)
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

      auto* page_channel_ids = match_params->mutable_page_channel_ids();
      for(const auto& channel : page_channels)
      {
        auto* result = page_channel_ids->Add();
        result->set_channel_id(channel.first);
        result->set_channel_trigger_id(channel.second);
      }

      auto* user_info = request.mutable_user_info();
      user_info->set_user_id(GrpcAlgs::pack_user_id(match_user_id));
      user_info->set_last_colo_id(-1);
      user_info->set_request_colo_id(common_config_->colo_id());
      user_info->set_current_colo_id(-1);
      user_info->set_temporary(false);
      user_info->set_time(state->request_info.time.tv_sec);
      return request;
    };

    if(user_info_client_coro_)
    {
      if(state->user_id != AdServer::Commons::PROBE_USER_ID)
      {
        auto history_match_request = build_history_match_request(state->user_id);
        auto match_result = co_await user_info_client_coro_->match(
          std::move(history_match_request));
        if(match_result.status.ok())
        {
          state->history_match_response = std::move(match_result.response);
          state->history_match_present = state->history_match_response.matched();
        }
        else
        {
          Stream::Error ostr;
          ostr << "UserInfoManager::match(): gRPC call failed: code=" <<
            static_cast<int>(match_result.status.error_code()) <<
            ", message=" << match_result.status.error_message();
          logger()->log(
            ostr.str(),
            match_result.status.error_code() == grpc::StatusCode::UNAVAILABLE ?
              Logging::Logger::WARNING :
              Logging::Logger::EMERGENCY,
            Aspect::IMPR_TRACK_FRONTEND,
            match_result.status.error_code() == grpc::StatusCode::UNAVAILABLE ?
              "" : "ADS-IMPL-112");
        }
      }

      if(state->user_id != state->resolved_cookie_user_id &&
        !state->resolved_cookie_user_id.is_null())
      {
        auto cookie_match_request = build_history_match_request(
          state->resolved_cookie_user_id);
        co_await user_info_client_coro_->match(std::move(cookie_match_request));
      }
    }

    adserver::campaign_svcs::campaign_manager::ProcessMatchRequestRequest request;
    fill_match_request_info_(
      *request.mutable_match_request_info(),
      *state);
    auto process_result = co_await campaign_manager_coro_->process_match_request(
      std::move(request));
    if(!process_result.status.ok())
    {
      Stream::Error ostr;
      ostr << "CampaignManager::process_match_request(): "
        "gRPC call failed: code=" <<
        static_cast<int>(process_result.status.error_code()) <<
        ", message=" << process_result.status.error_message();
      logger()->log(
        ostr.str(),
        Logging::Logger::EMERGENCY,
        Aspect::IMPR_TRACK_FRONTEND,
        "ADS-ICON-4");
    }

    finish_match_channels_request_();
    co_return FrontendCommons::RequestResult{};
  }

  void
  Frontend::fill_match_request_info_(
    adserver::campaign_svcs::campaign_manager::MatchRequestInfo& mri,
    const ImprTrackMatchRequestState& state)
    const noexcept
  {
    auto* match_info = mri.mutable_match_info();
    match_info->set_colo_id(common_config_->colo_id());
    mri.set_user_id(GrpcAlgs::pack_user_id(state.user_id));
    mri.set_request_time(GrpcAlgs::pack_time(state.request_info.time));

    const auto& page_channels =
      state.trigger_match_result.matched_channels().page_channels();
    for(const auto& channel : page_channels)
    {
      auto* pkw_channel = match_info->add_pkw_channels();
      pkw_channel->set_channel_id(channel.id());
      pkw_channel->set_channel_trigger_id(channel.trigger_channel_id());
    }

    if(state.history_match_present)
    {
      for(const auto& channel :
        state.history_match_response.match_result().channels())
      {
        match_info->add_channels(channel.channel_id());
      }
    }

    if(!state.request_info.peer_ip.empty() && ip_map_.get())
    {
      try
      {
        GeoIPMapping::IPMapCity2::CityLocation geo_location;
        if(ip_map_->city_location_by_addr(
             state.request_info.peer_ip.c_str(),
             geo_location,
             false))
        {
          FrontendCommons::Location_var location =
            std::make_shared<FrontendCommons::Location>();
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
  Frontend::finish_match_channels_request_()
    noexcept
  {
    match_tasks_count_.fetch_sub(1, std::memory_order_relaxed);
  }

  RequestInfoFiller::EncryptionKeys_var
  Frontend::read_keys_(
    const xsd::AdServer::Configuration::EncryptionKeysType& src)
    /*throw(eh::Exception)*/
  {
    RequestInfoFiller::EncryptionKeys_var res = new RequestInfoFiller::EncryptionKeys();

    if(src.openx_encryption_key().present())
    {
      res->openx_encryption_key_size = String::StringManip::hex_decode(
        *src.openx_encryption_key(), res->openx_encryption_key);
    }

    if(src.openx_integrity_key().present())
    {
      res->openx_integrity_key_size = String::StringManip::hex_decode(
        *src.openx_integrity_key(), res->openx_integrity_key);
    }

    if(src.google_encryption_key().present())
    {
      res->google_encryption_key_size = String::StringManip::hex_decode(
        *src.google_encryption_key(), res->google_encryption_key);
    }

    if(src.google_integrity_key().present())
    {
      res->google_integrity_key_size = String::StringManip::hex_decode(
        *src.google_integrity_key(), res->google_integrity_key);
    }

    return res;
  }

  void
  Frontend::report_bad_user_(
    const RequestInfo& request_info)
    noexcept
  {
    try
    {
      adserver::campaign_svcs::campaign_manager::ConsiderWebOperationRequest
        web_op;
      web_op.set_time(GrpcAlgs::pack_time(request_info.time));
      web_op.set_colo_id(request_info.colo_id);
      web_op.set_tag_id(0);
      web_op.set_cc_id(0);
      web_op.set_curct(request_info.external_user_id);
      web_op.set_app(WebStats::APPLICATION.str());
      web_op.set_source(WebStats::SOURCE.str());
      web_op.set_operation(WebStats::INVALID_MAPPING_OPERATION.str());
      web_op.set_user_bind_src(request_info.source_id);
      web_op.set_result('F');
      web_op.set_user_status(static_cast<unsigned long>(request_info.user_status));
      web_op.set_test_request(false);

      co_consider_web_operation_(std::move(web_op)).start_detached(nullptr);
    }
    catch(const eh::Exception&)
    {}
  }

}
