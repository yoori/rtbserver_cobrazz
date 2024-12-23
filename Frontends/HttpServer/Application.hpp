#ifndef FRONTENDS_HTTPSERVER_APPLICATION
#define FRONTENDS_HTTPSERVER_APPLICATION

// UNIXCOMMONS
#include <eh/Exception.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Generics/Singleton.hpp>
#include <UServerUtils/ComponentsBuilder.hpp>
#include <UServerUtils/Manager.hpp>

// THIS
#include <Commons/ProcessControlVarsImpl.hpp>
#include <Frontends/FrontendCommons/FrontendInterface.hpp>
#include <Frontends/FrontendCommons/GrpcContainer.hpp>
#include <Frontends/Modules/BiddingFrontend/BiddingFrontendStat.hpp>
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

private:
  using CorbaConfig = CORBACommons::CorbaConfig;
  using CorbaServerAdapter_var = CORBACommons::CorbaServerAdapter_var;
  using StatHolder_var = AdServer::StatHolder_var;
  using ServerConfig = xsd::AdServer::Configuration::ServerConfigType;
  using ServerConfigPtr = std::unique_ptr<ServerConfig>;
  using ComponentsBuilder = UServerUtils::ComponentsBuilder;
  using ManagerCoro = UServerUtils::Manager;
  using ManagerCoro_var = UServerUtils::Manager_var;
  using TaskProcessorContainer = UServerUtils::TaskProcessorContainer;
  using GrpcChannelOperationPool = AdServer::ChannelSvcs::GrpcChannelOperationPool;
  using GrpcChannelOperationPoolPtr = std::shared_ptr<GrpcChannelOperationPool>;
  using GrpcCampaignManagerPool = FrontendCommons::GrpcCampaignManagerPool;
  using GrpcCampaignManagerPoolPtr = std::shared_ptr<GrpcCampaignManagerPool>;
  using SchedulerPtr = UServerUtils::Grpc::Common::SchedulerPtr;
  using TaskProcessor = userver::engine::TaskProcessor;
  using GrpcUserBindOperationDistributor = AdServer::UserInfoSvcs::GrpcUserBindOperationDistributor;
  using GrpcUserBindOperationDistributor_var = AdServer::UserInfoSvcs::GrpcUserBindOperationDistributor_var;
  using GrpcUserInfoOperationDistributor = AdServer::UserInfoSvcs::GrpcUserInfoOperationDistributor;
  using GrpcUserInfoOperationDistributor_var = AdServer::UserInfoSvcs::GrpcUserInfoOperationDistributor_var;

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

  GrpcChannelOperationPoolPtr create_grpc_channel_operation_pool(
    const SchedulerPtr& scheduler,
    TaskProcessor& task_processor);

  GrpcCampaignManagerPoolPtr create_grpc_campaign_manager_pool(
    const SchedulerPtr& scheduler,
    TaskProcessor& task_processor);

  GrpcUserBindOperationDistributor_var create_grpc_user_bind_operation_distributor(
    const SchedulerPtr& scheduler,
    TaskProcessor& task_processor);

  GrpcUserInfoOperationDistributor_var create_grpc_user_info_operation_distributor(
    const SchedulerPtr& scheduler,
    TaskProcessor& task_processor);

private:
  CorbaConfig corba_config_;

  ServerConfigPtr server_config_;

  CorbaServerAdapter_var corba_server_adapter_;

  StatHolder_var stats_;
};

using Application_var = ReferenceCounting::SmartPtr<Application>;

} // namespace AdServer::Frontends::Http

#endif //FRONTENDS_HTTPSERVER_APPLICATION