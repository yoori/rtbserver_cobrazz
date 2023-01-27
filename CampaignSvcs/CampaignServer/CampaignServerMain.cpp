
#include <locale.h>

#include <eh/Exception.hpp>

#include <Commons/CorbaConfig.hpp>
#include <Commons/ErrorHandler.hpp>
#include <Commons/ConfigUtils.hpp>
#include <Commons/PathManip.hpp>

#include "CampaignServerMain.hpp"

#include "Compatibility/CampaignServerImpl_v350.hpp"

//#include <Commons/HeapController.hpp>

namespace
{
  const char* ASPECT  = "CampaignServer";
  const char* PROCESS_CONTROL_OBJ_KEY = "ProcessControl";
  //const char* CAMPAIGN_SERVER_V320_OBJ_KEY = "CampaignServer_v320";
  //const char* CAMPAIGN_SERVER_V330_OBJ_KEY = "CampaignServer_v330";
  //const char* CAMPAIGN_SERVER_V340_OBJ_KEY = "CampaignServer_v340";
  const char* CAMPAIGN_SERVER_V350_OBJ_KEY = "CampaignServer_v350";

  const char* CAMPAIGN_SERVER_OBJ_KEY = "CampaignServer_v360";

  const char PROCESS_STATS_CONTROL_OBJ_KEY[] = "ProcessStatsControl";
}

CampaignServerApp_::CampaignServerApp_() /*throw(eh::Exception)*/
  : AdServer::Commons::ProcessControlVarsLoggerImpl(
      "CampaignServerApp_", ASPECT)
{
}

void
CampaignServerApp_::shutdown(CORBA::Boolean wait_for_completion)
  /*throw(CORBA::SystemException)*/
{
  ShutdownGuard guard(shutdown_lock_);

  if(campaign_server_impl_.in() != 0)
  {
    campaign_server_impl_->deactivate_object();
    campaign_server_impl_->wait_object();
  }

  CORBACommons::ProcessControlImpl::shutdown(wait_for_completion);
}

CORBACommons::IProcessControl::ALIVE_STATUS
CampaignServerApp_::is_alive() /*throw(CORBA::SystemException)*/
{
  return CORBACommons::ProcessControlImpl::is_alive();
}

