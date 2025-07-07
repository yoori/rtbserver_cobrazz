// STD
#include <iostream>

// UNIXCOMMONS
#include <eh/Exception.hpp>
#include <Logger/Logger.hpp>
#include <Logger/StreamLogger.hpp>

// THIS
#include "Benchmark.hpp"

int main(int /*argc*/, char** /*argv*/)
{
  try
  {
    const std::string model_path = "/u03/ml/model_0_01.bin";
    const std::string csv_path = "/u03/ml/process_model_data.csv";
    const std::uint32_t number_line = 10000;

    Logging::Logger_var logger = new Logging::OStream::Logger(
      Logging::OStream::Config(
        std::cerr,
        Logging::Logger::INFO));

    Benchmark benchmark(
      logger.in(),
      model_path,
      csv_path,
      number_line);
    return benchmark.run();
  }
  catch (const eh::Exception& exc)
  {
    std::cerr << exc.what()
              << '\n';
    return EXIT_FAILURE;
  }
  catch (...)
  {
    std::cerr << "Unknown error"
              << '\n';
    return EXIT_FAILURE;
  }
}