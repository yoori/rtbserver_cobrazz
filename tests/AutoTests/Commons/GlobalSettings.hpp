
#ifndef _AUTOTESTS_COMMONS_GLOBALSETTINGS_HPP
#define _AUTOTESTS_COMMONS_GLOBALSETTINGS_HPP

#include <Generics/Singleton.hpp>
#include "AutoTestsXsd.hpp"

namespace AutoTest
{
  typedef xsd::tests::AutoTests::LocalParamsType LocalConfig;
  typedef xsd::tests::AutoTests::UnitLocalDataType LocalUnit;

  // Constants

  // Used to make delay between checks (in seconds)
  const unsigned short DEFAULT_SLEEP_TIME = 30;

  // Utils

  long
  find_local_params(
    const LocalConfig& local,
    const char *name);

  /**
   * @class GlobalSettings_
   * @brief AutoTest global settings storage
   *
   * Use for access to global settings.
   * Do not create this class directly, use Singleton logic:
   *  Generics::Singleton<GlobalSettings_>::instance()
   */
  class GlobalSettings_
  {
    // Default timeout for waiting statistic,
    // configuration change and server response.
    static const int DEFAULT_TIMEOUT = 5*60;
    // Global section name in LocalParams.xml
    static const char* GLOBAL_SECTION_NAME;
    // Default colocation ID for optout requests
    static const char* OPTOUT_COLO_NAME;

    static const unsigned long DEFAULT_OPTOUT_COLO = 1;

  public:
    /**
     * @brief Default constructor.
     *
     */
    GlobalSettings_();

    /**
     * @brief Global settings initialization.
     *
     * @param global configuration (Config.xml)
     * @param units configuration (LocalParams.xml)
     * @param global config dir path
     */
    void initialize(
      const GlobalConfig& global,
      const LocalConfig& local);

    /**
     * @brief Get  timeout.
     *
     * @return timeout for statistic & configuration changes checkers.
     */
    int wait_timeout() const;

    /**
     * @brief Get frontend timeout.
     *
     * @return timeout for waiting server response.
     */
    int frontend_timeout() const;

    /**
     * @brief Get colocation ID for optout requests.
     *
     * @return colo_id.
     */
    unsigned long optout_colo() const;

    /**
     * @brief Whether GlobalSettings was initialized
     *
     * Before init GlobalSettings singleton uses default values.
     * During init it reads config and rewrite default values.
     * @return whether GlobalSettings was initialized
     */
    bool initialized();

    /**
     * @brief Get global config.
     *
     * @return config.
     */
     Params config() const;

  private:
    int wait_timeout_;
    int frontend_timeout_;
    unsigned long optout_colo_;
    const GlobalConfig* config_;
  };


  typedef Generics::Singleton<GlobalSettings_> GlobalSettings;

  // GlobalSettings_ inlines

  inline
  int
  GlobalSettings_::wait_timeout() const
  {
    return wait_timeout_;
  }

  inline
  int
  GlobalSettings_::frontend_timeout() const
  {
    return frontend_timeout_;
  }

  inline
  unsigned long
  GlobalSettings_::optout_colo() const
  {
    return optout_colo_;
  }

  inline bool GlobalSettings_::initialized()
  {
    return config_ != 0;
  }

  inline
  Params
  GlobalSettings_::config() const
  {
    return config_->get_params();
  }
}

#endif  // _AUTOTESTS_COMMONS_GLOBALSETTINGS_HPP
