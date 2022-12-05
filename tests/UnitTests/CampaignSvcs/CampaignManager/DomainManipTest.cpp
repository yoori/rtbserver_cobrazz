// @file CampaignManager/DomainManipTest.cpp

#include <iostream>
#include <Generics/Time.hpp>
#include <Generics/AppUtils.hpp>
#include <HTTP/UrlAddress.hpp>
#include <Commons/ErrorHandler.hpp>
#include <xsd/CampaignSvcs/DomainConfig.hpp>
#include <CampaignSvcs/CampaignManager/DomainParser.hpp>

using namespace AdServer::CampaignSvcs;

namespace
{
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
  typedef xsd::AdServer::Configuration::DomainConfigurationType
    DomainConfig;
  typedef std::unique_ptr<DomainConfig> DomainConfigPtr;

  const char USAGE[] =
    "DomainManipTest [OPTIONS]\n"
    "OPTIONS:\n"
    "  -p, --path : path to test file"
    "  -h, --help : show this message.\n";

   Generics::AppUtils::Option<std::string> opt_file(
    "../../../../../tests/UnitTests/CampaignSvcs/CampaignManager/Data"
    "/DomainConfig.xml");
}

int
check_specific_domain(
  const DomainParser* domain_parser,
  const char* domain,
  const char* standard)
{
  std::string res;
  domain_parser->specific_domain(String::SubString(domain), res);

  if (res != standard)
  {
    std::cerr << "specific_domain() return '" << res <<
      "' instead '" << standard <<
      "' for '" << domain << "'" << std::endl;
    return 1;
  }

  return 0;
}

int
check_specific_url(
  const DomainParser* domain_parser,
  const char* url,
  const char* standard)
{
  std::string res;
  domain_parser->specific_domain(
    HTTP::BrowserAddress(String::SubString(url)).host(), res);

  if (res != standard)
  {
    std::cerr << "specific_domain() return '" << res <<
      "' instead '" << standard <<
      "' for '" << url << "'" << std::endl;
    return 1;
  }

  return 0;
}

int
stress_test(
  const DomainParser* domain_parser,
  const String::SubString& url,
  const std::string& standard)
{
  std::string res;
  domain_parser->specific_domain(url, res);
  if (res != standard)
  {
    std::cerr << "specific_domain() return '" << res <<
      "' instead '" << standard <<
      "' for '" << url << "'" << std::endl;
    return 1;
  }
  return 0;
}

