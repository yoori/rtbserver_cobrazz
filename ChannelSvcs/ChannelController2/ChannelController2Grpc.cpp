#include "ChannelController2Grpc.hpp"

#include <utility>

#include <grpcpp/grpcpp.h>

#include <Commons/Grpc/GrpcServer.hpp>

#include "ChannelControllerGrpc.grpc.pb.h"

namespace AdServer::ChannelSvcs
{
  namespace
  {
    constexpr const char channel_controller2_grpc_aspect[] =
      "ChannelController2";
  }

  class ChannelController2Grpc::ServiceImpl final:
    public AdServer::Grpc::GrpcServiceBase
  {
  public:
    explicit ServiceImpl(ChannelController2Impl* controller)
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
      public adserver::channel_svcs::channel_controller::
        ChannelControllerGrpc::Service
    {
    public:
      explicit Service(ChannelController2Impl* controller)
        : controller_(ReferenceCounting::add_ref(controller))
      {}

      grpc::Status get_session_description(
        grpc::ServerContext*,
        const adserver::channel_svcs::channel_controller::
          GetSessionDescriptionRequest*,
        adserver::channel_svcs::channel_controller::
          GetSessionDescriptionResponse* response) override
      {
        try
        {
          controller_->fill_session_description(*response);
          return grpc::Status::OK;
        }
        catch (const ChannelController2Impl::NotReady& ex)
        {
          return grpc::Status(grpc::StatusCode::UNAVAILABLE, ex.what());
        }
        catch (const eh::Exception& ex)
        {
          return grpc::Status(grpc::StatusCode::INTERNAL, ex.what());
        }
      }

    private:
      ChannelController2Impl_var controller_;
    };

  private:
    ChannelController2Impl_var controller_;
    Service service_;
  };

  ChannelController2Grpc::ChannelController2Grpc(
    ChannelController2Impl* controller,
    Logging::Logger* logger,
    std::string_view bind_address,
    unsigned int bind_port)
    : bind_address_(std::string(bind_address) + ":" + std::to_string(bind_port)),
      impl_(std::make_shared<Impl>(
        logger,
        channel_controller2_grpc_aspect,
        bind_address_,
        std::make_unique<ServiceImpl>(controller)))
  {
    add_child_object(impl_);
  }

  ChannelController2Grpc::~ChannelController2Grpc() noexcept = default;
}
