
#include <atomic>
#include <sstream>
#include <algorithm>
#include <set>
#include <type_traits>

#include <google/protobuf/arena.h>

#include <Logger/StreamLogger.hpp>
#include <HTTP/HTTPCookie.hpp>
#include <String/StringManip.hpp>
#include <String/AsciiStringManip.hpp>

#include <Generics/Uuid.hpp>
#include <Generics/GnuHashTable.hpp>
#include <Generics/HashTableAdapters.hpp>
#include <Generics/CompositeMetricsProvider.hpp>

#include <Commons/UserInfoManip.hpp>
#include <Commons/Algs.hpp>

#include <Commons/ErrorHandler.hpp>
#include <Commons/GrpcAlgs.hpp>
#include <LogCommons/AdRequestLogger.hpp>
#include <ChannelSvcs/ChannelCommons/ChannelUtils.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>

#include <Frontends/FrontendCommons/OptOutManip.hpp>
#include <Frontends/FrontendCommons/add_UID_cookie.hpp>
#include <Frontends/FrontendCommons/CampaignManagerGrpcClientConfig.hpp>

#include <Frontends/FrontendCommons/UserInfoClientConfig.hpp>

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

  class GrpcClientMetricsProvider final : public Generics::MetricsProvider
  {
  public:
    struct ClientSource
    {
      std::string prefix;
      std::weak_ptr<AdServer::Grpc::Client> client;
    };

    explicit GrpcClientMetricsProvider(std::vector<ClientSource> clients)
      : clients_(std::move(clients))
    {}

    MetricArray get_values() override
    {
      MetricArray result;
      for (const auto& source : clients_)
      {
        if (auto client = source.client.lock())
        {
          add_client_stats_(result, source.prefix, client->stats());
        }
      }
      return result;
    }

  private:
    static void add_counter_(
      MetricArray& result,
      const std::string& prefix,
      const char* name,
      std::uint64_t value)
    {
      result.emplace_back(prefix + "_" + name, static_cast<long>(value));
    }

    static void add_client_stats_(
      MetricArray& result,
      const std::string& prefix,
      const AdServer::Grpc::Stats& stats)
    {
      add_counter_(result, prefix, "input_items", stats.input_items);
      add_counter_(result, prefix, "call_total", stats.input_items);
      add_counter_(result, prefix, "completed_items", stats.completed_items);
      add_counter_(
        result,
        prefix,
        "completed_error_items",
        stats.completed_error_items);
      add_counter_(
        result,
        prefix,
        "call_error_total",
        stats.completed_error_items);
      add_counter_(
        result,
        prefix,
        "outstanding_items",
        stats.input_items > stats.completed_items ?
          stats.input_items - stats.completed_items :
          0);
      add_counter_(result, prefix, "write_batches", stats.write_batches);
      add_counter_(result, prefix, "batch_total", stats.write_batches);
      add_counter_(result, prefix, "write_batch_total", stats.write_batches);
      add_counter_(result, prefix, "write_items", stats.write_items);
      add_counter_(result, prefix, "read_batches", stats.read_batches);
      add_counter_(result, prefix, "read_batch_total", stats.read_batches);
      add_counter_(result, prefix, "read_items", stats.read_items);
      add_counter_(result, prefix, "queue_wait_total", stats.queue_wait_count);
      add_counter_(result, prefix, "queue_wait_time", stats.queue_wait_sum_us);
      add_counter_(result, prefix, "queue_wait_max_time", stats.queue_wait_max_us);
      add_counter_(
        result,
        prefix,
        "queue_timeout_total",
        stats.queue_timeout_count);
      add_counter_(
        result,
        prefix,
        "response_wait_total",
        stats.response_wait_count);
      add_counter_(
        result,
        prefix,
        "response_wait_time",
        stats.response_wait_sum_us);
      add_counter_(
        result,
        prefix,
        "response_wait_max_time",
        stats.response_wait_max_us);
      add_counter_(result, prefix, "queue_items", stats.queue_items);
      add_counter_(result, prefix, "pending_batches", stats.pending_batches);
      add_counter_(
        result,
        prefix,
        "pending_batch_items",
        stats.pending_batch_items);
      add_counter_(result, prefix, "inflight_items", stats.inflight_items);
      add_counter_(
        result,
        prefix,
        "stream_inflight_items",
        stats.stream_inflight_items);
      add_counter_(result, prefix, "active_streams", stats.active_streams);
      add_counter_(result, prefix, "available_streams", stats.available_streams);
      add_counter_(
        result,
        prefix,
        "connecting_streams",
        stats.connecting_streams);
      add_counter_(result, prefix, "draining_streams", stats.draining_streams);
      add_counter_(result, prefix, "deferred_streams", stats.deferred_streams);

      if (stats.consumer_stream_write.has_value())
      {
        add_counter_(
          result,
          prefix,
          "consumer_stream_write_total",
          stats.consumer_stream_write->count);
        add_counter_(
          result,
          prefix,
          "consumer_stream_write_time",
          stats.consumer_stream_write->sum_us);
        add_counter_(
          result,
          prefix,
          "consumer_stream_write_max_time",
          stats.consumer_stream_write->max_us);
      }

      if (stats.last_error.has_value())
      {
        result.emplace_back(
          prefix + "_last_error_time",
          stats.last_error->time.get_gm_time().format("%F %T"));
        result.emplace_back(
          prefix + "_last_error_endpoint",
          stats.last_error->endpoint);
        result.emplace_back(
          prefix + "_last_error_code",
          static_cast<long>(stats.last_error->code));
        result.emplace_back(
          prefix + "_last_error_message",
          stats.last_error->message);
        result.emplace_back(
          prefix + "_last_error_source",
          stats.last_error->source);
      }
    }

  private:
    std::vector<ClientSource> clients_;
  };

  struct ValuesMetricWriter
  {
    explicit ValuesMetricWriter(Generics::MetricsProvider::MetricArray& result)
      : result(result)
    {}

    void
    operator()(const std::size_t)
    {}

    template<typename Type>
    void
    operator()(
      const Generics::Values::Key& key,
      const Type& value)
    {
      if constexpr (std::is_integral_v<Type>)
      {
        result.emplace_back(key.text(), static_cast<long>(value));
      }
      else if constexpr (std::is_floating_point_v<Type>)
      {
        result.emplace_back(key.text(), static_cast<double>(value));
      }
      else
      {
        result.emplace_back(key.text(), value);
      }
    }

    Generics::MetricsProvider::MetricArray& result;
  };

  class AdFrontendMetricsProvider final : public Generics::MetricsProvider
  {
  public:
    explicit AdFrontendMetricsProvider(AdServer::AdFrontendStat* stats)
      : stats_(ReferenceCounting::add_ref(stats))
    {}

    MetricArray get_values() override
    {
      MetricArray result;
      if(stats_.in())
      {
        Generics::Values_var values = stats_->extract_stats_values();
        ValuesMetricWriter writer(result);
        values->enumerate_all(writer);
      }
      return result;
    }

  private:
    AdServer::AdFrontendStat_var stats_;
  };

  class AdRequestInProgressGuard
  {
  public:
    explicit AdRequestInProgressGuard(AdServer::AdFrontendStat* stats) noexcept
      : stats_(ReferenceCounting::add_ref(stats))
    {
      if(stats_.in())
      {
        stats_->add_request();
      }
    }

    ~AdRequestInProgressGuard() noexcept
    {
      if(stats_.in())
      {
        stats_->complete_request();
      }
    }

    AdRequestInProgressGuard(const AdRequestInProgressGuard&) = delete;
    AdRequestInProgressGuard& operator=(const AdRequestInProgressGuard&) = delete;

  private:
    AdServer::AdFrontendStat_var stats_;
  };

  class AdStageInProgressGuard
  {
  public:
    AdStageInProgressGuard(
      AdServer::AdFrontendStat* stats,
      AdServer::AdFrontendStat::Stage stage,
      bool track_time = true)
      noexcept
      : stats_(ReferenceCounting::add_ref(stats)),
        stage_(stage),
        track_time_(track_time),
        started_at_(track_time ? Generics::Time::get_time_of_day() :
          Generics::Time::ZERO)
    {
      if(stats_.in())
      {
        stats_->add_stage(stage_);
      }
    }

    ~AdStageInProgressGuard() noexcept
    {
      if(stats_.in())
      {
        stats_->complete_stage(stage_);
        if(track_time_)
        {
          stats_->add_stage_time(
            stage_,
            Generics::Time::get_time_of_day() - started_at_);
        }
      }
    }

    void
    add_error() noexcept
    {
      if(stats_.in())
      {
        stats_->add_stage_error(stage_);
      }
    }

    AdStageInProgressGuard(const AdStageInProgressGuard&) = delete;
    AdStageInProgressGuard& operator=(const AdStageInProgressGuard&) = delete;

  private:
    AdServer::AdFrontendStat_var stats_;
    AdServer::AdFrontendStat::Stage stage_;
    bool track_time_;
    Generics::Time started_at_;
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
    for(std::size_t i = 0; i < source.length(); ++i)
    {
      result[i] = static_cast<char>(source[i]);
    }
    return result;
  }
}

