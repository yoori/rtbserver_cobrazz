#include <sstream>
#include <algorithm>
#include <iostream>
#include <set>
#include <utility>
#include <vector>
#include <zlib.h>

#include <google/protobuf/arena.h>

#include <HTTP/HTTPCookie.hpp>
#include <String/StringManip.hpp>
#include <String/AsciiStringManip.hpp>

#include <Generics/Uuid.hpp>
#include "Generics/CompositeMetricsProvider.hpp"

#include <Commons/UserInfoManip.hpp>
#include <Commons/Algs.hpp>
#include <Commons/ConfigUtils.hpp>
#include <Commons/DelegateTaskGoal.hpp>
#include <Commons/ExternalUserIdUtils.hpp>
#include <Commons/Hostname.hpp>

#include <Commons/GrpcAlgs.hpp>
#include <Commons/Grpc/ResponseHolder.hpp>
#include <Frontends/FrontendCommons/HTTPUtils.hpp>
#include <LogCommons/AdRequestLogger.hpp>
#include <ChannelSvcs/ChannelCommons/ChannelUtils.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>

#include <Frontends/FrontendCommons/UserInfoClientConfig.hpp>

#include "GrpcClientMetricsProvider.hpp"

#include "BiddingFrontend.hpp"
#include "BidRequestState.hpp"

namespace Config
{
  const char ENABLE[] = "BiddingFrontend_Enable";
  const char CONFIG_FILES[] = "BiddingFrontend_Config";
  const char CONFIG_FILE[] = "BiddingFrontend_ConfigFile";
}

namespace Aspect
{
  extern const char BIDDING_FRONTEND[] = "BiddingFrontend";
}

namespace Response::Header
{
  const std::string CONTENT_TYPE("Content-Type");
}

namespace AdServer::Bidding
{
  namespace
  {
    namespace PB = adserver::campaign_svcs::campaign_manager;

    const CampaignSvcs::RevenueDecimal MAX_CPM_CONF_MULTIPLIER(false, 100, 0);

    unsigned long
    parse_process_coef_time(const std::string& value)
    {
      if (value.size() != 5 ||
        value[0] < '0' || value[0] > '2' ||
        value[1] < '0' || value[1] > '9' ||
        value[2] != ':' ||
        value[3] < '0' || value[3] > '5' ||
        value[4] < '0' || value[4] > '9')
      {
        Stream::Error ostr;
        ostr << "Invalid processCoef time: '" << value << "'";
        throw Frontend::Exception(ostr);
      }

      const unsigned long hours =
        static_cast<unsigned long>(value[0] - '0') * 10 +
        static_cast<unsigned long>(value[1] - '0');
      const unsigned long minutes =
        static_cast<unsigned long>(value[3] - '0') * 10 +
        static_cast<unsigned long>(value[4] - '0');

      if (hours > 23)
      {
        Stream::Error ostr;
        ostr << "Invalid processCoef time: '" << value << "'";
        throw Frontend::Exception(ostr);
      }

      return hours * 60 + minutes;
    }

    BiddingFrontendCore::ProcessCoefSchedule
    read_process_coef_config(const Frontend::BiddingFeConfiguration& config)
    {
      BiddingFrontendCore::ProcessCoefSchedule result;
      result.coef = config.process_coef();

      if (config.processCoef().present())
      {
        const auto& process_coef = *config.processCoef();
        result.coef = process_coef.coef();
        result.intervals.reserve(process_coef.interval().size());

        for (auto it = process_coef.interval().begin(); it != process_coef.interval().end(); ++it)
        {
          result.intervals.push_back(BiddingFrontendCore::ProcessCoefInterval{
            parse_process_coef_time(it->from()),
            parse_process_coef_time(it->to()),
            it->coef()});
        }
      }

      return result;
    }
  }

