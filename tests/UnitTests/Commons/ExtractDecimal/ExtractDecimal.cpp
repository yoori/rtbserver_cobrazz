#include<vector>
#include<string>
#include <Commons/DecimalUtils.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>

typedef Generics::Decimal<uint64_t, 25, 8> UseDecimalType;

std::vector<std::pair<std::string, std::string> > correct_test_cases = {
  {"", "0.0"},

  {"000123.45", "123.45"},
  {"+000123.45", "123.45"},
  {"-000123.45", "-123.45"},

  {"123.45", "123.45"},
  {"+123.45", "123.45"},
  {"-123.45", "-123.45"},

  {"0.0012345", "0.0012345"},
  {"+0.0012345", "0.0012345"},
  {"-0.0012345", "-0.0012345"},

  {"1.2345E10", "12345000000.0"},
  {"+1.2345E10", "12345000000.0"},
  {"-1.2345E10", "-12345000000.0"},

  {"1.2345E+10", "12345000000.0"},
  {"+1.2345E+10", "12345000000.0"},
  {"-1.2345E+10", "-12345000000.0"},

  {"1.2345E-10", "0.0"},
  {"+1.2345E-10", "0.0"},
  {"-1.2345E-10", "0.0"},

  {"1.2345e10", "12345000000.0"},
  {"+1.2345e10", "12345000000.0"},
  {"-1.2345e10", "-12345000000.0"},

  {"1.2345e+10", "12345000000.0"},
  {"+1.2345e+10", "12345000000.0"},
  {"-1.2345e+10", "-12345000000.0"},

  {"1.2345e-5", "0.00001235"},
  {"+1.2345e-5", "0.00001235"},
  {"-1.2345e-5", "-0.00001235"},

  {"1.2345e-10", "0.0"},
  {"+1.2345e-10", "0.0"},
  {"-1.2345e-10", "0.0"},

  {"1.23456789012345678", "1.23456789"},
  {"+1.23456789012345678", "1.23456789"},
  {"-1.23456789012345678", "-1.23456789"},

  {"000123,12", "123.0"},
  {"+000123!34", "123.0"},
  {"-000123~56", "-123.0"},

  {"1.2345E+10,12", "12345000000.0"},
  {"+1.2345E+10!34", "12345000000.0"},
  {"-1.2345E+10~56", "-12345000000.0"},

  {"-3.123456789012345678901234567890e-19", "0.0"},
  {"-3.12345678901234567890123456e-19", "0.0"},
  {"-3.1234567890123456789012345e-19", "0.0"},
  {"-3.123456789012345678901234e-19", "0.0"}
};

std::vector<std::string> incorrect_test_cases = {
  "",

  "123,45",
  "+123!45",
  "-123~45",

  ",123.45",
  "!+123.45",
  "~-123.45",

  "123.4,5",
  "+123.4!5",
  "-123.4~5",

  "123.45,",
  "+123.45!",
  "-123.45~",

  ",123E45",
  "!+123E45",
  "~-123E45",

  "123,E45",
  "+123!E45",
  "-123~E45",

  "123E,45",
  "+123E!45",
  "-123E~45",

  "123E4,5",
  "+123E4!5",
  "-123E4~5",

  "123E10,",
  "+123E10!",
  "-123E10~"
};

void test()
{
  for (auto it = correct_test_cases.begin(); it != correct_test_cases.end(); ++it)
  {
    try
    {
      UseDecimalType v(
        AdServer::Commons::extract_decimal<UseDecimalType>(
          String::SubString(it->first), Generics::DMR_ROUND));
      if (v.str() != it->second)
      {
        std::cerr << "Error in correct cases for '" << it->first << "': " << v.str() <<
          " != " << it->second << ", case: " << (it - correct_test_cases.begin()) << std::endl;
        exit(1);
      }
    }
    catch (const UseDecimalType::Exception& ex)
    {
      std::cerr << "Unexpected exception happens for correct case '" <<
        it->first << "': " << ex.what() << std::endl;
      exit(1);
    }
  }

  for (auto it = incorrect_test_cases.begin(); it != incorrect_test_cases.end(); ++it)
  {
    try
    {
      UseDecimalType v(
        AdServer::Commons::extract_decimal<UseDecimalType>(
          String::SubString(*it),
          Generics::DMR_ROUND,
          true));
    }
    catch (const UseDecimalType::NotNumber& ex)
    {
      continue;
    }
    catch (const UseDecimalType::Exception& ex)
    {
      std::cerr << "Unexpected exception happens for incorrect case '" <<
        *it << "': " << ex.what() << std::endl;
      exit(1);
    }

    std::cerr << "Exception doesn't happen for incorrect case: " <<
      (it - incorrect_test_cases.begin()) << std::endl;
    exit(1);
  }
}

int
main()
{
  test();

  return 0;
}
