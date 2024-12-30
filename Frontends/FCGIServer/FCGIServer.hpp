#pragma once

// UNIXCOMMONS
#include <eh/Exception.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Generics/Singleton.hpp>

// THIS
#include <BiddingFrontend/BiddingFrontendStat.hpp>
#include <Commons/ProcessControlVarsImpl.hpp>
#include <FrontendCommons/FrontendInterface.hpp>
#include <xsd/Frontends/FCGIServerConfig.hpp>

namespace AdServer::Frontends
{
  class FCGIServer final:
    public AdServer::Commons::ProcessControlVarsLoggerImpl,
    private Generics::CompositeActiveObject
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  public:
    FCGIServer();

    void main(int& argc, char** argv) noexcept;

    void shutdown(CORBA::Boolean wait_for_completion) override;

    CORBACommons::IProcessControl::ALIVE_STATUS
    is_alive() override;

  private:
    using FCGIServerConfig = xsd::AdServer::Configuration::FCGIServerConfigType;
    using FCGIServerConfigPtr = std::unique_ptr<FCGIServerConfig>;

  private:
    ~FCGIServer() override = default;

    void read_config_(
      const char* filename,
      const char* argv0);

    void init_corba_();

    void init_fcgi_();

  private:
    CORBACommons::CorbaConfig corba_config_;
    FCGIServerConfigPtr config_;
    CORBACommons::CorbaServerAdapter_var corba_server_adapter_;
    StatHolder_var stats_;
  };

  using FCGIServer_var = ReferenceCounting::QualPtr<FCGIServer>;
  using FCGIServerApp = Generics::Singleton<FCGIServer, FCGIServer_var>;
} // namespace AdServer::Frontends
