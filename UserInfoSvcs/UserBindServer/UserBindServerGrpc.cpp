#include "UserBindServerGrpc.hpp"

#include <grpcpp/grpcpp.h>

#include <algorithm>
#include <thread>
#include <utility>
#include <vector>

#include <ReferenceCounting/ReferenceCounting.hpp>
#include <Commons/GrpcAlgs.hpp>
#include <Commons/Grpc/GrpcServer.hpp>

#include <Commons/Grpc/Batch.grpc.pb.h>
#include <UserInfoSvcs/UserBindServer/UserBindServerGrpc.grpc.pb.h>

#include "UserBindServerCore.hpp"

namespace AdServer::UserInfoSvcs
{
  namespace
  {
    constexpr const char user_bind_server_grpc_aspect[] = "UserBindServerGrpc";
  }

  class UserBindServerGrpc::ServiceImpl final:
    public AdServer::Grpc::GrpcAsyncServiceBase<
      UserBindServerGrpc::ServiceImpl,
      adserver::user_info_svcs::user_bind::UserBindServerGrpc,
      adserver::user_info_svcs::user_bind::UserBindServerGrpc::AsyncService>
  {
    using AsyncService =
      adserver::user_info_svcs::user_bind::UserBindServerGrpc::AsyncService;

  public:
    ServiceImpl(UserBindServerCore* core);

    static auto grpc_calls()
    {
      return std::make_tuple(
        MAKE_GRPC_CALL(
          adserver::user_info_svcs::user_bind::GetBindRequestRequest,
          adserver::user_info_svcs::user_bind::GetBindRequestResponse,
          get_bind_request),
        MAKE_GRPC_CALL(
          adserver::user_info_svcs::user_bind::AddBindRequestRequest,
          adserver::user_info_svcs::user_bind::AddBindRequestResponse,
          add_bind_request),
        MAKE_GRPC_CALL(
          adserver::user_info_svcs::user_bind::GetUserIdRequest,
          adserver::user_info_svcs::user_bind::GetUserIdResponse,
          get_user_id),
        MAKE_GRPC_CALL(
          adserver::user_info_svcs::user_bind::AddUserIdRequest,
          adserver::user_info_svcs::user_bind::AddUserIdResponse,
          add_user_id),
        MAKE_GRPC_CALL(
          adserver::user_info_svcs::user_bind::GetSourceRequest,
          adserver::user_info_svcs::user_bind::GetSourceResponse,
          get_source));
    }

    void get_bind_request(
      const adserver::user_info_svcs::user_bind::GetBindRequestRequest& request,
      adserver::user_info_svcs::user_bind::GetBindRequestResponse& response,
      ::grpc::Status& result_status) const;

    void add_bind_request(
      const adserver::user_info_svcs::user_bind::AddBindRequestRequest& request,
      adserver::user_info_svcs::user_bind::AddBindRequestResponse&,
      ::grpc::Status& result_status) const;

    void get_user_id(
      const adserver::user_info_svcs::user_bind::GetUserIdRequest& request,
      adserver::user_info_svcs::user_bind::GetUserIdResponse& response,
      ::grpc::Status& result_status) const;

    void add_user_id(
      const adserver::user_info_svcs::user_bind::AddUserIdRequest& request,
      adserver::user_info_svcs::user_bind::AddUserIdResponse& response,
      ::grpc::Status& result_status) const;

    void get_source(
      const adserver::user_info_svcs::user_bind::GetSourceRequest&,
      adserver::user_info_svcs::user_bind::GetSourceResponse& response,
      ::grpc::Status& result_status) const;

  private:
    const UserBindServerCore_var core_;
  };

  // UserBindServerGrpc::ServiceImpl
  UserBindServerGrpc::ServiceImpl::ServiceImpl(
    UserBindServerCore* core)
    : core_(ReferenceCounting::add_ref(core))
  {}

