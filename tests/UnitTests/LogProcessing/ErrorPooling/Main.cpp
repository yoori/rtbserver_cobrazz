// @file ErrorPooling/Main.cpp

#include <iostream>
#include <sstream>
#include <Logger/StreamLogger.hpp>
#include <Generics/Rand.hpp>
#include <TestCommons/MTTester.hpp>

#include <SyncLogs/Utils.hpp>
#include <Frontends/Modules/BiddingFrontend/GroupLogger.hpp>

using namespace AdServer::LogProcessing::Utils;
using namespace AdServer::Bidding;

namespace
{
  const char STANDARD[] = "Aggregate 100 errors, descriptions: fun():"
    " some error,some error,some error,some error,some error,some error,"
    "some error,some error,some error,some error,some error,...[cut]...\n";
  const char* FRONTEND_STANDARDS[] =
  {
    "interrupt at two 0.00 (sec), 5 requests",
    "interrupt at two 1 (sec), 10 requests",
    "interrupt at one 2 (sec), 5 requests",
    "interrupt at one 12 (sec), 5 requests",
    "interrupt at one 0.00 (sec), 5 requests",
    "interrupt at one 0.50 (sec), 5 requests",
    "interrupt at two 0.50 (sec), 5 requests",
    "interrupt at two 0.01 (sec), 15 requests",
    "interrupt at one 1 (sec), 10 requests",
    "interrupt at one 0.01 (sec), 15 requests",
    "interrupt at two 2 (sec), 5 requests",
    "interrupt at two 12 (sec), 5 requests",
  };

  const char STANDARD_LOG_MESSAGE[] = "Log message";
}

class ErrorGenerator
{
public:
  ErrorGenerator(ErrorPool& errors) noexcept;

  void
  operator ()() noexcept;

private:
  std::string src_host_, dst_host_, src_dir_;
  ErrorPool& errors_;
};

class LogGenerator
{
public:
  LogGenerator(ErrorPool& errors) noexcept;

  void
  operator ()() noexcept;

private:
  ErrorPool& errors_;
};

class CellsErrorGenerator
{
public:
  CellsErrorGenerator(ErrorPool& errors) noexcept;

  void
  operator ()() noexcept;

private:
  std::string dst_host_, src_dir_;
  ErrorPool& errors_;
  volatile _Atomic_word count_;
};

ErrorGenerator::ErrorGenerator(ErrorPool& errors) noexcept
  : errors_(errors)
{
}

void
ErrorGenerator::operator ()() noexcept
{
  if (Generics::safe_rand() % 2)
  {
    *errors_.add_error(src_host_, dst_host_, src_dir_, String::SubString(),
      String::SubString("fun(): ")) << "some error";
  }
  else
  {
    Stream::Error ostr;
    ostr << "some error";
    errors_.add_error(src_host_, dst_host_, src_dir_,
      String::SubString(), String::SubString("fun(): "), ostr.str());
  }
}

LogGenerator::LogGenerator(ErrorPool& errors) noexcept
  : errors_(errors)
{
}

void
LogGenerator::operator ()() noexcept
{
  Stream::Error ostr;
  ostr << STANDARD_LOG_MESSAGE;
  errors_.log(ostr.str(), 0, 0, 0);
}

CellsErrorGenerator::CellsErrorGenerator(ErrorPool& errors) noexcept
  : errors_(errors),
    count_(0)
{
}

void
CellsErrorGenerator::operator ()() noexcept
{
  _Atomic_word old = __gnu_cxx::__exchange_and_add(&count_, 1);
  if (old == 5000)
  {
    errors_.dump(Logging::Logger::INFO, "Test flush");
  }
  std::string host;
  char rnd = Generics::safe_rand(0, 9);
  host += char('0' + rnd);
  if (rnd % 2)
  {
    *errors_.add_error(host, dst_host_, src_dir_, String::SubString(),
      String::SubString("fun(): ")) << (old > 5000 ? "new fail" : "kind error");
  }
  else
  {
    Stream::Error ostr;
    ostr << (old > 5000 ? "new fail" : "kind error");
    errors_.add_error(host, dst_host_, src_dir_,
      String::SubString(), String::SubString("fun(): "), ostr.str());
  }
}

