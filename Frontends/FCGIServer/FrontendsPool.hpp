#pragma once

#include <vector>
#include <Logger/Logger.hpp>
#include <Logger/ActiveObjectCallback.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <Generics/CompositeMetricsProvider.hpp>

#include <Commons/ExecutorPool.hpp>
#include <Commons/FastScheduler.hpp>
#include <Commons/BoostAsioContextRunActiveObject.hpp>
#include <Commons/Grpc/GrpcExecutor.hpp>
#include <Frontends/FrontendCommons/FrontendInterface.hpp>
// TO FIX !!! :
#include <Frontends/Modules/BiddingFrontend/BiddingFrontendStat.hpp>
#include <Frontends/CommonModule/CommonModule.hpp>

namespace AdServer::Frontends
{
  class FrontendsPool :
    public virtual FrontendCommons::FrontendInterface,
    public virtual Generics::CompositeActiveObject,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    enum ModuleId
    {
      M_BIDDING,
      M_PUBPIXEL,
      M_CONTENT,
      M_DIRECTORY,
      M_WEBSTAT,
      M_ACTION,
      M_USERBIND,
      M_PASSBACK,
      M_PASSBACKPIXEL,
      M_OPTOUT,
      M_NULLAD,
      M_ADINST,
      M_CLICK,
      M_IMPRTRACK,
      M_AD
    };

    using ModuleIdArray = std::vector<ModuleId>;

  public:
    FrontendsPool(
      const char* config_path,
      const ModuleIdArray& modules,
      Logging::Logger* logger,
      StatHolder* stats,
      Generics::CompositeMetricsProvider* composite_metrics_provider,
      unsigned long grpc_coalesce_threads,
      unsigned long service_index);

    virtual bool
    will_handle(const String::SubString& uri) noexcept;

    virtual void
    handle_request(
      FCGI::HttpRequestHolder_var request_holder,
      FCGI::BaseHttpResponseWriter_var response_writer)
      noexcept;

    virtual void
    handle_request_noparams(
      FCGI::HttpRequestHolder_var request_holder,
      FCGI::BaseHttpResponseWriter_var response_writer)
      /*throw(eh::Exception)*/;

    virtual void
    init() /*throw(eh::Exception)*/;

  protected:
    virtual
    ~FrontendsPool() noexcept = default;

  private:
    template <class Frontend, typename Config, typename ...T>
    void
    init_frontend_(const char* name, const Config& cfg, T&&... params);

    void
    init_frontends_();

  private:
    Configuration_var config_;
    ModuleIdArray modules_;
    Logging::Logger_var logger_;
    Logging::ActiveObjectCallbackImpl_var callback_;
    StatHolder_var stats_;
    Generics::CompositeMetricsProvider_var composite_metrics_provider_;
    unsigned long service_index_;

    CommonModule_var common_module_;
    std::shared_ptr<AdServer::Commons::BoostAsioContextRunActiveObject> grpc_coalesce_runner_;
    std::shared_ptr<AdServer::Commons::ExecutorPool> request_workers_;
    std::shared_ptr<AdServer::Commons::FastScheduler> timeout_scheduler_;
    std::vector<std::string> frontend_names_;
    std::vector<FrontendCommons::Frontend_var> frontends_;
  };
}