int
total_test(DomainParser_var& domain_parser)
{
  DomainConfigPtr domain_config;
  Config::ErrorHandler error_handler;
  int res = 0;
  try
  {
    domain_config = xsd::AdServer::Configuration::DomainConfiguration(
      opt_file->c_str(), error_handler);

    if (error_handler.has_errors())
    {
      std::string error_string;
      throw Exception(error_handler.text(error_string));
    }
    domain_parser = new DomainParser(*domain_config);
  }
  catch (const xml_schema::parsing& e)
  {
    Stream::Error ostr;
    ostr << "Can't parse config file: ";
    if (error_handler.has_errors())
    {
      std::string error_string;
      ostr << error_handler.text(error_string);
    }

    throw Exception(ostr);
  }

  // 2. Total test
  for(DomainConfig::Domain_sequence::const_iterator d_it =
    domain_config->Domain().begin();
    d_it != domain_config->Domain().end(); ++d_it)
  {
    if (!d_it->SubDomain().size()) // The first level domains
    {
      res += check_specific_domain(domain_parser,
        ("www." + d_it->name()).c_str(), ("www." + d_it->name()).c_str());
      res += check_specific_domain(domain_parser,
        ("." + d_it->name()).c_str(), ("." + d_it->name()).c_str());
      res += check_specific_domain(domain_parser,
        (".." + d_it->name()).c_str(), ("." + d_it->name()).c_str());
      res += check_specific_domain(domain_parser,
        ("..." + d_it->name()).c_str(), ("." + d_it->name()).c_str());
      res += check_specific_domain(domain_parser,
        ("www" + d_it->name()).c_str(), ("www" + d_it->name()).c_str());
      res += check_specific_domain(domain_parser,
        (".www" + d_it->name()).c_str(), (".www" + d_it->name()).c_str());
      res += check_specific_domain(domain_parser,
        ("add_level." + d_it->name()).c_str(), ("add_level." + d_it->name()).c_str());
      res += check_specific_domain(domain_parser,
        ("add.level." + d_it->name()).c_str(), ("level." + d_it->name()).c_str());
      res += check_specific_domain(domain_parser,
        d_it->name().c_str(), d_it->name().c_str());
      continue;
    }
    std::string suffix = "." + d_it->name();
    for (xsd::AdServer::Configuration::DomainType::
      SubDomain_sequence::const_iterator sd_it =
        d_it->SubDomain().begin();
      sd_it != d_it->SubDomain().end(); ++sd_it)
    {
      std::string domain = sd_it->name() + suffix;
      std::string::size_type pos = sd_it->name().find('.');
      if (sd_it->name().find('.') != std::string::npos)
      {
        std::string domain_first = "_some_text_" + suffix;
        std::string domain_second_first = "_some_text_" +
          sd_it->name().substr(pos) + suffix;
        res += check_specific_domain(domain_parser,
          domain_first.c_str(), domain_first.c_str());
        res += check_specific_domain(domain_parser,
          ("add." + domain_first).c_str(), domain_first.c_str());
        res += check_specific_domain(domain_parser,
          domain_second_first.c_str(), domain_second_first.c_str());
        res += check_specific_domain(domain_parser,
          ("add." + domain_second_first).c_str(), domain_second_first.c_str());
      }
      res += check_specific_domain(domain_parser,
        ("www." + domain).c_str(), domain.c_str());
      res += check_specific_domain(domain_parser,
        ("." + domain).c_str(), domain.c_str());
      res += check_specific_domain(domain_parser,
        (".." + domain).c_str(), ("." + domain).c_str());
      res += check_specific_domain(domain_parser,
        ("..." + domain).c_str(), ("." + domain).c_str());
      res += check_specific_domain(domain_parser,
        ("www" + domain).c_str(), ("www" + domain).c_str());
      res += check_specific_domain(domain_parser,
        ("*." + domain).c_str(), ("*." + domain).c_str());
      res += check_specific_domain(domain_parser,
        ("www.*." + domain).c_str(), ("*." + domain).c_str());
      res += check_specific_domain(domain_parser,
        (".www." + domain).c_str(), ("www." + domain).c_str());
      res += check_specific_domain(domain_parser,
        ("s.*." + domain).c_str(), ("*." + domain).c_str());
      res += check_specific_domain(domain_parser,
        ("some.up." + domain).c_str(), ("up." + domain).c_str());
      res += check_specific_domain(domain_parser,
        domain.c_str(), domain.c_str());
    }
  }
  return res;
}