namespace Aspect
{
  extern const char AD_FRONTEND[] = "AdFrontend";
}

namespace Request::Context
{
    const String::AsciiStringManip::Caseless CLIENT_ID("uid");
    const String::AsciiStringManip::Caseless OPTIN("OPTED_IN");
  }

namespace Request::Cookie
  {
    const Generics::SubStringHashAdapter OPTOUT(String::SubString("OPTED_OUT"));
    const Generics::SubStringHashAdapter OPTOUT_TRUE_VALUE(String::SubString("YES"));
    const Generics::SubStringHashAdapter OI_PROMPT(String::SubString("oi_prompt"));
    const Generics::SubStringHashAdapter OI_PROMPT_VALUE(String::SubString("yes-trial-end"));
    const Generics::SubStringHashAdapter OPT_IN_TRIAL(String::SubString("trialoptin"));
    const Generics::SubStringHashAdapter LAST_COLOCATION_ID(String::SubString("lc"));
  }


namespace AdServer
{
  namespace
  {
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

  struct AdFrontend::RequestContext
  {
    FCGI::HttpRequestHolder_var request_holder;
    FCGI::HttpResponse_var response_ptr;
    RequestInfo request_info;
    PassbackInfo passback_info;
    DebugSink debug_sink;
    RequestTimeMetering request_time_metering;
    std::string str_response;
    int http_status = 200;

    explicit RequestContext(bool allow_show_history_profile)
      : response_ptr(new FCGI::HttpResponse()),
        debug_sink(allow_show_history_profile)
    {}
  };

  /**
   *  AdFrontend implementation
   */
  AdFrontend::AdFrontend(
    Configuration* frontend_config,
    Logging::Logger* logger,
    std::shared_ptr<AdServer::Commons::ExecutorPool> request_workers,
    CommonModule* common_module,
    Generics::CompositeMetricsProvider* composite_metrics_provider)
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
      fe_config_path_(frontend_config->path()),
      frontend_config_(ReferenceCounting::add_ref(frontend_config)),
      common_module_(ReferenceCounting::add_ref(common_module)),
      composite_metrics_provider_(
        ReferenceCounting::add_ref(composite_metrics_provider)),
      workers_(std::move(request_workers))
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

        task_runner_ = new Generics::TaskRunner(callback(), 2);
        task_scheduler_ = new FrontendCommons::TaskScheduler(
          callback(), task_runner_);
        add_child_object(task_scheduler_.in());
        grpc_executor_ = common_module_->grpc_executor();

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

        auto campaign_manager_client =
          std::make_shared<
            AdServer::CampaignSvcs::CampaignManagerDistributedGrpcClient>(
              FrontendCommons::read_campaign_manager_grpc_refs(*common_config_),
              AdServer::Grpc::BatchingOptions(),
              grpc_executor_,
              common_module_->grpc_coalesce_runner());
        campaign_manager_coro_ = std::make_shared<
          AdServer::CampaignSvcs::CampaignManagerGrpcCoroClient>(
            campaign_manager_client,
            workers_);
        add_child_object(campaign_manager_client);

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

        stats_ = new AdFrontendStat();

