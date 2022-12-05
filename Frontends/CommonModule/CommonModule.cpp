#include <Logger/StreamLogger.hpp>
#include <Language/GenericSegmentor/Polyglot.hpp>
//#include <Language/ChineeseSegmentor/NLPIR.hpp>
#include <Language/SegmentorManager/SegmentorManager.hpp>
#include <Commons/ErrorHandler.hpp>
#include <Commons/CorbaConfig.hpp>
#include <Commons/CorbaAlgs.hpp>
#include <xsd/Frontends/FeConfig.hpp>

#include "CommonModule.hpp"

namespace Aspect
{
  const char COMMON_MODULE[] = "CommonModule";
}

namespace
{
  const char HANDLE_COMMAND_ERROR[] = "handle command error";
}

namespace AdServer
{
  namespace Configuration
  {
    using namespace xsd::AdServer::Configuration;
  };

  namespace
  {
    class UpdateTask: public Generics::GoalTask
    {
    public:
      UpdateTask(
        CommonModule* common_module,
        Generics::Planner* planner,
        Generics::TaskRunner* task_runner,
        const Generics::Time& update_period,
        unsigned service_index,
        Logging::Logger* logger)
        /*throw(eh::Exception)*/
        : Generics::GoalTask(planner, task_runner),
          common_module_(ReferenceCounting::add_ref(common_module)),
          update_period_(update_period),
          service_index_(service_index),
          logger_(ReferenceCounting::add_ref(logger))
      {}

      virtual void
      execute() noexcept
      {
        common_module_->update(service_index_);

        try
        {
          schedule(Generics::Time::get_time_of_day() + update_period_);
        }
        catch (const eh::Exception& ex)
        {
          logger_->sstream(Logging::Logger::EMERGENCY,
            Aspect::COMMON_MODULE) <<
            "UpdateTask::execute(): schedule failed: " << ex.what();
        }
      }

    protected:
      virtual
      ~UpdateTask() noexcept = default;

    private:
      CommonModule_var common_module_;
      const Generics::Time update_period_;
      unsigned service_index_;
      Logging::Logger_var logger_;
    };

    typedef ReferenceCounting::SmartPtr<UpdateTask> UpdateTask_var;
  }

  CommonModule::CommonModule(Logging::Logger* logger) /*throw(eh::Exception)*/
    : Logging::LoggerCallbackHolder(
        Logging::Logger_var(
          logger ? ReferenceCounting::add_ref(logger) :
          new Logging::OStream::Logger(Logging::OStream::Config(std::cerr))),
        "CommonModule", Aspect::COMMON_MODULE, 0)
  {
  }

  void
  CommonModule::parse_config_(
    CommonConfigPtr& common_config,
    DomainConfigPtr& domain_config)
    /*throw(Exception)*/
  {
    Config::ErrorHandler error_handler;
    try
    {
      std::unique_ptr<xsd::AdServer::Configuration::FeConfigurationType>
        fe_config(xsd::AdServer::Configuration::FeConfiguration(
          config_file_.c_str(), error_handler).release());

      if(error_handler.has_errors())
      {
        std::string error_string;
        throw Exception(error_handler.text(error_string));
      }

      if(!fe_config->CommonFeConfiguration().present())
      {
        throw Exception("CommonFeConfiguration not presented.");
      }

      common_config.reset(new CommonFeConfiguration(*fe_config->CommonFeConfiguration()));

      domain_config.reset(xsd::AdServer::Configuration::DomainConfiguration(
        common_config->domain_config_path(), error_handler).release());

      if(error_handler.has_errors())
      {
        std::string error_string;
        throw Exception(error_handler.text(error_string));
      }
      return;
    }
    catch(const xml_schema::parsing& e)
    {
      std::string str;
      Stream::Error ostr;
      ostr << "Can't parse config file '" << config_file_ << "': " <<
        error_handler.text(str);
      throw Exception(ostr);
    }
  }

