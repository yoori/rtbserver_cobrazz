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

  template<typename StringType>
  void check_iab_categories()
  {
    AdServer::Bidding::BasicKeywordFormatter<StringType> formatter("yandex");
    {
      std::string category = "IAB9-30";
      formatter.add_external_cat(category, true);
      category.assign(100, '!');
    }
    formatter.add_external_cat("iab1-100", true);
    formatter.add_external_cat("IaB7-45", true);
    formatter.add_external_cat("IAB9", true);
    formatter.add_external_cat("IAB9x30", true);
    formatter.add_external_cat("vendor-30", true);
    formatter.add_external_cat("IAB9-", true);
    formatter.add_external_cat("IAB9--30", true);
    formatter.add_external_cat("IABx-30", true);
    formatter.add_external_cat("IAB9-30extra", true);
    formatter.add_cat(std::string_view("IAB9-30"), false);
    overwrite_stack();
    std::string actual;
    formatter.assign_to(actual);
    const std::string expected =
      "rtbIAB9x30\nrtbyandexIAB9x30\n"
      "rtbiab1x100\nrtbyandexiab1x100\n"
      "rtbIaB7x45\nrtbyandexIaB7x45\n"
      "rtbIAB9\nrtbyandexIAB9\n"
      "rtbIAB9x30\nrtbyandexIAB9x30\n"
      "rtbvendor-30\nrtbyandexvendor-30\n"
      "rtbIAB9-\nrtbyandexIAB9-\n"
      "rtbIAB9--30\nrtbyandexIAB9--30\n"
      "rtbIABx-30\nrtbyandexIABx-30\n"
      "rtbIAB9-30extra\nrtbyandexIAB9-30extra\nrtbyandexIAB9-30";
    if (actual != expected)
    {
      throw std::runtime_error("IAB category formatting or converted-string lifetime changed");
    }

    AdServer::Bidding::BasicKeywordFormatter<StringType> children("");
    std::string expected_children;
    for (unsigned int child = 1; child <= 100; ++child)
    {
      const std::string category = "IAB1-" + std::to_string(child);
      children.add_external_cat(category, true);
      if (!expected_children.empty())
      {
        expected_children += '\n';
      }
      expected_children += "rtbIAB1x" + std::to_string(child);
    }

    actual.clear();
    children.assign_to(actual);
    if (actual != expected_children)
    {
      throw std::runtime_error("IAB child range or empty source prefix changed");
    }
  }
}

int main()
{
  try
  {
    check_numeric_lifetime<std::string>();
    check_numeric_lifetime<Generics::MonoString>();
    check_iab_categories<std::string>();
    check_iab_categories<Generics::MonoString>();
    std::cout << "KeywordFormatterTest: OK" << std::endl;
    return 0;
  }
  catch (const std::exception& ex)
  {
    std::cerr << ex.what() << std::endl;
    return 1;
  }
}
