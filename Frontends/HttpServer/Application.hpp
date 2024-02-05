#ifndef FRONTENDS_HTTPSERVER_APPLICATION
#define FRONTENDS_HTTPSERVER_APPLICATION

// UNIXCOMMONS
#include <eh/Exception.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Generics/CompositeMetricsProvider.hpp>
#include <Generics/Singleton.hpp>
#include <UServerUtils/Grpc/ComponentsBuilder.hpp>
#include <UServerUtils/Grpc/Manager.hpp>
#include <UServerUtils/MetricsHTTPProvider.hpp>

// THIS
#include <Frontends/Modules/BiddingFrontend/BiddingFrontendStat.hpp>
#include <Commons/ProcessControlVarsImpl.hpp>
#include <Frontends/FrontendCommons/FrontendInterface.hpp>
#include <xsd/Frontends/HttpServerConfig.hpp>

namespace AdServer::Frontends::Http
{

class Application final :
  public AdServer::Commons::ProcessControlVarsLoggerImpl,
  private Generics::CompositeActiveObject
{
public:
  using ALIVE_STATUS = CORBACommons::IProcessControl::ALIVE_STATUS;
  using Boolean = CORBA::Boolean;
  using CorbaConfig = CORBACommons::CorbaConfig;
  using CorbaServerAdapter_var = CORBACommons::CorbaServerAdapter_var;
  using CompositeMetricsProvider_var = Generics::CompositeMetricsProvider_var;
  using Frontend_var = FrontendCommons::Frontend_var;
  using StatHolder_var = AdServer::StatHolder_var;
  using ServerConfig = xsd::AdServer::Configuration::ServerConfigType;
  using ServerConfigPtr = std::unique_ptr<ServerConfig>;
  using ComponentsBuilder = UServerUtils::Grpc::ComponentsBuilder;
  using ManagerCoro = UServerUtils::Grpc::Manager;
  using ManagerCoro_var = UServerUtils::Grpc::Manager_var;
  using TaskProcessorContainer = UServerUtils::Grpc::TaskProcessorContainer;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  Application();

  int run(int argc, char** argv);

  void shutdown(Boolean wait_for_completion) override;

  ALIVE_STATUS is_alive() override;

private:
  ~Application() override = default;

  void read_config(
    const char* filename,
    const char* argv0);

  void init_corba();

  void init_http();

private:
  CorbaConfig corba_config_;

  ServerConfigPtr server_config_;

  CorbaServerAdapter_var corba_server_adapter_;

  StatHolder_var stats_;

  CompositeMetricsProvider_var composite_metrics_provider_;

  Frontend_var frontend_;
};

using Application_var = ReferenceCounting::SmartPtr<Application>;

} // namespace AdServer::Frontends::Http

#endif //FRONTENDS_HTTPSERVER_APPLICATION