  void
  CommonModule::init() noexcept
  {
    // static const char* FUN = "CommonModule::init()";

    CommonConfigPtr common_config;
    DomainConfigPtr domain_config;

    try
    {
      parse_config_(common_config, domain_config);

      corba_client_adapter_ = new CORBACommons::CorbaClientAdapter();

      task_runner_ = new Generics::TaskRunner(callback(), 2);
      add_child_object(task_runner_);

      scheduler_ = new Generics::Planner(callback());
      add_child_object(scheduler_);

      const CommonFeConfiguration::UserIdConfig_type&
        user_id_config = common_config->UserIdConfig();

      Commons::UserIdBlackList uid_blacklist;
      uid_blacklist.load(user_id_config, logger(), Aspect::COMMON_MODULE);

      // initialize uid verifier
      user_id_controller_ = new UserIdController(
        user_id_config.public_key().c_str(),
        user_id_config.temp_public_key().c_str(),
        user_id_config.private_key().c_str(),
        user_id_config.ssp_public_key().c_str(),
        user_id_config.ssp_private_key().c_str(),
        user_id_config.ssp_uid_key(),
        user_id_config.cache_size(),
        user_id_config.temp_cache_size(),
        user_id_config.ssp_cache_size(),
        uid_blacklist);

      domain_parser_ = new CampaignSvcs::DomainParser(*domain_config);

      // initialize campaign servers pool
      CORBACommons::CorbaObjectRefList campaign_server_refs;

      Config::CorbaConfigReader::read_multi_corba_ref(
        common_config->CampaignServerCorbaRef(),
        campaign_server_refs);

      CampaignServerPoolConfig pool_config(corba_client_adapter_.in());
      pool_config.timeout = Generics::Time(common_config->update_period());

      std::copy(
        campaign_server_refs.begin(),
        campaign_server_refs.end(),
        std::back_inserter(pool_config.iors_list));

      campaign_servers_.reset(new CampaignServerPool(
        pool_config,
        CORBACommons::ChoosePolicyType::PT_PERSISTENT));

      UpdateTask_var msg = new UpdateTask(
        this,
        scheduler_,
        task_runner_,
        Generics::Time(common_config->update_period()),
        common_config->service_index(),
        logger());

      msg->execute(); // Complete task before init done and

      // init IPMatcher
      if(common_config->IPMapping().present())
      {
        ip_matcher_ = new FrontendCommons::IPMatcher();

        for(Configuration::IPMappingType::Rule_sequence::const_iterator
              it = common_config->IPMapping()->Rule().begin();
            it != common_config->IPMapping()->Rule().end(); ++it)
        {
          FrontendCommons::IPMatcher::MatchResult match_result;
          match_result.colo_id = it->colo_id();
          match_result.profile_referer = it->profile_referer();
          ip_matcher_->add_rule(
            it->ip_range(),
            it->cohorts(),
            match_result);
        }
      }

      if(common_config->CountryFiltering().present())
      {
        country_filter_ = new FrontendCommons::CountryFilter();

        for(Configuration::CountryFilteringType::Country_sequence::const_iterator
              it = common_config->CountryFiltering()->Country().begin();
            it != common_config->CountryFiltering()->Country().end(); ++it)
        {
          country_filter_->enable_country(it->country_code());
        }
      }
      /*
      if(common_config->Chinese().present()) 
      {
        segmentor_ = new Language::Segmentor::Chineese::NlpirSegmentor(
          common_config->Chinese().get().base().c_str());
      }
      */
      else if(common_config->Polyglot().present())
      {
        segmentor_ = new Language::Segmentor::NormalizePolyglotSegmentor(
            common_config->Polyglot().get().base().c_str());
      }
      activate_object();
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << __func__ << ": eh::Exception: " << ex.what();
      logger()->log(ostr.str(),
        Logging::Logger::EMERGENCY,
        Aspect::COMMON_MODULE,
        "ADS-IMPL-133");

      ::kill(::getppid(), SIGTERM);
    }
  }

