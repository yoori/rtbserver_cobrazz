// STD
#include <iostream>
#include <string>

// THIS
#include <Generics/AppUtils.hpp>
#include <Logger/StreamLogger.hpp>
#include "Application.hpp"
#include "Aggregator.hpp"
#include "AggregatorMultyThread.hpp"
#include "DaemonImpl.hpp"
#include "Pid.hpp"
#include "Processor.hpp"
#include "Reaggregator.hpp"
#include "ReaggregatorMultyThread.hpp"
#include "Regenerator.hpp"
#include "Test.hpp"

namespace Aspect
{
const char* APPLICATION = "APPLICATION";
}

namespace
{
const char USAGE[] =
        "Usage: BitCostPredictor <COMMAND> [OPTIONS]\n"
        "Commands:\n"
        "  regenerate <INPUT DIRECTORY> <OUTPUT DIRECTORY>\n"
        "  aggregate <INPUT DIRECTORY> <OUTPUT DIRECTORY>\n"
        "  reaggregate <INPUT DIRECTORY> <OUTPUT DIRECTORY>\n"
        "  test <TEST DIRECTORY>"
        "Sample:\n"
        "  test /tmp/test_bid_cost_predictor\n"
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
  if (command == "test")
  {
    ++it;
    if (it == commands.end())
    {
      std::cerr << "test: directory not defined";
      return EXIT_FAILURE;
    }

    const std::string directory = *it;
    Test::testSuit(directory);
    return EXIT_SUCCESS;
  }
  else if (command == "regenerate"
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
  else if (command == "daemon")
  {
    ++it;
    if (it == commands.end())
    {
      std::cerr << "daemon: daemon option not defined";
      return EXIT_FAILURE;
    }
    const std::string& daemon_option = *it;

    const std::string pid_path = "/home/artem_bogdanov/model/daemon.pid";

    if (daemon_option == "start")
    {
      const std::string model_dir = "/home/artem_bogdanov/model";
      const std::string model_file_name = "bid_cost.csv";
      const std::string model_temp_dir = "/home/artem_bogdanov/temp/model";
      const std::string ctr_model_dir = "/home/artem_bogdanov/model_ctr";
      const std::string ctr_model_file_name = "trivial_ctr.csv";
      const std::string ctr_model_temp_dir = "/home/artem_bogdanov/temp/model_ctr";
      const std::string model_agg_dir = "/home/artem_bogdanov/BidCostStatAgg2";
      const std::size_t model_period = 300;
      const std::size_t agg_max_process_files = 10000;
      const std::size_t agg_dump_max_size = 100000;
      const std::string agg_input_dir = "/home/artem_bogdanov/BidCostStatAgg";
      const std::string agg_output_dir = "/home/artem_bogdanov/BidCostStatAgg";
      const std::size_t agg_period = 120;
      const std::string reagg_input_dir = "/home/artem_bogdanov/BidCostStatAgg2";
      const std::string reagg_output_dir = "/home/artem_bogdanov/BidCostStatAgg2";
      const std::size_t reagg_period = 120;
      const std::string path_log_file = "/home/artem_bogdanov/model/daemon_log.txt";

      std::ofstream ostream(path_log_file, std::ios::app);
      if (!ostream.is_open())
      {
        Stream::Error ostr;
        ostr << __PRETTY_FUNCTION__
             << "Can't open file="
             << path_log_file;
        throw Exception(ostr);
      }
      Logging::Logger_var logger(
              new Logging::OStream::Logger(
                      Logging::OStream::Config(ostream)));

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
      try
      {
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
          std::cerr << "Daemon not running";
        }
      }
      catch (const eh::Exception& exc)
      {
        std::stringstream stream;
        stream << __PRETTY_FUNCTION__
               << " : Reason: "
               << exc.what();
        std::cerr << stream.str();
      }
    }
  }
  else
  {
    std::cerr << "Unknown command '"
              << command << "'\n"
              << "See help for more information.";
    return EXIT_FAILURE;
  }


  return EXIT_SUCCESS;
}

} // namespace BidCostPredictor
} // namespace PredictorSvcs