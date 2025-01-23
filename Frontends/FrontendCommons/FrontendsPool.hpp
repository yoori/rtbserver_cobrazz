#ifndef FRONTENDCOMMONS_FRONTENDSPOOL_H
#define FRONTENDCOMMONS_FRONTENDSPOOL_H

// STD
#include <vector>

// UNIXCOMMONS
#include <Logger/Logger.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <UServerUtils/Component.hpp>
#include <UServerUtils/Grpc/Common/Scheduler.hpp>

// USERVER
#include <engine/task/task_processor.hpp>

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
      public FrontendCommons::FrontendInterface,
      public UServerUtils::Component,
      public ReferenceCounting::AtomicImpl
    {
    public:
      using TaskProcessor = userver::engine::TaskProcessor;
      using GrpcContainerPtr = FrontendCommons::GrpcContainerPtr;

      enum class ServerType
      {
        HTTP,
        FCGI
      };

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
        TaskProcessor& helper_task_processor,
        const ServerType server_type,
        const GrpcContainerPtr& grpc_container,
        const char* config_path,
        const ModuleIdArray& modules,
        Logging::Logger* logger,
        StatHolder* stats,
        FrontendCommons::HttpResponseFactory* response_factory);

      /**
       * @brief Handle or not URI.
       * @param uri.
       */
      bool will_handle(const String::SubString& uri) noexcept override;

      /**
       * @brief Handle HTTP request.
       * @param HTTP request
       * @param[out] HTTP response
       */
      void handle_request(
        FrontendCommons::HttpRequestHolder_var request_holder,
        FrontendCommons::HttpResponseWriter_var response_writer)
        noexcept override;

      /**
       * @brief Handle HTTP request without params.
       * @param HTTP request
       * @param[out] HTTP response
       * throw(eh::Exception)
       */
      void handle_request_noparams(
        FrontendCommons::HttpRequestHolder_var request_holder,
        FrontendCommons::HttpResponseWriter_var response_writer) override;

    protected:
      ~FrontendsPool() noexcept override = default;

      void activate_object_() override;

      void deactivate_object_() override;

    private:
      /**
       * @brief Initialize frontend.
       * throw(eh::Exception)
       */
      void init() override;

      /**
       * @brief Shutdown frontend.
       */
      void shutdown() noexcept override;

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
      TaskProcessor& helper_task_processor_;

      const ServerType server_type_;

      const GrpcContainerPtr grpc_container_;

      Configuration_var config_;

      ModuleIdArray modules_;

      Logging::Logger_var logger_;

      StatHolder_var stats_;

      FrontendCommons::HttpResponseFactory_var http_response_factory_;

      CommonModule_var common_module_;

      std::vector<FrontendCommons::Frontend_var> frontends_;
    };

    using FrontendsPool_var = ReferenceCounting::SmartPtr<FrontendsPool>;
  }
}

#endif //FRONTENDCOMMONS_FRONTENDSPOOL_H