#include "UrlInfoStorageGrpc.hpp"
#include "UrlInfoStorage.grpc.pb.h"

namespace AdServer::RequestInfoSvcs
{
  namespace Proto = adserver::request_info_svcs::url_info_storage;

  class UrlInfoStorageGrpc::ServiceImpl final:
    public Grpc::GrpcAsyncServiceBase<
      ServiceImpl, Proto::UrlInfoStorage, Proto::UrlInfoStorage::AsyncService>
  {
    using AsyncService = Proto::UrlInfoStorage::AsyncService;

  public:
    explicit ServiceImpl(UrlInfoStorage* storage);
    static auto grpc_calls();

    Commons::StartableAwaitable<void> co_get_url_profile(
      Proto::GetUrlProfileRequest&& request, Proto::GetUrlProfileResponse& response,
      grpc::Status& status) const;

    Commons::StartableAwaitable<void> co_set_url_profile(
      Proto::SetUrlProfileRequest&& request, Proto::SetUrlProfileResponse& response,
      grpc::Status& status) const;

  private:
    UrlInfoStorage_var storage_;
  };

  auto UrlInfoStorageGrpc::ServiceImpl::grpc_calls()
  {
    return std::make_tuple(
      MAKE_GRPC_CORO_CALL(
        Proto::GetUrlProfileRequest, Proto::GetUrlProfileResponse,
        get_url_profile, co_get_url_profile),
      MAKE_GRPC_CORO_CALL(
        Proto::SetUrlProfileRequest, Proto::SetUrlProfileResponse,
        set_url_profile, co_set_url_profile));
  }

  UrlInfoStorageGrpc::ServiceImpl::ServiceImpl(UrlInfoStorage* storage)
    : storage_(ReferenceCounting::add_ref(storage))
  {}

  Commons::StartableAwaitable<void> UrlInfoStorageGrpc::ServiceImpl::co_get_url_profile(
    Proto::GetUrlProfileRequest&& request, Proto::GetUrlProfileResponse& response,
    grpc::Status& status) const
  {
    try
    {
      response.set_text(co_await storage_->get_url_profile(request.url()));
      status = grpc::Status::OK;
    }
    catch (const std::invalid_argument& error)
    {
      status = grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, error.what());
    }
    catch (const std::exception& error)
    {
      status = grpc::Status(grpc::StatusCode::INTERNAL, error.what());
    }
  }

  Commons::StartableAwaitable<void> UrlInfoStorageGrpc::ServiceImpl::co_set_url_profile(
    Proto::SetUrlProfileRequest&& request, Proto::SetUrlProfileResponse&, grpc::Status& status)
    const
  {
    try
    {
      co_await storage_->set_url_profile(request.url(), request.text());
      status = grpc::Status::OK;
    }
    catch (const std::invalid_argument& error)
    {
      status = grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, error.what());
    }
    catch (const std::exception& error)
    {
      status = grpc::Status(grpc::StatusCode::INTERNAL, error.what());
    }
  }

  UrlInfoStorageGrpc::UrlInfoStorageGrpc(
    UrlInfoStorage* storage, Logging::Logger* logger, const std::string& address,
    std::size_t cq_threads, std::size_t threads_per_cq)
    : impl_(std::make_shared<Grpc::GrpcServer<ServiceImpl>>(
        logger,
        "UrlInfoStorage",
        address,
        cq_threads,
        threads_per_cq,
        std::make_unique<ServiceImpl>(storage)))
  {
    add_child_object(impl_);
  }

  UrlInfoStorageGrpc::~UrlInfoStorageGrpc() noexcept = default;
}
