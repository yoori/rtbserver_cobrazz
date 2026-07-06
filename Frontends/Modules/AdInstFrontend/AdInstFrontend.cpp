#include <sstream>
#include <algorithm>
#include <coroutine>
#include <set>

#include <Logger/StreamLogger.hpp>
#include <HTTP/HTTPCookie.hpp>
#include <String/StringManip.hpp>
#include <String/AsciiStringManip.hpp>

#include <Generics/Uuid.hpp>
#include <Generics/GnuHashTable.hpp>
#include <Generics/HashTableAdapters.hpp>

#include <Commons/UserInfoManip.hpp>
#include <Commons/Algs.hpp>

#include <Commons/ErrorHandler.hpp>
#include <Commons/GrpcAlgs.hpp>
#include <LogCommons/LogCommons.hpp>

#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>

#include <Frontends/FrontendCommons/OptOutManip.hpp>

#include <Frontends/FrontendCommons/UserInfoClientConfig.hpp>

#include "AdInstFrontend.hpp"

namespace
{
  DECLARE_EXCEPTION(GrpcCallException, eh::DescriptiveException);

  const String::AsciiStringManip::Caseless
    MIME_ENCODED_HTTP_PREFIX("http%3a%2f%2f");
  const String::AsciiStringManip::Caseless
    MIME_ENCODED_HTTPS_PREFIX("https%3a%2f%2f");
  const String::AsciiStringManip::Caseless
    MIME_ENCODED_SHEME_RELATIVE_PREFIX("%2f%2f");

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

}

namespace Aspect
{
  const char AD_INST_FRONTEND[] = "Instantiate::Frontend";
}

namespace Request::Context
{
    const String::SubString PRECLICK_URL("preclick");
  }

namespace Request::Cookie
  {
    const Generics::SubStringHashAdapter LAST_COLOCATION_ID(String::SubString("lc"));
  }

namespace Response::Header
{
    const String::SubString MERGE_FAILED("x-Merge-Failed");
    const String::SubString LOCATION("Location");
    const String::SubString DEBUG_INFO("Debug-Info");
  }

