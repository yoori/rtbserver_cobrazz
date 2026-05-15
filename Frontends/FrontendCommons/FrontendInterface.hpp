#pragma once

#include <String/SubString.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <Stream/MemoryStream.hpp>

#include <xsd/Frontends/FeConfig.hpp>
#include "HttpResponse.hpp"
#include "RequestTask.hpp"

namespace FrontendCommons
{
  /**
   * @class FrontendInterface
   *
   * @brief HTTP Frontend interface.
   */
  class FrontendInterface :
    public virtual ReferenceCounting::Interface
  {
  public:
    /**
     * @class Configuration
     *
     * @brief Frontend configuration.
     */
    class Configuration: public ReferenceCounting::AtomicImpl
    {
    public:
      DECLARE_EXCEPTION(InvalidConfiguration, eh::DescriptiveException);

      typedef xsd::AdServer::Configuration::FeConfigurationType FeConfig;

      /**
       * @brief Constructor.
       * @param config path
       */
      Configuration(const char* config_path);

      /**
       * @brief Read config.
       * Parse config file
       */
      void
      read() /*throw(InvalidConfiguration)*/;

      /**
       * @brief Get config file path.
       * @return path
       */
      const std::string&
      path() const;

      /**
       * @brief Get frontend config.
       * @return frontend config
       */
      const FeConfig&
      get() const /*throw(InvalidConfiguration)*/;

    protected:
      virtual
      ~Configuration() noexcept
      {}

    private:
      const std::string config_path_;
      std::unique_ptr<FeConfig> config_;
    };

    typedef ReferenceCounting::SmartPtr<Configuration> Configuration_var;

    /**
     * @brief Handle or not URI.
     * @param uri.
     */
    virtual bool
    will_handle(const String::SubString& uri) noexcept = 0;

    /**
     * @brief Handle HTTP request.
     * @param HTTP request
     * @param[out] HTTP response
     */
    virtual void
    handle_request(
      FCGI::HttpRequestHolder_var request,
      FCGI::BaseHttpResponseWriter_var response_writer)
      noexcept = 0;

    /**
     * @brief Handle HTTP request without params.
     * @param HTTP request
     * @param[out] HTTP response
     */
    virtual void
    handle_request_noparams(
      FCGI::HttpRequestHolder_var request_holder,
      FCGI::BaseHttpResponseWriter_var response_writer)
      /*throw(eh::Exception)*/;

    /**
     * @brief Initialize frontend.
     */
    virtual void
    init() /*throw(eh::Exception)*/ = 0;

    /**
     * @brief Shutdown frontend.
     */
    virtual void
    shutdown() noexcept = 0;

  protected:
    virtual
    ~FrontendInterface() noexcept = default;

    static bool
    parse_args_(
      FCGI::HttpRequestHolder_var request_holder)
      /*throw(eh::Exception)*/;
  };

  typedef ReferenceCounting::SmartPtr<FrontendInterface> Frontend_var;

  class CoroFrontendInterface :
    public virtual FrontendInterface
  {
  public:
    virtual RequestTask
    handle_request_coro(
      FCGI::HttpRequestHolder_var request,
      FCGI::BaseHttpResponseWriter_var response_writer)
      noexcept = 0;

    virtual RequestTask
    handle_request_noparams_coro(
      FCGI::HttpRequestHolder_var request_holder,
      FCGI::BaseHttpResponseWriter_var response_writer)
      noexcept;

    void
    handle_request(
      FCGI::HttpRequestHolder_var request,
      FCGI::BaseHttpResponseWriter_var response_writer)
      noexcept override;

    void
    handle_request_noparams(
      FCGI::HttpRequestHolder_var request_holder,
      FCGI::BaseHttpResponseWriter_var response_writer)
      noexcept override;

  protected:
    ~CoroFrontendInterface() noexcept override = default;
  };

  class NoCoroFrontendAdapter final :
    public virtual CoroFrontendInterface,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    explicit NoCoroFrontendAdapter(FrontendInterface* frontend);

    bool
    will_handle(const String::SubString& uri) noexcept override;

    RequestTask
    handle_request_coro(
      FCGI::HttpRequestHolder_var request,
      FCGI::BaseHttpResponseWriter_var response_writer)
      noexcept override;

    RequestTask
    handle_request_noparams_coro(
      FCGI::HttpRequestHolder_var request_holder,
      FCGI::BaseHttpResponseWriter_var response_writer)
      noexcept override;

    void
    init() override;

    void
    shutdown() noexcept override;

  private:
    ~NoCoroFrontendAdapter() noexcept override = default;

    Frontend_var frontend_;
  };

  typedef ReferenceCounting::SmartPtr<CoroFrontendInterface> CoroFrontend_var;
}

#include "FrontendInterface.ipp"
