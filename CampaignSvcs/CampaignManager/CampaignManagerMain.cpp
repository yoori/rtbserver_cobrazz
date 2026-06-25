
#include <unistd.h>

#include <eh/Errno.hpp>
#include <eh/Exception.hpp>

#include <Logger/StreamLogger.hpp>

#include <Commons/ErrorHandler.hpp>
#include <Commons/ConfigUtils.hpp>
#include <Commons/PathManip.hpp>
#include <Commons/PidFileGuard.hpp>
#include <Commons/ScopeGuard.hpp>
#include <Commons/SignalActiveObject.hpp>

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
#include <Commons/LogReferrerUtils.hpp>

#include "CampaignManagerLogger.hpp"
#include "CampaignManagerMain.hpp"

namespace
{
  const char* ASPECT  = "CampaignManager";

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

  std::shared_ptr<Generics::ActiveObject>
  non_owning_active_object(Generics::ActiveObject* object)
  {
    return std::shared_ptr<Generics::ActiveObject>(
      object,
      [](Generics::ActiveObject*) {});
  }

  std::string
  sanitize_endpoint_key(const std::string& endpoint)
  {
    std::string result;
    result.reserve(endpoint.size());
    for(const char ch : endpoint)
    {
      if((ch >= 'a' && ch <= 'z') ||
        (ch >= 'A' && ch <= 'Z') ||
        (ch >= '0' && ch <= '9'))
      {
        result += ch;
      }
      else
      {
        result += '_';
      }
    }

    return result.empty() ? std::string("unknown") : result;
  }

  void
  append_json_string(std::string& body, const std::string& value)
  {
    static const char HEX[] = "0123456789abcdef";

    body += '"';
    for(const unsigned char ch : value)
    {
      switch(ch)
      {
      case '"':
        body += "\\\"";
        break;
      case '\\':
        body += "\\\\";
        break;
      case '\b':
        body += "\\b";
        break;
      case '\f':
        body += "\\f";
        break;
      case '\n':
        body += "\\n";
        break;
      case '\r':
        body += "\\r";
        break;
      case '\t':
        body += "\\t";
        break;
      default:
        if(ch < 0x20)
        {
          body += "\\u00";
          body += HEX[(ch >> 4) & 0x0f];
          body += HEX[ch & 0x0f];
        }
        else
        {
          body += static_cast<char>(ch);
        }
        break;
      }
    }
    body += '"';
  }

  void
  append_json_field_name(
    std::string& body,
    bool& first,
    const std::string& name)
  {
    if(!first)
    {
      body += ',';
    }
    first = false;
    append_json_string(body, name);
    body += ':';
  }

  void
  append_json_stat(
    std::string& body,
    bool& first,
    const std::string& name,
    std::uint64_t value)
  {
    append_json_field_name(body, first, name);
    body += std::to_string(value);
  }

  void
  append_json_string_stat(
    std::string& body,
    bool& first,
    const std::string& name,
    const std::string& value)
  {
    append_json_field_name(body, first, name);
    append_json_string(body, value);
  }

