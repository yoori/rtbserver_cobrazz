// STD
#include <iostream>

// UNIXCOMMONS
#include <eh/Exception.hpp>
#include <Logger/Logger.hpp>
#include <Logger/StreamLogger.hpp>

// THIS
#include <PredictorML/ClickModel/CsvConverter/CsvConverter.hpp>

int main(int /*argc*/, char** /*argv*/)
{
  try
  {
    const std::string csv_original_path = "/u03/jurij_kuznecov/model_data3.csv";
    const std::string csv_process_path = "/u03/ml/process_model_data.csv";

    Logging::Logger_var logger = new Logging::OStream::Logger(
      Logging::OStream::Config(
        std::cerr,
        Logging::Logger::INFO));

    CsvConverter csv_converter(
      logger.in(),
      csv_original_path,
      csv_process_path);
    csv_converter.process();
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