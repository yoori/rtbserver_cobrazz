#ifndef RTBSERVER_PARSER_HPP
#define RTBSERVER_PARSER_HPP

// STD
#include <string>

// UNIXCOMMONS
#include <Logger/Logger.hpp>

class CsvConverter final
{
public:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;

public:
  explicit CsvConverter(
    Logger* logger,
    const std::string& csv_original_path,
    const std::string& csv_process_path);

  void process();
private:
  Logger_var logger_;

  const std::string csv_original_path_;

  const std::string csv_process_path_;
};

#endif //RTBSERVER_PARSER_HPP
