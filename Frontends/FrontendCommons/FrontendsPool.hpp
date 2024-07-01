#ifndef FRONTENDCOMMONS_FRONTENDSPOOL_H
#define FRONTENDCOMMONS_FRONTENDSPOOL_H

// STD
#include <vector>

// UNIXCOMMONS
#include <Logger/Logger.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <UServerUtils/Grpc/Common/Scheduler.hpp>

// USERVER
#include <userver/engine/task/task_processor.hpp>

// THIS
#include <BiddingFrontend/BiddingFrontendStat.hpp>
#include <Frontends/CommonModule/CommonModule.hpp>
#include <Frontends/FrontendCommons/FrontendInterface.hpp>
#include <Frontends/FrontendCommons/GrpcContainer.hpp>
#include <Frontends/FrontendCommons/HttpResponse.hpp>


namespace AdServer
{
  namespace Frontends
  {
    /**
     * @class FrontendsPool
     *
     * @brief HTTP frontends pool.
     */
    class FrontendsPool final :
      public virtual FrontendCommons::FrontendInterface,
      public virtual ReferenceCounting::AtomicImpl  
    {
    public:
      using TaskProcessor = userver::engine::TaskProcessor;
      using SchedulerPtr = UServerUtils::Grpc::Common::SchedulerPtr;
      using GrpcContainerPtr = FrontendCommons::GrpcContainerPtr;

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
        M_AD,
        M_ECHO
      };

      typedef std::vector<ModuleId> ModuleIdArray;

    public:
      /**
       * @brief Constructor.
       * @param config path
       */
      FrontendsPool(
        const GrpcContainerPtr& grpc_container,
        TaskProcessor& task_processor,
        const SchedulerPtr& scheduler,
        const char* config_path,
        const ModuleIdArray& modules,
        Logging::Logger* logger,
        StatHolder* stats,
        FrontendCommons::HttpResponseFactory* response_factory);

      /**
       * @brief Handle or not URI.
       * @param uri.
       */
      virtual bool
      will_handle(const String::SubString& uri) noexcept;

      /**
       * @brief Handle HTTP request.
       * @param HTTP request
       * @param[out] HTTP response
       */
      virtual void
      handle_request(
        FrontendCommons::HttpRequestHolder_var request_holder,
        FrontendCommons::HttpResponseWriter_var response_writer)
        noexcept;

      /**
       * @brief Handle HTTP request without params.
       * @param HTTP request
       * @param[out] HTTP response
       */
      virtual void
      handle_request_noparams(
        FrontendCommons::HttpRequestHolder_var request_holder,
        FrontendCommons::HttpResponseWriter_var response_writer)
        /*throw(eh::Exception)*/;

      /**
       * @brief Initialize frontend.
       */
      virtual void
      init() /*throw(eh::Exception)*/;

      /**
       * @brief Shutdown frontend.
       */
      virtual void
      shutdown() noexcept;

    protected:
      virtual
      ~FrontendsPool() noexcept = default;

    private:
      /**
       * @brief Init a frontend.
       * @param frontend config
       * @param frontend params
       */
      template <class Frontend, typename Config, typename ...T>
      void
      init_frontend(
        const Config& cfg,
        T&&... params);

    private:
      const GrpcContainerPtr grpc_container_;

      TaskProcessor& task_processor_;

      SchedulerPtr scheduler_;

      Configuration_var config_;

      ModuleIdArray modules_;

      Logging::Logger_var logger_;

      StatHolder_var stats_;

      FrontendCommons::HttpResponseFactory_var http_response_factory_;

      CommonModule_var common_module_;

      std::vector<FrontendCommons::Frontend_var> frontends_;
    };
  }
}

#endif //FRONTENDCOMMONS_FRONTENDSPOOL_H