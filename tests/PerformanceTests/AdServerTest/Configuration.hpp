
#ifndef __CONFIGURATION_HPP
#define __CONFIGURATION_HPP

#include <XMLUtility/Utility.hpp>
#include <Commons/ErrorHandler.hpp>
#include <eh/Exception.hpp>
#include <tests/PerformanceTests/Commons/ConfigCommons.hpp>
#include <xsd/tests/PerformanceTests/AdServerTest/AdServerTestConfig.hpp>

/**
 * @class OptOutRequestConfig
 * @brief Optout request config
 */
class OptOutRequestConfig : public RequestConfig
{
  
public:
  /**
   * @brief Constructor.
   *
   * @param _constraint constraint for request.
   * @param _url Base URL for request.
   */  
  OptOutRequestConfig(ConstraintConfig* _constraint,
                      const char* _url);
};

/**
 * @class ClientConfig
 * @brief HTTP-client configuration
 */
class ClientConfig
{
public:
  /**
   * @brief Constructor.
   *
   * @param client_config XML-presentation of client configuration.
   * @param request_lists XML-presentation of parameters/headers/cookies values.
   */  
  ClientConfig(const AdClientType& client_config,
               const RequestLists& request_lists)
    /*throw(SelectorPolicy::InvalidConfigRequestData)*/;

  const unsigned count;                         // requests per second
  const unsigned long lifetime;                 // client lifetime 
  const unsigned long optout_rate;              // percentage of client without uid per second
  const double        action_rate;              // rate of action's requests
  const double        click_rate;               // rate of clicks's requests
  const double        passback_rate;            // rate of passback's requests
  const unsigned long incr_count_step;          // incrementation of requests count per step_interval
  const unsigned long step_interval;            // time (in seconds) for request count incrementation
  const bool          ad_all_optouts;           // Always send ad parameters

  /**
   * @brief Access to ns-lookup request parameters.
   *
   * @return reference to request configuration object.
   */
  const RequestConfig_var& ns_request() const;
  
  /**
   * @brief Access to action request parameters.
   *
   * @return reference to request configuration object.
   */
  const RequestConfig_var& action_request() const;

  /**
   * @brief Access to click request parameters.
   *
   * @return reference to request configuration object.
   */
  const RequestConfig_var& click_request() const;

  /**
   * @brief Access to passback request parameters.
   *
   * @return reference to request configuration object.
   */
  const RequestConfig_var& passback_request() const;

  /**
   * @brief Access to optout request parameters.
   *
   * @return reference to request configuration object.
   */
  const RequestConfig_var& optout_request() const;



private:
  
  RequestConfig_var ns_request_;
  RequestConfig_var action_request_;
  RequestConfig_var click_request_;
  RequestConfig_var passback_request_;
  RequestConfig_var optout_request_;
};

typedef std::unique_ptr<ClientConfig> ClientConfig_var;


/**
* @class Configuration
* @brief Performance test configuration
*/
class Configuration
{
  
public:

  DECLARE_EXCEPTION(InvalidConfiguration, eh::DescriptiveException);
  DECLARE_EXCEPTION(InvalidConfigName, eh::DescriptiveException);
  DECLARE_EXCEPTION(InvalidXmlFieldFormat, eh::DescriptiveException);

  // Field names 
  static const char*          EXECUTION_TIME;   // Default test execution time.
  static const char*          THREADS_NUMBER;   // Default threads number for test.

  // List names
  static const char*          URL;              // URL's list name
  static const char*          REFERER_KW;       // Referer's list name 
  static const char*          FT;               // Full text's list name
  static const char*          VERSION;          // Version's list name  
  static const char*          BROWSER;          // Browsers's list name  
  static const char*          USER_AGENT;       // UserAgent's list name  
  static const char*          OS;               // OS's list name  
  static const char*          TID;              // Tid's list name  
  static const char*          COUNTRY;          // Country's list name  
  static const char*          FORMAT;           // Format's list name  
  static const char*          CT_PSWND_COOKIE;  
  static const char*          SIZENOTSUP;
  static const char*          COLO_ID;          // ColoId's list name  
  
  
public:

  /**
   * @brief Constructor.
   *
   * @param config_path Path of the configuration XML-file
   */    
  Configuration(const char* config_path) :
    config_path_(config_path),
    description_(""),
    confluence_report_path_(""),
    execution_time_(0),
    threads_number_(0),
    server_urls_(),
    statistics_interval_dump_(0)
  {};

  /**
   * @brief Destructor.
   */
  ~Configuration() {};

  /**
   * @brief Read configuration from XML-file.
   */
  void read() /*throw(InvalidConfiguration)*/;

  /**
   * @brief get parameters/headers/cookies list by name.
   *
   * @param name configuration list name.
   * @return reference to list.
   */
  const ConfigList_var& get_list_by_name(const char* name) /*throw(InvalidConfigName)*/;


  /**
   * @brief get test description
   *
   * @return description.
   */
  const std::string& description() const;

    /**
   * @brief get confluence report path.
   *
   * @return confluence report path.
   */
  const std::string& confluence_report_path() const;

  /**
   * @brief get threads number using for test.
   *
   * @return threads number.
   */
  unsigned long threads_number() const;

  /**
   * @brief get test execution time.
   *
   * @return execution time in seconds.
   */
  unsigned long execution_time() const;

  /**
   * @brief get testing server's list.
   *
   * @return server's list.
   */
  const ConfigList_var& server_urls() const;

  /**
   * @brief get client config.
   *
   * @return reference to client config.
   */
  const ClientConfig_var& client_config() const;

  /**
   * @brief get interval dump for statistics (time between adjacent statistic dumps).
   *
   * @return statistics interval dump in seconds.
   */
  unsigned long statistics_interval_dump() const;
  
private:
  std::string config_path_;
  std::string description_;
  std::string confluence_report_path_;
  unsigned long execution_time_;
  unsigned long threads_number_;
  RequestLists request_params_;
  ConfigList_var server_urls_;
  ClientConfig_var client_config_;
  unsigned long statistics_interval_dump_;
};


#endif  // __CONFIGURATION_HPP
