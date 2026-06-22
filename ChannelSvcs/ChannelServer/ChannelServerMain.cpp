#include <eh/Exception.hpp>

#include <memory>

#include <Generics/ActiveObject.hpp>
#include <Generics/Scheduler.hpp>

#include <ReferenceCounting/ReferenceCounting.hpp>

#include <Commons/CorbaConfig.hpp>
#include <Commons/ErrorHandler.hpp>
#include <Commons/ConfigUtils.hpp>
#include <Commons/HttpServer/HttpServer.hpp>
#include <Commons/PidFileGuard.hpp>
#include <Commons/SignalActiveObject.hpp>

#include "ChannelServerMain.hpp"
#include "ChannelServerCore.hpp"
#include "ChannelServerControlImpl.hpp"
#include "ChannelUpdateImpl.hpp"
//#include "ChannelServer.hpp"

namespace
{
  const char ASPECT[] = "ChannelServer";
  const char CHANNEL_SERVER_OBJ_KEY[] = "ChannelServer";
  const char CHANNEL_SERVER_CONTROL_OBJ_KEY[] = "ChannelServerControl";
  const char CHANNEL_UPDATE_OBJ_KEY[] = "ChannelUpdate";

  template<typename T>
  std::shared_ptr<T>
  to_shared(ReferenceCounting::SmartPtr<T> ptr)
  {
    T* raw_ptr = ptr.in();
    return std::shared_ptr<T>(
      raw_ptr,
      [ptr = std::move(ptr)](T*) mutable
      {
        ptr.reset();
      });
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
}

ChannelServerApp_::ChannelServerApp_() /*throw(eh::Exception)*/
  : Logging::LoggerCallbackHolder(
      Logging::Logger_var(new Logging::OStream::Logger(
        Logging::OStream::Config(std::cerr))),
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

    if(error_handler.has_errors())
    {
      std::string error_string;
      throw Exception(error_handler.text(error_string));
    }

    configuration_ =
      ConfigPtr(new ChannelServerConfigType(
        ad_configuration->ChannelServerConfig()));

  }
  catch(const xml_schema::parsing& e)
  {
    Stream::Error ostr;

    ostr << "Can't parse config file '"
         << name << "'."
         << ": ";

    if(error_handler.has_errors())
    {
      std::string error_string;
      ostr << error_handler.text(error_string);
    }

    throw Exception(ostr);
  }
  catch(const eh::Exception& e)
  {
    Stream::Error ostr;
    ostr << "Can't parse config file '"
         << name << "'."
         << ": "
         << e.what();
    throw Exception(ostr);
  }
  catch(...)
  {
    Stream::Error ostr;
    ostr << "Unknown Exception at parsing of config " << name;
    throw Exception(ostr);
  }
}

