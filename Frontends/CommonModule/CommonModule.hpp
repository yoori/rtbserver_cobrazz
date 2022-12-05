#pragma once

#include <eh/Exception.hpp>

#include <ReferenceCounting/AtomicImpl.hpp>
#include <Sync/SyncPolicy.hpp>
#include <Generics/TaskRunner.hpp>
#include <Language/SegmentorCommons/SegmentorInterface.hpp>
#include <Logger/Logger.hpp>
#include <Logger/ActiveObjectCallback.hpp>
#include <Logger/StreamLogger.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <CORBACommons/CorbaAdapters.hpp>
#include <CORBACommons/ObjectPool.hpp>

#include <CampaignSvcs/CampaignServer/CampaignServer.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignSvcsVersionAdapter.hpp>
#include <CampaignSvcs/CampaignManager/DomainParser.hpp>
#include <xsd/Frontends/FeConfig.hpp>
#include <Frontends/FrontendCommons/RequestMatchers.hpp>
#include <Frontends/FrontendCommons/UserIdController.hpp>

namespace AdServer
{
  namespace Configuration
  {
    using namespace xsd::AdServer::Configuration;
  }

  class CommonModule:
    private Logging::LoggerCallbackHolder,
    private virtual Generics::CompositeActiveObject,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    CommonModule(Logging::Logger* logger = 0) /*throw(eh::Exception)*/;

    void
    init() noexcept;

    void
    shutdown() noexcept;

    UserIdController_var
    user_id_controller() const noexcept;

    FrontendCommons::UrlMatcher_var
    url_matcher() const noexcept;

    FrontendCommons::WebBrowserMatcher_var
    web_browser_matcher() const noexcept;

    FrontendCommons::PlatformMatcher_var
    platform_matcher() const noexcept;

    FrontendCommons::IPMatcher_var
    ip_matcher() const noexcept;

    FrontendCommons::CountryFilter_var
    country_filter() const noexcept;

    Language::Segmentor::SegmentorInterface_var
    segmentor() const noexcept;

    void
    update(unsigned service_index) noexcept;

    CampaignSvcs::DomainParser_var
    domain_parser() const noexcept;

    AdServer::CampaignSvcs::ColocationFlagsSeq_var
    get_colocation_flags(unsigned service_index) /*throw(Exception)*/;

    void set_config_file(const char* config_file) noexcept;

  private:
    typedef Sync::Policy::PosixThreadRW SyncPolicy;

    typedef CORBACommons::ObjectPoolRefConfiguration
      CampaignServerPoolConfig;

    typedef CORBACommons::ObjectPool<
      AdServer::CampaignSvcs::CampaignServer,
      CampaignServerPoolConfig>
      CampaignServerPool;

    typedef Configuration::CommonFeConfigurationType
      CommonFeConfiguration;
    typedef Configuration::DomainConfigurationType
      DomainConfiguration;

    typedef std::unique_ptr<CampaignServerPool> CampaignServerPoolPtr;

    typedef std::unique_ptr<CommonFeConfiguration> CommonConfigPtr;
    typedef std::unique_ptr<DomainConfiguration> DomainConfigPtr;

  protected:
    virtual
    ~CommonModule() noexcept;

  private:
    void parse_config_(
      CommonConfigPtr& common_config,
      DomainConfigPtr& domain_config)
      /*throw(Exception)*/;

  private:
    /* configuration */
    std::string config_file_;

    Generics::TaskRunner_var task_runner_;
    Generics::Planner_var scheduler_;
    CORBACommons::CorbaClientAdapter_var corba_client_adapter_;

    UserIdController_var user_id_controller_;

    CampaignServerPoolPtr campaign_servers_;
    mutable SyncPolicy::Mutex matchers_lock_;
    FrontendCommons::UrlMatcher_var url_matcher_;
    FrontendCommons::WebBrowserMatcher_var web_browser_matcher_;
    FrontendCommons::PlatformMatcher_var platform_matcher_;
    FrontendCommons::IPMatcher_var ip_matcher_;
    FrontendCommons::CountryFilter_var country_filter_;
    Generics::Time matchers_timestamp_;

    CampaignSvcs::DomainParser_var domain_parser_;
    Language::Segmentor::SegmentorInterface_var segmentor_;

  };

  typedef ReferenceCounting::SmartPtr<CommonModule> CommonModule_var;
}

/* Inlines */
namespace AdServer
{
  inline
  CommonModule::~CommonModule() noexcept
  {}

  inline
  void
  CommonModule::set_config_file(const char* config_file) noexcept
  {
    config_file_ = config_file;
  }

  inline
  UserIdController_var
  CommonModule::user_id_controller() const noexcept
  {
    return user_id_controller_;
  }

  inline
  CampaignSvcs::DomainParser_var
  CommonModule::domain_parser() const noexcept
  {
    return domain_parser_;
  }
} // namespace AdServer