  void
  UserBindServerGrpc::ServiceImpl::get_bind_request(
    const adserver::user_info_svcs::user_bind::GetBindRequestRequest& request,
    adserver::user_info_svcs::user_bind::GetBindRequestResponse& response,
    ::grpc::Status& result_status) const
  {
    try
    {
      const auto result = core_->get_bind_request(
        request.request_id(),
        GrpcAlgs::unpack_time(request.timestamp()));

      for (const auto& bind_user_id : result.bind_user_ids)
      {
        response.add_bind_user_ids(bind_user_id);
      }

      result_status = ::grpc::Status::OK;
    }
    catch (const UserBindServerCore::NotReady& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::UNAVAILABLE,
        ex.what());
    }
    catch (const UserBindServerCore::ChunkNotFound& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::NOT_FOUND,
        ex.what());
    }
    catch (const UserBindServerCore::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }
  }

  void
  UserBindServerGrpc::ServiceImpl::add_bind_request(
    const adserver::user_info_svcs::user_bind::AddBindRequestRequest& request,
    adserver::user_info_svcs::user_bind::AddBindRequestResponse&,
    ::grpc::Status& result_status) const
  {
    try
    {
      UserBindServerCore::BindRequestInfo bind_request;
      bind_request.bind_user_ids.reserve(request.bind_user_ids_size());
      for (int i = 0; i < request.bind_user_ids_size(); ++i)
      {
        bind_request.bind_user_ids.push_back(request.bind_user_ids(i));
      }

      core_->add_bind_request(
        request.request_id(),
        bind_request,
        GrpcAlgs::unpack_time(request.timestamp()));

      result_status = ::grpc::Status::OK;
    }
    catch (const UserBindServerCore::NotReady& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::UNAVAILABLE,
        ex.what());
    }
    catch (const UserBindServerCore::ChunkNotFound& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::NOT_FOUND,
        ex.what());
    }
    catch (const UserBindServerCore::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }
  }

  void
  UserBindServerGrpc::ServiceImpl::get_user_id(
    const adserver::user_info_svcs::user_bind::GetUserIdRequest& request,
    adserver::user_info_svcs::user_bind::GetUserIdResponse& response,
    ::grpc::Status& result_status) const
  {
    try
    {
      UserBindServerCore::GetUserRequestInfo req_info;
      req_info.id = request.id();
      req_info.silent = request.silent();
      req_info.generate_user_id = request.generate_user_id();
      req_info.for_set_cookie = request.for_set_cookie();

      req_info.timestamp = GrpcAlgs::unpack_time(request.timestamp());
      req_info.create_timestamp = GrpcAlgs::unpack_time(request.create_timestamp());
      req_info.current_user_id = GrpcAlgs::unpack_user_id(request.current_user_id());

      const auto result = core_->get_user_id(req_info);
      response.set_user_id(GrpcAlgs::pack_user_id(result.user_id));
      response.set_min_age_reached(result.min_age_reached);
      response.set_created(result.created);
      response.set_invalid_operation(result.invalid_operation);
      response.set_user_found(result.user_found);

      result_status = ::grpc::Status::OK;
    }
    catch (const UserBindServerCore::NotReady& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::UNAVAILABLE,
        ex.what());
    }
    catch (const UserBindServerCore::ChunkNotFound& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::NOT_FOUND,
        ex.what());
    }
    catch (const UserBindServerCore::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }
  }

  void
  UserBindServerGrpc::ServiceImpl::add_user_id(
    const adserver::user_info_svcs::user_bind::AddUserIdRequest& request,
    adserver::user_info_svcs::user_bind::AddUserIdResponse& response,
    ::grpc::Status& result_status) const
  {
    try
    {
      UserBindServerCore::AddUserRequestInfo req_info;
      req_info.id = request.id();

      req_info.timestamp = GrpcAlgs::unpack_time(request.timestamp());
      req_info.user_id = GrpcAlgs::unpack_user_id(request.user_id());

      const auto result = core_->add_user_id(req_info);

      response.set_merge_user_id(GrpcAlgs::pack_user_id(result.merge_user_id));
      response.set_invalid_operation(result.invalid_operation);

      result_status = ::grpc::Status::OK;
    }
    catch (const UserBindServerCore::NotReady& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::UNAVAILABLE,
        ex.what());
    }
    catch (const UserBindServerCore::ChunkNotFound& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::NOT_FOUND,
        ex.what());
    }
    catch (const UserBindServerCore::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }
  }

  void
  UserBindServerGrpc::ServiceImpl::get_source(
    const adserver::user_info_svcs::user_bind::GetSourceRequest&,
    adserver::user_info_svcs::user_bind::GetSourceResponse& response,
    ::grpc::Status& result_status) const
  {
    try
    {
      const auto result = core_->get_source();
      for (const auto chunk_id : result.chunks)
      {
        response.add_chunks(chunk_id);
      }
      response.set_chunks_number(result.chunks_number);

      result_status = ::grpc::Status::OK;
    }
    catch (const UserBindServerCore::NotReady& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::UNAVAILABLE,
        ex.what());
    }
    catch (const UserBindServerCore::ChunkNotFound& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::NOT_FOUND,
        ex.what());
    }
    catch (const UserBindServerCore::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }
  }

  // UserBindServerGrpc impl
  UserBindServerGrpc::UserBindServerGrpc(
    UserBindServerCore* core,
    Logging::Logger* logger,
    std::string_view bind_address,
    unsigned int bind_port)
    : bind_address_(std::string(bind_address) + ":" + std::to_string(bind_port)),
      impl_(std::make_shared<Impl>(
        logger,
        user_bind_server_grpc_aspect,
        bind_address_,
        std::make_unique<ServiceImpl>(core)))
  {
    add_child_object(impl_);
  }

  UserBindServerGrpc::~UserBindServerGrpc() noexcept
  {}
}