namespace AdServer::Instantiate
{
  /**
   *  Adinstfrontend implementation
   */
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
            frontend_config->get().AdInstFeConfiguration()->Logger().log_level())),
        "Frontend",
        Aspect::AD_INST_FRONTEND,
        0),
      frontend_config_(ReferenceCounting::add_ref(frontend_config)),
      common_module_(ReferenceCounting::add_ref(common_module)),
      workers_(std::move(request_workers))
  {}

  /** Frontend::will_handle */
  bool
  Frontend::will_handle(const String::SubString& uri) noexcept
  {
    std::string found_uri;

    bool result = FrontendCommons::find_uri(
      config_->UriList().Uri(), uri, found_uri);

    if(logger()->log_level() >= TraceLevel::MIDDLE)
    {
      Stream::Error ostr;
      ostr << "Instantiate::Frontend::will_handle(" << uri <<
        "), service: '" << found_uri << "'";

      logger()->log(ostr.str());
    }

    return result;
  }

  void Frontend::parse_configs_() /*throw(Exception)*/
  {
    static const char* FUN = "Instantiate::Frontend::parse_configs_()";

    /* load common configuration */
    try
    {
      typedef Configuration::FeConfig Config;
      const Config& fe_config = frontend_config_->get();

      if(!fe_config.CommonFeConfiguration().present())
      {
        throw Exception("CommonFeConfiguration isn't present");
      }

      common_config_ = CommonConfigPtr(
        new CommonFeConfiguration(*fe_config.CommonFeConfiguration()));

      if(!fe_config.AdInstFeConfiguration().present())
      {
        throw Exception("AdInstFeConfiguration isn't present");
      }

      config_.reset(
        new AdInstFeConfiguration(*fe_config.AdInstFeConfiguration()));

      cookie_manager_.reset(
        new FrontendCommons::CookieManager<
          FCGI::HttpRequest, FCGI::HttpResponse>(
            common_config_->Cookies()));
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't parse config file: " << e.what();
      throw Exception(ostr);
    }
  }

  /** Frontend::init */
  void
  Frontend::init() /*throw(eh::Exception)*/
  {
    static const char* FUN = "Instantiate::Frontend::init()";

    if(true) // module_used())
    {
      try
      {
        parse_configs_();
        grpc_executor_ = common_module_->grpc_executor();

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

        request_info_filler_.reset(
          new RequestInfoFiller(
            logger(),
            common_module_,
            common_module_->ip_mapper(),
            common_config_->IpEncryptConfig().present() ?
              String::SubString(common_config_->IpEncryptConfig()->key()) :
            String::SubString()));
      }
      catch (const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << ex.what();
        throw Exception(ostr);
      }

      logger()->log(String::SubString(
          "Frontend::init(): frontend is running ..."),
        Logging::Logger::INFO,
        Aspect::AD_INST_FRONTEND);
    }
  }

  FrontendCommons::RequestTask
  Frontend::co_handle_request(
    FCGI::HttpRequestHolder_var request_holder)
    noexcept
  {
    co_await AdServer::Commons::ExecutorPool::yield(workers_);
    auto result = co_await co_process_request_(std::move(request_holder));
    co_return std::move(result);
  }

  FrontendCommons::RequestTask
  Frontend::co_handle_request_noparams(
    FCGI::HttpRequestHolder_var request_holder)
    noexcept
  {
    co_await AdServer::Commons::ExecutorPool::yield(workers_);
    auto result = co_await handle_request_noparams_(std::move(request_holder));
    co_return std::move(result);
  }

  /** Frontend::shutdown */
  void
  Frontend::shutdown() noexcept
  {
    try
    {
      deactivate_object();
      wait_object();

      Stream::Error ostr;
      ostr << "Frontend::shutdown: frontend terminated (pid = " <<
        ::getpid() << ").";

      logger()->log(ostr.str(),
        Logging::Logger::INFO,
        Aspect::AD_INST_FRONTEND);
    }
    catch(...)
    {}
  }

  void
  Frontend::log_request_(
    const char* function_name,
    const FCGI::HttpRequest& request,
    unsigned int log_level_val)
    /*throw(eh::Exception)*/
  {
    if(logger()->log_level() >= log_level_val)
    {
      std::ostringstream ostr;

      ostr << function_name << ":" << std::endl <<
        "Args: " << request.args() << std::endl <<
        "Params ("<< request.params().size() << "):"  << std::endl;

      for(HTTP::ParamList::const_iterator it =
            request.params().begin();
          it != request.params().end(); ++it)
      {
        ostr << "    " << it->name << " : " << it->value << std::endl;
      }

      ostr << "Headers ("<< request.headers().size() << "):"  << std::endl;

      for (HTTP::SubHeaderList::const_iterator it =
        request.headers().begin(); it != request.headers().end(); ++it)
      {
        ostr << "    " << it->name << " : " << it->value << std::endl;
      }

      ostr << "    " << "Header_only : " << request.header_only() << std::endl;

      logger()->log(ostr.str(),
        log_level_val,
        Aspect::AD_INST_FRONTEND);
    }
  }

  FrontendCommons::RequestTask
  Frontend::handle_request_noparams_(
    FCGI::HttpRequestHolder_var request_holder)
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

          if (param.name == Request::Context::PRECLICK_URL)
          {
            // preclick is a last parameter since 3.3.0
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
    auto result = co_await co_process_request_(std::move(request_holder));
    co_return std::move(result);
  }

  /** Frontend::handle_request */
  FrontendCommons::RequestTask
  Frontend::co_process_request_(
    FCGI::HttpRequestHolder_var request_holder)
    noexcept
  {
    static const char* FUN = "Frontend::handle_request()";

    const FCGI::HttpRequest& request = request_holder->request();

    FCGI::HttpResponse_var response_ptr(new FCGI::HttpResponse());

    if(logger()->log_level() >= TraceLevel::MIDDLE)
    {
      logger()->log(String::SubString("Frontend::handle_request(): entered"),
        TraceLevel::MIDDLE,
        Aspect::AD_INST_FRONTEND);
    }

    int http_status = 200;
    RequestInfo request_info;

    try
    {
      log_request_("Frontend::handle_request()", request, TraceLevel::MIDDLE);

      request_info_filler_->fill(request_info, request);

      Generics::SubStringHashAdapter instantiate_type =
        FrontendCommons::deduce_instantiate_type(&request_info.secure, request);

      auto finish_response =
        [this,
          request_holder,
          response_ptr,
          request_info](
            int status,
            bool merge_success,
            const std::string& merge_error_message) mutable
        {
          FCGI::HttpResponse& response = *response_ptr;
          const FCGI::HttpRequest& request = request_holder->request();

          try
          {
            HTTP::CookieList cookies;
            cookies.load_from_headers(request.headers());

            if(!merge_success)
            {
              response.add_header_nocopy_name(
                Response::Header::MERGE_FAILED,
                merge_error_message);
            }

            if(common_config_->ResponseHeaders().present())
            {
              FrontendCommons::add_headers(
                *(common_config_->ResponseHeaders()),
                response);
            }

            if(request_info.set_uid && request_info.user_id.is_null())
            {
              const Generics::SignedUuid signed_uid =
                common_module_->user_id_controller()->generate();
              cookie_manager_->set(
                response,
                request,
                FrontendCommons::Cookies::CLIENT_ID,
                signed_uid.str());
            }

            if(request_info.format == "vast")
            {
              FrontendCommons::CORS::set_headers(request, response);
            }
          }
          catch(const HTTP::CookieList::Exception& e)
          {
            status = 400;

            Stream::Error ostr;
            ostr << FUN << ": HTTP::CookieList::Exception caught: " <<
              e.what();

            logger()->log(ostr.str(),
              Logging::Logger::NOTICE,
              Aspect::AD_INST_FRONTEND);
          }
          catch(const eh::Exception& e)
          {
            status = 500;

            Stream::Error ostr;
            ostr << FUN << ": eh::Exception caught: " << e.what();

            logger()->log(ostr.str(),
              Logging::Logger::EMERGENCY,
              Aspect::AD_INST_FRONTEND,
              "ADS-IMPL-109");
          }

          return status;
        };

      const MergeUsersResult merge_result =
        co_await co_merge_users_(request_info);

      http_status = co_await co_instantiate_ad_(
        response_ptr,
        request_info,
        instantiate_type);

      http_status = finish_response(
        http_status,
        merge_result.success,
        merge_result.error_message);
    }
    catch (const ForbiddenException &ex)
    {
      /* forbidden request */
      http_status = 403;

      if(logger()->log_level() >= TraceLevel::LOW)
      {
        Stream::Error ostr;
        ostr << FUN << ": ForbiddenException caught: " << ex.what();

        logger()->log(ostr.str(),
          TraceLevel::LOW,
          Aspect::AD_INST_FRONTEND);
      }
    }
    catch (const InvalidParamException& e)
    {
      /* non correct or passback request */
      http_status = 400;

      if(logger()->log_level() >= TraceLevel::MIDDLE)
      {
        Stream::Error ostr;
        ostr << FUN << ": InvalidParamException caught: " << e.what();

        if(logger()->log_level() >= TraceLevel::MIDDLE)
        {
          logger()->log(ostr.str(),
            TraceLevel::MIDDLE,
            Aspect::AD_INST_FRONTEND);
        }
      }
    }
    catch(const HTTP::CookieList::Exception& e)
    {
      http_status = 400;

      Stream::Error ostr;
      ostr << FUN << ": HTTP::CookieList::Exception caught: " << e.what();

      logger()->log(ostr.str(),
        Logging::Logger::NOTICE,
        Aspect::AD_INST_FRONTEND);
    }
    catch(const eh::Exception& e)
    {
      http_status = 500;

      Stream::Error ostr;
      ostr << FUN << ": eh::Exception caught: " << e.what();

      logger()->log(ostr.str(),
        Logging::Logger::EMERGENCY,
        Aspect::AD_INST_FRONTEND,
        "ADS-IMPL-109");
    }

    co_return FrontendCommons::RequestResult{
      http_status,
      response_ptr,
      false};
  }

  Frontend::MergeUsersTask
  Frontend::co_merge_users_(
    const RequestInfo& request_info)
    noexcept
  {
    static const char* FUN = "Frontend::co_merge_users_()";

    if(request_info.temp_user_id.is_null() || request_info.user_id.is_null())
    {
      co_return MergeUsersResult{};
    }

    if(!user_info_client_coro_)
    {
      co_return MergeUsersResult{false, MergeMessage::SOURCE_NOT_READY};
    }

    if(request_info.temp_user_id == AdServer::Commons::PROBE_USER_ID)
    {
      co_return MergeUsersResult{false, MergeMessage::SOURCE_IS_PROBE};
    }

    try
    {
      adserver::user_info_svcs::user_info_manager::GetUserProfileRequest
        get_profile_request;
      get_profile_request.set_user_id(
        GrpcAlgs::pack_user_id(request_info.temp_user_id));
      get_profile_request.set_temporary(true);
      auto* profiles_request =
        get_profile_request.mutable_profile_request();
      profiles_request->set_base_profile(true);
      profiles_request->set_add_profile(true);
      profiles_request->set_history_profile(true);
      profiles_request->set_freq_cap_profile(false);
      profiles_request->set_pref_profile(false);

      auto get_profile_result =
        co_await user_info_client_coro_->get_user_profile(
          std::move(get_profile_request));
      if(!get_profile_result.status.ok())
      {
        throw_grpc_exception_(
          "UserInfoManager::get_user_profile()",
          get_profile_result.status);
      }

      bool merge_success = get_profile_result.response.found();

      if(get_profile_result.response.user_profile().
           base_user_profile().empty() &&
         get_profile_result.response.user_profile().
           add_user_profile().empty())
      {
        merge_success = false;
      }

      if(!merge_success)
      {
        co_return MergeUsersResult{false, MergeMessage::SOURCE_IS_UNKNOWN};
      }

      if(!request_info.remove_merged_uid)
      {
        co_return MergeUsersResult{};
      }

      adserver::user_info_svcs::user_info_manager::RemoveUserProfileRequest
        remove_request;
      remove_request.set_user_id(
        GrpcAlgs::pack_user_id(request_info.temp_user_id));
      auto remove_result =
        co_await user_info_client_coro_->remove_user_profile(
          std::move(remove_request));
      if(!remove_result.status.ok())
      {
        throw_grpc_exception_(
          "UserInfoManager::remove_user_profile()",
          remove_result.status);
      }

      co_return MergeUsersResult{};
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't get user profile for merging: " <<
        ex.what();
      logger()->log(
        ostr.str(),
        Logging::Logger::NOTICE,
        Aspect::AD_INST_FRONTEND,
        "ADS-IMPL-111");

      co_return MergeUsersResult{false, MergeMessage::SOURCE_EXCEPTION};
    }
  }

  Frontend::InstantiateTask
  Frontend::co_instantiate_click_(
    FCGI::HttpResponse_var response,
    const RequestInfo& request_info,
    const adserver::campaign_svcs::campaign_manager::InstantiateAdResult&
      inst_ad_result)
    noexcept
  {
    static const char* FUN = "Frontend::co_instantiate_click_()";

    try
    {
      if(request_info.creatives.empty())
      {
        co_return 204;
      }

      adserver::campaign_svcs::campaign_manager::ClickInfo click_info;

      const RequestInfo::CreativeInfo& first_creative =
        request_info.creatives.front();
      click_info.set_colo_id(request_info.colo_id);
      click_info.set_tag_id(request_info.tag_id);
      click_info.set_tag_size_id(request_info.tag_size_id);
      click_info.set_log_click(request_info.consider_request);
      click_info.set_ccid(first_creative.ccid);
      click_info.set_ccg_keyword_id(first_creative.ccg_keyword_id);
      if(request_info.creative_id)
      {
        click_info.set_creative_id(request_info.creative_id);
      }
      click_info.mutable_ctr()->set_value(
        GrpcAlgs::pack_decimal(first_creative.ctr));

      if(request_info.user_id_hash_mod.present())
      {
        auto* user_id_hash_mod = click_info.mutable_user_id_hash_mod();
        user_id_hash_mod->set_defined(true);
        user_id_hash_mod->set_value(*request_info.user_id_hash_mod);
      }
      else if(request_info.consider_request &&
        !request_info.enabled_notice &&
        !request_info.user_id.is_null())
      {
        auto* user_id_hash_mod = click_info.mutable_user_id_hash_mod();
        user_id_hash_mod->set_defined(true);
        user_id_hash_mod->set_value(
          AdServer::LogProcessing::user_id_distribution_hash(
            request_info.user_id));
      }
      else
      {
        auto* user_id_hash_mod = click_info.mutable_user_id_hash_mod();
        user_id_hash_mod->set_value(0);
        user_id_hash_mod->set_defined(false);
      }

      click_info.set_time(GrpcAlgs::pack_time(request_info.time));
      click_info.set_bid_time(GrpcAlgs::pack_time(request_info.bid_time));

      if(inst_ad_result.request_ids_size())
      {
        click_info.set_request_id(inst_ad_result.request_ids(0));
      }
      click_info.set_referer(request_info.referer);

      adserver::campaign_svcs::campaign_manager::GetClickUrlRequest
        click_url_request;
      *click_url_request.mutable_click_info() = click_info;
      click_url_request.set_service_index(request_info.campaign_manager_index);

      auto click_url_result = co_await campaign_manager_coro_->get_click_url(
        std::move(click_url_request));
      if(!click_url_result.status.ok())
      {
        throw_grpc_exception_(
          "CampaignManager::get_click_url()",
          click_url_result.status);
      }

      if(click_url_result.response.found())
      {
        response->add_header_nocopy_name(
          Response::Header::LOCATION,
          request_info.click_prefix_url +
            click_url_result.response.click_result_info().url());
        co_return 302;
      }

      co_return 204;
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": fail. Caught eh::Exception: " << ex.what();
      logger()->log(
        ostr.str(),
        Logging::Logger::EMERGENCY,
        Aspect::AD_INST_FRONTEND);
      co_return 500;
    }
  }

  Frontend::InstantiateTask
  Frontend::co_instantiate_ad_(
    FCGI::HttpResponse_var response,
    const RequestInfo& request_info,
    const Generics::SubStringHashAdapter& instantiate_type)
    noexcept
  {
    static const char* FUN = "Frontend::co_instantiate_ad_()";

    try
    {
      adserver::campaign_svcs::campaign_manager::InstantiateAdInfo inst_ad_info;

      auto* common_info = inst_ad_info.mutable_common_info();
      common_info->set_time(GrpcAlgs::pack_time(request_info.bid_time));
      common_info->set_request_id(
        GrpcAlgs::pack_request_id(request_info.global_request_id));
      common_info->set_creative_instantiate_type(instantiate_type.text().str());
      common_info->set_request_type(AdServer::CampaignSvcs::AR_NORMAL);
      common_info->set_random(request_info.random);
      common_info->set_test_request(request_info.test_request);
      common_info->set_log_as_test(request_info.log_as_test);
      common_info->set_colo_id(request_info.colo_id);
      common_info->set_external_user_id(request_info.external_user_id);
      common_info->set_source_id(request_info.source_id);

      if(request_info.location)
      {
        auto* location = common_info->add_location();
        location->set_country(request_info.location->country);
        location->set_region(request_info.location->region);
        location->set_city(request_info.location->city);
      }

      if(request_info.coord_location)
      {
        auto* coord_location = common_info->add_coord_location();
        coord_location->set_longitude(
          GrpcAlgs::pack_decimal(request_info.coord_location->longitude));
        coord_location->set_latitude(
          GrpcAlgs::pack_decimal(request_info.coord_location->latitude));
        coord_location->set_accuracy(
          GrpcAlgs::pack_decimal(request_info.coord_location->accuracy));
      }

      common_info->set_referer(request_info.referer);
      common_info->set_security_token(request_info.security_token);
      common_info->set_pub_impr_track_url(request_info.pub_impr_track_url);
      common_info->set_preclick_url(request_info.preclick_url);
      common_info->set_click_prefix_url(request_info.click_prefix_url);
      common_info->set_original_url(request_info.original_url);

      common_info->set_track_user_id(GrpcAlgs::pack_user_id(
        request_info.track_user_id));
      common_info->set_user_id(GrpcAlgs::pack_user_id(request_info.user_id));
      common_info->set_user_status(
        request_info.user_status != AdServer::CampaignSvcs::US_PROBE ?
        request_info.user_status :
        AdServer::CampaignSvcs::US_UNDEFINED);
      common_info->set_signed_user_id(request_info.signed_user_id);
      common_info->set_peer_ip(request_info.peer_ip);
      common_info->set_user_agent(request_info.user_agent);
      common_info->set_cohort(request_info.cohort);
      common_info->set_hpos(request_info.hpos);
      common_info->set_ext_track_params(request_info.ext_track_params);
      for(const auto pubpixel_account : request_info.pubpixel_accounts)
      {
        inst_ad_info.add_pubpixel_accounts(pubpixel_account);
      }

      // required passback for non profiling requests
      common_info->set_passback_type(request_info.passback_type);
      common_info->set_passback_url(request_info.passback_url);
      common_info->set_set_cookie(request_info.set_cookie);

      for(auto it = request_info.tokens.begin();
        it != request_info.tokens.end(); ++it)
      {
        auto* token = common_info->add_tokens();
        token->set_name(it->first);
        token->set_value(it->second);
      }

      inst_ad_info.set_publisher_site_id(request_info.publisher_site_id);
      inst_ad_info.set_publisher_account_id(request_info.publisher_account_id);
      inst_ad_info.set_emulate_click(request_info.emulate_click);

      if(request_info.consider_request)
      {
        if(request_info.pub_imp_revenue.present())
        {
          inst_ad_info.mutable_pub_imp_revenue()->set_value(
            GrpcAlgs::pack_decimal(*request_info.pub_imp_revenue));
        }
        auto* context_info = inst_ad_info.add_context_info();
        context_info->set_enabled_notice(request_info.enabled_notice);

        context_info->set_client(request_info.client_app);
        context_info->set_client_version(request_info.client_app_version);
        context_info->set_web_browser(request_info.web_browser);
        context_info->set_platform(request_info.platform);
        context_info->set_full_platform(request_info.full_platform);
        for(const auto platform_id : request_info.platform_ids)
        {
          context_info->add_platform_ids(platform_id);
        }
        context_info->set_profile_referer(false);
        context_info->set_page_load_id(0);
        context_info->set_full_referer_hash(request_info.full_referer_hash);
        context_info->set_short_referer_hash(request_info.short_referer_hash);
      }

      if(request_info.user_id_hash_mod.present())
      {
        auto* user_id_hash_mod = inst_ad_info.mutable_user_id_hash_mod();
        user_id_hash_mod->set_defined(true);
        user_id_hash_mod->set_value(*request_info.user_id_hash_mod);
      }
      else if(request_info.consider_request &&
        !request_info.enabled_notice &&
        !request_info.user_id.is_null())
      {
        auto* user_id_hash_mod = inst_ad_info.mutable_user_id_hash_mod();
        user_id_hash_mod->set_defined(true);
        user_id_hash_mod->set_value(
          AdServer::LogProcessing::user_id_distribution_hash(request_info.user_id));
      }
      else
      {
        inst_ad_info.mutable_user_id_hash_mod()->set_defined(false);
      }

      inst_ad_info.set_consider_request(request_info.consider_request);
      inst_ad_info.set_open_price(request_info.open_price);
      inst_ad_info.set_openx_price(request_info.openx_price);
      inst_ad_info.set_liverail_price(request_info.liverail_price);
      inst_ad_info.set_google_price(request_info.google_price);
      inst_ad_info.set_format(request_info.format);
      inst_ad_info.set_tag_id(request_info.tag_id);
      inst_ad_info.set_tag_size_id(request_info.tag_size_id);
      inst_ad_info.set_ext_tag_id(request_info.ext_tag_id);
      inst_ad_info.set_video_width(request_info.video_width);
      inst_ad_info.set_video_height(request_info.video_height);
      RequestInfo::RequestIdList::const_iterator request_id_it =
        request_info.request_ids.begin();
      for(RequestInfo::CreativeList::const_iterator creative_it =
            request_info.creatives.begin();
          creative_it != request_info.creatives.end();
          ++creative_it)
      {
        auto* creative = inst_ad_info.add_creatives();
        creative->set_ccid(creative_it->ccid);
        creative->set_ccg_keyword_id(creative_it->ccg_keyword_id);
        if(request_info.creative_id)
        {
          creative->set_creative_id(request_info.creative_id);
        }
        creative->mutable_ctr()->set_value(
          GrpcAlgs::pack_decimal(creative_it->ctr));
        if(request_id_it != request_info.request_ids.end())
        {
          creative->set_request_id(GrpcAlgs::pack_request_id(*request_id_it));
          ++request_id_it;
        }
      }

      if(!request_info.temp_user_id.is_null() &&
         !request_info.user_id.is_null())
      {
        inst_ad_info.set_merged_user_id(
          GrpcAlgs::pack_user_id(request_info.temp_user_id));
      }

      adserver::campaign_svcs::campaign_manager::InstantiateAdRequest
        instantiate_ad_request;
      *instantiate_ad_request.mutable_instantiate_ad_info() = inst_ad_info;
      instantiate_ad_request.set_service_index(
        request_info.campaign_manager_index);

      auto instantiate_ad_result =
        co_await campaign_manager_coro_->instantiate_ad(
          std::move(instantiate_ad_request));
      if(!instantiate_ad_result.status.ok())
      {
        throw_grpc_exception_(
          "CampaignManager::instantiate_ad()",
          instantiate_ad_result.status);
      }

      const auto& inst_ad_result =
        instantiate_ad_result.response.instantiate_ad_result();

      if(request_info.emulate_click)
      {
        co_return co_await co_instantiate_click_(
          response,
          request_info,
          inst_ad_result);
      }
      else if(!inst_ad_result.creative_body().empty())
      {
        std::string response_body(inst_ad_result.creative_body());
        response->set_content_type(inst_ad_result.mime_format());

        response->get_output_stream().write(
          response_body.c_str(),
          response_body.length());

        if(logger()->log_level() >= TraceLevel::MIDDLE)
        {
          Stream::Error ostr;
          ostr << FUN << ": response:" << std::endl <<
            response_body;

          logger()->log(ostr.str(),
            TraceLevel::MIDDLE,
            Aspect::AD_INST_FRONTEND);
        }

        co_return 200;
      }

      co_return 204;
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": fail. Caught eh::Exception: " << ex.what();
      logger()->log(
        ostr.str(),
        Logging::Logger::EMERGENCY,
        Aspect::AD_INST_FRONTEND);
      co_return 500;
    }
  }
}
