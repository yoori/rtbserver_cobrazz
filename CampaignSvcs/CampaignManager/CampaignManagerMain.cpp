
#include <unistd.h>

#include <eh/Errno.hpp>
#include <eh/Exception.hpp>

#include <Commons/CorbaConfig.hpp>
#include <Commons/ErrorHandler.hpp>
#include <Commons/ConfigUtils.hpp>
#include <Commons/PathManip.hpp>

#include <LogCommons/ActionRequest.hpp>
#include <LogCommons/AdRequestLogger.hpp>
#include <LogCommons/CcgStat.hpp>
#include <LogCommons/ChannelHitStat.hpp>
#include <LogCommons/ChannelTriggerStat.hpp>
#include <LogCommons/CreativeStat.hpp>
#include <LogCommons/PassbackStat.hpp>
#include <LogCommons/Request.hpp>
#include <LogCommons/RequestBasicChannels.hpp>
#include <LogCommons/SearchTermStat.hpp>
#include <LogCommons/SearchEngineStat.hpp>
#include <LogCommons/TagAuctionStat.hpp>
#include <LogCommons/TagRequest.hpp>
#include <LogCommons/UserAgentStat.hpp>
#include <LogCommons/UserProperties.hpp>
#include <LogCommons/WebStat.hpp>
#include <LogCommons/ResearchWebStat.hpp>
#include <LogCommons/ResearchProfStat.hpp>
#include <LogCommons/TagPositionStat.hpp>
#include <CampaignSvcs/CampaignManagerConfig.hpp>
#include <Commons/GrpcService.hpp>
#include <Commons/LogReferrerUtils.hpp>

#include "CampaignManagerLogger.hpp"
#include "CampaignManagerMain.hpp"
#include "CampaignManager_service.cobrazz.pb.hpp"

namespace
{
  const char* ASPECT  = "CampaignManager";
  const char* CAMPAIGN_MANAGER_OBJ_KEY = "CampaignManager";
  const char* PROCESS_CONTROL_OBJ_KEY = "ProcessControl";

  const char* OUT_LOGS_DIR_NAME = "Out";
}

namespace DefaultValues
{
  unsigned int UC_FREQ_CAPS_LIFETIME = 60; // 60 seconds
}

namespace
{
  template<typename _T>
  _T gcd(_T first, _T second)
  {
    while (second != 0)
    {
      _T t = first % second;
      first = second;
      second = t;
    }
    return first;
  }
}

CampaignManagerApp_::CampaignManagerApp_() /*throw(eh::Exception)*/
  : AdServer::Commons::ProcessControlVarsLoggerImpl(
      "CampaignManagerApp_", ASPECT)
{
}

void
CampaignManagerApp_::shutdown(CORBA::Boolean wait_for_completion)
  /*throw(CORBA::SystemException)*/
{
  ShutdownWriteGuard guard(shutdown_lock_);

  if(campaign_manager_impl_.in() != 0)
  {
    campaign_manager_impl_->deactivate_object();
    campaign_manager_impl_->wait_object();
  }

  CORBACommons::ProcessControlImpl::shutdown(wait_for_completion);
}

CORBACommons::IProcessControl::ALIVE_STATUS
CampaignManagerApp_::is_alive() /*throw(CORBA::SystemException)*/
{
  return CORBACommons::ProcessControlImpl::is_alive();
}

bool
CampaignManagerApp_::is_ready_() noexcept
{
  bool status = false;

  try
  {
    if (campaign_manager_impl_.in() &&
        campaign_manager_impl_->ready())
    {
      status = true;
    }
  }
  catch(const eh::Exception& e)
  {
    logger()->sstream(Logging::Logger::EMERGENCY,
                      ASPECT,
                      "ADS-IMPL-167")
      << "CampaignManagerImpl::ready(): "
      << "Got eh:Exception. : "
      << e.what();
  }

  return status;
}

char* CampaignManagerApp_::comment() /*throw(CORBACommons::OutOfMemory)*/
{
  try
  {
    std::string res;
    if (campaign_manager_impl_.in())
    {
      campaign_manager_impl_->progress_comment(res);
    }
    CORBA::String_var r;
    r << res;
    return r._retn();
  }
  catch(const CORBA::Exception&)
  {
    throw CORBACommons::OutOfMemory();
  }
  catch(const eh::Exception& e)
  {
    throw CORBACommons::OutOfMemory();
  }
}

