
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
#include <Commons/UserverConfigUtils.hpp>
#include <LogCommons/AdRequestLogger.hpp>
#include <ChannelSvcs/ChannelCommons/ChannelUtils.hpp>
#include <ChannelSvcs/ChannelManagerController/ChannelSessionFactory.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>
#include <Controlling/StatsDumper/StatsDumper.hpp>

#include <Frontends/FrontendCommons/OptOutManip.hpp>
#include <Frontends/FrontendCommons/GeoInfoUtils.hpp>
#include <Frontends/FrontendCommons/add_UID_cookie.hpp>

#include <UserInfoSvcs/UserInfoManager/ProtoConvertor.hpp>

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
      const AdServer::ChannelSvcs::ChannelServerBase::ChannelAtom& atom)
      noexcept
    {
      return ChannelMatch(atom.id, atom.trigger_channel_id);
    }
  };
}

namespace
{
  AdServer::CampaignSvcs::CampaignManager::ChannelTriggerMatchInfo
  convert_channel_atom(
    const AdServer::ChannelSvcs::ChannelServerBase::ChannelAtom& atom)
    noexcept
  {
    AdServer::CampaignSvcs::CampaignManager::ChannelTriggerMatchInfo out;
    out.channel_id = atom.id;
    out.channel_trigger_id = atom.trigger_channel_id;
    return out;
  }