void ChannelServerApp_::init_corba_() /*throw(Exception, CORBA::SystemException)*/
{
  //Fill corba_config
  try
  {
    corba_config_.custom_reactor = false;
    Config::CorbaConfigReader::read_config(
      configuration_->CorbaConfig(),
      corba_config_);
  }
  catch(const eh::Exception& e)
  {
    Stream::Error ostr;
    ostr << "Can't read Corba Config. : "
         << e.what();
    throw Exception(ostr);
  }

  try
  {
    // init CORBA Server
    corba_server_adapter_ =
      new CORBACommons::CorbaServerAdapter(corba_config_);

  }
  catch(const eh::Exception& e)
  {
    Stream::Error ostr;
    ostr << "Can't init CorbaServerAdapter. : "
         << e.what();
    throw Exception(ostr);
  }

  try
  {
    server_core_ = std::make_shared<AdServer::ChannelSvcs::ChannelServerCore>(
      logger(), configuration_.get());
    server_impl_ = new AdServer::ChannelSvcs::ChannelServerCustomImpl(
      server_core_);

    AdServer::ChannelSvcs::ChannelServerControlImpl_var server_control_impl(
      new AdServer::ChannelSvcs::ChannelServerControlImpl(server_core_));

    AdServer::ChannelSvcs::ChannelUpdateImpl_var update_impl(
      new AdServer::ChannelSvcs::ChannelUpdateImpl(server_core_));

    corba_server_adapter_->add_binding(
      CHANNEL_SERVER_OBJ_KEY, server_impl_.in());

    corba_server_adapter_->add_binding(
      CHANNEL_UPDATE_OBJ_KEY, update_impl.in());

    corba_server_adapter_->add_binding(
      CHANNEL_SERVER_CONTROL_OBJ_KEY, server_control_impl.in());

    if(configuration_->GrpcConfig().present())
    {
      grpc_adapter_ = to_shared<AdServer::ChannelSvcs::ChannelServerGrpc>(
        new AdServer::ChannelSvcs::ChannelServerGrpc(
          server_core_,
          logger(),
          configuration_->GrpcConfig()->Endpoint().host().present() &&
            *(configuration_->GrpcConfig()->Endpoint().host()) != "*" ?
            *configuration_->GrpcConfig()->Endpoint().host() :
          "0.0.0.0",
          configuration_->GrpcConfig()->Endpoint().port(),
          configuration_->GrpcConfig()->cq_threads().present() ?
            *configuration_->GrpcConfig()->cq_threads() :
            configuration_->GrpcConfig()->process_threads(),
          configuration_->GrpcConfig()->max_split().present() ?
            static_cast<std::size_t>(*configuration_->GrpcConfig()->max_split()) :
            static_cast<std::size_t>(configuration_->GrpcConfig()->process_threads())));
    }

    if(configuration_->HttpConfig().present())
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
        ](
          const AdServer::Commons::HttpServer::HttpServer::Request&)
        {
          AdServer::ChannelSvcs::ChannelServerStats stats;
          server_core->get_stats(stats);

          std::string body = "{";
          body += "\"total_requests\":";
          body += std::to_string(
            stats.params[
              AdServer::ChannelSvcs::ChannelServerStats::MATCHINGS_COUNT]);
          for (std::size_t i = 0;
            i < AdServer::ChannelSvcs::ChannelServerStats::PARAMS_MAX;
            ++i)
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
            body += ",\"set_proxy_sources_in_progress\":";
            body += std::to_string(grpc_stats.set_proxy_sources_in_progress);
            body += ",\"batch_total\":";
            body += std::to_string(grpc_stats.batch_total);
            body += ",\"batch_total_time\":";
            body += std::to_string(grpc_stats.batch_total_time);
            body += ",\"batch_in_progress\":";
            body += std::to_string(grpc_stats.batch_in_progress);
          }
          body += "}\n";

          return AdServer::Commons::HttpServer::HttpServer::Response{
            200,
            "application/json",
            std::move(body)
          };
        });
    }

    active_objects_ =
      std::make_shared<Generics::CompositeActiveObject>(false, false);
    active_objects_->add_child_object(server_core_);
    if(grpc_adapter_)
    {
      active_objects_->add_child_object(
        std::static_pointer_cast<Generics::ActiveObject>(grpc_adapter_));
    }
    if(http_server_.in() != 0)
    {
      active_objects_->add_child_object(http_server_.in());
    }
    active_objects_->add_child_object(corba_server_adapter_.in());
  }
  catch(const AdServer::ChannelSvcs::ChannelServerCore::Exception& e)
  {
    Stream::Error ostr;
    ostr << "ChannelServerApp::init_corba_: "
      "Catch ChannelServerCore::Exception on creating "
      " ChannelServer core. : " << e.what();
    throw Exception(ostr);
  }
  catch(const eh::Exception& e)
  {
    Stream::Error ostr;
    ostr << "ChannelServerApp::init_corba_: "
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
      logger(Config::LoggerConfigReader::create(
        configuration_->Logger(), argv[0]));
    }
    catch (const Config::LoggerConfigReader::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << "got LoggerConfigReader::Exception: "
        << e.what();
      throw Exception(ostr);
    }

    pid_file_guard = std::make_unique<AdServer::Commons::PidFileGuard>(
      std::string(configuration_->pid_file()));

    //Initialization CORBA
    init_corba_();

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
      logger()->sstream(Logging::Logger::EMERGENCY,
                        ASPECT,
                        "ADS-IMPL-11")
        << FUN
        << "Got ChannelServerApp_::Exception. : \n"
        << e.what();
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
      logger()->sstream(Logging::Logger::EMERGENCY,
                        ASPECT,
                        "ADS-IMPL-11")
        << FUN
        << "Got CORBA::SystemException. : \n"
        << e;
    }
    catch (...)
    {
      logger()->log(String::SubString("ChannelServerApp_::main(): "
                    "Got unknown exception."),
                    Logging::Logger::EMERGENCY,
                    ASPECT,
                    "ADS-IMPL-11");
    }
  }
  catch (const eh::Exception& e)
  {
    try
    {
      logger()->sstream(Logging::Logger::EMERGENCY,
                        ASPECT,
                        "ADS-IMPL-11")
       << FUN
       << "Got eh::Exception. : \n"
       << e.what();
    }
    catch (...)
    {
      logger()->log(String::SubString("ChannelServerApp_::main(): "
                    "Got unknown exception."),
                    Logging::Logger::EMERGENCY,
                    ASPECT,
                    "ADS-IMPL-11");
    }
  }
  catch (...)
  {
    logger()->log(String::SubString("ChannelServerApp_::main(): "
                  "Got Unknown exception."),
                  Logging::Logger::EMERGENCY,
                  ASPECT,
                  "ADS-IMPL-11");
  }

  /* references to servants in ORB must be destroyed before
     destroying var pointer to its */
  try
  {
    corba_server_adapter_.reset();
  }
  catch(const CORBA::Exception& ex)
  {
    logger()->sstream(Logging::Logger::EMERGENCY,
                      ASPECT,
                      "ADS-IMPL-11")
      << FUN
      << "Got CORBA::Exception in destroy ORB. : \n"
      << ex;
  }
  catch(...)
  {
    logger()->sstream(Logging::Logger::EMERGENCY,
                      ASPECT,
                      "ADS-IMPL-11")
      << FUN
      << "Got unknown exception in destroy ORB \n";
  }
}

int main(int argc, char** argv)
{
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
