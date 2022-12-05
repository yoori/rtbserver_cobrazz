
#ifndef _CONFIGURATION_HPP
#define _CONFIGURATION_HPP

#include <XMLUtility/Utility.hpp>
#include <Commons/ErrorHandler.hpp>
#include <eh/Exception.hpp>
#include <tests/PerformanceTests/Commons/ConfigCommons.hpp>
#include <xsd/tests/PerformanceTests/AdServerBenchmark/AdServerBenchmarkConfig.hpp>

/**
* @class BenchmarkConfig
* @brief Benchmark configuration
*/
class BenchmarkConfig  :
  public virtual ReferenceCounting::Interface,
  public virtual ReferenceCounting::DefaultImpl<>
{
public:

  /**
   * @brief Constructor.
   *
   * @param server address
   * @param benchmark size
   * @param benchmark client type
   * @param is initial benchmark (recreate users)
   * @param frontend type
   * @param request configuration
   */    
  BenchmarkConfig(const char* server_,
                  unsigned long size_,
                  ClientType client_type_,
                  bool initial_,
                  const char* description_,
                  FrontendType frontend_type_,
                  RequestConfig* config_);

  /**
   * @brief Destructor.
   */
  virtual ~BenchmarkConfig() noexcept;
  
  const std::string server;   // server address
  const unsigned long size;   // benchmark size
  const ClientType client_type;    // client type
  const bool initial;         // is initial benchmark (recreate users)
  const std::string description;  // benchmark description
  const FrontendType frontend_type; // frontend type
  RequestConfig_var config;   // request configuration
};

typedef ReferenceCounting::SmartPtr<BenchmarkConfig> BenchmarkConfig_var;
  

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

  typedef std::list<BenchmarkConfig_var> BenchmarkList;
  
  // Default threads number for test.
  static const unsigned long  DEFAULT_THREADS_NUMBER = 10;

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
  Configuration(const char* config_path);

  /**
   * @brief Destructor.
   */
  ~Configuration();

  /**
   * @brief Read configuration from XML-file.
   */
  void read() /*throw(InvalidConfiguration)*/;

  /**
   * @brief get threads number using for test.
   *
   * @return threads number.
   */
  unsigned long threads_number() const;

  /**
   * @brief get benchmark list.
   *
   * @return benchmarks.
   */
  const BenchmarkList& benchmarks() const;


private:
  std::string config_path_;
  unsigned long threads_number_;
  RequestLists request_params_;
  BenchmarkList benchmarks_;
};


#endif  // _CONFIGURATION_HPP
