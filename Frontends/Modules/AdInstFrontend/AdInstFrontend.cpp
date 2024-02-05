#include <sstream>
#include <algorithm>
#include <set>

#include <Logger/StreamLogger.hpp>
#include <HTTP/HTTPCookie.hpp>
#include <String/StringManip.hpp>
#include <String/AsciiStringManip.hpp>

#include <Generics/Uuid.hpp>
#include <Generics/GnuHashTable.hpp>
#include <Generics/HashTableAdapters.hpp>

#include <CORBACommons/CorbaAdapters.hpp>

#include <Commons/UserInfoManip.hpp>
#include <Commons/Algs.hpp>

#include <Commons/ErrorHandler.hpp>
#include <Commons/CorbaConfig.hpp>
#include <Commons/CorbaAlgs.hpp>
#include <LogCommons/LogCommons.hpp>

#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>

#include <Frontends/FrontendCommons/OptOutManip.hpp>
#include <Frontends/FrontendCommons/GeoInfoUtils.hpp>

#include "AdInstFrontend.hpp"

namespace
{
  const String::AsciiStringManip::Caseless
    MIME_ENCODED_HTTP_PREFIX("http%3a%2f%2f");
  const String::AsciiStringManip::Caseless
    MIME_ENCODED_HTTPS_PREFIX("https%3a%2f%2f");
  const String::AsciiStringManip::Caseless
    MIME_ENCODED_SHEME_RELATIVE_PREFIX("%2f%2f");
}

namespace Aspect
{
  const char AD_INST_FRONTEND[] = "Instantiate::Frontend";
}

namespace Request
{
  namespace Context
  {
    const String::SubString PRECLICK_URL("preclick");
  }

  namespace Cookie
  {
    const Generics::SubStringHashAdapter LAST_COLOCATION_ID(String::SubString("lc"));
  }
}

namespace Response
{
  namespace Header
  {
    const String::SubString MERGE_FAILED("x-Merge-Failed");
    const String::SubString LOCATION("Location");
    const String::SubString DEBUG_INFO("Debug-Info");
  }
}

namespace AdServer
{
namespace Instantiate
{
  /**
   *  Adinstfrontend implementation
   */
  Frontend::Frontend(
    Configuration* frontend_config,
    Logging::Logger* logger,
    CommonModule* common_module,
    FrontendCommons::HttpResponseFactory* response_factory)
    /*throw(eh::Exception)*/
    : FrontendCommons::FrontendInterface(response_factory),
      Logging::LoggerCallbackHolder(
        Logging::Logger_var(
          new Logging::SeveritySelectorLogger(
            logger,
            0,
            frontend_config->get().AdInstFeConfiguration()->Logger().log_level())),
        "Frontend",
        Aspect::AD_INST_FRONTEND,
        0),
      FrontendCommons::FrontendTaskPool(
        this->callback(),
        response_factory,
        frontend_config->get().AdInstFeConfiguration()->threads(),
        0), // max pending tasks
      frontend_config_(ReferenceCounting::add_ref(frontend_config)),
      common_module_(ReferenceCounting::add_ref(common_module)),
      campaign_managers_(this->logger(), Aspect::AD_INST_FRONTEND)
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
          FrontendCommons::HttpRequest, FrontendCommons::HttpResponse>(
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

        corba_client_adapter_ = new CORBACommons::CorbaClientAdapter();

        campaign_managers_.resolve(
          *common_config_, corba_client_adapter_);

        user_info_client_ = new FrontendCommons::UserInfoClient(
          common_config_->UserInfoManagerControllerGroup(),
          corba_client_adapter_.in(),
          logger());
        add_child_object(user_info_client_);

        request_info_filler_.reset(
          new RequestInfoFiller(
            logger(),
            common_module_,
            common_config_->GeoIP().present() ?
              common_config_->GeoIP()->path().c_str() : 0,
            common_config_->IpEncryptConfig().present() ?
              String::SubString(common_config_->IpEncryptConfig()->key()) :
            String::SubString()));

        activate_object();
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
    const FrontendCommons::HttpRequest& request,
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

  void
  Frontend::handle_request_noparams_(
    FrontendCommons::HttpRequestHolder_var request_holder,
    FrontendCommons::HttpResponseWriter_var response_writer)
    noexcept
  {
    FrontendCommons::HttpRequest& request = request_holder->request();

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
      FrontendCommons::HttpRequest::parse_params(request.body(), params);
    }

    request.set_params(std::move(params));
    handle_request_(request_holder, std::move(response_writer));
  }