void
CampaignServerApp_::main(int& argc, char** argv) noexcept
{
//AdServer::Commons::HeapController::instance().initialize();

  try
  {
    const char* usage = "usage: CampaignServer <config_file>";

    // Checking CL params
    if (argc < 2)
    {
      Stream::Error ostr;
      ostr << "config file is not specified\n" << usage;
      throw InvalidArgument(ostr);
    }

    try
    {
      read_config(argv[1], argv[0]);
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << "Can't parse config file '" << argv[1] << "': " << e.what();
      throw Exception(ostr);
    }
    catch(...)
    {
      Stream::Error ostr;
      ostr << "Unknown Exception at parsing of config.";
      throw Exception(ostr);
    }

    corba_server_adapter_ =
      new CORBACommons::CorbaServerAdapter(corba_config_);

    proc_stat_impl_ = new AdServer::CampaignSvcs::ProcStatImpl;

    proc_stat_ctrl_ =
      new CORBACommons::ProcessStatsGen<
        AdServer::CampaignSvcs::ProcStatImpl>(proc_stat_impl_);

    // Creating campaign server servant
    if(configuration_.server_mode == Configuration::SM_SERVER)
    {
      campaign_server_impl_ =
        new AdServer::CampaignSvcs::CampaignServerImpl(
          callback(),
          logger(),
          proc_stat_impl_,
          configuration_.colo_id,
          configuration_.version.c_str(),
          configuration_.config_update_period,
          configuration_.ecpm_update_period,
          configuration_.bill_stat_update_period,
          configuration_.colo_update_flush_traits,
          configuration_.server_id,
          configuration_.campaign_statuses,
          configuration_.channel_statuses,
          configuration_.pg_connection_string.c_str(),
          configuration_.stat_stamp_sync_period,
          configuration_.stat_provider_refs,
          configuration_.audience_expiration_time,
          configuration_.pending_expire_time,
          configuration_.enable_delivery_thresholds);
    }
    else if(configuration_.server_mode == Configuration::SM_PROXY)
    {
      campaign_server_impl_ =
        new AdServer::CampaignSvcs::CampaignServerProxyImpl(
          callback(),
          logger(),
          proc_stat_impl_,
          configuration_.colo_id,
          configuration_.version.c_str(),
          configuration_.config_update_period,
          configuration_.ecpm_update_period,
          configuration_.bill_stat_update_period,
          configuration_.colo_update_flush_traits,
          configuration_.server_id,
          configuration_.channel_statuses,
          configuration_.campaign_server_refs,
          configuration_.campaign_statuses.c_str(),
          configuration_.country.c_str(),
          configuration_.only_tags);
    }
    {
      using namespace AdServer::Commons;
      register_vars_controller();
      add_var_processor(DbStateProcessor::VAR_NAME,
        new DbStateProcessor(
          new_simple_db_state_changer(campaign_server_impl_)));
    }

    if (configuration_.snmp_config)
    {
      try
      {
        proc_stat_impl_->set("index",
          static_cast<unsigned long>(configuration_.snmp_config.index()));

        snmp_stat_impl_ =
          new SNMPProcStatsImpl(
            proc_stat_impl_, configuration_.snmp_config.index(),
            Logging::Logger_var(new Logging::LoggerDefaultHolder(
              logger(), 0, "ADS-IMPL-155")),
            "",
            "CampaignServer-MIB:campaignServer",
            configuration_.snmp_config.mib_dirs().c_str());
      }
      catch(const eh::Exception& e)
      {
        Stream::Error ostr;
        ostr << "CampaignServerApp_::main(): "
          "Got eh::Exception: " << e.what();
        logger()->log(ostr.str(),
          Logging::Logger::ERROR,
          ASPECT,
          "ADS-IMPL-154");
      }
    }

    shutdowner_ = corba_server_adapter_->shutdowner();

    {
      AdServer::CampaignSvcs::CampaignServerImpl_v350_var
        campaign_server_impl_v350 =
          new AdServer::CampaignSvcs::CampaignServerImpl_v350(
            logger(),
            campaign_server_impl_);

      /*
      AdServer::CampaignSvcs::CampaignServerImpl_v340_var
        campaign_server_impl_v340 =
          new AdServer::CampaignSvcs::CampaignServerImpl_v340(
            logger(),
            campaign_server_impl_v350);

      AdServer::CampaignSvcs::CampaignServerImpl_v331_var
        campaign_server_impl_v331 =
          new AdServer::CampaignSvcs::CampaignServerImpl_v331(
            logger(),
            campaign_server_impl_v340);

      AdServer::CampaignSvcs::CampaignServerImpl_v320_var
        campaign_server_impl_v320 =
          new AdServer::CampaignSvcs::CampaignServerImpl_v320(
            logger(),
            campaign_server_impl_v331);

      corba_server_adapter_->add_binding(
        CAMPAIGN_SERVER_V320_OBJ_KEY, campaign_server_impl_v320.in());

      corba_server_adapter_->add_binding(
        CAMPAIGN_SERVER_V330_OBJ_KEY, campaign_server_impl_v331.in());

      corba_server_adapter_->add_binding(
        CAMPAIGN_SERVER_V340_OBJ_KEY, campaign_server_impl_v340.in());
      */

      corba_server_adapter_->add_binding(
        CAMPAIGN_SERVER_V350_OBJ_KEY, campaign_server_impl_v350.in());
    }

    corba_server_adapter_->add_binding(
      CAMPAIGN_SERVER_OBJ_KEY, campaign_server_impl_.in());

    corba_server_adapter_->add_binding(
      PROCESS_CONTROL_OBJ_KEY, this);

    corba_server_adapter_->add_binding(PROCESS_STATS_CONTROL_OBJ_KEY,
      proc_stat_ctrl_.in());

    campaign_server_impl_->activate_object();

    // Running orb loop
    logger()->sstream(Logging::Logger::NOTICE, ASPECT) <<
      "service started.";

    corba_server_adapter_->run();

    wait();

    logger()->sstream(Logging::Logger::NOTICE, ASPECT) <<
      "service stopped.";
  }
  catch (const Exception& e)
  {
    Stream::Error ostr;
    ostr << "CampaignServerApp_::main(): "
      "Got CampaignServerApp_::Exception: " <<
      e.what();
    logger()->log(ostr.str(),
      Logging::Logger::CRITICAL,
      ASPECT,
      "ADS-IMPL-150");
  }
  catch (const CORBA::SystemException& e)
  {
    Stream::Error ostr;
    ostr << "CampaignServerApp_::main(): "
      "Got CORBA::SystemException: " << e;
    logger()->log(ostr.str(),
      Logging::Logger::EMERGENCY,
      ASPECT,
      "ADS-IMPL-150");
  }
  catch (const eh::Exception& e)
  {
    Stream::Error ostr;
    ostr << "CampaignServerApp_::main(): "
      "Got eh::Exception: " << e.what();
    logger()->log(ostr.str(),
      Logging::Logger::EMERGENCY,
      ASPECT,
      "ADS-IMPL-150");
  }
  catch (...)
  {
    logger()->log(String::SubString("CampaignServerApp_::main(): "
      "Got Unknown exception."),
      Logging::Logger::EMERGENCY,
      ASPECT,
      "ADS-IMPL-150");
  }
}

