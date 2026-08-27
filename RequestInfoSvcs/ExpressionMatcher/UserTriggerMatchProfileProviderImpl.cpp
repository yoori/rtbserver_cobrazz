#include <utility>

#include "UserTriggerMatchProfileProviderImpl.hpp"

namespace AdServer::RequestInfoSvcs
{
  UserTriggerMatchProfileProviderImpl::Client::Client(
    std::string endpoint_val,
    std::shared_ptr<ExpressionMatcherGrpcAsyncBatchingClient> async_client_val,
    std::shared_ptr<AdServer::Commons::ExecutorPool> processing_executor_pool)
    : endpoint(std::move(endpoint_val)),
      async_client(std::move(async_client_val)),
      coro_client(async_client, std::move(processing_executor_pool))
  {}

  UserTriggerMatchProfileProviderImpl::UserTriggerMatchProfileProviderImpl(
    const EndpointByHost& expression_matcher_endpoints,
    Commons::HostDistributionFile* host_distr,
    const char* self_host_name,
    UserTriggerMatchContainer* self_provider,
    unsigned long common_chunks_number,
    std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor,
    std::shared_ptr<AdServer::Commons::BoostAsioContextRunActiveObject>
      coalesce_runner,
    std::shared_ptr<AdServer::Commons::ExecutorPool> processing_executor_pool)
    noexcept
    : self_provider_(ReferenceCounting::add_ref(self_provider)),
      common_chunks_number_(common_chunks_number)
  {
    Commons::HostDistributionFile::IndexToHostMap chunk_id_to_host = host_distr->all_indexes();
    std::map<std::string, std::shared_ptr<Client>> clients_by_endpoint;

    for (Commons::HostDistributionFile::IndexToHostMap::const_iterator iter =
          chunk_id_to_host.begin();
        iter != chunk_id_to_host.end();
        ++iter)
    {
      if (iter->second == self_host_name)
      {
        own_chunks_.insert(iter->first);
        continue;
      }

      const auto endpoint_it = expression_matcher_endpoints.find(iter->second);
      if (endpoint_it != expression_matcher_endpoints.end())
      {
        auto [client_it, inserted] = clients_by_endpoint.try_emplace(endpoint_it->second);
        if (inserted)
        {
          AdServer::Grpc::BatchingOptions options;
          options.max_batch_delay = Generics::Time::ZERO;

          auto async_client =
            std::make_shared<ExpressionMatcherGrpcAsyncBatchingClient>(
              endpoint_it->second,
              grpc_executor,
              coalesce_runner,
              std::move(options));
          add_child_object(async_client);
          client_it->second = std::make_shared<Client>(
            endpoint_it->second,
            std::move(async_client),
            processing_executor_pool);
        }

        chunks_client_map_.emplace(iter->first, client_it->second);
      }
    }
  }

  AdServer::Commons::Awaitable<Generics::ConstSmartMemBuf_var>
  UserTriggerMatchProfileProviderImpl::co_get_user_profile(const AdServer::Commons::UserId& user_id)
  {
    const unsigned long chunk_index =
      AdServer::Commons::uuid_distribution_hash(user_id) % common_chunks_number_;

    if (own_chunks_.count(chunk_index) > 0)
    {
      co_return co_await self_provider_->co_get_user_profile(user_id);
    }

    const auto client_it = chunks_client_map_.find(chunk_index);
    if (client_it == chunks_client_map_.end())
    {
      Stream::Error ostr;
      ostr << "No ExpressionMatcher gRPC endpoint for chunk " << chunk_index;
      throw UserTriggerMatchProfileProvider::Exception(ostr);
    }

    namespace Proto = adserver::request_info_svcs::expression_matcher;
    Proto::UserTriggerMatchProfileRequest request;
    request.set_user_id(user_id.to_string());
    request.set_temporary_user(true);

    const auto result =
      co_await client_it->second->coro_client.co_get_user_trigger_match_profile(request);
    if (!result.status.ok())
    {
      Stream::Error ostr;
      ostr << "ExpressionMatcher gRPC get_user_trigger_match_profile failed: endpoint=" <<
        client_it->second->endpoint << ", code=" << result.status.error_code() <<
        ", message=" << result.status.error_message();
      throw UserTriggerMatchProfileProvider::Exception(ostr);
    }

    if (!result.response.found())
    {
      co_return Generics::ConstSmartMemBuf_var();
    }

    const std::string& profile = result.response.profile();
    co_return Generics::ConstSmartMemBuf_var(
      new Generics::ConstSmartMemBuf(profile.data(), profile.size()));
  }
}