  class Frontend::UpdateConfigTask: public Generics::TaskGoal
  {
  public:
    UpdateConfigTask(Frontend* bid_frontend, Generics::TaskRunner* task_runner)
      /*throw(eh::Exception)*/
      : Generics::TaskGoal(task_runner),
        bid_frontend_(bid_frontend)
    {}

    virtual void
    execute() noexcept
    {
      bid_frontend_->update_config_();
    }

  private:
    Frontend* bid_frontend_;
  };

  class Frontend::FlushStateTask: public Generics::TaskGoal
  {
  public:
    FlushStateTask(Frontend* bid_frontend, Generics::TaskRunner* task_runner)
      /*throw(eh::Exception)*/
      : Generics::TaskGoal(task_runner),
        bid_frontend_(bid_frontend)
    {}

    virtual void
    execute() noexcept
    {
      bid_frontend_->flush_state_();
    }

  private:
    Frontend* bid_frontend_;
  };

  //
  // Frontend implementation
  //
  Frontend::Frontend(
    Configuration* frontend_config,
    Logging::Logger* logger,
    CommonModule* common_module,
    StatHolder* stats,
    Generics::CompositeMetricsProvider* composite_metrics_provider,
    std::shared_ptr<AdServer::Commons::ExecutorPool> request_workers,
    std::shared_ptr<AdServer::Commons::FastScheduler> timeout_scheduler,
    unsigned long service_index) /*throw(eh::Exception)*/
    : GroupLogger(
        Logging::Logger_var(
          new Logging::SeveritySelectorLogger(
            logger,
            0,
            frontend_config->get().BidFeConfiguration()->Logger().log_level())),
        "Bidding::Frontend",
        Aspect::BIDDING_FRONTEND,
        0),
      frontend_config_(ReferenceCounting::add_ref(frontend_config)),
      common_module_(ReferenceCounting::add_ref(common_module)),
      colo_id_(0),
      sources_(std::make_shared<RequestInfoFiller::SourceMap>()),
      account_traits_(std::make_shared<RequestInfoFiller::AccountTraitsById>()),
      campaign_manager_(),
      bid_workers_(std::move(request_workers)),
      timeout_scheduler_(std::move(timeout_scheduler)),
      stats_(ReferenceCounting::add_ref(stats)),
      composite_metrics_provider_(ReferenceCounting::add_ref(composite_metrics_provider))
  {
    if (!timeout_scheduler_)
    {
      timeout_scheduler_ = std::make_shared<AdServer::Commons::FastScheduler>(1);
    }

    const auto& hostname = AdServer::Commons::hostname();
    if (!hostname.empty())
    {
      server_id_ = hostname + "." + std::to_string(service_index);
    }
  }

  bool
  Frontend::will_handle(const String::SubString& uri) noexcept
  {
    std::string found_uri;
    bool result = false;

    if (!uri.empty())
    {
      result =
        FrontendCommons::find_uri(config_->GoogleUriList().Uri(), uri, found_uri) ||
        FrontendCommons::find_uri(config_->OpenRtbUriList().Uri(), uri, found_uri) ||
        (config_->AdfoxUriList().present() &&
         FrontendCommons::find_uri(config_->AdfoxUriList()->Uri(), uri, found_uri)) ||
        (config_->AdXmlUriList().present() &&
         FrontendCommons::find_uri(config_->AdXmlUriList()->Uri(), uri, found_uri)) ||
        (config_->ClickStarUriList().present() &&
         FrontendCommons::find_uri(config_->ClickStarUriList()->Uri(), uri, found_uri)) ||
        (config_->DAOUriList().present() &&
         FrontendCommons::find_uri(config_->DAOUriList()->Uri(), uri, found_uri))
        ;
    }

    if (logger()->log_level() >= TraceLevel::MIDDLE)
    {
      Stream::Error ostr;
      ostr << "Bidding::Frontend::will_handle(" << uri << "), service: '" << found_uri << "'";

      logger()->log(ostr.str());
    }

    return result;
  }

