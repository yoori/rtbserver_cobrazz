#include "ImprTrackRequestState.hpp"

#include <Logger/Logger.hpp>
#include <Stream/MemoryStream.hpp>

#include <Commons/GrpcAlgs.hpp>
#include <Frontends/FrontendCommons/UserBindClientConfig.hpp>

#include "ImprTrackFrontend.hpp"

namespace
{
  const char FUN[] = "ImprTrackRequestState";
  const char IMPR_TRACK_FRONTEND_ASPECT[] = "ImprTrackFrontend";
}

namespace AdServer::ImprTrack
{
  ImprTrackRequestState::ImprTrackRequestState(
    Frontend* frontend,
    const RequestInfo& request_info,
    const AdServer::Commons::UserId& input_user_id,
    std::function<void(
      const AdServer::Commons::UserId& result_user_id,
      bool invalid_bind_operation)> finish)
    : frontend_(frontend),
      request_info_(request_info),
      result_user_id_(input_user_id),
      invalid_bind_operation_(false),
      finish_(std::move(finish))
  {}

  void
  ImprTrackRequestState::start()
  {
    if(request_info_.user_status == AdServer::CampaignSvcs::US_OPTOUT)
    {
      result_user_id_ = AdServer::Commons::UserId();
      complete_stage_();
      return;
    }

    if(!frontend_->user_bind_client_)
    {
      complete_stage_();
      return;
    }

    if(result_user_id_.is_null())
    {
      rebind_external_stage_();
      return;
    }

    resolve_current_user_stage_();
  }

  void
  ImprTrackRequestState::complete_stage_()
  {
    finish_(result_user_id_, invalid_bind_operation_);
  }

  void
  ImprTrackRequestState::rebind_external_stage_()
  {
    if(request_info_.current_user_id == result_user_id_ ||
      request_info_.external_user_id.empty())
    {
      complete_stage_();
      return;
    }

    if(!result_user_id_.is_null())
    {
      add_external_user_stage_();
    }
    else
    {
      resolve_external_user_stage_();
    }
  }

  void
  ImprTrackRequestState::add_external_user_stage_()
  {
    adserver::user_info_svcs::user_bind::AddUserIdRequest add_user_request;
    add_user_request.set_id(request_info_.external_user_id);
    add_user_request.set_user_id(GrpcAlgs::pack_user_id(result_user_id_));
    add_user_request.set_timestamp(GrpcAlgs::pack_time(request_info_.time));

    auto self = shared_from_this();
    frontend_->user_bind_client_->add_user_id(
      add_user_request,
      [self](
        const grpc::Status& status,
        const adserver::user_info_svcs::user_bind::AddUserIdResponse& response)
      {
        self->frontend_->workers_->post(
          [self, status, response]()
          {
            self->add_external_user_done_stage_(status, response);
          });
      });
  }

  void
  ImprTrackRequestState::add_external_user_done_stage_(
    const grpc::Status& status,
    const adserver::user_info_svcs::user_bind::AddUserIdResponse& response)
  {
    if(!status.ok())
    {
      log_rebind_error_(status);
      complete_stage_();
      return;
    }

    if(response.invalid_operation())
    {
      invalid_bind_operation_ = true;
      frontend_->report_bad_user_(request_info_);
    }

    complete_stage_();
  }

  void
  ImprTrackRequestState::resolve_external_user_stage_()
  {
    adserver::user_info_svcs::user_bind::GetUserIdRequest get_request;
    get_request.set_id(request_info_.external_user_id);
    get_request.set_timestamp(GrpcAlgs::pack_time(request_info_.time));
    get_request.set_silent(true);
    get_request.set_generate_user_id(false);
    get_request.set_for_set_cookie(request_info_.set_cookie);
    get_request.set_create_timestamp(
      GrpcAlgs::pack_time(Generics::Time::ZERO));

    auto self = shared_from_this();
    frontend_->user_bind_client_->get_user_id(
      get_request,
      [self](
        const grpc::Status& status,
        const adserver::user_info_svcs::user_bind::GetUserIdResponse& response)
      {
        self->frontend_->workers_->post(
          [self, status, response]()
          {
            self->resolve_external_user_done_stage_(status, response);
          });
      });
  }

  void
  ImprTrackRequestState::resolve_external_user_done_stage_(
    const grpc::Status& status,
    const adserver::user_info_svcs::user_bind::GetUserIdResponse& response)
  {
    if(!status.ok())
    {
      log_rebind_error_(status);
      complete_stage_();
      return;
    }

    if(response.invalid_operation())
    {
      invalid_bind_operation_ = true;
      frontend_->report_bad_user_(request_info_);
    }
    else
    {
      const AdServer::Commons::UserId resolved_user_id =
        GrpcAlgs::unpack_user_id(response.user_id());
      if(!resolved_user_id.is_null())
      {
        result_user_id_ = resolved_user_id;
        frontend_->common_module_->user_id_controller()->null_blacklisted(
          result_user_id_);
      }
    }

    complete_stage_();
  }

  void
  ImprTrackRequestState::resolve_current_user_stage_()
  {
    adserver::user_info_svcs::user_bind::GetUserIdRequest get_request;
    get_request.set_id(std::string("c/") + result_user_id_.to_string());
    get_request.set_timestamp(GrpcAlgs::pack_time(request_info_.time));
    get_request.set_silent(true);
    get_request.set_generate_user_id(false);
    get_request.set_for_set_cookie(request_info_.set_cookie);
    get_request.set_create_timestamp(
      GrpcAlgs::pack_time(Generics::Time::ZERO));
    get_request.set_current_user_id(GrpcAlgs::pack_user_id(result_user_id_));

    auto self = shared_from_this();
    frontend_->user_bind_client_->get_user_id(
      get_request,
      [self](
        const grpc::Status& status,
        const adserver::user_info_svcs::user_bind::GetUserIdResponse& response)
      {
        self->frontend_->workers_->post(
          [self, status, response]()
          {
            self->resolve_current_user_done_stage_(status, response);
          });
      });
  }

  void
  ImprTrackRequestState::resolve_current_user_done_stage_(
    const grpc::Status& status,
    const adserver::user_info_svcs::user_bind::GetUserIdResponse& response)
  {
    if(!status.ok())
    {
      complete_stage_();
      return;
    }

    if(response.invalid_operation())
    {
      invalid_bind_operation_ = true;
      frontend_->report_bad_user_(request_info_);
      complete_stage_();
      return;
    }

    const AdServer::Commons::UserId resolved_user_id =
      GrpcAlgs::unpack_user_id(response.user_id());
    if(!resolved_user_id.is_null())
    {
      result_user_id_ = resolved_user_id;
    }

    rebind_external_stage_();
  }

  void
  ImprTrackRequestState::log_rebind_error_(
    const grpc::Status& status) const noexcept
  {
    Stream::Error ostr;
    ostr << FUN << ": UserBindServer gRPC call failed: code=" <<
      static_cast<int>(status.error_code()) <<
      ", message=" << status.error_message();
    frontend_->logger()->log(
      ostr.str(),
      status.error_code() == grpc::StatusCode::UNAVAILABLE ?
        Logging::Logger::WARNING :
        Logging::Logger::ERROR,
      IMPR_TRACK_FRONTEND_ASPECT,
      status.error_code() == grpc::StatusCode::UNAVAILABLE ?
        "" : "ADS-IMPL-109");
  }
}
