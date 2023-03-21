#pragma once

#include <vector>
#include <Logger/Logger.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <Generics/CompositeMetricsProvider.hpp>

#include <Frontends/FrontendCommons/FrontendInterface.hpp>
#include <BiddingFrontend/BiddingFrontendStat.hpp>
#include <Frontends/CommonModule/CommonModule.hpp>

namespace AdServer
{
  namespace Frontends
  {
    /**
     * @class FrontendsPool
     *
     * @brief HTTP frontends pool.
     */
    class FrontendsPool :
      public virtual FrontendCommons::FrontendInterface,
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

      typedef std::vector<ModuleId> ModuleIdArray;

    public:
      /**
       * @brief Constructor.
       * @param config path
       */
      FrontendsPool(
        const char* config_path,
        const ModuleIdArray& modules,
        Logging::Logger* logger,
        StatHolder* stats,
        Generics::CompositeMetricsProvider* composite_metrics_provider);

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
        FCGI::HttpRequestHolder_var request_holder,
        FCGI::HttpResponseWriter_var response_writer)
        noexcept;

      /**
       * @brief Handle HTTP request without params.
       * @param HTTP request
       * @param[out] HTTP response
       */
      virtual void
      handle_request_noparams(
        FCGI::HttpRequestHolder_var request_holder,
        FCGI::HttpResponseWriter_var response_writer)
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
      Configuration_var config_;
      ModuleIdArray modules_;
      Logging::Logger_var logger_;
      StatHolder_var stats_;
      Generics::CompositeMetricsProvider_var composite_metrics_provider_;

      CommonModule_var common_module_;
      std::vector<FrontendCommons::Frontend_var> frontends_;
    };
  }
}