  void Frontend::parse_configs_() /*throw(Exception)*/
  {
    static const char* FUN = "Bidding::Frontend::parse_configs_()";

    /* load common configuration */

    try
    {
      typedef Configuration::FeConfig Config;
      const Config& fe_config = frontend_config_->get();

      if (!fe_config.CommonFeConfiguration().present())
      {
        throw Exception("CommonFeConfiguration isn't present");
      }

      common_config_.reset(new CommonFeConfiguration(*fe_config.CommonFeConfiguration()));

      colo_id_ = common_config_->colo_id();

      if (!fe_config.BidFeConfiguration().present())
      {
        throw Exception("BidFeConfiguration isn't present");
      }

      config_.reset(new BiddingFeConfiguration(*fe_config.BidFeConfiguration()));

      fill_account_traits_();
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << "': " << e.what();
      throw Exception(ostr);
    }
  }

  void
  Frontend::init() /*throw(eh::Exception)*/
  {
    static const char* FUN = "Bidding::Frontend::init()";

    if (true) // module_used()
    {
      try
      {
        parse_configs_();

        planner_ = new Generics::Planner(callback());
        add_child_object(planner_);

        control_task_runner_ = new Generics::TaskRunner(callback(), 4);
        add_child_object(control_task_runner_);

        Generics::Planner_var task_scheduler(new Generics::Planner(callback()));
        add_child_object(task_scheduler);

        // FlushLoggerTask
        Generics::Time flush_period(config_->flush_period().present() ? *config_->flush_period() : 10);
        Commons::make_goal_task(
          std::bind(
            &Commons::MessagePacker<CellsKey, MessageOut>::dump,
            group_logger(), Logging::Logger::ERROR, "", ""),
          control_task_runner_,
          task_scheduler,
          flush_period)->schedule(flush_period);
        bidding_frontend_logger_ = new BiddingFrontendLogger(
          callback(),
          logger(),
          config_->Logging().log_root(),
          Generics::Time(config_->Logging().Geo().period()),
          config_->Logging().Geo().shards());
        add_child_object(bidding_frontend_logger_.in());
        grpc_executor_ = common_module_->grpc_executor();

        auto user_info_client =
          AdServer::UserInfoSvcs::create_distributed_user_info_client(
            *common_config_,
            grpc_executor_,
            common_module_->grpc_coalesce_runner(),
            logger());
        user_info_distributed_client_ = user_info_client;
        user_info_client_ = user_info_client;
        user_info_client_coro_ = std::make_shared<
          AdServer::UserInfoSvcs::UserInfoManagerGrpcCoroClient>(user_info_client_, bid_workers_);
        add_child_object(user_info_client);

        auto campaign_manager_client =
          std::make_shared<AdServer::CampaignSvcs::CampaignManagerDistributedGrpcClient>(
            FrontendCommons::read_campaign_manager_grpc_refs(*common_config_),
            FrontendCommons::read_campaign_manager_grpc_batching_options(*common_config_),
            grpc_executor_,
            common_module_->grpc_coalesce_runner());
        campaign_manager_ = campaign_manager_client;
        campaign_manager_coro_ = std::make_shared<
          AdServer::CampaignSvcs::CampaignManagerGrpcCoroClient>(campaign_manager_, bid_workers_);
        add_child_object(campaign_manager_client);

        auto user_bind_client = AdServer::UserInfoSvcs::create_distributed_user_bind_client(
          *common_config_,
          grpc_executor_,
          common_module_->grpc_coalesce_runner(),
          logger());
        if (user_bind_client)
        {
          user_bind_client_ = user_bind_client;
          user_bind_client_coro_ = std::make_shared<
            AdServer::UserInfoSvcs::UserBindServerGrpcCoroClient>(user_bind_client_, bid_workers_);
          add_child_object(user_bind_client);
        }

        auto channel_client = AdServer::ChannelSvcs::create_distributed_channel_client(
          *common_config_,
          grpc_executor_,
          common_module_->grpc_coalesce_runner(),
          logger());
        channel_client_ = channel_client;
        channel_client_coro_ = std::make_shared<AdServer::ChannelSvcs::ChannelServerGrpcCoroClient>(
          channel_client_,
          bid_workers_);
        add_child_object(channel_client);

        {
          std::vector<GrpcClientMetricsProvider::ClientSource> client_sources;
          auto add_client_source = [&client_sources](const char* prefix, const auto& client) {
            if (client)
            {
              client_sources.push_back(GrpcClientMetricsProvider::ClientSource{
                prefix,
                std::static_pointer_cast<AdServer::Grpc::Client>(client)
              });
            }
          };

          add_client_source("user_bind_client", user_bind_client_);
          add_client_source("user_info_client", user_info_client_);
          add_client_source("channel_client", channel_client_);
          add_client_source("campaign_client", campaign_manager_);

          ReferenceCounting::SmartPtr<Generics::MetricsProvider> grpc_client_metrics_provider(
            new GrpcClientMetricsProvider(std::move(client_sources)));
          composite_metrics_provider_->add_provider(grpc_client_metrics_provider.in());
        }

        for (auto it = config_->Source().begin(); it != config_->Source().end(); ++it)
        {
          SourceTraits source_traits;
          if (it->default_account_id().present())
          {
            source_traits.default_account_id = *(it->default_account_id());
          }
          source_traits.instantiate_type = AdServer::CampaignSvcs::AIT_BODY;

          // banner notice : disable notice if notice_url defined
          source_traits.notice_instantiate_type = SourceTraits::NIT_NONE;
          std::string notice_instantiate_type = it->notice();
          if (notice_instantiate_type == "nurl")
          {
            source_traits.notice_instantiate_type = SourceTraits::NIT_NURL;
          }
          else if (notice_instantiate_type == "burl")
          {
            source_traits.notice_instantiate_type = SourceTraits::NIT_BURL;
          }
          else if (notice_instantiate_type == "nurl and burl")
          {
            source_traits.notice_instantiate_type = SourceTraits::NIT_NURL_AND_BURL;
          }

          source_traits.vast_instantiate_type = AdServer::CampaignSvcs::AIT_BODY;

          // vast notice
          source_traits.vast_notice_instantiate_type = SourceTraits::NIT_NONE;
          std::string vast_notice_instantiate_type = it->vast_notice();
          if (vast_notice_instantiate_type == "nurl")
          {
            source_traits.vast_notice_instantiate_type = SourceTraits::NIT_NURL;
          }
          else if (vast_notice_instantiate_type == "burl")
          {
            source_traits.vast_notice_instantiate_type = SourceTraits::NIT_BURL;
          }

          // native notice
          source_traits.native_notice_instantiate_type = SourceTraits::NIT_NONE;
          std::string native_notice_instantiate_type = it->native_notice();
          if (native_notice_instantiate_type == "nurl")
          {
            source_traits.native_notice_instantiate_type = SourceTraits::NIT_NURL;
          }
          else if (native_notice_instantiate_type == "burl")
          {
            source_traits.native_notice_instantiate_type = SourceTraits::NIT_BURL;
          }
          else if (native_notice_instantiate_type == "nurl and burl")
          {
            source_traits.native_notice_instantiate_type = SourceTraits::NIT_NURL_AND_BURL;
          }

          source_traits.ipw_extension = it->ipw_extension();
          source_traits.truncate_domain = it->truncate_domain();
          source_traits.fill_adid = it->fill_adid();
          if (it->seat().present())
          {
            source_traits.seat = *(it->seat());
          }

          if (it->request_type().present())
          {
            std::string type = *(it->request_type());
            if (type == "openrtb")
            {
              source_traits.request_type = AdServer::CampaignSvcs::AR_OPENRTB;
            }
            else if (type == "openrtb with click url")
            {
              source_traits.request_type = AdServer::CampaignSvcs::AR_OPENRTB_WITH_CLICKURL;
            }
            else if (type == "openx")
            {
              source_traits.request_type = AdServer::CampaignSvcs::AR_OPENX;
            }
            else if (type == "liverail")
            {
              source_traits.request_type = AdServer::CampaignSvcs::AR_LIVERAIL;
            }
            else if (type == "adriver")
            {
              source_traits.request_type = AdServer::CampaignSvcs::AR_ADRIVER;
            }
            else if (type == "yandex")
            {
              source_traits.request_type = AdServer::CampaignSvcs::AR_YANDEX;
            }
          }

          source_traits.instantiate_type = adapt_instantiate_type_(it->instantiate_type());
          source_traits.vast_instantiate_type = adapt_instantiate_type_(
            it->vast_instantiate_type());
          source_traits.native_ads_instantiate_type = adapt_native_ads_instantiate_type_(
            it->native_instantiate_type());
          if (it->native_impression_tracker_type().present())
          {
            source_traits.native_ads_impression_tracker_type =
              adapt_native_ads_impression_tracker_type_(*it->native_impression_tracker_type());
          }

          source_traits.erid_return_type = adapt_erid_return_type_(it->erid_return_type());

          if (it->max_bid_time().present())
          {
            source_traits.max_bid_time = Generics::Time(*(it->max_bid_time()));
            (*source_traits.max_bid_time) /= 1000;
          }

          source_traits.skip_ext_category = it->skip_ext_category();
          if (it->notice_url().present())
          {
            source_traits.notice_url = *(it->notice_url());
          }

          sources_->insert(std::make_pair(it->id(), source_traits));
        }

        RequestInfoFiller::ExternalUserIdSet skip_external_ids;

        if (common_config_->SkipExternalIds().present())
        {
          for (auto it = common_config_->SkipExternalIds()->Id().begin();
            it != common_config_->SkipExternalIds()->Id().end(); ++it)
          {
            skip_external_ids.insert(it->value());
          }

          String::SubString skip_ids = common_config_->SkipExternalIds()->skip_external_ids();

          if (!skip_ids.empty())
          {
            String::StringManip::SplitNL tokenizer(skip_ids);
            for (String::SubString skip_id; tokenizer.get_token(skip_id);)
            {
              skip_external_ids.insert(skip_id.str());
            }
          }
        }

        request_info_filler_ = std::make_shared<RequestInfoFiller>(
          logger(),
          common_config_->colo_id(),
          common_module_.in(),
          common_module_->ip_mapper(),
          "", //user_agent_filter_path.c_str()
          skip_external_ids,
          common_config_->ip_logging_enabled(),
          common_config_->ip_salt().c_str(),
          *sources_,
          config_->enable_profile_referer(),
          *account_traits_);

        Generics::Time request_timeout;
        if (config_->request_timeout().present())
        {
          request_timeout = Generics::Time(*(config_->request_timeout()));
          request_timeout /= 1000;
        }

        auto fill_uri_list = [](auto& dst, const auto& src) {
          dst.reserve(src.size());
          for (auto it = src.begin(); it != src.end(); ++it)
          {
            dst.emplace_back(it->path());
          }
        };

        BiddingFrontendCore::InitParams core_params;
        core_params.logger = Logging::Logger_var(ReferenceCounting::add_ref(logger()));
        core_params.common_module = common_module_;
        core_params.user_id_controller = common_module_->user_id_controller();
        core_params.request_info_filler = request_info_filler_;
        core_params.sources = sources_;
        core_params.account_traits = account_traits_;
        core_params.stats = stats_;
        core_params.bid_workers = bid_workers_;
        core_params.timeout_scheduler = timeout_scheduler_;
        core_params.bidding_frontend_logger = bidding_frontend_logger_;
        core_params.user_bind_client = user_bind_client_;
        core_params.user_info_distributed_client = user_info_distributed_client_;
        core_params.user_info_client = user_info_client_;
        core_params.campaign_manager = campaign_manager_;
        core_params.channel_client = channel_client_;
        fill_uri_list(core_params.google_uris, config_->GoogleUriList().Uri());
        if (config_->AdfoxUriList().present())
        {
          fill_uri_list(core_params.adfox_uris, config_->AdfoxUriList()->Uri());
        }

        if (config_->AdXmlUriList().present())
        {
          fill_uri_list(core_params.adxml_uris, config_->AdXmlUriList()->Uri());
        }

        if (config_->ClickStarUriList().present())
        {
          fill_uri_list(core_params.clickstar_uris, config_->ClickStarUriList()->Uri());
        }

        if (config_->DAOUriList().present())
        {
          fill_uri_list(core_params.dao_uris, config_->DAOUriList()->Uri());
        }

        core_params.max_pending_tasks = config_->max_pending_tasks();
        core_params.process_coef = read_process_coef_config(*config_);
        core_params.threads = config_->threads();
        core_params.request_timeout = request_timeout;
        core_params.server_id = server_id_;
        core_params.colo_id = colo_id_;
        core_params.trace_mapping = config_->trace_mapping();
        core_params.enable_profile_referer = config_->enable_profile_referer();
        core_ = std::make_unique<BiddingFrontendCore>(core_params);

        control_task_runner_->enqueue_task(
          Generics::Task_var(new UpdateConfigTask(this, control_task_runner_)));

        control_task_runner_->enqueue_task(
          Generics::Task_var(new FlushStateTask(this, control_task_runner_)));
      }
      catch (const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << ex.what();
        throw Exception(ostr);
      }

      logger()->log(
        String::SubString("Bidding::Frontend::init(): frontend is running ..."),
        Logging::Logger::INFO,
        Aspect::BIDDING_FRONTEND);
    }
  }

