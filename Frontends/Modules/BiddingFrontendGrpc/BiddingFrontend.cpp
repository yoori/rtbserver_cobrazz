// STD
#include <algorithm>
#include <set>
#include <sstream>
#include <zlib.h>

// UNIXCOMMONS
#include <Generics/Uuid.hpp>
#include <Generics/HashTableAdapters.hpp>
#include <Generics/TaskPool.hpp>
#include <HTTP/HTTPCookie.hpp>
#include <String/StringManip.hpp>
#include <String/AsciiStringManip.hpp>

// THIS
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>
#include <Commons/Algs.hpp>
#include <Commons/DelegateTaskGoal.hpp>
#include <Commons/ExternalUserIdUtils.hpp>
#include <Commons/GrpcAlgs.hpp>
#include <Frontends/FrontendCommons/HTTPUtils.hpp>
#include <Frontends/FrontendCommons/Statistics.hpp>
#include <Frontends/Modules/BiddingFrontendGrpc/AdJsonBidRequestTask.hpp>
#include <Frontends/Modules/BiddingFrontendGrpc/AdXmlBidRequestTask.hpp>
#include <Frontends/Modules/BiddingFrontendGrpc/AppNexusBidRequestTask.hpp>
#include <Frontends/Modules/BiddingFrontendGrpc/BiddingFrontend.hpp>
#include <Frontends/Modules/BiddingFrontendGrpc/BidRequestTask.hpp>
#include <Frontends/Modules/BiddingFrontendGrpc/ClickStarBidRequestTask.hpp>
#include <Frontends/Modules/BiddingFrontendGrpc/GoogleBidRequestTask.hpp>
#include <Frontends/Modules/BiddingFrontendGrpc/OpenRtbBidRequestTask.hpp>
#include <LogCommons/AdRequestLogger.hpp>

namespace Config
{
  inline constexpr char ENABLE[] = "BiddingFrontend_Enable";
  inline constexpr char CONFIG_FILES[] = "BiddingFrontend_Config";
  inline constexpr char CONFIG_FILE[] = "BiddingFrontend_ConfigFile";
}

namespace Aspect
{
  inline constexpr char BIDDING_FRONTEND[] = "BiddingFrontend";
}

namespace Response::Header
{
  const std::string CONTENT_TYPE = "Content-Type";
}

namespace AdServer::Bidding::Grpc
{
  namespace
  {
    const CampaignSvcs::RevenueDecimal MAX_CPM_CONF_MULTIPLIER(false, 100, 0);

    struct ChannelMatch final
    {
      ChannelMatch(
        const unsigned long channel_id_val,
        const unsigned long channel_trigger_id_val)
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

      const unsigned long channel_id;
      const unsigned long channel_trigger_id;
    };

    Commons::Interval<Generics::Time>
    construct_time_interval(const String::SubString& str)
    {
      const std::size_t pos = str.find('-');
      if (pos == String::SubString::NPOS)
      {
        Stream::Error ostr;
        ostr << "Separator not found in '" << str << "'";
        throw Frontend::Exception(ostr);
      }

      const Generics::Time min_val = Generics::Time(str.substr(0, pos), "%H:%M");
      const Generics::Time max_val = Generics::Time(str.substr(pos + 1), "%H:%M");
      return Commons::Interval<Generics::Time>(min_val, max_val);
    }
  }

