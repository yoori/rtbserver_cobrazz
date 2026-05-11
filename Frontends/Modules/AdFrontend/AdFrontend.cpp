
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
#include <Commons/GrpcAlgs.hpp>
#include <LogCommons/AdRequestLogger.hpp>
#include <ChannelSvcs/ChannelCommons/ChannelUtils.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>
#include <Controlling/StatsDumper/StatsDumper.hpp>

#include <Frontends/FrontendCommons/OptOutManip.hpp>
#include <Frontends/FrontendCommons/add_UID_cookie.hpp>
#include <Frontends/FrontendCommons/CampaignManagerGrpcClientConfig.hpp>

#include <UserInfoSvcs/UserInfoClient/UserInfoCorbaClient.hpp>

#include "AdFrontend.hpp"

namespace
{
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

namespace
{
  namespace CM = adserver::campaign_svcs::campaign_manager;

  CM::ChannelTriggerMatchInfo
  convert_channel_atom(
    const adserver::channel_svcs::channel_server::ChannelAtom& atom)
    noexcept
  {
    CM::ChannelTriggerMatchInfo out;
    out.set_channel_id(atom.id());
    out.set_channel_trigger_id(atom.trigger_channel_id());
    return out;
  }

  template<typename OctSeq>
  std::string
  pack_oct_seq(const OctSeq& source)
  {
    std::string result;
    result.resize(source.length());
    for(CORBA::ULong i = 0; i < source.length(); ++i)
    {
      result[i] = static_cast<char>(source[i]);
    }
    return result;
  }

  struct ContextualChannelIdLess
  {
    bool
    operator()(
      const AdServer::UserInfoSvcs::UserInfoMatcher::ChannelWeight& ch_weight,
      const adserver::channel_svcs::channel_server::ContentChannelAtom&
        contextual_channel)
      const
    {
      return ch_weight.channel_id < contextual_channel.id();
    }

    bool
    operator()(
      const adserver::channel_svcs::channel_server::ContentChannelAtom&
        contextual_channel,
      const AdServer::UserInfoSvcs::UserInfoMatcher::ChannelWeight& ch_weight)
      const
    {
      return contextual_channel.id() < ch_weight.channel_id;
    }
  };

  struct ContextualChannelConverter
  {
    const AdServer::UserInfoSvcs::UserInfoMatcher::ChannelWeight&
    operator()(const AdServer::UserInfoSvcs::UserInfoMatcher::ChannelWeight& ch_weight)
      const
    {
      return ch_weight;
    }

    AdServer::UserInfoSvcs::UserInfoMatcher::ChannelWeight
    operator()(
      const adserver::channel_svcs::channel_server::ContentChannelAtom&
        contextual_channel)
      const
    {
      AdServer::UserInfoSvcs::UserInfoMatcher::ChannelWeight res;
      res.channel_id = contextual_channel.id();
      res.weight = contextual_channel.weight();
      return res;
    }
  };
}

namespace Aspect
{
  extern const char AD_FRONTEND[] = "AdFrontend";
}

namespace Request
{
  namespace Context
  {
    const String::AsciiStringManip::Caseless CLIENT_ID("uid");
    const String::AsciiStringManip::Caseless OPTIN("OPTED_IN");
  }

  namespace Cookie
  {
    const Generics::SubStringHashAdapter OPTOUT(String::SubString("OPTED_OUT"));
    const Generics::SubStringHashAdapter OPTOUT_TRUE_VALUE(String::SubString("YES"));
    const Generics::SubStringHashAdapter OI_PROMPT(String::SubString("oi_prompt"));
    const Generics::SubStringHashAdapter OI_PROMPT_VALUE(String::SubString("yes-trial-end"));
    const Generics::SubStringHashAdapter OPT_IN_TRIAL(String::SubString("trialoptin"));
    const Generics::SubStringHashAdapter LAST_COLOCATION_ID(String::SubString("lc"));
  }

}

namespace AdServer
{
  namespace
  {
    static const UserInfoSvcs::CampaignIdSeq EMPTY_CAMPAIGN_ID_SEQ;

    class TimeGuard
    {
    public:
      TimeGuard() noexcept;

      Generics::Time consider() noexcept;

      virtual ~TimeGuard() noexcept {};

    private:
      Generics::Timer timer_;
    };

    //
    // TimeGuard implementation
    //
    TimeGuard::TimeGuard() noexcept
    {
      timer_.start();
    }

    Generics::Time
    TimeGuard::consider() noexcept
    {
      timer_.stop();
      return timer_.elapsed_time();
    }

    class UpdateTask: public Generics::GoalTask
    {
    public:
      UpdateTask(
        AdFrontend* ad_frontend,
        Generics::Planner* planner,
        Generics::TaskRunner* task_runner,
        const Generics::Time& update_period,
        Logging::Logger* logger)
        /*throw(eh::Exception)*/
        : Generics::GoalTask(planner, task_runner),
          ad_frontend_(ad_frontend),
          update_period_(update_period),
          logger_(ReferenceCounting::add_ref(logger))
      {}

      virtual void
      execute() noexcept
      {
        ad_frontend_->update_colocation_flags();

        try
        {
          schedule(Generics::Time::get_time_of_day() + update_period_);
        }
        catch (const eh::Exception& ex)
        {
          logger_->sstream(Logging::Logger::EMERGENCY,
            Aspect::AD_FRONTEND) <<
            "UpdateTask::execute(): schedule failed: " << ex.what();
        }
      }

    private:
      AdFrontend* ad_frontend_;
      const Generics::Time update_period_;
      Logging::Logger_var logger_;
    };

    typedef ReferenceCounting::SmartPtr<UpdateTask> UpdateTask_var;
  }

  /**
   *  AdFrontend implementation
   */
  AdFrontend::AdFrontend(
    Configuration* frontend_config,
    Logging::Logger* logger,
    CommonModule* common_module)
    /*throw(eh::Exception)*/
    : Logging::LoggerCallbackHolder(
        Logging::Logger_var(
          new Logging::SeveritySelectorLogger(
            logger,
            0,
            frontend_config->get().AdFeConfiguration()->Logger().log_level())),
        "AdFrontend",
        Aspect::AD_FRONTEND,
        0),
      FrontendCommons::FrontendTaskPool(
        this->callback(),
        frontend_config->get().AdFeConfiguration()->threads(),
        0), // max pending tasks
      fe_config_path_(frontend_config->path()),
      frontend_config_(ReferenceCounting::add_ref(frontend_config)),
      common_module_(ReferenceCounting::add_ref(common_module)),
      campaign_manager_()
  {}

  bool
  AdFrontend::will_handle(const String::SubString& uri) noexcept
  {
    std::string found_uri;
    bool result = FrontendCommons::find_uri(
      config_->UriList().Uri(), uri, found_uri);

    if(logger()->log_level() >= TraceLevel::MIDDLE)
    {
      Stream::Error ostr;
      ostr << "AdFrontend::will_handle(" << uri << "), service: '" << found_uri << "'";

      logger()->log(ostr.str());
    }

    return result;
  }

  void AdFrontend::parse_configs_() /*throw(Exception)*/
  {
    static const char* FUN = "AdFrontend::parse_configs_()";

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

      if(!fe_config.AdFeConfiguration().present())
      {
        throw Exception("AdFeConfiguration isn't present");
      }

      config_.reset(
        new AdFeConfiguration(*fe_config.AdFeConfiguration()));

      if(fe_config.PassFeConfiguration().present())
      {
        pass_config_ = PassConfigPtr(
          new PassFeConfiguration(*fe_config.PassFeConfiguration()));
      }

      cookie_manager_.reset(
        new FrontendCommons::CookieManager<
          FCGI::HttpRequest, FCGI::HttpResponse>(
            common_config_->Cookies()));
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't parse config file '" << fe_config_path_ << "': " <<
        e.what();
      throw Exception(ostr);
    }
  }

