#pragma once

#include <memory>

#include <eh/Exception.hpp>

#include <ReferenceCounting/AtomicImpl.hpp>
#include <Logger/ActiveObjectCallback.hpp>
#include <Logger/Logger.hpp>
#include <Logger/DistributorLogger.hpp>

#include <Generics/CompositeActiveObject.hpp>

#include <Frontends/FrontendCommons/HTTPUtils.hpp>
#include <Commons/TextTemplateCache.hpp>
#include <CampaignManagerGrpc.grpc-client.hpp>
#include <Frontends/FrontendCommons/CampaignManagerGrpcClientConfig.hpp>
#include <Frontends/FrontendCommons/HTTPExceptions.hpp>
#include <Frontends/FrontendCommons/FrontendInterface.hpp>

#include "ContentFrontendWorkers.hpp"

namespace AdServer
{
  class ContentFrontend:
    public FrontendCommons::HTTPExceptions,
    private Logging::LoggerCallbackHolder,
    public virtual FrontendCommons::FrontendInterface,
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
      Logging::Logger* logger)
      /*throw(eh::Exception)*/;

    void
    init() override /*throw(eh::Exception)*/;

    bool
    will_handle(
      const String::SubString& uri) noexcept override;

    void
    handle_request_noparams(
      FCGI::HttpRequestHolder_var request_holder,
      FCGI::BaseHttpResponseWriter_var response_writer)
      noexcept override;

    void
    handle_request(
      FCGI::HttpRequestHolder_var request_holder,
      FCGI::BaseHttpResponseWriter_var response_writer)
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
      using Exception = ConfigType::Exception;

      ~CreativesUpdater() noexcept override;

      std::shared_ptr<AdServer::CampaignSvcs::CampaignManagerGrpcAsyncClient>
        campaign_manager_;
    public:

      CreativesUpdater(
        std::shared_ptr<AdServer::CampaignSvcs::CampaignManagerGrpcAsyncClient>
          campaign_manager)
        noexcept;

      Holder
      far_update(const char* file, const char* service_index)
        override /*throw(Exception)*/;
    };

    ~ContentFrontend() noexcept override = default;

    int
    handle_request_(
      const FCGI::HttpRequest& request,
      FCGI::HttpResponse& response)
      noexcept;

    void
    handle_request_noparams_(
      FCGI::HttpRequestHolder_var request_holder,
      FCGI::BaseHttpResponseWriter_var response_writer)
      noexcept;

    void
    handle_request_(
      FCGI::HttpRequestHolder_var request_holder,
      FCGI::BaseHttpResponseWriter_var response_writer)
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

    std::shared_ptr<AdServer::CampaignSvcs::CampaignManagerGrpcAsyncClient>
      campaign_manager_;
    ContentFrontendWorkers_var workers_;
    Commons::TextTemplateCache_var template_files_;
  };


}
