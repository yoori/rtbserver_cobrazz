#include "../../TestHelpers.hpp"

#include <LogCommons/LogCommons.hpp>

using namespace AdServer::LogProcessing;

TEST(restore_log_file_name)
{
  LogFileNameInfo info("Request");
  info.timestamp = Generics::Time(String::SubString("20090525.111111"), "%Y%m%d.%H%M%S");
  info.distrib_index = 5555;
  info.random = 1234567890;

  ASSERT_EQUALS (
    restore_log_file_name(info, "/some_path"), 
    "/some_path/Request.20090525.111111.000000.1234567890.1.5555");

  info.processed_lines_count = 311;
  ASSERT_EQUALS (
    restore_log_file_name(info, "/some_path"), 
    "/some_path/Request.20090525.111111.000000.1234567890.1.5555.311");

  ASSERT_EQUALS (
    restore_log_file_name(info, ""), 
    "Request.20090525.111111.000000.1234567890.1.5555.311");
}

RUN_TESTS