  void
  Frontend::shutdown() noexcept
  {
    try
    {
      deactivate_object();
      wait_object();
      clear();

      Stream::Error ostr;
      ostr << "Bidding::Frontend::shutdown(): frontend terminated (pid = " << ::getpid() << ").";

      logger()->log(ostr.str(), Logging::Logger::INFO, Aspect::BIDDING_FRONTEND);
    }
    catch(...)
    {}
  }

  void
  Frontend::handle_request(
    FCGI::HttpRequestHolder_var request_holder,
    FCGI::BaseHttpResponseWriter_var response_writer)
    noexcept
  {
    core_->handle_request(std::move(request_holder), std::move(response_writer));
  }

  void
  Frontend::update_config_() noexcept
  {
    static const char* FUN = "Frontend::update_config_()";

    co_update_config_().start_detached(nullptr);

    try
    {
      planner_->schedule(
        Generics::Goal_var(new UpdateConfigTask(this, control_task_runner_)),
        Generics::Time::get_time_of_day() + common_config_->update_period());
    }
    catch (const eh::Exception& ex)
    {
      logger()->sstream(Logging::Logger::EMERGENCY, Aspect::BIDDING_FRONTEND, "ADS-IMPL-7605") <<
        FUN << ": schedule failed: " << ex.what();
    }
  }

