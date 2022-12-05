
#ifndef _AD_SERVER_CAMPAIGN_SERVICE_CAMPAIGN_SERVER_CAMPAIGN_SERVER_MAIN_HPP_
#define _AD_SERVER_CAMPAIGN_SERVICE_CAMPAIGN_SERVER_CAMPAIGN_SERVER_MAIN_HPP_

#include <limits.h>

#include <eh/Exception.hpp>
#include <Generics/Singleton.hpp>

#include <SNMPAgent/SNMPAgentX.hpp>

#include <CORBACommons/StatsImpl.hpp>
#include <CORBACommons/CorbaAdapters.hpp>
#include <Commons/CorbaConfig.hpp>
#include <Commons/ProcessControlVarsImpl.hpp>
#include <CampaignSvcs/CampaignServerConfig.hpp>

#include "CampaignServerLogger.hpp"
#include "CampaignServerImpl.hpp"

/**
 * Parses command line parameters, loads configuration file,
 * runs orb, creates corba objects, etc.
 * Responsible for general configuration, logging and error handling.
 */
class CampaignServerApp_ :
  public AdServer::Commons::ProcessControlVarsLoggerImpl
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

  //
  // IDL:CORBACommons/IProcessControl/shutdown:1.0
  //
  virtual void shutdown(CORBA::Boolean wait_for_completion)
    /*throw(CORBA::SystemException)*/;

  //
  // IDL:CORBACommons/IProcessControl/is_alive:1.0
  //
  virtual CORBACommons::IProcessControl::ALIVE_STATUS
  is_alive() /*throw(CORBA::SystemException)*/;

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
    Generics::Time audience_expiration_time;
    Generics::Time pending_expire_time;
    bool enable_delivery_thresholds;

    Logger logger;

    struct SnmpConfig
    {
      SnmpConfig(): index_(), mib_dirs_(), empty_(true) {}

      void init(unsigned index, const std::string &mib_dirs)
      {
        index_ = index;
        mib_dirs_ = mib_dirs;
        empty_ = false;
      }

      unsigned index() const { return index_; }

      const std::string& mib_dirs() const { return mib_dirs_; }

      bool empty() const { return empty_; }

      operator bool() const { return !empty_; }

    private:
      unsigned index_;
      std::string mib_dirs_;
      bool empty_;
    };

    SnmpConfig snmp_config;

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

  // Reads configration from config XML tree.
  void read_config(const char* filename, const char* argv0)
    /*throw(Exception, eh::Exception)*/;

private:
  CORBACommons::CorbaServerAdapter_var corba_server_adapter_;
  Configuration configuration_;
  AdServer::CampaignSvcs::CampaignServerBaseImpl_var campaign_server_impl_;
  CORBACommons::CorbaConfig corba_config_;
  AdServer::CampaignSvcs::ProcStatImpl_var proc_stat_impl_;
  CORBACommons::POA_ProcessStatsControl_var proc_stat_ctrl_;

  typedef SNMPAgentX::SNMPStatsGen<AdServer::CampaignSvcs::ProcStatImpl>
    SNMPProcStatsImpl;
  typedef ReferenceCounting::SmartPtr<SNMPProcStatsImpl>
    SNMPProcStatsImpl_var;
  SNMPProcStatsImpl_var snmp_stat_impl_;

  typedef Sync::PosixMutex ShutdownMutex;
  typedef Sync::PosixGuard ShutdownGuard;

  ShutdownMutex shutdown_lock_;
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

#endif // _AD_SERVER_CAMPAIGN_SERVICE_CAMPAIGN_SERVER_CAMPAIGN_SERVER_MAIN_HPP_