  /** Frontend::handle_request */
  void
  Frontend::handle_request_(
    FrontendCommons::HttpRequestHolder_var request_holder,
    FrontendCommons::HttpResponseWriter_var response_writer)
    noexcept
  {
    static const char* FUN = "Frontend::handle_request()";

    const FrontendCommons::HttpRequest& request = request_holder->request();

    FrontendCommons::HttpResponse_var response_ptr = create_response();
    FrontendCommons::HttpResponse& response = *response_ptr;

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

      bool merge_success = true;
      std::string merge_error_message;

      // do users merge
      if(!request_info.temp_user_id.is_null() &&
         !request_info.user_id.is_null())
      {
        merge_success = false;

        merge_users_(
          merge_success,
          merge_error_message,
          request_info);
      }

      http_status = instantiate_ad_(
        response,
        request_info,
        instantiate_type);

      HTTP::CookieList cookies;
      cookies.load_from_headers(request.headers());

      // fill response
      if(!merge_success)
      {
        response.add_header(
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
        // set new user id
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

    response_writer->write(http_status, response_ptr);
  }

  void
  Frontend::merge_users_(
    bool& merge_success,
    std::string& merge_error_message,
    const RequestInfo& request_info)
    noexcept
  {
    static const char* FUN = "Frontend::merge_users_()";

    merge_success = true;

    AdServer::UserInfoSvcs::UserProfiles_var merge_user_profile;
    AdServer::UserInfoSvcs::UserInfo user_info;

    user_info.user_id = CorbaAlgs::pack_user_id(request_info.user_id);

    user_info.last_colo_id = request_info.colo_id;
    user_info.request_colo_id = request_info.colo_id;
    user_info.current_colo_id = -1;
    user_info.temporary = false;
    user_info.time = Generics::Time::get_time_of_day().tv_sec;

    AdServer::UserInfoSvcs::UserInfoMatcher_var
      uim_session = user_info_client_->user_info_session();

    try
    {
      merge_success = false;

      if(uim_session.in())
      {
        if(request_info.temp_user_id == AdServer::Commons::PROBE_USER_ID)
        {
          merge_error_message = MergeMessage::SOURCE_IS_PROBE;
        }
        else
        {
          AdServer::UserInfoSvcs::ProfilesRequestInfo profiles_request;
          profiles_request.base_profile = true;
          profiles_request.add_profile = true;
          profiles_request.history_profile = true;
          profiles_request.freq_cap_profile = false; // don't merge freq cap profiles
          profiles_request.pref_profile = false;

          merge_success = uim_session->get_user_profile(
            CorbaAlgs::pack_user_id(request_info.temp_user_id),
            true,
            profiles_request,
            merge_user_profile.out());

          if (merge_user_profile->base_user_profile.length() == 0 &&
              merge_user_profile->add_user_profile.length() == 0)
          {
            merge_success = false;
          }

          if(!merge_success)
          {
            merge_error_message = MergeMessage::SOURCE_IS_UNKNOWN;
          }
        }
      }
      else
      {
        merge_error_message = MergeMessage::SOURCE_NOT_READY;
      }

      if(merge_success && request_info.remove_merged_uid)
      {
        AdServer::UserInfoSvcs::UserInfoMatcher_var
          uim_session = user_info_client_->user_info_session();

        if(uim_session.in())
        {
          uim_session->remove_user_profile(
            CorbaAlgs::pack_user_id(request_info.temp_user_id));
        }
      }
    }
    catch(const UserInfoSvcs::UserInfoMatcher::NotReady& e)
    {
      logger()->log(
        String::SubString("UserInfoManager not ready for merging."),
        TraceLevel::MIDDLE,
        Aspect::AD_INST_FRONTEND);

      merge_error_message = MergeMessage::SOURCE_NOT_READY;
    }
    catch(const UserInfoSvcs::UserInfoManager::ImplementationException& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't get user profile for merging. "
        "Caught UserInfoManager::ImplementationException: " <<
        ex.description;

      logger()->log(ostr.str(),
        Logging::Logger::NOTICE,
        Aspect::AD_INST_FRONTEND,
        "ADS-IMPL-111");

      merge_error_message = MergeMessage::SOURCE_EXCEPTION;
    }
    catch(const CORBA::SystemException& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't get user profile for merging. "
        "Caught CORBA::SystemException: " <<
        ex;

      logger()->log(ostr.str(),
        Logging::Logger::EMERGENCY,
        Aspect::AD_INST_FRONTEND,
        "ADS-ICON-2");

      merge_error_message = MergeMessage::SOURCE_IS_UNAVAILABLE;
    }
  }

  int
  Frontend::instantiate_click_(
    HttpResponse& response,
    const RequestInfo& request_info,
    const AdServer::CampaignSvcs::CampaignManager::InstantiateAdResult*
      inst_ad_result)
    /*throw(Exception)*/
  {
    static const char* FUN = "Frontend::instantiate_click_()";

    try
    {
      if (!request_info.creatives.empty())
      {
        AdServer::CampaignSvcs::CampaignManager::ClickResultInfo_var click_result_info;
        AdServer::CampaignSvcs::CampaignManager::ClickInfo click_info;

        const RequestInfo::CreativeInfo& first_creative = request_info.creatives.front();
        click_info.colo_id = request_info.colo_id;
        click_info.tag_id = request_info.tag_id;
        click_info.tag_size_id = request_info.tag_size_id;
        click_info.creative_id = request_info.creative_id;
        click_info.log_click = request_info.consider_request;
        click_info.ccid = first_creative.ccid;
        click_info.ccg_keyword_id = first_creative.ccg_keyword_id;
        click_info.ctr = CorbaAlgs::pack_decimal(first_creative.ctr);

        if(request_info.user_id_hash_mod.present())
        {
          click_info.user_id_hash_mod.defined = true;
          click_info.user_id_hash_mod.value = *request_info.user_id_hash_mod;
        }
        else if(request_info.consider_request &&
          !request_info.enabled_notice &&
          !request_info.user_id.is_null())
        {
          click_info.user_id_hash_mod.defined = true;
          click_info.user_id_hash_mod.value =
            AdServer::LogProcessing::user_id_distribution_hash(request_info.user_id);
        }
        else
        {
          click_info.user_id_hash_mod.value = 0;
          click_info.user_id_hash_mod.defined = false;
        }

        click_info.time = CorbaAlgs::pack_time(request_info.time);
        click_info.bid_time = CorbaAlgs::pack_time(request_info.bid_time);

        if (inst_ad_result->request_ids.length())
        {
          click_info.request_id =
            inst_ad_result->request_ids[0];
        }
        click_info.referer << request_info.referer;
        
        CORBA::Boolean got_click_url = false;
        got_click_url = campaign_managers_.get_click_url(
          click_info,
          click_result_info,
          request_info.campaign_manager_index.c_str());

        if(got_click_url)
        {
          // do redirect to click_url
          response.add_header(
            Response::Header::LOCATION,
            request_info.click_prefix_url + click_result_info->url.in());
          return 302;
        }
      }
    }
    catch (const Exception&)
    {
      throw;
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": fail. Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
    return 204;
  }

  int
  Frontend::instantiate_ad_(
    HttpResponse& response,
    const RequestInfo& request_info,
    const Generics::SubStringHashAdapter& instantiate_type)
    /*throw(Exception)*/
  {
    static const char* FUN = "Frontend::instantiate_ad_()";

    AdServer::CampaignSvcs::CampaignManager::RequestCreativeResult_var
      campaign_matching_result;

    try
    {
      AdServer::CampaignSvcs::CampaignManager::InstantiateAdInfo inst_ad_info;

      inst_ad_info.common_info.time = CorbaAlgs::pack_time(request_info.bid_time);
      inst_ad_info.common_info.request_id = CorbaAlgs::pack_request_id(
        request_info.global_request_id);
      inst_ad_info.common_info.creative_instantiate_type << instantiate_type;
      inst_ad_info.common_info.request_type = AdServer::CampaignSvcs::AR_NORMAL;
      inst_ad_info.common_info.random = request_info.random;
      inst_ad_info.common_info.test_request = request_info.test_request;
      inst_ad_info.common_info.log_as_test = request_info.log_as_test;
      inst_ad_info.common_info.colo_id = request_info.colo_id;
      inst_ad_info.common_info.external_user_id << request_info.external_user_id;
      inst_ad_info.common_info.source_id << request_info.source_id;

      FrontendCommons::fill_geo_location_info(
        inst_ad_info.common_info.location,
        request_info.location);

      FrontendCommons::fill_geo_coord_location_info(
        inst_ad_info.common_info.coord_location,
        request_info.coord_location);

      inst_ad_info.common_info.referer << request_info.referer;
      inst_ad_info.common_info.security_token << request_info.security_token;
      inst_ad_info.common_info.pub_impr_track_url << request_info.pub_impr_track_url;
      inst_ad_info.common_info.preclick_url << request_info.preclick_url;
      inst_ad_info.common_info.click_prefix_url << request_info.click_prefix_url;
      inst_ad_info.common_info.original_url << request_info.original_url;

      inst_ad_info.common_info.track_user_id = CorbaAlgs::pack_user_id(
        request_info.track_user_id);
      inst_ad_info.common_info.user_id = CorbaAlgs::pack_user_id(
        request_info.user_id);
      inst_ad_info.common_info.user_status =
        static_cast<CORBA::ULong>(
          request_info.user_status != AdServer::CampaignSvcs::US_PROBE ?
          request_info.user_status :
          AdServer::CampaignSvcs::US_UNDEFINED);
      inst_ad_info.common_info.signed_user_id << request_info.signed_user_id;
      inst_ad_info.common_info.peer_ip << request_info.peer_ip;
      inst_ad_info.common_info.user_agent << request_info.user_agent;
      inst_ad_info.common_info.cohort << request_info.cohort;
      inst_ad_info.common_info.hpos = request_info.hpos;
      inst_ad_info.common_info.ext_track_params << request_info.ext_track_params;
      CorbaAlgs::fill_sequence(
        request_info.pubpixel_accounts.begin(),
        request_info.pubpixel_accounts.end(),
        inst_ad_info.pubpixel_accounts);

      // required passback for non profiling requests
      inst_ad_info.common_info.passback_type << request_info.passback_type;
      inst_ad_info.common_info.passback_url << request_info.passback_url;
      inst_ad_info.common_info.set_cookie = request_info.set_cookie;

      inst_ad_info.common_info.tokens.length(request_info.tokens.size());
      CORBA::ULong tok_i = 0;
      for(auto it = request_info.tokens.begin();
        it != request_info.tokens.end(); ++it, ++tok_i)
      {
        inst_ad_info.common_info.tokens[tok_i].name << it->first;
        inst_ad_info.common_info.tokens[tok_i].value << it->second;
      }

      inst_ad_info.publisher_site_id = request_info.publisher_site_id;
      inst_ad_info.publisher_account_id = request_info.publisher_account_id;
      inst_ad_info.pub_imp_revenue_defined = false;
      inst_ad_info.emulate_click = request_info.emulate_click;

      if(request_info.consider_request)
      {
        inst_ad_info.context_info.length(1);

        if(request_info.pub_imp_revenue.present())
        {
          inst_ad_info.pub_imp_revenue_defined = true;
          inst_ad_info.pub_imp_revenue = CorbaAlgs::pack_decimal(
            *request_info.pub_imp_revenue);
        }
        AdServer::CampaignSvcs::CampaignManager::
          ContextAdRequestInfo& context_info = inst_ad_info.context_info[0];
        context_info.enabled_notice = request_info.enabled_notice;

        context_info.client << request_info.client_app;
        context_info.client_version << request_info.client_app_version;
        context_info.web_browser << request_info.web_browser;
        context_info.platform << request_info.platform;
        context_info.full_platform << request_info.full_platform;
        CorbaAlgs::fill_sequence(
          request_info.platform_ids.begin(),
          request_info.platform_ids.end(),
          context_info.platform_ids);
        context_info.profile_referer = false;
        context_info.page_load_id = 0;
        context_info.full_referer_hash = request_info.full_referer_hash;
        context_info.short_referer_hash = request_info.short_referer_hash;
      }

      if(request_info.user_id_hash_mod.present())
      {
        inst_ad_info.user_id_hash_mod.defined = true;
        inst_ad_info.user_id_hash_mod.value = *request_info.user_id_hash_mod;
      }
      else if(request_info.consider_request &&
        !request_info.enabled_notice &&
        !request_info.user_id.is_null())
      {
        inst_ad_info.user_id_hash_mod.defined = true;
        inst_ad_info.user_id_hash_mod.value =
          AdServer::LogProcessing::user_id_distribution_hash(request_info.user_id);
      }
      else
      {
        inst_ad_info.user_id_hash_mod.defined = false;
      }

      inst_ad_info.consider_request = request_info.consider_request;
      inst_ad_info.open_price << request_info.open_price;
      inst_ad_info.openx_price << request_info.openx_price;
      inst_ad_info.liverail_price << request_info.liverail_price;
      inst_ad_info.google_price << request_info.google_price;
      inst_ad_info.format << request_info.format;
      inst_ad_info.tag_id = request_info.tag_id;
      inst_ad_info.tag_size_id = request_info.tag_size_id;
      inst_ad_info.creative_id = request_info.creative_id;
      inst_ad_info.ext_tag_id << request_info.ext_tag_id;
      inst_ad_info.creatives.length(request_info.creatives.size());
      inst_ad_info.video_width = request_info.video_width;
      inst_ad_info.video_height = request_info.video_height;
      CORBA::ULong creative_i = 0;
      RequestInfo::RequestIdList::const_iterator request_id_it =
        request_info.request_ids.begin();
      for(RequestInfo::CreativeList::const_iterator creative_it =
            request_info.creatives.begin();
          creative_it != request_info.creatives.end();
          ++creative_it, ++creative_i)
      {
        inst_ad_info.creatives[creative_i].ccid = creative_it->ccid;
        inst_ad_info.creatives[creative_i].ccg_keyword_id =
          creative_it->ccg_keyword_id;
        inst_ad_info.creatives[creative_i].ctr =
          CorbaAlgs::pack_decimal(creative_it->ctr);
        if(request_id_it != request_info.request_ids.end())
        {
          inst_ad_info.creatives[creative_i].request_id =
            CorbaAlgs::pack_request_id(*request_id_it);
          ++request_id_it;
        }
      }
      
      if(!request_info.temp_user_id.is_null() &&
         !request_info.user_id.is_null())
      {
        inst_ad_info.merged_user_id = CorbaAlgs::pack_user_id(
          request_info.temp_user_id);
      }

      AdServer::CampaignSvcs::CampaignManager::InstantiateAdResult_var
        inst_ad_result;

      campaign_managers_.instantiate_ad(
        inst_ad_info,
        inst_ad_result.out(),
        request_info.campaign_manager_index.c_str());

      if (request_info.emulate_click)
      {
        return instantiate_click_(
          response,
          request_info,
          inst_ad_result.ptr());
      }
      else if (inst_ad_result->creative_body[0])
      {
        std::string response_body(inst_ad_result->creative_body);
        response.set_content_type(String::SubString(inst_ad_result->mime_format.in()));

        response.get_output_stream().write(
          response_body.c_str(), response_body.length());
          
        if(logger()->log_level() >= TraceLevel::MIDDLE)
        {
          Stream::Error ostr;
          ostr << FUN << ": response:" << std::endl << response_body;
            
          logger()->log(ostr.str(),
            TraceLevel::MIDDLE,
            Aspect::AD_INST_FRONTEND);
        }
          
        return 200;
      }
    }
    catch (const Exception&)
    {
      throw;
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": fail. Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }

    return 204;
  }
}
}
