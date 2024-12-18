#ifndef FRONTENDCOMMONS_FRONTENDINTERFACE_HPP_
#define FRONTENDCOMMONS_FRONTENDINTERFACE_HPP_

// UNIXCOMMONS
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <Stream/MemoryStream.hpp>
#include <String/SubString.hpp>

// THIS
#include <xsd/Frontends/FeConfig.hpp>
#include "FCGI.hpp"

namespace FrontendCommons
{
  class FrontendInterface : public virtual ReferenceCounting::Interface
  {
  public:
    class Configuration final: public ReferenceCounting::AtomicImpl
    {
    public:
      using FeConfig = xsd::AdServer::Configuration::FeConfigurationType;

      DECLARE_EXCEPTION(InvalidConfiguration, eh::DescriptiveException);

    public:
      Configuration(const char* config_path);

      void read();

      const std::string& path() const;

      const FeConfig& get() const;

    protected:
      ~Configuration() override = default;

    private:
      const std::string config_path_;

      std::unique_ptr<FeConfig> config_;
    };

    using Configuration_var = ReferenceCounting::SmartPtr<Configuration>;

    FrontendInterface(
      FrontendCommons::HttpResponseFactory* response_factory);

    virtual bool will_handle(
      const String::SubString& uri) noexcept = 0;

    virtual void handle_request(
      FrontendCommons::HttpRequestHolder_var request,
      FrontendCommons::HttpResponseWriter_var response_writer) noexcept = 0;

    virtual void handle_request_noparams(
      FrontendCommons::HttpRequestHolder_var request_holder,
      FrontendCommons::HttpResponseWriter_var response_writer);

    FrontendCommons::HttpResponse_var create_response();

    virtual void init() = 0;

    virtual void shutdown() noexcept = 0;

  protected:
    virtual ~FrontendInterface() noexcept = default;

    bool parse_args_(
      FrontendCommons::HttpRequestHolder_var request_holder,
      FrontendCommons::HttpResponseWriter_var response_writer);

  private:
    FrontendCommons::HttpResponseFactory_var response_factory_;
  };

  using Frontend_var = ReferenceCounting::SmartPtr<FrontendInterface>;
} // namespace FrontendCommons

#include "FrontendInterface.ipp"

#endif /*FRONTENDCOMMONS_FRONTENDINTERFACE_HPP_*/
