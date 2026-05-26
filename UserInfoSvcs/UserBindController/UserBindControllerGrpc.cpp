#include "UserBindControllerGrpc.hpp"

#include <utility>

#include <grpcpp/grpcpp.h>

#include <Commons/Grpc/GrpcServer.hpp>

#include "UserBindControllerGrpc.grpc.pb.h"

namespace AdServer::UserInfoSvcs
{
  namespace
  {
    constexpr const char user_bind_controller_grpc_aspect[] =
      "UserBindController";

    namespace pb = adserver::user_info_svcs::user_bind_controller;
  }

  class UserBindControllerGrpc::ServiceImpl final:
    public AdServer::Grpc::GrpcAsyncServiceBase<
      UserBindControllerGrpc::ServiceImpl,
      adserver::user_info_svcs::user_bind_controller::UserBindControllerGrpc,
      adserver::user_info_svcs::user_bind_controller::UserBindControllerGrpc::AsyncService>
  {
    using AsyncService =
      adserver::user_info_svcs::user_bind_controller::UserBindControllerGrpc::AsyncService;

  public:
    explicit ServiceImpl(UserBindControllerImpl* controller)
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
      catch (const UserBindControllerImpl::NotReady& ex)
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
    UserBindControllerImpl_var controller_;
  };

  UserBindControllerGrpc::UserBindControllerGrpc(
    UserBindControllerImpl* controller,
    Logging::Logger* logger,
    std::string_view bind_address,
    unsigned int bind_port)
    : bind_address_(std::string(bind_address) + ":" + std::to_string(bind_port)),
      impl_(std::make_shared<Impl>(
        logger,
        user_bind_controller_grpc_aspect,
        bind_address_,
        std::make_unique<ServiceImpl>(controller)))
  {
    add_child_object(impl_);
  }

  UserBindControllerGrpc::~UserBindControllerGrpc() noexcept = default;
}
