#ifndef FRONTENDS_CONTENTFRONTEND_HPP
#define FRONTENDS_CONTENTFRONTEND_HPP

#include <eh/Exception.hpp>

#include <ReferenceCounting/AtomicImpl.hpp>
#include <Logger/Logger.hpp>
#include <Logger/DistributorLogger.hpp>

#include <Commons/TextTemplateCache.hpp>
#include <Frontends/FrontendCommons/CampaignManagersPool.hpp>
#include <Frontends/FrontendCommons/HTTPExceptions.hpp>
#include <Frontends/FrontendCommons/HTTPUtils.hpp>
#include <Frontends/FrontendCommons/FrontendInterface.hpp>
#include <Frontends/FrontendCommons/FrontendTaskPool.hpp>
#include <Frontends/FrontendCommons/GrpcContainer.hpp>

namespace AdServer
{
  class ContentFrontend:
    public FrontendCommons::HTTPExceptions,
    private Logging::LoggerCallbackHolder,
    public FrontendCommons::FrontendTaskPool,
    public ReferenceCounting::AtomicImpl
  {
  public:
    using TaskProcessor = userver::engine::TaskProcessor;
    using GrpcContainerPtr = FrontendCommons::GrpcContainerPtr;
    using CommonFeConfiguration = Configuration::FeConfig::CommonFeConfiguration_type;
    using ContentFeConfiguration = Configuration::FeConfig::ContentFeConfiguration_type;
    using ContentFrontend_var = ReferenceCounting::SmartPtr<ContentFrontend>;

  public:
    static ContentFrontend_var instance;

  public:
    ContentFrontend(
      TaskProcessor& helper_task_processor,
      const GrpcContainerPtr& grpc_container,
      Configuration* frontend_config,
      Logging::Logger* logger,
      FrontendCommons::HttpResponseFactory* response_factory)
      /*throw(eh::Exception)*/;

    virtual void
    init() /*throw(eh::Exception)*/;

    virtual bool
    will_handle(
      const String::SubString& uri) noexcept;

    virtual void
    handle_request_noparams_(
      FrontendCommons::HttpRequestHolder_var request_holder,
      FrontendCommons::HttpResponseWriter_var response_writer)
      noexcept;

    virtual void
    handle_request_(
      FrontendCommons::HttpRequestHolder_var request_holder,
      FrontendCommons::HttpResponseWriter_var response_writer)
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
      using ConfigType = Commons::TextTemplateCacheConfiguration<
        Commons::TextTemplate>;
      using Holder = ConfigType::Holder;
      using Exception = ConfigType::Exception;
      using GrpcCampaignManagerPool = FrontendCommons::GrpcCampaignManagerPool;
      using GrpcCampaignManagerPoolPtr = std::shared_ptr<GrpcCampaignManagerPool>;

      virtual ~CreativesUpdater() noexcept {}

      FrontendCommons::CampaignManagersPool<Exception>& campaign_managers_;
      GrpcCampaignManagerPoolPtr grpc_campaign_manager_pool_;
    public:

      CreativesUpdater(
        FrontendCommons::CampaignManagersPool<Exception>& campaign_managers,
        const GrpcCampaignManagerPoolPtr& grpc_campaign_manager_pool) noexcept
        : campaign_managers_(campaign_managers),
          grpc_campaign_manager_pool_(grpc_campaign_manager_pool)
      {}

      virtual Holder
      far_update(const char* file, const char* service_index) /*throw(Exception)*/
      {
        String::SubString file_body;
        CORBACommons::OctSeq_var content;
        GrpcCampaignManagerPool::GetFileResponsePtr response;

        bool is_grpc_success = false;
        if (grpc_campaign_manager_pool_)
        {
          try
          {
            response = grpc_campaign_manager_pool_->get_file(
              service_index,
              file);

            if (response && response->has_info())
            {
              auto& info_proto = response->info();
              const auto& file_proto = info_proto.file();
              file_body = file_proto;
              is_grpc_success = true;
            }
          }
          catch (...)
          {
          }
        }

        if (!is_grpc_success)
        {
          campaign_managers_.get_file(file, content, service_index);
          file_body = String::SubString(reinterpret_cast<char*>(content->get_buffer()), content->length());
        }

        try
        {
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
      const FrontendCommons::HttpRequest& request,
      FrontendCommons::HttpResponse& response)
      noexcept;

    void parse_configs_() /*throw(Exception)*/;

    void parse_headers_(
      const FrontendCommons::HttpRequest& request,
      bool& secure) noexcept;

  private:
    TaskProcessor& helper_task_processor_;
    const GrpcContainerPtr grpc_container_;

    Configuration_var frontend_config_;
    CommonConfigPtr common_config_;
    ConfigPtr config_;

    StringList strings_; // string's holder for SubString using
    TemplateRuleMap template_rules_;

    CORBACommons::CorbaClientAdapter_var corba_client_adapter_;
    FrontendCommons::CampaignManagersPool<Exception> campaign_managers_;
    Commons::TextTemplateCache_var template_files_;
  };


}

#endif /*FRONTENDS_CONTENTFRONTEND_HPP*/
