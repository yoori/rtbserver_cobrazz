// STD
#include <iostream>
#include <string>
#include <thread>

// POSIX
#include <signal.h>

//UNIXCOMMONS
#include <Generics/AppUtils.hpp>

// THIS
#include <Logger/StreamLogger.hpp>
#include "Application.hpp"
#include "AggregatorMultyThread.hpp"
#include "Configuration.h"
#include "DaemonImpl.hpp"
#include "Pid.hpp"
#include "ModelProcessor.hpp"
#include "ReaggregatorMultyThread.hpp"
#include "Regenerator.hpp"

namespace Aspect
{

inline constexpr char APPLICATION[] = "APPLICATION";

} // namespace Aspect

inline constexpr char USAGE[] =
  "Usage: BitCostPredictor <COMMAND> [OPTIONS]\n"
  "Commands:\n"
  "  service start <PATH CONFIG>\n"
  "  service stop <PATH CONFIG>\n"
  "  service status <PATH CONFIG>\n"
  "  regenerate <INPUT DIRECTORY> <OUTPUT DIRECTORY>\n"
  "Sample:\n"
  "  service start /home/user/config.json\n"
  "  service stop /home/user/config.json\n"
  "  service status /home/user/config.json\n"
  "  regenerate /tmp/original /tmp/destination\n";

