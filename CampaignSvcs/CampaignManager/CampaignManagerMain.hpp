#pragma once

#include <memory>

#include <eh/Exception.hpp>

#include <Generics/CompositeActiveObject.hpp>
#include <Logger/ActiveObjectCallback.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

#include <LogCommons/LogHolder.hpp>

#include <Commons/HttpServer/HttpServer.hpp>
#include <CampaignSvcs/CampaignManagerConfig.hpp>
#include <CampaignSvcs/DomainConfig.hpp>

#include "CampaignManagerGrpc.hpp"
#include "CreativeInstantiator.hpp"

/**
 * Parses command line parameters, loads configuration file,
 * creates service objects.
 * Responsible for general configuration, logging and error handling.
 */
class CampaignManagerApp_: private Logging::LoggerCallbackHolder
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
  DECLARE_EXCEPTION(InvalidArgument, Exception);

public:
  CampaignManagerApp_() /*throw(eh::Exception)*/;
  virtual ~CampaignManagerApp_() noexcept;

  /**
   * Parses command line, opens config file,
   * creates service objects and initializes them.
   */
  void main(int& argc, char** argv) noexcept;

private:
  struct Configuration
  {
    std::string log_root;
    std::string out_logs_dir;
    std::string pid_file;
    AdServer::LogProcessing::PrimaryDumpPtr primary_dump;

    AdServer::CampaignSvcs::CreativeInstantiator::CreativeInstantiate
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

  void read_creative_config(
    AdServer::CampaignSvcs::CreativeInstantiator::CreativeInstantiate&
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
  using Logging::LoggerCallbackHolder::callback;
  using Logging::LoggerCallbackHolder::logger;

  typedef std::unique_ptr<
    AdServer::CampaignSvcs::CampaignManagerCore::CampaignManagerConfig>
    ConfigPtr;

  typedef xsd::AdServer::Configuration::DomainConfigurationType
    DomainConfig;
  typedef std::unique_ptr<DomainConfig> DomainConfigPtr;

private:
  ConfigPtr campaign_manager_config_;
  DomainConfigPtr domain_config_;
  Configuration configuration_;
};

//////////////////////////////////////////////////////////////////////////////
// Inlines
//////////////////////////////////////////////////////////////////////////////

inline
CampaignManagerApp_::~CampaignManagerApp_() noexcept
{
}