void
CampaignManagerApp_::main(int& argc, char** argv) noexcept
{
  using ComponentsBuilder = UServerUtils::ComponentsBuilder;
  using ManagerCoro = UServerUtils::Manager;
  using ManagerCoro_var = UServerUtils::Manager_var;
  using TaskProcessorContainer = UServerUtils::TaskProcessorContainer;
  using CampaignManagerImpl = AdServer::CampaignSvcs::CampaignManagerImpl;

  const char* stage = "beginning main()";

  try
  {
    if (!::setlocale(LC_CTYPE, "en_US.utf8"))
    {
      throw Exception("CampaignManagerApp_::main(): "
                      "cannot set locale.");
    }

    const char* usage = "usage: CampaignManager <config_file>";

    stage = "checking commond line parameters";

    if (argc < 2)
    {
      Stream::Error ostr;
      ostr << "config file is not specified\n" << usage;
      throw InvalidArgument(ostr);
    }

    stage = "reading config file";
    read_config(argv[1], argv[0]);

    stage = "init corba server adapter";
    try
    {
      // init CORBA Server
      corba_server_adapter_ =
        new CORBACommons::CorbaServerAdapter(corba_config_);

      shutdowner_ = corba_server_adapter_->shutdowner();
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << "SyncLogsApp_::init_corba(): "
        << "Can't init CorbaServerAdapter. : "
        << e.what();
      throw Exception(ostr);
    }

    stage = "Creating coroutine manager";
    auto task_processor_container_builder =
      Config::create_task_processor_container_builder(
        logger(),
        campaign_manager_config_->Coroutine());

    auto init_func = [this, &stage] (TaskProcessorContainer& task_processor_container)
    {
      auto& main_task_processor = task_processor_container.get_main_task_processor();
      auto grpc_server_builder = Config::create_grpc_server_builder(
        logger(),
        campaign_manager_config_->GrpcServer());
      const auto& grpc_scheduler = grpc_server_builder->scheduler();

      stage = "creating CampaignManager servant";

      AdServer::CampaignSvcs::CampaignManagerLogger_var
        campaign_manager_logger =
        new AdServer::CampaignSvcs::CampaignManagerLogger(
          configuration_.log_params, logger());

      campaign_manager_impl_ =
        new AdServer::CampaignSvcs::CampaignManagerImpl(
          main_task_processor,
          grpc_scheduler,
          *campaign_manager_config_,
          *domain_config_,
          callback(),
          logger(),
          campaign_manager_logger,
          configuration_.creative_instantiate,
          configuration_.campaigns_types.c_str());

      register_vars_controller();

      stage = "Initializing CORBA bindings";
      corba_server_adapter_->add_binding(
        CAMPAIGN_MANAGER_OBJ_KEY, campaign_manager_impl_.in());

      corba_server_adapter_->add_binding(
        PROCESS_CONTROL_OBJ_KEY, this);

      stage = "activating CampaignManagerImpl active object";
      campaign_manager_impl_->activate_object();

      auto get_campaign_creative_service = AdServer::Commons::create_grpc_service<
        AdServer::CampaignSvcs::Proto::CampaignManager_get_campaign_creative_Service,
        CampaignManagerImpl,
        &CampaignManagerImpl::get_campaign_creative>(
          logger(),
          campaign_manager_impl_.in());
      grpc_server_builder->add_service(
        get_campaign_creative_service.in(),
        main_task_processor);

      auto process_match_request_service = AdServer::Commons::create_grpc_service<
        AdServer::CampaignSvcs::Proto::CampaignManager_process_match_request_Service,
        CampaignManagerImpl,
        &CampaignManagerImpl::process_match_request>(
          logger(),
          campaign_manager_impl_.in());
      grpc_server_builder->add_service(
        process_match_request_service.in(),
        main_task_processor);

      auto match_geo_channels_service = AdServer::Commons::create_grpc_service<
        AdServer::CampaignSvcs::Proto::CampaignManager_match_geo_channels_Service,
        CampaignManagerImpl,
        &CampaignManagerImpl::match_geo_channels>(
          logger(),
          campaign_manager_impl_.in());
      grpc_server_builder->add_service(
        match_geo_channels_service.in(),
        main_task_processor);

      auto instantiate_ad_service = AdServer::Commons::create_grpc_service<
        AdServer::CampaignSvcs::Proto::CampaignManager_instantiate_ad_Service,
        CampaignManagerImpl,
        &CampaignManagerImpl::instantiate_ad>(
          logger(),
          campaign_manager_impl_.in());
      grpc_server_builder->add_service(
        instantiate_ad_service.in(),
        main_task_processor);

      auto get_channel_links_service = AdServer::Commons::create_grpc_service<
        AdServer::CampaignSvcs::Proto::CampaignManager_get_channel_links_Service,
        CampaignManagerImpl,
        &CampaignManagerImpl::get_channel_links>(
          logger(),
          campaign_manager_impl_.in());
      grpc_server_builder->add_service(
        get_channel_links_service.in(),
        main_task_processor);

      auto get_discover_channels_service = AdServer::Commons::create_grpc_service<
        AdServer::CampaignSvcs::Proto::CampaignManager_get_discover_channels_Service,
        CampaignManagerImpl,
        &CampaignManagerImpl::get_discover_channels>(
          logger(),
          campaign_manager_impl_.in());
      grpc_server_builder->add_service(
        get_discover_channels_service.in(),
        main_task_processor);

      auto get_category_channels_service = AdServer::Commons::create_grpc_service<
        AdServer::CampaignSvcs::Proto::CampaignManager_get_category_channels_Service,
        CampaignManagerImpl,
        &CampaignManagerImpl::get_category_channels>(
          logger(),
          campaign_manager_impl_.in());
      grpc_server_builder->add_service(
        get_category_channels_service.in(),
        main_task_processor);

      auto consider_passback_service = AdServer::Commons::create_grpc_service<
        AdServer::CampaignSvcs::Proto::CampaignManager_consider_passback_Service,
        CampaignManagerImpl,
        &CampaignManagerImpl::consider_passback>(
          logger(),
          campaign_manager_impl_.in());
      grpc_server_builder->add_service(
        consider_passback_service.in(),
        main_task_processor);

      auto consider_passback_track_service = AdServer::Commons::create_grpc_service<
        AdServer::CampaignSvcs::Proto::CampaignManager_consider_passback_track_Service,
        CampaignManagerImpl,
        &CampaignManagerImpl::consider_passback_track>(
          logger(),
          campaign_manager_impl_.in());
      grpc_server_builder->add_service(
        consider_passback_track_service.in(),
        main_task_processor);

      auto get_click_url_service = AdServer::Commons::create_grpc_service<
        AdServer::CampaignSvcs::Proto::CampaignManager_get_click_url_Service,
        CampaignManagerImpl,
        &CampaignManagerImpl::get_click_url>(
          logger(),
          campaign_manager_impl_.in());
      grpc_server_builder->add_service(
        get_click_url_service.in(),
        main_task_processor);

      auto verify_impression_service = AdServer::Commons::create_grpc_service<
        AdServer::CampaignSvcs::Proto::CampaignManager_verify_impression_Service,
        CampaignManagerImpl,
        &CampaignManagerImpl::verify_impression>(
          logger(),
          campaign_manager_impl_.in());
      grpc_server_builder->add_service(
        verify_impression_service.in(),
        main_task_processor);

      auto action_taken_service = AdServer::Commons::create_grpc_service<
        AdServer::CampaignSvcs::Proto::CampaignManager_action_taken_Service,
        CampaignManagerImpl,
        &CampaignManagerImpl::action_taken>(
          logger(),
          campaign_manager_impl_.in());
      grpc_server_builder->add_service(
        action_taken_service.in(),
        main_task_processor);

      auto verify_opt_operation_service = AdServer::Commons::create_grpc_service<
        AdServer::CampaignSvcs::Proto::CampaignManager_verify_opt_operation_Service,
        CampaignManagerImpl,
        &CampaignManagerImpl::verify_opt_operation>(
          logger(),
          campaign_manager_impl_.in());
      grpc_server_builder->add_service(
        verify_opt_operation_service.in(),
        main_task_processor);

      auto consider_web_operation_service = AdServer::Commons::create_grpc_service<
        AdServer::CampaignSvcs::Proto::CampaignManager_consider_web_operation_Service,
        CampaignManagerImpl,
        &CampaignManagerImpl::consider_web_operation>(
          logger(),
          campaign_manager_impl_.in());
      grpc_server_builder->add_service(
        consider_web_operation_service.in(),
        main_task_processor);

      auto get_config_service = AdServer::Commons::create_grpc_service<
        AdServer::CampaignSvcs::Proto::CampaignManager_get_config_Service,
        CampaignManagerImpl,
        &CampaignManagerImpl::get_config>(
          logger(),
          campaign_manager_impl_.in());
      grpc_server_builder->add_service(
        get_config_service.in(),
        main_task_processor);

      auto trace_campaign_selection_index_service = AdServer::Commons::create_grpc_service<
        AdServer::CampaignSvcs::Proto::CampaignManager_trace_campaign_selection_index_Service,
        CampaignManagerImpl,
        &CampaignManagerImpl::trace_campaign_selection_index>(
          logger(),
          campaign_manager_impl_.in());
      grpc_server_builder->add_service(
        trace_campaign_selection_index_service.in(),
        main_task_processor);

      auto trace_campaign_selection_service = AdServer::Commons::create_grpc_service<
        AdServer::CampaignSvcs::Proto::CampaignManager_trace_campaign_selection_Service,
        CampaignManagerImpl,
        &CampaignManagerImpl::trace_campaign_selection>(
          logger(),
          campaign_manager_impl_.in());
      grpc_server_builder->add_service(
        trace_campaign_selection_service.in(),
        main_task_processor);

      auto get_campaign_creative_by_ccid_service = AdServer::Commons::create_grpc_service<
        AdServer::CampaignSvcs::Proto::CampaignManager_get_campaign_creative_by_ccid_Service,
        CampaignManagerImpl,
        &CampaignManagerImpl::get_campaign_creative_by_ccid>(
          logger(),
          campaign_manager_impl_.in());
      grpc_server_builder->add_service(
        get_campaign_creative_by_ccid_service.in(),
        main_task_processor);

      auto get_colocation_flags_service = AdServer::Commons::create_grpc_service<
        AdServer::CampaignSvcs::Proto::CampaignManager_get_colocation_flags_Service,
        CampaignManagerImpl,
        &CampaignManagerImpl::get_colocation_flags>(
          logger(),
          campaign_manager_impl_.in());
      grpc_server_builder->add_service(
        get_colocation_flags_service.in(),
        main_task_processor);

      auto get_pub_pixels_service = AdServer::Commons::create_grpc_service<
        AdServer::CampaignSvcs::Proto::CampaignManager_get_pub_pixels_Service,
        CampaignManagerImpl,
        &CampaignManagerImpl::get_pub_pixels>(
          logger(),
          campaign_manager_impl_.in());
      grpc_server_builder->add_service(
        get_pub_pixels_service.in(),
        main_task_processor);

      auto process_anonymous_request_service = AdServer::Commons::create_grpc_service<
        AdServer::CampaignSvcs::Proto::CampaignManager_process_anonymous_request_Service,
        CampaignManagerImpl,
        &CampaignManagerImpl::process_anonymous_request>(
          logger(),
          campaign_manager_impl_.in());
      grpc_server_builder->add_service(
        process_anonymous_request_service.in(),
        main_task_processor);

      auto get_file_service = AdServer::Commons::create_grpc_service<
        AdServer::CampaignSvcs::Proto::CampaignManager_get_file_Service,
        CampaignManagerImpl,
        &CampaignManagerImpl::get_file>(
          logger(),
          campaign_manager_impl_.in());
      grpc_server_builder->add_service(
        get_file_service.in(),
        main_task_processor);

      auto components_builder = std::make_unique<ComponentsBuilder>();
      components_builder->add_grpc_cobrazz_server(
        std::move(grpc_server_builder));

      return components_builder;
    };

    ManagerCoro_var manager_coro(
      new ManagerCoro(
        std::move(task_processor_container_builder),
        std::move(init_func),
        logger()));

    stage = "activating coroutine manager";
    manager_coro->activate_object();

    stage = "running orb loop";
    logger()->sstream(Logging::Logger::NOTICE, ASPECT)
      << "service started.";
    corba_server_adapter_->run();

    stage =
      "waiting while CORBACommons::ProcessControlImpl completes it's tasks";

    wait();

    logger()->sstream(Logging::Logger::NOTICE, ASPECT)
      << "service stopped.";

    manager_coro->deactivate_object();
    manager_coro->wait_object();

    campaign_manager_impl_.reset();
  }
  catch (const Exception& e)
  {
    logger()->sstream(Logging::Logger::CRITICAL,
                      ASPECT,
                      "ADS-IMPL-168")
      << "CampaignManagerApp_::main(): "
      << "Got CampaignManagerApp_::Exception on "
      << stage << " stage. : \n"
      << e.what();
  }
  catch (const eh::Exception& e)
  {
    logger()->sstream(Logging::Logger::EMERGENCY,
                      ASPECT,
                      "ADS-IMPL-168")
      << "CampaignManagerApp_::main(): "
      << "Got eh::Exception on "
      << stage << " stage. : \n"
      << e.what();
  }
  catch (...)
  {
    logger()->log(String::SubString("CampaignManagerApp_::main(): "
                  "Got Unknown exception. "),
                  Logging::Logger::EMERGENCY,
                  ASPECT,
                  "ADS-IMPL-168");
  }
}

