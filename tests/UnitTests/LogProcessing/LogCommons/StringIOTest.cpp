/// @file StringIOTest.cpp
#include <iostream>
#include <LogCommons/LogCommons.hpp>
#include <LogCommons/Request.hpp>
#include <String/UTF8Tables.hpp>

using namespace AdServer::LogProcessing;

/**
 * Check Request.hpp UserProperty (std::pair<StringIO, StringIO>) input
 * reading empty strings, example "Client=PS,OsVersion=,BrowserVersion=\t"
 */
int
empty_separated_string()
{
  /*
   *  Sergey Vetoshkin:
   *  UserProperties is the part of Request log.
   *  Where we read value: UserPropertiesList (separated from other by \t
   *  and each element in UserPropertiesList separated by Comma
   *  see: 3.5.0 LogCommons.ipp: 
   *  InputArchive::operator &
   *  operator >>(std::istream& is, std::list<ValueType>& values)
   *  So all test cases with ',' '\t' in TestCase::in below
   *  wrong and impossible in real files IMO.
   *  Such as broke Request log format
   *  Switched off in ADSC-9944
  */ 

  struct TestCase
  {
    const char* in;
    const char* first;
    const char* second;
  };
  const TestCase READ_MUST_FAIL[] =
  {
    // {",,", ",,", ""},
    // {",", ",", ""},
    // {",\t", ",", "nil"},
    // {"T,", "T,", ""},
    // {"T,\t", "T,", "nil"},
    {"T", "", ""},
    // {"T\t", "T", "nil"},
    {"=T", "", ""},
    {"=", "", ""},
    {"T1=T2=T3", "T1", "T2"},
  };

  const TestCase VALID[] =
  {
    //{",=", ",", ""},
    //{"", "", ""},
    //{"\t,", "", ""},
    //{"\t", "", ""},
    //{"\t\t,", "", ""},
    //{"\t\t", "", ""},
    //{"\t=,", "", ""},
    //{"\t=", "", ""},
    //{"\t=T,", "", ""},
    //{"\t=T", "", ""},
    //{"\tT,", "", ""},
    //{"\tT", "", ""},
    //{"\tT=,", "", ""},
    //{"\tT=T,", "", ""},
    //{"\tT=T", "", ""},
    //{"=,", "", ""},
    //{"=,\t", "", ""},
    //{"=", "", ""},
    //{"=\t,=", "", ""},
    //{"=\t", "", ""},
    //{"=\t=", "", ""},
    //{"==", "", "="},
    //{"=T,", "", "T"},
    //{"=T", "", "T"},
    //{"T=,", "T", ""},
    //{"T=,\t", "T", ""},
    {"N=", "N", ""},
    {" N = ", "N", ""},

    //{"T=\t", "T", ""},
    //{"T=T,", "T", "T"},
    //{"T=T,\t", "T", "T"},
    {"N=V", "N", "V"},
    //{"T=T\t", "T", "T"},
  };

  int fails = 0;
  UserProperty up;
  for (std::size_t i = 0;
    i < sizeof(READ_MUST_FAIL) / sizeof(READ_MUST_FAIL[0]); ++i)
  {
    FixedBufStream<CommaCategory> is (String::SubString(READ_MUST_FAIL[i].in));
    up = UserProperty("nil", "nil");
    is >> up;
    if (!is.fail() || up.first != READ_MUST_FAIL[i].first ||
      up.second != READ_MUST_FAIL[i].second)
    {
      std::cerr << "pass or broken read invalid string: \'" << READ_MUST_FAIL[i].in << "\'" << std::endl;
      std::cerr << "Stream state: " << is.fail()
        << ", first: '" << static_cast<std::string>(up.first) << "':"
        << up.first.size()
        << ", second: '" << static_cast<std::string>(up.second) << "':"
        << up.second.size() << std::endl;
      std::cerr << "Expected: " 
        << "first: '" << static_cast<std::string>(READ_MUST_FAIL[i].first) << "':"
        << static_cast<std::string>(READ_MUST_FAIL[i].first).size()
        << ", second: '" << static_cast<std::string>(READ_MUST_FAIL[i].second) << "':"
        << static_cast<std::string>(READ_MUST_FAIL[i].second).size() << std::endl << std::endl;
      ++fails;
    }
  }
  for (std::size_t i = 0;
    i < sizeof(VALID) / sizeof(VALID[0]); ++i)
  {
    FixedBufStream<CommaCategory> is (String::SubString(VALID[i].in));
    up = UserProperty("nil", "nil");
    is >> up;
    if (is.fail() || up.first != VALID[i].first ||
      up.second != VALID[i].second)
    {
      std::cerr << "Fail read string \'" << VALID[i].in << "\'" << std::endl;
      std::cerr << "Stream state: " << is.fail()
        << ", first: '" << static_cast<std::string>(up.first) << "':"
        << up.first.size()
        << ", second: '" << static_cast<std::string>(up.second) << "':"
        << up.second.size() << std::endl;
      std::cerr << "Expected: " 
        << "first: '" << static_cast<std::string>(VALID[i].first) << "':"
        << static_cast<std::string>(VALID[i].first).size()
        << ", second: '" << static_cast<std::string>(VALID[i].second) << "':"
        << static_cast<std::string>(VALID[i].second).size() << std::endl << std::endl;
      ++fails;
    }
  }
  return fails;
}