  void
  Frontend::flush_state_() noexcept
  {
    static const char* FUN = "Frontend::flush_state_()";

    const unsigned long reached_max_pending_tasks = core_->reset_reached_max_pending_tasks();

    if (reached_max_pending_tasks > 0)
    {
      Stream::Error ostr;
      ostr << FUN << ": reached max pending tasks: " << reached_max_pending_tasks;

      logger()->log(ostr.str(),
        Logging::Logger::WARNING,
        Aspect::BIDDING_FRONTEND,
        "ADS-IMPL-7602");
    }

    try
    {
      planner_->schedule(
        Generics::Goal_var(new FlushStateTask(this, control_task_runner_)),
        Generics::Time::get_time_of_day() + (
          config_->flush_period().present() ? *config_->flush_period() : 10));
    }
    catch (const eh::Exception& ex)
    {
      logger()->sstream(Logging::Logger::EMERGENCY, Aspect::BIDDING_FRONTEND, "ADS-IMPL-7605") <<
        FUN << ": schedule failed: " << ex.what();
    }
  }

  void
  Frontend::fill_account_traits_() noexcept
  {
    for (auto account_it = config_->Account().begin();
      account_it != config_->Account().end(); ++account_it)
    {
      RequestInfoFiller::AccountTraits_var& target_account_ptr =
        (*account_traits_)[account_it->account_id()];

      if (!target_account_ptr.in())
      {
        target_account_ptr = new RequestInfoFiller::AccountTraits();
      }

      RequestInfoFiller::AccountTraits& target_account = *target_account_ptr;

      if (account_it->max_cpm_value().present())
      {
        CampaignSvcs::RevenueDecimal limit = CampaignSvcs::RevenueDecimal::mul(
          AdServer::Commons::extract_decimal<CampaignSvcs::RevenueDecimal>(
            account_it->max_cpm_value().get()),
          MAX_CPM_CONF_MULTIPLIER,
          Generics::DMR_FLOOR);

        target_account.max_cpm = limit;
      }

      if (account_it->display_billing_id().present())
      {
        target_account.display_billing_id = *(account_it->display_billing_id());
      }

      if (account_it->video_billing_id().present())
      {
        target_account.video_billing_id = *(account_it->video_billing_id());
      }

      if (account_it->google_encryption_key().present())
      {
        target_account.google_encryption_key_size = String::StringManip::hex_decode(
          *(account_it->google_encryption_key()),
          target_account.google_encryption_key);
      }

      if (account_it->google_integrity_key().present())
      {
        target_account.google_integrity_key_size = String::StringManip::hex_decode(
          *(account_it->google_integrity_key()),
          target_account.google_integrity_key);
      }
    }
  }