void
CampaignManagerApp_::read_logger_config(
  const char *log_dir_name,
  const xsd::AdServer::Configuration::CampaignManagerLoggerType&
    xsd_logger,
  AdServer::LogProcessing::LogFlushTraits& logger_params)
  /*throw(Exception, eh::Exception)*/
{
  logger_params.out_dir = configuration_.out_logs_dir;
  AdServer::PathManip::create_path(logger_params.out_dir, log_dir_name);

  logger_params.period = Generics::Time(
    xsd_logger.flush_period().present() ?
    xsd_logger.flush_period().get(): 0);

  std::optional<AdServer::LogProcessing::ArchiveParams> archive_params;
  const auto archive = xsd_logger.archive();
  if (archive.present())
  {
    switch (*archive)
    {
      case xsd::AdServer::Configuration::ArchiveType::value::no_compression:
        break;
      case xsd::AdServer::Configuration::ArchiveType::value::gzip_default:
        archive_params = AdServer::LogProcessing::Archive::gzip_default_compression_params;
        break;
      case xsd::AdServer::Configuration::ArchiveType::value::gzip_best_compression:
        archive_params = AdServer::LogProcessing::Archive::gzip_best_compression_params;
        break;
      case xsd::AdServer::Configuration::ArchiveType::value::gzip_best_speed:
        archive_params = AdServer::LogProcessing::Archive::gzip_best_speed_params;
        break;
      case xsd::AdServer::Configuration::ArchiveType::value::bz2_default:
        archive_params = AdServer::LogProcessing::Archive::bzip2_default_compression_params;
        break;
      default:
      {
        Stream::Error stream;
        stream << FNS
               << "Unknown archive type";
        throw Exception(stream);
      }
    }
  }

  logger_params.archive_params = archive_params;
}

