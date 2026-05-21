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
  }

  class UserBindControllerGrpc::ServiceImpl final:
    public AdServer::Grpc::GrpcServiceBase
  {
  public:
    explicit ServiceImpl(UserBindControllerImpl* controller)
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
      public adserver::user_info_svcs::user_bind_controller::
        UserBindControllerGrpc::Service
    {
    public:
      explicit Service(UserBindControllerImpl* controller)
        : controller_(ReferenceCounting::add_ref(controller))
      {}

      grpc::Status get_session_description(
        grpc::ServerContext*,
        const adserver::user_info_svcs::user_bind_controller::
          GetSessionDescriptionRequest*,
        adserver::user_info_svcs::user_bind_controller::
          GetSessionDescriptionResponse* response) override
      {
        try
        {
          controller_->fill_session_description(*response);
          return grpc::Status::OK;
        }
        catch (const UserBindControllerImpl::NotReady& ex)
        {
          return grpc::Status(grpc::StatusCode::UNAVAILABLE, ex.what());
        }
        catch (const eh::Exception& ex)
        {
          return grpc::Status(grpc::StatusCode::INTERNAL, ex.what());
        }
      }

    private:
      UserBindControllerImpl_var controller_;
    };

  private:
    UserBindControllerImpl_var controller_;
    Service service_;
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
