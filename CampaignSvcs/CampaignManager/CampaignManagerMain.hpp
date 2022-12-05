
#ifndef _AD_SERVER_CAMPAIGN_SERVICE_CAMPAIGN_MANAGER_CAMPAIGN_MANAGER_MAIN_HPP_
#define _AD_SERVER_CAMPAIGN_SERVICE_CAMPAIGN_MANAGER_CAMPAIGN_MANAGER_MAIN_HPP_

#include <eh/Exception.hpp>

#include <Generics/Singleton.hpp>

#include <Logger/ActiveObjectCallback.hpp>

#include <LogCommons/LogHolder.hpp>

#include <Commons/CorbaConfig.hpp>
#include <Commons/ProcessControlVarsImpl.hpp>
#include <CampaignSvcs/CampaignManagerConfig.hpp>
#include <CampaignSvcs/DomainConfig.hpp>

#include "CampaignManagerImpl.hpp"

/**
 * Parses command line parameters, loads configuration file,
 * runs orb, creates corba objects, etc.
 * Responsible for general configuration, logging and error handling.
 */
class CampaignManagerApp_
  : public AdServer::Commons::ProcessControlVarsLoggerImpl
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
  DECLARE_EXCEPTION(InvalidArgument, Exception);

public:
  CampaignManagerApp_() /*throw(eh::Exception)*/;

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

  virtual bool is_ready_() noexcept;

  virtual char* comment()
    /*throw(CORBACommons::OutOfMemory)*/;

private:
  struct Configuration
  {
    CORBACommons::CorbaConfig corba_config;

    std::string log_root;
    std::string out_logs_dir;

    AdServer::CampaignSvcs::CampaignManagerImpl::CreativeInstantiate
      creative_instantiate;

    std::string campaigns_types;
    unsigned int uc_freq_caps_lifetime;

    AdServer::CampaignSvcs::CampaignManagerLogger::Params log_params;
  };

private:

  // Reads configuration from config XML tree.
  xsd::AdServer::Configuration::CampaignManagerType
  read_config(const char* filename, const char* argv0)
    /*throw(Exception, eh::Exception)*/;

  virtual ~CampaignManagerApp_() noexcept;

  void read_creative_config(
    AdServer::CampaignSvcs::CampaignManagerImpl::CreativeInstantiate&
      creative_instantiate,
    const xsd::AdServer::Configuration::CampaignManagerCreative&
      xsd_creative_description)
    /*throw(Exception, eh::Exception)*/;

  void read_creative_rule_config(
    long& cur_option_id,
    std::string& name,
    AdServer::CampaignSvcs::CreativeInstantiateRule& rule,
    const xsd::AdServer::Configuration::CampaignManagerCreativeRuleType&
      xsd_creative_rule_description)
    /*throw(Exception, eh::Exception)*/;

  void read_logging_config(
    const xsd::AdServer::Configuration::CampaignManagerLoggingType&
      xsd_logging_config,
    AdServer::CampaignSvcs::CampaignManagerLogger::Params& log_params)
    /*throw(Exception)*/;

  void read_logger_config(
    const char *log_dir_name,
    const xsd::AdServer::Configuration::CampaignManagerLoggerType&
      xsd_collector_logger_description,
    AdServer::LogProcessing::LogFlushTraits& log_params)
    /*throw(Exception, eh::Exception)*/;

  static void create_path_(std::string& first, const char* second)
    /*throw(eh::Exception)*/;

private:
  typedef std::unique_ptr<
    AdServer::CampaignSvcs::CampaignManagerImpl::CampaignManagerConfig>
    ConfigPtr;

  typedef xsd::AdServer::Configuration::DomainConfigurationType
    DomainConfig;
  typedef std::unique_ptr<DomainConfig> DomainConfigPtr;

  CORBACommons::CorbaServerAdapter_var corba_server_adapter_;

  ConfigPtr campaign_manager_config_;
  DomainConfigPtr domain_config_;
  Configuration configuration_;
  CORBACommons::CorbaConfig corba_config_;

  AdServer::CampaignSvcs::CampaignManagerImpl_var campaign_manager_impl_;

  typedef Sync::Policy::PosixThread SyncPolicy;
  typedef SyncPolicy::Mutex ShutdownMutex;
  typedef SyncPolicy::ReadGuard ShutdownReadGuard;
  typedef SyncPolicy::WriteGuard ShutdownWriteGuard;

  ShutdownMutex shutdown_lock_;
};

typedef ReferenceCounting::SmartPtr<CampaignManagerApp_>
  CampaignManagerApp_var;

typedef Generics::Singleton<CampaignManagerApp_, CampaignManagerApp_var>
  CampaignManagerApp;

//////////////////////////////////////////////////////////////////////////////
// Inlines
//////////////////////////////////////////////////////////////////////////////

inline
CampaignManagerApp_::~CampaignManagerApp_() noexcept
{
}

#endif // _AD_SERVER_CAMPAIGN_SERVICE_CAMPAIGN_MANAGER_CAMPAIGN_MANAGER_MAIN_HPP_