  template<class Error>
  inline const char* get_merge_error_message(
    const Error& error) noexcept
  {
    using ErrorType = typename Error::Type;

    switch (error.type())
    {
      case ErrorType::Error_Type_ChunkNotFound:
      {
        return MergeMessage::SOURCE_EXCEPTION;
      }
      case ErrorType::Error_Type_NotReady:
      {
        return MergeMessage::SOURCE_NOT_READY;
      }
      case ErrorType::Error_Type_Implementation:
      {
        return MergeMessage::SOURCE_EXCEPTION;
      }
      default:
      {
        return MergeMessage::SOURCE_EXCEPTION;
      }
    }
  }
} // namespace

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
    const GrpcContainerPtr& grpc_container,
    TaskProcessor& task_processor,
    const SchedulerPtr& scheduler,
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
            frontend_config->get().AdFeConfiguration()->Logger().log_level())),
        "AdFrontend",
        Aspect::AD_FRONTEND,
        0),
      FrontendCommons::FrontendTaskPool(
        this->callback(),
        response_factory,
        frontend_config->get().AdFeConfiguration()->threads(),
        0), // max pending tasks
      grpc_container_(grpc_container),
      task_processor_(task_processor),
      scheduler_(scheduler),
      fe_config_path_(frontend_config->path()),
      frontend_config_(ReferenceCounting::add_ref(frontend_config)),
      common_module_(ReferenceCounting::add_ref(common_module)),
      campaign_managers_(this->logger(), Aspect::AD_FRONTEND)
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
          FrontendCommons::HttpRequest, FrontendCommons::HttpResponse>(
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
        add_child_object(task_scheduler_);

        CORBACommons::CorbaObjectRefList channel_manager_controller_refs;
        Config::CorbaConfigReader::read_multi_corba_ref(
          common_config_->ChannelManagerControllerRefs().get(),
          channel_manager_controller_refs);

        channel_servers_.reset(
          new FrontendCommons::ChannelServerSessionPool(
            channel_manager_controller_refs,
            corba_client_adapter_,
            callback()));
        
        campaign_managers_.resolve(
          *common_config_, corba_client_adapter_);

        const auto& config_grpc_client = common_config_->GrpcClientPool();
        const auto config_grpc_data = Config::create_pool_client_config(
          config_grpc_client);

        user_info_client_ = new FrontendCommons::UserInfoClient(
          common_config_->UserInfoManagerControllerGroup(),
          corba_client_adapter_.in(),
          logger(),
          task_processor_,
          config_grpc_data.first,
          config_grpc_data.second,
          config_grpc_client.enable());
        add_child_object(user_info_client_);

        if(!common_config_->UserBindControllerGroup().empty())
        {
          user_bind_client_ = new FrontendCommons::UserBindClient(
            common_config_->UserBindControllerGroup(),
            corba_client_adapter_.in(),
            logger(),
            task_processor_,
            scheduler_,
            config_grpc_data.first,
            config_grpc_data.second,
            config_grpc_client.enable());
          add_child_object(user_bind_client_);
        }

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

          add_child_object(stats_);
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

        start_update_loop_();

        activate_object();
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
        Aspect::AD_FRONTEND);
    }
  }

  /** AdFrontend::handle_request */
  void
  AdFrontend::handle_request_(
    FrontendCommons::HttpRequestHolder_var request_holder,
    FrontendCommons::HttpResponseWriter_var response_writer)
    noexcept
  {
    static const char* FUN = "AdFrontend::handle_request()";

    const FrontendCommons::HttpRequest& request = request_holder->request();

    FrontendCommons::HttpResponse_var response_ptr = create_response();
    FrontendCommons::HttpResponse& response = *response_ptr;

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

    AdServer::UserInfoSvcs::GrpcUserInfoOperationDistributor_var
      grpc_distributor = user_info_client_->grpc_distributor();

    bool is_grpc_success = false;
    if (grpc_distributor)
    {
      using ProfilesRequestInfo = AdServer::UserInfoSvcs::Types::ProfilesRequestInfo;
      using MatchParams = AdServer::UserInfoSvcs::Types::MatchParams;
      using UserInfo = AdServer::UserInfoSvcs::Types::UserInfo;
      using UserProfiles = AdServer::UserInfoSvcs::Types::UserProfiles;

      try
      {
        is_grpc_success = true;
        merge_success = false;
        TimeGuard user_merge_time_metering;

        UserProfiles merge_user_profiles;

        const bool merge_temp =
          request_info.merge_persistent_client_id.is_null();
        const auto merged_uid_info = merge_temp ?
          GrpcAlgs::pack_user_id(request_info.temp_client_id) :
          GrpcAlgs::pack_user_id(request_info.merge_persistent_client_id);

        if((merge_temp && request_info.temp_client_id == AdServer::Commons::PROBE_USER_ID) ||
           request_info.merge_persistent_client_id == AdServer::Commons::PROBE_USER_ID)
        {
          merge_error_message = MergeMessage::SOURCE_IS_PROBE;
          request_time_metering.merge_users_time =
            user_merge_time_metering.consider();
          return;
        }
        else
        {
          ProfilesRequestInfo profiles_request;
          profiles_request.base_profile = true;
          profiles_request.add_profile = true;
          profiles_request.history_profile = true;
          profiles_request.freq_cap_profile = !merge_temp;
          profiles_request.pref_profile = false;

          auto response = grpc_distributor->get_user_profile(
            merged_uid_info,
            merge_temp,
            profiles_request);
          if (!response || response->has_error())
          {
            GrpcAlgs::print_grpc_error_response(
              response,
              logger(),
              Aspect::AD_FRONTEND);

            if (response)
            {
              merge_error_message = get_merge_error_message(response->error());
            }
            else
            {
              merge_error_message = MergeMessage::SOURCE_EXCEPTION;
            }

            throw Exception("get_user_profile is failed");
          }

          const auto& info_proto = response->info();
          if (!info_proto.return_value())
          {
            merge_error_message = MergeMessage::SOURCE_IS_UNKNOWN;
            request_time_metering.merge_users_time =
              user_merge_time_metering.consider();
            return;
          }

          const auto& user_profiles_proto = info_proto.user_profiles();
          if (user_profiles_proto.base_user_profile().size() == 0 &&
              user_profiles_proto.add_user_profile().size() == 0)
          {
            merge_error_message = MergeMessage::SOURCE_IS_UNKNOWN;
            request_time_metering.merge_users_time =
              user_merge_time_metering.consider();
            return;
          }

          merge_user_profiles.add_user_profile = user_profiles_proto.add_user_profile();
          merge_user_profiles.base_user_profile = user_profiles_proto.base_user_profile();
          merge_user_profiles.freq_cap = user_profiles_proto.freq_cap();
          merge_user_profiles.history_user_profile = user_profiles_proto.history_user_profile();
          merge_user_profiles.pref_profile = user_profiles_proto.pref_profile();

          if (request_info.remove_merged_uid)
          {
            auto response = grpc_distributor->remove_user_profile(
              merged_uid_info);
            if (!response || response->has_error())
            {
              GrpcAlgs::print_grpc_error_response(
                response,
                logger(),
                Aspect::AD_FRONTEND);

              if (response)
              {
                merge_error_message = get_merge_error_message(response->error());
              }
              else
              {
                merge_error_message = MergeMessage::SOURCE_EXCEPTION;
              }

              throw Exception("remove_user_profile is failed");
            }
          }
        }

        if (request_info.silent_match)
        {
          throw Exception("Merge operation with installed silent_match");
        }

        UserInfo user_info;
        user_info.user_id = GrpcAlgs::pack_user_id(request_info.client_id);
        user_info.huser_id = GrpcAlgs::pack_user_id(request_info.household_client_id);
        user_info.last_colo_id = request_info.last_colo_id;
        user_info.request_colo_id = request_info.colo_id;
        user_info.current_colo_id = -1;
        user_info.temporary =
          request_info.user_status == AdServer::CampaignSvcs::US_TEMPORARY;
        user_info.time = request_info.current_time.tv_sec;

        MatchParams match_params;
        match_params.use_empty_profile =
          request_info.user_status != AdServer::CampaignSvcs::US_OPTIN &&
          request_info.user_status != AdServer::CampaignSvcs::US_TEMPORARY;
        match_params.silent_match = request_info.silent_match;
        match_params.no_match = request_info.no_match;
        match_params.no_result = request_info.no_result;
        match_params.provide_persistent_channels = false;
        match_params.change_last_request = true;
        match_params.filter_contextual_triggers = false;
        match_params.publishers_optin_timeout =
          request_info.tag_id != 0 ?
            request_info.current_time - Generics::Time::ONE_DAY * 15 :
            Generics::Time::ZERO;

        auto response = grpc_distributor->merge(
          user_info,
          match_params,
          merge_user_profiles);
        if (!response || response->has_error())
        {
          GrpcAlgs::print_grpc_error_response(
            response,
            logger(),
            Aspect::AD_FRONTEND);

          if (response)
          {
            merge_error_message = get_merge_error_message(response->error());
          }
          else
          {
            merge_error_message = MergeMessage::SOURCE_EXCEPTION;
          }

          throw Exception("merge is failed");
        }

        const auto& info_proto = response->info();

        last_request = GrpcAlgs::unpack_time(info_proto.last_request());
        request_time_metering.merge_users_time =
          user_merge_time_metering.consider();
        merge_success = info_proto.merge_success();
      }
      catch (const eh::Exception& exc)
      {
        is_grpc_success = false;
        if (merge_error_message.empty())
        {
          merge_error_message = MergeMessage::SOURCE_EXCEPTION;
        }
        Stream::Error stream;
        stream << FUN
               << ": "
               << exc.what();
        logger()->error(stream.str(), Aspect::AD_FRONTEND);
      }
      catch (...)
      {
        is_grpc_success = false;
        if (merge_error_message.empty())
        {
          merge_error_message = MergeMessage::SOURCE_EXCEPTION;
        }
        Stream::Error stream;
        stream << FUN
               << ": Unknown error";
        logger()->error(stream.str(), Aspect::AD_FRONTEND);
      }
    }

    if (is_grpc_success)
      return;

    merge_success = true;

    AdServer::UserInfoSvcs::UserProfiles_var merge_user_profile;
    AdServer::UserInfoSvcs::UserInfo user_info;

    user_info.user_id = CorbaAlgs::pack_user_id(request_info.client_id);
    user_info.huser_id = CorbaAlgs::pack_user_id(request_info.household_client_id);

    user_info.last_colo_id = request_info.last_colo_id;
    user_info.request_colo_id = request_info.colo_id;
    user_info.current_colo_id = -1;
    user_info.temporary =
      request_info.user_status == AdServer::CampaignSvcs::US_TEMPORARY;
    user_info.time = request_info.current_time.tv_sec;

    AdServer::UserInfoSvcs::UserInfoMatcher_var
      uim_session = user_info_client_->user_info_session();

    try
    {
      TimeGuard user_merge_time_metering;

      merge_success = false;

      bool merge_temp = request_info.merge_persistent_client_id.is_null();

      CORBACommons::UserIdInfo merged_uid_info = merge_temp ?
        CorbaAlgs::pack_user_id(request_info.temp_client_id) :
        CorbaAlgs::pack_user_id(request_info.merge_persistent_client_id);

      if(uim_session.in())
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

          merge_success = uim_session->get_user_profile(
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
        AdServer::UserInfoSvcs::UserInfoMatcher_var
          uim_session = user_info_client_->user_info_session();

        if(uim_session.in())
        {
          uim_session->remove_user_profile(merged_uid_info);
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
        uim_session->merge(
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
//    res->last_ad_request = CorbaAlgs::pack_time(Generics::Time::ZERO);
    res->colo_id = -1;
    return res._retn();
  }

  AdServer::ChannelSvcs::ChannelServerBase::MatchResult*
  AdFrontend::get_empty_trigger_matching()
    /*throw(eh::Exception)*/
  {
    AdServer::ChannelSvcs::ChannelServerBase::MatchResult_var res =
      new AdServer::ChannelSvcs::ChannelServerBase::MatchResult;
    res->no_track = 0;
    res->no_adv = 0;
    return res._retn();
  }

  struct ContextualChannelIdLess
  {
    bool
    operator()(
      const AdServer::UserInfoSvcs::UserInfoMatcher::ChannelWeight& ch_weight,
      const AdServer::ChannelSvcs::ChannelServerBase::ContentChannelAtom& contextual_channel)
      const
    {
      return ch_weight.channel_id < contextual_channel.id;
    }

    bool
    operator()(
      const AdServer::ChannelSvcs::ChannelServerBase::ContentChannelAtom& contextual_channel,
      const AdServer::UserInfoSvcs::UserInfoMatcher::ChannelWeight& ch_weight)
      const
    {
      return contextual_channel.id < ch_weight.channel_id;
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
    operator()(const AdServer::ChannelSvcs::
      ChannelServerBase::ContentChannelAtom& contextual_channel)
      const
    {
      AdServer::UserInfoSvcs::UserInfoMatcher::ChannelWeight res;
      res.channel_id = contextual_channel.id;
      res.weight = contextual_channel.weight;
      return res;
    }
  };

  void
  AdFrontend::acquire_user_info_matcher(
    const RequestInfo& request_info,
    const AdServer::ChannelSvcs::ChannelServerBase::MatchResult*
      trigger_matching_result,
    AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult_out match_result_out,
    bool& profiling_available,
    RequestTimeMetering& request_time_metering)
    noexcept
  {
    static const char* FUN = "AdFrontend::acquire_user_info_matcher()";

    bool match_success = false;

    AdServer::UserInfoSvcs::UserInfoMatcher_var
      uim_session = user_info_client_->user_info_session();
    AdServer::UserInfoSvcs::GrpcUserInfoOperationDistributor_var
      grpc_distributor = user_info_client_->grpc_distributor();

    AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult_var match_result;

    bool do_history_matching =
      request_info.user_status == AdServer::CampaignSvcs::US_OPTIN ||
      request_info.user_status == AdServer::CampaignSvcs::US_TEMPORARY;

    bool is_grpc_success = false;
    if (grpc_distributor && do_history_matching)
    {
      using MatchParams = AdServer::UserInfoSvcs::Types::MatchParams;
      using UserInfo = AdServer::UserInfoSvcs::Types::UserInfo;
      using ChannelMatchSet = std::set<ChannelMatch>;

      try
      {
        is_grpc_success = true;
        TimeGuard history_match_time_metering;

        MatchParams match_params;
        match_params.use_empty_profile = false;
        match_params.silent_match = request_info.silent_match;
        match_params.no_match = request_info.no_match
          || (trigger_matching_result && trigger_matching_result->no_track);
        match_params.no_result = request_info.no_result;
        match_params.ret_freq_caps = request_info.tag_id != 0;
        match_params.provide_channel_count = false;
        match_params.provide_persistent_channels = false;
        match_params.change_last_request = true;
        match_params.filter_contextual_triggers = false;
        match_params.publishers_optin_timeout =
          request_info.tag_id != 0 ? (request_info.current_time - Generics::Time::ONE_DAY * 15) :
          Generics::Time::ZERO;

        if (request_info.coord_location.in())
        {
          const auto& latitude = request_info.coord_location->latitude;
          const auto& longitude = request_info.coord_location->longitude;
          const auto& accuracy = request_info.coord_location->accuracy;

          match_params.geo_data_seq.emplace_back(latitude, longitude, accuracy);
        }

        if(request_info.tag_id == 0 || config_->ad_request_profiling())
        {
          if(trigger_matching_result && !trigger_matching_result->no_track)
          {
            ChannelMatchSet url_channels;
            ChannelMatchSet page_channels;
            ChannelMatchSet search_channels;
            ChannelMatchSet url_keyword_channels;

            std::transform(
              trigger_matching_result->matched_channels.url_channels.get_buffer(),
              trigger_matching_result->matched_channels.url_channels.get_buffer() +
                trigger_matching_result->matched_channels.url_channels.length(),
              std::inserter(url_channels, url_channels.end()),
              GetChannelTriggerId());

            std::transform(
              trigger_matching_result->matched_channels.page_channels.get_buffer(),
              trigger_matching_result->matched_channels.page_channels.get_buffer() +
                trigger_matching_result->matched_channels.page_channels.length(),
              std::inserter(page_channels, page_channels.end()),
              GetChannelTriggerId());

            std::transform(
              trigger_matching_result->matched_channels.search_channels.get_buffer(),
              trigger_matching_result->matched_channels.search_channels.get_buffer() +
                trigger_matching_result->matched_channels.search_channels.length(),
              std::inserter(search_channels, search_channels.end()),
              GetChannelTriggerId());

            std::transform(
              trigger_matching_result->matched_channels.url_keyword_channels.get_buffer(),
              trigger_matching_result->matched_channels.url_keyword_channels.get_buffer() +
                trigger_matching_result->matched_channels.url_keyword_channels.length(),
              std::inserter(url_keyword_channels, url_keyword_channels.end()),
              GetChannelTriggerId());

            match_params.url_channel_ids.reserve(
              url_channels.size() + request_info.hit_channel_ids.size());
            for (const auto& url_channel : url_channels)
            {
              match_params.url_channel_ids.emplace_back(
                url_channel.channel_id,
                url_channel.channel_trigger_id);
            }

            match_params.page_channel_ids.reserve(
              page_channels.size() + request_info.hit_channel_ids.size());
            for (const auto& page_channel : page_channels)
            {
              match_params.page_channel_ids.emplace_back(
                page_channel.channel_id,
                page_channel.channel_trigger_id);
            }

            match_params.search_channel_ids.reserve(
              search_channels.size() + request_info.hit_channel_ids.size());
            for (const auto& search_channel : search_channels)
            {
              match_params.search_channel_ids.emplace_back(
                search_channel.channel_id,
                search_channel.channel_trigger_id);
            }

            match_params.url_keyword_channel_ids.reserve(
              url_keyword_channels.size() + request_info.hit_channel_ids.size());
            for (const auto& url_keyword_channel : url_keyword_channels)
            {
              match_params.url_keyword_channel_ids.emplace_back(
                url_keyword_channel.channel_id,
                url_keyword_channel.channel_trigger_id);
            }

            match_params.persistent_channel_ids.reserve(
              request_info.platform_ids.size());
            match_params.persistent_channel_ids.insert(
              std::end(match_params.persistent_channel_ids),
              std::begin(request_info.platform_ids),
              std::end(request_info.platform_ids));
          }

          for (const auto& hit_channel_id : request_info.hit_channel_ids)
          {
            match_params.url_channel_ids.emplace_back(hit_channel_id, 0);
            match_params.url_keyword_channel_ids.emplace_back(hit_channel_id, 0);
            match_params.page_channel_ids.emplace_back(hit_channel_id, 0);
            match_params.search_channel_ids.emplace_back(hit_channel_id, 0);
          }
        }

        match_params.cohort = request_info.curct;

        /* process match & merge request */
        UserInfo user_info;
        user_info.user_id = GrpcAlgs::pack_user_id(request_info.client_id);
        user_info.huser_id = GrpcAlgs::pack_user_id(request_info.household_client_id);
        user_info.last_colo_id = request_info.last_colo_id;
        user_info.request_colo_id =
          request_info.user_status != AdServer::CampaignSvcs::US_TEMPORARY ?
        request_info.colo_id : -1;
        user_info.current_colo_id = -1;
        user_info.temporary =
          request_info.user_status == AdServer::CampaignSvcs::US_TEMPORARY;
        user_info.time = request_info.current_time.tv_sec;

        auto response = grpc_distributor->match(user_info, match_params);
        if (!response || response->has_error())
        {
          GrpcAlgs::print_grpc_error_response(
            response,
            logger(),
            Aspect::AD_FRONTEND);
          throw Exception("match is failed");
        }

        const auto& info_proto = response->info();
        const auto& match_result_proto = info_proto.match_result();

        request_time_metering.matched_channels =
          match_result_proto.channels().size();
        request_time_metering.history_match_time =
          history_match_time_metering.consider();

        request_time_metering.history_match_local_time =
          GrpcAlgs::unpack_time(match_result_proto.process_time());

        match_result = AdServer::UserInfoSvcs::convertor_proto_match_result(match_result_proto);
        match_success = true;

        /* log user info request */
        if (logger()->log_level() >= TraceLevel::MIDDLE)
        {
          const auto& channels = match_result_proto.channels();

          std::ostringstream ostream;
          ostream << FUN
                  << ": history matched channels for uid = '"
                  << request_info.client_id
                  << "': ";

          if(channels.size() == 0)
          {
            ostream << "empty";
          }
          else
          {
            GrpcAlgs::print_repeated_fields(
              ostream,
              ",",
              ":",
              channels,
              &AdServer::UserInfoSvcs::Proto::ChannelWeight::channel_id);
          }

          logger()->log(
            ostream.str(),
            TraceLevel::MIDDLE,
            Aspect::AD_FRONTEND);
        }
      }
      catch (const eh::Exception& exc)
      {
        is_grpc_success = false;
        Stream::Error stream;
        stream << FUN
               << ": "
               << exc.what();
        logger()->error(stream.str(), Aspect::AD_FRONTEND);
      }
      catch (...)
      {
        is_grpc_success = false;
        Stream::Error stream;
        stream << FUN
               << ": Unknown error";
        logger()->error(stream.str(), Aspect::AD_FRONTEND);
      }
    }

    if(!is_grpc_success && uim_session.in() && do_history_matching)
    {
      try
      {
        TimeGuard history_match_time_metering;

        AdServer::UserInfoSvcs::UserInfoMatcher::MatchParams match_params;
        match_params.use_empty_profile = false;
        match_params.silent_match = request_info.silent_match;
        match_params.no_match = request_info.no_match ||
          (trigger_matching_result && trigger_matching_result->no_track);
        match_params.no_result = request_info.no_result;
        match_params.ret_freq_caps = request_info.tag_id != 0;
        match_params.provide_channel_count = false;
        match_params.provide_persistent_channels = false;
        match_params.change_last_request = true;
        match_params.filter_contextual_triggers = false;
        match_params.publishers_optin_timeout = request_info.tag_id != 0 ?
          CorbaAlgs::pack_time(request_info.current_time - Generics::Time::ONE_DAY * 15) :
          CorbaAlgs::pack_time(Generics::Time::ZERO);

        if (request_info.coord_location.in())
        {
          match_params.geo_data_seq.length(1);
          match_params.geo_data_seq[0].latitude =
            CorbaAlgs::pack_decimal<AdServer::CampaignSvcs::CoordDecimal>(
              request_info.coord_location->latitude);
          match_params.geo_data_seq[0].longitude =
            CorbaAlgs::pack_decimal<AdServer::CampaignSvcs::CoordDecimal>(
              request_info.coord_location->longitude);
          match_params.geo_data_seq[0].accuracy =
            CorbaAlgs::pack_decimal<AdServer::CampaignSvcs::AccuracyDecimal>(
              request_info.coord_location->accuracy);
        }

        if(request_info.tag_id == 0 || config_->ad_request_profiling())
        {
          prepare_ui_match_params_(
            match_params,
            trigger_matching_result,
            request_info);
        }

        match_params.cohort << request_info.curct;

        /* process match & merge request */
        AdServer::UserInfoSvcs::UserInfo user_info;

        user_info.user_id = CorbaAlgs::pack_user_id(request_info.client_id);
        user_info.huser_id = CorbaAlgs::pack_user_id(request_info.household_client_id);
        user_info.last_colo_id = request_info.last_colo_id;
        user_info.request_colo_id =
          request_info.user_status != AdServer::CampaignSvcs::US_TEMPORARY ?
          request_info.colo_id : -1;
        user_info.current_colo_id = -1;
        user_info.temporary =
          request_info.user_status == AdServer::CampaignSvcs::US_TEMPORARY;
        user_info.time = request_info.current_time.tv_sec;

        /* get merge target profile, if need */
        uim_session->match(
          user_info,
          match_params,
          match_result.out());

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
    else if(!uim_session.in())
    {
      logger()->log(
        String::SubString("Match with non resolved user info session."),
        TraceLevel::MIDDLE,
        Aspect::AD_FRONTEND);
    }

    if(trigger_matching_result && !trigger_matching_result->no_track)
    {
      if(!match_success || !do_history_matching)
      {
        match_result = get_empty_history_matching();

        /* fill history channels with context channels */
        AdServer::UserInfoSvcs::UserInfoMatcher::ChannelWeightSeq&
          history_matched_channels = match_result->channels;
        const AdServer::ChannelSvcs::ChannelServerBase::ContentChannelAtomSeq&
          content_channels = trigger_matching_result->content_channels;

        history_matched_channels.length(content_channels.length());

        std::copy(content_channels.get_buffer(),
          content_channels.get_buffer() + content_channels.length(),
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
        const AdServer::ChannelSvcs::ChannelServerBase::ContentChannelAtomSeq&
          content_channels = trigger_matching_result->content_channels;
        AdServer::UserInfoSvcs::UserInfoMatcher::ChannelWeightSeq result_channels;
        result_channels.length(
          history_matched_channels.length() + content_channels.length());

        AdServer::UserInfoSvcs::UserInfoMatcher::ChannelWeight* last_ins =
          Algs::merge_unique(
            history_matched_channels.get_buffer(),
            history_matched_channels.get_buffer() + history_matched_channels.length(),
            content_channels.get_buffer(),
            content_channels.get_buffer() + content_channels.length(),
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
    const AdServer::CampaignSvcs::CampaignManager::RequestCreativeResult&
      campaign_select_result)
    noexcept
  {
    static const char* FUN = "AdFrontend::user_info_post_match_()";

    Generics::Timer timer;
    timer.start();

    CORBA::ULong seq_order_len = 0;

    for(CORBA::ULong ad_slot_i = 0;
        ad_slot_i < campaign_select_result.ad_slots.length(); ++ad_slot_i)
    {
      const AdServer::CampaignSvcs::CampaignManager::
        AdSlotResult& ad_slot = campaign_select_result.ad_slots[ad_slot_i];

      for(CORBA::ULong i = 0;
          i < ad_slot.selected_creatives.length(); ++i)
      {
        if(ad_slot.selected_creatives[i].order_set_id)
        {
          ++seq_order_len;
        }
      }
    }

    AdServer::UserInfoSvcs::GrpcUserInfoOperationDistributor_var
      grpc_distributor = user_info_client_->grpc_distributor();

    bool is_grpc_success = false;
    if (grpc_distributor)
    {
      using CampaignIds = AdServer::UserInfoSvcs::Types::CampaignIds;
      using SeqOrders = AdServer::UserInfoSvcs::Types::SeqOrders;
      using FreqCaps = AdServer::UserInfoSvcs::Types::FreqCaps;

      try
      {
        is_grpc_success = true;

        for(CORBA::ULong ad_slot_i = 0;
             ad_slot_i < campaign_select_result.ad_slots.length();
             ++ad_slot_i)
        {
          const auto& ad_slot_result = campaign_select_result.ad_slots[ad_slot_i];

          if(ad_slot_result.selected_creatives.length() > 0)
          {
            CampaignIds campaign_ids;
            campaign_ids.reserve(ad_slot_result.selected_creatives.length());

            SeqOrders seq_orders;
            seq_orders.reserve(ad_slot_result.selected_creatives.length());

            for(CORBA::ULong creative_i = 0;
              creative_i < ad_slot_result.selected_creatives.length();
              ++creative_i)
            {
              const auto& creative =
                ad_slot_result.selected_creatives[creative_i];
              if(creative.order_set_id)
              {
                seq_orders.emplace_back(
                  creative.cmp_id,
                  creative.order_set_id,
                  1);
              }

              campaign_ids.emplace_back(creative.campaign_group_id);
            }

            FreqCaps freq_caps;
            freq_caps.insert(
              std::end(freq_caps),
              ad_slot_result.freq_caps.get_buffer(),
              ad_slot_result.freq_caps.get_buffer() + ad_slot_result.freq_caps.length());

            FreqCaps uc_freq_caps;
            uc_freq_caps.insert(
              std::end(uc_freq_caps),
              ad_slot_result.uc_freq_caps.get_buffer(),
              ad_slot_result.uc_freq_caps.get_buffer() + ad_slot_result.uc_freq_caps.length());

            std::string request_id;
            if (ad_slot_result.request_id.length() != 0)
            {
              request_id.resize(ad_slot_result.request_id.length());
              std::memcpy(
                request_id.data(),
                ad_slot_result.request_id.get_buffer(),
                ad_slot_result.request_id.length());
            }

            auto response = grpc_distributor->update_user_freq_caps(
              GrpcAlgs::pack_user_id(request_info.client_id),
              request_info.current_time,
              request_id,
              freq_caps,
              uc_freq_caps,
              FreqCaps{},
              seq_orders,
              ad_slot_result.track_impr ? CampaignIds{} : campaign_ids,
              ad_slot_result.track_impr ? campaign_ids : CampaignIds{});
            if (!response || response->has_error())
            {
              is_grpc_success = false;
              GrpcAlgs::print_grpc_error_response(
                response,
                logger(),
                Aspect::AD_FRONTEND);
            }
          }
        }
      }
      catch (const eh::Exception& exc)
      {
        is_grpc_success = false;
        Stream::Error stream;
        stream << FUN
               << ": "
               << exc.what();
        logger()->error(stream.str(), Aspect::AD_FRONTEND);
      }
      catch (...)
      {
        is_grpc_success = false;
        Stream::Error stream;
        stream << FUN
               << ": Unknown error";
        logger()->error(stream.str(), Aspect::AD_FRONTEND);
      }

      if (is_grpc_success)
      {
        timer.stop();
        request_time_metering.history_post_match_time = timer.elapsed_time();
      }
    }

    if (is_grpc_success)
      return;

    try
    {
      AdServer::UserInfoSvcs::UserInfoMatcher_var uim_session =
        user_info_client_->user_info_session();

      if(!uim_session.in())
      {
        logger()->log(
          String::SubString("AdFrontend::user_info_post_match_():"
            " non resolved user info session."),
          Logging::Logger::TRACE,
          Aspect::AD_FRONTEND);

        return;
      }

      for(CORBA::ULong ad_slot_i = 0;
          ad_slot_i < campaign_select_result.ad_slots.length(); ++ad_slot_i)
      {
        const AdServer::CampaignSvcs::CampaignManager::
          AdSlotResult& ad_slot_result = campaign_select_result.ad_slots[ad_slot_i];

        if(ad_slot_result.selected_creatives.length() > 0)
        {
          UserInfoSvcs::CampaignIdSeq campaign_ids;
          campaign_ids.length(ad_slot_result.selected_creatives.length());

          UserInfoSvcs::UserInfoManager::SeqOrderSeq seq_orders;
          CORBA::ULong result_seq_order_i = 0;

          for(CORBA::ULong creative_i = 0;
            creative_i < ad_slot_result.selected_creatives.length();
            ++creative_i)
          {
            const AdServer::CampaignSvcs::CampaignManager::
              CreativeSelectResult& creative =
                ad_slot_result.selected_creatives[creative_i];

            if(creative.order_set_id)
            {
              seq_orders[result_seq_order_i].ccg_id = creative.cmp_id;
              seq_orders[result_seq_order_i].set_id = creative.order_set_id;
              seq_orders[result_seq_order_i].imps = 1;

              ++result_seq_order_i;
            }

            campaign_ids[creative_i] = creative.campaign_group_id;
          }

          UserInfoSvcs::FreqCapIdSeq freq_caps;
          UserInfoSvcs::FreqCapIdSeq uc_freq_caps;

          CorbaAlgs::copy_sequence(ad_slot_result.freq_caps, freq_caps);
          CorbaAlgs::copy_sequence(ad_slot_result.uc_freq_caps, uc_freq_caps);

          uim_session->update_user_freq_caps(
            CorbaAlgs::pack_user_id(request_info.client_id),
            CorbaAlgs::pack_time(request_info.current_time),
            ad_slot_result.request_id,
            freq_caps,
            uc_freq_caps,
            UserInfoSvcs::FreqCapIdSeq(),
            seq_orders,
            ad_slot_result.track_impr ? EMPTY_CAMPAIGN_ID_SEQ : campaign_ids,
            ad_slot_result.track_impr ? campaign_ids : EMPTY_CAMPAIGN_ID_SEQ);
        } // ad_slot_result.selected_creatives.length() > 0
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

  AdServer::ChannelSvcs::ChannelServerBase::MatchQuery*
  AdFrontend::get_empty_matching_query()
    /*throw(eh::Exception)*/
  {
    AdServer::ChannelSvcs::ChannelServerBase::MatchQuery_var res;
    res = new AdServer::ChannelSvcs::ChannelServerBase::MatchQuery;
    res->non_strict_word_match = false;
    res->non_strict_url_match = false;
    res->return_negative = false;
    res->simplify_page = true;
    res->fill_content = true;
    res->statuses[0] = 'A';
    res->statuses[1] = '\0';
    return res._retn();
  }

  void
  AdFrontend::match_triggers_(
    RequestTimeMetering& request_time_metering,
    AdServer::ChannelSvcs::ChannelServerBase::MatchQuery* query,
    AdServer::ChannelSvcs::ChannelServerBase::MatchResult_out
      trigger_matched_channels,
    const RequestInfo& request_info)
    /*throw(Exception)*/
  {
    static const char* FUN = "AdFrontend::match_triggers_()";

    try
    {
      TimeGuard trigger_match_time_metering;

      AdServer::ChannelSvcs::ChannelServerBase::MatchQuery& in = *query;

      in.first_url << request_info.referer;
      in.first_url_words << request_info.referer_url_words;
      request_time_metering.recived_triggers = request_info.referer.empty() ? 0 : 1;

      /* only referer matching m.b. used for opted out clients */
      if(request_info.full_text_words.empty())
      {
        in.pwords << request_info.page_words;
        request_time_metering.recived_triggers +=
          request_info.page_words.size();
      }
      else
      {
        in.pwords << request_info.full_text_words;
        request_time_metering.recived_triggers +=
          request_info.full_text_words.size();
      }
      in.swords << request_info.search_words;

      in.uid = CorbaAlgs::pack_user_id(request_info.client_id);

      channel_servers_->match(in, trigger_matched_channels);

      request_time_metering.matched_triggers =
        trigger_matched_channels->matched_channels.page_channels.length() +
        trigger_matched_channels->matched_channels.search_channels.length() +
        trigger_matched_channels->matched_channels.url_channels.length() +
        trigger_matched_channels->matched_channels.url_keyword_channels.length() +
        trigger_matched_channels->matched_channels.uid_channels.length();

      request_time_metering.trigger_match_time =
        trigger_match_time_metering.consider();

      request_time_metering.detail_trigger_match_time.resize(
        trigger_matched_channels->match_time.length());
      
      for(size_t i = 0;
          i < request_time_metering.detail_trigger_match_time.size(); i++)
      {
        request_time_metering.detail_trigger_match_time[i] =
          CorbaAlgs::unpack_time(trigger_matched_channels->match_time[i]);
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
          trigger_matched_channels->matched_channels.page_channels, 'P',  ostr);
        fill_debug_channels_(
          trigger_matched_channels->matched_channels.search_channels, 'S',  ostr);
        fill_debug_channels_(
          trigger_matched_channels->matched_channels.url_channels, 'U',  ostr);
        fill_debug_channels_(
          trigger_matched_channels->matched_channels.url_keyword_channels, 'R',  ostr);
        ostr << std::endl;
        
      logger()->log(ostr.str(),
        TraceLevel::MIDDLE,
        Aspect::AD_FRONTEND);
      }
    }
    catch(const FrontendCommons::ChannelServerSessionPool::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN <<
        ": caught ChannelServerSessionPool::Exception: " <<
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
    const FrontendCommons::HttpRequest& request,
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

    AdServer::ChannelSvcs::ChannelServerBase::MatchQuery_var match_query;
    AdServer::ChannelSvcs::ChannelServerBase::MatchResult_var
      trigger_matched_channels;
    AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult_var
      history_match_result;
    bool profiling_available = false;

    bool merge_success = true;
    std::string merge_error_message;
    Generics::Time merged_last_request;

    AdServer::CampaignSvcs::CampaignManager::RequestCreativeResult_var
      campaign_matching_result;

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

    match_query = get_empty_matching_query();
    if(request_info.keywords_normalized)
    {
      match_query->simplify_page = false;
    }

    if (request_info.passback_by_colocation)
    {
      // disable trigger matching by colocations flags
      trigger_matched_channels = get_empty_trigger_matching();
    }
    else
    {
      // do trigger based channels matching
      match_triggers_(
        request_time_metering,
        match_query,
        trigger_matched_channels.out(),
        request_info);

      // do history based channels matching
      acquire_user_info_matcher(
        request_info,
        trigger_matched_channels.ptr(),
        history_match_result.out(),
        profiling_available,
        request_time_metering);
    }

    if(!history_match_result.ptr())
    {
      history_match_result = get_empty_history_matching();
    }

    AdServer::ChannelSvcs::ChannelServerBase::CCGKeywordSeq_var ccg_keywords;
    AdServer::ChannelSvcs::ChannelServerBase::CCGKeywordSeq_var hid_ccg_keywords;

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

          ccg_keywords = channel_servers_->get_ccg_traits(channel_ids);
        }

        if(history_match_result->hid_channels.length())
        {
          AdServer::ChannelSvcs::ChannelIdSeq channel_ids;
          channel_ids.length(history_match_result->hid_channels.length());

          for (CORBA::ULong i = 0;
            i < history_match_result->hid_channels.length(); ++i)
          {
            channel_ids[i] = history_match_result->hid_channels[i].channel_id;
          }

          hid_ccg_keywords = channel_servers_->get_ccg_traits(channel_ids);
        }
      }
      catch(const FrontendCommons::ChannelServerSessionPool::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN <<
          ": caught ChannelServerSessionPool::Exception: " <<
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
      trigger_matched_channels,
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
      trigger_matched_channels.ptr(),
      history_match_result.ptr(),
      merge_success ? merged_last_request :
        CorbaAlgs::unpack_time(history_match_result->last_request_time),
      profiling_available,
      ccg_keywords,
      hid_ccg_keywords,
      debug_sink);

    if(request_info.user_status == AdServer::CampaignSvcs::US_OPTIN &&
       campaign_matching_result)
    {
      user_info_post_match_(
        request_time_metering,
        request_info,
        *campaign_matching_result);
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
        response.add_header(
          Response::Header::MERGE_FAILED,
          merge_error_message);
      }

      debug_sink->print_creative_selection_debug_info(
        request_info,
        passback_info,
        campaign_matching_result,
        request_time_metering);

      if(campaign_matching_result &&
         campaign_matching_result->ad_slots.length() > 0)
      {
        if(campaign_matching_result->ad_slots[0].creative_body[0])
        {
          const AdServer::CampaignSvcs::CampaignManager::AdSlotResult&
            ad_slot_result = campaign_matching_result->ad_slots[0];

          str_response = ad_slot_result.creative_body;

          if(ad_slot_result.mime_format[0])
          {
            response.set_content_type(String::SubString(ad_slot_result.mime_format.in()));
          }
          else
          {
            response.set_content_type(Response::Type::TEXT_HTML);
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
    AdServer::CampaignSvcs::CampaignManager::CCGKeywordSeq& ccg_keywords,
    const AdServer::ChannelSvcs::ChannelServerBase::CCGKeywordSeq* src_ccg_keywords)
    noexcept
  {
    if(src_ccg_keywords)
    {
      ccg_keywords.length(src_ccg_keywords->length());
      for(CORBA::ULong i = 0; i < src_ccg_keywords->length(); ++i)
      {
        const AdServer::ChannelSvcs::ChannelServerBase::CCGKeyword&
          src_ccg_kw = (*src_ccg_keywords)[i];
        AdServer::CampaignSvcs::CampaignManager::CCGKeywordInfo&
          res_ccg_kw = ccg_keywords[i];
        res_ccg_kw.ccg_keyword_id = src_ccg_kw.ccg_keyword_id;
        res_ccg_kw.ccg_id = src_ccg_kw.ccg_id;
        res_ccg_kw.channel_id = src_ccg_kw.channel_id;
        res_ccg_kw.max_cpc = src_ccg_kw.max_cpc;
        res_ccg_kw.ctr = src_ccg_kw.ctr;
        res_ccg_kw.click_url = src_ccg_kw.click_url;
        res_ccg_kw.original_keyword = src_ccg_kw.original_keyword;
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

    if(!request_info.client_id.is_null() && user_bind_client_)
    {
      AdServer::UserInfoSvcs::GrpcUserBindOperationDistributor_var
        grpc_distributor = user_bind_client_->grpc_distributor();

      if (grpc_distributor)
      {
        try
        {
          const std::string ext_user_id = std::string("c/") +
            request_info.client_id.to_string();

          auto response = grpc_distributor->get_user_id(
            ext_user_id,
            GrpcAlgs::pack_user_id(request_info.client_id),
            request_info.current_time,
            Generics::Time::ZERO,
            true,
            false,
            true);
          if (response && response->has_info())
          {
            const auto& info_proto = response->info();
            resolved_user_id = GrpcAlgs::unpack_user_id(info_proto.user_id());
            common_module_->user_id_controller()->null_blacklisted(resolved_user_id);

            return !resolved_user_id.is_null();
          }
          else
          {
            GrpcAlgs::print_grpc_error_response(
              response,
              logger(),
              Aspect::AD_FRONTEND);
          }
        }
        catch (const eh::Exception& exc)
        {
          Stream::Error stream;
          stream << FUN
                 << ": "
                 << exc.what();
          logger()->error(stream.str(), Aspect::AD_FRONTEND);
        }
        catch (...)
        {
          Stream::Error stream;
          stream << FUN
                 << ": Unknown error";
          logger()->error(stream.str(), Aspect::AD_FRONTEND);
        }
      }

      try
      {
        AdServer::UserInfoSvcs::UserBindMapper_var user_bind_mapper =
          user_bind_client_->user_bind_mapper();

        const std::string ext_user_id = std::string("c/") +
          request_info.client_id.to_string();

        AdServer::UserInfoSvcs::UserBindMapper::GetUserRequestInfo get_request_info;
        get_request_info.id << ext_user_id;
        get_request_info.timestamp = CorbaAlgs::pack_time(request_info.current_time);
        get_request_info.silent = true;
        get_request_info.generate_user_id = false;
        get_request_info.for_set_cookie = true;
        get_request_info.create_timestamp = CorbaAlgs::pack_time(Generics::Time::ZERO);
        get_request_info.current_user_id = CorbaAlgs::pack_user_id(request_info.client_id);

        AdServer::UserInfoSvcs::UserBindServer::GetUserResponseInfo_var user_bind_info =
          user_bind_mapper->get_user_id(get_request_info);

        resolved_user_id = CorbaAlgs::unpack_user_id(user_bind_info->user_id);

        common_module_->user_id_controller()->null_blacklisted(resolved_user_id);

        return !resolved_user_id.is_null();
      }
      catch(const AdServer::UserInfoSvcs::UserBindMapper::NotReady& )
      {
        Stream::Error ostr;
        ostr << FUN << ": caught UserBindMapper::NotReady";

        logger()->log(ostr.str(),
          Logging::Logger::WARNING,
          Aspect::AD_FRONTEND,
          "ADS-IMPL-10681");
      }
      catch(const AdServer::UserInfoSvcs::UserBindMapper::ChunkNotFound& )
      {
        Stream::Error ostr;
        ostr << FUN << ": caught UserBindMapper::ChunkNotFound";

        logger()->log(ostr.str(),
          Logging::Logger::ERROR,
          Aspect::AD_FRONTEND,
          "ADS-IMPL-10681");
      }
      catch(const AdServer::UserInfoSvcs::UserBindMapper::ImplementationException& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": caught UserBindMapper::ImplementationException: " <<
          ex.description;

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
    AdServer::CampaignSvcs::CampaignManager::RequestCreativeResult_var&
      campaign_matching_result,
    RequestTimeMetering& request_time_metering,
    const RequestInfo& request_info,
    const Generics::SubStringHashAdapter& instantiate_type,
    AdServer::ChannelSvcs::ChannelServerBase::MatchResult*
      trigger_matched_channels,
    AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult* history_match_result,
    const Generics::Time& /*merged_last_request*/,
    bool profiling_available,
    const AdServer::ChannelSvcs::ChannelServerBase::CCGKeywordSeq* ccg_keywords,
    const AdServer::ChannelSvcs::ChannelServerBase::CCGKeywordSeq* hid_ccg_keywords,
    DebugSink* debug_sink)
    /*throw(Exception)*/
  {
    static const char* FUN = "AdFrontend::request_campaign_manager_()";

    /* do campaign selection */
    try
    {
      AdServer::CampaignSvcs::CampaignManager::RequestParams request_params;

      if (trigger_matched_channels)
      {
        request_params.trigger_match_result.pkw_channels.length(
          trigger_matched_channels->matched_channels.page_channels.length());
        request_params.trigger_match_result.skw_channels.length(
          trigger_matched_channels->matched_channels.search_channels.length());
        request_params.trigger_match_result.url_channels.length(
          trigger_matched_channels->matched_channels.url_channels.length());
        request_params.trigger_match_result.ukw_channels.length(
          trigger_matched_channels->matched_channels.url_keyword_channels.length());

        std::transform(
          trigger_matched_channels->matched_channels.page_channels.get_buffer(),
          trigger_matched_channels->matched_channels.page_channels.get_buffer() +
          trigger_matched_channels->matched_channels.page_channels.length(),
          request_params.trigger_match_result.pkw_channels.get_buffer(),
          convert_channel_atom);
        std::transform(
          trigger_matched_channels->matched_channels.search_channels.get_buffer(),
          trigger_matched_channels->matched_channels.search_channels.get_buffer() +
          trigger_matched_channels->matched_channels.search_channels.length(),
          request_params.trigger_match_result.skw_channels.get_buffer(),
          convert_channel_atom);
        std::transform(
          trigger_matched_channels->matched_channels.url_channels.get_buffer(),
          trigger_matched_channels->matched_channels.url_channels.get_buffer() +
          trigger_matched_channels->matched_channels.url_channels.length(),
          request_params.trigger_match_result.url_channels.get_buffer(),
          convert_channel_atom);
        std::transform(
          trigger_matched_channels->matched_channels.url_keyword_channels.get_buffer(),
          trigger_matched_channels->matched_channels.url_keyword_channels.get_buffer() +
          trigger_matched_channels->matched_channels.url_keyword_channels.length(),
          request_params.trigger_match_result.ukw_channels.get_buffer(),
          convert_channel_atom);
        CorbaAlgs::copy_sequence(
          trigger_matched_channels->matched_channels.uid_channels,
          request_params.trigger_match_result.uid_channels);
      }
      else
      {
        request_params.trigger_match_result.pkw_channels.length(0);
        request_params.trigger_match_result.skw_channels.length(0);
        request_params.trigger_match_result.url_channels.length(0);
        request_params.trigger_match_result.ukw_channels.length(0);
        request_params.trigger_match_result.uid_channels.length(0);
      }

      request_params.common_info.creative_instantiate_type << instantiate_type;
      FrontendCommons::fill_geo_location_info(
        request_params.common_info.location,
        request_info.location);

      if(history_match_result)
      {
        request_params.common_info.coord_location.length(
          history_match_result->geo_data_seq.length());
        for(CORBA::ULong i = 0;
            i < history_match_result->geo_data_seq.length(); ++i)
        {
          AdServer::CampaignSvcs::CampaignManager::GeoCoordInfo& res_loc =
            request_params.common_info.coord_location[i];
          res_loc.longitude = history_match_result->geo_data_seq[i].longitude;
          res_loc.latitude = history_match_result->geo_data_seq[i].latitude;
          res_loc.accuracy = history_match_result->geo_data_seq[i].accuracy;
        }
      }
      else
      {
        FrontendCommons::fill_geo_coord_location_info(
          request_params.common_info.coord_location,
          request_info.coord_location);
      }

      request_params.common_info.user_id = CorbaAlgs::pack_user_id(
        request_info.user_status != AdServer::CampaignSvcs::US_PROBE ?
        request_info.client_id :
        AdServer::Commons::UserId());
      request_params.common_info.track_user_id =
        request_params.common_info.user_id;

      request_params.common_info.signed_user_id << request_info.signed_client_id;
      if(!request_info.temp_client_id.is_null() &&
         !request_info.client_id.is_null())
      {
        request_params.merged_user_id = CorbaAlgs::pack_user_id(
          request_info.temp_client_id);
      }

      request_params.ad_instantiate_type = static_cast<CORBA::ULong>(
        AdServer::CampaignSvcs::AIT_BODY);
      request_params.fill_track_pixel = false;

      request_params.household_id = CorbaAlgs::pack_user_id(
        request_info.household_client_id);

      // reduce user status values
      request_params.common_info.user_status = static_cast<CORBA::ULong>(
        request_info.user_status);

      if(request_info.user_status == AdServer::CampaignSvcs::US_OPTIN && (
           trigger_matched_channels && (
             trigger_matched_channels->no_track ||
             trigger_matched_channels->no_adv)))
      {
        request_params.common_info.user_status = static_cast<CORBA::ULong>(
          AdServer::CampaignSvcs::US_BLACKLISTED);
      }

      request_params.client_create_time = history_match_result->create_time;
      request_params.common_info.full_referer << request_info.referer;
      request_params.common_info.referer << request_info.allowable_referer;
      request_params.context_info.full_referer_hash = request_info.full_referer_hash;
      request_params.context_info.short_referer_hash = request_info.short_referer_hash;
      request_params.common_info.cohort << request_info.curct;
      request_params.common_info.peer_ip << request_info.peer_ip;
      request_params.common_info.random = request_info.random;

      request_params.fraud = history_match_result->fraud_request &&
        !request_info.disable_fraud_detection;
      request_params.common_info.test_request = request_info.test_request ||
        request_info.disable_fraud_detection;
      request_params.common_info.log_as_test = request_info.log_as_test;
      request_params.disable_fraud_detection = request_info.disable_fraud_detection;
      request_params.profiling_available = profiling_available;

      request_params.search_engine_id = request_info.search_engine_id;
      request_params.page_keywords_present = !request_info.page_words.empty() ||
        !request_info.full_text_words.empty();

      // sample requests
      if ((((double)request_info.random * 100.0) / CampaignSvcs::RANDOM_PARAM_MAX)
          <= common_config_->profiling_log_sampling())
      {
        bool added = false;
        if (!request_info.full_text_words.empty())
        {
          request_params.page_keywords << request_info.full_text_words;
          added = true;
        }
        if (!request_info.page_words.empty())
        {
          if (added)
          {
            request_params.page_keywords << String::SubString(" ");
          }
          request_params.page_keywords << request_info.page_words;
          added = true;
        }

        if (!request_info.referer_url_words.empty())
        {
          request_params.url_keywords << request_info.referer_url_words;
        }
      }

      request_params.common_info.colo_id = request_info.colo_id;

      request_params.common_info.original_url << request_info.original_url;
      request_params.common_info.request_id = CorbaAlgs::pack_request_id(
        request_info.request_id);
      request_params.common_info.time = CorbaAlgs::pack_time(
        request_info.current_time);
      
      request_params.common_info.user_agent << request_info.user_agent;

      // fill request_params.context_info
      request_params.context_info.enabled_notice = false;
      request_params.context_info.profile_referer = false;
      request_params.context_info.client << request_info.client_app;
      request_params.context_info.client_version << request_info.client_app_version;
      request_params.context_info.web_browser << request_info.web_browser;
      CorbaAlgs::fill_sequence(
        request_info.platform_ids.begin(),
        request_info.platform_ids.end(),
        request_params.context_info.platform_ids);
      request_params.context_info.platform << request_info.platform;
      request_params.context_info.full_platform << request_info.full_platform;
      request_params.context_info.page_load_id = request_info.page_load_id;
      if(common_config_->ip_logging_enabled())
      {
        std::string ip_hash;
        FrontendCommons::ip_hash(ip_hash, request_info.peer_ip, common_config_->ip_salt());
        request_params.context_info.ip_hash << ip_hash;
      }

      request_params.full_freq_caps.swap(history_match_result->full_freq_caps);

      request_params.seq_orders.length(history_match_result->seq_orders.length());
      for(CORBA::ULong seq_order_i = 0;
          seq_order_i != history_match_result->seq_orders.length();
          ++seq_order_i)
      {
        request_params.seq_orders[seq_order_i].ccg_id =
          history_match_result->seq_orders[seq_order_i].ccg_id;
        request_params.seq_orders[seq_order_i].set_id =
          history_match_result->seq_orders[seq_order_i].set_id;
        request_params.seq_orders[seq_order_i].imps =
          history_match_result->seq_orders[seq_order_i].imps;
      }

      request_params.campaign_freqs.swap(history_match_result->campaign_freqs);

      // required passback for non profiling requests
      request_params.common_info.passback_type << request_info.passback_type;
      request_params.common_info.passback_url << request_info.passback_url;
      request_params.common_info.security_token << request_info.request_token;
      request_params.common_info.preclick_url << request_info.preclick_url;
      request_params.common_info.pub_impr_track_url << request_info.pub_impr_track_url;
      request_params.common_info.request_type = AdServer::CampaignSvcs::AR_NORMAL;
      request_params.common_info.hpos = CampaignSvcs::UNDEFINED_PUB_POSITION_BOTTOM;
      request_params.common_info.set_cookie = true;

      request_params.publisher_site_id = 0;
      request_params.required_passback = (request_info.tag_id != 0);
      request_params.preview_ccid = request_info.ccid;

      // fill input channel sequence for CampaignManager
      request_params.channels.length(
        history_match_result->channels.length() +
        (trigger_matched_channels?
          trigger_matched_channels->matched_channels.uid_channels.length(): 0));

      CORBA::ULong j = 0;
      for (CORBA::ULong i = 0; i < history_match_result->channels.length(); ++i, ++j)
      {
        request_params.channels[j] = history_match_result->channels[i].channel_id;
      }
      if (trigger_matched_channels)
      {
        for (CORBA::ULong i = 0;
             i < trigger_matched_channels->matched_channels.uid_channels.length();
             ++i, ++j)
        {
          request_params.channels[j] =
            trigger_matched_channels->matched_channels.uid_channels[i];
        }
      }

      request_params.hid_channels.length(history_match_result->hid_channels.length());

      for (CORBA::ULong i = 0; i < history_match_result->hid_channels.length(); ++i)
      {
        request_params.hid_channels[i] = history_match_result->hid_channels[i].channel_id;
      }

      CorbaAlgs::copy_sequence(
        history_match_result->exclude_pubpixel_accounts,
        request_params.exclude_pubpixel_accounts);

      convert_ccg_keywords_(request_params.ccg_keywords, ccg_keywords);
      convert_ccg_keywords_(request_params.hid_ccg_keywords, hid_ccg_keywords);

      request_params.search_words << request_info.search_words;
      request_params.need_debug_info = debug_sink->require_debug_info();
      request_params.session_start = history_match_result->session_start;
      request_params.only_display_ad = false;
      request_params.profiling_type = AdServer::CampaignSvcs::PT_ALL;

      if(request_info.tag_id)
      {
        // initialize slot
        request_params.ad_slots.length(1);
        AdServer::CampaignSvcs::CampaignManager::AdSlotInfo& ad_slot =
          request_params.ad_slots[0];
        ad_slot.format << request_info.format;
        ad_slot.tag_id = request_info.tag_id;
        ad_slot.passback =
          request_info.do_passback ||
          request_info.passback_by_colocation ||
          history_match_result->fraud_request ||
          (trigger_matched_channels &&
            (trigger_matched_channels->no_track ||
              trigger_matched_channels->no_adv));
        ad_slot.ext_tag_id << request_info.ext_tag_id;
        ad_slot.min_ecpm = CorbaAlgs::pack_decimal<CampaignSvcs::RevenueDecimal>(
          CampaignSvcs::RevenueDecimal::ZERO);

        ad_slot.up_expand_space = request_info.up_expand_space.present() ?
          static_cast<long>(*request_info.up_expand_space) : -1;
        ad_slot.right_expand_space = request_info.right_expand_space.present() ?
          static_cast<long>(*request_info.right_expand_space) : -1;
        ad_slot.down_expand_space = request_info.down_expand_space.present() ?
          static_cast<long>(*request_info.down_expand_space) : -1;
        ad_slot.left_expand_space = request_info.left_expand_space.present() ?
          static_cast<long>(*request_info.left_expand_space) : -1;
        ad_slot.tag_visibility = request_info.tag_visibility.present() ?
          static_cast<long>(*request_info.tag_visibility) : -1;

        ad_slot.debug_ccg = request_info.debug_ccg;
        ad_slot.video_min_duration = 0;
        ad_slot.video_max_duration = -1;
        ad_slot.video_skippable_max_duration = -1;
        ad_slot.video_width = 0;
        ad_slot.video_height = 0;
        ad_slot.video_allow_skippable = true;
        ad_slot.video_allow_unskippable = true;        
      }

      TimeGuard creative_selection_time_metering;

      CORBA::String_var hostname;
      campaign_managers_.get_campaign_creative(
        request_params,
        hostname,
        campaign_matching_result);

      assert(campaign_matching_result->ad_slots.length() ==
        request_params.ad_slots.length());

      request_time_metering.creative_selection_local_time =
        CorbaAlgs::unpack_time(campaign_matching_result->process_time);

      if(campaign_matching_result->ad_slots.length() > 0)
      {
        AdServer::CampaignSvcs::CampaignManager::
          AdSlotResult& ad_slot_result = campaign_matching_result->ad_slots[0];

        if(ad_slot_result.passback &&
           ad_slot_result.passback_url[0] != 0)
        {
          passback_info.url = ad_slot_result.passback_url.in();
        }

        log_as_test |= ad_slot_result.test_request;

        request_time_metering.creative_selection_time =
          creative_selection_time_metering.consider();

        request_time_metering.creative_count =
          ad_slot_result.selected_creatives.length();
        request_time_metering.passback =
          ad_slot_result.passback;
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
    const ChannelSvcs::ChannelServerBase::ChannelAtomSeq& in,
    char type,
    DebugStream& out)
    /*throw(eh::Exception)*/
  {
    size_t count = 0;
    for(size_t i = 0; i < in.length(); i++)
    {
      if(count)
      {
        out << ",";
      }
      out << in[i].id << type;
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
    const FrontendCommons::HttpRequest& request,
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

      campaign_managers_.verify_opt_operation(
        request_info.current_time.tv_sec,
        request_info.colo_id,
        "",
        AdServer::CampaignSvcs::CampaignManager::OO_OUT,
        11,
        CampaignSvcs::US_OPTOUT,
        request_info.log_as_test,
        request_info.web_browser.c_str(),
        request_info.full_platform.c_str(),
        "",
        "");
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
      AdServer::CampaignSvcs::ColocationFlagsSeq_var colocations =
        campaign_managers_.get_colocation_flags();

      RequestInfoFiller::ColoFlagsMap_var new_colo_flags(
        new RequestInfoFiller::ColoFlagsMap());
      for (CORBA::ULong i = 0; i < colocations->length(); ++i)
      {
        RequestInfoFiller::ColoFlags colo_flags;
        colo_flags.flags = colocations[i].flags;
        colo_flags.hid_profile = colocations[i].hid_profile;
        new_colo_flags->insert(
          RequestInfoFiller::ColoFlagsMap::value_type(
            colocations[i].colo_id,
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
    AdServer::UserInfoSvcs::UserInfoMatcher::ChannelMatchSeq& result_channel_ids,
    const AdServer::CampaignSvcs::ChannelIdArray& hit_channels)
  {
    CORBA::ULong i = result_channel_ids.length();
    result_channel_ids.length(
      result_channel_ids.length() + hit_channels.size());
    for(auto it = hit_channels.begin(); it != hit_channels.end(); ++it, ++i)
    {
      result_channel_ids[i].channel_id = *it;
      result_channel_ids[i].channel_trigger_id = 0;
    }
  }
  
  void AdFrontend::prepare_ui_match_params_(
    AdServer::UserInfoSvcs::UserInfoMatcher::MatchParams& match_params,
    const AdServer::ChannelSvcs::ChannelServerBase::MatchResult* match_result,
    const RequestInfo& request_info)
    /*throw(eh::Exception)*/
  {
    typedef std::set<ChannelMatch> ChannelMatchSet;

    if(match_result && !match_result->no_track)
    {
      ChannelMatchSet url_channels;
      ChannelMatchSet page_channels;
      ChannelMatchSet search_channels;
      ChannelMatchSet url_keyword_channels;

      std::transform(
        match_result->matched_channels.url_channels.get_buffer(),
        match_result->matched_channels.url_channels.get_buffer() +
        match_result->matched_channels.url_channels.length(),
        std::inserter(url_channels, url_channels.end()),
        GetChannelTriggerId());

      std::transform(
        match_result->matched_channels.page_channels.get_buffer(),
        match_result->matched_channels.page_channels.get_buffer() +
        match_result->matched_channels.page_channels.length(),
        std::inserter(page_channels, page_channels.end()),
        GetChannelTriggerId());

      std::transform(
        match_result->matched_channels.search_channels.get_buffer(),
        match_result->matched_channels.search_channels.get_buffer() +
        match_result->matched_channels.search_channels.length(),
        std::inserter(search_channels, search_channels.end()),
        GetChannelTriggerId());

      std::transform(
        match_result->matched_channels.url_keyword_channels.get_buffer(),
        match_result->matched_channels.url_keyword_channels.get_buffer() +
        match_result->matched_channels.url_keyword_channels.length(),
        std::inserter(url_keyword_channels, url_keyword_channels.end()),
        GetChannelTriggerId());

      match_params.url_channel_ids.length(url_channels.size());
      CORBA::ULong i = 0;
      for (ChannelMatchSet::const_iterator it = url_channels.begin();
           it != url_channels.end(); ++it, ++i)
      {
        match_params.url_channel_ids[i].channel_id = it->channel_id;
        match_params.url_channel_ids[i].channel_trigger_id = it->channel_trigger_id;
      }

      match_params.page_channel_ids.length(page_channels.size());
      i = 0;
      for (ChannelMatchSet::const_iterator it = page_channels.begin();
           it != page_channels.end(); ++it, ++i)
      {
        match_params.page_channel_ids[i].channel_id = it->channel_id;
        match_params.page_channel_ids[i].channel_trigger_id = it->channel_trigger_id;
      }

      match_params.search_channel_ids.length(search_channels.size());
      i = 0;
      for (ChannelMatchSet::const_iterator it = search_channels.begin();
           it != search_channels.end(); ++it, ++i)
      {
        match_params.search_channel_ids[i].channel_id = it->channel_id;
        match_params.search_channel_ids[i].channel_trigger_id = it->channel_trigger_id;
      }

      match_params.url_keyword_channel_ids.length(url_keyword_channels.size());
      i = 0;
      for (ChannelMatchSet::const_iterator it = url_keyword_channels.begin();
           it != url_keyword_channels.end(); ++it, ++i)
      {
        match_params.url_keyword_channel_ids[i].channel_id = it->channel_id;
        match_params.url_keyword_channel_ids[i].channel_trigger_id =
          it->channel_trigger_id;
      }

      CorbaAlgs::fill_sequence(
        request_info.platform_ids.begin(),
        request_info.platform_ids.end(),
        match_params.persistent_channel_ids);
    }

    add_hit_channels_(
      match_params.url_channel_ids,
      request_info.hit_channel_ids);

    add_hit_channels_(
      match_params.url_keyword_channel_ids,
      request_info.hit_channel_ids);

    add_hit_channels_(
      match_params.page_channel_ids,
      request_info.hit_channel_ids);

    add_hit_channels_(
      match_params.search_channel_ids,
      request_info.hit_channel_ids);
  }

}
