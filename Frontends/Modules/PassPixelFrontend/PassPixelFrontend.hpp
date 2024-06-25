#ifndef FRONTENDS_PASSPIXELFRONTEND_PASSPIXELFRONTEND_HPP
#define FRONTENDS_PASSPIXELFRONTEND_PASSPIXELFRONTEND_HPP

#include <eh/Exception.hpp>

#include <ReferenceCounting/AtomicImpl.hpp>
#include <Logger/Logger.hpp>
#include <Logger/DistributorLogger.hpp>
#include <Generics/FileCache.hpp>
#include <HTTP/Http.hpp>
#include <HTTP/HTTPCookie.hpp>
#include <CORBA/CORBACommons/CorbaAdapters.hpp>
#include <UServerUtils/Grpc/Common/Scheduler.hpp>
#include <userver/engine/task/task_processor.hpp>

#include <xsd/Frontends/FeConfig.hpp>

#include <Frontends/FrontendCommons/HTTPUtils.hpp>
#include <Frontends/FrontendCommons/CampaignManagersPool.hpp>
#include <Frontends/FrontendCommons/FrontendInterface.hpp>
#include <Frontends/CommonModule/CommonModule.hpp>
#include <Frontends/FrontendCommons/FrontendTaskPool.hpp>

#include "RequestInfoFiller.hpp"

namespace AdServer
{
namespace PassbackPixel
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
    using TaskProcessor = userver::engine::TaskProcessor;
    using SchedulerPtr = UServerUtils::Grpc::Common::SchedulerPtr;
    using Exception = FrontendCommons::HTTPExceptions::Exception;

  public:
    Frontend(
      TaskProcessor& task_processor,
      const SchedulerPtr& scheduler,
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

    /** Performs initialization for the module child process. */
    virtual void
    init() /*throw(eh::Exception)*/;

    /** Performs shutdown for the module child process. */
    virtual void
    shutdown() noexcept;

    int
    handle_track_request(const FrontendCommons::HttpRequest& request,
      FrontendCommons::HttpResponse& response)
      /*throw(ForbiddenException, InvalidParamException, eh::Exception)*/;

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

    typedef Configuration::FeConfig::PassPixelFeConfiguration_type
      PassPixelFeConfiguration;

    typedef std::unique_ptr<CommonFeConfiguration> CommonConfigPtr;
    typedef std::unique_ptr<PassPixelFeConfiguration> ConfigPtr;

    typedef Generics::FileCache<> FileCache;
    typedef FileCache::Cache_var FileCachePtr;

  private:
    virtual ~Frontend() noexcept;

    void parse_config_() /*throw(Exception)*/;

    virtual int
    handle_request_(
      const FrontendCommons::HttpRequest& request,
      FrontendCommons::HttpResponse& response)
      noexcept;

  private:
    TaskProcessor& task_processor_;
    const SchedulerPtr scheduler_;

    /* configuration */
    //std::string config_file_;
    CommonConfigPtr common_config_;
    ConfigPtr config_;
    Configuration_var frontend_config_;

    CommonModule_var common_module_;

    std::unique_ptr<RequestInfoFiller> request_info_filler_;
    CORBACommons::CorbaClientAdapter_var corba_client_adapter_;
    FileCachePtr track_pixel_;
    FrontendCommons::CampaignManagersPool<Exception> campaign_managers_;
  };
}
}

//
// Inlines
//
namespace AdServer
{
namespace PassbackPixel
{
  inline
  Frontend::~Frontend() noexcept
  {}
}
}

#endif // FRONTENDS_PASSPIXELFRONTEND_PASSPIXELFRONTEND_HPP
