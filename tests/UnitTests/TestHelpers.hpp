
#include <cmath>
#include <iostream>
#include <limits>
#include <string>
#include <sstream>
#include <stdexcept>
#include <set>

struct TestCase
{
  void (*test)();
  void (*setup)();
  void (*teardown)();
  std::string name;
};

bool
operator< (const TestCase& test1, const TestCase& test2) noexcept;

class TestFactory
{
public:
  typedef std::set<TestCase> Tests;

public:
  static TestFactory&
  instance() noexcept;

  void
  register_test(const TestCase& test_case) /*throw(std::exception)*/;

  int
  run_all() /*throw(std::exception)*/;

  int
  run(const std::string& test_name) /*throw(std::exception)*/;

  const Tests&
  available_tests() const noexcept;

private:
  Tests registered_tests_;

private:
  bool
  run_test_(const TestCase& test_case, std::string& msg) /*throw(std::exception)*/;

  int
  run_tests_(const Tests& tests) /*throw(std::exception)*/;

};

bool
operator< (const TestCase& test1, const TestCase& test2) noexcept
{
  return (test1.name < test2.name);
}


TestFactory&
TestFactory::instance() noexcept
{
  static TestFactory instance_;
  return instance_;
}

void
TestFactory::register_test(const TestCase& test_case) 
  /*throw(std::exception)*/
{
  registered_tests_.insert(test_case);
}

int
TestFactory::run_all() /*throw(std::exception)*/
{
  return run_tests_(registered_tests_);
}

int
TestFactory::run(const std::string& test_name) /*throw(std::exception)*/
{
  for (Tests::const_iterator i = registered_tests_.begin();
       i != registered_tests_.end(); ++i)
  {
    if (i->name == test_name)
    {
      Tests tests;
      tests.insert(*i);
      return run_tests_(tests);
    }
  }
  
  std::cout << "Test '" << test_name << "' not found" << std::endl;
  return 1;
}

int
TestFactory::run_tests_(const Tests& tests) /*throw(std::exception)*/
{
  size_t successed = 0;
  std::string msg;

  for (Tests::const_iterator ci = tests.begin();
       ci != tests.end(); ++ci)
  {
    std::ostringstream oss;
    oss << ci->name << ": ";
    
    const bool result = run_test_(*ci, msg);
    oss << msg;

    if (result)
    {
      ++successed;
      std::cout << oss.str() << std::endl;
    }
    else
    {
      std::cerr << oss.str() << std::endl;
    }
  }
  
  if (successed == tests.size())
  {
    std::cout << "OK (" << successed << ")" << std::endl;
    return 0;
  }

  std::cerr << "FAILED " << (tests.size() - successed) << " from " << tests.size() << std::endl;
  return (tests.size() - successed);
}

const TestFactory::Tests&
TestFactory::available_tests() const noexcept
{
  return registered_tests_;
}


class TestFailed : public std::runtime_error
{
public:
  TestFailed(const std::string& msg) noexcept;
};

TestFailed::TestFailed(const std::string& msg) noexcept
  : std::runtime_error(msg)
{}

bool
TestFactory::run_test_(const TestCase& test_case, std::string& msg)
  /*throw(std::exception)*/
{
  msg = "OK";
  bool result = false;

  try
  {
    (*test_case.setup)();
    (*test_case.test)();
    result = true;
  }
  catch (TestFailed& ex)
  {
    msg = ex.what();
  }
  catch (std::exception& ex)
  {
    msg = "std::exception was thrown: ";
    msg += ex.what();
  }
  catch (...)
  {
    msg = "unknown exception was thrown";
  }

  try
  {
    (*test_case.teardown)();
  }
  catch (...)
  {}

  return result;
}

std::string
pretty_file_name(std::string file_name)
  
/*throw(std::exception)*/{
  const std::string usless_prefix = "UnitTests/";
  const size_t pos = file_name.find(usless_prefix);
  
  if (pos != std::string::npos)
  {
    file_name = file_name.substr(pos + usless_prefix.length());
  }

  return file_name;
}

std::string
pretty_test_name(std::string file_name, const std::string& test_name)
  /*throw(std::exception)*/
{
  const size_t slash_pos = file_name.rfind('/');

  if (slash_pos != std::string::npos)
  {
    file_name = file_name.substr(slash_pos + 1);
  }

  const size_t ext_pos = file_name.rfind('.');

  if (ext_pos != std::string::npos)
  {
    file_name = file_name.substr(0, ext_pos);
  }

  return (file_name + "::" + test_name);
}

void
empty_setup() noexcept
{}

void
empty_teardown() noexcept
{}

void
usage()
{
  std::cout << "Usage:\n\tTestApp [test_case]" << std::endl;
  const TestFactory::Tests& tests = TestFactory::instance().available_tests();
  std::cout << "Available test cases:" << std::endl;
  
  for (TestFactory::Tests::const_iterator i = tests.begin();
       i != tests.end(); ++i)
  {
    std::cout << '\t' << i->name << std::endl;
  }
}

int
main_(int argc, const char* const argv[])
{
  if (argc > 1)
  {
    const std::string opt(argv[1]);

    if (opt == "?" || opt == "-h" || opt == "--help")
    {
      usage();
      return 0;
    }
    else
    {
      return TestFactory::instance().run(opt);
    }
  }
  
  return TestFactory::instance().run_all();
}

template<typename T1, typename T2>
bool
compare(const T1& arg1, const T2& arg2)
{
  return (arg1 == arg2);
}

template<>
bool
compare(const double& arg1, const double& arg2)
{
  return std::fabs(arg1 - arg2) < std::numeric_limits<double>::epsilon();
}

#define TEST_EX(test_name, setup, teardown) \
  void test_name##_test(); \
  struct test_name##_registrator_t { test_name##_registrator_t(){ \
  const TestCase test_case = {test_name##_test, setup, teardown, #test_name}; \
  TestFactory::instance().register_test(test_case); } \
  } test_name##_registrator; \
  void test_name##_test() \

#define TEST(test_name) TEST_EX(test_name, empty_setup, empty_teardown)

#define PRETTY_FILE_NAME pretty_file_name(__FILE__)

#define ASSERT_TRUE(cond) if ( !(cond) ) { std::ostringstream oss; oss << "FAILED: " << PRETTY_FILE_NAME \
  << ", line " << __LINE__ << ": actual value of '" << #cond << "' is false"; throw TestFailed(oss.str()); } \
  
#define ASSERT_FALSE(cond) if ( cond ) { std::ostringstream oss; oss << "FAILED: " << PRETTY_FILE_NAME \
  << ", line " << __LINE__ << ": actual value of '" << #cond << "' is true"; throw TestFailed(oss.str()); } \

#define ASSERT_EQUALS(arg1, arg2) if ( !compare(arg1, arg2) ) { std::ostringstream oss; oss << "FAILED: " << PRETTY_FILE_NAME \
  << ", line " << __LINE__ << ": '" << arg1 << "' != '" << arg2 << "'"; throw TestFailed(oss.str()); } \

#define FAIL(msg) { std::ostringstream oss; oss << "FAILED: " << PRETTY_FILE_NAME \
  << ", line " << __LINE__ << ": " << msg; throw TestFailed(oss.str()); } \

#define RUN_TESTS int main(int argc, const char* const argv[]) { return main_(argc, argv); }