void
CampaignManagerApp_::read_creative_config(
  AdServer::CampaignSvcs::CampaignManagerImpl::CreativeInstantiate&
    creative_instantiate,
  const xsd::AdServer::Configuration::CampaignManagerCreative&
    xsd_creative_description)
  /*throw(Exception, eh::Exception)*/
{
  /* using xsd namespace */
  using namespace xsd::AdServer::Configuration;

  long cur_option_id = -100000;
  for(auto it = xsd_creative_description.CreativeRule().begin();
    it != xsd_creative_description.CreativeRule().end();
    ++it)
  {
    std::string rule_name;
    AdServer::CampaignSvcs::CreativeInstantiateRule rule;
    read_creative_rule_config(cur_option_id, rule_name, rule, *it);
    creative_instantiate.creative_rules[rule_name] = rule;
  }

  for(auto it = xsd_creative_description.SourceRule().begin();
     it != xsd_creative_description.SourceRule().end();
     ++it)
  {
    AdServer::CampaignSvcs::CampaignManagerImpl::
      CreativeInstantiate::SourceRule source_rule;

    if(it->click_prefix().present())
    {
      source_rule.click_prefix = *(it->click_prefix());
    }

    if(it->mime_encoded_click_prefix().present())
    {
      source_rule.mime_encoded_click_prefix = *(it->mime_encoded_click_prefix());
    }

    if(it->preclick().present())
    {
      source_rule.preclick = *(it->preclick());
    }

    if(it->mime_encoded_preclick().present())
    {
      source_rule.mime_encoded_preclick = *(it->mime_encoded_preclick());
    }

    if(it->vast_preclick().present())
    {
      source_rule.vast_preclick = *(it->vast_preclick());
    }

    if(it->mime_encoded_vast_preclick().present())
    {
      source_rule.mime_encoded_vast_preclick = *(it->mime_encoded_vast_preclick());
    }

    creative_instantiate.source_rules[it->name()] = source_rule;
  }
}