  /** AdFrontend::init */
  void
  AdFrontend::init() /*throw(eh::Exception)*/
  {
    static const char* FUN = "AdFrontend::init()";

    if(true) // module_used())
    {
      try
      {
        parse_configs_();

        /* create list of cookies to remove */
        if(common_config_->OutdatedCookies().present())
        {
          for(auto it = common_config_->OutdatedCookies()->Cookie().begin();
            it != common_config_->OutdatedCookies()->Cookie().end(); ++it)
          {
            remove_cookies_holder_.push_back(it->name());
            remove_cookies_.insert(
              Generics::SubStringHashAdapter(remove_cookies_holder_.back()));
          }
        }

        corba_client_adapter_ = new CORBACommons::CorbaClientAdapter();

        task_runner_ = new Generics::TaskRunner(callback(), 2);
        task_scheduler_ = new FrontendCommons::TaskScheduler(
          callback(), task_runner_);
        add_child_object(task_scheduler_.in());

        AdServer::UserInfoSvcs::UserInfoCorbaClient::ControllerRefList
          user_info_controller_groups;
        for(const auto& controller_group :
            common_config_->UserInfoManagerControllerGroup())
        {
          AdServer::UserInfoSvcs::UserInfoCorbaClient::ControllerRef
            controller_group_refs;
          Config::CorbaConfigReader::read_multi_corba_ref(
            controller_group,
            controller_group_refs);
          user_info_controller_groups.push_back(controller_group_refs);
        }
        auto user_info_client = std::make_shared<AdServer::UserInfoSvcs::UserInfoCorbaClient>(
          logger(),
          user_info_controller_groups,
          corba_client_adapter_.in());
        user_info_client_ = user_info_client;
        add_child_object(user_info_client);

        grpc_executor_ = std::make_shared<AdServer::Grpc::GrpcExecutor>(
          common_config_->grpc_executor_threads());
        add_child_object(grpc_executor_);

        auto campaign_manager_client =
          std::make_shared<
            AdServer::CampaignSvcs::CampaignManagerDistributedGrpcClient>(
              FrontendCommons::read_campaign_manager_grpc_refs(*common_config_),
              AdServer::Grpc::BatchingOptions(),
              grpc_executor_);
        campaign_manager_ = campaign_manager_client;
        add_child_object(campaign_manager_client);

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

        auto channel_client_objects =
          AdServer::ChannelSvcs::create_distributed_channel_client(
            *common_config_,
            grpc_executor_);
        channel_client_ = channel_client_objects.client;
        add_child_object(channel_client_objects.active_object);

        if(common_config_->StatsDumper().present())
        {
          CORBACommons::CorbaObjectRef dumper_ref;

          Config::CorbaConfigReader::read_corba_ref(
            common_config_->StatsDumper().get().StatsDumperRef(),
            dumper_ref);

          stats_ = new AdFrontendStat(
            logger(),
            dumper_ref,
            0,
            Generics::Time(common_config_->StatsDumper().get().period()),
            callback());

          add_child_object(stats_.in());
        }

        std::string user_agent_filter_path;
        if(common_config_->user_agent_filter_path().present())
        {
          std::string config_file_path =
            fe_config_path_.substr(0, fe_config_path_.rfind('/'));
          user_agent_filter_path = config_file_path +
            "/" + *common_config_->user_agent_filter_path();
        }

        std::set<std::string> acl_list;

        if(common_config_->DebugInfo().use_acl())
        {
          String::StringManip::Splitter<String::AsciiStringManip::SepNL>
            splitter(String::SubString(common_config_->DebugInfo().ips().c_str()));
          String::SubString token;
          while(splitter.get_token(token))
          {
            acl_list.insert(token.str());
          }
        }

        std::set<int> acl_colo;

        {
          String::StringManip::Splitter<String::AsciiStringManip::SepComma>
            splitter2(String::SubString(common_config_->DebugInfo().colocations().c_str()));
          String::SubString token;
          while(splitter2.get_token(token))
          {
            int colo_id;
            if (String::StringManip::str_to_int(token, colo_id))
            {
              acl_colo.insert(colo_id);
            }
          }
        }

        SetUidController_var set_uid_controller = new SetUidController(
          common_module_->user_id_controller(),
          config_->set_uid(),
          config_->probe_uid());

        request_info_filler_.reset(
          new RequestInfoFiller(
            logger(),
            common_config_->colo_id(),
            common_module_,
            common_config_->GeoIP().present() ?
              common_config_->GeoIP()->path().c_str() : 0,
            user_agent_filter_path.c_str(),
            set_uid_controller,
            common_config_->DebugInfo().use_acl() ? &acl_list : 0,
            acl_colo,
            Commons::LogReferrer::read_log_referrer_settings(
              config_->use_referrer_site_referrer_stats())));

        activate_object();

        start_update_loop_();
      }
      catch (const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << ex.what();
        throw Exception(ostr);
      }

      logger()->log(String::SubString(
          "AdFrontend::init(): frontend is running ..."),
        Logging::Logger::INFO,
        Aspect::AD_FRONTEND);
    }
  }

  /** AdFrontend::shutdown */
  void
  AdFrontend::shutdown() noexcept
  {
    try
    {
      deactivate_object();
      wait_object();

      Stream::Error ostr;
      ostr << "AdFrontend::shutdown: frontend terminated (pid = " <<
        ::getpid() << ").";

      logger()->log(ostr.str(),
        Logging::Logger::INFO,
        Aspect::AD_FRONTEND);
    }
    catch(...)
    {}
  }

