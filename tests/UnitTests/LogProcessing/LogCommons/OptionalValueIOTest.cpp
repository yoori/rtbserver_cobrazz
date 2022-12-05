#include <iostream>
#include <tr1/array>
#include <LogCommons/LogCommons.ipp>

using namespace AdServer::LogProcessing;
typedef OptionalValue<FixedNumber> Optional;
struct TestCase
{
  const char* stream;
  typedef std::tr1::array<Optional, 4> Optionals;
  Optionals numbers;
  const char* separator;
};

const TestCase TEST_CASES[] =
{
  {"@123.4 @2.0 @3.0 @4.0", {{123.4, 2, 3, 4}}, " " },
  {"@-123.4 @-2.0 @-3.0 @-4.0", {{-123.4, -2, -3, -4}}, " " },
  {"@-123.4 @2.0 @3.0 @4.0", {{-123.4, 2, 3, 4}}, " " },
  {"@123.4\t@-2.0\t@3.0\t@4.0", {{123.4, -2, 3, 4}}, "\t" },
  {"@123.4\t@2.0\t@-3.0\t@4.0", {{123.4, 2, -3, 4}}, "\t" },
  {"@123.4\t@2.0\t@3.0\t@-4.0", {{123.4, 2, 3, -4}}, "\t" },
};

template <typename Optional>
int
stress_read_test(const char* input, const char* standard)
{
  Stream::Parser istr(input);
  Stream::Stack<1024> ostr;
  Optional value;

  std::size_t j = -1;
  while (++j, !istr.eof())
  {
    istr >> value;
    if (istr.fail())
    {
      ostr << "fail";
      istr.clear(istr.rdstate() & ~std::ios_base::failbit);
    }
    if (value.present())
    {
      ostr << value;
    }
    else
    {
      ostr << "<nil>";
    }
    ostr << ' ';
    istr.get(); // read separator
  }
  if (ostr.str() != standard)
  {
    std::cerr << __PRETTY_FUNCTION__ <<": unexpected out: "
      << ostr.str() << ", but standard:" << standard << std::endl;
    return 1;
  }
  return 0;
}

int
test_optional_value()
{
  Optional value;
  int result = 0;
  // Stress read tests
  const char READ_TEST_INPUT[] =
    "@123.4 - @-15. @-15";
  const char FIXED_READ_TEST_STANDARD[] =
    "@123.4 <nil> @-15.0 @-15.0 ";
  result += stress_read_test<Optional>(
    READ_TEST_INPUT, FIXED_READ_TEST_STANDARD);
  const char STRING_READ_TEST_STANDARD[] =
    "@123.4 <nil> @-15. @-15 ";
  result += stress_read_test<OptionalValue<std::string> >(
    READ_TEST_INPUT, STRING_READ_TEST_STANDARD);

  // Input check
  for (std::size_t i = 0; i < sizeof(TEST_CASES) / sizeof(TEST_CASES[0]); ++i)
  {
    Stream::Parser istr(TEST_CASES[i].stream);
    for (std::size_t j = 0; j < TEST_CASES[i].numbers.size(); ++j)
    {
      istr >> value;
      if (value != TEST_CASES[i].numbers[j])
      {
        std::cerr << "fail coords:" << i << ',' << j << std::endl;
        ++result;
      }
      if (!istr)
      {
        std::cerr << "stream broken, coords:" << i << ',' << j << std::endl;
        ++result;
        break;
      }
      istr.get(); // read separator
    }
  }
  // Output check
  for (std::size_t i = 0; i < sizeof(TEST_CASES) / sizeof(TEST_CASES[0]); ++i)
  {
    Stream::Dynamic ostr(4096);
    output_sequence(ostr, TEST_CASES[i].numbers, TEST_CASES[i].separator);
    if (ostr.str() != TEST_CASES[i].stream)
    {
      std::cerr << "output failed, coords:" << i
        << "result: " << ostr.str() << ", standard: "
        << TEST_CASES[i].stream << std::endl;
      ++result;
    }
  }
  return result;
}

struct OptionalStringTestCase
{
  const char* in;
  const char* saved;
};

const OptionalStringTestCase OPTIONAL_STRINGS[] =
{
  {" ", "@^20"},
//  {"", ""},
  {"\t ", "@^09^20"},
  {" \t ", "@^20^09^20"},
  {"\t I", "@^09^20I"},
  {"I\t I", "@I^09^20I"},
  {"I\t ", "@I^09^20"},
  {" \t I", "@^20^09^20I"},

//  {"-", "-"},
  {"- ", "@-^20"},
  {"-\t", "@-^09"},
  {"--", "@--"},
  {"---", "@---"},
  {"\t-", "@^09-"},
  {" -", "@^20-"},
  {" - ", "@^20-^20"},

  {"^", "@^5E"},
  {"^ ", "@^5E^20"},
  {"^\t", "@^5E^09"},
  {"^^", "@^5E^5E"},
  {"^^^", "@^5E^5E^5E"},
  {"\t^", "@^09^5E"},
  {" ^", "@^20^5E"},
  {" ^ ", "@^20^5E^20"},

};