void
CampaignManagerApp_::read_creative_rule_config(
  long& cur_option_id,
  std::string& name,
  AdServer::CampaignSvcs::CreativeInstantiateRule& rule,
  const xsd::AdServer::Configuration::CampaignManagerCreativeRuleType&
    xsd_creative_rule)
  /*throw(Exception, eh::Exception)*/
{
  name = xsd_creative_rule.name();
  if(xsd_creative_rule.secure())
  {
    rule.url_prefix = HTTP::HTTPS_PREFIX.str.str();
  }
  else
  {
    rule.url_prefix = HTTP::HTTP_PREFIX.str.str();
  }
  rule.image_url = xsd_creative_rule.image_url();
  rule.publ_url = xsd_creative_rule.publ_url();
  rule.click_url = xsd_creative_rule.ad_click_url();
  rule.ad_server = xsd_creative_rule.ad_server();
  rule.ad_image_server = xsd_creative_rule.ad_image_server();
  rule.track_pixel_url = xsd_creative_rule.track_pixel_url();
  rule.notice_url = xsd_creative_rule.notice_url();
  rule.action_pixel_url = xsd_creative_rule.action_pixel_url();
  rule.local_passback_prefix = xsd_creative_rule.local_passback_prefix();
  rule.dynamic_creative_prefix = xsd_creative_rule.dynamic_creative_prefix();
  rule.passback_template_path_prefix =
    xsd_creative_rule.passback_template_path_prefix();
  rule.passback_pixel_url = xsd_creative_rule.passback_pixel_url();
  if(xsd_creative_rule.user_bind_url().present())
  {
    rule.user_bind_url = *xsd_creative_rule.user_bind_url();
  }
  rule.pub_pixels_optin = xsd_creative_rule.pub_pixels_optin();
  rule.pub_pixels_optout = xsd_creative_rule.pub_pixels_optout();
  rule.script_instantiate_url = xsd_creative_rule.script_instantiate_url();
  rule.iframe_instantiate_url = xsd_creative_rule.iframe_instantiate_url();
  rule.direct_instantiate_url = xsd_creative_rule.direct_instantiate_url();
  rule.nonsecure_direct_instantiate_url = xsd_creative_rule.nonsecure_direct_instantiate_url();
  rule.video_instantiate_url = xsd_creative_rule.video_instantiate_url();
  rule.nonsecure_video_instantiate_url = xsd_creative_rule.nonsecure_video_instantiate_url();

  for(xsd::AdServer::Configuration::CampaignManagerCreativeRuleType::
        Token_sequence::const_iterator tok_it =
          xsd_creative_rule.Token().begin();
      tok_it != xsd_creative_rule.Token().end(); ++tok_it)
  {
    rule.tokens[tok_it->name()] = AdServer::CampaignSvcs::OptionValue(
      ++cur_option_id, tok_it->value());
  }
}

