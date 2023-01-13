// STD
#include <iostream>
#include <string>

// THIS
#include <Generics/AppUtils.hpp>
#include <Logger/StreamLogger.hpp>
#include "Application.hpp"
#include "Aggregator.hpp"
#include "AggregatorMultyThread.hpp"
#include "Configuration.h"
#include "DaemonImpl.hpp"
#include "Pid.hpp"
#include "Processor.hpp"
#include "Reaggregator.hpp"
#include "ReaggregatorMultyThread.hpp"
#include "Regenerator.hpp"

namespace Aspect
{
const char* APPLICATION = "APPLICATION";
}

namespace
{
const char USAGE[] =
  "Usage: BitCostPredictor <COMMAND> [OPTIONS]\n"
  "Commands:\n"
  "  service start <PATH CONFIG>\n"
  "  service stop <PATH CONFIG>\n"
  "  service status\n"
  "  regenerate <INPUT DIRECTORY> <OUTPUT DIRECTORY>\n"
  "  aggregate <INPUT DIRECTORY> <OUTPUT DIRECTORY>\n"
  "  reaggregate <INPUT DIRECTORY> <OUTPUT DIRECTORY>\n"
  "Sample:\n"
  "  service start /home/user/config.json\n"
  "  service stop /home/user/config.json\n"
  "  service status\n"
  "  regenerate /tmp/original /tmp/destination\n"
  "  aggregate /tmp/original /tmp/destination\n"
  "  reaggregate /tmp/original /tmp/destination\n";
}

namespace PredictorSvcs
{
namespace BidCostPredictor
{

int Application::run(int argc, char **argv)
{
  Generics::AppUtils::CheckOption option_help;

  Generics::AppUtils::Args args(-1);
  args.add(
    Generics::AppUtils::equal_name("help")
    || Generics::AppUtils::short_name("h"),
    option_help);
  args.parse(argc - 1, argv + 1);
  const Generics::AppUtils::Args::CommandList& commands = args.commands();

  Logging::Logger_var logger(
    new Logging::OStream::Logger(
      Logging::OStream::Config(std::cerr)));

  if (commands.empty()
   || option_help.enabled()
   || *commands.begin() == "help")
  {
    std::cout << USAGE << std::endl;
    return EXIT_SUCCESS;
  }

  auto it = commands.begin();
  const std::string command = *it;
  if (command == "regenerate"
    || command == "aggregate"
    || command == "reaggregate")
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

    if (command == "regenerate")
    {
      Processor_var processor(
        new Regenerator(
          input_directory,
          output_directory,
          logger));
      processor->start();
      processor->wait();
    }
    else if (command == "aggregate")
    {
      const std::size_t max_process_files = 10000;
      const std::size_t dump_max_size = 100000;
      Processor_var processor(
        new AggregatorMultyThread(
          max_process_files,
          dump_max_size,
          input_directory,
          output_directory,
          logger));
      processor->start();
      processor->wait();
    }
    else if (command == "reaggregate")
    {
      Processor_var processor(
        new ReaggregatorMultyThread(
          input_directory,
          output_directory,
          logger));
      processor->start();
      processor->wait();
    }
  }
  else if (command == "service")
  {
    ++it;
    if (it == commands.end())
    {
      std::cerr << "daemon: daemon path_config not defined";
      return EXIT_FAILURE;
    }
    const std::string& daemon_option = *it;

    if (daemon_option == "start")
    {
      ++it;
      if (it == commands.end())
      {
        std::cerr << "service: path_config not defined";
        return EXIT_FAILURE;
      }
      const std::string& path_config = *it;

      const Configuration configuration(path_config);
      const std::string version =
        configuration.get("version");
      const std::string description =
        configuration.get("description");
      const std::string log_path =
        configuration.get("config.log_path");
      const std::string pid_path =
        configuration.get("config.pid_path");

      const Configuration config_model =
        configuration.get_config("config.model");
      const std::string model_agg_dir =
        config_model.get("input_directory");
      const std::size_t model_period =
        config_model.get<std::size_t>("period");
      const std::string model_dir =
        config_model.get("bid_cost.output_directory");
      const std::string model_temp_dir =
        config_model.get("bid_cost.temp_directory");
      const std::string model_file_name =
        config_model.get("bid_cost.file_name");
      const std::string ctr_model_dir =
        config_model.get("ctr.output_directory");
      const std::string ctr_model_temp_dir =
        config_model.get("ctr.temp_directory");
      const std::string ctr_model_file_name =
        config_model.get("ctr.file_name");

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
      const std::size_t agg_period =
        config_aggregator.get<std::size_t>("period");

      const Configuration config_reaggregator =
        configuration.get_config("config.reaggregator");
      const std::string reagg_input_dir =
        config_reaggregator.get("input_directory");
      const std::string reagg_output_dir =
        config_reaggregator.get("output_directory");
      const std::size_t reagg_period =
        config_reaggregator.get<std::size_t>("period");

      std::ofstream ostream(log_path, std::ios::app);
      if (!ostream.is_open())
      {
        Stream::Error ostr;
        ostr << __PRETTY_FUNCTION__
             << "Can't open file="
             << log_path;
        throw Exception(ostr);
      }
      Logging::Logger_var logger(
        new Logging::OStream::Logger(
          Logging::OStream::Config(
            ostream,
            Logging::Logger::INFO)));
      logger->info(
        "Config version=" + version,
        Aspect::APPLICATION);
      logger->info(
        "Config description=" + description,
        Aspect::APPLICATION);

      Daemon_var daemon(
        new DaemonImpl(
          pid_path,
          model_dir,
          model_file_name,
          model_temp_dir,
          ctr_model_dir,
          ctr_model_file_name,
          ctr_model_temp_dir,
          model_agg_dir,
          model_period,
          agg_max_process_files,
          agg_dump_max_size,
          agg_input_dir,
          agg_output_dir,
          agg_period,
          reagg_input_dir,
          reagg_output_dir,
          reagg_period,
          logger));
      try
      {
        daemon->run();
      }
      catch (const eh::Exception& exc)
      {
        std::stringstream stream;
        stream << __PRETTY_FUNCTION__
               << " : Reason: "
               << exc.what();
        logger->critical(stream.str(), Aspect::APPLICATION);
        return EXIT_FAILURE;
      }
    }
    else if (daemon_option == "stop")
    {
      ++it;
      if (it == commands.end())
      {
        std::cerr << "daemon: path_config not defined";
        return EXIT_FAILURE;
      }
      const std::string& path_config = *it;

      try
      {
        Configuration configuration(path_config);
        const std::string pid_path =
          configuration.get("config.pid_path");

        PidGetter getter(pid_path);
        const auto pid = getter.get();
        if (pid)
        {
          if (kill(*pid, SIGINT) == -1)
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
        std::stringstream stream;
        stream << __PRETTY_FUNCTION__
               << " : Reason: "
               << exc.what();
        std::cerr << stream.str()
                  << std::endl;
      }
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

} // namespace BidCostPredictor
} // namespace PredictorSvcs