  FrontendCommons::RequestTask
  Frontend::co_update_config_() noexcept
  {
    static const char* FUN = "Frontend::co_update_config_()";

    try
    {
      auto result = co_await campaign_manager_coro_->co_get_colocation_flags(
        PB::GetColocationFlagsRequest());
      if (!result.status.ok())
      {
        logger()->sstream(
          Logging::Logger::CRITICAL,
          Aspect::BIDDING_FRONTEND,
          "ADS-IMPL-118") << FUN <<
          ": CampaignManager::get_colocation_flags() failed: code=" <<
          static_cast<int>(result.status.error_code()) <<
          ", message=" << result.status.error_message();
        co_return FrontendCommons::RequestResult::written();
      }

      ExtConfig_var new_config(new ExtConfig());

      for (const auto& colocation_info : result.response.colocations())
      {
        ExtConfig::Colocation colocation;
        colocation.flags = colocation_info.flags();
        new_config->colocations.emplace(colocation_info.colo_id(), colocation);
      }

      set_ext_config_(new_config);
    }
    catch(const eh::Exception& ex)
    {
      logger()->sstream(
        Logging::Logger::CRITICAL,
        Aspect::BIDDING_FRONTEND,
        "ADS-IMPL-118") << FUN << ": CampaignManager::get_colocation_flags() failed: " << ex.what();
    }

    co_return FrontendCommons::RequestResult::written();
  }