void
CampaignManagerApp_::read_logging_config(
  const xsd::AdServer::Configuration::CampaignManagerLoggingType& config,
  AdServer::CampaignSvcs::CampaignManagerLogger::Params& log_params)
  /*throw(Exception)*/
{
  if (config.ChannelTriggerStat().present())
  {
    read_logger_config(
      AdServer::LogProcessing::ChannelTriggerStatTraits::log_base_name(),
      config.ChannelTriggerStat().get(),
      log_params.channel_trigger_stat);
  }

  if (config.ChannelHitStat().present())
  {
    read_logger_config(
      AdServer::LogProcessing::ChannelHitStatTraits::log_base_name(),
      config.ChannelHitStat().get(),
      log_params.channel_hit_stat);
  }

  if (config.RequestBasicChannels().present())
  {
    read_logger_config(
      AdServer::LogProcessing::RequestBasicChannelsTraits::log_base_name(),
      config.RequestBasicChannels().get(),
      log_params.request_basic_channels);

    log_params.request_basic_channels.inventory_users_percentage =
      config.inventory_users_percentage();

    log_params.request_basic_channels.distrib_count =
      config.distrib_count();

    log_params.request_basic_channels.dump_channel_triggers =
      config.RequestBasicChannels()->dump_channel_triggers();

    log_params.request_basic_channels.adrequest_anonymize =
      config.RequestBasicChannels()->adrequest_anonymize();
  }

  if (config.WebStat().present())
  {
    read_logger_config(
      AdServer::LogProcessing::WebStatTraits::log_base_name(),
      config.WebStat().get(),
      log_params.web_stat);
  }

  if (config.ResearchWebStat().present())
  {
    read_logger_config(
      AdServer::LogProcessing::ResearchWebStatTraits::log_base_name(),
      config.ResearchWebStat().get(),
      log_params.research_web_stat);
  }

  if (config.CreativeStat().present())
  {
    read_logger_config(
      AdServer::LogProcessing::CreativeStatTraits::log_base_name(),
      config.CreativeStat().get(),
      log_params.creative_stat);
  }

  if (config.ActionRequest().present())
  {
    read_logger_config(
      AdServer::LogProcessing::ActionRequestTraits::log_base_name(),
      config.ActionRequest().get(),
      log_params.action_request);
  }

  if (config.PassbackStat().present())
  {
    read_logger_config(
      AdServer::LogProcessing::PassbackStatTraits::log_base_name(),
      config.PassbackStat().get(),
      log_params.passback_stat);
  }

  if (config.UserAgentStat().present())
  {
    read_logger_config(
      AdServer::LogProcessing::UserAgentStatTraits::log_base_name(),
      config.UserAgentStat().get(),
      log_params.user_agent_stat);
  }

  if (config.ProfilingResearch().present())
  {
    read_logger_config(
      AdServer::LogProcessing::ResearchProfTraits::log_base_name(),
      config.ProfilingResearch().get(),
      log_params.prof_research);
  }

  log_params.profiling_research_record_limit = config.profiling_research_record_limit();
  log_params.profiling_log_sampling = config.profiling_log_sampling();

  if (config.Request().present())
  {
    read_logger_config(
      AdServer::LogProcessing::RequestTraits::log_base_name(),
      config.Request().get(),
      log_params.request);
  }

  if (config.Impression().present())
  {
    read_logger_config(
      AdServer::LogProcessing::ImpressionTraits::log_base_name(),
      config.Impression().get(),
      log_params.impression);
  }

  if (config.Click().present())
  {
    read_logger_config(
      AdServer::LogProcessing::ClickTraits::log_base_name(),
      config.Click().get(),
      log_params.click);
  }

  if (config.AdvertiserAction().present())
  {
    read_logger_config(
      AdServer::LogProcessing::AdvertiserActionTraits::log_base_name(),
      config.AdvertiserAction().get(),
      log_params.advertiser_action);
  }

  if (config.PassbackImpression().present())
  {
    read_logger_config(
      AdServer::LogProcessing::PassbackImpressionTraits::log_base_name(),
      config.PassbackImpression().get(),
      log_params.passback_impression);
  }

  if (config.UserProperties().present())
  {
    read_logger_config(
      AdServer::LogProcessing::UserPropertiesTraits::log_base_name(),
      config.UserProperties().get(),
      log_params.user_properties);
  }

  if (config.TagRequest().present())
  {
    read_logger_config(
      AdServer::LogProcessing::TagRequestTraits::log_base_name(),
      config.TagRequest().get(),
      log_params.tag_request);
  }

  if (config.TagPositionStat().present())
  {
    read_logger_config(
      AdServer::LogProcessing::TagPositionStatTraits::log_base_name(),
      config.TagPositionStat().get(),
      log_params.tag_position_stat);
  }

  if (config.CcgStat().present())
  {
    read_logger_config(
      AdServer::LogProcessing::CcgStatTraits::log_base_name(),
      config.CcgStat().get(),
      log_params.ccg_stat);
  }

  if (config.CcStat().present())
  {
    read_logger_config(
      AdServer::LogProcessing::CcStatTraits::log_base_name(),
      config.CcStat().get(),
      log_params.cc_stat);
  }

  if (config.SearchTermStat().present())
  {
    read_logger_config(
      AdServer::LogProcessing::SearchTermStatTraits::log_base_name(),
      config.SearchTermStat().get(),
      log_params.search_term_stat);
  }

  if (config.SearchEngineStat().present())
  {
    read_logger_config(
      AdServer::LogProcessing::SearchEngineStatTraits::log_base_name(),
      config.SearchEngineStat().get(),
      log_params.search_engine_stat);
  }

  if (config.TagAuctionStat().present())
  {
    read_logger_config(
      AdServer::LogProcessing::TagAuctionStatTraits::log_base_name(),
      config.TagAuctionStat().get(),
      log_params.tag_auction_stat);
  }

  log_params.log_referrer_setting = 
      AdServer::Commons::LogReferrer::read_log_referrer_settings(
        config.use_referrer_site_referrer_stats());
}