  void
  CommonModule::shutdown() noexcept
  {
    try
    {
      scheduler_->clear();
      
      deactivate_object();
      wait_object();

      Stream::Error ostr;
      ostr << "CommonModule::shutdown: frontend terminated (pid = " <<
        ::getpid() << ").";

      logger()->log(ostr.str(),
        Logging::Logger::INFO,
        Aspect::COMMON_MODULE);
    }
    catch(...)
    {}
  }

  void CommonModule::update(unsigned service_index) noexcept
  {
    static const char* FUN = "CommonModule::update()";

    try
    {
      for (;;)
      {
        CampaignServerPool::ObjectHandlerType campaign_server =
          campaign_servers_->get_object<Exception>(
            logger(),
            Logging::Logger::EMERGENCY,
            Aspect::COMMON_MODULE,
            "ADS-ICON-6",
            service_index,
            service_index);

        try
        {
          AdServer::CampaignSvcs::DetectorsConfig_var detectors =
            campaign_server->detectors(
              CorbaAlgs::pack_time(matchers_timestamp_));

          if(CorbaAlgs::unpack_time(detectors->timestamp) >
             matchers_timestamp_)
          {
            FrontendCommons::UrlMatcher_var new_url_matcher(
              new FrontendCommons::UrlMatcher());

            for(CORBA::ULong i = 0; i < detectors->engines.length(); ++i)
            {
              const AdServer::CampaignSvcs::SearchEngineInfo& se =
                detectors->engines[i];

              for(CORBA::ULong re_i = 0; re_i < se.regexps.length(); ++re_i)
              {
                new_url_matcher->add_rule(
                  se.id,
                  se.regexps[re_i].host_postfix,
                  se.regexps[re_i].regexp,
                  se.regexps[re_i].decoding_depth,
                  se.regexps[re_i].encoding,
                  se.regexps[re_i].post_encoding);
              }
            }

            FrontendCommons::WebBrowserMatcher_var new_web_browser_matcher(
              new FrontendCommons::WebBrowserMatcher());

            for(CORBA::ULong i = 0; i < detectors->web_browsers.length(); ++i)
            {
              const AdServer::CampaignSvcs::WebBrowserInfo& info =
                detectors->web_browsers[i];

              for(CORBA::ULong det_i = 0; det_i < info.detectors.length(); ++det_i)
              {
                new_web_browser_matcher->add_rule(
                  String::SubString(info.name),
                  info.detectors[det_i].marker,
                  info.detectors[det_i].regexp,
                  info.detectors[det_i].regexp_required,
                  info.detectors[det_i].priority);
              }
            }

            FrontendCommons::PlatformMatcher_var new_platform_matcher(
              new FrontendCommons::PlatformMatcher());

            for(CORBA::ULong i = 0; i < detectors->platforms.length(); ++i)
            {
              const AdServer::CampaignSvcs::PlatformInfo& info =
                detectors->platforms[i];

              for(CORBA::ULong det_i = 0; det_i < info.detectors.length(); ++det_i)
              {
                new_platform_matcher->add_rule(
                  info.platform_id,
                  String::SubString(info.name),
                  info.type,
                  info.detectors[det_i].use_name,
                  info.detectors[det_i].marker,
                  info.detectors[det_i].match_regexp,
                  info.detectors[det_i].output_regexp,
                  info.detectors[det_i].priority);
              }
            }

            // create new search engine matcher for received rules
            matchers_timestamp_ = CorbaAlgs::unpack_time(
              detectors->timestamp);

            SyncPolicy::WriteGuard lock(matchers_lock_);
            url_matcher_.swap(new_url_matcher);
            web_browser_matcher_.swap(new_web_browser_matcher);
            platform_matcher_.swap(new_platform_matcher);
          }
          return;
        }
        catch (const CORBA::SystemException& e)
        {
          Stream::Error ostr;
          ostr << FUN << ": caught CORBA::SystemException: " << e;
          campaign_server.release_bad(ostr.str());
          logger()->log(ostr.str(),
            Logging::Logger::CRITICAL,
            Aspect::COMMON_MODULE,
            "ADS-ICON-6");
        }
        catch (const AdServer::CampaignSvcs::CampaignServer::NotReady& )
        {
          logger()->sstream(Logging::Logger::NOTICE,
            Aspect::COMMON_MODULE,
            "ADS-IMPL-121") << FUN << ": Caught CampaignServer::NotReady";
          campaign_server.release_bad(
            String::SubString("CampaignServer is not ready"));
        }
        catch (const AdServer::CampaignSvcs::CampaignServer::
          ImplementationException& ex)
        {
          Stream::Error ostr;
          ostr << FUN << ": Caught CampaignServer::ImplementationException: "
            << ex.description;
          campaign_server.release_bad(ostr.str());
          logger()->log(ostr.str(),
            Logging::Logger::ERROR,
            Aspect::COMMON_MODULE,
            "ADS-IMPL-121");
        }
      }
    }
    catch (const eh::Exception& ex)
    {
      logger()->sstream(Logging::Logger::CRITICAL,
        Aspect::COMMON_MODULE,
        "ADS-IMPL-121") << FUN << ": Caught eh::Exception" << ex.what();
    }
  }