void
print_reserved_chars_table(const char* reserved)
{
  std::cout.setf(std::ios::hex | std::ios::uppercase);
  std::cout << "{\n  " << std::hex << std::setfill('0');
  typedef std::ctype<char> CType;
  const CType& ctype = std::use_facet<CType>(std::cout.getloc());

  unsigned char ch = 0;
  do
  {
    if (strchr(reserved, ch) || ctype.is(std::ctype_base::space, (char)ch))
    {
      char buf[32];
      {
        Stream::Buffer<sizeof(buf)> os(buf);
        os.setf(std::ios::hex | std::ios::uppercase);
        os.width(2);
        os << std::hex << std::setfill('0') << static_cast<unsigned>(ch);
      }
      std::cout << "{'" << buf[0] << "', '" <<
        buf[1] << "'}, ";
    }
    else
    {
      std::cout << "{0, 0}, ";
    }
    if (!((ch + 1) % 6))
    {
      std::cout << "\n  ";
    }
  }
  while (ch++ != 127);
  std::cout << "\n};" << std::endl;
}

template <typename String>
int
direct_test_io_escapes()
{
  String s("# #,\t#:=,#"), restored;

  Stream::Stack<1024> os;
  os << s;
  if (os.str() != "#^20#^2C^09#^3A^3D^2C#")
  {
    std::cerr << __PRETTY_FUNCTION__ << "Output binary string failed, result:"
      << os.str() << "." << std::endl;
    return 1;
  }
  Stream::Parser is(os.str().data(), os.str().size());
  is >> restored;
  if (!is || restored != s)
  {
    std::cerr << __PRETTY_FUNCTION__ << "read failed" << std::endl;
    return 1;
  }
  return 0;
}

namespace
{
  struct AllConvert
  {
    static const String::Detail::CodeUnit2Bytes
      WRITE_RESERVED_CHAR_TABLE[128];
  };