namespace PredictorSvcs::BidCostPredictor
{

int Application::run(int argc, char **argv)
{
  Generics::AppUtils::CheckOption option_help;

  Generics::AppUtils::Args args(-1);
  args.add(
    Generics::AppUtils::equal_name("help") || Generics::AppUtils::short_name("h"),
    option_help);
  args.parse(argc - 1, argv + 1);
  const Generics::AppUtils::Args::CommandList& commands = args.commands();

  Logging::Logger_var logger = new Logging::OStream::Logger(
    Logging::OStream::Config(std::cerr));

  if (commands.empty()
   || option_help.enabled()
   || *commands.begin() == "help")
  {
    std::cout << USAGE << std::endl;
    return EXIT_SUCCESS;
  }

  auto it = commands.begin();
  const std::string command = *it;
  if (command == "service")
  {
    ++it;
    if (it == commands.end())
    {
      std::cerr << "service: option not defined (start/stop/status)";
      return EXIT_FAILURE;
    }
    const std::string& service_option = *it;

    ++it;
    if (it == commands.end())
    {
      std::cerr << "service: path_config not defined";
      return EXIT_FAILURE;
    }
    const std::string& path_config = *it;

    if (service_option == "start")
    {
      const Configuration configuration(path_config);
      const std::string log_path = configuration.get("config.log_path");
      const std::string pid_path = configuration.get("config.pid_path");

      const std::size_t model_period =
        configuration.get<std::size_t>("config.model.period");
      const std::size_t agg_period =
        configuration.get<std::size_t>("config.aggregator.period");
      const std::size_t reagg_period =
        configuration.get<std::size_t>("config.reaggregator.period");

      std::ofstream ostream(log_path, std::ios::app);
      if (!ostream.is_open())
      {
        Stream::Error stream;
        stream << FNS
               << "Can't open file="
               << log_path;
        throw Exception(stream);
      }
      Logging::Logger_var logger = new Logging::OStream::Logger(
        Logging::OStream::Config(
          ostream,
          Logging::Logger::INFO));

      std::stringstream stream;
      stream << "Configuration:\n"
             << configuration;
      logger->info(stream.str(), Aspect::APPLICATION);

      Daemon_var daemon(
        new DaemonImpl(
          path_config,
          pid_path,
          model_period,
          agg_period,
          reagg_period,
          logger));
      try
      {
        daemon->run();
      }
      catch (const eh::Exception& exc)
      {
        Stream::Error stream;
        stream << FNS
               << "Reason: "
               << exc.what();
        logger->critical(stream.str(), Aspect::APPLICATION);
        return EXIT_FAILURE;
      }
    }
    else if (service_option == "stop")
    {
      try
      {
        Configuration configuration(path_config);
        const std::string pid_path =
          configuration.get("config.pid_path");

        PidGetter getter(pid_path);
        const auto pid = getter.get();
        if (pid)
        {
          if (::kill(*pid, SIGINT) == -1)
          {
            std::cerr << "Command kill is failed";
          }
        }
        else
        {
          std::cerr << "Service not running";
        }
      }
      catch (const eh::Exception& exc)
      {
        Stream::Error stream;
        stream << FNS
               << "Reason: "
               << exc.what();
        std::cerr << stream.str()
                  << std::endl;
        return EXIT_FAILURE;
      }
    }
    else if (service_option == "status")
    {
      try
      {
        Configuration configuration(path_config);
        const std::string pid_path =
          configuration.get("config.pid_path");

        PidSetter pid_setter(pid_path);
        if (pid_setter.set())
        {
          std::cout << "FAILED" << std::endl;
          return EXIT_FAILURE;
        }
        else
        {
          std::cout << "OK" << std::endl;
          return EXIT_SUCCESS;
        }
      }
      catch (const eh::Exception& exc)
      {
        Stream::Error stream;
        stream << FNS
               << "Reason: "
               << exc.what();
        std::cerr << stream.str()
                  << std::endl;
        return EXIT_FAILURE;
      }
    }
  }
  else if (command == "aggregate"
    || command == "reaggregate"
    || command == "model")
  {
    ++it;
    if (it == commands.end())
    {
      std::cerr << "path_config not defined";
      return EXIT_FAILURE;
    }

    const std::string& path_config = *it;

    const Configuration configuration(path_config);
    const std::string log_path =
      configuration.get("config.log_path");

    std::ofstream ostream(log_path, std::ios::app);
    if (!ostream.is_open())
    {
      Stream::Error ostr;
      ostr << FNS
           << "Can't open file="
           << log_path;
      throw Exception(ostr);
    }
    Logging::Logger_var logger(
      new Logging::OStream::Logger(
        Logging::OStream::Config(
          ostream,
          Logging::Logger::INFO)));

    try
    {
      const Configuration config_model =
        configuration.get_config("config.model");
      const std::string model_agg_dir =
        config_model.get("input_directory");
      const std::string bid_cost_model_dir =
        config_model.get("bid_cost.output_directory");
      const std::string bid_cost_model_temp_dir =
        config_model.get("bid_cost.temp_directory");
      const std::string bid_cost_model_file_name =
        config_model.get("bid_cost.file_name");
      const std::string ctr_model_dir =
        config_model.get("ctr.output_directory");
      const std::string ctr_model_temp_dir =
        config_model.get("ctr.temp_directory");
      const std::string ctr_model_file_name =
        config_model.get("ctr.file_name");
      const Imps ctr_model_max_imps =
        config_model.get<Imps>("ctr.max_imps");
      const Imps ctr_model_trust_imps =
        config_model.get<Imps>("ctr.trust_imps");
      const Imps ctr_model_tag_imps =
        config_model.get<Imps>("ctr.tag_imps");

      const Configuration config_aggregator =
        configuration.get_config("config.aggregator");
      const std::size_t agg_max_process_files =
        config_aggregator.get<std::size_t>("max_process_files");
      const std::size_t agg_dump_max_size =
        config_aggregator.get<std::size_t>("dump_max_size");
      const std::string agg_input_dir =
        config_aggregator.get("input_directory");
      const std::string agg_output_dir =
        config_aggregator.get("output_directory");

      const Configuration config_reaggregator =
        configuration.get_config("config.reaggregator");
      const std::string reagg_input_dir =
        config_reaggregator.get("input_directory");
      const std::string reagg_output_dir =
        config_reaggregator.get("output_directory");

      Processor_var processor;
      if (command == "model")
      {
        processor = new ModelProcessor(
            bid_cost_model_dir,
            bid_cost_model_file_name,
            bid_cost_model_temp_dir,
            ctr_model_dir,
            ctr_model_file_name,
            ctr_model_temp_dir,
            ctr_model_max_imps,
            ctr_model_trust_imps,
            ctr_model_tag_imps,
            model_agg_dir,
            logger);
      }
      else if (command == "aggregate")
      {
        processor = new AggregatorMultyThread(
            agg_max_process_files,
            agg_dump_max_size,
            agg_input_dir,
            agg_output_dir,
            logger);
      }
      else if (command == "reaggregate")
      {
        processor = new ReaggregatorMultyThread(
            reagg_input_dir,
            reagg_output_dir,
            logger);
      }
      else
      {
        return EXIT_FAILURE;
      }

      sigset_t mask;
      sigemptyset(&mask);
      sigaddset(&mask, SIGINT);
      sigaddset(&mask, SIGQUIT);
      sigaddset(&mask, SIGTERM);
      sigaddset(&mask, SIGUSR1);
      sigprocmask(SIG_BLOCK, &mask, nullptr);

      std::jthread thread([processor, logger, mask] () {
        try
        {
          int signo = 0;
          if (sigwait(&mask, &signo) != 0)
          {
            Stream::Error stream;
            stream << FNS
                   << "sigwait is failed";
            logger->critical(stream.str(), Aspect::APPLICATION);
            processor->deactivate_object();
          }
          else
          {
            std::stringstream stream;
            stream << "Signal=";
            switch (signo) {
              case SIGINT:
                stream << "SIGINT";
                break;
              case SIGQUIT:
                stream << "SIGQUIT";
                break;
              case SIGTERM:
                stream << "SIGTERM";
                break;
              default:
                stream << "Unexpected signal";
            }

            if (signo != SIGUSR1)
            {
              stream << " interrupted service";
              logger->info(stream.str(), Aspect::APPLICATION);
              processor->deactivate_object();
            }
          }
        }
        catch (const eh::Exception &exc)
        {
          try
          {
            Stream::Error stream;
            stream << FNS
                   << "Reason: "
                   << exc.what();
            logger->critical(stream.str(), Aspect::APPLICATION);
            processor->deactivate_object();
          }
          catch (...)
          {
          }
        }
      });

      try
      {
        processor->activate_object();
      }
      catch (const eh::Exception& exc)
      {
        kill(::getpid(), SIGUSR1);
        logger->critical(std::string(exc.what()), Aspect::APPLICATION);

        Stream::Error stream;
        stream << processor->name()
               << ": is failed";
        logger->critical(stream.str(), Aspect::APPLICATION);

        try
        {
          processor->deactivate_object();
        }
        catch (...)
        {
        }
      }

      try
      {
        processor->wait_object();
      }
      catch (...)
      {
      }

      const auto pid = ::getpid();
      if (::kill(pid, SIGUSR1) == -1)
      {
        Stream::Error stream;
        stream << FNS
               << "Reason: kill is failed";
        logger->error(stream.str(), Aspect::APPLICATION);
      }
    }
    catch (const eh::Exception& exc)
    {
      Stream::Error stream;
      stream << FNS
             << "Reason: "
             << exc.what();
      logger->critical(stream.str(), Aspect::APPLICATION);
      return EXIT_FAILURE;
    }
  }
  else if (command == "regenerate")
  {
    ++it;
    std::string input_directory;
    if (it == commands.end())
    {
      std::cerr << "regenerate: input directory not defined";
      return EXIT_FAILURE;
    }
    input_directory = *it;

    ++it;
    std::string output_directory;
    if (it == commands.end())
    {
      std::cerr << "regenerate: output directory not defined";
      return EXIT_FAILURE;
    }
    output_directory = *it;

    Processor_var processor = new Regenerator(
      input_directory,
      output_directory,
      logger);

    try
    {
      processor->activate_object();
    }
    catch (const eh::Exception& exc)
    {
      Stream::Error stream;
      stream << processor->name()
             << ": is failed";
      logger->critical(stream.str(), Aspect::APPLICATION);

      try
      {
        processor->deactivate_object();
      }
      catch (...)
      {
      }
    }

    try
    {
      processor->wait_object();
    }
    catch (...)
    {
    }
  }
  else
  {
    std::cerr << "Unknown command '"
              << command << "'\n"
              << "See help for more information.\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

} // namespace PredictorSvcs::BidCostPredictor