int
main(int argc, char* argv[]) noexcept
{
  int res = 0;

  {
    const String::SubString UTM_TERM_1("?utm_term=");
    const String::SubString UTM_TERM_2("&utm_term=");

    {
      String::SubString ref("test.com?k=1&utm_term=XXX&TEST");
      String::SubString::SizeType utm_term_pos1 = ref.find(UTM_TERM_1);
      String::SubString::SizeType utm_term_pos2 = ref.find(UTM_TERM_2);
      String::SubString::SizeType start_pos = String::SubString::NPOS;
      if(utm_term_pos1 != String::SubString::NPOS)
      {
        start_pos = utm_term_pos1 + UTM_TERM_1.size();
      }
      else if(utm_term_pos2 != String::SubString::NPOS)
      {
        start_pos = utm_term_pos2 + UTM_TERM_2.size();
      }

      if(start_pos != String::SubString::NPOS)
      {
        String::SubString::SizeType end_pos = ref.find('&', start_pos);
        String::SubString utm_term_user_id = ref.substr(
          start_pos,
          end_pos == String::SubString::NPOS ? String::SubString::NPOS :
          end_pos - start_pos);
        std::cout << "<" << utm_term_user_id << ">(" << utm_term_user_id.size() << ")" << std::endl;
      }
    }
  }

  try
  {
    Generics::AppUtils::Args args;
    Generics::AppUtils::CheckOption opt_help;
    args.add(
      Generics::AppUtils::equal_name("help") ||
      Generics::AppUtils::short_name("h"),
      opt_help);
    args.add(
      Generics::AppUtils::equal_name("path") ||
      Generics::AppUtils::short_name("p"),
      opt_file);

    args.parse(argc - 1, argv + 1);

    if (opt_help.enabled())
    {
      std::cout << USAGE << std::endl;
      return 0;
    }

    DomainParser_var domain_parser;
    std::cout << "Total testing.." << std::endl;
    res += total_test(domain_parser);
    std::cout << "STRESS testing.." << std::endl;

    // 1. Overflow test
    char BUF[] = "BU";
    BUF[2] = 'G';

    res += stress_test(domain_parser,
      String::SubString(BUF, (std::size_t)0), "");
    res += stress_test(domain_parser,
      String::SubString(BUF, 1), "B");
    res += stress_test(domain_parser,
      String::SubString(BUF, 2), "BU");
    std::fill(BUF, BUF+sizeof(BUF), 0);
    res += stress_test(domain_parser,
      String::SubString(BUF, (std::size_t)0), "");
    res += stress_test(domain_parser,
      String::SubString(BUF, 1), std::string("\0", 1));
    res += stress_test(domain_parser,
      String::SubString(BUF, 2), std::string("\0\0", 2));
    std::fill(BUF, BUF+sizeof(BUF), '.');
    res += stress_test(domain_parser,
      String::SubString(BUF, (std::size_t)0), "");
    res += stress_test(domain_parser,
      String::SubString(BUF, 1), ".");
    res += stress_test(domain_parser,
      String::SubString(BUF, 2), ".");
    // check SubStrings without termination zero
    res += stress_test(domain_parser,
      String::SubString("fedora.wiki.brAdditionalTextWithoutZero", 14),
        "fedora.wiki.br");

    std::cout << "Some cases testing.." << std::endl;

    res += check_specific_domain(domain_parser, "www", "www");
    res += check_specific_domain(domain_parser, ".www.", "www.");
    res += check_specific_domain(domain_parser, ".za.", "za.");
    res += check_specific_domain(domain_parser, "..za", ".za");

    res += check_specific_domain(domain_parser, "bla", "bla");
    res += check_specific_domain(domain_parser, "bla.bla", "bla.bla");
    res += check_specific_domain(domain_parser, "www.bla", "www.bla");
    res += check_specific_domain(domain_parser, "www.bla.bla", "bla.bla");

    res += check_specific_domain(domain_parser,
      "levelup.pref.kumamoto.jp", "levelup.pref.kumamoto.jp");
    res += check_specific_domain(domain_parser,
      ".pref.kumamoto.jp", "pref.kumamoto.jp");
    res += check_specific_domain(domain_parser, ".unknown", ".unknown");

    // Old tests
    res += check_specific_url(domain_parser, "test.com", "test.com");
    res += check_specific_url(domain_parser, "test.co.kr", "test.co.kr");
    res += check_specific_url(domain_parser, "s.test.com", "test.com");
    res += check_specific_url(domain_parser, "s.test.co.kr", "test.co.kr");
    res += check_specific_domain(domain_parser, "com", "com");
    res += check_specific_url(domain_parser, "test.cccom", "test.cccom");

    res += check_specific_domain(domain_parser, "www.ru", "www.ru");
    res += check_specific_domain(domain_parser, "www.www.ru", "www.ru");
    res += check_specific_url(domain_parser, "a.b.ru", "b.ru");
    res += check_specific_url(domain_parser, "a.b.c.ru", "c.ru");

    res += check_specific_domain(domain_parser, ".ru", ".ru");
    res += check_specific_domain(domain_parser, ".ss.ru", "ss.ru");
    res += check_specific_domain(domain_parser, ".ss.ss.ru", "ss.ru");

    res += check_specific_url(domain_parser, "fedora.wiki.br", "fedora.wiki.br");
    res += check_specific_url(domain_parser, "http://fedora.wiki.br/", "fedora.wiki.br");
  }
  catch (const eh::Exception& ex)
  {
    std::cerr << "Exception: " << ex.what() << std::endl;
    res = -1;
  }

  return res;
}