  AdServer::CampaignSvcs::AdInstantiateType
  Frontend::adapt_instantiate_type_(const std::string& inst_type_str)
    /*throw(Exception)*/
  {
    if (inst_type_str == "url")
    {
      return AdServer::CampaignSvcs::AIT_URL;
    }
    else if (inst_type_str == "nonsecure url")
    {
      return AdServer::CampaignSvcs::AIT_NONSECURE_URL;
    }
    else if (inst_type_str == "url in body")
    {
      return AdServer::CampaignSvcs::AIT_URL_IN_BODY;
    }
    else if (inst_type_str == "video url")
    {
      return AdServer::CampaignSvcs::AIT_VIDEO_URL;
    }
    else if (inst_type_str == "nonsecure video url")
    {
      return AdServer::CampaignSvcs::AIT_VIDEO_NONSECURE_URL;
    }
    else if (inst_type_str == "video url in body")
    {
      return AdServer::CampaignSvcs::AIT_VIDEO_URL_IN_BODY;
    }
    else if (inst_type_str == "body")
    {
      return AdServer::CampaignSvcs::AIT_BODY;
    }
    else if (inst_type_str == "script with url")
    {
      return AdServer::CampaignSvcs::AIT_SCRIPT_WITH_URL;
    }
    else if (inst_type_str == "iframe with url")
    {
      return AdServer::CampaignSvcs::AIT_IFRAME_WITH_URL;
    }
    else if (inst_type_str == "url parameters")
    {
      return AdServer::CampaignSvcs::AIT_URL_PARAMS;
    }
    else if (inst_type_str == "encoded url parameters")
    {
      return AdServer::CampaignSvcs::AIT_DATA_URL_PARAM;
    }
    else if (inst_type_str == "data parameter value")
    {
      return AdServer::CampaignSvcs::AIT_DATA_PARAM_VALUE;
    }

    Stream::Error ostr;
    ostr << "unknown instantiate type '" << inst_type_str << "'";
    throw Exception(ostr);
  }

