
#pragma once

#include <eh/Exception.hpp>

#include <ReferenceCounting/AtomicImpl.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Logger/Logger.hpp>
#include <Logger/DistributorLogger.hpp>
#include <HTTP/Http.hpp>
#include <HTTP/HTTPCookie.hpp>
#include <Frontends/FrontendCommons/FrontendInterface.hpp>

#include <xsd/Frontends/FeConfig.hpp>

#include <Commons/ExecutorPool.hpp>
#include <Commons/Grpc/GrpcExecutor.hpp>
#include <CampaignManagerGrpc.grpc-client.hpp>
#include <UserInfoManagerGrpc.grpc-client.hpp>
#include <Frontends/FrontendCommons/CampaignManagerGrpcClientConfig.hpp>
#include <Frontends/FrontendCommons/HTTPUtils.hpp>
#include <Frontends/FrontendCommons/FrontendWorkers.hpp>

#include "RequestInfoFiller.hpp"

namespace AdServer::Passback
{
  namespace Configuration
  {
    using namespace xsd::AdServer::Configuration;
  }

  class Frontend:
    private FrontendCommons::HTTPExceptions,
    private Logging::LoggerCallbackHolder,
    public virtual FrontendCommons::CoroFrontendInterface,
    public Generics::CompositeActiveObject,
    public virtual ReferenceCounting::AtomicImpl
  {
    typedef FrontendCommons::HTTPExceptions::Exception Exception;

  public:
    Frontend(
      Configuration* frontend_config,
      Logging::Logger* logger,
      std::shared_ptr<AdServer::Commons::ExecutorPool> request_workers,
      CommonModule* common_module)
      /*throw(eh::Exception)*/;

    virtual bool
    will_handle(const String::SubString& uri) noexcept;

    FrontendCommons::RequestTask
    co_handle_request(
      FCGI::HttpRequestHolder_var request_holder)
      noexcept override;

    virtual int
    handle_redirect_request(
      const FCGI::HttpRequest& request,
      FCGI::HttpResponse& response)
      /*throw(ForbiddenException, InvalidParamException, eh::Exception)*/;

    /** Performs initialization for the module child process. */
    virtual void
    init() /*throw(eh::Exception)*/;

    /** Performs shutdown for the module child process. */
    virtual void
    shutdown() noexcept;

  private:
    struct TraceLevel
    {
      enum
      {
        LOW = Logging::Logger::TRACE,
        MIDDLE,
        HIGH
      };
    };

    typedef Configuration::FeConfig::CommonFeConfiguration_type
      CommonFeConfiguration;

    typedef Configuration::FeConfig::PassFeConfiguration_type
      PassFeConfiguration;

    typedef std::unique_ptr<CommonFeConfiguration> CommonConfigPtr;
    typedef std::unique_ptr<PassFeConfiguration> ConfigPtr;

  private:
    virtual ~Frontend() noexcept;

    void parse_config_() /*throw(Exception)*/;

    virtual int
    process_request_(
      const FCGI::HttpRequest& request,
      FCGI::HttpResponse& response)
      noexcept;

    FrontendCommons::RequestTask
    co_consider_passback_(
      adserver::campaign_svcs::campaign_manager::ConsiderPassbackRequest
        request)
      noexcept;

    FrontendCommons::RequestTask
    co_confirm_user_freq_caps_(
      adserver::user_info_svcs::user_info_manager::ConfirmUserFreqCapsRequest
        request)
      noexcept;

  private:
    /* configuration */
    std::string config_file_;
    CommonConfigPtr common_config_;
    ConfigPtr config_;
    Configuration_var frontend_config_;

    CommonModule_var common_module_;

    std::unique_ptr<RequestInfoFiller> request_info_filler_;
    std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor_;
    std::shared_ptr<AdServer::CampaignSvcs::CampaignManagerGrpcCoroClient>
      campaign_manager_coro_;
    std::shared_ptr<AdServer::UserInfoSvcs::UserInfoManagerGrpcCoroClient>
      user_info_client_coro_;
    std::shared_ptr<AdServer::Commons::ExecutorPool> workers_;
  };
}

//
// Inlines
//
namespace AdServer::Passback
{
  inline
  Frontend::~Frontend() noexcept
  {}
}
