#include <iostream>
#include <sstream>

#include <Generics/AppUtils.hpp>
#include <Logger/StreamLogger.hpp>

#include <LogCommons/LogCommons.hpp>

namespace
{
  const char DEFAULT_ROOT_PATH[] = "./";
  Generics::AppUtils::Option<std::string> root_path(DEFAULT_ROOT_PATH);

  const char CHECK_ROOT[] = "/SearchFilesTestDir/";
  const char USAGE[] =
    "SearchFilesTest [OPTIONS]\n"
    "OPTIONS:\n"
    "  -p, --path : path to folder with temporary files.\n"
    "  -h, --help : show this message.\n";

  std::string working_folder;
}

bool
init(int& argc, char**& argv) /*throw(eh::Exception)*/
{
  using namespace Generics::AppUtils;
  Args args;
  CheckOption opt_help;

  args.add(equal_name("path") || short_name("p"), root_path);
  args.add(equal_name("help") || short_name("h"), opt_help);

  args.parse(argc - 1, argv + 1);

  if (opt_help.enabled())
  {
    std::cout << USAGE << std::endl;
    return false;
  }

  return true;
}

using namespace AdServer::LogProcessing;

struct TestCase
{
  const char* file_name;
  LogFileNameInfo &standard;
};

static LogFileNameInfo test_log_file_name_info[4];

void prepare_test_cases()
{
  test_log_file_name_info[0].format = LogFileNameInfo::LFNF_EXTENDED;
  test_log_file_name_info[0].base_name = "ColoUpdateStat";
  test_log_file_name_info[0].timestamp.set(
    String::SubString("20110504.045959.123456"),
    "%Y%m%d.%H%M%S.%q"
  );
  test_log_file_name_info[0].distrib_count = 0;
  test_log_file_name_info[0].distrib_index = 1;

  test_log_file_name_info[1].format = LogFileNameInfo::LFNF_BASIC;
  test_log_file_name_info[1].base_name = "ColoUpdateStat";
  test_log_file_name_info[1].timestamp.set(1290285094);
  test_log_file_name_info[1].distrib_count = 0;
  test_log_file_name_info[1].distrib_index = 1;

  test_log_file_name_info[2].format = LogFileNameInfo::LFNF_V_2_6;
  test_log_file_name_info[2].base_name = "ColoUpdateStat";
  test_log_file_name_info[2].timestamp.set(
    String::SubString("20120831.102438.736496"),
    "%Y%m%d.%H%M%S.%q"
  );
  test_log_file_name_info[2].distrib_count = 1;
  test_log_file_name_info[2].distrib_index = 0;

  test_log_file_name_info[3] = test_log_file_name_info[2];
  test_log_file_name_info[3].processed_lines_count = 1043;
}

const TestCase test_cases[] =
{
  {
    "ColoUpdateStat.log_1.0_20110504.045959.123456.12345678",
    test_log_file_name_info[0]
  },
  {
    "ColoUpdateStat.log_1.1_20101120.1290285094.17556678",
    test_log_file_name_info[1]
  },
  {
    "ColoUpdateStat.20120831.102438.736496.43242548.1.0",
    test_log_file_name_info[2]
  },
  {
    "ColoUpdateStat.20120831.102438.736496.43242548.1.0.1043",
    test_log_file_name_info[3]
  },
};

void
direct_test()
{
  LogFileNameInfo file_name_info;

  for (std::size_t i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); ++i)
  {
    const LogFileNameInfo &standard = test_cases[i].standard;
    parse_log_file_name(test_cases[i].file_name, file_name_info);
    if (
      file_name_info.format != standard.format ||
      file_name_info.base_name != standard.base_name ||
      file_name_info.timestamp != standard.timestamp ||
      file_name_info.distrib_count != standard.distrib_count ||
      file_name_info.distrib_index != standard.distrib_index ||
      file_name_info.processed_lines_count != standard.processed_lines_count
    )
    {
      std::cerr << "Failed parsing case " << i << ": "
        << test_cases[i].file_name << std::endl
        << "format:" << file_name_info.str_format() << std::endl
        << "base_name:" << file_name_info.base_name << std::endl
        << "distrib_count:" << file_name_info.distrib_count << std::endl
        << "distrib_index:" << file_name_info.distrib_index << std::endl
        << "timestamp:"
        << file_name_info.timestamp.get_gm_time().format("%Y%m%d.%H%M%S.%q")
        << std::endl << std::endl << std::endl;
    }
  }
}

