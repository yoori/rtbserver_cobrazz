// STD
#include <algorithm>
#include <set>
#include <sstream>

// UNIXCOMMONS
#include <Generics/GnuHashTable.hpp>
#include <Generics/HashTableAdapters.hpp>
#include <Generics/Uuid.hpp>
#include <HTTP/HTTPCookie.hpp>
#include <Logger/StreamLogger.hpp>
#include <String/AsciiStringManip.hpp>
#include <String/StringManip.hpp>

// THIS
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>
#include <Commons/Algs.hpp>
#include <Commons/ErrorHandler.hpp>
#include <Commons/GrpcAlgs.hpp>
#include <Commons/UserInfoManip.hpp>
#include <Frontends/FrontendCommons/add_UID_cookie.hpp>
#include <Frontends/FrontendCommons/OptOutManip.hpp>
#include <Frontends/Modules/AdFrontendGrpc/AdFrontend.hpp>

namespace
{
  struct ChannelMatch final
  {
    explicit ChannelMatch(
      const std::uint32_t channel_id_val,
      const std::uint32_t channel_trigger_id_val)
      : channel_id(channel_id_val),
        channel_trigger_id(channel_trigger_id_val)
    {
    }

    bool operator<(const ChannelMatch& right) const
    {
      return
        (channel_id < right.channel_id ||
         (channel_id == right.channel_id &&
          channel_trigger_id < right.channel_trigger_id));
    }

    std::uint32_t channel_id;
    std::uint32_t channel_trigger_id;
  };

  struct GetChannelTriggerId final
  {
    ChannelMatch operator() (
      const AdServer::ChannelSvcs::Proto::ChannelAtom& atom) noexcept
    {
      return ChannelMatch(atom.id(), atom.trigger_channel_id());
    }
  };

  template<class Container, typename ModifyOpType>
  class BackInsertProtoIterator final:
    public std::iterator<std::output_iterator_tag, void, void, void, void>
  {
  public:
    BackInsertProtoIterator(
      Container& container,
      ModifyOpType modify_op)
      : container_(container),
        modify_op_(modify_op)
    {
    }

    template<typename Value>
    BackInsertProtoIterator& operator=(const Value& val)
    {
      if constexpr (std::is_rvalue_reference_v<decltype(std::declval<ModifyOpType&>()(
        std::declval<Value&>()))>)
      {
        container_.Add(modify_op_(val));
      }
      else
      {
        *container_.Add() = modify_op_(val);
      }

      return *this;
    }

    BackInsertProtoIterator& operator*()
    {
      return *this;
    }

    BackInsertProtoIterator& operator++()
    {
      return *this;
    }

    BackInsertProtoIterator& operator++(int)
    {
      return *this;
    }

  private:
    Container& container_;
    ModifyOpType modify_op_;
  };

