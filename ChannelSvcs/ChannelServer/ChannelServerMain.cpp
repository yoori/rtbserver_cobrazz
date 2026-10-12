#include <eh/Exception.hpp>
#include <tao/SystemException.h>

#include <memory>

#include <Generics/ActiveObject.hpp>
#include <Generics/Scheduler.hpp>

#include <ReferenceCounting/ReferenceCounting.hpp>

#include <Commons/ErrorHandler.hpp>
#include <Commons/ConfigUtils.hpp>
#include <Commons/HttpServer/HttpServer.hpp>
#include <Commons/PidFileGuard.hpp>
#include <Commons/SignalActiveObject.hpp>

#include "ChannelServerMain.hpp"
#include "ChannelServerCore.hpp"

namespace
{
  const char ASPECT[] = "ChannelServer";

  template<typename T>
  std::shared_ptr<T>
  to_shared(ReferenceCounting::SmartPtr<T> ptr)
  {
    T* raw_ptr = ptr.in();
    return std::shared_ptr<T>(raw_ptr, [ptr = std::move(ptr)](T*) mutable { ptr.reset(); });
  }

  std::string
  json_escape_(const std::string& value)
  {
    std::string result;
    result.reserve(value.size());
    for (const char ch : value)
    {
      switch (ch)
      {
      case '\\':
        result += "\\\\";
        break;
      case '"':
        result += "\\\"";
        break;
      case '\n':
        result += "\\n";
        break;
      case '\r':
        result += "\\r";
        break;
      case '\t':
        result += "\\t";
        break;
      default:
        result += ch;
        break;
      }
    }
    return result;
  }

  void
  append_json_stat_(std::string& body, const char* name, const std::uint64_t value)
  {
    body += ",\"";
    body += name;
    body += "\":";
    body += std::to_string(value);
  }

  void
  append_grpc_lifecycle_stats_(
    std::string& body,
    const AdServer::Grpc::GrpcServiceBase::LifecycleStatsSnapshot& stats)
  {
    append_json_stat_(body, "grpc_unary_call_created_total", stats.unary_call_created_total);
    append_json_stat_(body, "grpc_unary_call_deleted_total", stats.unary_call_deleted_total);
    append_json_stat_(body, "grpc_unary_call_live", stats.unary_call_live);
    append_json_stat_(
      body,
      "grpc_coro_unary_call_created_total",
      stats.coro_unary_call_created_total);
    append_json_stat_(
      body,
      "grpc_coro_unary_call_deleted_total",
      stats.coro_unary_call_deleted_total);
    append_json_stat_(body, "grpc_coro_unary_call_live", stats.coro_unary_call_live);
    append_json_stat_(
      body,
      "grpc_batch_stream_call_created_total",
      stats.batch_stream_call_created_total);
    append_json_stat_(
      body,
      "grpc_batch_stream_call_deleted_total",
      stats.batch_stream_call_deleted_total);
    append_json_stat_(body, "grpc_batch_stream_call_live", stats.batch_stream_call_live);
  }
}

ChannelServerApp_::ChannelServerApp_() /*throw(eh::Exception)*/
  : Logging::LoggerCallbackHolder(
      Logging::Logger_var(new Logging::OStream::Logger(Logging::OStream::Config(std::cerr))),
      "ChannelServerApp_", ASPECT, 0)
{
}

void ChannelServerApp_::load_config_(const char* name) /*throw(Exception)*/
{
  Config::ErrorHandler error_handler;
  try
  {
    /* using xsd namespace */
    using namespace xsd::AdServer::Configuration;

    std::string file_name(name);

    std::unique_ptr<AdConfigurationType>
      ad_configuration = AdConfiguration(file_name.c_str(), error_handler);

    if (error_handler.has_errors())
    {
      std::string error_string;
      throw Exception(error_handler.text(error_string));
    }

    configuration_ =
      ConfigPtr(new ChannelServerConfigType(ad_configuration->ChannelServerConfig()));

  }
  catch(const xml_schema::parsing& e)
  {
    Stream::Error ostr;

    ostr << "Can't parse config file '" << name << "'." << ": ";

    if (error_handler.has_errors())
    {
      std::string error_string;
      ostr << error_handler.text(error_string);
    }

    throw Exception(ostr);
  }
  catch(const eh::Exception& e)
  {
    Stream::Error ostr;
    ostr << "Can't parse config file '" << name << "'." << ": " << e.what();
    throw Exception(ostr);
  }
  catch(...)
  {
    Stream::Error ostr;
    ostr << "Unknown Exception at parsing of config " << name;
    throw Exception(ostr);
  }
}