void
test_parser()
{
  const char* valid_names[] =
  {
     "ColoUpdateStat.20120831.102438.736496.43242548.1.0",
     "ColoUpdateStat.log_1.0_20110504.045959.123456.12345678",
     "ColoUpdateStat.log_1.1_20101120.1290285094.17556678",
     "ColoUpdateStat.log_1.1_20101120.0.17556678",
     "ColoUpdateStat.log_1.1_20101120.2147483647.17556678",
     "ColoUpdateStat.log_1.1_20101120.9999999999.17556678", // overflow..
  };
  const char* invalid_names[] =
  {
     "ColoUpdateStat_20120831.102438.736496.43242548.1.0",
     "ColoUpdateStat.20120831.102438.736496.43242548.0x1.0",
     "ColoUpdateStat.20120831.102438.736496.43242548.1.FF",
     "ColoUpdateStat.201X0831.102438.736496.43242548.1.0",
     "ColoUpdateStat.20120831.102X38.736496.43242548.1.0",
     "ColoUpdateStat.20120831.102438.736X96.43242548.1.0",
     "ColoUpdateStat.20120831.102438.736496.4324X548.1.0",
     "ColoUpdateStat.20120831.102438.736496.43242548.X.0",
     "ColoUpdateStat.20120831.102438.736496.43242548.1.X",
     "ColoUpdateStat.log_1.1_20101120.1290285094.17556",
     "ColoUpdateStat.lo  1.0_20110504.045959.123456.12345678",
     "ColoUpdateStat.lo1.0_20110504.045959.123456.12345678",
     ".log_1.0_20110504.045959.123456.12345678",
     "",
     "__._..",
     "._..",
     "..",
     ".",
     "ColoUpdateStat.log_1.-_20110504.045959.123456.12345678",
     "ColoUpdateStat.log_1.0_20F10504.045959.123456.12345678",
     "ColoUpdateStat.log_1.0_20110504.04A959.123456.12345678",
     "ColoUpdateStat.log_1.0_20110504.045959.1234L6.12345678",
     "ColoUpdateStat.log_1.0_20110504.045959.123456.ED345678",
     "ColoUpdateStat.log_1.0_20110504.045959.123456.123ED678",
     "ColoUpdateStat.log_.0_20110504.045959.123456.12345678",
  };

  LogFileNameInfo file_name_info;

  std::cout << "Check positive tests" << std::endl;
  for (std::size_t i = 0;
    i < sizeof(valid_names) / sizeof(valid_names[0]); ++i)
  {
    try
    {
      parse_log_file_name(valid_names[i], file_name_info);
    }
    catch (const InvalidLogFileNameFormat&)
    {
      std::cerr << "Failed to parse a valid file name '"
                << valid_names[i] << "'" << std::endl;
    }
  }
  std::cout << "Check negative tests" << std::endl;
  for (std::size_t i = 0;
    i < sizeof(invalid_names) / sizeof(invalid_names[0]); ++i)
  {
    bool fails = false;
    try
    {
      parse_log_file_name(invalid_names[i], file_name_info);
    }
    catch (const InvalidLogFileNameFormat&)
    {
      fails = true;
      std::cout << "File name '" << invalid_names[i] << "' is invalid"
        << std::endl;
    }
    if (!fails)
    {
      std::cerr << "Parsed bad name: " << invalid_names[i] << std::endl;
    }
  }
}

FileReceiver_var
make_receiver(size_t max_files_to_store) noexcept
{
  return FileReceiver_var(
    new FileReceiver((working_folder+"Intermediate").c_str(),
      max_files_to_store, 0, 0));
}

int
run_test(unsigned long files_number)
{
  std::string TEST_NAME;

  {
    std::ostringstream tname;
    tname << "search_for_files(" << files_number << ")";
    tname.str().swap(TEST_NAME);
  }

  try
  {
    Logging::Logger_var logger(
      new Logging::OStream::Logger(Logging::OStream::Config(std::cerr)));
    logger->log_level(TraceLevel::MIDDLE);

    std::ostringstream cmd;
    cmd << "rm -rf " << working_folder << "; mkdir -p "
      << working_folder << "/Intermediate" << " && for i in {10000000.."
      << 10000000 + files_number - 1 <<
      "} ; do touch " << working_folder
      << "ColoUpdateStat.log_1.1_20101120.1290285094.$i ; done";

    std::cout << TEST_NAME << ": " << Generics::Time::get_time_of_day() <<
      ": To create files ..." << std::endl;
    system(cmd.str().c_str());

    std::cout << TEST_NAME << ": " << Generics::Time::get_time_of_day() <<
      ": To fetch files ..." << std::endl;

    FileReceiver_var file_receiver = make_receiver(500);
    std::string res;
    std::size_t counter = 0;
    FileReceiver::FileGuard_var file;

    Generics::Timer timer;
    Generics::CPUTimer cpu_timer;
    timer.start();
    cpu_timer.start();

    file_receiver->fetch_files(working_folder.c_str(), "ColoUpdateStat");
    while (file=file_receiver->get_eldest(res))
    {
      ++counter;
      unlink(file->full_path().c_str());
    }

    cpu_timer.stop();
    timer.stop();
    std::cout << TEST_NAME << ": " << Generics::Time::get_time_of_day() <<
      ": Files fetched: elapsed time = " << timer.elapsed_time() <<
      ", elapsed cpu time = " << cpu_timer.elapsed_time() << std::endl;

    if(counter != files_number)
    {
      std::cerr << TEST_NAME << ": fetched "
        << counter << " files instead "
        << files_number << std::endl;
      return 1;
    }
    return 0;
  }
  catch (const eh::Exception& ex)
  {
    std::cerr << ex.what() << std::endl;
  }

  return 1;
}

int
main(int argc, char* argv[]) noexcept
{
  int ret = 0;

  try
  {
    std::cout << "SearchFilesTest started.." << std::endl;
    if (!init(argc, argv))
    {
      return 0;
    }
    working_folder = *root_path + CHECK_ROOT;
    system(
      ("rm -rf " + working_folder + "; mkdir -p " + working_folder).c_str());

    prepare_test_cases();
    direct_test();
    test_parser();

    ret += run_test(100);
    ret += run_test(501);
    ret += run_test(1000);
/*    ret += run_test(10000);
    ret += run_test(100000);*/

    if (!ret)
    {
      std::string clean("rm -rf ");
      clean += working_folder;
      system(clean.c_str());
    }
  }
  catch (const eh::Exception& ex)
  {
    std::cerr << "Exception occurred:\nex.what() = \"" << ex.what() << "\"" << std::endl;
    ret *= -1;
  }
  catch (...)
  {
    std::cerr << "unknown exception" << std::endl;
    ret *= -1;
  }
  return ret;
}