        if (composite_metrics_provider_)
        {
          std::vector<GrpcClientMetricsProvider::ClientSource> client_sources;
          const auto add_client_source =
            [&client_sources](const char* prefix, const auto& client)
            {
              if (client)
              {
                client_sources.push_back(GrpcClientMetricsProvider::ClientSource{
                  prefix,
                  std::static_pointer_cast<AdServer::Grpc::Client>(client)
                });
              }
            };

          add_client_source("ad_user_info_client", user_info_client);
          add_client_source("ad_campaign_client", campaign_manager_client);
          add_client_source("ad_user_bind_client", user_bind_client);
          add_client_source("ad_channel_client", channel_client);

          ReferenceCounting::SmartPtr<Generics::MetricsProvider>
            grpc_client_metrics_provider(
              new GrpcClientMetricsProvider(std::move(client_sources)));
          composite_metrics_provider_->add_provider(
            grpc_client_metrics_provider.in());

          ReferenceCounting::SmartPtr<Generics::MetricsProvider>
            ad_frontend_metrics_provider(
              new AdFrontendMetricsProvider(stats_.in()));
          composite_metrics_provider_->add_provider(
            ad_frontend_metrics_provider.in());
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
            common_module_->ip_mapper(),
            user_agent_filter_path.c_str(),
            set_uid_controller,
            common_config_->DebugInfo().use_acl() ? &acl_list : 0,
            acl_colo,
            Commons::LogReferrer::read_log_referrer_settings(
              config_->use_referrer_site_referrer_stats())));
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
  FrontendCommons::RequestResult
  AdFrontend::finish_request_(
    const std::shared_ptr<RequestContext>& context)
    noexcept
  {
    FCGI::HttpResponse_var response_ptr = context->response_ptr;
    if(!response_ptr)
    {
      response_ptr = new FCGI::HttpResponse();
      context->response_ptr = response_ptr;
    }

    try
    {
      FCGI::HttpResponse& response = *response_ptr;
      if(context->request_holder)
      {
        const FCGI::HttpRequest& request = context->request_holder->request();

        HTTP::CookieList cookies;
        cookies.load_from_headers(request.headers());

        cookie_manager_->remove(
          response, request, cookies, remove_cookies_);

        if(context->request_info.do_opt_out)
        {
          opt_out_client_(
            cookies,
            response,
            request,
            context->request_info);
        }

        if(context->request_info.have_uid_cookie)
        {
          FrontendCommons::add_UID_cookie(
            response,
            request,
            *cookie_manager_,
            context->request_info.signed_client_id);
        }

        if(context->request_info.format == "vast")
        {
          FrontendCommons::CORS::set_headers(request, response);
        }
      }

      context->debug_sink.write_response(
        response,
        context->str_response,
        context->http_status);

      if(common_config_->ResponseHeaders().present())
      {
        FrontendCommons::add_headers(
          *(common_config_->ResponseHeaders()),
          response);
      }

      if(logger()->log_level() >= TraceLevel::MIDDLE)
      {
        Stream::Error ostr;
        ostr << "AdFrontend::handle_request(): response:" <<
          std::endl << context->str_response;

        logger()->log(
          ostr.str(),
          TraceLevel::MIDDLE,
          Aspect::AD_FRONTEND);
      }

      if(context->http_status != 204)
      {
        response.get_output_stream().write(
          context->str_response.c_str(),
          context->str_response.length());
      }
    }
    catch(const eh::Exception& ex)
    {
      context->http_status = 500;
      Stream::Error ostr;
      ostr << "AdFrontend::finish_request_(): eh::Exception caught: " <<
        ex.what();
      logger()->log(
        ostr.str(),
        Logging::Logger::EMERGENCY,
        Aspect::AD_FRONTEND,
        "ADS-IMPL-109");
      if(!response_ptr)
      {
        response_ptr = new FCGI::HttpResponse();
        context->response_ptr = response_ptr;
      }
      context->debug_sink.fill_debug_body(
        *response_ptr,
        context->http_status,
        ostr);
    }

    if(stats_.in())
    {
      stats_->consider_request(
        context->request_info,
        context->request_time_metering);
    }

    if(context->http_status != 200)
    {
      try
      {
        if(!context->request_info.original_url.empty())
        {
          context->http_status = FrontendCommons::redirect(
            context->request_info.original_url,
            *response_ptr);
        }
        else if(!context->passback_info.url.empty())
        {
          context->http_status = FrontendCommons::redirect(
            context->passback_info.url,
            *response_ptr);
        }
      }
      catch(...)
      {}
    }

    return FrontendCommons::RequestResult{
      context->http_status,
      response_ptr,
      false};
  }

  FrontendCommons::RequestTask
  AdFrontend::co_handle_request(
    FCGI::HttpRequestHolder_var request_holder)
    noexcept
  {
    static const char* FUN = "AdFrontend::handle_request()";

    co_await AdServer::Commons::ExecutorPool::yield(workers_);

    auto context = std::make_shared<RequestContext>(
      common_config_->DebugInfo().show_history_matching());
    context->request_holder = std::move(request_holder);
    const FCGI::HttpRequest& request = context->request_holder->request();
    AdRequestInProgressGuard request_in_progress(stats_.in());

    if(logger()->log_level() >= TraceLevel::MIDDLE)
    {
      logger()->log(String::SubString("AdFrontend::handle_request: entered"),
        TraceLevel::MIDDLE,
        Aspect::AD_FRONTEND);
    }

    try
    {
      log_request("AdFrontend::handle_request", request, TraceLevel::MIDDLE);

      TimeGuard request_fill_time_metering;

      request_info_filler_->fill(
        context->request_info,
        &context->debug_sink,
        request);

      context->request_time_metering.request_fill_time =
        request_fill_time_metering.consider();

      const bool success = co_await co_acquire_ad_(context);
      if(!success)
      {
        context->http_status = 500;
      }

      co_return finish_request_(context);
    }
    catch(const ForbiddenException& ex)
    {
      context->http_status = 403;
      if(logger()->log_level() >= TraceLevel::LOW ||
        context->debug_sink.require_debug_body())
      {
        Stream::Error ostr;
        ostr << FUN << ": ForbiddenException caught: " << ex.what();

        if(logger()->log_level() >= TraceLevel::MIDDLE)
        {
          logger()->log(ostr.str(), TraceLevel::LOW, Aspect::AD_FRONTEND);
        }

        context->debug_sink.fill_debug_body(
          *context->response_ptr,
          context->http_status,
          ostr);
      }
      co_return finish_request_(context);
    }
    catch(const InvalidParamException& e)
    {
      context->http_status = 400;
      if(logger()->log_level() >= TraceLevel::MIDDLE ||
        context->debug_sink.require_debug_body())
      {
        Stream::Error ostr;
        ostr << FUN << ": InvalidParamException caught: " << e.what();

        if(logger()->log_level() >= TraceLevel::MIDDLE)
        {
          logger()->log(ostr.str(), TraceLevel::MIDDLE, Aspect::AD_FRONTEND);
        }

        context->debug_sink.fill_debug_body(
          *context->response_ptr,
          context->http_status,
          ostr);
      }
      co_return finish_request_(context);
    }
    catch(const HTTP::CookieList::Exception& e)
    {
      context->http_status = 400;
      Stream::Error ostr;
      ostr << FUN << ": HTTP::CookieList::Exception caught: " << e.what();
      logger()->log(
        ostr.str(),
        Logging::Logger::NOTICE,
        Aspect::AD_FRONTEND);
      context->debug_sink.fill_debug_body(
        *context->response_ptr,
        context->http_status,
        ostr);
      co_return finish_request_(context);
    }
    catch(const eh::Exception& e)
    {
      context->http_status = 500;
      Stream::Error ostr;
      ostr << FUN << ": eh::Exception caught: " << e.what();
      logger()->log(
        ostr.str(),
        Logging::Logger::EMERGENCY,
        Aspect::AD_FRONTEND,
        "ADS-IMPL-109");
      context->debug_sink.fill_debug_body(
        *context->response_ptr,
        context->http_status,
        ostr);
      co_return finish_request_(context);
    }

  }

