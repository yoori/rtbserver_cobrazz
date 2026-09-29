#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

#include <Frontends/Modules/BiddingFrontend/KeywordFormatter.hpp>

namespace
{
  __attribute__((noinline)) void overwrite_stack()
  {
    volatile char scratch[4096];
    for (std::size_t i = 0; i < sizeof(scratch); ++i)
    {
      scratch[i] = '!';
    }
  }

  template<typename StringType>
  void check_numeric_lifetime()
  {
    AdServer::Bidding::BasicKeywordFormatter<StringType> formatter("otm");
    formatter.add_yob(1984UL);
    formatter.add_age(42UL);
    formatter.add_yob(0UL);
    formatter.add_age(std::numeric_limits<unsigned long>::max());
    overwrite_stack();
    const std::string maximum = std::to_string(std::numeric_limits<unsigned long>::max());
    const std::string expected = "rtbyob1984\nrtbotmyob1984\nrtbage42\nrtbotmage42\n"
      "rtbyob0\nrtbotmyob0\nrtbage" + maximum + "\nrtbotmage" + maximum;
    std::string actual;
    formatter.assign_to(actual);
    if (actual != expected)
    {
      throw std::runtime_error("Numeric keyword changed after its source buffer expired");
    }
    formatter.assign_to(actual);
    if (actual != expected + '\n' + expected)
    {
      throw std::runtime_error("Repeated keyword serialization changed numeric values");
    }
  }
}

int main()
{
  try
  {
    check_numeric_lifetime<std::string>();
    check_numeric_lifetime<Generics::MonoString>();
    std::cout << "KeywordFormatterTest: OK" << std::endl;
    return 0;
  }
  catch (const std::exception& ex)
  {
    std::cerr << ex.what() << std::endl;
    return 1;
  }
}
