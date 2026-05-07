#include "UserInfoController2Grpc.hpp"

#include <utility>

#include <grpcpp/grpcpp.h>

#include <Commons/Grpc/GrpcServer.hpp>

#include "UserInfoControllerGrpc.grpc.pb.h"

namespace AdServer::UserInfoSvcs
{
  namespace
  {
    constexpr const char user_info_controller2_grpc_aspect[] =
      "UserInfoController2";
  }

  class UserInfoController2Grpc::ServiceImpl final:
    public AdServer::Grpc::GrpcServiceBase
  {
  public:
    explicit ServiceImpl(UserInfoController2Impl* controller)
      : controller_(ReferenceCounting::add_ref(controller)),
        service_(controller_.in())
    {
      add_grpc_service(&service_);
    }

    std::size_t completion_queues_count() const noexcept override
    {
      return 0;
    }

  private:
    void register_in_queue(grpc::ServerCompletionQueue*) override
    {}

    class Service final:
      public adserver::user_info_svcs::user_info_controller::
        UserInfoControllerGrpc::Service
    {
    public:
      explicit Service(UserInfoController2Impl* controller)
        : controller_(ReferenceCounting::add_ref(controller))
      {}

      grpc::Status get_session_description(
        grpc::ServerContext*,
        const adserver::user_info_svcs::user_info_controller::
          GetSessionDescriptionRequest*,
        adserver::user_info_svcs::user_info_controller::
          GetSessionDescriptionResponse* response) override
      {
        try
        {
          controller_->fill_session_description(*response);
          return grpc::Status::OK;
        }
        catch (const UserInfoController2Impl::NotReady& ex)
        {
          return grpc::Status(grpc::StatusCode::UNAVAILABLE, ex.what());
        }
        catch (const eh::Exception& ex)
        {
          return grpc::Status(grpc::StatusCode::INTERNAL, ex.what());
        }
      }

    private:
      UserInfoController2Impl_var controller_;
    };

  private:
    UserInfoController2Impl_var controller_;
    Service service_;
  };

  UserInfoController2Grpc::UserInfoController2Grpc(
    UserInfoController2Impl* controller,
    Logging::Logger* logger,
    std::string_view bind_address,
    unsigned int bind_port)
    : bind_address_(std::string(bind_address) + ":" + std::to_string(bind_port)),
      impl_(std::make_shared<Impl>(
        logger,
        user_info_controller2_grpc_aspect,
        bind_address_,
        std::make_unique<ServiceImpl>(controller)))
  {
    add_child_object(impl_);
  }

  UserInfoController2Grpc::~UserInfoController2Grpc() noexcept = default;
}