  AdFrontend::MergeUsersTask
  AdFrontend::co_merge_users_(
    const std::shared_ptr<RequestContext>& context)
    noexcept
  {
    const RequestInfo& request_info = context->request_info;
    const bool merge_temp = request_info.merge_persistent_client_id.is_null();
    const AdServer::Commons::UserId merged_uid =
      merge_temp ? request_info.temp_client_id :
        request_info.merge_persistent_client_id;

    if(!user_info_client_coro_)
    {
      co_return MergeUsersResult{
        false,
        Generics::Time::ZERO,
        MergeMessage::SOURCE_NOT_READY};
    }

    if(merged_uid == AdServer::Commons::PROBE_USER_ID)
    {
      co_return MergeUsersResult{
        false,
        Generics::Time::ZERO,
        MergeMessage::SOURCE_IS_PROBE};
    }

    TimeGuard user_merge_time_metering;
    google::protobuf::Arena get_profile_request_arena;
    auto* get_profile_request = google::protobuf::Arena::CreateMessage<
      adserver::user_info_svcs::user_info_manager::GetUserProfileRequest>(
        &get_profile_request_arena);
    get_profile_request->set_user_id(GrpcAlgs::pack_user_id(merged_uid));
    get_profile_request->set_temporary(merge_temp);
    auto* profile_request = get_profile_request->mutable_profile_request();
    profile_request->set_base_profile(true);
    profile_request->set_add_profile(true);
    profile_request->set_history_profile(true);
    profile_request->set_freq_cap_profile(!merge_temp);
    profile_request->set_pref_profile(false);

    auto get_profile_result =
      co_await user_info_client_coro_->get_user_profile(
        *get_profile_request);
    context->request_time_metering.merge_users_time =
      user_merge_time_metering.consider();

    if(!get_profile_result.status.ok())
    {
      Stream::Error ostr;
      ostr << "UserInfoManager::get_user_profile(): "
        "gRPC call failed: code=" <<
        static_cast<int>(get_profile_result.status.error_code()) <<
        ", message=" << get_profile_result.status.error_message();
      logger()->log(
        ostr.str(),
        get_profile_result.status.error_code() == grpc::StatusCode::UNAVAILABLE ?
          Logging::Logger::WARNING :
          Logging::Logger::NOTICE,
        Aspect::AD_FRONTEND,
        get_profile_result.status.error_code() == grpc::StatusCode::UNAVAILABLE ?
          "" : "ADS-IMPL-111");
      co_return MergeUsersResult{
        false,
        Generics::Time::ZERO,
        get_profile_result.status.error_code() == grpc::StatusCode::UNAVAILABLE ?
          MergeMessage::SOURCE_IS_UNAVAILABLE :
          MergeMessage::SOURCE_EXCEPTION};
    }

    const auto& response = get_profile_result.response;
    if(!response.found() ||
      (response.user_profile().base_user_profile().empty() &&
        response.user_profile().add_user_profile().empty()))
    {
      co_return MergeUsersResult{
        false,
        Generics::Time::ZERO,
        MergeMessage::SOURCE_IS_UNKNOWN};
    }

    if(context->request_info.remove_merged_uid)
    {
      google::protobuf::Arena remove_request_arena;
      auto* remove_request = google::protobuf::Arena::CreateMessage<
        adserver::user_info_svcs::user_info_manager::RemoveUserProfileRequest>(
          &remove_request_arena);
      remove_request->set_user_id(GrpcAlgs::pack_user_id(merged_uid));
      auto remove_result =
        co_await user_info_client_coro_->remove_user_profile(
          *remove_request);
      if(!remove_result.status.ok())
      {
        Stream::Error ostr;
        ostr << "UserInfoManager::remove_user_profile(): "
          "gRPC call failed: code=" <<
          static_cast<int>(remove_result.status.error_code()) <<
          ", message=" << remove_result.status.error_message();
        logger()->log(
          ostr.str(),
          Logging::Logger::NOTICE,
          Aspect::AD_FRONTEND,
          "ADS-IMPL-111");
      }
    }

    if(context->request_info.silent_match)
    {
      logger()->log(
        String::SubString(
          "AdFrontend::co_merge_users_(): merge operation with "
          "installed silent_match"),
        Logging::Logger::EMERGENCY,
        Aspect::AD_FRONTEND,
        "ADS-IMPL-111");
      co_return MergeUsersResult{
        false,
        Generics::Time::ZERO,
        MergeMessage::MERGE_EXCEPTION};
    }

    google::protobuf::Arena merge_request_arena;
    auto* merge_request = google::protobuf::Arena::CreateMessage<
      adserver::user_info_svcs::user_info_manager::MergeRequest>(
        &merge_request_arena);
    auto* user_info = merge_request->mutable_user_info();
    user_info->set_user_id(
      GrpcAlgs::pack_user_id(context->request_info.client_id));
    user_info->set_last_colo_id(context->request_info.last_colo_id);
    user_info->set_request_colo_id(context->request_info.colo_id);
    user_info->set_current_colo_id(-1);
    user_info->set_temporary(
      context->request_info.user_status ==
        AdServer::CampaignSvcs::US_TEMPORARY);
    user_info->set_time(context->request_info.current_time.tv_sec);

    auto* match_params = merge_request->mutable_match_params();
    match_params->set_use_empty_profile(
      context->request_info.user_status !=
        AdServer::CampaignSvcs::US_OPTIN &&
      context->request_info.user_status !=
        AdServer::CampaignSvcs::US_TEMPORARY);
    match_params->set_silent_match(
      context->request_info.silent_match);
    match_params->set_no_match(context->request_info.no_match);
    match_params->set_no_result(context->request_info.no_result);
    match_params->set_provide_persistent_channels(false);
    match_params->set_change_last_request(true);
    match_params->set_filter_contextual_triggers(false);
    match_params->set_publishers_optin_timeout(
      context->request_info.tag_id != 0 ?
        GrpcAlgs::pack_time(
          context->request_info.current_time -
            Generics::Time::ONE_DAY * 15) :
        GrpcAlgs::pack_time(Generics::Time::ZERO));
    *merge_request->mutable_merge_user_profile() =
      response.user_profile();

    auto merge_result = co_await user_info_client_coro_->merge(
      *merge_request);
    if(!merge_result.status.ok())
    {
      Stream::Error ostr;
      ostr << "UserInfoManager::merge(): "
        "gRPC call failed: code=" <<
        static_cast<int>(merge_result.status.error_code()) <<
        ", message=" << merge_result.status.error_message();
      logger()->log(
        ostr.str(),
        merge_result.status.error_code() ==
          grpc::StatusCode::UNAVAILABLE ?
            Logging::Logger::WARNING :
            Logging::Logger::EMERGENCY,
        Aspect::AD_FRONTEND,
        merge_result.status.error_code() ==
          grpc::StatusCode::UNAVAILABLE ?
            "" : "ADS-IMPL-111");
      co_return MergeUsersResult{
        false,
        Generics::Time::ZERO,
        merge_result.status.error_code() == grpc::StatusCode::UNAVAILABLE ?
          MergeMessage::MERGE_UNAVAILABLE :
          MergeMessage::MERGE_EXCEPTION};
    }

    co_return MergeUsersResult{
      merge_result.response.result() &&
        merge_result.response.merge_success(),
      merge_result.response.last_request().empty() ?
        Generics::Time::ZERO :
        GrpcAlgs::unpack_time(merge_result.response.last_request()),
      merge_result.response.result() &&
        merge_result.response.merge_success() ?
          std::string() : MergeMessage::MERGE_EXCEPTION};
  }

