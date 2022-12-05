/// @file UrlMatcherTest.hpp

#ifndef URL_MATCHER_TEST_HPP_INCLUDED
#define URL_MATCHER_TEST_HPP_INCLUDED

#include <Commons/ErrorHandler.hpp>
#include <Frontends/FrontendCommons/RequestMatchers.hpp>
#include <xsd/Frontends/PermanentFeConfig.hpp>


  DECLARE_EXCEPTION(TestException, eh::DescriptiveException);


  const std::string
  get_config_path(const char* file_name) /*throw(eh::Exception)*/
  {
    std::string config_path;
    char* ev = getenv("TEST_TOP_SRC_DIR");
    config_path = ev ? ev : "../../../../..";
    config_path +=
      "/tests/UnitTests/Frontends/FrontendCommons/";
    config_path += file_name;
    return config_path;
  }


void
load_url_matcher(FrontendCommons::UrlMatcher& matcher,
  const char* file_name) /*throw(eh::Exception)*/
{
  const char FUN[] = "load_url_matcher()";
  typedef xsd::AdServer::Configuration::PermanentFeConfigurationType
    PermanentFeConfiguration;
  // load permanent configuration
  Config::ErrorHandler error_handler;
  std::unique_ptr<PermanentFeConfiguration> permanent_fe_config;
  const std::string CONFIG_FILE(get_config_path(file_name));
  try
  {
    permanent_fe_config =
      xsd::AdServer::Configuration::PermanentFeConfiguration(
        CONFIG_FILE.c_str(), error_handler);

    if (error_handler.has_errors())
    {
      std::string error_string;
      throw TestException(error_handler.text(error_string));
    }
  }
  catch (const xml_schema::parsing& e)
  {
    Stream::Error ostr;
    ostr << FUN << ": Can't parse config file '" << CONFIG_FILE << "': "
      << std::endl;

    std::string str;
    ostr << error_handler.text(str);
    throw TestException(ostr);
  }
  catch (const eh::Exception& e)
  {
    Stream::Error ostr;
    ostr << FUN << "Can't parse config file '" << CONFIG_FILE << "': "
      << e.what();
    throw TestException(ostr);
  }
  catch (...)
  {
    Stream::Error ostr;
    ostr << FUN << ": Unknown Exception at parsing of config.";
    throw TestException(ostr);
  }

  matcher.read_config(permanent_fe_config->SearchersConfig().get());

//    load_urls(permanent_fe_config->SearchersConfig());
}

#endif // URL_MATCHER_TEST_HPP_INCLUDED
