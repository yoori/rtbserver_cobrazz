#pragma once

#include <eh/Exception.hpp>

#include <ReferenceCounting/AtomicImpl.hpp>
#include <Logger/Logger.hpp>
#include <Logger/DistributorLogger.hpp>

#include <Frontends/FrontendCommons/HTTPUtils.hpp>
// #include <Frontends/FrontendCommons/CookieManager.hpp>
#include <Commons/TextTemplateCache.hpp>
#include <Commons/Grpc/GrpcSync.hpp>
#include <CampaignManagerGrpc.grpc-client.hpp>
#include <Frontends/FrontendCommons/CampaignManagerGrpcClientConfig.hpp>
#include <Frontends/FrontendCommons/HTTPExceptions.hpp>
#include <Frontends/FrontendCommons/FrontendInterface.hpp>
#include <Frontends/FrontendCommons/FrontendTaskPool.hpp>

namespace AdServer
{
  class ContentFrontend:
    public FrontendCommons::HTTPExceptions,
    private Logging::LoggerCallbackHolder,
    public FrontendCommons::FrontendTaskPool,
    public ReferenceCounting::AtomicImpl
  {
  public:
    typedef Configuration::FeConfig::CommonFeConfiguration_type
      CommonFeConfiguration;
    typedef Configuration::FeConfig::ContentFeConfiguration_type
      ContentFeConfiguration;

    typedef ReferenceCounting::SmartPtr<ContentFrontend>
      ContentFrontend_var;

  public:
    static ContentFrontend_var instance;

  public:
    ContentFrontend(
      Configuration* frontend_config,
      Logging::Logger* logger)
      /*throw(eh::Exception)*/;

    virtual void
    init() /*throw(eh::Exception)*/;

    virtual bool
    will_handle(
      const String::SubString& uri) noexcept;

    virtual void
    handle_request_noparams_(
      FCGI::HttpRequestHolder_var request_holder,
      FCGI::BaseHttpResponseWriter_var response_writer)
      noexcept;

    virtual void
    handle_request_(
      FCGI::HttpRequestHolder_var request_holder,
      FCGI::BaseHttpResponseWriter_var response_writer)
      noexcept;

    /** Performs shutdown for the module child process. */
    virtual void
    shutdown() noexcept;

    bool
    log(const String::SubString& text,
      unsigned long severity = Logging::Logger::INFO,
      const char* aspect = 0,
      const char* error_code = 0) const
      noexcept;

  private:
    typedef Commons::TextTemplateCacheConfiguration<
      Commons::TextTemplate>::Exception Exception;

    struct TraceLevel
    {
      enum
      {
        LOW = Logging::Logger::TRACE,
        MIDDLE,
        HIGH
      };
    };

    typedef std::unique_ptr<CommonFeConfiguration> CommonConfigPtr;
    typedef std::unique_ptr<ContentFeConfiguration> ConfigPtr;

    typedef std::map<std::string, std::string> TokenValueMap;

    struct TemplateRule
    {
      TokenValueMap tokens;
      std::string resource_url_prefix;
    };

    typedef std::list<std::string> StringList;

    typedef Generics::GnuHashTable<
      Generics::SubStringHashAdapter, TemplateRule>
      TemplateRuleMap;

  private:
    class CreativesUpdater :
      public Commons::TextTemplateCacheConfiguration<
        Commons::TextTemplate>::FarUpdater
    {
      typedef Commons::TextTemplateCacheConfiguration<
        Commons::TextTemplate> ConfigType;
      typedef ConfigType::Holder Holder;
      typedef ConfigType::Exception Exception;

      virtual ~CreativesUpdater() noexcept {}

      AdServer::CampaignSvcs::CampaignManagerGrpcAsyncClient&
        campaign_manager_;
    public:

      CreativesUpdater(
        AdServer::CampaignSvcs::CampaignManagerGrpcAsyncClient&
          campaign_manager)
        noexcept
        : campaign_manager_(campaign_manager)
      {}

      virtual Holder
      far_update(const char* file, const char* service_index) /*throw(Exception)*/
      {
        try
        {
          adserver::campaign_svcs::campaign_manager::GetFileRequest request;
          request.set_file_name(file);
          if(service_index)
          {
            request.set_service_index(service_index);
          }

          const auto response = AdServer::Grpc::sync_call<
            adserver::campaign_svcs::campaign_manager::GetFileResponse>(
              [this, &request](auto callback)
              {
                campaign_manager_.get_file(request, std::move(callback));
              },
              [](const grpc::Status& status)
              {
                Stream::Error ostr;
                ostr << "CampaignManager get_file failed: code=" <<
                  static_cast<int>(status.error_code()) <<
                  ", message=" << status.error_message();
                throw Exception(ostr);
              });

          String::SubString file_body(
            response.file().data(),
            response.file().size());
          Generics::Time now = Generics::Time::get_time_of_day();
          return new ConfigType::TextTemplateHolder(
            Commons::TextTemplate_var(new Commons::TextTemplate(file_body)),
            now,
            now,
            file_body.size());
        }
        catch (const eh::Exception& e)
        {
          Stream::Error ostr;
          ostr << "CreativesUpdater::far_update(): caugth eh::Exception: "
            << e.what();
          throw Exception(ostr);
        }
      }
    };

    virtual ~ContentFrontend() noexcept {};

    int
    handle_request_(
      const FCGI::HttpRequest& request,
      FCGI::HttpResponse& response)
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
    Commons::TextTemplateCache_var template_files_;
  };


}