void
frontend_group_logger_test() noexcept
{
  const char FUN[] = "frontend_group_logger_test()";
  const char* messages[] = {"interrupt at one", "interrupt at two"};
  const timeval times[] = { {1, 0}, {2, 0}, {12, 0},
    {0, 999999}, {0, 500000}, {0, 10000}, {0, 9999}, {0, 9}, {0, 0}
  };
  Logging::Logger_var logger(
    new Logging::OStream::Logger(Logging::OStream::Config(std::cout)));
  GroupLogger_var gl(new GroupLogger(logger, "prefix", "aspect", "code"));

  std::streambuf* cout_buf = std::cout.rdbuf();
  std::stringbuf my_buf;
  std::cout.rdbuf(&my_buf);

  for (std::size_t iters = 0; iters < 5; ++iters)
  {
    for (unsigned i = 0; i < sizeof(messages) / sizeof(messages[0]); i++)
    {
      for (unsigned j = 0; j < sizeof(times) / sizeof(times[0]); ++j)
      {
        gl->add_error("host.com",
          String::SubString(messages[i]), Generics::Time(times[j]),
          Logging::Logger::ERROR, "local_asp", "lcode");
      }
    }
  }
  gl->dump();
  std::cout.rdbuf(cout_buf);
  const std::string& result = my_buf.str();
  for (std::size_t i = 0;
    i < sizeof(FRONTEND_STANDARDS) / sizeof(FRONTEND_STANDARDS[0]); ++i)
  {
    std::string::size_type pos = result.find(FRONTEND_STANDARDS[i]);
    if (pos == std::string::npos)
    {
      std::cerr << "FAIL: " << i << " incorrect grouping" << std::endl;
      std::cerr << "Result is: " << result << std::endl;
      break;
    }
  }

  std::cout << "Individuals test" << std::endl;
  std::string ostr(FUN);
  ostr += ": interrupted at starting, after";

  gl->add_error("host.com", ostr, Generics::Time(12), Logging::Logger::ERROR, "loc_asp", "lcode");
  gl->add_error("host.com", ostr, Generics::Time(12), Logging::Logger::ERROR, "loc_asp", "lcode");
  gl->add_error("host.com", ostr, Generics::Time(12), Logging::Logger::ERROR, "loca_asp", "lcode");
  gl->add_error("host.com", ostr, Generics::Time(12), Logging::Logger::ERROR, "local_asp", "lcode");
  gl->add_error("host.com", ostr, Generics::Time(12), Logging::Logger::ERROR, "local_asp", "l_code");
}


int
main()
{
  std::streambuf* cout_buf = std::cout.rdbuf();
  std::stringbuf my_buf;
  std::cout.rdbuf(&my_buf);

  {
    Logging::Logger_var logger(
      new Logging::OStream::Logger(Logging::OStream::Config(std::cout)));
    ErrorPool errors(128, logger);
    ErrorGenerator worker(errors);
    TestCommons::MTTester<ErrorGenerator&> mt_tester(
      worker, 10);

    mt_tester.run(100, 0, 100);
  }
  std::stringbuf my_buf2;
  std::cout.rdbuf(&my_buf2);
  {
    Logging::Logger_var logger(
      new Logging::OStream::Logger(Logging::OStream::Config(std::cout)));
    ErrorPool errors(128, logger);
    LogGenerator worker(errors);
    TestCommons::MTTester<LogGenerator&> mt_tester(
      worker, 10);

    mt_tester.run(10, 0, 10);
  }

  std::stringbuf my_buf3;
  std::cout.rdbuf(&my_buf3);
  {
    Logging::Logger_var logger(
      new Logging::OStream::Logger(Logging::OStream::Config(std::cout)));
    ErrorPool errors(128, logger);
    CellsErrorGenerator worker(errors);
    TestCommons::MTTester<CellsErrorGenerator&> mt_tester(
      worker, 10);

    mt_tester.run(10000, 0, 10000);
  }


  // Checking results
  std::cout.rdbuf( cout_buf );

  std::string result(my_buf.str());
  std::string::size_type pos = result.find("Aggregate");
  if (pos == std::string::npos)
  {
    pos = 0;
  }
  if (result.substr(pos) == STANDARD)
  {
    std::cout << my_buf.str() << std::endl;
    std::cout << "SUCCESS" << std::endl;
  }
  else
  {
    std::cerr << "Substring:'" << result.substr(pos) << "'" << std::endl;
    std::cerr << my_buf.str() << std::endl;
    std::cerr << "FAIL" << std::endl;
  }

  std::string result2(my_buf2.str());
  std::string std2(STANDARD_LOG_MESSAGE);
  pos = 0;
  for (std::size_t i = 0; i < 10; ++i, ++pos)
  {
    pos = result2.find(std2, pos);
    if (pos == std::string::npos)
    {
      std::cerr << "Log test FAIL" << std::endl;
      break;
    }
  }

  // Please, check that sum of errors counter = 10000
  std::cout << my_buf3.str() << std::endl;
  frontend_group_logger_test();

  return 0;
}