const char* STRESS[] =
{
  "^9\t7",
  "^ 9\t7",
  "^ z\t7",
  "^z9 7",
  "^  7",
  "^.F7 ^",
  "^^",
  "^^^",
  "^^^^",
  "^7z ^^",
  "^7zz ^^",
  "^-",
  "^--",
  "^---",
  "^0.",
  "^0^.",
  "^7z -",
  "^7-^^",
  "^7-^--^%5^4^^^20*",
};

int
test_optional_strings()
{
  char str[] = "I       9";
  char str2[] = "  ";
  typedef OptionalValue<Aux_::StringIoWrapper,
          Aux_::ClearableOptionalValueTraits<Aux_::StringIoWrapper> >
            OptionalString;
  OptionalString v;
  Stream::Parser istr(str);
  istr >> v;
  Stream::Stack<1024> os;
  os << v;
  Stream::Parser istr2(str2);
  istr2 >> v;
  Stream::Stack<1024> os2;
  os2 << v;
  if (os.str() != "@I" || os2.str() != "-")
  {
    std::cerr << "optional io failed" << std::endl;
  }
  int result = 0;
  for (std::size_t i = 0;
    i < sizeof(OPTIONAL_STRINGS) / sizeof(OPTIONAL_STRINGS[0]);
    ++i)
  {
    Stream::Parser istr(OPTIONAL_STRINGS[i].saved);
    istr >> v;
    if (v.get() != OPTIONAL_STRINGS[i].in)
    {
      std::cerr << "Optional strings read fail " << i << ": '"
        << OPTIONAL_STRINGS[i].in << "' read as '" << v.get()
        << "', but should '" << OPTIONAL_STRINGS[i].in << "'" << std::endl;
      ++result;
    }
    Stream::Stack<1024> os;
    os << v;
    if (os.str() != OPTIONAL_STRINGS[i].saved)
    {
      std::cerr << "Optional strings write fail " << i << ": '"
        << v.get() << "' wrote as '" << os.str() << ", but should '"
        << OPTIONAL_STRINGS[i].saved << "'" << std::endl;
      ++result;
    }
  }

  return result;
}

int
test_empty_objects()
{
  std::string s;
  EmptyHolder<std::string> outstr(s);
  Stream::Stack<1024> os;
  os << outstr;
  outstr.get() = "Wow";
  os << outstr;
  outstr.get() = "-";
  os << outstr;
  outstr.get().clear();
  os << outstr;
  outstr.get() = "Wow";
  os << outstr;
  int fails = 0;
  if (os.str() != "-Wow--Wow")
  {
    std::cerr << "test_empty_objects write failed" << std::endl;
    ++fails;
  }
  Stream::Parser is("-");
  is >> outstr;
  if (!outstr.get().empty())
  {
    std::cerr << "test_empty_objects empty read failed" << std::endl;
    ++fails;
  }
  Stream::Parser iss("-text");
  iss >> outstr;
  if (outstr.get() != "-text")
  {
    std::cerr << "test_empty_objects read failed" << std::endl;
    ++fails;
  }
  return fails;
}

int
test_sequences_io()
{
  typedef std::tr1::array<unsigned, 10> Numbers;
  const Numbers NUMBERS = {{0, static_cast<unsigned>(-1), 999, 1234567, 5, 6, 7, 8, 9, 10}};
  Stream::Stack<1024> os;
  output_sequence(os, NUMBERS);
  os << " someText" << std::endl;
  Stream::Parser is(os.str().data(), os.str().size());
  NumberList box;
  read_sequence(is, box, ',', Aux_::SpacesAtEnd());
  std::string s;
  is >> s;
  int fails = 0;

  NumberList::const_iterator list_cit(box.begin());
  for (Numbers::const_iterator cit(NUMBERS.begin());
    cit != NUMBERS.end(); ++cit, ++list_cit)
  {
    if (list_cit == box.end())
    {
      std::cerr << "output_sequence failed" << std::endl;
      return 1;
    }
    if (*list_cit != *cit)
    {
      std::cerr << "output_sequence/read_sequence failed" << std::endl;
      return 1;
    }
  }
  if (s != "someText")
  {
    std::cerr << "output_sequence/read_sequence failed" << std::endl;
    ++fails;
  }
  return fails;
}

int
main()
{
  try
  {
    std::cout << "OptionalValueIOTest started.." << std::endl;

    int fails = 0;
    fails += test_optional_strings();
    fails += test_sequences_io();
    fails += test_optional_value();
    fails += test_empty_objects();
    if (fails)
    {
      std::cerr << "Test FAILED, fails count=" << fails << std::endl;
    }
    else
    {
      std::cout << "SUCCESS" << std::endl;
    }
    return fails;
  }
  catch (const eh::Exception& ex)
  {
    std::cerr << "eh::Exception: " << ex.what() << std::endl;
  }
  catch (...)
  {
    std::cerr << "unknown exception" << std::endl;
  }
  return -1;
}