  Frontend::Frontend(
    TaskProcessor& helper_task_processor,
    const GrpcContainerPtr& grpc_container,
    Configuration* frontend_config,
    Logging::Logger* logger,
    CommonModule* common_module,
    StatHolder* stats,
    FrontendCommons::HttpResponseFactory* response_factory) /*throw(eh::Exception)*/
    : GroupLogger(
        Logging::Logger_var(
          new Logging::SeveritySelectorLogger(
            logger,
            0,
            frontend_config->get().BidFeConfiguration()->Logger().log_level())),
        "Bidding::Frontend",
        Aspect::BIDDING_FRONTEND,
        0),
      FrontendCommons::FrontendInterface(response_factory),
      helper_task_processor_(helper_task_processor),
      grpc_container_(grpc_container),
      frontend_config_(ReferenceCounting::add_ref(frontend_config)),
      common_module_(ReferenceCounting::add_ref(common_module)),
      colo_id_(0),
      stats_(ReferenceCounting::add_ref(stats))
  {
    if (!grpc_container_->grpc_campaign_manager_pool)
    {
      Stream::Error stream;
      stream << FNS
             << "grpc_campaign_manager_pool is null";
      throw Exception(stream);
    }

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

  void Frontend::set_ext_config_(ExtConfig* config) noexcept
  {
    ExtConfig_var new_config = ReferenceCounting::add_ref(config);

    std::unique_lock lock(ext_config_lock_);
    ext_config_.swap(new_config);
  }

  Frontend::ExtConfig_var Frontend::get_ext_config_() noexcept
  {
    std::shared_lock lock(ext_config_lock_);
    return ext_config_;
  }

  RequestInfoFiller* Frontend::request_info_filler() noexcept
  {
    return request_info_filler_.get();
  }

  Logging::Logger* Frontend::logger() noexcept
  {
    return GroupLogger::logger();
  }

  bool Frontend::will_handle(const String::SubString& uri) noexcept
  {
    std::string found_uri;
    bool result = false;

    if (!uri.empty())
    {
      result =
        FrontendCommons::find_uri(
          config_->GoogleUriList().Uri(), uri, found_uri) ||
        FrontendCommons::find_uri(
          config_->OpenRtbUriList().Uri(), uri, found_uri) ||
        FrontendCommons::find_uri(
          config_->AppNexusUriList().Uri(), uri, found_uri) ||
        (config_->AdXmlUriList().present() &&
         FrontendCommons::find_uri(
           config_->AdXmlUriList()->Uri(), uri, found_uri)) ||
        (config_->ClickStarUriList().present() &&
         FrontendCommons::find_uri(
           config_->ClickStarUriList()->Uri(), uri, found_uri)) ||
        (config_->DAOUriList().present() &&
         FrontendCommons::find_uri(
           config_->DAOUriList()->Uri(), uri, found_uri));
    }

    if (logger()->log_level() >= static_cast<unsigned long>(TraceLevel::MIDDLE))
    {
      Stream::Error ostr;
      ostr << "Bidding::Frontend::will_handle(" << uri <<
        "), service: '" << found_uri << "'";
      logger()->log(ostr.str());
    }

    return result;
  }

  void Frontend::parse_configs_() /*throw(Exception)*/
  {
    try
    {
      using Config = Configuration::FeConfig;

      const Config& fe_config = frontend_config_->get();
      if (!fe_config.CommonFeConfiguration().present())
      {
        throw Exception("CommonFeConfiguration isn't present");
      }

      common_config_.reset(
        new CommonFeConfiguration(*fe_config.CommonFeConfiguration()));

      colo_id_ = common_config_->colo_id();

      if (!fe_config.BidFeConfiguration().present())
      {
        throw Exception("BidFeConfiguration isn't present");
      }

      config_.reset(
        new BiddingFeConfiguration(*fe_config.BidFeConfiguration()));

      fill_account_traits_();
    }
    catch (const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FNS
           << e.what();
      throw Exception(ostr);
    }
  }

  void Frontend::init()
  {
    try
    {
      parse_configs_();

      Generics::Time flush_period(
        config_->flush_period().present() ? *config_->flush_period() : 10);
      background_task_storage_.Detach(userver::engine::CriticalAsyncNoSpan(
        helper_task_processor_,
        [this, flush_period = flush_period] () {
          while (!userver::engine::current_task::ShouldCancel())
          {
            group_logger()->dump(Logging::Logger::ERROR, "", "");
            userver::engine::InterruptibleSleepFor(
              std::chrono::microseconds(flush_period.microseconds()));
          }
        }));

      for (auto it = config_->Source().begin();
           it != config_->Source().end();
           ++it)
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

        if (it->appnexus_member_id().present())
        {
          source_traits.appnexus_member_id = *(it->appnexus_member_id());
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

        source_traits.instantiate_type = adapt_instantiate_type_(
          it->instantiate_type());
        source_traits.vast_instantiate_type = adapt_instantiate_type_(
          it->vast_instantiate_type());
        source_traits.native_ads_instantiate_type = adapt_native_ads_instantiate_type_(
          it->native_instantiate_type());
        if (it->native_impression_tracker_type().present())
        {
          source_traits.native_ads_impression_tracker_type =
            adapt_native_ads_impression_tracker_type_(
              *it->native_impression_tracker_type());
        }

        source_traits.erid_return_type = adapt_erid_return_type_(
          it->erid_return_type());

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

        sources_.insert(std::make_pair(it->id(), source_traits));
      }

      {
        const String::SubString intervalsBlacklist = config_->intervalsBlacklist();
        String::StringManip::SplitNL tokenizer(intervalsBlacklist);

        for (String::SubString interval; tokenizer.get_token(interval);)
        {
          blacklisted_time_intervals_.insert(construct_time_interval(interval.str()));
        }
      }

      RequestInfoFiller::ExternalUserIdSet skip_external_ids;

      if (common_config_->SkipExternalIds().present())
      {
        for (auto it = common_config_->SkipExternalIds()->Id().begin();
             it != common_config_->SkipExternalIds()->Id().end();
             ++it)
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

      request_info_filler_.reset(
        new RequestInfoFiller(
          logger(),
          common_config_->colo_id(),
          common_module_.in(),
          common_config_->GeoIP().present() ?
            common_config_->GeoIP()->path().c_str() : 0,
          "", //user_agent_filter_path.c_str()
          skip_external_ids,
          common_config_->ip_logging_enabled(),
          common_config_->ip_salt().c_str(),
          sources_,
          config_->enable_profile_referer(),
          account_traits_));

      if (config_->request_timeout().present())
      {
        request_timeout_ = Generics::Time(*(config_->request_timeout()));
        request_timeout_ /= 1000;
      }

      google::protobuf::SetLogHandler(&Frontend::protobuf_log_handler_);

      background_task_storage_.Detach(userver::engine::CriticalAsyncNoSpan(
        helper_task_processor_,
        [this] () {
          update_config_();
        }));

      activate_object();
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FNS
           << ex.what();
      throw Exception(ostr);
    }

    logger()->log(String::SubString(
      "Bidding::Frontend::init(): frontend is running ..."),
      Logging::Logger::INFO,
      Aspect::BIDDING_FRONTEND);
  }

  void Frontend::shutdown() noexcept
  {
    try
    {
      background_task_storage_.CancelAndWait();
      deactivate_object();
      wait_object();
      clear();

      Stream::Error ostr;
      ostr << "Bidding::Frontend::shutdown(): frontend terminated (pid = "
           << ::getpid() << ").";

      logger()->log(ostr.str(),
        Logging::Logger::INFO,
        Aspect::BIDDING_FRONTEND);

      common_module_->shutdown();
    }
    catch (...)
    {
    }
  }

  Generics::Time
  Frontend::get_request_timeout_(const FrontendCommons::HttpRequest& request) noexcept
  {
    const HTTP::ParamList& params = request.params();
    for (auto it = params.begin(); it != params.end(); ++it)
    {
      if (it->name == Request::Context::Grpc::SOURCE_ID)
      {
        const auto source_it = sources_.find(it->value);
        if (source_it != sources_.end() && source_it->second.max_bid_time)
        {
          return *(source_it->second.max_bid_time);
        }
      }
    }

    return request_timeout_;
  }

  void Frontend::handle_request(
    FrontendCommons::HttpRequestHolder_var request_holder,
    FrontendCommons::HttpResponseWriter_var response_writer) noexcept
  {
    DO_TIME_STATISTIC_FRONTEND(FrontendCommons::TimeStatisticId::BiddingFrontend_InputRequest);

    const Generics::Time start_process_time = Generics::Time::get_time_of_day();
    BidRequestTask_var request_task;

    try
    {
      const FrontendCommons::HttpRequest& request = request_holder->request();

      std::string found_uri;
      if (FrontendCommons::find_uri(
        config_->GoogleUriList().Uri(), request.uri(), found_uri))
      {
        request_task = new GoogleBidRequestTask(
          this,
          request_holder,
          response_writer,
          start_process_time);
      }
      else if (FrontendCommons::find_uri(
        config_->AppNexusUriList().Uri(), request.uri(), found_uri))
      {
        request_task = new AppNexusBidRequestTask(
          this,
          request_holder,
          response_writer,
          start_process_time);
      }
      else if (config_->AdXmlUriList().present() &&
        FrontendCommons::find_uri(
          config_->AdXmlUriList()->Uri(), request.uri(), found_uri))
      {
        request_task = new AdXmlBidRequestTask(
          this,
          request_holder,
          response_writer,
          start_process_time);
      }
      else if (config_->ClickStarUriList().present() &&
        FrontendCommons::find_uri(
          config_->ClickStarUriList()->Uri(), request.uri(), found_uri))
      {
        request_task = new ClickStarBidRequestTask(
          this,
          request_holder,
          response_writer,
          start_process_time);
      }
      else if (config_->DAOUriList().present() &&
        FrontendCommons::find_uri(
          config_->DAOUriList()->Uri(), request.uri(), found_uri))
      {
        request_task = new AdJsonBidRequestTask(
          this,
          request_holder,
          response_writer,
          start_process_time);
      }
      else
      {
        request_task = new OpenRtbBidRequestTask(
          this,
          request_holder,
          response_writer,
          start_process_time);
      }

      const auto timeout = get_request_timeout_(request).microseconds();
      auto& task_processor = userver::engine::current_task::GetTaskProcessor();
      auto task_timeout = userver::engine::AsyncNoSpan(
        task_processor,
        [&request_task, &timeout] () {
          userver::engine::InterruptibleSleepFor(std::chrono::microseconds(timeout));
          if (!userver::engine::current_task::IsCancelRequested())
          {
            request_task->interrupt();
          }
        });

      request_task->execute();
      task_timeout.SyncCancel();
    }
    catch (const BidRequestTask::Invalid& exc)
    {
      if (request_task)
      {
        request_task->write_empty_response(400);
      }
      else
      {
        response_writer->write(
          400,
          create_response());
      }

      Stream::Error ostr;
      ostr << FNS
           << "BidRequestTask::Invalid caught: "
           << exc.what();
      logger()->log(ostr.str(),
        Logging::Logger::INFO,
        Aspect::BIDDING_FRONTEND);
    }
    catch (const eh::Exception& exc)
    {
      if (request_task)
      {
        request_task->write_empty_response(503);
      }
      else
      {
        response_writer->write(
          503,
          create_response());
      }

      Stream::Error ostr;
      ostr << FNS
           << "eh::Exception caught: "
           << exc.what();

      logger()->log(ostr.str(),
        Logging::Logger::EMERGENCY,
        Aspect::BIDDING_FRONTEND,
        "ADS-IMPL-109");
    }
  }

  void Frontend::resolve_user_id_(
    AdServer::Commons::UserId& match_user_id,
    FrontendCommons::GrpcCampaignManagerPool::CommonAdRequestInfo& common_info,
    RequestInfo& request_info) noexcept
  {
    static const char* FUN = "Bidding::Frontend::resolve_user_id_()";

    Generics::Time start_process_time;

    if (logger()->log_level() >= Logging::Logger::TRACE)
    {
      start_process_time = Generics::Time::get_time_of_day();
    }

    if (request_info.filter_request)
    {
      common_info.user_status = static_cast<std::uint32_t>(
        AdServer::CampaignSvcs::US_FOREIGN);
    }
    else if (!common_info.signed_user_id.empty() &&
      common_info.user_status != AdServer::CampaignSvcs::US_PROBE)
    {
      common_info.user_status = AdServer::CampaignSvcs::US_OPTIN;
      match_user_id = common_info.user_id;
    }
    else if (!request_info.advertising_id.empty() ||
      !request_info.idfa.empty() ||
      !common_info.external_user_id.empty())
    {
      std::vector<std::string> external_user_ids;
      for(auto ext_user_id = request_info.ext_user_ids.begin();
          ext_user_id != request_info.ext_user_ids.end();
          ++ext_user_id)
      {
        external_user_ids.emplace_back(*ext_user_id);
      }

      if(!request_info.idfa.empty())
      {
        std::string resolve_idfa = request_info.idfa;
        String::AsciiStringManip::to_lower(resolve_idfa);
        external_user_ids.push_back(std::string("ifa/") + resolve_idfa);
      }

      if(!request_info.advertising_id.empty())
      {
        std::string resolve_idfa = request_info.advertising_id;
        String::AsciiStringManip::to_lower(resolve_idfa);
        external_user_ids.push_back(std::string("ifa/") + resolve_idfa);
      }

      if(!common_info.external_user_id.empty())
      {
        external_user_ids.push_back(common_info.external_user_id);
      }

      assert(!external_user_ids.empty());

      try
      {
        const auto& grpc_user_bind_operation_distributor =
          grpc_container_->grpc_user_bind_operation_distributor;

        auto base_ext_user_id_it = external_user_ids.begin();
        AdServer::Commons::UserId local_match_user_id;
        bool blacklisted = false;
        bool min_age_reached = false;
        bool user_found = false;

        for(auto ext_user_id_it = external_user_ids.begin();
            ext_user_id_it != external_user_ids.end();
            ++ext_user_id_it)
        {
          auto response = grpc_user_bind_operation_distributor->get_user_id(
            *ext_user_id_it,
            {},
            request_info.current_time,
            request_info.user_create_time,
            false,
            false,
            false);

          if (response && response->has_info())
          {
            const auto& info = response->info();
            min_age_reached |= info.min_age_reached();
            local_match_user_id = GrpcAlgs::unpack_user_id(info.user_id());
            user_found = info.user_found();
          }
          else
          {
            GrpcAlgs::print_grpc_error_response(
              response,
              logger(),
              Aspect::BIDDING_FRONTEND);
            throw Exception("unpack_user_id is failed");
          }

          blacklisted |= common_module_->user_id_controller()->null_blacklisted(match_user_id);

          if(!local_match_user_id.is_null())
          {
            common_info.external_user_id = *ext_user_id_it;
            base_ext_user_id_it = ext_user_id_it;
            break;
          }
          else if(common_info.external_user_id.empty())
          {
            common_info.external_user_id = *ext_user_id_it;
          }
        }

        match_user_id = local_match_user_id;

        common_module_->user_id_controller()->null_blacklisted(match_user_id);

        if (!match_user_id.is_null())
        { //(external_user_id is not found and user_id is not null,
          //  in other words: min_age_reached=true && bind_on_min_age=true -> user_id generated)
          //or
          //(external_user_id is found and user_id is not null)
          // link other external user ids to base
          for (auto ext_user_id_it = external_user_ids.begin();
              ext_user_id_it != external_user_ids.end();
              ++ext_user_id_it)
          {
            if (ext_user_id_it != base_ext_user_id_it)
            {
              auto response = grpc_user_bind_operation_distributor->add_user_id(
                *ext_user_id_it,
                request_info.current_time,
                GrpcAlgs::pack_user_id(match_user_id));
              if (!response || response->has_error())
              {
                GrpcAlgs::print_grpc_error_response(
                  response,
                  logger(),
                  Aspect::BIDDING_FRONTEND);
                throw Exception("add_user_id is failed");
              }
            }
          }

          common_info.user_status = AdServer::CampaignSvcs::US_OPTIN;
          common_info.signed_user_id =
            common_module_->user_id_controller()->sign(match_user_id).str();
        }
        else if (blacklisted)
        {
          common_info.user_status = AdServer::CampaignSvcs::US_UNDEFINED;
        }
        else if (user_found)
        { //external_user_id is found and user_id is null
          common_info.user_status = AdServer::CampaignSvcs::US_OPTOUT;
        }
        //external_user_id is not found and user_id is null
        else if (min_age_reached)
        {
          // uid generation on RTB requests disabled (bind_on_min_age=false)
          common_info.user_status = AdServer::CampaignSvcs::US_UNDEFINED;
        }
        else
        {
          common_info.user_status = AdServer::CampaignSvcs::US_EXTERNALPROBE;
        }
      }
      catch (const eh::Exception& exc)
      {
        Stream::Error ostr;
        ostr << FNS << exc.what();
        logger()->log(ostr.str(),
          Logging::Logger::ERROR,
          Aspect::BIDDING_FRONTEND,
          "ADS-ICON-7800");
      }
    }
    else if (common_info.user_status != AdServer::CampaignSvcs::US_PROBE)
    {
      common_info.user_status = AdServer::CampaignSvcs::US_NOEXTERNALID;
    }

    if (common_info.user_status != AdServer::CampaignSvcs::US_OPTIN)
    {
      // US_FOREIGN already filtered - filter US_NOEXTERNALID fully
      // and disable ccg keywords loading for non opt in (US_EXTERNALPROBE actually)
      // if colocation configured to passback for non opt-in
      ExtConfig_var ext_config = get_ext_config_();

      if (ext_config.in())
      {
        ExtConfig::ColocationMap::const_iterator colo_it =
          ext_config->colocations.find(common_info.colo_id);

        if( colo_it != ext_config->colocations.end() &&
           (colo_it->second.flags == CampaignSvcs::CS_NONE ||
            colo_it->second.flags == CampaignSvcs::CS_ONLY_OPTIN))
        {
          if (colo_it->second.flags == CampaignSvcs::CS_NONE ||
            common_info.user_status == AdServer::CampaignSvcs::US_NOEXTERNALID)
          {
            request_info.filter_request = true;
          }
          request_info.skip_ccg_keywords = true;
        }
      }
    }

    if (logger()->log_level() >= Logging::Logger::TRACE)
    {
      Generics::Time end_process_time = Generics::Time::get_time_of_day();
      Stream::Error ostr;
      ostr << FUN << ": user is resolving time = " <<
        (end_process_time - start_process_time);
      logger()->log(ostr.str(),
        Logging::Logger::TRACE,
        Aspect::BIDDING_FRONTEND);
    }

    if (config_->trace_mapping() &&
        logger()->log_level() >= Logging::Logger::DEBUG)
    {
      Stream::Error ostr;
      ostr << FUN << ": SSP user mapping: " << match_user_id.to_string() <<
        " <-> (" << common_info.external_user_id << ", " <<
        request_info.source_id << ')';
      logger()->log(ostr.str(),
        Logging::Logger::DEBUG,
        Aspect::BIDDING_FRONTEND);
    }
  }

  void Frontend::trigger_match_(
    std::unique_ptr<AdServer::ChannelSvcs::Proto::MatchResponseInfo>& trigger_matched_channels,
    FrontendCommons::GrpcCampaignManagerPool::RequestParams& request_params,
    const RequestInfo& request_info,
    const AdServer::Commons::UserId& user_id,
    const char* keywords) noexcept
  {
    DO_TIME_STATISTIC_FRONTEND(FrontendCommons::TimeStatisticId::BiddingFrontend_ServerRequest);

    if (!request_info.filter_request)
    {
      try
      {
        std::string first_url_words;
        try
        {
          std::string ref_words;
          FrontendCommons::extract_url_keywords(
            ref_words,
            String::SubString(request_params.common_info.referer),
            common_module_->segmentor());

          if (!ref_words.empty())
          {
            first_url_words = std::move(ref_words);
          }
        }
        catch (const eh::Exception& exc)
        {
          Stream::Error stream;
          stream << FNS
                 << "Url keywords extracting error: "
                 << exc.what();
          logger()->log(
            stream.str(),
            Logging::Logger::TRACE,
            Aspect::BIDDING_FRONTEND);
        }

        std::string urls;
        std::string urls_words;
        const std::size_t common_info_urls_size =
          request_params.common_info.urls.size();
        for (std::size_t i = 0; i < common_info_urls_size; ++i)
        {
          if (i != 0)
          {
            urls += '\n';
          }
          urls += request_params.common_info.urls[i];

          std::string url_words_res;
          FrontendCommons::extract_url_keywords(
            url_words_res,
            String::SubString(request_params.common_info.urls[i]),
            common_module_->segmentor());

          if (!url_words_res.empty())
          {
            if (!urls_words.empty())
            {
              urls_words += '\n';
            }
            urls_words += url_words_res;
          }
        }

        const auto& grpc_channel_operation_pool =
          grpc_container_->grpc_channel_operation_pool;
        auto response = grpc_channel_operation_pool->match(
          {},                                        // request_id
          request_params.common_info.referer,        // first_url
          first_url_words,                           // first_url_words
          urls,                                      // urls
          urls_words,                                // urls_words
          keywords,                                  // pwords
          request_info.search_words,                 // swords
          GrpcAlgs::pack_user_id(user_id),           // uid
          {'A', '\0'},                               // statuses
          false,                                     // non_strict_word_match
          false,                                     // non_strict_url_match
          false,                                     // return_negative,
          true,                                      // simplify_page,
          true);                                     // fill_content

        if (response && response->has_info())
        {
          trigger_matched_channels.reset(response->release_info());
        }
        else
        {
          GrpcAlgs::print_grpc_error_response(
            response,
            logger(),
            Aspect::BIDDING_FRONTEND);
          throw Exception("match is failed");
        }

        if (trigger_matched_channels)
        {
          const auto& trigger_page_channels =
            trigger_matched_channels->matched_channels().page_channels();
          auto& pkw_channels = request_params.trigger_match_result.pkw_channels;
          pkw_channels.reserve(trigger_page_channels.size());
          for (const auto& trigger_page_channel : trigger_page_channels)
          {
            pkw_channels.emplace_back(
              trigger_page_channel.trigger_channel_id(),
              trigger_page_channel.id());
          }

          const auto& trigger_url_channels =
            trigger_matched_channels->matched_channels().url_channels();
          auto& url_channels = request_params.trigger_match_result.url_channels;
          url_channels.reserve(trigger_url_channels.size());
          for (const auto& trigger_url_channel : trigger_url_channels)
          {
            url_channels.emplace_back(
              trigger_url_channel.trigger_channel_id(),
              trigger_url_channel.id());
          }

          const auto& trigger_url_keyword_channels =
            trigger_matched_channels->matched_channels().url_keyword_channels();
          auto& ukw_channels = request_params.trigger_match_result.ukw_channels;
          ukw_channels.reserve(trigger_url_keyword_channels.size());
          for (const auto& trigger_url_keyword_channel : trigger_url_keyword_channels)
          {
            ukw_channels.emplace_back(
              trigger_url_keyword_channel.trigger_channel_id(),
              trigger_url_keyword_channel.id());
          }

          const auto& trigger_search_channels =
            trigger_matched_channels->matched_channels().search_channels();
          auto& skw_channels = request_params.trigger_match_result.skw_channels;
          skw_channels.reserve(trigger_search_channels.size());
          for (const auto& trigger_search_channel : trigger_search_channels)
          {
            skw_channels.emplace_back(
              trigger_search_channel.trigger_channel_id(),
              trigger_search_channel.id());
          }

          const auto& trigger_uid_channels =
            trigger_matched_channels->matched_channels().uid_channels();
          auto& uid_channels = request_params.trigger_match_result.uid_channels;
          uid_channels.reserve(trigger_uid_channels.size());
          uid_channels.insert(
            std::end(uid_channels),
            std::begin(trigger_uid_channels),
            std::end(trigger_uid_channels));

          if (request_params.common_info.user_status ==
              static_cast<std::uint32_t>(AdServer::CampaignSvcs::US_OPTIN) &&
              (trigger_matched_channels->no_track() || trigger_matched_channels->no_adv()))
          {
            request_params.common_info.user_status = AdServer::CampaignSvcs::US_BLACKLISTED;
          }
        }
      }
      catch (const eh::Exception& exc)
      {
        Stream::Error stream;
        stream << FNS
               << exc.what();
        logger()->log(stream.str(),
          Logging::Logger::EMERGENCY,
          Aspect::BIDDING_FRONTEND,
          "ADS-IMPL-117");
      }
    }
  }

  void Frontend::history_match_(
    std::unique_ptr<AdServer::UserInfoSvcs::Proto::MatchResult>& history_match_result,
    FrontendCommons::GrpcCampaignManagerPool::RequestParams& request_params,
    const RequestInfo& request_info,
    const AdServer::ChannelSvcs::Proto::MatchResponseInfo* const trigger_match_result,
    const AdServer::Commons::UserId& user_id,
    const Generics::Time& time) noexcept
  {
    using UserInfo = AdServer::UserInfoSvcs::Types::UserInfo;
    using MatchParams = AdServer::UserInfoSvcs::Types::MatchParams;
    using ChannelMatchSet = std::set<ChannelMatch>;

    DO_TIME_STATISTIC_FRONTEND(FrontendCommons::TimeStatisticId::BiddingFrontend_UserInfoRequest);

    request_params.profiling_available = false;

    if(!user_id.is_null())
    {
      const auto grpc_user_info_operation_distributor =
        grpc_container_->grpc_user_info_operation_distributor;

      try
      {
        MatchParams match_params;
        match_params.use_empty_profile = false;
        match_params.silent_match = false;
        match_params.no_match = trigger_match_result && trigger_match_result->no_track();
        match_params.no_result = false;
        match_params.ret_freq_caps = true;
        match_params.provide_channel_count = false;
        match_params.provide_persistent_channels = false;
        match_params.change_last_request = true;
        match_params.publishers_optin_timeout = time - Generics::Time::ONE_DAY * 15;
        match_params.cohort = (!request_info.idfa.empty() ?
          request_info.idfa : request_info.advertising_id);

        const auto& platform_ids = request_params.context_info.platform_ids;
        match_params.persistent_channel_ids.reserve(platform_ids.size());
        match_params.persistent_channel_ids.insert(
          match_params.persistent_channel_ids.end(),
          std::begin(platform_ids),
          std::end(platform_ids));

        UserInfo user_info;
        user_info.user_id = GrpcAlgs::pack_user_id(user_id);
        user_info.huser_id = GrpcAlgs::pack_user_id(AdServer::Commons::UserId{});
        user_info.last_colo_id = colo_id_;
        user_info.request_colo_id = colo_id_;
        user_info.current_colo_id = -1;
        user_info.temporary = false;
        user_info.time = time.tv_sec;

        if (trigger_match_result && !trigger_match_result->no_track())
        {
          ChannelMatchSet page_channels;
          ChannelMatchSet url_channels;
          ChannelMatchSet search_channels;
          ChannelMatchSet url_keyword_channels;

          const auto& trigger_page_channels =
            trigger_match_result->matched_channels().page_channels();
          for (const auto& trigger_page_channel : trigger_page_channels)
          {
            page_channels.emplace(
              trigger_page_channel.id(),
              trigger_page_channel.trigger_channel_id());
          }

          const auto& trigger_url_channels =
            trigger_match_result->matched_channels().url_channels();
          for (const auto& trigger_url_channel : trigger_url_channels)
          {
            url_channels.emplace(
              trigger_url_channel.id(),
              trigger_url_channel.trigger_channel_id());
          }

          const auto& trigger_search_channels =
            trigger_match_result->matched_channels().search_channels();
          for (const auto& trigger_search_channel : trigger_search_channels)
          {
            search_channels.emplace(
              trigger_search_channel.id(),
              trigger_search_channel.trigger_channel_id());
          }

          const auto& trigger_url_keyword_channels =
            trigger_match_result->matched_channels().url_keyword_channels();
          for (const auto& trigger_url_keyword_channel : trigger_url_keyword_channels)
          {
            url_keyword_channels.emplace(
              trigger_url_keyword_channel.id(),
              trigger_url_keyword_channel.trigger_channel_id());
          }

          match_params.page_channel_ids.reserve(page_channels.size());
          for (const auto& page_channel : page_channels)
          {
            match_params.page_channel_ids.emplace_back(
              page_channel.channel_id,
              page_channel.channel_trigger_id);
          }

          match_params.url_channel_ids.reserve(url_channels.size());
          for (const auto& url_channel : url_channels)
          {
            match_params.url_channel_ids.emplace_back(
              url_channel.channel_id,
              url_channel.channel_trigger_id);
          }

          match_params.search_channel_ids.reserve(search_channels.size());
          for (const auto& search_channel : search_channels)
          {
            match_params.search_channel_ids.emplace_back(
              search_channel.channel_id,
              search_channel.channel_trigger_id);
          }

          match_params.url_keyword_channel_ids.reserve(url_keyword_channels.size());
          for (const auto& url_keyword_channel : url_keyword_channels)
          {
            match_params.url_keyword_channel_ids.emplace_back(
              url_keyword_channel.channel_id,
              url_keyword_channel.channel_trigger_id);
          }
        }

        auto response = grpc_user_info_operation_distributor->match(user_info, match_params);
        if (response && response->has_info())
        {
          request_params.profiling_available = true;

          auto* response_info = response->mutable_info();
          history_match_result.reset(response_info->release_match_result());
        }
        else
        {
          GrpcAlgs::print_grpc_error_response(
            response,
            logger(),
            Aspect::BIDDING_FRONTEND);
        }
      }
      catch (const eh::Exception& exc)
      {
        Stream::Error stream;
        stream << FNS
               << exc.what();
        logger()->error(
          stream.str(),
          Aspect::BIDDING_FRONTEND);
      }
      catch (...)
      {
        Stream::Error stream;
        stream << FNS
               << ": Unknown error";
        logger()->error(stream.str(), Aspect::BIDDING_FRONTEND);
      }
    }
    else if (trigger_match_result && !trigger_match_result->no_track())
    {
      history_match_result = get_empty_history_matching_();

      auto* history_matched_channels = history_match_result->mutable_channels();
      const auto& content_channels = trigger_match_result->content_channels();

      history_matched_channels->Reserve(content_channels.size());
      for(const auto& content_channel : content_channels)
      {
        auto* channel_weight = history_matched_channels->Add();
        channel_weight->set_channel_id(content_channel.id());
        channel_weight->set_weight(content_channel.weight());
      }
    }

    if (!history_match_result)
    {
      history_match_result = get_empty_history_matching_();

      if (trigger_match_result)
      {
        const auto& content_channels = trigger_match_result->content_channels();
        auto* history_matched_channels = history_match_result->mutable_channels();
        history_matched_channels->Reserve(content_channels.size());
        for (const auto& content_channel : content_channels)
        {
          auto* channel_weight = history_matched_channels->Add();
          channel_weight->set_channel_id(content_channel.id());
          channel_weight->set_weight(content_channel.weight());
        }
      }
    }

    // resolve ip to colo_id
    FrontendCommons::IPMatcher_var ip_matcher =
      common_module_->ip_matcher();

    try
    {
      FrontendCommons::IPMatcher::MatchResult ip_match_result;
      if (ip_matcher.in() &&
         !request_params.common_info.peer_ip.empty() &&
         ip_matcher->match(
           ip_match_result,
           String::SubString(request_params.common_info.peer_ip),
           String::SubString()))
      {
        request_params.common_info.colo_id = ip_match_result.colo_id;
        request_params.context_info.profile_referer =
           config_->enable_profile_referer() && ip_match_result.profile_referer;
      }
    }
    catch (const FrontendCommons::IPMatcher::InvalidParameter&)
    {
    }
  }

  bool Frontend::consider_campaign_selection_(
    const AdServer::Commons::UserId& user_id, // not null
    const Generics::Time& now,
    const AdServer::CampaignSvcs::Proto::RequestCreativeResult& campaign_match_result) noexcept
  {
    bool result = false;
    if (user_id.is_null())
    {
      return true;
    }
    else
    {
      std::size_t seq_order_size = 0;
      const auto& ad_slots = campaign_match_result.ad_slots();
      for (const auto& ad_slot : ad_slots)
      {
        const auto& selected_creatives = ad_slot.selected_creatives();
        for (const auto& selected_creative : selected_creatives)
        {
          if (selected_creative.order_set_id())
          {
            seq_order_size += 1;
          }
        }
      }

      try
      {
        using SeqOrders = AdServer::UserInfoSvcs::Types::SeqOrders;
        using CampaignIds = AdServer::UserInfoSvcs::Types::CampaignIds;
        using FreqCaps = AdServer::UserInfoSvcs::Types::FreqCaps;
        using UcFreqCaps = AdServer::UserInfoSvcs::Types::UcFreqCaps;
        using UcCampaignIds = AdServer::UserInfoSvcs::Types::UcCampaignIds;

        auto& grpc_user_info_operation_distributor = grpc_container_->grpc_user_info_operation_distributor;

        result = true;
        SeqOrders seq_orders;
        seq_orders.reserve(seq_order_size);
        for (const auto& ad_slot : ad_slots)
        {
          if (!ad_slot.selected_creatives().empty())
          {
            const auto& selected_creatives = ad_slot.selected_creatives();
            CampaignIds campaign_ids;
            campaign_ids.reserve(selected_creatives.size());
            for (const auto& creative : selected_creatives)
            {
              if (creative.order_set_id())
              {
                const auto ccg_id = creative.cmp_id();
                seq_orders.emplace_back(ccg_id, creative.order_set_id(), 1);
                ADD_COMMON_COUNTER_STATISTIC(FrontendCommons::CounterStatisticId::BiddingFrontend_Bids, ccg_id, 1);
              }

              campaign_ids.emplace_back(creative.campaign_group_id());
            }

            FreqCaps freq_caps;
            freq_caps.insert(
              std::end(freq_caps),
              std::begin(ad_slot.freq_caps()),
              std::end(ad_slot.freq_caps()));

            UcFreqCaps uc_freq_caps;
            uc_freq_caps.insert(
              std::end(uc_freq_caps),
              std::begin(ad_slot.uc_freq_caps()),
              std::end(ad_slot.uc_freq_caps()));

            auto response = grpc_user_info_operation_distributor->update_user_freq_caps(
              GrpcAlgs::pack_user_id(user_id),
              now,
              ad_slot.request_id(),
              freq_caps,
              uc_freq_caps,
              {},
              seq_orders,
              ad_slot.track_impr() ? CampaignIds{} : campaign_ids,
              ad_slot.track_impr() ? campaign_ids : UcCampaignIds{});
            if (!response || response->has_error())
            {
              GrpcAlgs::print_grpc_error_response(
                response,
                logger(),
                Aspect::BIDDING_FRONTEND);

              result = false;
              break;
            }
          }
        }
      }
      catch (const eh::Exception& exc)
      {
        result = false;
        Stream::Error stream;
        stream << FNS
               << exc.what();
        logger()->error(
          stream.str(),
          Aspect::BIDDING_FRONTEND);
      }
      catch (...)
      {
        result = false;
        Stream::Error stream;
        stream << FNS
               << "Unknown error";
        logger()->error(
          stream.str(),
          Aspect::BIDDING_FRONTEND);
      }
    }

    return result;
  }

  bool Frontend::process_bid_request_(
    const char* fn,
    std::unique_ptr<AdServer::CampaignSvcs::Proto::RequestCreativeResult>& campaign_match_result,
    AdServer::Commons::UserId& user_id,
    BidRequestTask* request_task,
    RequestInfo& request_info,
    const std::string& keywords) noexcept
  {
    bool interrupted = false;
    auto& request_params = *request_task->request_params();

    // map external id to uid
    resolve_user_id_(
      user_id,
      request_params.common_info,
      request_info);

    if (check_interrupt_(fn, Stage::UserResolving, request_task))
    {
      interrupted = true;
    }

    std::unique_ptr<AdServer::ChannelSvcs::Proto::MatchResponseInfo> trigger_match_result;
    if (!interrupted)
    {
      trigger_match_(
        trigger_match_result,
        request_params,
        request_info,
        user_id,
        keywords.c_str());

      if (check_interrupt_(fn, Stage::TriggerMatching, request_task))
      {
        interrupted = true;
      }
    }

    // process bid request source independently
    std::unique_ptr<AdServer::UserInfoSvcs::Proto::MatchResult> history_match_result;
    if (!interrupted)
    {
      history_match_(
        history_match_result,
        request_params,
        request_info,
        trigger_match_result.get(),
        user_id,
        request_info.current_time);

      if (check_interrupt_(fn, Stage::HistoryMatching, request_task))
      {
        interrupted = true;
      }
    }
    else
    {
      history_match_result = get_empty_history_matching_();
    }

    if(interrupted)
    {
      return false;
    }

    select_campaign_(
      campaign_match_result,
      *history_match_result,
      trigger_match_result.get(),
      request_params,
      request_task->hostname(),
      request_info,
      user_id,
      (trigger_match_result &&
        (trigger_match_result->no_track() || trigger_match_result->no_adv())) ||
      request_info.filter_request,
      interrupted);

    if (!interrupted)
    {
      if(check_interrupt_(fn, Stage::CampaignSelection, request_task))
      {
        return false;
      }

      if(campaign_match_result &&
        !campaign_match_result->ad_slots().empty() &&
        !campaign_match_result->ad_slots()[0].debug_info().trace_ccg().empty() &&
        request_params.ad_slots.size() > 0 &&
        logger()->log_level() >= Logging::Logger::TRACE)
      {
        std::ostringstream ostr;
        ostr << fn << ": CCG Trace of " <<
        request_params.ad_slots[0].debug_ccg <<
          " for request:" << std::endl;

        request_task->print_request(ostr);

        ostr << std::endl << campaign_match_result->ad_slots()[0].debug_info().trace_ccg();
        std::cout << ostr.str() << std::endl;

        logger()->log(
          ostr.str(),
          Logging::Logger::TRACE,
          Aspect::BIDDING_FRONTEND);
      }
    }
    else
    {
      // Interrupted CM selection should be the last call of the RequestTask,
      // RequestTask's request params are invalid after the call.
      interrupted_select_campaign_(
        request_task);

      return false;
    }

    return true;
  }

  void Frontend::interrupted_select_campaign_(
    BidRequestTask* request_task) noexcept
  {
    try
    {
      ConstRequestParamsHolder_var
        request_params(request_task->request_params());

      auto& current_task_processor = userver::engine::current_task::GetTaskProcessor();
      userver::engine::AsyncNoSpan(
        current_task_processor,
        [
          grpc_campaign_manager = grpc_container_->grpc_campaign_manager_pool,
          request_params = std::move(request_params)
        ] () {
          try
          {
            grpc_campaign_manager->get_campaign_creative(*request_params);
          }
          catch (...)
          {
          }
        }).Detach();
    }
    catch (const eh::Exception& exc)
    {
      Stream::Error stream;
      stream << FNS
             << "eh::Exception caught: "
             << exc.what();
      logger()->log(stream.str(),
        Logging::Logger::EMERGENCY,
        Aspect::BIDDING_FRONTEND,
        "ADS-IMPL-10554");
    }
  }

  void Frontend::select_campaign_(
    std::unique_ptr<AdServer::CampaignSvcs::Proto::RequestCreativeResult>& campaign_match_result,
    AdServer::UserInfoSvcs::Proto::MatchResult& history_match_result,
    const AdServer::ChannelSvcs::Proto::MatchResponseInfo* trigger_match_result,
    FrontendCommons::GrpcCampaignManagerPool::RequestParams& request_params,
    std::string& hostname,
    const RequestInfo& request_info,
    const AdServer::Commons::UserId& user_id,
    const bool passback,
    const bool interrupted) noexcept
  {
    // do campaign selection
    try
    {
      // Fill user info
      if (!user_id.is_null())
      {
        request_params.common_info.user_id = user_id;
        request_params.common_info.track_user_id = request_params.common_info.user_id;
      }

      // Fill debug-info & process fraud
      request_params.only_display_ad = false;
      if (!request_params.ad_slots.empty())
      {
        if (!interrupted)
        {
          request_params.need_debug_info = true;
          request_params.ad_slots[0].debug_ccg = request_info.debug_ccg;
        }

        auto& ad_slots = request_params.ad_slots;
        for (auto& ad_slot : ad_slots)
        {
          ad_slot.passback |= passback || interrupted || history_match_result.fraud_request();
        }
      }

      // Fill user history data
      request_params.client_create_time = GrpcAlgs::unpack_time(history_match_result.create_time());
      request_params.session_start = GrpcAlgs::unpack_time(history_match_result.session_start());

      const auto& history_full_freq_caps = history_match_result.full_freq_caps();
      request_params.full_freq_caps.reserve(history_full_freq_caps.size());
      request_params.full_freq_caps.insert(
        std::end(request_params.full_freq_caps),
        std::begin(history_full_freq_caps),
        std::end(history_full_freq_caps));

      const auto& history_exclude_pubpixel_accounts = history_match_result.exclude_pubpixel_accounts();
      request_params.exclude_pubpixel_accounts.reserve(history_exclude_pubpixel_accounts.size());
      request_params.exclude_pubpixel_accounts.insert(
        std::end(request_params.exclude_pubpixel_accounts),
        std::begin(history_exclude_pubpixel_accounts),
        std::end(history_exclude_pubpixel_accounts));

      const auto& history_campaign_freqs = history_match_result.campaign_freqs();
      request_params.campaign_freqs.reserve(history_campaign_freqs.size());
      for (const auto& freq : history_campaign_freqs)
      {
        request_params.campaign_freqs.emplace_back(
          freq.campaign_id(),
          freq.imps());
      }

      const auto& history_seq_orders = history_match_result.seq_orders();
      request_params.seq_orders.reserve(history_seq_orders.size());
      for (const auto& seq_order : history_seq_orders)
      {
        request_params.seq_orders.emplace_back(
          seq_order.ccg_id(),
          seq_order.set_id(),
          seq_order.imps());
      }

      const auto& history_geo_data_seq = history_match_result.geo_data_seq();
      request_params.common_info.coord_location.reserve(history_geo_data_seq.size());
      for (const auto& geo_data : history_geo_data_seq)
      {
        request_params.common_info.coord_location.emplace_back(
          GrpcAlgs::unpack_decimal<AdServer::CampaignSvcs::CoordDecimal>(geo_data.longitude()),
          GrpcAlgs::unpack_decimal<AdServer::CampaignSvcs::CoordDecimal>(geo_data.latitude()),
          GrpcAlgs::unpack_decimal<AdServer::CampaignSvcs::CoordDecimal>(geo_data.accuracy()));
      }

      std::size_t uid_channels_size = 0;
      if (trigger_match_result)
      {
        uid_channels_size = trigger_match_result->matched_channels().uid_channels().size();
      }

      request_params.channels.reserve(
        history_match_result.channels().size() + uid_channels_size);

      const auto& history_channels = history_match_result.channels();
      for (const auto& channel : history_channels)
      {
        request_params.channels.emplace_back(channel.channel_id());
      }

      if (trigger_match_result)
      {
        const auto& trigger_uid_channels = trigger_match_result->matched_channels().uid_channels();
        for (const auto& uid_channel : trigger_uid_channels)
        {
          request_params.channels.emplace_back(uid_channel);
        }
      }

      // Process black list users
      const Generics::Time day_time =
        request_info.current_time - request_info.current_time.get_gm_time().get_date();

      if (blacklisted_time_intervals_.contains(day_time))
      {
        if (request_params.common_info.user_status ==
            static_cast<std::uint32_t>(CampaignSvcs::US_OPTIN))
        {
          request_params.common_info.user_status = CampaignSvcs::US_BLACKLISTED;
        }

        for (auto& ad_slot : request_params.ad_slots)
        {
          ad_slot.passback = true;
        }
      }

      {
        // fill special tokens
        if (!request_info.bid_request_id.empty())
        {
          request_params.common_info.tokens.emplace_back(
            "BR_ID",
            request_info.bid_request_id);
        }

        if (!request_info.bid_site_id.empty())
        {
          request_params.common_info.tokens.emplace_back(
            "BS_ID",
            request_info.bid_site_id);
        }

        if (!request_info.bid_publisher_id.empty())
        {
          request_params.common_info.tokens.emplace_back(
            "BP_ID",
            request_info.bid_publisher_id);
        }

        if (!request_info.application_id.empty())
        {
          request_params.common_info.tokens.emplace_back(
            "APPLICATION_ID",
            request_info.application_id);
        }

        if (!request_info.idfa.empty())
        {
          request_params.common_info.tokens.emplace_back(
            "IDFA",
            request_info.idfa);
        }
        else if (!request_info.advertising_id.empty())
        {
          request_params.common_info.tokens.emplace_back(
            "ADVERTISING_ID",
            request_info.advertising_id);
        }
        else if (!history_match_result.cohort().empty())
        {
          if(request_info.platform_names.find("ipad") !=
              request_info.platform_names.end() ||
            request_info.platform_names.find("iphone") !=
              request_info.platform_names.end() ||
            request_info.platform_names.find("ios") !=
              request_info.platform_names.end())
          {
            request_params.common_info.tokens.emplace_back(
              "IDFA",
              history_match_result.cohort());
          }
          else
          {
            request_params.common_info.tokens.emplace_back(
              "ADVERTISING_ID",
              history_match_result.cohort());
          }
        }

        if (!request_params.common_info.ext_track_params.empty())
        {
          request_params.common_info.tokens.emplace_back(
            "EXT_TRACK_PARAMS",
            request_params.common_info.ext_track_params);
        }

        if (request_info.location)
        {
          request_params.common_info.tokens.emplace_back(
            "GEO_REGION",
            request_info.location->region);
        }

        if (!request_info.ssp_devicetype_str.empty())
        {
          request_params.common_info.tokens.emplace_back(
            "SSP_DEVICETYPE",
            request_info.ssp_devicetype_str);
        }
      }

      if (interrupted)
      {
        request_params.required_passback = false;
      }
      else
      {
        auto& grpc_campaign_manager_pool = grpc_container_->grpc_campaign_manager_pool;
        auto response = grpc_campaign_manager_pool->get_campaign_creative(request_params);
        if (response && response->has_info())
        {
          auto* info_proto = response->mutable_info();
          campaign_match_result.reset(info_proto->release_request_result());
          hostname = info_proto->hostname();
        }
        else
        {
          GrpcAlgs::print_grpc_error_response(
            response,
            logger(),
            Aspect::BIDDING_FRONTEND);
        }
      }
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error stream;
      stream << FNS
             << ex.what();
      logger()->log(
        stream.str(),
        Logging::Logger::EMERGENCY,
        Aspect::BIDDING_FRONTEND,
        "ADS-IMPL-118");
    }
  }

  void Frontend::update_config_() noexcept
  {
    while (!userver::engine::current_task::ShouldCancel())
    {
      try
      {
        auto& grpc_campaign_manager_pool =
          grpc_container_->grpc_campaign_manager_pool;
        auto response = grpc_campaign_manager_pool->get_colocation_flags();
        if (response && response->has_info())
        {
          const auto& info_proto = response->info();
          const auto& colocation_flags_proto = info_proto.colocation_flags();

          ExtConfig_var new_config(new ExtConfig());
          for (const auto& colocation_flag: colocation_flags_proto)
          {
            ExtConfig::Colocation colocation;
            colocation.flags = colocation_flag.flags();
            new_config->colocations.insert(
              ExtConfig::ColocationMap::value_type(
                colocation_flag.colo_id(),
                colocation));
          }

          set_ext_config_(new_config);
        }
        else
        {
          GrpcAlgs::print_grpc_error_response(
            response,
            logger(),
            Aspect::BIDDING_FRONTEND);
          throw Exception("get_colocation_flags is failed");
        }
      }
      catch (const eh::Exception& exc)
      {
        logger()->sstream(
          Logging::Logger::CRITICAL,
          Aspect::BIDDING_FRONTEND,
          "ADS-IMPL-118")
          << FNS
          << "Can't update colocation flags, "
             "caught eh::Exception: "
          << exc.what();
      }

      userver::engine::InterruptibleSleepFor(
        std::chrono::seconds(common_config_->update_period()));
    }
  }

  std::unique_ptr<AdServer::UserInfoSvcs::Proto::MatchResult>
  Frontend::get_empty_history_matching_()
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

  void Frontend::protobuf_log_handler_(
    google::protobuf::LogLevel level,
    const char* filename,
    int line,
    const std::string& message)
  {
    static const char* level_names[] = { "INFO", "WARNING", "ERROR", "FATAL" };

    if (level == google::protobuf::LOGLEVEL_ERROR ||
      level == google::protobuf::LOGLEVEL_FATAL)
    {
      Stream::Error ostr;
      ostr << "[libprotobuf " << level_names[level] <<
        ' ' << filename << ':' << line << "] " << message;
      throw BidRequestTask::Invalid(ostr.str());
    }
  }

  void Frontend::fill_account_traits_() noexcept
  {
    for (auto account_it = config_->Account().begin();
         account_it != config_->Account().end();
         ++account_it)
    {
      RequestInfoFiller::AccountTraits_var& target_account_ptr =
        account_traits_[account_it->account_id()];

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

  void Frontend::limit_max_cpm_(
    AdServer::CampaignSvcs::RevenueDecimal& val,
    const std::vector<std::uint32_t>& account_ids) const noexcept
  {
    for (const auto account_id : account_ids)
    {
      auto account_it = account_traits_.find(account_id);
      if (account_it != account_traits_.end() && account_it->second->max_cpm)
      {
        val = std::min(val, *(account_it->second->max_cpm));
      }
    }
  }

  void Frontend::interrupt_(
    const char* fun,
    const Stage stage,
    const BidRequestTask* request_task) noexcept
  {
    Generics::Time timeout = Generics::Time::get_time_of_day() - request_task->start_processing_time();
    if (stats_.in())
    {
      stats_->add_timeout(timeout);
    }

    if (logger()->log_level() >= Logging::Logger::TRACE)
    {
      std::ostringstream ostr;
      ostr << FNS
           << "request processing timed out(" << timeout << "):"
           << std::endl;

      request_task->print_request(ostr);

      logger()->log(
        ostr.str(),
        Logging::Logger::TRACE,
        Aspect::BIDDING_FRONTEND);
    }

    std::string ostr(fun);
    ostr += ": interrupted at ";
    ostr += convert_stage_to_string(stage);
    ostr += ", after";

    group_logger()->add_error(
      !request_task->hostname().empty() ?
        request_task->hostname().c_str() : "Undefined host",
      ostr,
      timeout,
      Logging::Logger::ERROR,
      Aspect::BIDDING_FRONTEND,
      "ADS-IMPL-7600");
  }

  bool Frontend::check_interrupt_(
    const char* fun,
    const Stage stage,
    BidRequestTask* request_task) noexcept
  {
    request_task->set_current_stage(stage);
    if (request_task->interrupted())
    {
      interrupt_(fun, stage, request_task);
      return true;
    }

    return false;
  }

  AdServer::CampaignSvcs::AdInstantiateType
  Frontend::adapt_instantiate_type_(const std::string& inst_type_str)
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
    ostr << "unknown instantiate type '"
         << inst_type_str
         << "'";
    throw Exception(ostr);
  }

  SourceTraits::NativeAdsInstantiateType
  Frontend::adapt_native_ads_instantiate_type_(
    const std::string& inst_type_str)
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
    ostr << "unknown native ads instantiate type '"
         << inst_type_str
         << "'";
    throw Exception(ostr);
  }

  SourceTraits::ERIDReturnType
  Frontend::adapt_erid_return_type_(
    const std::string& erid_type_str)
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
    ostr << "unknown erid return type '"
        << erid_type_str
        << "'";
    throw Exception(ostr);
  }

  AdServer::CampaignSvcs::NativeAdsImpressionTrackerType
  Frontend::adapt_native_ads_impression_tracker_type_(
    const std::string& imp_type_str)
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
    ostr << "unknown native ads impression tracker type '"
         << imp_type_str
         << "'";
    throw Exception(ostr);
  }
} // namespace AdServer::Bidding::Grpc