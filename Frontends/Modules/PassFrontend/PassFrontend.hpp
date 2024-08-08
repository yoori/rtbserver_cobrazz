
#ifndef _AD_SERVER_PASS_FRONTEND_HPP_
#define _AD_SERVER_PASS_FRONTEND_HPP_

#include <eh/Exception.hpp>

#include <ReferenceCounting/AtomicImpl.hpp>
#include <Logger/Logger.hpp>
#include <Logger/DistributorLogger.hpp>
#include <HTTP/Http.hpp>
#include <HTTP/HTTPCookie.hpp>
#include <CORBA/CORBACommons/CorbaAdapters.hpp>

#include <xsd/Frontends/FeConfig.hpp>

#include <Frontends/FrontendCommons/HTTPUtils.hpp>
#include <Frontends/FrontendCommons/CampaignManagersPool.hpp>
#include <Frontends/FrontendCommons/FrontendInterface.hpp>
#include <Frontends/FrontendCommons/FrontendTaskPool.hpp>
#include <Frontends/FrontendCommons/GrpcContainer.hpp>

#include "RequestInfoFiller.hpp"

namespace AdServer
{
namespace Passback
{
  namespace Configuration
  {
    using namespace xsd::AdServer::Configuration;
  }

  class Frontend:
    private FrontendCommons::HTTPExceptions,
    private Logging::LoggerCallbackHolder,
    public FrontendCommons::FrontendTaskPool,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    using GrpcContainerPtr = FrontendCommons::GrpcContainerPtr;
    using Exception = FrontendCommons::HTTPExceptions::Exception;

  public:
    Frontend(
      const GrpcContainerPtr& grpc_container,
      Configuration* frontend_config,
      Logging::Logger* logger,
      CommonModule* common_module,
      FrontendCommons::HttpResponseFactory* response_factory)
      /*throw(eh::Exception)*/;

    virtual bool
    will_handle(const String::SubString& uri) noexcept;

    void
    handle_request_(
      FrontendCommons::HttpRequestHolder_var request_holder,
      FrontendCommons::HttpResponseWriter_var response_writer)
      noexcept;

    virtual int
    handle_redirect_request(
      const FrontendCommons::HttpRequest& request,
      FrontendCommons::HttpResponse& response)
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
    handle_request_(
      const FrontendCommons::HttpRequest& request,
      FrontendCommons::HttpResponse& response)
      noexcept;

  private:
    const GrpcContainerPtr grpc_container_;

    /* configuration */
    std::string config_file_;
    CommonConfigPtr common_config_;
    ConfigPtr config_;
    Configuration_var frontend_config_;

    CommonModule_var common_module_;

    std::unique_ptr<RequestInfoFiller> request_info_filler_;
    CORBACommons::CorbaClientAdapter_var corba_client_adapter_;
    FrontendCommons::CampaignManagersPool<Exception> campaign_managers_;
    FrontendCommons::UserInfoClient_var user_info_client_;
  };
}
}

//
// Inlines
//
namespace AdServer
{
namespace Passback
{
  inline
  Frontend::~Frontend() noexcept
  {}
}
}

#endif // _AD_SERVER_PASS_FRONTEND_HPP_
