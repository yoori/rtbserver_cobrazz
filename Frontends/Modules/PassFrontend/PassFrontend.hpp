
#ifndef _AD_SERVER_PASS_FRONTEND_HPP_
#define _AD_SERVER_PASS_FRONTEND_HPP_

#include <eh/Exception.hpp>

#include <ReferenceCounting/AtomicImpl.hpp>
#include <Logger/Logger.hpp>
#include <Logger/DistributorLogger.hpp>
#include <HTTP/Http.hpp>
#include <HTTP/HTTPCookie.hpp>
#include <CORBA/CORBACommons/CorbaAdapters.hpp>
#include <Frontends/FrontendCommons/FrontendInterface.hpp>

#include <xsd/Frontends/FeConfig.hpp>

#include <Frontends/FrontendCommons/HTTPUtils.hpp>
#include <Frontends/FrontendCommons/CampaignManagersPool.hpp>
#include <Frontends/FrontendCommons/FrontendTaskPool.hpp>

#include <UServerUtils/Grpc/ComponentsBuilder.hpp>
#include <UServerUtils/Grpc/Manager.hpp>

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
  private:
    using ComponentsBuilder = UServerUtils::Grpc::ComponentsBuilder;
    using ManagerCoro = UServerUtils::Grpc::Manager;
    using ManagerCoro_var = UServerUtils::Grpc::Manager_var;
    using TaskProcessorContainer = UServerUtils::Grpc::TaskProcessorContainer;
    using Exception = FrontendCommons::HTTPExceptions::Exception;

  public:
    Frontend(
      Configuration* frontend_config,
      Logging::Logger* logger,
      CommonModule* common_module)
      /*throw(eh::Exception)*/;

    virtual bool
    will_handle(const String::SubString& uri) noexcept;

    void
    handle_request_(
      FCGI::HttpRequestHolder_var request_holder,
      FCGI::HttpResponseWriter_var response_writer)
      noexcept;

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
    handle_request_(
      const FCGI::HttpRequest& request,
      FCGI::HttpResponse& response)
      noexcept;

  private:
    /* configuration */
    std::string config_file_;
    CommonConfigPtr common_config_;
    ConfigPtr config_;
    Configuration_var frontend_config_;

    CommonModule_var common_module_;
    ManagerCoro_var manager_coro_;

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
