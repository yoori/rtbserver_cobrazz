#pragma once

#include <string>

#include <eh/Exception.hpp>

#include <ReferenceCounting/AtomicImpl.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Generics/ActiveObject.hpp>
#include <Generics/FileCache.hpp>
#include <Logger/Logger.hpp>
#include <Logger/DistributorLogger.hpp>
#include <Logger/ActiveObjectCallback.hpp>
#include <String/StringManip.hpp>
#include <HTTP/Http.hpp>
#include <HTTP/HTTPCookie.hpp>
//#include <Apache/Adapters.hpp>

#include <xsd/Frontends/FeConfig.hpp>
#include <Commons/Grpc/GrpcExecutor.hpp>
#include <CampaignManagerGrpc.grpc-client.hpp>
#include <Frontends/FrontendCommons/CampaignManagerGrpcClientConfig.hpp>
#include <Frontends/FrontendCommons/HTTPUtils.hpp>
#include <Frontends/FrontendCommons/FrontendInterface.hpp>
#include <Frontends/FrontendCommons/FrontendWorkers.hpp>

#include "RequestInfoFiller.hpp"

namespace AdServer
{
namespace WebStat
{
  class Frontend:
    private FrontendCommons::HTTPExceptions,
    private Logging::LoggerCallbackHolder,
    public virtual FrontendCommons::FrontendInterface,
    public Generics::CompositeActiveObject,
    public ReferenceCounting::AtomicImpl
  {
  public:
    typedef FrontendCommons::HTTPExceptions::Exception Exception;

  public:
    Frontend(
      Configuration* frontend_config,
      Logging::Logger* logger,
      CommonModule* common_module)
      /*throw(eh::Exception)*/;

    virtual bool
    will_handle(const String::SubString& uri) noexcept;

    void
    handle_request(
      FCGI::HttpRequestHolder_var request_holder,
      FCGI::BaseHttpResponseWriter_var response_writer)
      noexcept override;

    void
    handle_request_(
      FCGI::HttpRequestHolder_var request_holder,
      FCGI::BaseHttpResponseWriter_var response_writer)
      noexcept;

    /** Performs initialization for the module child process. */
    virtual void
    init() /*throw(eh::Exception)*/;

    /** Performs shutdown for the module child process. */
    virtual void
    shutdown() noexcept;

  private:
    typedef Configuration::FeConfig::CommonFeConfiguration_type
      CommonFeConfiguration;

    typedef Configuration::FeConfig::WebStatFeConfiguration_type
      WebStatFeConfiguration;

    typedef std::unique_ptr<CommonFeConfiguration> CommonConfigPtr;
    typedef std::unique_ptr<WebStatFeConfiguration> ConfigPtr;

    typedef Generics::FileCache<> FileCache;
    typedef FileCache::Cache_var FileCachePtr;

  private:
    virtual ~Frontend() noexcept;

    void parse_config_() /*throw(Exception)*/;

    class WebStatRequestState;

    void
    process_request_(
      FCGI::HttpRequestHolder_var request_holder,
      FCGI::BaseHttpResponseWriter_var response_writer,
      FCGI::HttpResponse_var response)
      noexcept;

    void
    consider_web_operation_(
      const std::shared_ptr<WebStatRequestState>& state,
      std::size_t index)
      noexcept;

    void
    finish_request_(
      const std::shared_ptr<WebStatRequestState>& state,
      int http_result)
      noexcept;

  private:
    // configuration
    Configuration_var frontend_config_;
    CommonConfigPtr common_config_;
    ConfigPtr config_;
    CommonModule_var common_module_;

    std::unique_ptr<RequestInfoFiller> request_info_filler_;
    std::shared_ptr<AdServer::CampaignSvcs::CampaignManagerGrpcAsyncClient>
      campaign_manager_;
    std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor_;
    FrontendCommons::FrontendWorkers_var workers_;

    FileCachePtr pixel_;
  };
} /*WebStat*/
} /*AdServer*/

//
// Inlines
//
namespace AdServer
{
namespace WebStat
{
  inline
  Frontend::~Frontend() noexcept
  {
  }
}
} /* AdServer */