  std::shared_ptr<
    adserver::user_info_svcs::user_info_manager::MatchResponse>
  AdFrontend::get_empty_history_matching()
    /*throw(eh::Exception)*/
  {
    auto res = std::make_shared<
      adserver::user_info_svcs::user_info_manager::MatchResponse>();
    auto* match_result = res->mutable_match_result();
    match_result->set_fraud_request(false);
    match_result->set_times_inited(false);
    match_result->set_last_request_time(GrpcAlgs::pack_time(Generics::Time::ZERO));
    match_result->set_create_time(GrpcAlgs::pack_time(Generics::Time::ZERO));
    match_result->set_session_start(GrpcAlgs::pack_time(Generics::Time::ZERO));
    match_result->set_process_time(GrpcAlgs::pack_time(Generics::Time::ZERO));
    match_result->set_colo_id(-1);
    return res;
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

  AdFrontend::UserInfoMatcherTask
  AdFrontend::co_acquire_user_info_matcher_(
    const std::shared_ptr<RequestContext>& context,
    const adserver::channel_svcs::channel_server::MatchResponse*
      trigger_matching_result,
    bool trigger_matching_result_present)
    noexcept
  {
    const RequestInfo& request_info = context->request_info;
    const bool do_history_matching =
      request_info.user_status == AdServer::CampaignSvcs::US_OPTIN ||
      request_info.user_status == AdServer::CampaignSvcs::US_TEMPORARY;

    auto finish = [&](
      AdServer::Grpc::ResponseHolder<
        adserver::user_info_svcs::user_info_manager::MatchResponse>
          match_response,
      bool match_success)
    {
        if(!match_response)
        {
          auto empty_response = *get_empty_history_matching();
          match_response =
            AdServer::Grpc::ResponseHolder<
              adserver::user_info_svcs::user_info_manager::MatchResponse>::
                make_value(std::move(empty_response));
        }
        auto* match_result = match_response->mutable_match_result();

        if(trigger_matching_result_present &&
          !trigger_matching_result->no_track())
        {
          if(!match_success || !do_history_matching)
          {
            auto empty_response = *get_empty_history_matching();
            match_response =
              AdServer::Grpc::ResponseHolder<
                adserver::user_info_svcs::user_info_manager::MatchResponse>::
                  make_value(std::move(empty_response));
            match_result = match_response->mutable_match_result();
            const auto& content_channels =
              trigger_matching_result->content_channels();

            for(const auto& content_channel : content_channels)
            {
              auto* channel = match_result->add_channels();
              channel->set_channel_id(content_channel.id());
              channel->set_weight(content_channel.weight());
            }
          }
          else if(context->request_info.tag_id != 0 &&
            !config_->ad_request_profiling())
          {
            const auto& content_channels =
              trigger_matching_result->content_channels();
            std::set<std::uint64_t> channel_ids;
            for(const auto& channel : match_result->channels())
            {
              channel_ids.insert(channel.channel_id());
            }
            for(const auto& content_channel : content_channels)
            {
              if(channel_ids.insert(content_channel.id()).second)
              {
                auto* channel = match_result->add_channels();
                channel->set_channel_id(content_channel.id());
                channel->set_weight(content_channel.weight());
              }
            }
          }
        }

        return UserInfoMatcherResult{std::move(match_response), match_success};
    };

    if(!user_info_client_coro_ || !do_history_matching)
    {
      co_return finish({}, false);
    }

    AdStageInProgressGuard history_match_in_progress(
      stats_.in(),
      AdFrontendStat::Stage::HistoryMatch);

    google::protobuf::Arena history_match_request_arena;
    auto* history_match_request = google::protobuf::Arena::CreateMessage<
      adserver::user_info_svcs::user_info_manager::MatchRequest>(
        &history_match_request_arena);
    auto* user_info = history_match_request->mutable_user_info();
    user_info->set_user_id(GrpcAlgs::pack_user_id(request_info.client_id));
    user_info->set_last_colo_id(request_info.last_colo_id);
    user_info->set_request_colo_id(
      request_info.user_status != AdServer::CampaignSvcs::US_TEMPORARY ?
      request_info.colo_id : -1);
    user_info->set_current_colo_id(-1);
    user_info->set_temporary(
      request_info.user_status == AdServer::CampaignSvcs::US_TEMPORARY);
    user_info->set_time(request_info.current_time.tv_sec);

    auto* match_params = history_match_request->mutable_match_params();
    match_params->set_use_empty_profile(false);
    match_params->set_silent_match(request_info.silent_match);
    match_params->set_no_match(request_info.no_match ||
      (trigger_matching_result_present && trigger_matching_result->no_track()));
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

    if(request_info.coord_location)
    {
      auto* geo_data = match_params->add_geo_data_seq();
      geo_data->set_latitude(
        GrpcAlgs::pack_decimal(request_info.coord_location->latitude));
      geo_data->set_longitude(
        GrpcAlgs::pack_decimal(request_info.coord_location->longitude));
      geo_data->set_accuracy(
        GrpcAlgs::pack_decimal(request_info.coord_location->accuracy));
    }

    if(request_info.tag_id == 0 || config_->ad_request_profiling())
    {
      prepare_ui_match_params_(
        *match_params,
        trigger_matching_result_present ? trigger_matching_result : nullptr,
        request_info);
    }

    match_params->set_cohort(request_info.curct);

    TimeGuard history_match_time_metering;
    auto match_result = co_await user_info_client_coro_->match(
      *history_match_request);
    if(!match_result.status.ok())
    {
      history_match_in_progress.add_error();
      Stream::Error ostr;
      ostr << "UserInfoManager::match(): gRPC call failed: code=" <<
        static_cast<int>(match_result.status.error_code()) <<
        ", message=" << match_result.status.error_message();
      logger()->log(
        ostr.str(),
        match_result.status.error_code() == grpc::StatusCode::UNAVAILABLE ?
          Logging::Logger::WARNING :
          Logging::Logger::EMERGENCY,
        Aspect::AD_FRONTEND,
        match_result.status.error_code() == grpc::StatusCode::UNAVAILABLE ?
          "" : "ADS-IMPL-112");
      co_return finish({}, false);
    }

    context->request_time_metering.matched_channels =
      match_result.response.match_result().channels_size();
    context->request_time_metering.history_match_time =
      history_match_time_metering.consider();
    context->request_time_metering.history_match_local_time =
      GrpcAlgs::unpack_time(
        match_result.response.match_result().process_time());

    co_return finish(std::move(match_result.response_holder), true);
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

      if(!user_info_client_coro_)
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
          std::vector<unsigned long> campaign_ids;
          std::vector<unsigned long> uc_campaign_ids;
          for(int creative_i = 0;
            creative_i < ad_slot_result.selected_creatives_size();
            ++creative_i)
          {
            const CM::CreativeSelectResult& creative =
              ad_slot_result.selected_creatives(creative_i);

            if(ad_slot_result.track_impr())
            {
              uc_campaign_ids.push_back(creative.campaign_group_id());
            }
            else
            {
              campaign_ids.push_back(creative.campaign_group_id());
            }
          }

          adserver::user_info_svcs::user_info_manager::
            UpdateUserFreqCapsRequest update_request;
          update_request.set_user_id(GrpcAlgs::pack_user_id(
            request_info.client_id));
          update_request.set_time(GrpcAlgs::pack_time(
            request_info.current_time));
          Commons::RequestId request_id;
          try
          {
            request_id = GrpcAlgs::unpack_request_id(
              ad_slot_result.request_id());
          }
          catch(const std::exception& ex)
          {
            Stream::Error ostr;
            ostr << FUN << ": skip post match for ad slot #" << ad_slot_i <<
              ": invalid request_id size=" <<
              ad_slot_result.request_id().size() <<
              ": " << ex.what();

            logger()->log(
              ostr.str(),
              Logging::Logger::EMERGENCY,
              Aspect::AD_FRONTEND,
              "ADS-IMPL-112");
            continue;
          }

          update_request.set_request_id(GrpcAlgs::pack_request_id(request_id));
          update_request.mutable_freq_caps()->Add(
            ad_slot_result.freq_caps().begin(),
            ad_slot_result.freq_caps().end());
          update_request.mutable_uc_freq_caps()->Add(
            ad_slot_result.uc_freq_caps().begin(),
            ad_slot_result.uc_freq_caps().end());
          for(const auto& creative : ad_slot_result.selected_creatives())
          {
            if(creative.order_set_id())
            {
              auto* seq_order = update_request.add_seq_orders();
              seq_order->set_ccg_id(creative.cmp_id());
              seq_order->set_set_id(creative.order_set_id());
              seq_order->set_imps(1);
            }
          }
          update_request.mutable_campaign_ids()->Add(
            campaign_ids.begin(),
            campaign_ids.end());
          update_request.mutable_uc_campaign_ids()->Add(
            uc_campaign_ids.begin(),
            uc_campaign_ids.end());

          co_user_info_post_match_(std::move(update_request)).
            start_detached(nullptr);
        } // ad_slot_result.selected_creatives_size() > 0
      }
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN <<
        ": post match failed: " << e.what();