  template<class Error>
  const char* get_merge_error_message(
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

namespace AdServer::Grpc
{
  namespace
  {
    class TimeGuard final
    {
    public:
      TimeGuard() noexcept;

      Generics::Time consider() noexcept;

      ~TimeGuard() = default;

    private:
      Generics::Timer timer_;
    };

    TimeGuard::TimeGuard() noexcept
    {
      timer_.start();
    }

    Generics::Time TimeGuard::consider() noexcept
    {
      timer_.stop();
      return timer_.elapsed_time();
    }

    class UpdateTask final : public Generics::GoalTask
    {
    public:
      UpdateTask(
        AdFrontend* ad_frontend,
        Generics::Planner* planner,
        Generics::TaskRunner* task_runner,
        const Generics::Time& update_period,
        Logging::Logger* logger)
        : Generics::GoalTask(planner, task_runner),
          ad_frontend_(ad_frontend),
          update_period_(update_period),
          logger_(ReferenceCounting::add_ref(logger))
      {
      }

      void execute() noexcept override
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
      AdFrontend* const ad_frontend_;
      const Generics::Time update_period_;
      const Logging::Logger_var logger_;
    };

    using UpdateTask_var = ReferenceCounting::SmartPtr<UpdateTask>;
  } // namespace

  AdFrontend::AdFrontend(
    TaskProcessor& helper_task_processor,
    const GrpcContainerPtr& grpc_container,
    Configuration* frontend_config,
    Logging::Logger* logger,
    CommonModule* common_module,
    FrontendCommons::HttpResponseFactory* response_factory)
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
      helper_task_processor_(helper_task_processor),
      grpc_container_(grpc_container),
      fe_config_path_(frontend_config->path()),
      frontend_config_(ReferenceCounting::add_ref(frontend_config)),
      common_module_(ReferenceCounting::add_ref(common_module))
  {
    if (!grpc_container_->grpc_user_info_operation_distributor)
    {
      Stream::Error stream;
      stream << FNS
             << "grpc_user_info_operation_distributor is null";
      throw Exception(stream);
    }

    if (!grpc_container_->grpc_user_bind_operation_distributor)
    {
      Stream::Error stream;
      stream << FNS
             << "grpc_user_bind_operation_distributor is null";
      throw Exception(stream);
    }

    if (!grpc_container_->grpc_channel_operation_pool)
    {
      Stream::Error stream;
      stream << FNS
             << "grpc_channel_operation_pool is null";
      throw Exception(stream);
    }
  }

  bool AdFrontend::will_handle(const String::SubString& uri) noexcept
  {
    std::string found_uri;
    bool result = FrontendCommons::find_uri(
      config_->UriList().Uri(), uri, found_uri);

    if (logger()->log_level() >= TraceLevel::MIDDLE)
    {
      Stream::Error ostr;
      ostr << "AdFrontend::will_handle(" << uri << "), service: '" << found_uri << "'";
      logger()->log(ostr.str());
    }

    return result;
  }

  void AdFrontend::parse_configs_()
  {
    try
    {
      using Config = Configuration::FeConfig;
      const Config& fe_config = frontend_config_->get();

      if (!fe_config.CommonFeConfiguration().present())
      {
        throw Exception("CommonFeConfiguration isn't present");
      }

      common_config_ = CommonConfigPtr(
        new CommonFeConfiguration(*fe_config.CommonFeConfiguration()));

      if (!fe_config.AdFeConfiguration().present())
      {
        throw Exception("AdFeConfiguration isn't present");
      }

      config_.reset(
        new AdFeConfiguration(*fe_config.AdFeConfiguration()));

      if (fe_config.PassFeConfiguration().present())
      {
        pass_config_ = PassConfigPtr(
          new PassFeConfiguration(*fe_config.PassFeConfiguration()));
      }

      cookie_manager_.reset(
        new FrontendCommons::CookieManager<
          FrontendCommons::HttpRequest, FrontendCommons::HttpResponse>(
            common_config_->Cookies()));
    }
    catch (const eh::Exception& exc)
    {
      Stream::Error stream;
      stream << FNS
             << "Can't parse config file '"
             << fe_config_path_
             << "': "
             << exc.what();
      throw Exception(stream);
    }
  }

  void AdFrontend::init()
  {
    try
    {
      parse_configs_();

      // create list of cookies to remove
      if (common_config_->OutdatedCookies().present())
      {
        for (auto it = common_config_->OutdatedCookies()->Cookie().begin();
            it != common_config_->OutdatedCookies()->Cookie().end();
            ++it)
        {
          remove_cookies_holder_.push_back(it->name());
          remove_cookies_.insert(
            Generics::SubStringHashAdapter(remove_cookies_holder_.back()));
        }
      }

      task_runner_ = new Generics::TaskRunner(callback(), 2);
      task_scheduler_ = new FrontendCommons::TaskScheduler(
        callback(),
        task_runner_);
      add_child_object(task_scheduler_);

      if (common_config_->StatsDumper().present())
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
      if (common_config_->user_agent_filter_path().present())
      {
        std::string config_file_path =
          fe_config_path_.substr(0, fe_config_path_.rfind('/'));
        user_agent_filter_path = config_file_path +
          "/" + *common_config_->user_agent_filter_path();
      }

      std::set<std::string> acl_list;

      if (common_config_->DebugInfo().use_acl())
      {
        String::StringManip::Splitter<String::AsciiStringManip::SepNL>
          splitter(String::SubString(common_config_->DebugInfo().ips().c_str()));
        String::SubString token;
        while (splitter.get_token(token))
        {
          acl_list.insert(token.str());
        }
      }

      std::set<int> acl_colo;

      {
        String::StringManip::Splitter<String::AsciiStringManip::SepComma> splitter2(
          String::SubString(common_config_->DebugInfo().colocations().c_str()));
        String::SubString token;
        while (splitter2.get_token(token))
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
    catch (const eh::Exception& exc)
    {
      Stream::Error stream;
      stream << FNS
             << "eh::Exception caught: "
             << exc.what();
      throw Exception(stream);
    }

    logger()->log(String::SubString(
        "AdFrontend::init(): frontend is running ..."),
      Logging::Logger::INFO,
      Aspect::AD_FRONTEND);
  }

  void AdFrontend::shutdown() noexcept
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
    {
    }
  }

  void AdFrontend::log_request(
    const char* function_name,
    const FrontendCommons::HttpRequest& request,
    const unsigned int log_level_val)
  {
    if (logger()->log_level() >= log_level_val)
    {
      std::ostringstream ostr;

      ostr << function_name << ":" << std::endl <<
        "Args: " << request.args() << std::endl <<
        "Params ("<< request.params().size() << "):"  << std::endl;

      for (auto it = request.params().begin();
           it != request.params().end();
           ++it)
      {
        ostr << "    " << it->name << " : " << it->value << std::endl;
      }

      ostr << "Headers ("<< request.headers().size() << "):"  << std::endl;

      for (auto it = request.headers().begin();
           it != request.headers().end();
           ++it)
      {
        ostr << "    " << it->name << " : " << it->value << std::endl;
      }

      ostr << "    " << "Header_only : " << request.header_only() << std::endl;

      logger()->log(ostr.str(),
        log_level_val,
        Aspect::AD_FRONTEND);
    }
  }

  void AdFrontend::handle_request_(
    FrontendCommons::HttpRequestHolder_var request_holder,
    FrontendCommons::HttpResponseWriter_var response_writer) noexcept
  {
    const FrontendCommons::HttpRequest& request = request_holder->request();
    FrontendCommons::HttpResponse_var response_ptr = create_response();
    FrontendCommons::HttpResponse& response = *response_ptr;

    if (logger()->log_level() >= TraceLevel::MIDDLE)
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

      // tad request processing

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

      if (request_info.do_opt_out)
      {
        opt_out_client_(
          cookies,
          response,
          request,
          request_info);
      }

      debug_sink.write_response(response, str_response, http_status);

      if (common_config_->ResponseHeaders().present())
      {
        FrontendCommons::add_headers(
          *(common_config_->ResponseHeaders()),
          response);
      }

      if (logger()->log_level() >= TraceLevel::MIDDLE)
      {
        Stream::Error ostr;
        ostr << FNS
             << "response:\n"
             << str_response;

        logger()->log(ostr.str(),
          TraceLevel::MIDDLE,
          Aspect::AD_FRONTEND);
      }

      if (request_info.have_uid_cookie)
      {
        FrontendCommons::add_UID_cookie(
          response,
          request,
          *cookie_manager_,
          request_info.signed_client_id);
      }

      if (request_info.format == "vast")
      {
        FrontendCommons::CORS::set_headers(request, response);
      }

      if (http_status != 204)
      {
        response.get_output_stream().write(
          str_response.c_str(), str_response.length());
      }
    }
    catch (const ForbiddenException &ex)
    {
      // forbidden request
      http_status = 403;

      if (logger()->log_level() >= TraceLevel::LOW ||
          debug_sink.require_debug_body())
      {
        Stream::Error ostr;
        ostr << FNS
             << "ForbiddenException caught: "
             << ex.what();

        if (logger()->log_level() >= TraceLevel::MIDDLE)
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

      if (logger()->log_level() >= TraceLevel::MIDDLE ||
        debug_sink.require_debug_body())
      {
        Stream::Error ostr;
        ostr << FNS
             << "InvalidParamException caught: "
             << e.what();

        if (logger()->log_level() >= TraceLevel::MIDDLE)
        {
          logger()->log(ostr.str(),
            TraceLevel::MIDDLE,
            Aspect::AD_FRONTEND);
        }

        debug_sink.fill_debug_body(response, http_status, ostr);
      }
    }
    catch (const HTTP::CookieList::Exception& exc)
    {
      http_status = 400;

      Stream::Error ostr;
      ostr << FNS
           << "HTTP::CookieList::Exception caught: "
           << exc.what();

      logger()->log(ostr.str(),
        Logging::Logger::NOTICE,
        Aspect::AD_FRONTEND);

      debug_sink.fill_debug_body(response, http_status, ostr);
    }
    catch (const eh::Exception& exc)
    {
      http_status = 500;

      Stream::Error ostr;
      ostr << FNS
           << "eh::Exception caught: "
           << exc.what();

      logger()->log(ostr.str(),
        Logging::Logger::EMERGENCY,
        Aspect::AD_FRONTEND,
        "ADS-IMPL-109");

      debug_sink.fill_debug_body(response, http_status, ostr);
    }

    if (stats_.in())
    {
      stats_->consider_request(request_info, request_time_metering);
    }

    // campaign manager request failed
    if (http_status != 200)
    {
      try
      {
        if (!request_info.original_url.empty())
        {
          http_status = FrontendCommons::redirect(
            request_info.original_url, response);
        }
        else if (!passback_info.url.empty())
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

  void AdFrontend::merge_users(
    RequestTimeMetering& request_time_metering,
    bool& merge_success,
    Generics::Time& last_request,
    std::string& merge_error_message,
    const RequestInfo& request_info) noexcept
  {
    using ProfilesRequestInfo = AdServer::UserInfoSvcs::Types::ProfilesRequestInfo;
    using MatchParams = AdServer::UserInfoSvcs::Types::MatchParams;
    using UserInfo = AdServer::UserInfoSvcs::Types::UserInfo;
    using UserProfiles = AdServer::UserInfoSvcs::Types::UserProfiles;

    TimeGuard user_merge_time_metering;
    try
    {
      const auto& grpc_user_info_distributor =
        grpc_container_->grpc_user_info_operation_distributor;
      merge_success = false;

      UserProfiles merge_user_profiles;

      const bool merge_temp =
        request_info.merge_persistent_client_id.is_null();
      const auto merged_uid_info = merge_temp ?
        GrpcAlgs::pack_user_id(request_info.temp_client_id) :
        GrpcAlgs::pack_user_id(request_info.merge_persistent_client_id);

      if ((merge_temp && request_info.temp_client_id == AdServer::Commons::PROBE_USER_ID) ||
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

        auto response = grpc_user_info_distributor->get_user_profile(
          merged_uid_info,
          merge_temp,
          profiles_request);
        if (!response || response->has_error())
        {
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
        if (user_profiles_proto.base_user_profile().empty() &&
            user_profiles_proto.add_user_profile().empty())
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
          auto response = grpc_user_info_distributor->remove_user_profile(
            merged_uid_info);
          if (!response || response->has_error())
          {
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

      auto response = grpc_user_info_distributor->merge(
        user_info,
        match_params,
        merge_user_profiles);
      if (!response || response->has_error())
      {
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
      merge_success = info_proto.merge_success();
    }
    catch (const eh::Exception& exc)
    {
      if (merge_error_message.empty())
      {
        merge_error_message = MergeMessage::SOURCE_EXCEPTION;
      }
      Stream::Error stream;
      stream << FNS
             << exc.what();
      logger()->error(stream.str(), Aspect::AD_FRONTEND);
    }
    catch (...)
    {
      if (merge_error_message.empty())
      {
        merge_error_message = MergeMessage::SOURCE_EXCEPTION;
      }
      Stream::Error stream;
      stream << FNS
             << "Unknown error";
      logger()->error(stream.str(), Aspect::AD_FRONTEND);
    }

    request_time_metering.merge_users_time =
      user_merge_time_metering.consider();
  }

  std::unique_ptr<AdServer::UserInfoSvcs::Proto::MatchResult>
  AdFrontend::get_empty_history_matching()
  {
    auto result = std::make_unique<AdServer::UserInfoSvcs::Proto::MatchResult>();
    result->set_fraud_request(false);
    result->set_times_inited(false);
    result->set_last_request_time(GrpcAlgs::pack_time(Generics::Time::ZERO));
    result->set_create_time(GrpcAlgs::pack_time(Generics::Time::ZERO));
    result->set_session_start(GrpcAlgs::pack_time(Generics::Time::ZERO));
    result->set_colo_id(-1);

    return result;
  }

  std::unique_ptr<AdServer::ChannelSvcs::Proto::MatchResponseInfo>
  AdFrontend::get_empty_trigger_matching()
  {
    auto result = std::make_unique<AdServer::ChannelSvcs::Proto::MatchResponseInfo>();
    result->set_no_track(false);
    result->set_no_adv(false);

    return result;
  }

  struct ContextualChannelIdLess
  {
    bool operator()(
      const AdServer::UserInfoSvcs::Proto::ChannelWeight& ch_weight,
      const AdServer::ChannelSvcs::Proto::ContentChannelAtom& contextual_channel) const
    {
      return ch_weight.channel_id() < contextual_channel.id();
    }

    bool operator()(
      const AdServer::ChannelSvcs::Proto::ContentChannelAtom& contextual_channel,
      const AdServer::UserInfoSvcs::Proto::ChannelWeight& ch_weight) const
    {
      return contextual_channel.id() < ch_weight.channel_id();
    }
  };

  struct ContextualChannelConverter final
  {
    using ChannelWeight = AdServer::UserInfoSvcs::Proto::ChannelWeight;
    using ContentChannelAtom = AdServer::ChannelSvcs::Proto::ContentChannelAtom;

    const ChannelWeight& operator()(
      const ChannelWeight& ch_weight) const
    {
      return ch_weight;
    }

    ChannelWeight operator()(
      const ContentChannelAtom& contextual_channel) const
    {
      ChannelWeight result;
      result.set_channel_id(contextual_channel.id());
      result.set_weight(contextual_channel.weight());
      return result;
    }
  };

  void AdFrontend::acquire_user_info_matcher(
    const RequestInfo& request_info,
    const AdServer::ChannelSvcs::Proto::MatchResponseInfo* trigger_matching_result,
    std::unique_ptr<AdServer::UserInfoSvcs::Proto::MatchResult>& match_result_out,
    bool& profiling_available,
    RequestTimeMetering& request_time_metering) noexcept
  {
    using MatchParams = AdServer::UserInfoSvcs::Types::MatchParams;
    using UserInfo = AdServer::UserInfoSvcs::Types::UserInfo;

    std::unique_ptr<AdServer::UserInfoSvcs::Proto::MatchResult> match_result;
    bool match_success = false;
    const bool do_history_matching =
      request_info.user_status == AdServer::CampaignSvcs::US_OPTIN ||
      request_info.user_status == AdServer::CampaignSvcs::US_TEMPORARY;

    if (do_history_matching)
    {
      try
      {
        TimeGuard history_match_time_metering;

        MatchParams match_params;
        match_params.use_empty_profile = false;
        match_params.silent_match = request_info.silent_match;
        match_params.no_match = request_info.no_match
          || (trigger_matching_result && trigger_matching_result->no_track());
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

          match_params.geo_data_seq.emplace_back(
            latitude,
            longitude,
            accuracy);
        }

        if (request_info.tag_id == 0 || config_->ad_request_profiling())
        {
          prepare_ui_match_params_(
            match_params,
            trigger_matching_result,
            request_info);
        }

        match_params.cohort = request_info.curct;

        // process match & merge request
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

        const auto& grpc_user_info_distributor =
          grpc_container_->grpc_user_info_operation_distributor;
        auto response = grpc_user_info_distributor->match(
          user_info,
          match_params);
        if (!response || response->has_error())
        {
          throw Exception("match is failed");
        }

        auto* info_proto = response->mutable_info();
        match_result.reset(info_proto->release_match_result());

        request_time_metering.matched_channels =
          match_result->channels().size();
        request_time_metering.history_match_time =
          history_match_time_metering.consider();

        request_time_metering.history_match_local_time =
          GrpcAlgs::unpack_time(match_result->process_time());
        match_success = true;
      }
      catch (const eh::Exception& exc)
      {
        Stream::Error stream;
        stream << FNS
               << exc.what();
        logger()->error(stream.str(), Aspect::AD_FRONTEND);
      }
      catch (...)
      {
        Stream::Error stream;
        stream << FNS
               << ": Unknown error";
        logger()->error(stream.str(), Aspect::AD_FRONTEND);
      }
    }

    if (!match_result)
    {
      match_result = get_empty_history_matching();
    }

    if (logger()->log_level() >= TraceLevel::MIDDLE)
    {
      const auto& channels = match_result->channels();

      std::ostringstream ostream;
      ostream << FNS
              << "history matched channels for uid = '"
              << request_info.client_id
              << "': ";

      if (channels.empty())
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

    if (trigger_matching_result && !trigger_matching_result->no_track())
    {
      if (!match_success || !do_history_matching)
      {
        match_result = get_empty_history_matching();

        // fill history channels with context channels
        auto* history_matched_channels = match_result->mutable_channels();
        const auto& content_channels = trigger_matching_result->content_channels();

        history_matched_channels->Reserve(content_channels.size());
        for (const auto& content_channel : content_channels)
        {
          auto* channel_weight = history_matched_channels->Add();
          channel_weight->set_channel_id(content_channel.id());
          channel_weight->set_weight(content_channel.weight());
        }
      }
      else if (request_info.tag_id != 0 && !config_->ad_request_profiling())
      {
        // merge history match result & contextually matched channels:
        // history matched channel override contextually matched channel
        const auto& history_matched_channels = match_result->channels();
        const auto& content_channels = trigger_matching_result->content_channels();
        auto* result_channels = match_result->mutable_channels();
        result_channels->Reserve(
          history_matched_channels.size() + content_channels.size());
        Algs::merge_unique(
          std::begin(history_matched_channels),
          std::end(history_matched_channels),
          std::begin(content_channels),
          std::end(content_channels),
          BackInsertProtoIterator(*result_channels, ContextualChannelConverter()),
          ContextualChannelIdLess(),
          Algs::FirstArg());
      }
    }

    match_result_out = std::move(match_result);
    profiling_available = match_success;
  }

  void AdFrontend::user_info_post_match_(
    RequestTimeMetering& request_time_metering,
    const RequestInfo& request_info,
    const AdServer::CampaignSvcs::Proto::RequestCreativeResult&
      campaign_select_result) noexcept
  {
    using CampaignIds = AdServer::UserInfoSvcs::Types::CampaignIds;
    using SeqOrders = AdServer::UserInfoSvcs::Types::SeqOrders;
    using FreqCaps = AdServer::UserInfoSvcs::Types::FreqCaps;

    Generics::Timer timer;
    timer.start();

    try
    {
      const auto& grpc_user_info_distributor =
        grpc_container_->grpc_user_info_operation_distributor;
      const auto& ad_slots = campaign_select_result.ad_slots();
      for (const auto& ad_slot : ad_slots)
      {
        if (!ad_slot.selected_creatives().empty())
        {
          CampaignIds campaign_ids;
          campaign_ids.reserve(ad_slot.selected_creatives().size());

          SeqOrders seq_orders;
          seq_orders.reserve(ad_slot.selected_creatives().size());

          for (const auto& creative : ad_slot.selected_creatives())
          {
            if (creative.order_set_id())
            {
              seq_orders.emplace_back(
                creative.cmp_id(),
                creative.order_set_id(),
                1);
            }

            campaign_ids.emplace_back(creative.campaign_group_id());
          }

          FreqCaps freq_caps;
          freq_caps.reserve(ad_slot.freq_caps().size());
          freq_caps.insert(
            std::end(freq_caps),
            std::begin(ad_slot.freq_caps()),
            std::end(ad_slot.freq_caps()));

          FreqCaps uc_freq_caps;
          uc_freq_caps.reserve(ad_slot.uc_freq_caps().size());
          uc_freq_caps.insert(
            std::end(uc_freq_caps),
            std::begin(ad_slot.uc_freq_caps()),
            std::end(ad_slot.uc_freq_caps()));

          std::string request_id;
          request_id = ad_slot.request_id();

          auto response = grpc_user_info_distributor->update_user_freq_caps(
            GrpcAlgs::pack_user_id(request_info.client_id),           // user_id
            request_info.current_time,                                // time
            request_id,                                               // request_id
            freq_caps,                                                // freq_caps
            uc_freq_caps,                                             // uc_freq_caps
            FreqCaps{},                                               // virtual_freq_caps
            seq_orders,                                               // seq_orders
            ad_slot.track_impr() ? CampaignIds{} : campaign_ids,      // campaign_ids
            ad_slot.track_impr() ? campaign_ids : CampaignIds{});     // campaign_ids
          if (!response || response->has_error())
          {
            throw Exception("update_user_freq_caps is failed");
          }
        }
      }
    }
    catch (const eh::Exception& exc)
    {
      Stream::Error stream;
      stream << FNS
             << exc.what();
      logger()->emergency(stream.str(), Aspect::AD_FRONTEND);
    }
    catch (...)
    {
      Stream::Error stream;
      stream << FNS
             << "Unknown error";
      logger()->emergency(stream.str(), Aspect::AD_FRONTEND);
    }

    timer.stop();
    request_time_metering.history_post_match_time = timer.elapsed_time();
  }

  void AdFrontend::match_triggers_(
    RequestTimeMetering& request_time_metering,
    std::unique_ptr<AdServer::ChannelSvcs::Proto::MatchResponseInfo>&
      trigger_matched_channels,
    const RequestInfo& request_info)
  {
    try
    {
      TimeGuard trigger_match_time_metering;

      request_time_metering.recived_triggers =
        request_info.referer.empty() ? 0 : 1;

      std::string pwords;
      // only referer matching m.b. used for opted out clients
      if (request_info.full_text_words.empty())
      {
        pwords = request_info.page_words;
        request_time_metering.recived_triggers +=
          request_info.page_words.size();
      }
      else
      {
        pwords = request_info.full_text_words;
        request_time_metering.recived_triggers +=
          request_info.full_text_words.size();
      }

      bool simplify_page = true;
      if (request_info.keywords_normalized)
      {
        simplify_page = false;
      }

      const auto& grpc_channel_pool =
        grpc_container_->grpc_channel_operation_pool;
      auto response = grpc_channel_pool->match(
        {},                                               // request_id
        request_info.referer,                             // first_url
        request_info.referer_url_words,                   // first_url_words
        {},                                               // urls
        {},                                               // urls_words
        pwords,                                           // pwords
        request_info.search_words,                        // swords
        GrpcAlgs::pack_user_id(request_info.client_id),   // uid
        {'A', '\0'},                                      // statuses
        false,                                            // non_strict_word_match
        false,                                            // non_strict_url_match
        false,                                            // return_negative
        simplify_page,                                    // simplify_page
        true);                                            // fill_content
      if (!response || response->has_error())
      {
        throw Exception("match is failed");
      }

      trigger_matched_channels.reset(response->release_info());

      const auto& matched_channels = trigger_matched_channels->matched_channels();
      request_time_metering.matched_triggers = static_cast<unsigned long>(
        matched_channels.page_channels().size() +
        matched_channels.search_channels().size() +
        matched_channels.url_channels().size() +
        matched_channels.url_keyword_channels().size() +
        matched_channels.uid_channels().size());

      request_time_metering.trigger_match_time =
        trigger_match_time_metering.consider();

      request_time_metering.detail_trigger_match_time.emplace_back(
        GrpcAlgs::unpack_time(trigger_matched_channels->match_time()));

      if (logger()->log_level() >= TraceLevel::MIDDLE)
      {
        DebugStream ostr;
        ostr << FNS
             << "Channels matched for page-words '";
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
    catch (const eh::Exception& exc)
    {
      Stream::Error stream;
      stream << FNS
             << "Caught exception: "
             << exc.what();
      logger()->log(stream.str(),
        Logging::Logger::EMERGENCY,
        Aspect::AD_FRONTEND,
        "ADS-IMPL-117");
    }
    catch (...)
    {
      Stream::Error stream;
      stream << FNS
             << "Caught exception: "
             << "Unknown error";
      logger()->log(stream.str(),
        Logging::Logger::EMERGENCY,
        Aspect::AD_FRONTEND,
        "ADS-IMPL-117");
    }
  }

  int AdFrontend::acquire_ad(
    HttpResponse& response,
    const FrontendCommons::HttpRequest& request,
    const RequestInfo& request_info,
    const Generics::SubStringHashAdapter& instantiate_type,
    std::string& str_response,
    PassbackInfo& passback_info,
    bool& log_as_test,
    DebugSink* debug_sink,
    RequestTimeMetering& request_time_metering)
  {
    std::unique_ptr<AdServer::ChannelSvcs::Proto::MatchResponseInfo>
      trigger_matched_channels;
    std::unique_ptr<AdServer::UserInfoSvcs::Proto::MatchResult>
      history_match_result;
    bool profiling_available = false;

    bool merge_success = true;
    std::string merge_error_message;
    Generics::Time merged_last_request;

    std::unique_ptr<AdServer::CampaignSvcs::Proto::RequestCreativeResult>
      campaign_matching_result;

    bool make_merge = (!request_info.temp_client_id.is_null() ||
      !request_info.merge_persistent_client_id.is_null()) &&
      !request_info.client_id.is_null();

    // check user id by user bind
    if (make_merge || !request_info.passback_by_colocation)
    {
      AdServer::Commons::UserId resolved_user_id;
      if (resolve_cookie_user_id_(resolved_user_id, request_info))
      {
        request_info.client_id = resolved_user_id;
      }
    }

    if (make_merge)
    {
      merge_success = false;
      merge_users(
        request_time_metering,
        merge_success,
        merged_last_request,
        merge_error_message,
        request_info);
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
        trigger_matched_channels,
        request_info);

      // do history based channels matching
      acquire_user_info_matcher(
        request_info,
        trigger_matched_channels.get(),
        history_match_result,
        profiling_available,
        request_time_metering);
    }

    if (!history_match_result)
    {
      history_match_result = get_empty_history_matching();
    }

    request_time_metering.profiling = true;

    debug_sink->print_acquire_ad(
       request_info,
      trigger_matched_channels.get(),
      nullptr,
      *history_match_result);

    // do campaign selection
     request_campaign_manager_(
      passback_info,
      log_as_test,
      campaign_matching_result,
      request_time_metering,
      request_info,
      instantiate_type,
      trigger_matched_channels.get(),
      history_match_result.get(),
      merge_success ? merged_last_request :
        GrpcAlgs::unpack_time(history_match_result->last_request_time()),
      profiling_available,
      nullptr,
      nullptr,
      debug_sink);

    if (request_info.user_status == AdServer::CampaignSvcs::US_OPTIN &&
      campaign_matching_result)
    {
      user_info_post_match_(
        request_time_metering,
        request_info,
        *campaign_matching_result);
    }

    try
    {
      if (request_info.user_status == AdServer::CampaignSvcs::US_OPTIN &&
        history_match_result->colo_id() != -1)
      {
        std::ostringstream current_colo_ostr;
        current_colo_ostr << history_match_result->colo_id();

        cookie_manager_->set(
          response,
          request,
          Request::Cookie::LAST_COLOCATION_ID,
          current_colo_ostr.str());
      }

      if (!merge_success)
      {
        response.add_header(
          Response::Header::MERGE_FAILED,
          merge_error_message);
      }

      if (campaign_matching_result)
      {
        debug_sink->print_creative_selection_debug_info(
          request_info,
          passback_info,
          *campaign_matching_result,
          request_time_metering);
      }

      if (campaign_matching_result &&
         !campaign_matching_result->ad_slots().empty())
      {
        const auto& ad_slot = campaign_matching_result->ad_slots()[0];
        if (!ad_slot.creative_body().empty())
        {
          str_response = ad_slot.creative_body();
          if (!ad_slot.mime_format().empty())
          {
            response.set_content_type(ad_slot.mime_format());
          }
          else
          {
            response.set_content_type(Response::Type::TEXT_HTML);
          }

          return 200;
        }
      }
    }
    catch (const eh::Exception& exc)
    {
      Stream::Error stream;
      stream << FNS
             << "Can't generate response. Caught eh::Exception: "
             << exc.what();
      throw Exception(stream);
    }
    catch (...)
    {
      Stream::Error stream;
      stream << FNS
           << "Can't generate response. Caught eh::Exception: "
           << "Unknown error";
      throw Exception(stream);
    }

    return 204;
  }

  void AdFrontend::convert_ccg_keywords_(
    std::vector<FrontendCommons::GrpcCampaignManagerPool::CCGKeyword>& ccg_keywords,
    const google::protobuf::RepeatedField<AdServer::ChannelSvcs::Proto::CCGKeyword>*
     src_ccg_keywords) noexcept
  {
    using RevenueDecimal = AdServer::CampaignSvcs::RevenueDecimal;
    using CTRDecimal = AdServer::CampaignSvcs::CTRDecimal;

    if (src_ccg_keywords)
    {
      ccg_keywords.reserve(src_ccg_keywords->size());
      for (const auto& src_ccg_keyword : *src_ccg_keywords)
      {
        ccg_keywords.emplace_back(
          src_ccg_keyword.ccg_keyword_id(),
          src_ccg_keyword.ccg_id(),
          src_ccg_keyword.channel_id(),
          GrpcAlgs::unpack_decimal<RevenueDecimal>(src_ccg_keyword.max_cpc()),
          GrpcAlgs::unpack_decimal<CTRDecimal>(src_ccg_keyword.ctr()),
          src_ccg_keyword.click_url(),
          src_ccg_keyword.original_keyword());
      }
    }
  }

  bool AdFrontend::resolve_cookie_user_id_(
    AdServer::Commons::UserId& resolved_user_id,
    const RequestInfo& request_info) noexcept
  {
    if (!request_info.client_id.is_null())
    {
      const auto& grpc_user_bind_distributor =
        grpc_container_->grpc_user_bind_operation_distributor;
      try
      {
        const std::string ext_user_id = std::string("c/") +
          request_info.client_id.to_string();

        auto response = grpc_user_bind_distributor->get_user_id(
          ext_user_id,                         // id
          request_info.client_id,              // current_user_id
          request_info.current_time,           // timestamp
          Generics::Time::ZERO,                // create_timestamp
          true,                               // silent
          false,                              // generate_user_id
          true);                              // for_set_cookie
        if (response && response->has_info())
        {
          const auto& info_proto = response->info();
          resolved_user_id = GrpcAlgs::unpack_user_id(info_proto.user_id());
          common_module_->user_id_controller()->null_blacklisted(resolved_user_id);

          return !resolved_user_id.is_null();
        }
        else
        {
          throw Exception("get_user_id is failed");
        }
      }
      catch (const eh::Exception& exc)
      {
        Stream::Error stream;
        stream << FNS
               << exc.what();
        logger()->error(stream.str(), Aspect::AD_FRONTEND);
      }
      catch (...)
      {
        Stream::Error stream;
        stream << FNS
               << "Unknown error";
        logger()->error(stream.str(), Aspect::AD_FRONTEND);
      }
    }

    return false;
  }

  void
  AdFrontend::request_campaign_manager_(
    PassbackInfo& passback_info,
    bool& log_as_test,
    std::unique_ptr<AdServer::CampaignSvcs::Proto::RequestCreativeResult>&
      campaign_matching_result,
    RequestTimeMetering& request_time_metering,
    const RequestInfo& request_info,
    const Generics::SubStringHashAdapter& instantiate_type,
    AdServer::ChannelSvcs::Proto::MatchResponseInfo* trigger_matched_channels,
    AdServer::UserInfoSvcs::Proto::MatchResult* history_match_result,
    const Generics::Time&,
    bool profiling_available,
    const google::protobuf::RepeatedField<AdServer::ChannelSvcs::Proto::CCGKeyword>*
      ccg_keywords,
    const google::protobuf::RepeatedField<AdServer::ChannelSvcs::Proto::CCGKeyword>*
      hid_ccg_keywords,
    DebugSink* debug_sink)
  {
    using CoordDecimal = AdServer::CampaignSvcs::CoordDecimal;

    try
    {
      FrontendCommons::GrpcCampaignManagerPool::RequestParams request_params;

      if (trigger_matched_channels)
      {
        const auto& matched_channels = trigger_matched_channels->matched_channels();
        request_params.trigger_match_result.pkw_channels.reserve(
          matched_channels.page_channels().size());
        request_params.trigger_match_result.skw_channels.reserve(
          matched_channels.search_channels().size());
        request_params.trigger_match_result.url_channels.reserve(
          matched_channels.url_channels().size());
        request_params.trigger_match_result.ukw_channels.reserve(
          matched_channels.url_keyword_channels().size());

        auto converter_channel_atom = [] (
          const AdServer::ChannelSvcs::Proto::ChannelAtom& atom) {
          using ChannelTriggerMatchInfo =
            FrontendCommons::GrpcCampaignManagerPool::ChannelTriggerMatchInfo;
          return ChannelTriggerMatchInfo(
            atom.trigger_channel_id(),
            atom.id());
        };

        std::transform(
          std::begin(matched_channels.page_channels()),
          std::end(matched_channels.page_channels()),
          std::back_inserter(request_params.trigger_match_result.pkw_channels),
          converter_channel_atom);
        std::transform(
          std::begin(matched_channels.search_channels()),
          std::end(matched_channels.search_channels()),
          std::back_inserter(request_params.trigger_match_result.skw_channels),
          converter_channel_atom);
        std::transform(
          std::begin(matched_channels.url_channels()),
          std::end(matched_channels.url_channels()),
          std::back_inserter(request_params.trigger_match_result.url_channels),
          converter_channel_atom);
        std::transform(
          std::begin(matched_channels.url_keyword_channels()),
          std::end(matched_channels.url_keyword_channels()),
          std::back_inserter(request_params.trigger_match_result.ukw_channels),
          converter_channel_atom);
        request_params.trigger_match_result.uid_channels.reserve(
          matched_channels.uid_channels().size());
        request_params.trigger_match_result.uid_channels.insert(
          std::end(request_params.trigger_match_result.uid_channels),
          std::begin(matched_channels.uid_channels()),
          std::end(matched_channels.uid_channels()));
      }

      request_params.common_info.creative_instantiate_type = std::string_view(
        instantiate_type.text().data(), instantiate_type.text().size());

      if (request_info.location)
      {
        request_params.common_info.location.emplace_back(
          request_info.location->country,
          request_info.location->region,
          request_info.location->city);
      }

      if (history_match_result)
      {
        request_params.common_info.coord_location.reserve(
          history_match_result->geo_data_seq().size());
        for (const auto& geo_data : history_match_result->geo_data_seq())
        {
          request_params.common_info.coord_location.emplace_back(
            GrpcAlgs::unpack_decimal<CoordDecimal>(geo_data.longitude()),
            GrpcAlgs::unpack_decimal<CoordDecimal>(geo_data.latitude()),
            GrpcAlgs::unpack_decimal<CoordDecimal>(geo_data.accuracy()));
        }
      }
      else
      {
        if (request_info.coord_location)
        {
          request_params.common_info.coord_location.emplace_back(
            request_info.coord_location->longitude,
            request_info.coord_location->latitude,
            request_info.coord_location->accuracy);
        }
      }

      request_params.common_info.user_id = request_info.user_status != AdServer::CampaignSvcs::US_PROBE ?
        request_info.client_id : AdServer::Commons::UserId();
      request_params.common_info.track_user_id = request_params.common_info.user_id;

      request_params.common_info.signed_user_id = request_info.signed_client_id;
      if (!request_info.temp_client_id.is_null() && !request_info.client_id.is_null())
      {
        request_params.merged_user_id = request_info.temp_client_id;
      }

      request_params.ad_instantiate_type = AdServer::CampaignSvcs::AIT_BODY;
      request_params.fill_track_pixel = false;

      request_params.household_id = request_info.household_client_id;

      // reduce user status values
      request_params.common_info.user_status = request_info.user_status;

      if (request_info.user_status == AdServer::CampaignSvcs::US_OPTIN && (
        trigger_matched_channels && (
          trigger_matched_channels->no_track() ||
          trigger_matched_channels->no_adv())))
      {
        request_params.common_info.user_status = AdServer::CampaignSvcs::US_BLACKLISTED;
      }

      request_params.client_create_time = GrpcAlgs::unpack_time(history_match_result->create_time());
      request_params.common_info.full_referer = request_info.referer;
      request_params.common_info.referer = request_info.allowable_referer;
      request_params.context_info.full_referer_hash = request_info.full_referer_hash;
      request_params.context_info.short_referer_hash = request_info.short_referer_hash;
      request_params.common_info.cohort = request_info.curct;
      request_params.common_info.peer_ip = request_info.peer_ip;
      request_params.common_info.random = request_info.random;

      request_params.fraud = history_match_result->fraud_request() &&
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
      if (((static_cast<double>(request_info.random) * 100.0) / CampaignSvcs::RANDOM_PARAM_MAX)
        <= common_config_->profiling_log_sampling())
      {
        bool added = false;
        if (!request_info.full_text_words.empty())
        {
          request_params.page_keywords = request_info.full_text_words;
          added = true;
        }
        if (!request_info.page_words.empty())
        {
          if (added)
          {
            request_params.page_keywords = std::string(" ");
          }
          request_params.page_keywords = request_info.page_words;
          added = true;
        }

        if (!request_info.referer_url_words.empty())
        {
          request_params.url_keywords = request_info.referer_url_words;
        }
      }

      request_params.common_info.colo_id = request_info.colo_id;

      request_params.common_info.original_url = request_info.original_url;
      request_params.common_info.request_id = request_info.request_id;
      request_params.common_info.time = request_info.current_time;

      request_params.common_info.user_agent = request_info.user_agent;

      // fill request_params.context_info
      request_params.context_info.enabled_notice = false;
      request_params.context_info.profile_referer = false;
      request_params.context_info.client = request_info.client_app;
      request_params.context_info.client_version = request_info.client_app_version;
      request_params.context_info.web_browser = request_info.web_browser;
      request_params.context_info.platform_ids.reserve(
        request_info.platform_ids.size());
      request_params.context_info.platform_ids.insert(
        std::end(request_params.context_info.platform_ids),
        std::begin(request_info.platform_ids),
        std::end(request_info.platform_ids));
      request_params.context_info.platform = request_info.platform;
      request_params.context_info.full_platform = request_info.full_platform;
      request_params.context_info.page_load_id = request_info.page_load_id;
      if (common_config_->ip_logging_enabled())
      {
        std::string ip_hash;
        FrontendCommons::ip_hash(ip_hash, request_info.peer_ip, common_config_->ip_salt());
        request_params.context_info.ip_hash = ip_hash;
      }

      request_params.full_freq_caps.reserve(
        history_match_result->full_freq_caps().size());
      request_params.full_freq_caps.insert(
        std::end(request_params.full_freq_caps),
        std::begin(history_match_result->full_freq_caps()),
        std::end(history_match_result->full_freq_caps()));

      request_params.seq_orders.reserve(history_match_result->seq_orders().size());
      for (const auto& seq_order : history_match_result->seq_orders())
      {
        request_params.seq_orders.emplace_back(
          seq_order.ccg_id(),
          seq_order.set_id(),
          seq_order.imps());
      }

      request_params.campaign_freqs.reserve(
        history_match_result->campaign_freqs().size());
      std::transform(
        std::begin(history_match_result->campaign_freqs()),
        std::end(history_match_result->campaign_freqs()),
        std::back_inserter(request_params.campaign_freqs),
        [] (const auto& data) {
          return FrontendCommons::GrpcCampaignManagerPool::CampaignFreq(
            data.campaign_id(),
            data.imps());
      });

      // required passback for non profiling requests
      request_params.common_info.passback_type = request_info.passback_type;
      request_params.common_info.passback_url = request_info.passback_url;
      request_params.common_info.security_token = request_info.request_token;
      request_params.common_info.preclick_url = request_info.preclick_url;
      request_params.common_info.pub_impr_track_url = request_info.pub_impr_track_url;
      request_params.common_info.request_type = AdServer::CampaignSvcs::AR_NORMAL;
      request_params.common_info.hpos = CampaignSvcs::UNDEFINED_PUB_POSITION_BOTTOM;
      request_params.common_info.set_cookie = true;

      request_params.publisher_site_id = 0;
      request_params.required_passback = (request_info.tag_id != 0);
      request_params.preview_ccid = request_info.ccid;

      // fill input channel sequence for CampaignManager
      request_params.channels.reserve(
        history_match_result->channels().size() +
        (trigger_matched_channels?
          trigger_matched_channels->matched_channels().uid_channels().size(): 0));
      for (const auto& channel : history_match_result->channels())
      {
        request_params.channels.emplace_back(
          channel.channel_id());
      }

      if (trigger_matched_channels)
      {
        for (const auto& uid_channel : trigger_matched_channels->matched_channels().uid_channels())
        {
          request_params.channels.emplace_back(uid_channel);
        }
      }

      request_params.hid_channels.reserve(history_match_result->hid_channels().size());
      for (const auto& hid_channel : history_match_result->hid_channels())
      {
        request_params.hid_channels.emplace_back(
          hid_channel.channel_id());
      }

      request_params.exclude_pubpixel_accounts.reserve(
        history_match_result->exclude_pubpixel_accounts().size());
      request_params.exclude_pubpixel_accounts.insert(
        std::end(request_params.exclude_pubpixel_accounts),
        std::begin(history_match_result->exclude_pubpixel_accounts()),
        std::end(history_match_result->exclude_pubpixel_accounts()));

      convert_ccg_keywords_(request_params.ccg_keywords, ccg_keywords);
      convert_ccg_keywords_(request_params.hid_ccg_keywords, hid_ccg_keywords);

      request_params.search_words = request_info.search_words;
      request_params.need_debug_info = debug_sink->require_debug_info();
      request_params.session_start = GrpcAlgs::unpack_time(history_match_result->session_start());
      request_params.only_display_ad = false;
      request_params.profiling_type = AdServer::CampaignSvcs::PT_ALL;

      if (request_info.tag_id)
      {
        // initialize slot
        request_params.ad_slots.emplace_back();
        auto& ad_slot = request_params.ad_slots.back();
        ad_slot.format = request_info.format;
        ad_slot.tag_id = request_info.tag_id;
        ad_slot.passback =
          request_info.do_passback ||
          request_info.passback_by_colocation ||
          history_match_result->fraud_request() ||
          (trigger_matched_channels &&
            (trigger_matched_channels->no_track() ||
              trigger_matched_channels->no_adv()));
        ad_slot.ext_tag_id = request_info.ext_tag_id;
        ad_slot.min_ecpm = CampaignSvcs::RevenueDecimal::ZERO;

        ad_slot.up_expand_space = request_info.up_expand_space ?
          static_cast<long>(*request_info.up_expand_space) : -1;
        ad_slot.right_expand_space = request_info.right_expand_space ?
          static_cast<long>(*request_info.right_expand_space) : -1;
        ad_slot.down_expand_space = request_info.down_expand_space ?
          static_cast<long>(*request_info.down_expand_space) : -1;
        ad_slot.left_expand_space = request_info.left_expand_space ?
          static_cast<long>(*request_info.left_expand_space) : -1;
        ad_slot.tag_visibility = request_info.tag_visibility ?
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

      const auto& grpc_campaign_manager_pool =
        grpc_container_->grpc_campaign_manager_pool;
      auto response = grpc_campaign_manager_pool->get_campaign_creative(
        request_params);
      if (!response || response->has_error())
      {
        throw Exception("get_campaign_creative is failed");
      }
      auto* info_proto = response->mutable_info();
      campaign_matching_result.reset(info_proto->release_request_result());

      assert(campaign_matching_result->ad_slots().size() ==
        static_cast<int>(request_params.ad_slots.size()));

      request_time_metering.creative_selection_local_time =
        GrpcAlgs::unpack_time(campaign_matching_result->process_time());

      if (!campaign_matching_result->ad_slots().empty())
      {
        const auto& ad_slot_proto = campaign_matching_result->ad_slots()[0];
        if (ad_slot_proto.passback() && !ad_slot_proto.passback_url().empty())
        {
          passback_info.url = ad_slot_proto.passback_url();
        }

        log_as_test |= ad_slot_proto.test_request();

        request_time_metering.creative_selection_time =
          creative_selection_time_metering.consider();

        request_time_metering.creative_count = ad_slot_proto.selected_creatives().size();
        request_time_metering.passback = ad_slot_proto.passback();
      }
    }
    catch (const eh::Exception& exc)
    {
      Stream::Error stream;
      stream << FNS
             << "Fail. Caught eh::Exception: "
             << exc.what();
      throw Exception(stream);
    }
    catch (...)
    {
      Stream::Error stream;
      stream << FNS
             << "Fail. Caught exception: Unknown error";
      throw Exception(stream);
    }
  }

  void AdFrontend::fill_debug_channels_(
    const google::protobuf::RepeatedPtrField<AdServer::ChannelSvcs::Proto::ChannelAtom>& channel_atoms,
    const char type,
    DebugStream& out)
  {
    std::size_t count = 0;
    for (const auto& channel_atom : channel_atoms)
    {
      if (count)
      {
        out << ",";
      }
      out << channel_atom.id() << type;
      count += 1;
    }

    if (count == 0)
    {
      out << "empty";
    }
    else
    {
      out << " ";
    }
  }

  void AdFrontend::start_update_loop_()
  {
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
    catch (const eh::Exception& exc)
    {
      Stream::Error ostr;
      ostr << FNS
           << exc.what();
      throw Exception(ostr);
    }
  }


  void AdFrontend::opt_out_client_(
    const HTTP::CookieList& cookies,
    HttpResponse& response,
    const FrontendCommons::HttpRequest& request,
    const RequestInfo& request_info) noexcept
  {
    try
    {
      FrontendCommons::CookieNameSet remove_cookie_list;

      for(auto it = common_config_->OptOutRemoveCookies().Cookie().begin();
          it != common_config_->OptOutRemoveCookies().Cookie().end();
          ++it)
      {
        remove_cookie_list.insert(it->name());
      }

      cookie_manager_->remove(
        response,
        request,
        cookies,
        remove_cookie_list);

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

      const auto& grpc_campaign_manager_pool = grpc_container_->grpc_campaign_manager_pool;
      auto response = grpc_campaign_manager_pool->verify_opt_operation(
        request_info.current_time.tv_sec,                      // time
        request_info.colo_id,                                  // colo_id
        std::string{},                                         // referer
        AdServer::CampaignSvcs::Proto::OptOperation::OO_OUT,   // operation
        11,                                                    // status
        CampaignSvcs::US_OPTOUT,                               // user_status
        request_info.log_as_test,                              // log_as_test
        request_info.web_browser,                              // browser
        request_info.full_platform,                            // os
        std::string{},                                         // ct
        std::string{},                                         // curct
        AdServer::Commons::UserId{});                          // user_id
      if (!response || response->has_error())
      {
        throw Exception("verify_opt_operation is failed");
      }
    }
    catch (const eh::Exception& exc)
    {
      Stream::Error stream;
      stream << FNS
             << "Can't do opt out: "
             << exc.what();
      logger()->emergency(stream.str(), Aspect::AD_FRONTEND);
    }
    catch (...)
    {
      Stream::Error stream;
      stream << FNS
             << "Can't do opt out: Unknown error";
      logger()->emergency(stream.str(), Aspect::AD_FRONTEND);
    }
  }

  void AdFrontend::update_colocation_flags() noexcept
  {
    try
    {
      const auto& grpc_campaign_manager_pool = grpc_container_->grpc_campaign_manager_pool;
      auto response = grpc_campaign_manager_pool->get_colocation_flags();
      if (!response || response->has_error())
      {
        throw Exception("get_colocation_flags is failed");
      }
      const auto& info_proto = response->info();
      const auto& colocation_flags = info_proto.colocation_flags();
      const int size = colocation_flags.size();

      RequestInfoFiller::ColoFlagsMap_var new_colo_flags(
        new RequestInfoFiller::ColoFlagsMap);
      for (int i = 0; i < size; i += 1)
      {
        RequestInfoFiller::ColoFlags colo_flags;
        colo_flags.flags = colocation_flags[i].flags();
        colo_flags.hid_profile = colocation_flags[i].hid_profile();
        new_colo_flags->insert(
          RequestInfoFiller::ColoFlagsMap::value_type(
            colocation_flags[i].colo_id(),
            colo_flags));
      }
      request_info_filler_->colo_flags(new_colo_flags);
    }
    catch (const eh::Exception& exc)
    {
      Stream::Error stream;
      stream << FNS
             << "Can't update colocation flags: "
             << exc.what();
      logger()->critical(stream.str(), Aspect::AD_FRONTEND);
    }
    catch (...)
    {
      Stream::Error stream;
      stream << FNS
             << "Can't update colocation flags: Unknown error";
      logger()->critical(stream.str(), Aspect::AD_FRONTEND);
    }
  }

  void AdFrontend::add_hit_channels_(
    std::vector<AdServer::UserInfoSvcs::Types::ChannelTriggerMatch>& result_channel_ids,
    const AdServer::CampaignSvcs::ChannelIdArray& hit_channels)
  {
    result_channel_ids.reserve(
      result_channel_ids.size() + hit_channels.size());
    for (const auto& hit_channel : hit_channels)
    {
      result_channel_ids.emplace_back(hit_channel, 0);
    }
  }
  
  void AdFrontend::prepare_ui_match_params_(
    AdServer::UserInfoSvcs::Types::MatchParams& match_params,
    const AdServer::ChannelSvcs::Proto::MatchResponseInfo* match_result,
    const RequestInfo& request_info)
  {
    using ChannelMatchSet = std::set<ChannelMatch>;

    if(match_result && !match_result->no_track())
    {
      ChannelMatchSet url_channels;
      ChannelMatchSet page_channels;
      ChannelMatchSet search_channels;
      ChannelMatchSet url_keyword_channels;

      const auto& matched_channels = match_result->matched_channels();
      std::transform(
        std::begin(matched_channels.url_channels()),
        std::end(matched_channels.url_channels()),
        std::inserter(url_channels, url_channels.end()),
        GetChannelTriggerId());

      std::transform(
        std::begin(matched_channels.page_channels()),
        std::end(matched_channels.page_channels()),
        std::inserter(page_channels, page_channels.end()),
        GetChannelTriggerId());

      std::transform(
        std::begin(matched_channels.search_channels()),
        std::end(matched_channels.search_channels()),
        std::inserter(search_channels, search_channels.end()),
        GetChannelTriggerId());

      std::transform(
        std::begin(matched_channels.url_keyword_channels()),
        std::end(matched_channels.url_keyword_channels()),
        std::inserter(url_keyword_channels, url_keyword_channels.end()),
        GetChannelTriggerId());

      auto& url_channel_ids = match_params.url_channel_ids;
      url_channel_ids.reserve(url_channels.size());
      for (const auto& url_channel : url_channels)
      {
        url_channel_ids.emplace_back(
          url_channel.channel_id,
          url_channel.channel_trigger_id);
      }

      auto& page_channel_ids = match_params.page_channel_ids;
      page_channel_ids.reserve(page_channels.size());
      for (const auto& page_channel : page_channels)
      {
        page_channel_ids.emplace_back(
          page_channel.channel_id,
          page_channel.channel_trigger_id);
      }

      auto& search_channel_ids = match_params.search_channel_ids;
      search_channel_ids.reserve(search_channels.size());
      for (const auto& search_channel : search_channels)
      {
        search_channel_ids.emplace_back(
          search_channel.channel_id,
          search_channel.channel_trigger_id);
      }

      auto& url_keyword_channel_ids = match_params.url_keyword_channel_ids;
      url_keyword_channel_ids.reserve(url_keyword_channels.size());
      for (const auto& url_keyword_channel : url_keyword_channels)
      {
        url_keyword_channel_ids.emplace_back(
          url_keyword_channel.channel_id,
          url_keyword_channel.channel_trigger_id);
      }

      auto& persistent_channel_ids = match_params.persistent_channel_ids;
      persistent_channel_ids.reserve(request_info.platform_ids.size());
      persistent_channel_ids.insert(
        std::end(persistent_channel_ids),
        std::begin(request_info.platform_ids),
        std::end(request_info.platform_ids));
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
} // namespace AdServer::Grpc