  void
  append_grpc_client_stats(
    std::string& body,
    bool& first,
    const std::string& prefix,
    const AdServer::Grpc::Stats& stats)
  {
    append_json_stat(body, first, prefix + "_input_items", stats.input_items);
    append_json_stat(body, first, prefix + "_call_total", stats.input_items);
    append_json_stat(
      body,
      first,
      prefix + "_completed_items",
      stats.completed_items);
    append_json_stat(
      body,
      first,
      prefix + "_completed_error_items",
      stats.completed_error_items);
    append_json_stat(
      body,
      first,
      prefix + "_call_error_total",
      stats.completed_error_items);
    append_json_stat(
      body,
      first,
      prefix + "_outstanding_items",
      stats.input_items > stats.completed_items ?
        stats.input_items - stats.completed_items :
        0);
    append_json_stat(
      body,
      first,
      prefix + "_write_batches",
      stats.write_batches);
    append_json_stat(body, first, prefix + "_batch_total", stats.write_batches);
    append_json_stat(
      body,
      first,
      prefix + "_write_batch_total",
      stats.write_batches);
    append_json_stat(body, first, prefix + "_write_items", stats.write_items);
    append_json_stat(body, first, prefix + "_read_batches", stats.read_batches);
    append_json_stat(
      body,
      first,
      prefix + "_read_batch_total",
      stats.read_batches);
    append_json_stat(body, first, prefix + "_read_items", stats.read_items);
    append_json_stat(
      body,
      first,
      prefix + "_queue_wait_total",
      stats.queue_wait_count);
    append_json_stat(
      body,
      first,
      prefix + "_queue_wait_time",
      stats.queue_wait_sum_us);
    append_json_stat(
      body,
      first,
      prefix + "_queue_wait_max_time",
      stats.queue_wait_max_us);
    append_json_stat(
      body,
      first,
      prefix + "_queue_timeout_total",
      stats.queue_timeout_count);
    append_json_stat(
      body,
      first,
      prefix + "_response_wait_total",
      stats.response_wait_count);
    append_json_stat(
      body,
      first,
      prefix + "_response_wait_time",
      stats.response_wait_sum_us);
    append_json_stat(
      body,
      first,
      prefix + "_response_wait_max_time",
      stats.response_wait_max_us);
    append_json_stat(body, first, prefix + "_queue_items", stats.queue_items);
    append_json_stat(
      body,
      first,
      prefix + "_pending_batches",
      stats.pending_batches);
    append_json_stat(
      body,
      first,
      prefix + "_pending_batch_items",
      stats.pending_batch_items);
    append_json_stat(
      body,
      first,
      prefix + "_inflight_items",
      stats.inflight_items);
    append_json_stat(
      body,
      first,
      prefix + "_stream_inflight_items",
      stats.stream_inflight_items);
    append_json_stat(
      body,
      first,
      prefix + "_active_streams",
      stats.active_streams);
    append_json_stat(
      body,
      first,
      prefix + "_available_streams",
      stats.available_streams);
    append_json_stat(
      body,
      first,
      prefix + "_connecting_streams",
      stats.connecting_streams);
    append_json_stat(
      body,
      first,
      prefix + "_draining_streams",
      stats.draining_streams);
    append_json_stat(
      body,
      first,
      prefix + "_deferred_streams",
      stats.deferred_streams);

    if(stats.consumer_stream_write.has_value())
    {
      append_json_stat(
        body,
        first,
        prefix + "_consumer_stream_write_total",
        stats.consumer_stream_write->count);
      append_json_stat(
        body,
        first,
        prefix + "_consumer_stream_write_time",
        stats.consumer_stream_write->sum_us);
      append_json_stat(
        body,
        first,
        prefix + "_consumer_stream_write_max_time",
        stats.consumer_stream_write->max_us);
    }

    if(stats.last_error.has_value())
    {
      append_json_string_stat(
        body,
        first,
        prefix + "_last_error_time",
        stats.last_error->time.get_gm_time().format("%F %T"));
      append_json_string_stat(
        body,
        first,
        prefix + "_last_error_endpoint",
        stats.last_error->endpoint);
      append_json_stat(
        body,
        first,
        prefix + "_last_error_code",
        static_cast<std::uint64_t>(stats.last_error->code));
      append_json_string_stat(
        body,
        first,
        prefix + "_last_error_message",
        stats.last_error->message);
      append_json_string_stat(
        body,
        first,
        prefix + "_last_error_source",
        stats.last_error->source);
    }
  }

  void
  append_grpc_client_endpoint_stats(
    std::string& body,
    bool& first,
    const std::string& prefix,
    const AdServer::Grpc::Client::EndpointStats& endpoint_stats)
  {
    for(const auto& [endpoint, stats] : endpoint_stats)
    {
      const auto endpoint_prefix =
        prefix + "_endpoints_" + sanitize_endpoint_key(endpoint);
      append_json_string_stat(
        body,
        first,
        endpoint_prefix + "_endpoint",
        endpoint);
      append_grpc_client_stats(body, first, endpoint_prefix, stats);
    }
  }