      logger()->log(ostr.str(),
        Logging::Logger::EMERGENCY,
        Aspect::AD_FRONTEND,
        "ADS-IMPL-112");
    }

    timer.stop();
    request_time_metering.history_post_match_time = timer.elapsed_time();
  }

  FrontendCommons::RequestTask
  AdFrontend::co_user_info_post_match_(
    adserver::user_info_svcs::user_info_manager::UpdateUserFreqCapsRequest
      request)
    noexcept
  {
    AdStageInProgressGuard history_post_match_in_progress(
      stats_.in(),
      AdFrontendStat::Stage::HistoryPostMatch);

    try
    {
      auto update_result = co_await user_info_client_coro_->
        update_user_freq_caps(std::move(request));
      if(!update_result.status.ok())
      {
        history_post_match_in_progress.add_error();
        Stream::Error ostr;
        ostr << "UserInfoManager::update_user_freq_caps(): "
          "gRPC call failed: code=" <<
          static_cast<int>(update_result.status.error_code()) <<
          ", message=" << update_result.status.error_message();
        logger()->log(
          ostr.str(),
          update_result.status.error_code() == grpc::StatusCode::UNAVAILABLE ?
            Logging::Logger::WARNING :
            Logging::Logger::EMERGENCY,
          Aspect::AD_FRONTEND,
          update_result.status.error_code() == grpc::StatusCode::UNAVAILABLE ?
            "" : "ADS-IMPL-112");
      }
    }
    catch(const eh::Exception& ex)
    {
      history_post_match_in_progress.add_error();
      Stream::Error ostr;
      ostr << "UserInfoManager::update_user_freq_caps(): "
        "caught eh::Exception: " << ex.what();
      logger()->log(
        ostr.str(),
        Logging::Logger::EMERGENCY,
        Aspect::AD_FRONTEND,
        "ADS-IMPL-112");
    }

    co_return FrontendCommons::RequestResult{};
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

  AdFrontend::TriggerMatcherTask
  AdFrontend::co_match_triggers_(
    const std::shared_ptr<RequestContext>& context,
    adserver::channel_svcs::channel_server::MatchRequest& channel_request)
    noexcept
  {
    const RequestInfo& request_info = context->request_info;
    AdStageInProgressGuard trigger_match_in_progress(
      stats_.in(),
      AdFrontendStat::Stage::TriggerMatch,
      false);
    TimeGuard trigger_match_time_metering;

    channel_request.set_first_url(request_info.referer);
    channel_request.set_first_url_words(request_info.referer_url_words);
    context->request_time_metering.recived_triggers =
      request_info.referer.empty() ? 0 : 1;

    if(request_info.full_text_words.empty())
    {
      channel_request.set_pwords(request_info.page_words);
      context->request_time_metering.recived_triggers +=
        request_info.page_words.size();
    }
    else
    {
      channel_request.set_pwords(request_info.full_text_words);
      context->request_time_metering.recived_triggers +=
        request_info.full_text_words.size();
    }
    channel_request.set_swords(request_info.search_words);
    channel_request.set_uid(GrpcAlgs::pack_user_id(request_info.client_id));

    auto channel_result = co_await channel_client_coro_->match(
      channel_request);
    if(!channel_result.status.ok())
    {
      trigger_match_in_progress.add_error();
      Stream::Error ostr;
      ostr << "ChannelServer::match(): gRPC call failed: code=" <<
        static_cast<int>(channel_result.status.error_code()) <<
        ", message=" << channel_result.status.error_message();
      const auto error = ostr.str().str();
      logger()->log(
        ostr.str(),
        Logging::Logger::EMERGENCY,
        Aspect::AD_FRONTEND,
        "ADS-IMPL-117");
      co_return TriggerMatcherResult{{}, false, error};
    }

    const auto& response = channel_result.response;
    const auto& matched_channels = response.matched_channels();
    context->request_time_metering.matched_triggers =
      matched_channels.page_channels_size() +
      matched_channels.search_channels_size() +
      matched_channels.url_channels_size() +
      matched_channels.url_keyword_channels_size() +
      matched_channels.uid_channels_size();
    context->request_time_metering.trigger_match_time =
      trigger_match_time_metering.consider();
    context->request_time_metering.detail_trigger_match_time.resize(
      response.match_time().empty() ? 0 : 1);
    if(!response.match_time().empty())
    {
      context->request_time_metering.detail_trigger_match_time[0] =
        GrpcAlgs::unpack_time(response.match_time());
    }

    co_return TriggerMatcherResult{
      std::move(channel_result.response_holder),
      true,
      {}};
  }

  AdFrontend::BoolTask
  AdFrontend::co_acquire_ad_(
    const std::shared_ptr<RequestContext>& context)
    noexcept
  {
    try
    {
      google::protobuf::Arena channel_request_arena;
      auto* channel_request = google::protobuf::Arena::CreateMessage<
        adserver::channel_svcs::channel_server::MatchRequest>(
          &channel_request_arena);
      *channel_request = get_empty_matching_request();
      AdServer::Grpc::ResponseHolder<
        adserver::channel_svcs::channel_server::MatchResponse>
          trigger_matched_channels;
      auto campaign_matching_result =
        std::make_shared<CM::RequestCreativeResult>();
      const bool make_merge =
        (!context->request_info.temp_client_id.is_null() ||
          !context->request_info.merge_persistent_client_id.is_null()) &&
        !context->request_info.client_id.is_null();

      if(context->request_info.keywords_normalized)
      {
        channel_request->set_simplify_page(false);
      }

      MergeUsersResult merge_result;

      if(make_merge)
      {
        merge_result = co_await co_merge_users_(context);
      }

      bool trigger_success = true;
      std::string trigger_error;
      if(context->request_info.passback_by_colocation)
      {
        trigger_matched_channels =
          AdServer::Grpc::ResponseHolder<
            adserver::channel_svcs::channel_server::MatchResponse>::
              make_value(get_empty_trigger_matching());
      }
      else
      {
        auto trigger_matcher = co_await co_match_triggers_(
          context,
          *channel_request);
        trigger_success = trigger_matcher.success;
        trigger_error = std::move(trigger_matcher.error_message);
        trigger_matched_channels = std::move(trigger_matcher.trigger_match_result);
      }

      auto user_info_matcher = co_await co_acquire_user_info_matcher_(
        context,
        trigger_matched_channels ? &*trigger_matched_channels : nullptr,
        trigger_success);
      auto history_match_result = user_info_matcher.history_match_result;
      if(!history_match_result)
      {
        auto empty_response = *get_empty_history_matching();
        history_match_result =
          AdServer::Grpc::ResponseHolder<
            adserver::user_info_svcs::user_info_manager::MatchResponse>::
              make_value(std::move(empty_response));
      }

      std::shared_ptr<
        adserver::channel_svcs::channel_server::GetCcgTraitsResponse>
          ccg_keywords;
      context->request_time_metering.profiling = true;

      context->debug_sink.print_acquire_ad(
        context->request_info,
        trigger_success ? &*trigger_matched_channels : nullptr,
        ccg_keywords.get(),
        history_match_result->match_result());
      if(!trigger_success)
      {
        context->debug_sink.print_trigger_matching_error(
          String::SubString(trigger_error));
      }

      std::string campaign_error;
      const bool campaign_success = co_await co_request_campaign_manager_(
        context->passback_info,
        context->request_info.log_as_test,
        *campaign_matching_result,
        campaign_error,
        context->request_time_metering,
        context->request_info,
        FrontendCommons::deduce_instantiate_type(
          &context->request_info.secure,
          context->request_holder->request()),
        trigger_success ? &*trigger_matched_channels : nullptr,
        &*history_match_result,
        merge_result.success ? merge_result.merged_last_request :
          GrpcAlgs::unpack_time(
            history_match_result->match_result().last_request_time()),
        user_info_matcher.profiling_available,
        ccg_keywords.get(),
        &context->debug_sink);
      if(!campaign_success)
      {
        context->debug_sink.print_creative_selection_error(
          String::SubString(campaign_error));
        co_return false;
      }

      if(context->request_info.user_status ==
        AdServer::CampaignSvcs::US_OPTIN &&
        campaign_matching_result->ad_slots_size() > 0)
      {
        user_info_post_match_(
          context->request_time_metering,
          context->request_info,
          *campaign_matching_result);
      }

      if(context->request_info.user_status ==
        AdServer::CampaignSvcs::US_OPTIN &&
        history_match_result->match_result().colo_id() != -1)
      {
        std::ostringstream current_colo_ostr;
        current_colo_ostr << history_match_result->match_result().colo_id();

        cookie_manager_->set(
          *context->response_ptr,
          context->request_holder->request(),
          Request::Cookie::LAST_COLOCATION_ID,
          current_colo_ostr.str());
      }

      if(!merge_result.success)
      {
        context->response_ptr->add_header_nocopy_name(
          Response::Header::MERGE_FAILED,
          merge_result.error_message);
      }

      context->debug_sink.print_creative_selection_debug_info(
        context->request_info,
        context->passback_info,
        *campaign_matching_result,
        context->request_time_metering);

      context->http_status = 204;
      if(campaign_matching_result->ad_slots_size() > 0 &&
        !campaign_matching_result->ad_slots(0).creative_body().empty())
      {
        const CM::AdSlotResult& ad_slot_result =
          campaign_matching_result->ad_slots(0);
        context->str_response = ad_slot_result.creative_body();

        if(!ad_slot_result.mime_format().empty())
        {
          context->response_ptr->set_content_type(
            ad_slot_result.mime_format());
        }
        else
        {
          context->response_ptr->set_content_type_nocopy(
            Response::Type::TEXT_HTML);
        }

        context->http_status = 200;
      }

      co_return true;
    }
    catch(const eh::Exception& ex)
    {
      logger()->sstream(
        Logging::Logger::EMERGENCY,
        Aspect::AD_FRONTEND,
        "ADS-IMPL-109") <<
        "AdFrontend::co_acquire_ad_(): caught eh::Exception: " <<
        ex.what();
      co_return false;
    }
  }

  void
  AdFrontend::convert_ccg_keywords_(
    google::protobuf::RepeatedPtrField<CM::CcgKeywordInfo>& ccg_keywords,
    const adserver::channel_svcs::channel_server::GetCcgTraitsResponse*
      src_ccg_keywords)
    noexcept
  {
    if(src_ccg_keywords)
    {
      for(const auto& src_ccg_kw : src_ccg_keywords->ccg_keywords())
      {
        CM::CcgKeywordInfo* res_ccg_kw = ccg_keywords.Add();
        res_ccg_kw->set_ccg_keyword_id(src_ccg_kw.ccg_keyword_id());
        res_ccg_kw->set_ccg_id(src_ccg_kw.ccg_id());
        res_ccg_kw->set_channel_id(src_ccg_kw.channel_id());
        res_ccg_kw->mutable_max_cpc()->set_value(src_ccg_kw.max_cpc());
        res_ccg_kw->mutable_ctr()->set_value(src_ccg_kw.ctr());
        res_ccg_kw->set_click_url(src_ccg_kw.click_url());
        res_ccg_kw->set_original_keyword(src_ccg_kw.original_keyword());
      }
    }
  }

  bool
  AdFrontend::resolve_cookie_user_id_(
    AdServer::Commons::UserId& resolved_user_id,
    const RequestInfo& request_info)
    noexcept
  {
    (void)resolved_user_id;
    (void)request_info;
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
    const adserver::user_info_svcs::user_info_manager::MatchResponse*
      history_match_response,
    const Generics::Time& /*merged_last_request*/,
    bool profiling_available,
    const adserver::channel_svcs::channel_server::GetCcgTraitsResponse*
      ccg_keywords,
    DebugSink* debug_sink,
    CM::RequestParams* request_params_out)
    /*throw(Exception)*/
  {
    static const char* FUN = "AdFrontend::request_campaign_manager_()";
    (void)passback_info;
    (void)log_as_test;
    (void)campaign_matching_result;
    (void)request_time_metering;

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
      const auto& history_match_result =
        history_match_response->match_result();

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
        trigger_match_result->mutable_uid_channels()->Add(
          matched_channels.uid_channels().begin(),
          matched_channels.uid_channels().end());
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

      if(history_match_response)
      {
        for(const auto& geo_data : history_match_result.geo_data_seq())
        {
          CM::GeoCoordInfo* res_loc = common_info->add_coord_location();
          res_loc->set_longitude(geo_data.longitude());
          res_loc->set_latitude(geo_data.latitude());
          res_loc->set_accuracy(geo_data.accuracy());
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
        history_match_result.create_time());
      common_info->set_full_referer(request_info.referer);
      common_info->set_referer(request_info.allowable_referer);
      context_info->set_full_referer_hash(request_info.full_referer_hash);
      context_info->set_short_referer_hash(request_info.short_referer_hash);
      common_info->set_cohort(request_info.curct);
      common_info->set_peer_ip(request_info.peer_ip);
      common_info->set_random(request_info.random);

      request_params.set_fraud(
        history_match_result.fraud_request() &&
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
      context_info->mutable_platform_ids()->Add(
        request_info.platform_ids.begin(),
        request_info.platform_ids.end());
      context_info->set_platform(request_info.platform);
      context_info->set_full_platform(request_info.full_platform);
      context_info->set_page_load_id(request_info.page_load_id);
      if(common_config_->ip_logging_enabled())
      {
        std::string ip_hash;
        FrontendCommons::ip_hash(ip_hash, request_info.peer_ip, common_config_->ip_salt());
        context_info->set_ip_hash(ip_hash);
      }

      request_params.mutable_full_freq_caps()->Add(
        history_match_result.full_freq_caps().begin(),
        history_match_result.full_freq_caps().end());

      for(const auto& history_seq_order : history_match_result.seq_orders())
      {
        CM::SeqOrderInfo* seq_order = request_params.add_seq_orders();
        seq_order->set_ccg_id(history_seq_order.ccg_id());
        seq_order->set_set_id(history_seq_order.set_id());
        seq_order->set_imps(history_seq_order.imps());
      }

      for(const auto& history_campaign_freq :
          history_match_result.campaign_freqs())
      {
        CM::CampaignFreq* campaign_freq = request_params.add_campaign_freqs();
        campaign_freq->set_campaign_id(
          history_campaign_freq.campaign_id());
        campaign_freq->set_imps(history_campaign_freq.imps());
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
      for (const auto& channel : history_match_result.channels())
      {
        request_params.add_channels(channel.channel_id());
      }
      if (trigger_matched_channels)
      {
        const auto& uid_channels =
          trigger_matched_channels->matched_channels().uid_channels();
        request_params.mutable_channels()->Add(
          uid_channels.begin(),
          uid_channels.end());
      }

      request_params.mutable_exclude_pubpixel_accounts()->Add(
        history_match_result.exclude_pubpixel_accounts().begin(),
        history_match_result.exclude_pubpixel_accounts().end());

      convert_ccg_keywords_(*request_params.mutable_ccg_keywords(), ccg_keywords);

      request_params.set_search_words(request_info.search_words);
      request_params.set_need_debug_info(debug_sink->require_debug_info());
      request_params.set_session_start(
        history_match_result.session_start());
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
          history_match_result.fraud_request() ||
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

      *request_params_out = request_params;
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

  AdFrontend::BoolTask
  AdFrontend::co_request_campaign_manager_(
    PassbackInfo& passback_info,
    bool& log_as_test,
    CM::RequestCreativeResult& campaign_matching_result,
    std::string& error,
    RequestTimeMetering& request_time_metering,
    const RequestInfo& request_info,
    const Generics::SubStringHashAdapter& instantiate_type,
    const adserver::channel_svcs::channel_server::MatchResponse*
      trigger_matched_channels,
    const adserver::user_info_svcs::user_info_manager::MatchResponse*
      history_match_result,
    const Generics::Time& merged_last_request,
    bool profiling_available,
    const adserver::channel_svcs::channel_server::GetCcgTraitsResponse*
      ccg_keywords,
    DebugSink* debug_sink)
    noexcept
  {
    AdStageInProgressGuard campaign_selection_in_progress(
      stats_.in(),
      AdFrontendStat::Stage::CampaignSelection);

    try
    {
      google::protobuf::Arena request_arena;
      auto* request = google::protobuf::Arena::CreateMessage<
        CM::GetCampaignCreativeRequest>(&request_arena);
      request_campaign_manager_(
        passback_info,
        log_as_test,
        campaign_matching_result,
        request_time_metering,
        request_info,
        instantiate_type,
        trigger_matched_channels,
        history_match_result,
        merged_last_request,
        profiling_available,
        ccg_keywords,
        debug_sink,
        request->mutable_request_params());

      TimeGuard creative_selection_time_metering;
      auto campaign_result = co_await campaign_manager_coro_->get_campaign_creative(
        *request);
      if(!campaign_result.status.ok())
      {
        campaign_selection_in_progress.add_error();
        Stream::Error ostr;
        ostr << "CampaignManager::get_campaign_creative(): "
          "gRPC call failed: code=" <<
          static_cast<int>(campaign_result.status.error_code()) <<
          ", message=" << campaign_result.status.error_message();
        error = ostr.str().str();
        logger()->log(
          ostr.str(),
          Logging::Logger::EMERGENCY,
          Aspect::AD_FRONTEND,
          "ADS-IMPL-118");
        co_return false;
      }

      campaign_matching_result = std::move(campaign_result.response.request_result());
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

      co_return true;
    }
    catch(const eh::Exception& ex)
    {
      campaign_selection_in_progress.add_error();
      Stream::Error ostr;
      ostr << "AdFrontend::co_request_campaign_manager_(): fail. "
        "Caught eh::Exception: " << ex.what();
      error = ostr.str().str();
      logger()->log(
        ostr.str(),
        Logging::Logger::EMERGENCY,
        Aspect::AD_FRONTEND,
        "ADS-IMPL-118");
      co_return false;
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

      msg->deliver();
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

      co_verify_opt_operation_(std::move(opt_operation_request)).
        start_detached(nullptr);
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

  FrontendCommons::RequestTask
  AdFrontend::co_verify_opt_operation_(
    CM::VerifyOptOperationRequest request)
    noexcept
  {
    auto opt_result = co_await campaign_manager_coro_->verify_opt_operation(
      std::move(request));
    if(!opt_result.status.ok())
    {
      Stream::Error ostr;
      ostr << "CampaignManager::verify_opt_operation(): "
        "gRPC call failed: code=" <<
        static_cast<int>(opt_result.status.error_code()) <<
        ", message=" << opt_result.status.error_message();
      logger()->log(
        ostr.str(),
        Logging::Logger::EMERGENCY,
        Aspect::AD_FRONTEND,
        "ADS-IMPL-116");
    }

    co_return FrontendCommons::RequestResult{};
  }

  void
  AdFrontend::update_colocation_flags()
    noexcept
  {
    co_update_colocation_flags_().start_detached(nullptr);
  }

  FrontendCommons::RequestTask
  AdFrontend::co_update_colocation_flags_()
    noexcept
  {
    CM::GetColocationFlagsRequest request;
    auto colocation_result =
      co_await campaign_manager_coro_->get_colocation_flags(
        std::move(request));
    if(!colocation_result.status.ok())
    {
      Stream::Error ostr;
      ostr << "CampaignManager::get_colocation_flags(): "
        "gRPC call failed: code=" <<
        static_cast<int>(colocation_result.status.error_code()) <<
        ", message=" << colocation_result.status.error_message();
      logger()->log(
        ostr.str(),
        Logging::Logger::CRITICAL,
        Aspect::AD_FRONTEND);
      co_return FrontendCommons::RequestResult{};
    }

    RequestInfoFiller::ColoFlagsMap_var new_colo_flags(
      new RequestInfoFiller::ColoFlagsMap());
    for(const auto& colocation : colocation_result.response.colocations())
    {
      RequestInfoFiller::ColoFlags colo_flags;
      colo_flags.flags = colocation.flags();
      new_colo_flags->insert(
        RequestInfoFiller::ColoFlagsMap::value_type(
          colocation.colo_id(),
          colo_flags));
    }

    request_info_filler_->colo_flags(new_colo_flags);
    co_return FrontendCommons::RequestResult{};
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

      match_params.mutable_persistent_channel_ids()->Add(
        request_info.platform_ids.begin(),
        request_info.platform_ids.end());
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