  AdServer::CampaignSvcs::ColocationFlagsSeq_var
  CommonModule::get_colocation_flags(unsigned service_index) /*throw(Exception)*/
  {
    static const char* FUN = "CommonModule::get_colocation_flags()";

    AdServer::CampaignSvcs::ColocationFlagsSeq_var result;

    try
    {
      for (;;)
      {
        CampaignServerPool::ObjectHandlerType campaign_server =
          campaign_servers_->get_object<Exception>(
            logger(),
            Logging::Logger::EMERGENCY,
            Aspect::COMMON_MODULE,
            "ADS-ICON-6",
            service_index,
            service_index);

        try
        {
          result = campaign_server->get_colocation_flags();
          return result;
        }
        catch (const CORBA::SystemException& e)
        {
          Stream::Error ostr;
          ostr << FUN << ": caught CORBA::SystemException: " << e;
          campaign_server.release_bad(ostr.str());
        }
        catch (const AdServer::CampaignSvcs::CampaignServer::NotReady& )
        {
          campaign_server.release_bad(String::SubString("CampaignServer is not ready"));
        }
        catch (const AdServer::CampaignSvcs::CampaignServer::
          ImplementationException& ex)
        {
          Stream::Error ostr;
          ostr << FUN << ": Caught CampaignServer::ImplementationException: "
            << ex.description;
          campaign_server.release_bad(ostr.str());
        }
      }
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception" << ex.what();
      throw Exception(ostr);
    }
  }

  FrontendCommons::UrlMatcher_var
  CommonModule::url_matcher() const noexcept
  {
    SyncPolicy::ReadGuard lock(matchers_lock_);
    return url_matcher_;
  }

  FrontendCommons::WebBrowserMatcher_var
  CommonModule::web_browser_matcher() const noexcept
  {
    SyncPolicy::ReadGuard lock(matchers_lock_);
    return web_browser_matcher_;
  }

  FrontendCommons::PlatformMatcher_var
  CommonModule::platform_matcher() const noexcept
  {
    SyncPolicy::ReadGuard lock(matchers_lock_);
    return platform_matcher_;
  }

  FrontendCommons::IPMatcher_var
  CommonModule::ip_matcher() const noexcept
  {
    return ip_matcher_;
  }

  FrontendCommons::CountryFilter_var
  CommonModule::country_filter() const noexcept
  {
    return country_filter_;
  }

  Language::Segmentor::SegmentorInterface_var
  CommonModule::segmentor() const noexcept
  {
    return segmentor_;
  }

}
