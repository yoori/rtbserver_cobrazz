#pragma once

#include <map>
#include <memory>
#include <string>

#include <eh/Exception.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/DefaultImpl.hpp>
#include <Logger/Logger.hpp>
#include <Generics/Time.hpp>
#include <Generics/MemBuf.hpp>
#include <Generics/CompositeActiveObject.hpp>

#include <Commons/UserInfoManip.hpp>
#include <Commons/HostDistribution.hpp>
#include <Commons/ExecutorPool.hpp>
#include <Commons/Grpc/GrpcExecutor.hpp>
#include <Commons/BoostAsioContextRunActiveObject.hpp>

#include "UserTriggerMatchContainer.hpp"
#include "ExpressionMatcherGrpc.grpc-client.hpp"

namespace AdServer::RequestInfoSvcs
{
  class UserTriggerMatchProfileProviderImpl:
    public UserTriggerMatchProfileProvider,
    public Generics::RefCountableCompositeActiveObject
  {
  public:
    using EndpointByHost = std::map<std::string, std::string>;

    UserTriggerMatchProfileProviderImpl(
      const EndpointByHost& expression_matcher_endpoints,
      Commons::HostDistributionFile* host_distr,
      const char* self_host_name,
      UserTriggerMatchContainer* self_provider,
      unsigned long common_chunks_number,
      std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor,
      std::shared_ptr<AdServer::Commons::BoostAsioContextRunActiveObject>
        coalesce_runner,
      std::shared_ptr<AdServer::Commons::ExecutorPool> processing_executor_pool)
      noexcept;

    AdServer::Commons::Awaitable<Generics::ConstSmartMemBuf_var>
    co_get_user_profile(const AdServer::Commons::UserId& user_id) override;

  protected:
    virtual ~UserTriggerMatchProfileProviderImpl() noexcept
    {}

  private:
    struct Client
    {
      std::string endpoint;
      std::shared_ptr<ExpressionMatcherGrpcAsyncBatchingClient> async_client;
      ExpressionMatcherGrpcCoroClient coro_client;

      Client(
        std::string endpoint,
        std::shared_ptr<ExpressionMatcherGrpcAsyncBatchingClient> async_client,
        std::shared_ptr<AdServer::Commons::ExecutorPool> processing_executor_pool);
    };

  private:
    std::map<unsigned long, std::shared_ptr<Client>> chunks_client_map_;
    std::set<unsigned long> own_chunks_;
    const UserTriggerMatchContainer_var self_provider_;
    unsigned long common_chunks_number_;
  };

  using UserTriggerMatchProfileProviderImpl_var =
    ReferenceCounting::SmartPtr<UserTriggerMatchProfileProviderImpl>;
}
