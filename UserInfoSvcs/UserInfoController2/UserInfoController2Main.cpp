#include "UserInfoController2Main.hpp"

#include <csignal>
#include <iostream>
#include <pthread.h>

#include <Commons/ConfigUtils.hpp>
#include <Commons/ErrorHandler.hpp>
#include <Logger/ActiveObjectCallback.hpp>

namespace
{
  const char ASPECT[] = "UserInfoController2";

  void
  wait_for_shutdown_signal_()
  {
    sigset_t signals;
    sigemptyset(&signals);
    sigaddset(&signals, SIGINT);
    sigaddset(&signals, SIGTERM);
    sigaddset(&signals, SIGQUIT);

    int signal_number = 0;
    sigwait(&signals, &signal_number);
  }
}

void
UserInfoController2App_::stop_() noexcept
{
  if (grpc_adapter_.in())
  {
    grpc_adapter_->deactivate_object();
    grpc_adapter_->wait_object();
    grpc_adapter_.reset();
  }

  if (controller_.in())
  {
    controller_->deactivate_object();
    controller_->wait_object();
    controller_.reset();
  }
}

void
UserInfoController2App_::main(int& argc, char** argv) noexcept
{
  static const char* FUN = "UserInfoController2App_::main()";

  try
  {
    const char* usage = "usage: UserInfoController2 <config_file>";
    if (argc < 2)
    {
      Stream::Error ostr;
      ostr << "config file is not specified\n" << usage;
      throw InvalidArgument(ostr);
    }

    Config::ErrorHandler error_handler;
    try
    {
      using namespace xsd::AdServer::Configuration;

      std::unique_ptr<AdConfigurationType> ad_configuration =
        AdConfiguration(argv[1], error_handler);
      if (error_handler.has_errors())
      {
        std::string error_string;
        throw Exception(error_handler.text(error_string));
      }

      configuration_.reset(
        new UserInfoController2ConfigType(
          ad_configuration->UserInfoController2Config()));
    }
    catch (const xml_schema::parsing& ex)
    {
      Stream::Error ostr;
      ostr << "Can't parse config file '" << argv[1] << "': ";
      if (error_handler.has_errors())
      {
        std::string error_string;
        ostr << error_handler.text(error_string);
      }
      else
      {
        ostr << ex;
      }
      throw Exception(ostr);
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "Can't parse config file '" << argv[1] << "': " << ex.what();
      throw Exception(ostr);
    }

    logger_ = Config::LoggerConfigReader::create(config().Logger(), argv[0]);
    Logging::ActiveObjectCallbackImpl_var callback(
      new Logging::ActiveObjectCallbackImpl(
        logger(),
        "UserInfoController2App",
        ASPECT));

    controller_ = new AdServer::UserInfoSvcs::UserInfoController2Impl(
      callback,
      logger(),
      config());

    grpc_adapter_ = new AdServer::UserInfoSvcs::UserInfoController2Grpc(
      controller_,
      logger(),
      config().GrpcConfig().Endpoint().host().present() &&
        *config().GrpcConfig().Endpoint().host() != "*" ?
        *config().GrpcConfig().Endpoint().host() :
        "0.0.0.0",
      config().GrpcConfig().Endpoint().port());

    controller_->activate_object();
    grpc_adapter_->activate_object();

    logger()->sstream(Logging::Logger::NOTICE, ASPECT) << "service started.";

    wait_for_shutdown_signal_();
    stop_();

    logger()->sstream(Logging::Logger::NOTICE, ASPECT) << "service stopped.";
  }
  catch (const Exception& ex)
  {
    if (logger())
    {
      logger()->sstream(Logging::Logger::CRITICAL, ASPECT, "ADS-IMPL-73") <<
        FUN << ": caught UserInfoController2App_::Exception: " << ex.what();
    }
    else
    {
      std::cerr << FUN << ": caught UserInfoController2App_::Exception: " <<
        ex.what() << '\n';
    }
  }
  catch (const eh::Exception& ex)
  {
    if (logger())
    {
      logger()->sstream(Logging::Logger::EMERGENCY, ASPECT, "ADS-IMPL-73") <<
        FUN << ": caught eh::Exception: " << ex.what();
    }
    else
    {
      std::cerr << FUN << ": caught eh::Exception: " << ex.what() << '\n';
    }
  }

  stop_();
}

int
main(int argc, char** argv)
{
  sigset_t signals;
  sigemptyset(&signals);
  sigaddset(&signals, SIGINT);
  sigaddset(&signals, SIGTERM);
  sigaddset(&signals, SIGQUIT);
  pthread_sigmask(SIG_BLOCK, &signals, nullptr);

  UserInfoController2App_ app;
  app.main(argc, argv);
}