  const String::Detail::CodeUnit2Bytes
    AllConvert::WRITE_RESERVED_CHAR_TABLE[128] =
  {
    {'0', '0'}, {'0', '1'}, {'0', '2'}, {'0', '3'}, {'0', '4'}, {'0', '5'},
    {'0', '6'}, {'0', '7'}, {'0', '8'}, {'0', '9'}, {'0', 'A'}, {'0', 'B'},
    {'0', 'C'}, {'0', 'D'}, {'0', 'E'}, {'0', 'F'}, {'1', '0'}, {'1', '1'},
    {'1', '2'}, {'1', '3'}, {'1', '4'}, {'1', '5'}, {'1', '6'}, {'1', '7'},
    {'1', '8'}, {'1', '9'}, {'1', 'A'}, {'1', 'B'}, {'1', 'C'}, {'1', 'D'},
    {'1', 'E'}, {'1', 'F'}, {'2', '0'}, {'2', '1'}, {'2', '2'}, {'2', '3'},
    {'2', '4'}, {'2', '5'}, {'2', '6'}, {'2', '7'}, {'2', '8'}, {'2', '9'},
    {'2', 'A'}, {'2', 'B'}, {'2', 'C'}, {'2', 'D'}, {'2', 'E'}, {'2', 'F'},
    {'3', '0'}, {'3', '1'}, {'3', '2'}, {'3', '3'}, {'3', '4'}, {'3', '5'},
    {'3', '6'}, {'3', '7'}, {'3', '8'}, {'3', '9'}, {'3', 'A'}, {'3', 'B'},
    {'3', 'C'}, {'3', 'D'}, {'3', 'E'}, {'3', 'F'}, {'4', '0'}, {'4', '1'},
    {'4', '2'}, {'4', '3'}, {'4', '4'}, {'4', '5'}, {'4', '6'}, {'4', '7'},
    {'4', '8'}, {'4', '9'}, {'4', 'A'}, {'4', 'B'}, {'4', 'C'}, {'4', 'D'},
    {'4', 'E'}, {'4', 'F'}, {'5', '0'}, {'5', '1'}, {'5', '2'}, {'5', '3'},
    {'5', '4'}, {'5', '5'}, {'5', '6'}, {'5', '7'}, {'5', '8'}, {'5', '9'},
    {'5', 'A'}, {'5', 'B'}, {'5', 'C'}, {'5', 'D'}, {'5', 'E'}, {'5', 'F'},
    {'6', '0'}, {'6', '1'}, {'6', '2'}, {'6', '3'}, {'6', '4'}, {'6', '5'},
    {'6', '6'}, {'6', '7'}, {'6', '8'}, {'6', '9'}, {'6', 'A'}, {'6', 'B'},
    {'6', 'C'}, {'6', 'D'}, {'6', 'E'}, {'6', 'F'}, {'7', '0'}, {'7', '1'},
    {'7', '2'}, {'7', '3'}, {'7', '4'}, {'7', '5'}, {'7', '6'}, {'7', '7'},
    {'7', '8'}, {'7', '9'}, {'7', 'A'}, {'7', 'B'}, {'7', 'C'}, {'7', 'D'},
    {'7', 'E'}, {'7', 'F'},
  };
}

template <typename String>
int
test_io()
{
  char all_chars[256];
  for (std::size_t i = 0; i < 256; ++i)
  {
    all_chars[i] = i;
  }
  String s(std::string(all_chars, sizeof(all_chars))), restored;

  Stream::Stack<1024> os;
  os << s;
  Stream::Parser is(os.str().data(), os.str().size());
  is >> restored;
  restored += '\xFF';
  if (!is || restored != s)
  {
    std::cerr << __PRETTY_FUNCTION__ << ": restore string failed"
      << std::endl;
    std::cerr << "OUT:" << (std::string)s << std::endl;
    std::cerr << "GET:" << (std::string)restored << std::endl;
    return 1;
  }
  return 0;
}

