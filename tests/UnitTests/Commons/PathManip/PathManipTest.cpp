// @file PathManipTest.cpp

#include <iostream>

#include <eh/Exception.hpp>
#include <Commons/PathManip.hpp>
#include <HTTP/UrlAddress.hpp>

namespace
{
  // Test cases
  struct TestCase
  {
    const char* FROM;
    const char* TO;
    const char* RESULT;
  };

  struct NormalizeTestCase
  {
    const char* FROM;
    const char* RESULT;
  };

  const TestCase TEST_CASES[] =
  {
    {"file:///usr/include/", "file:///usr/", "../"},
    {"file:///usr/include/", "file:///usr", "../"},
    {"/usr/include/", "/usr/", "../"},
    {"/usr/include/", "/usr", "../"},
      // combine "with '/'", "without '/'" FROM path.
      // The final slash of FROM should not be significant!!! 
    {"file:///usr/include", "file:///usr/", "../"},
    {"file:///usr/include", "file:///usr", "../"},
    {"/usr/include", "/usr/", "../"},
    {"/usr/include", "/usr", "../"},

    {"file:///usr/include/", "file:///", "../../"},
    {"file:///usr/include", "file:///", "../../"},
    {"/usr/include/", "/", "../../"},
    {"/usr/include", "/", "../../"},

    {"file:///usr/include/", "file:///usr/file.xsl", "../file.xsl"},
    {"file:///usr/include", "file:///usr/file.xsl", "../file.xsl"},
    {"/usr/include/", "/usr/file.xsl", "../file.xsl"},
    {"/usr/include", "/usr/file.xsl", "../file.xsl"},

    {"file:///usr/include/", "file:///usr/folder/", "../folder/"},
    {"file:///usr/include/", "file:///usr/folder", "../folder"},
    {"/usr/include/", "/usr/folder/", "../folder/"},
    {"/usr/include/", "/usr/folder", "../folder"},

    {"file:///usr/include", "file:///usr/folder/", "../folder/"},
    {"file:///usr/include", "file:///usr/folder", "../folder"},
    {"/usr/include", "/usr/folder/", "../folder/"},
    {"/usr/include", "/usr/folder", "../folder"},

    {"file:///", "file:///usr/folder/", "./usr/folder/"},
    {"file:///", "file:///usr/folder", "./usr/folder"},
    {"/", "/usr/folder/", "./usr/folder/"},
    {"/", "/usr/folder", "./usr/folder"},

    {"file:///usr/", "file:///usr/folder/", "./folder/"},
    {"file:///usr/", "file:///usr/folder", "./folder"},
    {"/usr/", "/usr/folder/", "./folder/"},
    {"/usr/", "/usr/folder", "./folder"},

    {"file:///usr", "file:///usr/folder/", "./folder/"},
    {"file:///usr", "file:///usr/folder", "./folder"},
    {"/usr", "/usr/folder/", "./folder/"},
    {"/usr", "/usr/folder", "./folder"},

    {"file:///home/user/folder/", "file:///usr/folder/",
      "../../../usr/folder/"},
    {"file:///home/user/folder/", "file:///usr/folder",
      "../../../usr/folder"},
    {"/home/user/folder/", "/usr/folder/", "../../../usr/folder/"},
    {"/home/user/folder/", "/usr/folder", "../../../usr/folder"},

    {"file:///home/user/folder", "file:///usr/folder/",
      "../../../usr/folder/"},
    {"file:///home/user/folder", "file:///usr/folder",
      "../../../usr/folder"},
    {"/home/user/folder", "/usr/folder/", "../../../usr/folder/"},
    {"/home/user/folder", "/usr/folder", "../../../usr/folder"},

    {"file:///usr/folder/", "file:///home/user/folder/",
      "../../home/user/folder/"},
    {"file:///usr/folder/", "file:///home/user/folder",
      "../../home/user/folder"},
    {"/usr/folder/", "/home/user/folder/", "../../home/user/folder/"},
    {"/usr/folder/", "/home/user/folder", "../../home/user/folder"},

    {"file:///usr/folder", "file:///home/user/folder/",
      "../../home/user/folder/"},
    {"file:///usr/folder", "file:///home/user/folder",
      "../../home/user/folder"},
    {"/usr/folder", "/home/user/folder/", "../../home/user/folder/"},
    {"/usr/folder", "/home/user/folder", "../../home/user/folder"},

    {"file:///usr/folder/", "file:///usr/folder/", "./"},
// bad, inconsistent input data!
    {"file:///usr/folder/", "file:///usr/folder", ""/*"../folder" ?*/},
    {"/usr/folder/", "/usr/folder/", "./"},
    {"/usr/folder/", "/usr/folder", ""/*"../folder" ?*/},

    {"file:///usr/folder", "file:///usr/folder/", "./"},
    {"file:///usr/folder", "file:///usr/folder", "./"/*"../folder" ?*/}, 
    {"/usr/folder", "/usr/folder/", "./"},
    {"/usr/folder", "/usr/folder", "./"/*"../folder" ?*/},

    {"file:///usr/", "file:///usr/folder/files/", "./folder/files/"},
    {"file:///usr/", "file:///usr/folder/files", "./folder/files"},
    {"/usr/", "/usr/folder/files/", "./folder/files/"},
    {"/usr/", "/usr/folder/files", "./folder/files"},

    {"/", "/usr/folder/files", "./usr/folder/files"},
    {"/", "/", "./"},

  };

// negative test cases should return errors 
  const TestCase NEGATIVE_TEST_CASES[] =
  {
// suspect
    {"", "", ""}, // good ?
// badness
    {"", "/usr/folder/files", ""},
    {"", "/", ""},
    {"/", "", ""},
    {"/usr/folder/files", "", ""},
    {"/usr/folder/files/", "", ""},
  };

