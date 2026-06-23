
#pragma once

#include <limits.h>

#include <eh/Exception.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Generics/Singleton.hpp>
#include <Logger/ActiveObjectCallback.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>

#include <CORBACommons/StatsImpl.hpp>
#include <CORBACommons/CorbaAdapters.hpp>
#include <Commons/CorbaConfig.hpp>
#include <CampaignSvcs/CampaignServerConfig.hpp>

#include "CampaignServerLogger.hpp"
#include "CampaignServerImpl.hpp"

/**
 * Parses command line parameters, loads configuration file,
 * runs orb, creates corba objects, etc.
 * Responsible for general configuration, logging and error handling.
 */
class CampaignServerApp_ :
  private Logging::LoggerCallbackHolder,
  public virtual ReferenceCounting::AtomicImpl
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
  DECLARE_EXCEPTION(InvalidArgument, Exception);

public:
  CampaignServerApp_() /*throw(eh::Exception)*/;

  /**
   * Parses command line, opens config file,
   * creates corba objects, initialize.
   */
  void main(int& argc, char** argv) noexcept;

private:
  struct Configuration
  {
    enum ServerMode
    {
      SM_SERVER,
      SM_PROXY,
    };

    struct Logger
    {
      Logger() noexcept;

      std::string filename;
      unsigned int log_level;
    };

    unsigned long config_update_period;
    unsigned long ecpm_update_period;
    Generics::Time bill_stat_update_period;
    std::string log_root;
    std::string pid_file;
    Generics::Time audience_expiration_time;
    Generics::Time pending_expire_time;
    bool enable_delivery_thresholds;

    Logger logger;

    CORBACommons::CorbaConfig corba_config;
    AdServer::CampaignSvcs::LogFlushTraits colo_update_flush_traits;

    ServerMode server_mode;

    /* Proxy mode conf */
    unsigned long colo_id;
    std::string version;
    CORBACommons::CorbaObjectRefList campaign_server_refs;

    /* Server mode conf */
    unsigned long server_id;
    std::string pg_connection_string;
    Generics::Time stat_stamp_sync_period;
    CORBACommons::CorbaObjectRefList stat_provider_refs;

    std::string channel_statuses;
    std::string campaign_statuses;
    std::string country;
    bool only_tags;
  };

private:
  virtual ~CampaignServerApp_() noexcept {}

  using Logging::LoggerCallbackHolder::callback;
  using Logging::LoggerCallbackHolder::logger;

  // Reads configration from config XML tree.
  void read_config(const char* filename, const char* argv0)
    /*throw(Exception, eh::Exception)*/;

private:
  Configuration configuration_;
  CORBACommons::CorbaConfig corba_config_;

};

typedef ReferenceCounting::SmartPtr<CampaignServerApp_>
  CampaignServerApp_var;

typedef Generics::Singleton<CampaignServerApp_, CampaignServerApp_var>
  CampaignServerApp;


//////////////////////////////////////////////////////////////////////////////
// Inlines
//////////////////////////////////////////////////////////////////////////////

inline
CampaignServerApp_::Configuration::Logger::Logger() noexcept
    : log_level(Logging::Logger::INFO)
{}