  SourceTraits::NativeAdsInstantiateType
  Frontend::adapt_native_ads_instantiate_type_(const std::string& inst_type_str)
    /*throw(Exception)*/
  {
    if (inst_type_str == "none")
    {
      return SourceTraits::NAIT_NONE;
    }
    else if (inst_type_str == "adm")
    {
      return SourceTraits::NAIT_ADM;
    }
    else if (inst_type_str == "adm_native")
    {
      return SourceTraits::NAIT_ADM_NATIVE;
    }
    else if (inst_type_str == "ext")
    {
      return SourceTraits::NAIT_EXT;
    }
    else if (inst_type_str == "escape_slash_adm")
    {
      return SourceTraits::NAIT_ESCAPE_SLASH_ADM;
    }
    else if (inst_type_str == "native_as_element-1.2")
    {
      return SourceTraits::NAIT_NATIVE_AS_ELEMENT_1_2;
    }
    else if (inst_type_str == "adm-1.2")
    {
      return SourceTraits::NAIT_ADM_1_2;
    }
    else if (inst_type_str == "adm_native-1.2")
    {
      return SourceTraits::NAIT_ADM_NATIVE_1_2;
    }

    Stream::Error ostr;
    ostr << "unknown native ads instantiate type '" << inst_type_str << "'";
    throw Exception(ostr);
  }

  SourceTraits::ERIDReturnType
  Frontend::adapt_erid_return_type_(const std::string& erid_type_str)
  {
    if (erid_type_str == "single")
    {
      return SourceTraits::ERIDRT_SINGLE;
    }
    else if (erid_type_str == "array")
    {
      return SourceTraits::ERIDRT_ARRAY;
    }
    else if (erid_type_str == "ext0")
    {
      return SourceTraits::ERIDRT_EXT0;
    }
    else if (erid_type_str == "buzsape")
    {
      return SourceTraits::ERIDRT_EXT_BUZSAPE;
    }

    Stream::Error ostr;
    ostr << "unknown erid return type '" << erid_type_str << "'";
    throw Exception(ostr);
  }

  AdServer::CampaignSvcs::NativeAdsImpressionTrackerType
  Frontend::adapt_native_ads_impression_tracker_type_(const std::string& imp_type_str)
    /*throw(Exception)*/
  {
    if (imp_type_str == "imp")
    {
      return AdServer::CampaignSvcs::NAITT_IMP;
    }

    if (imp_type_str == "js")
    {
      return AdServer::CampaignSvcs::NAITT_JS;
    }

    if (imp_type_str == "resources")
    {
      return AdServer::CampaignSvcs::NAITT_RESOURCES;
    }

    Stream::Error ostr;
    ostr << "unknown native ads impression tracker type '" << imp_type_str << "'";
    throw Exception(ostr);
  }
}