void ChannelServerApp_::init_services_() /*throw(Exception)*/
{
  try
  {
    server_core_ = std::make_shared<AdServer::ChannelSvcs::ChannelServerCore>(
      logger(), configuration_.get());
    const auto& grpc_config = configuration_->GrpcConfig();
    grpc_adapter_ = to_shared<AdServer::ChannelSvcs::ChannelServerGrpc>(
      new AdServer::ChannelSvcs::ChannelServerGrpc(
        server_core_,
        logger(),
        grpc_config.Endpoint().host().present() && *grpc_config.Endpoint().host() != "*" ?
          *grpc_config.Endpoint().host() : "0.0.0.0",
        grpc_config.Endpoint().port(),
        grpc_config.process_threads(),
        grpc_config.cq_threads(),
        grpc_config.threads_per_cq(),
        static_cast<std::size_t>(grpc_config.max_sequential_ops())));

    if (configuration_->HttpConfig().present())
    {
      http_server_ = new AdServer::Commons::HttpServer::HttpServer(
        configuration_->HttpConfig()->Endpoint().host().present() &&
          *(configuration_->HttpConfig()->Endpoint().host()) != "*" ?
          *configuration_->HttpConfig()->Endpoint().host() :
          "0.0.0.0",
        configuration_->HttpConfig()->Endpoint().port(),
        4);
      http_server_->add_handler(
        "/stats",
        [
          server_core = server_core_,
          grpc_adapter = grpc_adapter_
        ](AdServer::Commons::HttpServer::HttpServer::Request)
        {
          AdServer::ChannelSvcs::ChannelServerStats stats;
          server_core->get_stats(stats);

          std::string body = "{";
          body += "\"total_requests\":";
          body += std::to_string(stats.total_requests);
          for (std::size_t i = 0; i < AdServer::ChannelSvcs::ChannelServerStats::PARAMS_MAX; ++i)
          {
            body += ",\"";
            body += AdServer::ChannelSvcs::ChannelServerStats::param_name[i];
            body += "\":";
            body += std::to_string(stats.params[i]);
          }
          body += ",\"configuration\":\"";
          body += json_escape_(stats.configuration);
          body += "\",\"configuration_date\":";
          body += std::to_string(stats.configuration_date.tv_sec);
          if (grpc_adapter)
          {
            const auto grpc_stats = grpc_adapter->stats();
            body += ",\"call_in_progress\":";
            body += std::to_string(grpc_stats.call_in_progress);
            body += ",\"match_in_progress\":";
            body += std::to_string(grpc_stats.match_in_progress);
            body += ",\"get_ccg_traits_in_progress\":";
            body += std::to_string(grpc_stats.get_ccg_traits_in_progress);
            body += ",\"check_configuration_in_progress\":";
            body += std::to_string(grpc_stats.check_configuration_in_progress);
            body += ",\"set_sources_in_progress\":";
            body += std::to_string(grpc_stats.set_sources_in_progress);
            append_json_stat_(
              body, "channel_update_in_progress", grpc_stats.channel_update_in_progress);
            body += ",\"set_proxy_sources_in_progress\":";
            body += std::to_string(grpc_stats.set_proxy_sources_in_progress);
            body += ",\"batch_total\":";
            body += std::to_string(grpc_stats.batch_total);
            body += ",\"batch_total_time\":";
            body += std::to_string(grpc_stats.batch_total_time);
            body += ",\"batch_in_progress\":";
            body += std::to_string(grpc_stats.batch_in_progress);
            append_grpc_lifecycle_stats_(body, grpc_stats.grpc_lifecycle_stats);
          }
          body += "}\n";

          return AdServer::Commons::HttpServer::make_ready_response(
            AdServer::Commons::HttpServer::HttpServer::Response{
              200,
              "application/json",
              std::move(body)
            });
        });
    }

    active_objects_ = std::make_shared<Generics::CompositeActiveObject>(false, false);
    active_objects_->add_child_object(server_core_);
    if (grpc_adapter_)
    {
      active_objects_->add_child_object(
        std::static_pointer_cast<Generics::ActiveObject>(grpc_adapter_));
    }

    if (http_server_.in() != 0)
    {
      active_objects_->add_child_object(http_server_.in());
    }
  }
  catch(const AdServer::ChannelSvcs::ChannelServerCore::Exception& e)
  {
    Stream::Error ostr;
    ostr << "ChannelServerApp::init_services_: "
      "Catch ChannelServerCore::Exception on creating "
      " ChannelServer core. : " << e.what();
    throw Exception(ostr);
  }
  catch(const eh::Exception& e)
  {
    Stream::Error ostr;
    ostr << "ChannelServerApp::init_services_: "
      "Catch eh::Exception on creating ChannelServer servants. "
      ": " << e.what();
    throw Exception(ostr);
  }
}