  void
  append_campaign_manager_stats(
    std::string& body,
    bool& first,
    const AdServer::CampaignSvcs::CampaignManagerGrpc* grpc_adapter,
    const AdServer::CampaignSvcs::CampaignManagerCore* campaign_manager_core,
    const AdServer::CampaignSvcs::CampaignManagerLogger_var& campaign_manager_logger)
  {
    auto append_stat = [&body, &first](
      const std::string& name,
      std::uint64_t value)
    {
      append_json_stat(body, first, name, value);
    };

    if(grpc_adapter != 0)
    {
      const auto stats = grpc_adapter->stats();

      append_stat("call_in_progress", stats.call_in_progress);
      append_stat("call_total", stats.call_total);
      append_stat("call_time", stats.call_time);
      append_stat("ready_in_progress", stats.ready_in_progress);
      append_stat(
        "progress_comment_in_progress",
        stats.progress_comment_in_progress);
      append_stat(
        "match_geo_channels_in_progress",
        stats.match_geo_channels_in_progress);
      append_stat("get_file_in_progress", stats.get_file_in_progress);
      append_stat(
        "get_campaign_creative_in_progress",
        stats.get_campaign_creative_in_progress);
      append_stat(
        "process_match_request_in_progress",
        stats.process_match_request_in_progress);
      append_stat(
        "process_anonymous_request_in_progress",
        stats.process_anonymous_request_in_progress);
      append_stat(
        "instantiate_ad_in_progress",
        stats.instantiate_ad_in_progress);
      append_stat(
        "trace_campaign_selection_index_in_progress",
        stats.trace_campaign_selection_index_in_progress);
      append_stat(
        "trace_campaign_selection_in_progress",
        stats.trace_campaign_selection_in_progress);
      append_stat(
        "get_campaign_creative_by_ccid_in_progress",
        stats.get_campaign_creative_by_ccid_in_progress);
      append_stat(
        "get_channel_links_in_progress",
        stats.get_channel_links_in_progress);
      append_stat(
        "get_discover_channels_in_progress",
        stats.get_discover_channels_in_progress);
      append_stat(
        "get_category_channels_in_progress",
        stats.get_category_channels_in_progress);
      append_stat(
        "get_colocation_flags_in_progress",
        stats.get_colocation_flags_in_progress);
      append_stat(
        "get_pub_pixels_in_progress",
        stats.get_pub_pixels_in_progress);
      append_stat(
        "consider_passback_in_progress",
        stats.consider_passback_in_progress);
      append_stat(
        "consider_passback_track_in_progress",
        stats.consider_passback_track_in_progress);
      append_stat(
        "get_click_url_in_progress",
        stats.get_click_url_in_progress);
      append_stat(
        "verify_impression_in_progress",
        stats.verify_impression_in_progress);
      append_stat(
        "action_taken_in_progress",
        stats.action_taken_in_progress);
      append_stat(
        "verify_opt_operation_in_progress",
        stats.verify_opt_operation_in_progress);
      append_stat(
        "consider_web_operation_in_progress",
        stats.consider_web_operation_in_progress);
      append_stat(
        "get_config_in_progress",
        stats.get_config_in_progress);

#define APPEND_RPC_TOTAL_TIME_(name) \
      append_stat(#name "_total", stats.name##_total); \
      append_stat(#name "_time", stats.name##_time)

      APPEND_RPC_TOTAL_TIME_(ready);
      APPEND_RPC_TOTAL_TIME_(progress_comment);
      APPEND_RPC_TOTAL_TIME_(match_geo_channels);
      APPEND_RPC_TOTAL_TIME_(get_file);
      APPEND_RPC_TOTAL_TIME_(get_campaign_creative);
      APPEND_RPC_TOTAL_TIME_(process_match_request);
      APPEND_RPC_TOTAL_TIME_(process_anonymous_request);
      APPEND_RPC_TOTAL_TIME_(instantiate_ad);
      APPEND_RPC_TOTAL_TIME_(trace_campaign_selection_index);
      APPEND_RPC_TOTAL_TIME_(trace_campaign_selection);
      APPEND_RPC_TOTAL_TIME_(get_campaign_creative_by_ccid);
      APPEND_RPC_TOTAL_TIME_(get_channel_links);
      APPEND_RPC_TOTAL_TIME_(get_discover_channels);
      APPEND_RPC_TOTAL_TIME_(get_category_channels);
      APPEND_RPC_TOTAL_TIME_(get_colocation_flags);
      APPEND_RPC_TOTAL_TIME_(get_pub_pixels);
      APPEND_RPC_TOTAL_TIME_(consider_passback);
      APPEND_RPC_TOTAL_TIME_(consider_passback_track);
      APPEND_RPC_TOTAL_TIME_(get_click_url);
      APPEND_RPC_TOTAL_TIME_(verify_impression);
      APPEND_RPC_TOTAL_TIME_(action_taken);
      APPEND_RPC_TOTAL_TIME_(verify_opt_operation);
      APPEND_RPC_TOTAL_TIME_(consider_web_operation);
      APPEND_RPC_TOTAL_TIME_(get_config);

#undef APPEND_RPC_TOTAL_TIME_
      const auto& lifecycle_stats = stats.grpc_lifecycle_stats;
      append_stat(
        "grpc_unary_call_created_total",
        lifecycle_stats.unary_call_created_total);
      append_stat(
        "grpc_unary_call_deleted_total",
        lifecycle_stats.unary_call_deleted_total);
      append_stat(
        "grpc_unary_call_live",
        lifecycle_stats.unary_call_live);
      append_stat(
        "grpc_coro_unary_call_created_total",
        lifecycle_stats.coro_unary_call_created_total);
      append_stat(
        "grpc_coro_unary_call_deleted_total",
        lifecycle_stats.coro_unary_call_deleted_total);
      append_stat(
        "grpc_coro_unary_call_live",
        lifecycle_stats.coro_unary_call_live);
      append_stat(
        "grpc_batch_stream_call_created_total",
        lifecycle_stats.batch_stream_call_created_total);
      append_stat(
        "grpc_batch_stream_call_deleted_total",
        lifecycle_stats.batch_stream_call_deleted_total);
      append_stat(
        "grpc_batch_stream_call_live",
        lifecycle_stats.batch_stream_call_live);
      append_stat(
        "grpc_debug_watchdog_scheduled_total",
        lifecycle_stats.debug_watchdog_scheduled_total);
      append_stat(
        "grpc_debug_watchdog_finished_total",
        lifecycle_stats.debug_watchdog_finished_total);
      append_stat(
        "grpc_debug_watchdog_live",
        lifecycle_stats.debug_watchdog_live);
    }

    const auto logger_stats = campaign_manager_logger->get_stats();
    append_stat("logging_request_in_progress", logger_stats.request_in_progress);
    append_stat("logging_queue_size", logger_stats.queue_size);
    append_stat("logging_queue_total", logger_stats.queue_total);
    append_stat(
      "logging_processing_requests",
      logger_stats.processing_requests);
    append_stat(
      "logging_processing_requests_total",
      logger_stats.processing_requests_total);

    if(campaign_manager_core != 0)
    {
      const auto billing_stats = campaign_manager_core->billing_server_stats();
      append_grpc_client_stats(
        body,
        first,
        "billing_server_client",
        billing_stats.total);
      append_grpc_client_endpoint_stats(
        body,
        first,
        "billing_server_client",
        billing_stats.endpoints);
    }
  }
}

CampaignManagerApp_::CampaignManagerApp_() /*throw(eh::Exception)*/
  : Logging::LoggerCallbackHolder(
      Logging::Logger_var(new Logging::OStream::Logger(
        Logging::OStream::Config(std::cerr))),
      "CampaignManagerApp_", ASPECT, 0)
{
}

void
CampaignManagerApp_::main(int& argc, char** argv) noexcept
{
  const char* stage = "beginning main()";
  std::unique_ptr<AdServer::Commons::PidFileGuard> pid_file_guard;

  try
  {
    if (!::setlocale(LC_CTYPE, "en_US.utf8"))
    {
      throw Exception("CampaignManagerApp_::main(): cannot set locale.");
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

    stage = "creating CampaignManager core";

    AdServer::CampaignSvcs::CampaignManagerLogger_var
      campaign_manager_logger =
        new AdServer::CampaignSvcs::CampaignManagerLogger(
          configuration_.log_params, logger());

    AdServer::CampaignSvcs::CampaignManagerCore_var campaign_manager_core =
      new AdServer::CampaignSvcs::CampaignManagerCore(
        *campaign_manager_config_,
        *domain_config_,
        callback(),
        logger(),
        campaign_manager_logger,
        configuration_.creative_instantiate,
        configuration_.campaigns_types.c_str());

    pid_file_guard = std::make_unique<AdServer::Commons::PidFileGuard>(
      configuration_.pid_file);

    AdServer::CampaignSvcs::CampaignManagerGrpc_var grpc_adapter;
    auto active_objects =
      std::make_shared<Generics::CompositeActiveObject>(false, false);
    auto active_objects_shutdown_guard = AdServer::Commons::make_scope_guard(
      [&]() noexcept
      {
        if(active_objects->active())
        {
          active_objects->deactivate_object();
          active_objects->wait_object();
        }
      }
    );

    active_objects->add_child_object(campaign_manager_logger.in());
    active_objects->add_child_object(campaign_manager_core.in());

    if(campaign_manager_config_->GrpcConfig().present())
    {
      stage = "creating CampaignManagerGrpc";
      grpc_adapter = new AdServer::CampaignSvcs::CampaignManagerGrpc(
        campaign_manager_core.in(),
        logger(),
        campaign_manager_config_->GrpcConfig()->Endpoint().host().present() &&
          *(campaign_manager_config_->GrpcConfig()->Endpoint().host()) != "*" ?
          *campaign_manager_config_->GrpcConfig()->Endpoint().host() :
        "0.0.0.0",
        campaign_manager_config_->GrpcConfig()->Endpoint().port(),
        campaign_manager_config_->GrpcConfig()->process_threads(),
        campaign_manager_config_->GrpcConfig()->cq_threads(),
        campaign_manager_config_->GrpcConfig()->max_split().present() ?
          *campaign_manager_config_->GrpcConfig()->max_split() :
          16);
      active_objects->add_child_object(non_owning_active_object(
        grpc_adapter.in()));
    }

    if(campaign_manager_config_->HttpConfig().present())
    {
      AdServer::Commons::HttpServer::HttpServer_var http_server;
      stage = "creating CampaignManager HttpServer";
      http_server = new AdServer::Commons::HttpServer::HttpServer(
        campaign_manager_config_->HttpConfig()->Endpoint().host().present() &&
          *(campaign_manager_config_->HttpConfig()->Endpoint().host()) != "*" ?
          *campaign_manager_config_->HttpConfig()->Endpoint().host() :
          "0.0.0.0",
        campaign_manager_config_->HttpConfig()->Endpoint().port(),
        4);
      http_server->add_handler(
        "/stats",
        [
          grpc_adapter,
          campaign_manager_core,
          campaign_manager_logger
        ](
          const AdServer::Commons::HttpServer::HttpServer::Request&)
        {
          std::string body = "{";
          bool first = true;
            append_campaign_manager_stats(
              body,
              first,
              grpc_adapter.in(),
            campaign_manager_core.in(),
            campaign_manager_logger);

          body += "}\n";

          return AdServer::Commons::HttpServer::HttpServer::Response{
            200,
            "application/json",
            std::move(body)
          };
        });

      active_objects->add_child_object(http_server.in());
    }

    active_objects->activate_object();

    logger()->sstream(Logging::Logger::NOTICE, ASPECT) << "service started.";

    AdServer::Commons::SignalActiveObject signal_active_object;
    signal_active_object.wait_object();

    logger()->sstream(Logging::Logger::NOTICE, ASPECT) << "service stopped.";
  }
  catch (const Exception& e)
  {
    logger()->sstream(
      Logging::Logger::CRITICAL,
      ASPECT,
      "ADS-IMPL-168") <<
      "CampaignManagerApp_::main(): Got CampaignManagerApp_::Exception on " <<
      stage << " stage. : \n" << e.what();
  }
  catch (const eh::Exception& e)
  {
    logger()->sstream(
      Logging::Logger::EMERGENCY,
      ASPECT,
      "ADS-IMPL-168") <<
      "CampaignManagerApp_::main(): Got eh::Exception on " <<
      stage << " stage. : \n" << e.what();
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
}

void
CampaignManagerApp_::read_creative_config(
  AdServer::CampaignSvcs::CampaignManagerCore::CreativeInstantiate&
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
    AdServer::CampaignSvcs::CampaignManagerCore::
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
  log_params.threads = config.threads();

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
          CampaignManagerCore::CampaignManagerConfig(
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
    configuration_.pid_file = configuration->pid_file();
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
  try
  {
    CampaignManagerApp_ app;
    app.main(argc, argv);
  }
  catch (const eh::Exception& ex)
  {
    std::cerr << "main(): Critical: Got exception while "
      "creating application object: " << ex.what() << "\n";
    return -1;
  }
  catch (...)
  {
    std::cerr << "main(): Critical: Got exception while "
      "creating application object.\n";
    return -1;
  }
}
