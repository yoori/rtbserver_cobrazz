#ifndef FRONTENDS_ECHOFRONTEND
#define FRONTENDS_ECHOFRONTEND

// UNIX_COMMONS
#include <Logger/Logger.hpp>
#include <Logger/ActiveObjectCallback.hpp>
#include <UServerUtils/Grpc/Common/Scheduler.hpp>

// USERVER
#include <userver/engine/task/task_processor.hpp>

// THIS
#include <Frontends/CommonModule/CommonModule.hpp>
#include <Frontends/FrontendCommons/FrontendTaskPool.hpp>
#include <Frontends/FrontendCommons/GrpcContainer.hpp>
#include <Frontends/FrontendCommons/HTTPExceptions.hpp>

namespace AdServer::Echo
{

class Frontend final :
  private FrontendCommons::HTTPExceptions,
  private Logging::LoggerCallbackHolder,
  public FrontendCommons::FrontendTaskPool,
  public ReferenceCounting::AtomicImpl
{
public:
  using HttpResponseFactory = FrontendCommons::HttpResponseFactory;
  using HttpResponseFactory_var = FrontendCommons::HttpResponseFactory_var;
  using GrpcContainerPtr = FrontendCommons::GrpcContainerPtr;
  using SchedulerPtr = UServerUtils::Grpc::Common::SchedulerPtr;
  using TaskProcessor = userver::engine::TaskProcessor;
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;

public:
  Frontend(
    const GrpcContainerPtr& grpc_container,
    TaskProcessor& task_processor,
    const SchedulerPtr& scheduler,
    Configuration* frontend_config,
    Logger* logger,
    CommonModule* common_module,
    HttpResponseFactory* response_factory);

  bool will_handle(const String::SubString& uri) noexcept override;

  void handle_request_(
    FrontendCommons::HttpRequestHolder_var request_holder,
    FrontendCommons::HttpResponseWriter_var response_writer) noexcept override;

private:
  void init() override;

  void shutdown() noexcept override;

private:
  const GrpcContainerPtr grpc_container_;

  TaskProcessor& task_processor_;

  const SchedulerPtr scheduler_;

  Logger_var logger_;
};

using Frontend_var = ReferenceCounting::SmartPtr<Frontend>;

} // namespace AdServer::Echo

#endif //FRONTENDS_ECHOFRONTEND