xsd::AdServer::Configuration::CampaignManagerType
CampaignManagerApp_::read_config(const char* filename, const char* argv0)
  /*throw(Exception, eh::Exception)*/
{
  Config::ErrorHandler error_handler;

  try
  {
    /* using xsd namespace */
    using namespace xsd::AdServer::Configuration;

    std::unique_ptr<AdConfigurationType> ad_configuration;

    try
    {
      ad_configuration = AdConfiguration(filename, error_handler);

      if(error_handler.has_errors())
      {
        std::string error_string;
        throw Exception(error_handler.text(error_string));
      }

      campaign_manager_config_ =
        ConfigPtr(
          new AdServer::CampaignSvcs::
          CampaignManagerImpl::CampaignManagerConfig(
            ad_configuration->CampaignManager()));
    }
    catch(const xml_schema::parsing& e)
    {
      Stream::Error ostr;
      ostr << "Can't parse config file '" << filename << "': ";
      if(error_handler.has_errors())
      {
        std::string error_string;
        ostr << error_handler.text(error_string);
      }

      throw Exception(ostr);
    }

    std::string domain_config_path = campaign_manager_config_->domain_config_path();

    try
    {
      domain_config_ = DomainConfiguration(
        domain_config_path.c_str(), error_handler);

      if(error_handler.has_errors())
      {
        std::string error_string;
        throw Exception(error_handler.text(error_string));
      }
    }
    catch(const xml_schema::parsing& e)
    {
      Stream::Error ostr;
      ostr << "Can't parse config file '" << domain_config_path << "': ";
      if(error_handler.has_errors())
      {
        std::string error_string;
        ostr << error_handler.text(error_string);
      }

      throw Exception(ostr);
    }

    CampaignManagerType* configuration = campaign_manager_config_.get();

    configuration_.log_root = configuration->log_root();
    if (configuration_.log_root[0] != '/')
    {
      Stream::Error ostr;
      ostr << "CampaignManagerApp_::read_config(): "
        << "log_root should have '/' on the first position.";
      throw Exception(ostr);
    }

    if (configuration->output_logs_dir().present())
    {
      configuration_.out_logs_dir = configuration->output_logs_dir().get();
      if (configuration_.out_logs_dir[0] != '/')
      {
        Stream::Error ostr;
        ostr << "CampaignManagerApp_::read_config(): "
          << "output_logs_dir should have '/' on the first position.";
        throw Exception(ostr);
      }
    }
    else
    {
      configuration_.out_logs_dir = configuration_.log_root;
      AdServer::PathManip::create_path(
        configuration_.out_logs_dir,
        OUT_LOGS_DIR_NAME);
    }

    configuration_.uc_freq_caps_lifetime =
      configuration->uc_freq_caps_lifetime().present() ?
      configuration->uc_freq_caps_lifetime().get() :
      DefaultValues::UC_FREQ_CAPS_LIFETIME;

    if(configuration->campaigns_type().present())
    {
      std::string sval = configuration->campaigns_type().get();
      if(sval == "active")
      {
        configuration_.campaigns_types = "A";
      }
      else if(sval == "virtual")
      {
        configuration_.campaigns_types = "AV";
      }
    }
    else
    {
      configuration_.campaigns_types = "A"; /* default value */
    }

    // init logger
    try
    {
      logger(Config::LoggerConfigReader::create(
        configuration->Logger(), argv0));
    }
    catch (const Config::LoggerConfigReader::Exception& e)
    {
      Stream::Error ostr;
      ostr << "CampaignManagerApp_::read_config(): "
        << "got LoggerConfigReader::Exception: "
        << e.what();
      throw Exception(ostr);
    }

    // Fill corba_config
    try
    {
      Config::CorbaConfigReader::read_config(
        configuration->CorbaConfig(),
        corba_config_);
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << "CampaignManagerApp_::read_config: "
        << "Can't read Corba Config. : "
        << e.what();
      throw Exception(ostr);
    }

    read_creative_config(
      configuration_.creative_instantiate,
      configuration->Creative());

    read_logging_config(
      configuration->Logging(),
      configuration_.log_params);

    return *configuration;
  }
  catch (const eh::Exception& e)
  {
    Stream::Error ostr;
    ostr << "CampaignManagerApp_::read_config(): "
      << "got eh::Exception. "
      << "Invalid CampaignManager configuration. : \n"
      << e.what();
    throw Exception(ostr);
  }
}

int
main(int argc, char** argv)
{
  CampaignManagerApp_* app = 0;

  try
  {
    app = &CampaignManagerApp::instance();
  }
  catch (...)
  {
    std::cerr << "main(): Critical: Got exception while "
      "creating application object.\n";
    return -1;
  }

  if (app == 0)
  {
    std::cerr << "main(): Critical: got NULL application object.\n";
    return -1;
  }

  app->main(argc, argv);
}

