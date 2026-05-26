#include "UserInfoControllerGrpc.hpp"

#include <utility>

#include <grpcpp/grpcpp.h>

#include <Commons/Grpc/GrpcServer.hpp>

#include "UserInfoControllerGrpc.grpc.pb.h"

namespace AdServer::UserInfoSvcs
{
  namespace
  {
    constexpr const char user_info_controller2_grpc_aspect[] =
      "UserInfoController";

    namespace pb = adserver::user_info_svcs::user_info_controller;
  }

  class UserInfoControllerGrpc::ServiceImpl final:
    public AdServer::Grpc::GrpcAsyncServiceBase<
      UserInfoControllerGrpc::ServiceImpl,
      adserver::user_info_svcs::user_info_controller::UserInfoControllerGrpc,
      adserver::user_info_svcs::user_info_controller::UserInfoControllerGrpc::AsyncService>
  {
    using AsyncService =
      adserver::user_info_svcs::user_info_controller::UserInfoControllerGrpc::AsyncService;

  public:
    explicit ServiceImpl(UserInfoControllerImpl* controller)
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
      catch (const UserInfoControllerImpl::NotReady& ex)
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
    UserInfoControllerImpl_var controller_;
  };

  UserInfoControllerGrpc::UserInfoControllerGrpc(
    UserInfoControllerImpl* controller,
    Logging::Logger* logger,
    std::string_view bind_address,
    unsigned int bind_port,
    std::size_t grpc_threads)
    : bind_address_(std::string(bind_address) + ":" + std::to_string(bind_port)),
      impl_(std::make_shared<Impl>(
        logger,
        user_info_controller2_grpc_aspect,
        bind_address_,
        grpc_threads,
        std::make_unique<ServiceImpl>(controller)))
  {
    add_child_object(impl_);
  }

  UserInfoControllerGrpc::~UserInfoControllerGrpc() noexcept = default;
}