  /** AdFrontend::log_request */
  void
  AdFrontend::log_request(
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
        Aspect::AD_FRONTEND);
    }
  }

  /** AdFrontend::handle_request */
  void
  AdFrontend::handle_request_(
    FCGI::HttpRequestHolder_var request_holder,
    FCGI::BaseHttpResponseWriter_var response_writer)
    noexcept
  {
    static const char* FUN = "AdFrontend::handle_request()";

    const FCGI::HttpRequest& request = request_holder->request();

    FCGI::HttpResponse_var response_ptr(new FCGI::HttpResponse());
    FCGI::HttpResponse& response = *response_ptr;

    if(logger()->log_level() >= TraceLevel::MIDDLE)
    {
      logger()->log(String::SubString("AdFrontend::handle_request: entered"),
        TraceLevel::MIDDLE,
        Aspect::AD_FRONTEND);
    }

    int http_status = 200;
    RequestInfo request_info;
    PassbackInfo passback_info;

    DebugSink debug_sink(
      common_config_->DebugInfo().show_history_matching());

    RequestTimeMetering request_time_metering;

    try
    {
      log_request("AdFrontend::handle_request", request, TraceLevel::MIDDLE);

      /* tad request processing */

      TimeGuard request_fill_time_metering;

      request_info_filler_->fill(
        request_info,
        &debug_sink,
        request);

      request_time_metering.request_fill_time =
        request_fill_time_metering.consider();

      std::string str_response;
      Generics::SubStringHashAdapter instantiate_type =
        FrontendCommons::deduce_instantiate_type(&request_info.secure, request);

      http_status = acquire_ad(
        response,
        request,
        request_info,
        instantiate_type,
        str_response,
        passback_info,
        request_info.log_as_test,
        &debug_sink,
        request_time_metering);

      HTTP::CookieList cookies;
      cookies.load_from_headers(request.headers());

      cookie_manager_->remove(
        response, request, cookies, remove_cookies_);

      if(request_info.do_opt_out)
      {
        opt_out_client_(
          cookies,
          response,
          request,
          request_info);
      }

      debug_sink.write_response(response, str_response, http_status);

      if(common_config_->ResponseHeaders().present())
      {
        FrontendCommons::add_headers(
          *(common_config_->ResponseHeaders()),
          response);
      }

      if(logger()->log_level() >= TraceLevel::MIDDLE)
      {
        Stream::Error ostr;
        ostr << FUN << ": response:" << std::endl << str_response;

        logger()->log(ostr.str(),
          TraceLevel::MIDDLE,
          Aspect::AD_FRONTEND);
      }

      if(request_info.have_uid_cookie)
      {
        FrontendCommons::add_UID_cookie(
          response,
          request,
          *cookie_manager_,
          request_info.signed_client_id);
      }

      if(request_info.format == "vast")
      {
        FrontendCommons::CORS::set_headers(request, response);
      }

      if(http_status != 204)
      {
        response.get_output_stream().write(
          str_response.c_str(), str_response.length());
      }
    }
    catch (const ForbiddenException &ex)
    {
      /* forbidden request */
      http_status = 403;

      if(logger()->log_level() >= TraceLevel::LOW ||
         debug_sink.require_debug_body())
      {
        Stream::Error ostr;
        ostr << FUN << ": ForbiddenException caught: " << ex.what();

        if(logger()->log_level() >= TraceLevel::MIDDLE)
        {
          logger()->log(ostr.str(),
            TraceLevel::LOW,
            Aspect::AD_FRONTEND);
        }

        debug_sink.fill_debug_body(response, http_status, ostr);
      }
    }
    catch (const InvalidParamException& e)
    {
      // non correct or passback request
      http_status = 400;

      if(logger()->log_level() >= TraceLevel::MIDDLE ||
         debug_sink.require_debug_body())
      {
        Stream::Error ostr;
        ostr << FUN << ": InvalidParamException caught: " << e.what();

        if(logger()->log_level() >= TraceLevel::MIDDLE)
        {
          logger()->log(ostr.str(),
            TraceLevel::MIDDLE,
            Aspect::AD_FRONTEND);
        }

        debug_sink.fill_debug_body(response, http_status, ostr);
      }
    }
    catch(const HTTP::CookieList::Exception& e)
    {
      http_status = 400;

      Stream::Error ostr;
      ostr << FUN << ": HTTP::CookieList::Exception caught: " << e.what();

      logger()->log(ostr.str(),
        Logging::Logger::NOTICE,
        Aspect::AD_FRONTEND);

      debug_sink.fill_debug_body(response, http_status, ostr);
    }
    catch(const eh::Exception& e)
    {
      http_status = 500;

      Stream::Error ostr;
      ostr << FUN << ": eh::Exception caught: " << e.what();

      logger()->log(ostr.str(),
        Logging::Logger::EMERGENCY,
        Aspect::AD_FRONTEND,
        "ADS-IMPL-109");

      debug_sink.fill_debug_body(response, http_status, ostr);
    }

    if(stats_.in())
    {
      stats_->consider_request(request_info, request_time_metering);
    }

    // campaign manager request failed
    if(http_status != 200)
    {
      try
      {
        if(!request_info.original_url.empty())
        {
          http_status = FrontendCommons::redirect(
            request_info.original_url, response);
        }
        else if(!passback_info.url.empty())
        {
          http_status = FrontendCommons::redirect(
            passback_info.url, response);
        }
      }
      catch(...)
      {
      }
    }

    response_writer->write(http_status, response_ptr);
  }

  void
  AdFrontend::merge_users(
    RequestTimeMetering& request_time_metering,
    bool& merge_success,
    Generics::Time& last_request,
    std::string& merge_error_message,
    const RequestInfo& request_info)
    noexcept
  {
    static const char* FUN = "AdFrontend::merge_users()";

    merge_success = true;

    AdServer::UserInfoSvcs::UserProfiles_var merge_user_profile;
    AdServer::UserInfoSvcs::UserInfo user_info;

    user_info.user_id = CorbaAlgs::pack_user_id(request_info.client_id);
    user_info.huser_id = CorbaAlgs::pack_user_id(AdServer::Commons::UserId());

    user_info.last_colo_id = request_info.last_colo_id;
    user_info.request_colo_id = request_info.colo_id;
    user_info.current_colo_id = -1;
    user_info.temporary =
      request_info.user_status == AdServer::CampaignSvcs::US_TEMPORARY;
    user_info.time = request_info.current_time.tv_sec;
    try
    {
      TimeGuard user_merge_time_metering;

      merge_success = false;

      bool merge_temp = request_info.merge_persistent_client_id.is_null();

      CORBACommons::UserIdInfo merged_uid_info = merge_temp ?
        CorbaAlgs::pack_user_id(request_info.temp_client_id) :
        CorbaAlgs::pack_user_id(request_info.merge_persistent_client_id);

      if(user_info_client_)
      {
        if((merge_temp && request_info.temp_client_id == AdServer::Commons::PROBE_USER_ID) ||
           request_info.merge_persistent_client_id == AdServer::Commons::PROBE_USER_ID)
        {
          merge_error_message = MergeMessage::SOURCE_IS_PROBE;
        }
        else
        {
          AdServer::UserInfoSvcs::ProfilesRequestInfo profiles_request;
          profiles_request.base_profile = true;
          profiles_request.add_profile = true;
          profiles_request.history_profile = true;
          profiles_request.freq_cap_profile = !merge_temp;
          profiles_request.pref_profile = false;

          merge_success = AdServer::UserInfoSvcs::GrpcAlgs::get_user_profile(*user_info_client_,
            merged_uid_info,
            merge_temp,
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
        if(user_info_client_)
        {
          AdServer::UserInfoSvcs::GrpcAlgs::remove_user_profile(*user_info_client_, merged_uid_info);
        }
      }

      request_time_metering.merge_users_time =
        user_merge_time_metering.consider();
    }
    catch(const UserInfoSvcs::UserInfoMatcher::NotReady& e)
    {
      logger()->log(
        String::SubString("UserInfoManager not ready for merging."),
        TraceLevel::MIDDLE,
        Aspect::AD_FRONTEND);

      merge_error_message = MergeMessage::SOURCE_NOT_READY;
    }
    catch(const UserInfoSvcs::UserInfoManager::ImplementationException& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't get user profile for merging."
        "Caught UserInfoManager::ImplementationException: " <<
        ex.description;

      logger()->log(ostr.str(),
        Logging::Logger::NOTICE,
        Aspect::AD_FRONTEND,
        "ADS-IMPL-111");

      merge_error_message = MergeMessage::SOURCE_EXCEPTION;
    }
    catch(const CORBA::SystemException& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't get user profile for merging."
        "Caught CORBA::SystemException: " <<
        ex;

      logger()->log(ostr.str(),
        Logging::Logger::EMERGENCY,
        Aspect::AD_FRONTEND,
        "ADS-ICON-2");

      merge_error_message = MergeMessage::SOURCE_IS_UNAVAILABLE;
    }

    if (merge_success)
    {
      try
      {
        merge_success = false;

        if (request_info.silent_match)
        {
          Stream::Error ostr;
          ostr << FUN <<
            ": merge operation with installed silent_match";
          throw Exception(ostr);
        }

        AdServer::UserInfoSvcs::UserInfoMatcher::MatchParams match_params;
        match_params.use_empty_profile =
          request_info.user_status != AdServer::CampaignSvcs::US_OPTIN &&
          request_info.user_status != AdServer::CampaignSvcs::US_TEMPORARY;
        match_params.silent_match = request_info.silent_match;
        match_params.no_match = request_info.no_match;
        match_params.no_result = request_info.no_result;
        match_params.provide_persistent_channels = false;
        match_params.change_last_request = true;
        match_params.filter_contextual_triggers = false;
        match_params.publishers_optin_timeout = request_info.tag_id != 0 ?
          CorbaAlgs::pack_time(request_info.current_time - Generics::Time::ONE_DAY * 15) :
          CorbaAlgs::pack_time(Generics::Time::ZERO);

        CORBACommons::TimestampInfo_var last_req;
        AdServer::UserInfoSvcs::GrpcAlgs::merge(*user_info_client_,
          user_info,
          match_params,
          merge_user_profile.in(),
          merge_success,
          last_req);

        last_request = CorbaAlgs::unpack_time(last_req);

        merge_success = true;
      }
      catch(const UserInfoSvcs::UserInfoMatcher::ImplementationException& e)
      {
        Stream::Error ostr;
        ostr << FUN <<
          ": caught UserInfoSvcs::UserInfoMatcher::ImplementationException: " <<
          e.description;

        logger()->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          Aspect::AD_FRONTEND,
          "ADS-IMPL-111");

        merge_error_message = MergeMessage::MERGE_EXCEPTION;
      }
      catch(const UserInfoSvcs::UserInfoMatcher::NotReady& e)
      {
        logger()->log(
          String::SubString("UserInfoManager not ready for merging."),
          TraceLevel::MIDDLE,
          Aspect::AD_FRONTEND);

        merge_error_message = MergeMessage::MERGE_NOT_READY;
      }
      catch(const CORBA::SystemException& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": Can't merge users. Caught CORBA::SystemException: " <<
          ex;

        logger()->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          Aspect::AD_FRONTEND,
          "ADS-ICON-2");

        merge_error_message = MergeMessage::MERGE_UNAVAILABLE;
      }
    }
  }

  AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult*
  AdFrontend::get_empty_history_matching()
    /*throw(eh::Exception)*/
  {
    AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult_var res =
      new AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult();
    res->fraud_request = false;
    res->times_inited = false;
    res->last_request_time = CorbaAlgs::pack_time(Generics::Time::ZERO);
    res->create_time = CorbaAlgs::pack_time(Generics::Time::ZERO);
    res->session_start = CorbaAlgs::pack_time(Generics::Time::ZERO);
    res->process_time = CorbaAlgs::pack_time(Generics::Time::ZERO);
//    res->last_ad_request = CorbaAlgs::pack_time(Generics::Time::ZERO);
    res->colo_id = -1;
    return res._retn();
  }

  adserver::channel_svcs::channel_server::MatchResponse
  AdFrontend::get_empty_trigger_matching()
    /*throw(eh::Exception)*/
  {
    adserver::channel_svcs::channel_server::MatchResponse res;
    res.set_no_track(false);
    res.set_no_adv(false);
    return res;
  }

  void
  AdFrontend::acquire_user_info_matcher(
    const RequestInfo& request_info,
    const adserver::channel_svcs::channel_server::MatchResponse*
      trigger_matching_result,
    AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult_out match_result_out,
    bool& profiling_available,
    RequestTimeMetering& request_time_metering)
    noexcept
  {
    static const char* FUN = "AdFrontend::acquire_user_info_matcher()";

    bool match_success = false;
    AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult_var match_result;

    bool do_history_matching =
      request_info.user_status == AdServer::CampaignSvcs::US_OPTIN ||
      request_info.user_status == AdServer::CampaignSvcs::US_TEMPORARY;

    if(user_info_client_ && do_history_matching)
    {
      try
      {
        TimeGuard history_match_time_metering;

        adserver::user_info_svcs::user_info_manager::MatchRequest
          history_match_request;
        auto* user_info = history_match_request.mutable_user_info();
        user_info->set_user_id(GrpcAlgs::pack_user_id(request_info.client_id));
        user_info->set_huser_id(
          GrpcAlgs::pack_user_id(AdServer::Commons::UserId()));
        user_info->set_last_colo_id(request_info.last_colo_id);
        user_info->set_request_colo_id(
          request_info.user_status != AdServer::CampaignSvcs::US_TEMPORARY ?
          request_info.colo_id : -1);
        user_info->set_current_colo_id(-1);
        user_info->set_temporary(
          request_info.user_status == AdServer::CampaignSvcs::US_TEMPORARY);
        user_info->set_time(request_info.current_time.tv_sec);

        auto* match_params = history_match_request.mutable_match_params();
        match_params->set_use_empty_profile(false);
        match_params->set_silent_match(request_info.silent_match);
        match_params->set_no_match(request_info.no_match ||
          (trigger_matching_result && trigger_matching_result->no_track()));
        match_params->set_no_result(request_info.no_result);
        match_params->set_ret_freq_caps(request_info.tag_id != 0);
        match_params->set_provide_channel_count(false);
        match_params->set_provide_persistent_channels(false);
        match_params->set_change_last_request(true);
        match_params->set_filter_contextual_triggers(false);
        match_params->set_publishers_optin_timeout(request_info.tag_id != 0 ?
          GrpcAlgs::pack_time(
            request_info.current_time - Generics::Time::ONE_DAY * 15) :
          GrpcAlgs::pack_time(Generics::Time::ZERO));

        if (request_info.coord_location.in())
        {
          auto* geo_data = match_params->add_geo_data_seq();
          auto latitude = CorbaAlgs::pack_decimal<
            AdServer::CampaignSvcs::CoordDecimal>(
              request_info.coord_location->latitude);
          auto longitude = CorbaAlgs::pack_decimal<
            AdServer::CampaignSvcs::CoordDecimal>(
              request_info.coord_location->longitude);
          auto accuracy = CorbaAlgs::pack_decimal<
            AdServer::CampaignSvcs::AccuracyDecimal>(
              request_info.coord_location->accuracy);
          geo_data->set_latitude(
            reinterpret_cast<const char*>(latitude.get_buffer()),
            latitude.length());
          geo_data->set_longitude(
            reinterpret_cast<const char*>(longitude.get_buffer()),
            longitude.length());
          geo_data->set_accuracy(
            reinterpret_cast<const char*>(accuracy.get_buffer()),
            accuracy.length());
        }

        if(request_info.tag_id == 0 || config_->ad_request_profiling())
        {
          prepare_ui_match_params_(
            *match_params,
            trigger_matching_result,
            request_info);
        }

        match_params->set_cohort(request_info.curct);

        match_result =
          AdServer::UserInfoSvcs::GrpcAlgs::history_match(
            *user_info_client_,
            history_match_request);

        match_success = true;

        request_time_metering.matched_channels =
          match_result->channels.length();
        request_time_metering.history_match_time =
          history_match_time_metering.consider();

        request_time_metering.history_match_local_time =
          CorbaAlgs::unpack_time(match_result->process_time);
      }
      catch(const UserInfoSvcs::UserInfoMatcher::ImplementationException& e)
      {
        Stream::Error ostr;
        ostr << FUN <<
          ": UserInfoSvcs::UserInfoMatcher::ImplementationException caught: " <<
          e.description;

        logger()->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          Aspect::AD_FRONTEND,
          "ADS-IMPL-112");
      }
      catch(const UserInfoSvcs::UserInfoMatcher::NotReady& e)
      {
        logger()->log(
          String::SubString("UserInfoManager not ready for matching."),
          TraceLevel::MIDDLE,
          Aspect::AD_FRONTEND);
      }
      catch(const CORBA::SystemException& ex)
      {
        Stream::Error ostr;
        ostr << FUN <<
          ": Can't match history channels. Caught CORBA::SystemException: " <<
          ex;

        logger()->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          Aspect::AD_FRONTEND,
          "ADS-ICON-2");
      }

      if(!match_result.ptr())
      {
        match_result = get_empty_history_matching();
      }

      /* log user info request */
      if(logger()->log_level() >= TraceLevel::MIDDLE)
      {
        const AdServer::UserInfoSvcs::UserInfoMatcher::ChannelWeightSeq& channels =
          match_result->channels;

        std::ostringstream ostr;
        ostr << FUN << ": history matched channels for uid = '" <<
          request_info.client_id << "': ";

        if(channels.length() == 0)
        {
          ostr << "empty";
        }
        else
        {
          CorbaAlgs::print_sequence_field(ostr,
            channels,
            &AdServer::UserInfoSvcs::UserInfoMatcher::ChannelWeight::channel_id);
        }

        logger()->log(ostr.str(),
          TraceLevel::MIDDLE,
          Aspect::AD_FRONTEND);
      }
    }
    else if(!user_info_client_)
    {
      logger()->log(
        String::SubString("Match with non resolved user info session."),
        TraceLevel::MIDDLE,
        Aspect::AD_FRONTEND);
    }

    if(trigger_matching_result && !trigger_matching_result->no_track())
    {
      if(!match_success || !do_history_matching)
      {
        match_result = get_empty_history_matching();

        /* fill history channels with context channels */
        AdServer::UserInfoSvcs::UserInfoMatcher::ChannelWeightSeq&
          history_matched_channels = match_result->channels;
        const auto& content_channels = trigger_matching_result->content_channels();

        history_matched_channels.length(content_channels.size());

        std::copy(content_channels.begin(),
          content_channels.end(),
          Algs::modify_inserter(history_matched_channels.get_buffer(),
            ContextualChannelConverter()));
      }
      else if(request_info.tag_id != 0 && !config_->ad_request_profiling())
      {
        /* merge history match result & contextually matched channels:
         * history matched channel override contextually matched channel
         */
        const AdServer::UserInfoSvcs::UserInfoMatcher::ChannelWeightSeq&
          history_matched_channels = match_result->channels;
        const auto& content_channels = trigger_matching_result->content_channels();
        AdServer::UserInfoSvcs::UserInfoMatcher::ChannelWeightSeq result_channels;
        result_channels.length(
          history_matched_channels.length() + content_channels.size());

        AdServer::UserInfoSvcs::UserInfoMatcher::ChannelWeight* last_ins =
          Algs::merge_unique(
            history_matched_channels.get_buffer(),
            history_matched_channels.get_buffer() + history_matched_channels.length(),
            content_channels.begin(),
            content_channels.end(),
            Algs::modify_inserter(result_channels.get_buffer(), ContextualChannelConverter()),
            ContextualChannelIdLess(),
            Algs::FirstArg()).base();
        result_channels.length(last_ins - result_channels.get_buffer());
        match_result->channels = result_channels;
      }
    }

    match_result_out = match_result._retn();
    profiling_available = match_success;
  }

  void
  AdFrontend::user_info_post_match_(
    RequestTimeMetering& request_time_metering,
    const RequestInfo& request_info,
    const CM::RequestCreativeResult&
      campaign_select_result)
    noexcept
  {
    static const char* FUN = "AdFrontend::user_info_post_match_()";

    Generics::Timer timer;
    timer.start();

    try
    {

      if(!user_info_client_)
      {
        logger()->log(
          String::SubString("AdFrontend::user_info_post_match_():"
            " non resolved user info session."),
          Logging::Logger::TRACE,
          Aspect::AD_FRONTEND);

        return;
      }

      for(int ad_slot_i = 0;
          ad_slot_i < campaign_select_result.ad_slots_size(); ++ad_slot_i)
      {
        const CM::AdSlotResult& ad_slot_result =
          campaign_select_result.ad_slots(ad_slot_i);

        if(ad_slot_result.selected_creatives_size() > 0)
        {
          UserInfoSvcs::CampaignIdSeq campaign_ids;
          campaign_ids.length(ad_slot_result.selected_creatives_size());

          CORBA::ULong seq_order_len = 0;
          for(int i = 0; i < ad_slot_result.selected_creatives_size(); ++i)
          {
            if(ad_slot_result.selected_creatives(i).order_set_id())
            {
              ++seq_order_len;
            }
          }

          UserInfoSvcs::UserInfoManager::SeqOrderSeq seq_orders;
          seq_orders.length(seq_order_len);
          CORBA::ULong result_seq_order_i = 0;

          for(int creative_i = 0;
            creative_i < ad_slot_result.selected_creatives_size();
            ++creative_i)
          {
            const CM::CreativeSelectResult& creative =
              ad_slot_result.selected_creatives(creative_i);

            if(creative.order_set_id())
            {
              seq_orders[result_seq_order_i].ccg_id = creative.cmp_id();
              seq_orders[result_seq_order_i].set_id = creative.order_set_id();
              seq_orders[result_seq_order_i].imps = 1;

              ++result_seq_order_i;
            }

            campaign_ids[creative_i] = creative.campaign_group_id();
          }

          UserInfoSvcs::FreqCapIdSeq freq_caps;
          UserInfoSvcs::FreqCapIdSeq uc_freq_caps;

          freq_caps.length(ad_slot_result.freq_caps_size());
          for(int i = 0; i < ad_slot_result.freq_caps_size(); ++i)
          {
            freq_caps[i] = ad_slot_result.freq_caps(i);
          }
          uc_freq_caps.length(ad_slot_result.uc_freq_caps_size());
          for(int i = 0; i < ad_slot_result.uc_freq_caps_size(); ++i)
          {
            uc_freq_caps[i] = ad_slot_result.uc_freq_caps(i);
          }

          AdServer::UserInfoSvcs::GrpcAlgs::update_user_freq_caps(*user_info_client_,
            CorbaAlgs::pack_user_id(request_info.client_id),
            CorbaAlgs::pack_time(request_info.current_time),
            CorbaAlgs::pack_request_id(
              Commons::RequestId(ad_slot_result.request_id())),
            freq_caps,
            uc_freq_caps,
            UserInfoSvcs::FreqCapIdSeq(),
            seq_orders,
            ad_slot_result.track_impr() ? EMPTY_CAMPAIGN_ID_SEQ : campaign_ids,
            ad_slot_result.track_impr() ? campaign_ids : EMPTY_CAMPAIGN_ID_SEQ);
        } // ad_slot_result.selected_creatives_size() > 0
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
        Aspect::AD_FRONTEND,
        "ADS-IMPL-112");
    }
    catch(const UserInfoSvcs::UserInfoMatcher::NotReady& e)
    {
      logger()->log(
        String::SubString("UserInfoManager not ready for post match."),
        TraceLevel::MIDDLE,
        Aspect::AD_FRONTEND);
    }
    catch(const CORBA::SystemException& ex)
    {
      Stream::Error ostr;
      ostr << FUN <<
        ": Can't do post match. Caught CORBA::SystemException: " <<
        ex;

      logger()->log(ostr.str(),
        Logging::Logger::EMERGENCY,
        Aspect::AD_FRONTEND,
        "ADS-ICON-2");
    }

    timer.stop();
    request_time_metering.history_post_match_time = timer.elapsed_time();
  }

  adserver::channel_svcs::channel_server::MatchRequest
  AdFrontend::get_empty_matching_request()
  {
    adserver::channel_svcs::channel_server::MatchRequest request;
    request.set_non_strict_word_match(false);
    request.set_non_strict_url_match(false);
    request.set_return_negative(false);
    request.set_simplify_page(true);
    request.set_fill_content(true);
    request.set_statuses("A", 2);
    return request;
  }

  void
  AdFrontend::match_triggers_(
    RequestTimeMetering& request_time_metering,
    adserver::channel_svcs::channel_server::MatchRequest& channel_request,
    adserver::channel_svcs::channel_server::MatchResponse&
      trigger_matched_channels,
    bool& trigger_matched_channels_present,
    const RequestInfo& request_info)
    /*throw(Exception)*/
  {
    static const char* FUN = "AdFrontend::match_triggers_()";

    try
    {
      TimeGuard trigger_match_time_metering;

      channel_request.set_first_url(request_info.referer);
      channel_request.set_first_url_words(request_info.referer_url_words);
      request_time_metering.recived_triggers = request_info.referer.empty() ? 0 : 1;

      /* only referer matching m.b. used for opted out clients */
      if(request_info.full_text_words.empty())
      {
        channel_request.set_pwords(request_info.page_words);
        request_time_metering.recived_triggers +=
          request_info.page_words.size();
      }
      else
      {
        channel_request.set_pwords(request_info.full_text_words);
        request_time_metering.recived_triggers +=
          request_info.full_text_words.size();
      }
      channel_request.set_swords(request_info.search_words);

      channel_request.set_uid(GrpcAlgs::pack_user_id(request_info.client_id));

      trigger_matched_channels = AdServer::ChannelSvcs::GrpcAlgs::channel_match(
        *channel_client_,
        channel_request);
      trigger_matched_channels_present = true;

      const auto& matched_channels = trigger_matched_channels.matched_channels();

      request_time_metering.matched_triggers =
        matched_channels.page_channels_size() +
        matched_channels.search_channels_size() +
        matched_channels.url_channels_size() +
        matched_channels.url_keyword_channels_size() +
        matched_channels.uid_channels_size();

      request_time_metering.trigger_match_time =
        trigger_match_time_metering.consider();

      request_time_metering.detail_trigger_match_time.resize(
        trigger_matched_channels.match_time().empty() ? 0 : 1);

      if(!trigger_matched_channels.match_time().empty())
      {
        request_time_metering.detail_trigger_match_time[0] =
          GrpcAlgs::unpack_time(trigger_matched_channels.match_time());
      }

      if(logger()->log_level() >= TraceLevel::MIDDLE)
      {
        DebugStream ostr;
        ostr << FUN << ": channels matched for page-words '";
        if(request_info.full_text_words.empty())
        {
          ostr << request_info.page_words;
        }
        else
        {
          ostr << request_info.full_text_words;
        }
        ostr << "', search_words '" << request_info.search_words << "', "
          "referer '" << request_info.referer << "':" << std::endl;
        fill_debug_channels_(
          matched_channels.page_channels(), 'P',  ostr);
        fill_debug_channels_(
          matched_channels.search_channels(), 'S',  ostr);
        fill_debug_channels_(
          matched_channels.url_channels(), 'U',  ostr);
        fill_debug_channels_(
          matched_channels.url_keyword_channels(), 'R',  ostr);
        ostr << std::endl;

      logger()->log(ostr.str(),
        TraceLevel::MIDDLE,
        Aspect::AD_FRONTEND);
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN <<
        ": caught ChannelServerGrpcAsyncClient error: " <<
        ex.what();
      logger()->log(ostr.str(),
        Logging::Logger::EMERGENCY,
        Aspect::AD_FRONTEND,
        "ADS-IMPL-117");
    }

  }

  /** AdFrontend::acquire_ad */
  int
  AdFrontend::acquire_ad(
    HttpResponse& response,
    const FCGI::HttpRequest& request,
    const RequestInfo& request_info,
    const Generics::SubStringHashAdapter& instantiate_type,
    std::string& str_response,
    PassbackInfo& passback_info,
    bool& log_as_test,
    DebugSink* debug_sink,
    RequestTimeMetering& request_time_metering)
    /*throw(Exception)*/
  {
    static const char* FUN = "AdFrontend::acquire_ad()";

    adserver::channel_svcs::channel_server::MatchRequest channel_request;
    adserver::channel_svcs::channel_server::MatchResponse
      trigger_matched_channels;
    bool trigger_matched_channels_present = false;
    AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult_var
      history_match_result;
    bool profiling_available = false;

    bool merge_success = true;
    std::string merge_error_message;
    Generics::Time merged_last_request;

    CM::RequestCreativeResult campaign_matching_result;

    bool make_merge = (!request_info.temp_client_id.is_null() ||
      !request_info.merge_persistent_client_id.is_null()) &&
      !request_info.client_id.is_null();

    // check user id by user bind
    if(make_merge || !request_info.passback_by_colocation)
    {
      AdServer::Commons::UserId resolved_user_id;

      if(resolve_cookie_user_id_(resolved_user_id, request_info))
      {
        request_info.client_id = resolved_user_id;
      }
    }

    if(make_merge)
    {
      merge_success = false;

      merge_users(
        request_time_metering,
        merge_success,
        merged_last_request,
        merge_error_message,
        request_info);
    }

    channel_request = get_empty_matching_request();
    if(request_info.keywords_normalized)
    {
      channel_request.set_simplify_page(false);
    }

    if (request_info.passback_by_colocation)
    {
      // disable trigger matching by colocations flags
      trigger_matched_channels = get_empty_trigger_matching();
      trigger_matched_channels_present = true;
    }
    else
    {
      // do trigger based channels matching
      match_triggers_(
        request_time_metering,
        channel_request,
        trigger_matched_channels,
        trigger_matched_channels_present,
        request_info);

      // do history based channels matching
      acquire_user_info_matcher(
        request_info,
        trigger_matched_channels_present ? &trigger_matched_channels : nullptr,
        history_match_result.out(),
        profiling_available,
        request_time_metering);
    }

    if(!history_match_result.ptr())
    {
      history_match_result = get_empty_history_matching();
    }

    AdServer::ChannelSvcs::ChannelServerBase::CCGKeywordSeq_var ccg_keywords;
    request_time_metering.profiling = true;

    /* fill ccg keywords only for ad requests (tid defined) */
    /*
    if(request_info.tag_id)
    {
      request_time_metering.profiling = false;
      try
      {
        if(history_match_result->channels.length())
        {
          AdServer::ChannelSvcs::ChannelIdSeq channel_ids;
          channel_ids.length(history_match_result->channels.length());

          for (CORBA::ULong i = 0;
            i < history_match_result->channels.length(); ++i)
          {
            channel_ids[i] = history_match_result->channels[i].channel_id;
          }

          ccg_keywords = ([&]() {
            adserver::channel_svcs::channel_server::GetCcgTraitsRequest channel_request;
            adserver::channel_svcs::channel_server::GetCcgTraitsResponse channel_response;
            grpc::Status channel_status;
            AdServer::ChannelSvcs::GrpcAlgs::make_get_ccg_traits_request(channel_ids, channel_request);
            channel_client_->get_ccg_traits(
              channel_request,
              [&channel_status, &channel_response](
                const grpc::Status& status,
                const adserver::channel_svcs::channel_server::GetCcgTraitsResponse& response)
              {
                channel_status = status;
                channel_response = response;
              });
            if (!channel_status.ok())
            {
              Stream::Error ostr;
              ostr << "ChannelServer grpc get_ccg_traits failed: code=" <<
                static_cast<int>(channel_status.error_code()) <<
                ", message=" << channel_status.error_message();
              throw Exception(ostr);
            }
            return AdServer::ChannelSvcs::GrpcAlgs::make_ccg_traits_result(
              channel_response);
          })();
        }

      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN <<
          ": caught ChannelServerGrpcAsyncClient error: " <<
          ex.what();
        logger()->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          Aspect::AD_FRONTEND,
          "ADS-IMPL-117");
      }
    }
    */

    debug_sink->print_acquire_ad(
      request_info,
      trigger_matched_channels_present ? &trigger_matched_channels : nullptr,
      ccg_keywords,
      history_match_result);

    /* do campaign selection */
    request_campaign_manager_(
      passback_info,
      log_as_test,
      campaign_matching_result,
      request_time_metering,
      request_info,
      instantiate_type,
      trigger_matched_channels_present ? &trigger_matched_channels : nullptr,
      history_match_result.ptr(),
      merge_success ? merged_last_request :
        CorbaAlgs::unpack_time(history_match_result->last_request_time),
      profiling_available,
      ccg_keywords,
      debug_sink);

    if(request_info.user_status == AdServer::CampaignSvcs::US_OPTIN &&
       campaign_matching_result.ad_slots_size() > 0)
    {
      user_info_post_match_(
        request_time_metering,
        request_info,
        campaign_matching_result);
    }

    try
    {
      if(request_info.user_status == AdServer::CampaignSvcs::US_OPTIN &&
         history_match_result->colo_id != -1)
      {
        std::ostringstream current_colo_ostr;
        current_colo_ostr << history_match_result->colo_id;

        cookie_manager_->set(
          response,
          request,
          Request::Cookie::LAST_COLOCATION_ID,
          current_colo_ostr.str());
      } /* cookies filled */

      if(!merge_success)
      {
        response.add_header_nocopy_name(
          Response::Header::MERGE_FAILED,
          merge_error_message);
      }

      debug_sink->print_creative_selection_debug_info(
        request_info,
        passback_info,
        campaign_matching_result,
        request_time_metering);

      if(campaign_matching_result.ad_slots_size() > 0)
      {
        if(!campaign_matching_result.ad_slots(0).creative_body().empty())
        {
          const CM::AdSlotResult& ad_slot_result =
            campaign_matching_result.ad_slots(0);

          str_response = ad_slot_result.creative_body();

          if(!ad_slot_result.mime_format().empty())
          {
            response.set_content_type(ad_slot_result.mime_format());
          }
          else
          {
            response.set_content_type_nocopy(Response::Type::TEXT_HTML);
          }

          return 200;
        }
      }
    }
    catch(const CORBA::SystemException& ex)
    {
      Stream::Error ostr;
      ostr << FUN <<
        ": Can't generate response. Caught CORBA::SystemException: " << ex;
      throw Exception(ostr);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't generate response. Caught eh::Exception: " <<
        ex.what();
      throw Exception(ostr);
    }

    return 204;
  }

  void
  AdFrontend::convert_ccg_keywords_(
    google::protobuf::RepeatedPtrField<CM::CcgKeywordInfo>& ccg_keywords,
    const AdServer::ChannelSvcs::ChannelServerBase::CCGKeywordSeq* src_ccg_keywords)
    noexcept
  {
    if(src_ccg_keywords)
    {
      for(CORBA::ULong i = 0; i < src_ccg_keywords->length(); ++i)
      {
        const AdServer::ChannelSvcs::ChannelServerBase::CCGKeyword&
          src_ccg_kw = (*src_ccg_keywords)[i];
        CM::CcgKeywordInfo* res_ccg_kw = ccg_keywords.Add();
        res_ccg_kw->set_ccg_keyword_id(src_ccg_kw.ccg_keyword_id);
        res_ccg_kw->set_ccg_id(src_ccg_kw.ccg_id);
        res_ccg_kw->set_channel_id(src_ccg_kw.channel_id);
        res_ccg_kw->mutable_max_cpc()->set_value(
          pack_oct_seq(src_ccg_kw.max_cpc));
        res_ccg_kw->mutable_ctr()->set_value(
          pack_oct_seq(src_ccg_kw.ctr));
        res_ccg_kw->set_click_url(src_ccg_kw.click_url);
        res_ccg_kw->set_original_keyword(src_ccg_kw.original_keyword);
      }
    }
  }

  bool
  AdFrontend::resolve_cookie_user_id_(
    AdServer::Commons::UserId& resolved_user_id,
    const RequestInfo& request_info)
    noexcept
  {
    static const char* FUN = "AdFrontend::resolve_cookie_user_id_()";

    return false;

    if(!request_info.client_id.is_null() && user_bind_client_)
    {
      try
      {

        const std::string ext_user_id = std::string("c/") +
          request_info.client_id.to_string();

        adserver::user_info_svcs::user_bind::GetUserIdRequest
          get_request_info;
        get_request_info.set_id(ext_user_id);
        get_request_info.set_timestamp(
          GrpcAlgs::pack_time(request_info.current_time));
        get_request_info.set_silent(true);
        get_request_info.set_generate_user_id(false);
        get_request_info.set_for_set_cookie(true);
        get_request_info.set_create_timestamp(
          GrpcAlgs::pack_time(Generics::Time::ZERO));
        get_request_info.set_current_user_id(
          GrpcAlgs::pack_user_id(request_info.client_id));

        auto user_bind_info = AdServer::UserInfoSvcs::sync_get_user_id(
          user_bind_client_.get(),
          get_request_info);

        resolved_user_id =
          GrpcAlgs::unpack_user_id(user_bind_info.user_id());

        common_module_->user_id_controller()->null_blacklisted(resolved_user_id);

        return !resolved_user_id.is_null();
      }
      catch(const AdServer::UserInfoSvcs::UserBindClient::NotReady& )
      {
        Stream::Error ostr;
        ostr << FUN << ": caught UserBindClient::NotReady";

        logger()->log(ostr.str(),
          Logging::Logger::WARNING,
          Aspect::AD_FRONTEND,
          "ADS-IMPL-10681");
      }
      catch(const AdServer::UserInfoSvcs::UserBindClient::ChunkNotFound& )
      {
        Stream::Error ostr;
        ostr << FUN << ": caught UserBindClient::ChunkNotFound";

        logger()->log(ostr.str(),
          Logging::Logger::ERROR,
          Aspect::AD_FRONTEND,
          "ADS-IMPL-10681");
      }
      catch(const AdServer::UserInfoSvcs::UserBindClient::ImplementationException& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": caught UserBindClient::ImplementationException: " <<
          ex.what();

        logger()->log(ostr.str(),
          Logging::Logger::ERROR,
          Aspect::AD_FRONTEND,
          "ADS-IMPL-10681");
      }
      catch(const CORBA::SystemException& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": caught CORBA::SystemException: " << e;
        logger()->log(ostr.str(),
          Logging::Logger::ERROR,
          Aspect::AD_FRONTEND,
          "ADS-ICON-7800");
      }
    }

    return false;
  }

  void
  AdFrontend::request_campaign_manager_(
    PassbackInfo& passback_info,
    bool& log_as_test,
    CM::RequestCreativeResult&
      campaign_matching_result,
    RequestTimeMetering& request_time_metering,
    const RequestInfo& request_info,
    const Generics::SubStringHashAdapter& instantiate_type,
    const adserver::channel_svcs::channel_server::MatchResponse*
      trigger_matched_channels,
    AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult* history_match_result,
    const Generics::Time& /*merged_last_request*/,
    bool profiling_available,
    const AdServer::ChannelSvcs::ChannelServerBase::CCGKeywordSeq* ccg_keywords,
    DebugSink* debug_sink)
    /*throw(Exception)*/
  {
    static const char* FUN = "AdFrontend::request_campaign_manager_()";

    /* do campaign selection */
    try
    {
      CM::RequestParams request_params;
      CM::CommonAdRequestInfo* common_info =
        request_params.mutable_common_info();
      CM::ContextAdRequestInfo* context_info =
        request_params.mutable_context_info();
      CM::TriggerMatchResult* trigger_match_result =
        request_params.mutable_trigger_match_result();

      if (trigger_matched_channels)
      {
        const auto& matched_channels =
          trigger_matched_channels->matched_channels();
        for(const auto& channel : matched_channels.page_channels())
        {
          *trigger_match_result->add_pkw_channels() =
            convert_channel_atom(channel);
        }
        for(const auto& channel : matched_channels.search_channels())
        {
          *trigger_match_result->add_skw_channels() =
            convert_channel_atom(channel);
        }
        for(const auto& channel : matched_channels.url_channels())
        {
          *trigger_match_result->add_url_channels() =
            convert_channel_atom(channel);
        }
        for(const auto& channel : matched_channels.url_keyword_channels())
        {
          *trigger_match_result->add_ukw_channels() =
            convert_channel_atom(channel);
        }
        for(const auto channel : matched_channels.uid_channels())
        {
          trigger_match_result->add_uid_channels(channel);
        }
      }

      common_info->set_creative_instantiate_type(
        instantiate_type.text().str());

      if(request_info.location)
      {
        CM::GeoInfo* location = common_info->add_location();
        location->set_country(request_info.location->country);
        location->set_region(request_info.location->region);
        location->set_city(request_info.location->city);
      }

      if(history_match_result)
      {
        for(CORBA::ULong i = 0;
            i < history_match_result->geo_data_seq.length(); ++i)
        {
          CM::GeoCoordInfo* res_loc = common_info->add_coord_location();
          res_loc->set_longitude(
            pack_oct_seq(history_match_result->geo_data_seq[i].longitude));
          res_loc->set_latitude(
            pack_oct_seq(history_match_result->geo_data_seq[i].latitude));
          res_loc->set_accuracy(
            pack_oct_seq(history_match_result->geo_data_seq[i].accuracy));
        }
      }
      else if(request_info.coord_location)
      {
        CM::GeoCoordInfo* coord_location = common_info->add_coord_location();
        coord_location->set_longitude(GrpcAlgs::pack_decimal(
          request_info.coord_location->longitude));
        coord_location->set_latitude(GrpcAlgs::pack_decimal(
          request_info.coord_location->latitude));
        coord_location->set_accuracy(GrpcAlgs::pack_decimal(
          request_info.coord_location->accuracy));
      }

      const std::string user_id = GrpcAlgs::pack_user_id(
        request_info.user_status != AdServer::CampaignSvcs::US_PROBE ?
        request_info.client_id :
        AdServer::Commons::UserId());
      common_info->set_user_id(user_id);
      common_info->set_track_user_id(user_id);

      common_info->set_signed_user_id(request_info.signed_client_id);
      if(!request_info.temp_client_id.is_null() &&
         !request_info.client_id.is_null())
      {
        request_params.set_merged_user_id(
          GrpcAlgs::pack_user_id(request_info.temp_client_id));
      }

      request_params.set_ad_instantiate_type(AdServer::CampaignSvcs::AIT_BODY);
      request_params.set_fill_track_pixel(false);

      // reduce user status values
      common_info->set_user_status(request_info.user_status);

      if(request_info.user_status == AdServer::CampaignSvcs::US_OPTIN && (
           trigger_matched_channels && (
             trigger_matched_channels->no_track() ||
             trigger_matched_channels->no_adv())))
      {
        common_info->set_user_status(AdServer::CampaignSvcs::US_BLACKLISTED);
      }

      request_params.set_client_create_time(
        pack_oct_seq(history_match_result->create_time));
      common_info->set_full_referer(request_info.referer);
      common_info->set_referer(request_info.allowable_referer);
      context_info->set_full_referer_hash(request_info.full_referer_hash);
      context_info->set_short_referer_hash(request_info.short_referer_hash);
      common_info->set_cohort(request_info.curct);
      common_info->set_peer_ip(request_info.peer_ip);
      common_info->set_random(request_info.random);

      request_params.set_fraud(
        history_match_result->fraud_request &&
          !request_info.disable_fraud_detection);
      common_info->set_test_request(
        request_info.test_request || request_info.disable_fraud_detection);
      common_info->set_log_as_test(request_info.log_as_test);
      request_params.set_disable_fraud_detection(
        request_info.disable_fraud_detection);
      request_params.set_profiling_available(profiling_available);

      request_params.set_search_engine_id(request_info.search_engine_id);
      request_params.set_page_keywords_present(
        !request_info.page_words.empty() ||
          !request_info.full_text_words.empty());

      // sample requests
      if ((((double)request_info.random * 100.0) / CampaignSvcs::RANDOM_PARAM_MAX)
          <= common_config_->profiling_log_sampling())
      {
        bool added = false;
        if (!request_info.full_text_words.empty())
        {
          request_params.set_page_keywords(request_info.full_text_words);
          added = true;
        }
        if (!request_info.page_words.empty())
        {
          if (added)
          {
            request_params.mutable_page_keywords()->append(" ");
          }
          request_params.mutable_page_keywords()->append(
            request_info.page_words);
          added = true;
        }

        if (!request_info.referer_url_words.empty())
        {
          request_params.set_url_keywords(request_info.referer_url_words);
        }
      }

      common_info->set_colo_id(request_info.colo_id);

      common_info->set_original_url(request_info.original_url);
      common_info->set_request_id(
        GrpcAlgs::pack_request_id(request_info.request_id));
      common_info->set_time(GrpcAlgs::pack_time(request_info.current_time));

      common_info->set_user_agent(request_info.user_agent);

      // fill request_params.context_info
      context_info->set_enabled_notice(false);
      context_info->set_profile_referer(false);
      context_info->set_client(request_info.client_app);
      context_info->set_client_version(request_info.client_app_version);
      context_info->set_web_browser(request_info.web_browser);
      for(const auto platform_id : request_info.platform_ids)
      {
        context_info->add_platform_ids(platform_id);
      }
      context_info->set_platform(request_info.platform);
      context_info->set_full_platform(request_info.full_platform);
      context_info->set_page_load_id(request_info.page_load_id);
      if(common_config_->ip_logging_enabled())
      {
        std::string ip_hash;
        FrontendCommons::ip_hash(ip_hash, request_info.peer_ip, common_config_->ip_salt());
        context_info->set_ip_hash(ip_hash);
      }

      for(CORBA::ULong i = 0;
          i < history_match_result->full_freq_caps.length(); ++i)
      {
        request_params.add_full_freq_caps(history_match_result->full_freq_caps[i]);
      }

      for(CORBA::ULong seq_order_i = 0;
          seq_order_i != history_match_result->seq_orders.length();
          ++seq_order_i)
      {
        CM::SeqOrderInfo* seq_order = request_params.add_seq_orders();
        seq_order->set_ccg_id(
          history_match_result->seq_orders[seq_order_i].ccg_id);
        seq_order->set_set_id(
          history_match_result->seq_orders[seq_order_i].set_id);
        seq_order->set_imps(
          history_match_result->seq_orders[seq_order_i].imps);
      }

      for(CORBA::ULong i = 0;
          i < history_match_result->campaign_freqs.length(); ++i)
      {
        CM::CampaignFreq* campaign_freq = request_params.add_campaign_freqs();
        campaign_freq->set_campaign_id(
          history_match_result->campaign_freqs[i].campaign_id);
        campaign_freq->set_imps(history_match_result->campaign_freqs[i].imps);
      }

      // required passback for non profiling requests
      common_info->set_passback_type(request_info.passback_type);
      common_info->set_passback_url(request_info.passback_url);
      common_info->set_security_token(request_info.request_token);
      common_info->set_preclick_url(request_info.preclick_url);
      common_info->set_pub_impr_track_url(request_info.pub_impr_track_url);
      common_info->set_request_type(AdServer::CampaignSvcs::AR_NORMAL);
      common_info->set_hpos(CampaignSvcs::UNDEFINED_PUB_POSITION_BOTTOM);
      common_info->set_set_cookie(true);

      request_params.set_publisher_site_id(0);
      request_params.set_required_passback(request_info.tag_id != 0);
      request_params.set_preview_ccid(request_info.ccid);

      // fill input channel sequence for CampaignManager
      for (CORBA::ULong i = 0;
           i < history_match_result->channels.length(); ++i)
      {
        request_params.add_channels(
          history_match_result->channels[i].channel_id);
      }
      if (trigger_matched_channels)
      {
        const auto& uid_channels =
          trigger_matched_channels->matched_channels().uid_channels();
        for (int i = 0; i < uid_channels.size(); ++i)
        {
          request_params.add_channels(uid_channels[i]);
        }
      }

      for(CORBA::ULong i = 0;
          i < history_match_result->exclude_pubpixel_accounts.length(); ++i)
      {
        request_params.add_exclude_pubpixel_accounts(
          history_match_result->exclude_pubpixel_accounts[i]);
      }

      convert_ccg_keywords_(*request_params.mutable_ccg_keywords(), ccg_keywords);

      request_params.set_search_words(request_info.search_words);
      request_params.set_need_debug_info(debug_sink->require_debug_info());
      request_params.set_session_start(
        pack_oct_seq(history_match_result->session_start));
      request_params.set_only_display_ad(false);
      request_params.set_profiling_type(AdServer::CampaignSvcs::PT_ALL);
      request_params.set_additional_info("{}");

      if(request_info.tag_id)
      {
        // initialize slot
        CM::AdSlotInfo* ad_slot = request_params.add_ad_slots();
        ad_slot->set_format(request_info.format);
        ad_slot->set_tag_id(request_info.tag_id);
        ad_slot->set_passback(
          request_info.do_passback ||
          request_info.passback_by_colocation ||
          history_match_result->fraud_request ||
          (trigger_matched_channels &&
            (trigger_matched_channels->no_track() ||
              trigger_matched_channels->no_adv())));
        ad_slot->set_ext_tag_id(request_info.ext_tag_id);
        ad_slot->mutable_min_ecpm()->set_value(
          GrpcAlgs::pack_decimal<CampaignSvcs::RevenueDecimal>(
            CampaignSvcs::RevenueDecimal::ZERO));

        ad_slot->set_up_expand_space(request_info.up_expand_space.present() ?
          static_cast<long>(*request_info.up_expand_space) : -1);
        ad_slot->set_right_expand_space(request_info.right_expand_space.present() ?
          static_cast<long>(*request_info.right_expand_space) : -1);
        ad_slot->set_down_expand_space(request_info.down_expand_space.present() ?
          static_cast<long>(*request_info.down_expand_space) : -1);
        ad_slot->set_left_expand_space(request_info.left_expand_space.present() ?
          static_cast<long>(*request_info.left_expand_space) : -1);
        ad_slot->set_tag_visibility(request_info.tag_visibility.present() ?
          static_cast<long>(*request_info.tag_visibility) : -1);

        ad_slot->set_debug_ccg(request_info.debug_ccg);
        ad_slot->set_video_min_duration(0);
        ad_slot->set_video_max_duration(-1);
        ad_slot->set_video_skippable_max_duration(-1);
        ad_slot->set_video_width(0);
        ad_slot->set_video_height(0);
        ad_slot->set_video_allow_skippable(true);
        ad_slot->set_video_allow_unskippable(true);
      }

      TimeGuard creative_selection_time_metering;

      CM::GetCampaignCreativeRequest get_campaign_creative_request;
      *get_campaign_creative_request.mutable_request_params() = request_params;
      auto get_campaign_creative_response = AdServer::Grpc::sync_call<
        CM::GetCampaignCreativeResponse>(
          [&](auto callback)
          {
            campaign_manager_->get_campaign_creative(
              get_campaign_creative_request,
              std::move(callback));
          },
          [](const grpc::Status& status)
          {
            Stream::Error ostr;
            ostr << "CampaignManager::get_campaign_creative(): "
              "gRPC call failed: code=" <<
              static_cast<int>(status.error_code()) <<
              ", message=" << status.error_message();
            return Exception(ostr);
          });

      campaign_matching_result =
        std::move(*get_campaign_creative_response.mutable_request_result());

      assert(campaign_matching_result.ad_slots_size() ==
        request_params.ad_slots_size());

      request_time_metering.creative_selection_local_time =
        GrpcAlgs::unpack_time(campaign_matching_result.process_time());

      if(campaign_matching_result.ad_slots_size() > 0)
      {
        const CM::AdSlotResult& ad_slot_result =
          campaign_matching_result.ad_slots(0);

        if(ad_slot_result.passback() &&
           !ad_slot_result.passback_url().empty())
        {
          passback_info.url = ad_slot_result.passback_url();
        }

        log_as_test |= ad_slot_result.test_request();

        request_time_metering.creative_selection_time =
          creative_selection_time_metering.consider();

        request_time_metering.creative_count =
          ad_slot_result.selected_creatives_size();
        request_time_metering.passback =
          ad_slot_result.passback();
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
  }

  void
  AdFrontend::fill_debug_channels_(
    const google::protobuf::RepeatedPtrField<
      adserver::channel_svcs::channel_server::ChannelAtom>& in,
    char type,
    DebugStream& out)
    /*throw(eh::Exception)*/
  {
    size_t count = 0;
    for(int i = 0; i < in.size(); ++i)
    {
      if(count)
      {
        out << ",";
      }
      out << in[i].id() << type;
      count++;
    }
    if(count == 0)
    {
      out << "empty";
    }
    else
    {
      out << " ";
    }
  }

  void
  AdFrontend::start_update_loop_() /*throw(Exception)*/
  {
    static const char* FUN = "AdFrontend::start_update_loop_()";

    try
    {
      UpdateTask_var msg = new UpdateTask(
        this,
        task_scheduler_->planner(),
        task_runner_,
        Generics::Time(common_config_->update_period()),
        logger());

      msg->execute();
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  /* AdFrontend::opt_out_client */
  void
  AdFrontend::opt_out_client_(
    const HTTP::CookieList& cookies,
    HttpResponse& response,
    const FCGI::HttpRequest& request,
    const RequestInfo& request_info)
    noexcept
  {
    static const char* FUN = "AdFrontend::opt_out_client_()";

    try
    {
      FrontendCommons::CookieNameSet remove_cookie_list;

      for(auto it = common_config_->OptOutRemoveCookies().Cookie().begin();
        it != common_config_->OptOutRemoveCookies().Cookie().end(); ++it)
      {
        remove_cookie_list.insert(it->name());
      }

      cookie_manager_->remove(response, request, cookies, remove_cookie_list);

      cookie_manager_->set(
        response,
        request,
        Request::Cookie::OPTOUT,
        Request::Cookie::OPTOUT_TRUE_VALUE);

      cookie_manager_->set(
        response,
        request,
        Request::Cookie::OI_PROMPT,
        Request::Cookie::OI_PROMPT_VALUE);

      CM::VerifyOptOperationRequest opt_operation_request;
      opt_operation_request.set_time(request_info.current_time.tv_sec);
      opt_operation_request.set_colo_id(request_info.colo_id);
      opt_operation_request.set_referer("");
      opt_operation_request.set_operation(CM::OPT_OPERATION_OUT);
      opt_operation_request.set_status(11);
      opt_operation_request.set_user_status(CampaignSvcs::US_OPTOUT);
      opt_operation_request.set_log_as_test(request_info.log_as_test);
      opt_operation_request.set_browser(request_info.web_browser);
      opt_operation_request.set_os(request_info.full_platform);
      opt_operation_request.set_ct("");
      opt_operation_request.set_curct("");

      AdServer::Grpc::sync_call<CM::VerifyOptOperationResponse>(
        [&](auto callback)
        {
          campaign_manager_->verify_opt_operation(
            opt_operation_request,
            std::move(callback));
        },
        [](const grpc::Status& status)
        {
          Stream::Error ostr;
          ostr << "CampaignManager::verify_opt_operation(): "
            "gRPC call failed: code=" <<
            static_cast<int>(status.error_code()) <<
            ", message=" << status.error_message();
          return Exception(ostr);
        });
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't do opt out. Caught eh::Exception: " << ex.what();

      logger()->log(ostr.str(),
        Logging::Logger::EMERGENCY,
        Aspect::AD_FRONTEND,
        "ADS-IMPL-116");
    }
  }

  void
  AdFrontend::update_colocation_flags()
    noexcept
  {
    static const char* FUN = "AdFrontend::update_colocation_flags()";

    try
    {
      auto colocation_response =
        AdServer::Grpc::sync_call<CM::GetColocationFlagsResponse>(
          [&](auto callback)
          {
            campaign_manager_->get_colocation_flags(
              CM::GetColocationFlagsRequest(),
              std::move(callback));
          },
          [](const grpc::Status& status)
          {
            Stream::Error ostr;
            ostr << "CampaignManager::get_colocation_flags(): "
              "gRPC call failed: code=" <<
              static_cast<int>(status.error_code()) <<
              ", message=" << status.error_message();
            return Exception(ostr);
          });

      RequestInfoFiller::ColoFlagsMap_var new_colo_flags(
        new RequestInfoFiller::ColoFlagsMap());
      for (const auto& colocation : colocation_response.colocations())
      {
        RequestInfoFiller::ColoFlags colo_flags;
        colo_flags.flags = colocation.flags();
        colo_flags.hid_profile = colocation.hid_profile();
        new_colo_flags->insert(
          RequestInfoFiller::ColoFlagsMap::value_type(
            colocation.colo_id(),
            colo_flags));
      }

      request_info_filler_->colo_flags(new_colo_flags);
    }
    catch (const eh::Exception& e)
    {
      logger()->sstream(Logging::Logger::CRITICAL,
        Aspect::AD_FRONTEND,
        "ADS-IMPL-118") << FUN << ": Can't update colocation flags, "
        "caught eh::Exception: " << e.what();
    }
  }

  void
  AdFrontend::add_hit_channels_(
    google::protobuf::RepeatedPtrField<
      adserver::user_info_svcs::user_info_manager::ChannelTriggerMatch>&
        result_channel_ids,
    const AdServer::CampaignSvcs::ChannelIdArray& hit_channels)
  {
    for(auto it = hit_channels.begin(); it != hit_channels.end(); ++it)
    {
      auto* result_channel_id = result_channel_ids.Add();
      result_channel_id->set_channel_id(*it);
      result_channel_id->set_channel_trigger_id(0);
    }
  }

  void AdFrontend::prepare_ui_match_params_(
    adserver::user_info_svcs::user_info_manager::MatchParams& match_params,
    const adserver::channel_svcs::channel_server::MatchResponse* match_result,
    const RequestInfo& request_info)
    /*throw(eh::Exception)*/
  {
    typedef std::set<ChannelMatch> ChannelMatchSet;

    if(match_result && !match_result->no_track())
    {
      const auto& matched_channels = match_result->matched_channels();
      ChannelMatchSet url_channels;
      ChannelMatchSet page_channels;
      ChannelMatchSet search_channels;
      ChannelMatchSet url_keyword_channels;

      std::transform(
        matched_channels.url_channels().begin(),
        matched_channels.url_channels().end(),
        std::inserter(url_channels, url_channels.end()),
        GetChannelTriggerId());

      std::transform(
        matched_channels.page_channels().begin(),
        matched_channels.page_channels().end(),
        std::inserter(page_channels, page_channels.end()),
        GetChannelTriggerId());

      std::transform(
        matched_channels.search_channels().begin(),
        matched_channels.search_channels().end(),
        std::inserter(search_channels, search_channels.end()),
        GetChannelTriggerId());

      std::transform(
        matched_channels.url_keyword_channels().begin(),
        matched_channels.url_keyword_channels().end(),
        std::inserter(url_keyword_channels, url_keyword_channels.end()),
        GetChannelTriggerId());

      const auto fill_channel_matches =
        [](
          auto* out,
          const ChannelMatchSet& in)
      {
        for(const auto& channel_match : in)
        {
          auto* result = out->Add();
          result->set_channel_id(channel_match.channel_id);
          result->set_channel_trigger_id(channel_match.channel_trigger_id);
        }
      };
      fill_channel_matches(
        match_params.mutable_url_channel_ids(),
        url_channels);
      fill_channel_matches(
        match_params.mutable_page_channel_ids(),
        page_channels);
      fill_channel_matches(
        match_params.mutable_search_channel_ids(),
        search_channels);
      fill_channel_matches(
        match_params.mutable_url_keyword_channel_ids(),
        url_keyword_channels);

      for(const auto channel_id : request_info.platform_ids)
      {
        match_params.add_persistent_channel_ids(channel_id);
      }
    }

    add_hit_channels_(
      *match_params.mutable_url_channel_ids(),
      request_info.hit_channel_ids);

    add_hit_channels_(
      *match_params.mutable_url_keyword_channel_ids(),
      request_info.hit_channel_ids);

    add_hit_channels_(
      *match_params.mutable_page_channel_ids(),
      request_info.hit_channel_ids);

    add_hit_channels_(
      *match_params.mutable_search_channel_ids(),
      request_info.hit_channel_ids);
  }

}
