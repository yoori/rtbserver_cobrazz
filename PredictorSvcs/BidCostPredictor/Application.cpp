// STD
#include <iostream>
#include <string>

// THIS
#include <Generics/AppUtils.hpp>
#include <Logger/StreamLogger.hpp>
#include "Application.hpp"
#include "Aggregator.hpp"
#include "AggregatorMultyThread.hpp"
#include "Processor.hpp"
#include "Reaggregator.hpp"
#include "ReaggregatorMultyThread.hpp"
#include "Regenerator.hpp"
#include "Test.hpp"

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
  Generics::AppUtils::CheckOption option_test;

  Generics::AppUtils::Args args(-1);
  args.add(
          Generics::AppUtils::equal_name("help")
          || Generics::AppUtils::short_name("h"),
          option_help);
  args.parse(argc - 1, argv + 1);
  const Generics::AppUtils::Args::CommandList& commands = args.commands();

  ProcessorPtr processor;
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
      processor = ProcessorPtr(
              new Regenerator(
                      input_directory,
                      output_directory,
                      logger));
    }
    else if (command == "aggregate")
    {
      const std::size_t max_process_files = 10000;
      const std::size_t dump_max_size = 100000;
      processor = ProcessorPtr(
              new AggregatorMultyThread(
                      max_process_files,
                      dump_max_size,
                      input_directory,
                      output_directory,
                      logger));
    }
    else if (command == "reaggregate")
    {
      processor = ProcessorPtr(
              new ReaggregatorMultyThread(
                      input_directory,
                      output_directory,
                      logger));
    }
  }
  else
  {
    std::cerr << "Unknown command '"
              << command << "'\n"
              << "See help for more information.";
    return EXIT_FAILURE;
  }

  if (processor)
  {
    processor->start();
    processor->wait();
  }
  else
  {
    throw Exception("Processor is empty");
  }

  return EXIT_SUCCESS;
}

} // namespace BidCostPredictor
} // namespace PredictorSvcs