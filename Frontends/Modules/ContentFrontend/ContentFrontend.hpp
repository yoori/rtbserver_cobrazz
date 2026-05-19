#pragma once

#include <memory>

#include <eh/Exception.hpp>

#include <ReferenceCounting/AtomicImpl.hpp>
#include <Logger/ActiveObjectCallback.hpp>
#include <Logger/Logger.hpp>
#include <Logger/DistributorLogger.hpp>

#include <Generics/CompositeActiveObject.hpp>

#include <Commons/ExecutorPool.hpp>
#include <Frontends/FrontendCommons/HTTPUtils.hpp>
#include <Commons/TextTemplateCache.hpp>
#include <CampaignManagerGrpc.grpc-client.hpp>
#include <Frontends/FrontendCommons/CampaignManagerGrpcClientConfig.hpp>
#include <Frontends/FrontendCommons/HTTPExceptions.hpp>
#include <Frontends/FrontendCommons/FrontendInterface.hpp>

namespace AdServer
{
  class ContentFrontend:
    public FrontendCommons::HTTPExceptions,
    private Logging::LoggerCallbackHolder,
    public virtual FrontendCommons::CoroFrontendInterface,
    public Generics::CompositeActiveObject,
    public ReferenceCounting::AtomicImpl
  {
  public:
    using CommonFeConfiguration =
      Configuration::FeConfig::CommonFeConfiguration_type;
    using ContentFeConfiguration =
      Configuration::FeConfig::ContentFeConfiguration_type;

    using ContentFrontend_var =
      ReferenceCounting::SmartPtr<ContentFrontend>;

  public:
    ContentFrontend(
      Configuration* frontend_config,
      Logging::Logger* logger,
      std::shared_ptr<AdServer::Commons::ExecutorPool> request_workers)
      /*throw(eh::Exception)*/;

    void
    init() override /*throw(eh::Exception)*/;

    bool
    will_handle(
      const String::SubString& uri) noexcept override;

    FrontendCommons::RequestTask
    co_handle_request_noparams(
      FCGI::HttpRequestHolder_var request_holder)
      noexcept override;

    FrontendCommons::RequestTask
    co_handle_request(
      FCGI::HttpRequestHolder_var request_holder)
      noexcept override;

    /** Performs shutdown for the module child process. */
    void
    shutdown() noexcept override;

    bool
    log(const String::SubString& text,
      unsigned long severity = Logging::Logger::INFO,
      const char* aspect = 0,
      const char* error_code = 0) const
      noexcept;

  private:
    using Exception = Commons::TextTemplateCacheConfiguration<
      Commons::TextTemplate>::Exception;

    struct TraceLevel
    {
      enum
      {
        LOW = Logging::Logger::TRACE,
        MIDDLE,
        HIGH
      };
    };

    using CommonConfigPtr = std::unique_ptr<CommonFeConfiguration>;
    using ConfigPtr = std::unique_ptr<ContentFeConfiguration>;

    using TokenValueMap = std::map<std::string, std::string>;

    struct TemplateRule
    {
      TokenValueMap tokens;
      std::string resource_url_prefix;
    };

    using StringList = std::list<std::string>;

    using TemplateRuleMap = Generics::GnuHashTable<
      Generics::SubStringHashAdapter, TemplateRule>;

  private:
    class CreativesUpdater :
      public Commons::TextTemplateCacheConfiguration<
        Commons::TextTemplate>::FarUpdater
    {
      using ConfigType = Commons::TextTemplateCacheConfiguration<
        Commons::TextTemplate>;
      using Holder = ConfigType::Holder;

      ~CreativesUpdater() noexcept override;

      std::shared_ptr<AdServer::CampaignSvcs::CampaignManagerGrpcCoroClient>
        campaign_manager_coro_;

      FrontendCommons::RequestTask
      co_far_update_(
        adserver::campaign_svcs::campaign_manager::GetFileRequest request,
        ConfigType::UpdateCallback callback)
        noexcept;

    public:

      CreativesUpdater(
        std::shared_ptr<AdServer::CampaignSvcs::CampaignManagerGrpcCoroClient>
          campaign_manager_coro)
        noexcept;

      void
      far_update_async(
        const char* file,
        const char* service_index,
        ConfigType::UpdateCallback callback)
        noexcept override;
    };

    ~ContentFrontend() noexcept override = default;

    FrontendCommons::RequestTask
    process_request_(
      FCGI::HttpRequestHolder_var request_holder,
      FCGI::HttpResponse_var response)
      noexcept;

    int
    fill_response_(
      FCGI::HttpResponse& response,
      Commons::TextTemplate* templ,
      const Generics::SubStringHashAdapter& instantiate_type,
      const std::string& click_url0,
      const std::string& pub_preclick_url,
      const std::string& resource_url_suffix,
      const std::string& random_str)
      const /*throw(eh::Exception)*/;

    FrontendCommons::RequestTask
    handle_request_noparams_(
      FCGI::HttpRequestHolder_var request_holder)
      noexcept;

    FrontendCommons::RequestTask
    co_process_request_(
      FCGI::HttpRequestHolder_var request_holder)
      noexcept;

    void parse_configs_() /*throw(Exception)*/;

    void parse_headers_(
      const FCGI::HttpRequest& request,
      bool& secure) noexcept;

  private:
    Configuration_var frontend_config_;
    CommonConfigPtr common_config_;
    ConfigPtr config_;

    StringList strings_; // string's holder for SubString using
    TemplateRuleMap template_rules_;
    std::shared_ptr<AdServer::CampaignSvcs::CampaignManagerGrpcCoroClient>
      campaign_manager_coro_;
    std::shared_ptr<AdServer::Commons::ExecutorPool> workers_;
    Commons::TextTemplateCache_var template_files_;
  };


}