template <typename String>
int
test_separator()
{
  static const String STRINGS[] =
  {
    "\xFFtext", "text\xFF", "te\xFFxt",
  };
  static const String STANDARDS[] =
  {
    "", "text", "te",
  };
  String restored;
  int fails = 0;

  for (std::size_t i = 0; i < sizeof(STRINGS) / sizeof(STRINGS[0]); ++i)
  {
    Stream::Stack<1024> os;
    os << STRINGS[i];
    Stream::Parser is(os.str().data(), os.str().size());
    is >> restored;
    if (i == 0 && is.fail())
    {
      continue;
    }
    if (!is || restored != STANDARDS[i])
    {
      std::cerr << __PRETTY_FUNCTION__ << ": Test Separator failed"
        << std::endl;
      std::cerr << "OUT:" << (std::string)STRINGS[i] << std::endl;
      std::cerr << "REZ:" << os.str() << std::endl;
      std::cerr << "GET:" << (std::string)restored << std::endl;
      ++fails;
    }
  }
  return fails;
}

int
stress_test()
{
  struct StressCase
  {
    const char* in;
    const String::SubString rest_after_out;
  };
  const String::SubString EMPTY("");
  static const StressCase STRESS[] =
  {
    {"^9\t7", EMPTY},
    {"^ 9\t7", EMPTY},
    {"^ z\t7", EMPTY},
    {"^z9 7", String::SubString("\09", 2)},
    {"^  7", EMPTY},
    {"^.F7 ^", String::SubString("\0F7", 3)},
    {"^^", String::SubString("\0", 1)},
    {"^^^", String::SubString("\0", 1)},
    {"^^^^", String::SubString("\0\0", 2)},
    {"^7z ^^", String::SubString("p")},
    {"^7zz ^^", String::SubString("pz")},
    {"^-", String::SubString("\0", 1)},
    {"^--", String::SubString("\0-", 2)},
    {"^---", String::SubString("\0--", 3)},
    {"^0.", String::SubString("\0", 1)},
    {"^0^.", String::SubString("\0.", 2)},
    {"^7z -", String::SubString("p")},
    {"^z^^-^^", String::SubString("\0\0-\0", 4)},
    {"^z^^ ^^", String::SubString("\0\0", 2)},
    {"^z^^^ ^^", String::SubString("\0\0", 2)},
    {"^z^^8^ ^^", String::SubString("\0\08", 3)},
    {"^7-^--^%5^4^\t^^20*", String::SubString("p\0-\0005@", 6)},
  };

  int fails = 0;
  SpacesMarksString v;
  for (std::size_t i = 0;
    i < sizeof(STRESS) / sizeof(STRESS[0]); ++i)
  {
    Stream::Parser is(STRESS[i].in);
    v.clear();
    is >> v;
    if (!is.fail())
    {
      std::cerr << "Stress test: #" << i << ": stream state good" << std::endl;
      ++fails;
    }
    if (static_cast<std::string>(v) != STRESS[i].rest_after_out)
    {
      std::cerr << "Stress test: #" << i << ": rest '"
        << static_cast<std::string>(v) << "', not standard '"
        << STRESS[i].rest_after_out << "'" << std::endl;
      ++fails;
    }
  }
  return fails;
}

int
main()
{
  try
  {
    std::cout << "StringIOTest started.." << std::endl;

    int fails = 0;
    fails += stress_test();
    fails +=
      test_separator<StringIO<Aux_::ConvertSpacesSeparators, '\xFF'> >();
    fails +=
      test_io<StringIO<Aux_::ConvertSpacesSeparators, 255> >();
    fails +=
      test_io<StringIO<Aux_::ConvertSpaces, 255> >();
    fails +=
      test_io<StringIO<AllConvert, 255> >();
    fails +=
      direct_test_io_escapes<StringIO<Aux_::ConvertSpacesSeparators, '\0'> >();
    fails +=
      direct_test_io_escapes<StringIO<Aux_::ConvertSpacesSeparators, 255> >();
    fails += empty_separated_string();

//    print_reserved_chars_table("^");

    std::cout << "COMPLETE" << std::endl;
    return fails;
  }
  catch (const eh::Exception& ex)
  {
    std::cerr << ex.what() << std::endl;
  }
  catch (...)
  {
    std::cerr << "unknown exception" << std::endl;
  }
  return -1;
}
