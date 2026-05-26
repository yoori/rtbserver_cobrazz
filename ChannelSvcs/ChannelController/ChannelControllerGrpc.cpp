#include "ChannelControllerGrpc.hpp"

#include <utility>

#include <grpcpp/grpcpp.h>

#include <Commons/Grpc/GrpcServer.hpp>

#include "ChannelControllerGrpc.grpc.pb.h"

namespace AdServer::ChannelSvcs
{
  namespace
  {
    constexpr const char channel_controller_grpc_aspect[] =
      "ChannelController";

    namespace pb = adserver::channel_svcs::channel_controller;
  }

  class ChannelControllerGrpc::ServiceImpl final:
    public AdServer::Grpc::GrpcAsyncServiceBase<
      ChannelControllerGrpc::ServiceImpl,
      adserver::channel_svcs::channel_controller::ChannelControllerGrpc,
      adserver::channel_svcs::channel_controller::ChannelControllerGrpc::AsyncService>
  {
    using AsyncService =
      adserver::channel_svcs::channel_controller::ChannelControllerGrpc::AsyncService;

  public:
    explicit ServiceImpl(ChannelControllerImpl* controller)
      : controller_(ReferenceCounting::add_ref(controller))
    {}

    static auto grpc_calls()
    {
      return std::make_tuple(
        MAKE_GRPC_CALL(
          pb::GetSessionDescriptionRequest,
          pb::GetSessionDescriptionResponse,
          get_session_description));
    }

    void get_session_description(
      const pb::GetSessionDescriptionRequest&,
      pb::GetSessionDescriptionResponse& response,
      ::grpc::Status& result_status) const
    {
      try
      {
        controller_->fill_session_description(response);
        result_status = ::grpc::Status::OK;
      }
      catch (const ChannelControllerImpl::NotReady& ex)
      {
        result_status =
          ::grpc::Status(::grpc::StatusCode::UNAVAILABLE, ex.what());
      }
      catch (const eh::Exception& ex)
      {
        result_status =
          ::grpc::Status(::grpc::StatusCode::INTERNAL, ex.what());
      }
    }

  private:
    ChannelControllerImpl_var controller_;
  };

  ChannelControllerGrpc::ChannelControllerGrpc(
    ChannelControllerImpl* controller,
    Logging::Logger* logger,
    std::string_view bind_address,
    unsigned int bind_port,
    std::size_t grpc_threads)
    : bind_address_(std::string(bind_address) + ":" + std::to_string(bind_port)),
      impl_(std::make_shared<Impl>(
        logger,
        channel_controller_grpc_aspect,
        bind_address_,
        grpc_threads,
        std::make_unique<ServiceImpl>(controller)))
  {
    add_child_object(impl_);
  }

  ChannelControllerGrpc::~ChannelControllerGrpc() noexcept = default;
}