void ChannelServerApp_::main(int& argc, char** argv) noexcept
{
  const char FUN[] = "ChannelServerApp_::main(): ";
  std::unique_ptr<AdServer::Commons::PidFileGuard> pid_file_guard;

  try
  {
    const char* usage = "usage: ChannelServer <config_file>";

    if (argc < 2)
    {
      Stream::Error ostr;
      ostr << "config file is not specified\n" << usage;
      throw InvalidArgument(ostr);
    }

    //Load configuration
    load_config_(argv[1]);

    //Initializing logger
    try
    {
      logger(Config::LoggerConfigReader::create(configuration_->Logger(), argv[0]));
    }
    catch (const Config::LoggerConfigReader::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << "got LoggerConfigReader::Exception: " << e.what();
      throw Exception(ostr);
    }

    pid_file_guard = std::make_unique<AdServer::Commons::PidFileGuard>(
      std::string(configuration_->pid_file()));

    init_services_();

    AdServer::Commons::SignalActiveObject signal_active_object;
    active_objects_->activate_object();

    logger()->sstream(Logging::Logger::NOTICE, ASPECT) << "service started.";
    signal_active_object.wait_object();
    active_objects_->deactivate_object();
    active_objects_->wait_object();

    logger()->sstream(Logging::Logger::NOTICE, ASPECT) << "service stopped.";

  }
  catch (const Exception& e)
  {
    try
    {
      logger()->sstream(Logging::Logger::EMERGENCY, ASPECT, "ADS-IMPL-11")
        << FUN << "Got ChannelServerApp_::Exception. : \n" << e.what();
    }
    catch (...)
    {
      logger()->log(String::SubString("ChannelServerApp_::main(): "
                    "Got ChannelServerApp_::Exception."),
                    Logging::Logger::EMERGENCY,
                    ASPECT,
                    "ADS-IMPL-11");
    }
  }
  catch (const CORBA::SystemException& e)
  {
    try
    {
      logger()->sstream(Logging::Logger::EMERGENCY, ASPECT, "ADS-IMPL-11")
        << FUN << "Got CORBA::SystemException. : \n" << e;
    }
    catch (...)
    {
      logger()->log(String::SubString("ChannelServerApp_::main(): " "Got unknown exception."),
                    Logging::Logger::EMERGENCY,
                    ASPECT,
                    "ADS-IMPL-11");
    }
  }
  catch (const eh::Exception& e)
  {
    try
    {
      logger()->sstream(Logging::Logger::EMERGENCY, ASPECT, "ADS-IMPL-11")
       << FUN << "Got eh::Exception. : \n" << e.what();
    }
    catch (...)
    {
      logger()->log(String::SubString("ChannelServerApp_::main(): " "Got unknown exception."),
                    Logging::Logger::EMERGENCY,
                    ASPECT,
                    "ADS-IMPL-11");
    }
  }
  catch (...)
  {
    logger()->log(String::SubString("ChannelServerApp_::main(): " "Got Unknown exception."),
                  Logging::Logger::EMERGENCY,
                  ASPECT,
                  "ADS-IMPL-11");
  }
}

int main(int argc, char** argv)
{
  AdServer::Commons::PidFileGuard::enable_process_exit_cleanup();

  ChannelServerApp_* app = 0;
  try
  {
    app = &ChannelServerApp::instance();
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
  return 0;
}