  const NormalizeTestCase NORMALIZE_TEST_CASES[] =
  {
    {"/usr/include/../", "/usr"},
    {"/usr/include/..", "/usr"},
    {"/usr/include/./", "/usr/include"},
    {"/usr/include/.", "/usr/include"},
    {"/usr//include/", "/usr/include"},
    {"/usr//include", "/usr/include"},
    {"/usr//include/sub/./../../", "/usr"},
    {"/usr//include/../../", "/"},
    {"usr/include/../", "usr"},
    {"usr/include/..", "usr"},
    {"usr/include/./", "usr/include"},
    {"usr/include/.", "usr/include"},
    {"usr//include/", "usr/include"},
    {"usr//include", "usr/include"},
    {"usr//include/sub/./../../", "usr"},
    {"usr//include/../../", ""},
  };

  const NormalizeTestCase NEGATIVE_NORMALIZE_TEST_CASES[] =
  {
    {"../", ""},
    {"/usr/include/../../..", ""},
    {"/usr/include/../../../", ""},
    {"/usr/include/../../../t", ""},
    {"usr/include/../../..", ""},
    {"usr/include/../../../", ""},
    {"usr/include/../../../t", ""},
  };
}
 
using namespace AdServer::PathManip;

int do_relative_path_test() noexcept
{
  int result_code = 0;

  // positive tests
  for (std::size_t i = 0;
    i < sizeof(TEST_CASES) / sizeof(TEST_CASES[0]);
    ++i)
  {
    std::string result;
    const TestCase& test_case = TEST_CASES[i];
    if (!relative_path(test_case.FROM, test_case.TO, result) ||
      result != test_case.RESULT)
    {
      ++result_code;

      std::cerr << "Test case " << i << " failed. FROM: " <<
        test_case.FROM << " ---> " << test_case.TO <<
        "\nCorrect result:\n" <<
        test_case.RESULT << "\nReal result:\n" << result << std::endl;
    }
    else
    {
      std::cout << "Restore: " << result << std::endl;
    }
  }

  // negative tests
  for (std::size_t i = 0;
    i < sizeof(NEGATIVE_TEST_CASES) / sizeof(NEGATIVE_TEST_CASES[0]);
    ++i)
  {
    std::string result;
    const TestCase& test_case = NEGATIVE_TEST_CASES[i];
    if (relative_path(test_case.FROM, test_case.TO, result) ||
        !result.empty())
    {
      ++result_code;

      std::cerr << "Negative test case " << i << " successful. FROM: " <<
        test_case.FROM << " ---> " << test_case.TO <<
        "Result:\n" << result << std::endl;
    }
  }

  return result_code;
}

int do_normalize_path_test() noexcept
{
  int result_code = 0;

  // positive tests
  for (std::size_t i = 0;
    i < sizeof(NORMALIZE_TEST_CASES) / sizeof(NORMALIZE_TEST_CASES[0]);
    ++i)
  {
    const NormalizeTestCase& test_case = NORMALIZE_TEST_CASES[i];
    std::string result = test_case.FROM;
    if (!normalize_path(result) || result != test_case.RESULT)
    {
      ++result_code;

      std::cerr << "Normalize test case " << i << " failed: " <<
        test_case.FROM << " ---> " << result <<
        ", correct result: " <<
        test_case.RESULT << std::endl;
    }
    else
    {
      std::cout << "Success normalized: " <<
        test_case.FROM << " ---> " << result << std::endl;
    }
  }

  // negative tests
  for (std::size_t i = 0;
    i < sizeof(NEGATIVE_NORMALIZE_TEST_CASES) /
      sizeof(NEGATIVE_NORMALIZE_TEST_CASES[0]); ++i)
  {
    const NormalizeTestCase& test_case = NEGATIVE_NORMALIZE_TEST_CASES[i];
    std::string result = test_case.FROM;
    if(normalize_path(result))
    {
      ++result_code;

      std::cerr << "Negative normalize test case failed: " <<
        test_case.FROM << std::endl;
    }
    else
    {
      std::cout << "Negative normalize test success: " <<
        test_case.FROM << std::endl;
    }
  }

  return result_code;
}

int main() noexcept
{
  int res = 0;
  res += do_relative_path_test();
  res += do_normalize_path_test();
  return res;
}