void
CampaignServerApp_::read_config(const char* filename, const char* argv0)
  /*throw(Exception, eh::Exception)*/
{
  static const char* FUN = "CampaignServerApp_::read_config()";

  try
  {
    using namespace xsd::AdServer::Configuration;

    Config::ErrorHandler error_handler;
    std::unique_ptr<CampaignServerType> configuration;

    try
    {
      std::unique_ptr<AdConfigurationType>
        ad_configuration = AdConfiguration(filename, error_handler);

      if(error_handler.has_errors())
      {
        std::string error_string;
        throw Exception(error_handler.text(error_string));
      }

      configuration =
        std::unique_ptr<CampaignServerType>(
          new CampaignServerType(ad_configuration->CampaignServer()));
    }
    catch(const xml_schema::parsing& e)
    {
      Stream::Error ostr;

      ostr << FUN << ": Can't parse config file '" << filename << "': ";

      if(error_handler.has_errors())
      {
        std::string error_string;
        ostr << error_handler.text(error_string);
      }

      throw Exception(ostr);
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't parse config file '" << filename << "': " <<
        e.what();
      throw Exception(ostr);
    }
    catch(...)
    {
      Stream::Error ostr;
      ostr << FUN << ": Unknown Exception at parsing of config.";
      throw Exception(ostr);
    }

    configuration_.log_root = configuration->log_root();
    configuration_.colo_update_flush_traits.period = 60;

    if(configuration->Logging().present())
    {
      if(configuration->Logging().get().ColoUpdateStat().present())
      {
        const xsd::AdServer::Configuration::CampaignServerLoggerType&
          xsd_logger = configuration->Logging().get().ColoUpdateStat().get();

        configuration_.colo_update_flush_traits.out_dir =
          configuration_.log_root;

        AdServer::PathManip::create_path(
          configuration_.colo_update_flush_traits.out_dir,
          AdServer::LogProcessing::ColoUpdateStatTraits::log_base_name());

        configuration_.colo_update_flush_traits.size =
          xsd_logger.max_size().present() ?
          xsd_logger.max_size().get(): 0;

        configuration_.colo_update_flush_traits.period =
          xsd_logger.flush_period().present() ?
          xsd_logger.flush_period().get() : 60;
      }
    }

    configuration_.config_update_period =
      configuration->config_update_period();
    configuration_.ecpm_update_period =
      configuration->ecpm_update_period();
    configuration_.bill_stat_update_period = Generics::Time(
      configuration->bill_stat_update_period());

    // Fill corba_config
    try
    {
      Config::CorbaConfigReader::read_config(
        configuration->CorbaConfig(),
        corba_config_);
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't read Corba Config: " << e.what();
      throw Exception(ostr);
    }

    if (configuration->SNMPConfig().present())
    {
      unsigned snmp_index = configuration->SNMPConfig().get().index().present() ?
        configuration->SNMPConfig().get().index().get() :
        getpid();

      configuration_.snmp_config.init(snmp_index,
        configuration->SNMPConfig().get().mib_dirs());
    }

    try
    {
      logger(Config::LoggerConfigReader::create(
        configuration->Logger(), argv0));
    }
    catch (const Config::LoggerConfigReader::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught LoggerConfigReader::Exception: " << e.what();
      throw Exception(ostr);
    }

    configuration_.server_id = configuration->server_id();
    configuration_.colo_id =
      configuration->colo_id().present() ?
      configuration->colo_id().get() : 0;
    configuration_.version =
      configuration->version().present() ?
      configuration->version().get() : "";
    configuration_.only_tags = false;

    if(configuration->campaign_statuses().present())
    {
      const std::string sval = configuration->campaign_statuses().get();
      if(sval == "active")
      {
        configuration_.campaign_statuses = "AP";
      }
      else if(sval == "virtual")
      {
        configuration_.campaign_statuses = "APV";
      }
      else if(sval == "all")
      {
        configuration_.campaign_statuses = "AIPV";
      }
    }
    else
    {
      configuration_.campaign_statuses = "AP"; /* default value */
    }

    if(configuration->channel_statuses().present())
    {
      const std::string sval = configuration->channel_statuses().get();
      if(sval == "active")
      {
        configuration_.channel_statuses = "AW";
      }
      else if(sval == "all")
      {
        configuration_.channel_statuses = "AIW";
      }
    }
    else
    {
      configuration_.channel_statuses = "AW";
    }

    if (configuration->ProxyMode().present())
    {
      configuration_.server_mode = Configuration::SM_PROXY;

      const xsd::AdServer::Configuration::CampaignProxyModeType& proxy_mode =
        configuration->ProxyMode().get();

      try
      {
        Config::CorbaConfigReader::read_multi_corba_ref(
          proxy_mode.CampaignServerCorbaRef(),
          configuration_.campaign_server_refs);
      }
      catch(const eh::Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": Can't read Campaign Server Corba Object Reference: " <<
          e.what();
        throw Exception(ostr);
      }

      if(proxy_mode.only_tags().present() && *proxy_mode.only_tags())
      {
        configuration_.only_tags = true;
      }

      if(proxy_mode.load_country().present())
      {
        configuration_.country = *proxy_mode.load_country();
      }
    }
    else if (configuration->ServerMode().present())
    {
      CampaignServerType::ServerMode_optional& server_mode =
        configuration->ServerMode();

      configuration_.pending_expire_time = Generics::Time(
        server_mode->pending_expire_time());
      configuration_.audience_expiration_time = Generics::Time(
        server_mode->audience_expiration_time());
      configuration_.enable_delivery_thresholds =
        server_mode->enable_delivery_thresholds();

      if (server_mode->PGConnection().present())
      {
        configuration_.server_mode = Configuration::SM_SERVER;
        configuration_.pg_connection_string =
          server_mode->PGConnection()->connection_string();
      }
      else
      {
        configuration_.server_mode = Configuration::SM_PROXY;
        try
        {
          Config::CorbaConfigReader::read_multi_corba_ref(
            *server_mode->CampaignServerCorbaRef(),
            configuration_.campaign_server_refs);
        }
        catch(const eh::Exception& e)
        {
          Stream::Error ostr;
          ostr << FUN << ": Can't read primary Campaign Servers Corba Object References: " <<
            e.what();
          throw Exception(ostr);
        }
      }

      configuration_.stat_stamp_sync_period =
        Generics::Time(server_mode->stat_stamp_sync_period());

      Config::CorbaConfigReader::read_multi_corba_ref(
        server_mode->LogGeneralizerCorbaRef(),
        configuration_.stat_provider_refs);
    }
    else
    {
      Stream::Error ostr;
      ostr << FUN << ": ServerMode or ProxyMode is not specified.";
      throw Exception(ostr);
    }
  }
  catch (const eh::Exception& e)
  {
    Stream::Error ostr;
    ostr << FUN << ": Invalid CampaignServer configuration : " << e.what();
    throw Exception(ostr);
  }
}

int
main(int argc, char** argv)
{
  CampaignServerApp_* app = 0;

  try
  {
    app = &CampaignServerApp::instance();
  }
  catch (...)
  {
    std::cerr << "main(): Critical: Got exception while "
      "creating application object.\n";
    return -1;
  }

  if (app == 0)
  {
    std::cerr << "main(): Critical: got NULL application object.\n";
    return -1;
  }

  app->main(argc, argv);
  app = 0;
}
