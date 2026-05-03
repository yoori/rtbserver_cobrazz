#include <Commons/CorbaAlgs.hpp>
#include <Commons/CorbaConfig.hpp>
#include <Commons/GrpcAlgs.hpp>
#include <UserInfoSvcs/UserBindController/UserBindOperationDistributor.hpp>

#include "UserBindCorbaClient.hpp"

namespace FrontendCommons
{
  namespace
  {
    grpc::Status status_from_exception_(
      const AdServer::UserInfoSvcs::UserBindMapper::NotReady& ex)
    {
      return grpc::Status(grpc::StatusCode::UNAVAILABLE, ex.description.in());
    }

    grpc::Status status_from_exception_(
      const AdServer::UserInfoSvcs::UserBindMapper::ChunkNotFound& ex)
    {
      return grpc::Status(grpc::StatusCode::NOT_FOUND, ex.description.in());
    }

    grpc::Status status_from_exception_(
      const AdServer::UserInfoSvcs::UserBindMapper::ImplementationException& ex)
    {
      return grpc::Status(grpc::StatusCode::INTERNAL, ex.description.in());
    }

    grpc::Status status_from_exception_(const CORBA::SystemException& ex)
    {
      Stream::Error ostr;
      ostr << ex;
      return grpc::Status(grpc::StatusCode::UNAVAILABLE, ostr.str().str());
    }

    grpc::Status status_from_exception_(const eh::Exception& ex)
    {
      return grpc::Status(grpc::StatusCode::INTERNAL, ex.what());
    }

    CORBACommons::TimestampInfo unpack_timestamp_(const std::string& timestamp)
    {
      return CorbaAlgs::pack_time(GrpcAlgs::unpack_time(timestamp));
    }

    void fill_timestamp_(
      CORBACommons::TimestampInfo& target,
      const std::string& timestamp)
    {
      target.length(Generics::Time::TIME_PACK_LEN);
      GrpcAlgs::unpack_time(timestamp).pack(target.get_buffer());
    }

    void fill_user_id_(
      CORBACommons::UserIdInfo& target,
      const std::string& user_id)
    {
      const auto unpacked_user_id = GrpcAlgs::unpack_user_id(user_id);
      target.length(unpacked_user_id.size());
      std::copy(
        unpacked_user_id.begin(),
        unpacked_user_id.end(),
        target.get_buffer());
    }
  }

  UserBindCorbaClient::UserBindCorbaClient(
    const UserBindControllerGroupSeq& user_bind_controller_group,
    const CORBACommons::CorbaClientAdapter* corba_client_adapter,
    Logging::Logger* logger)
    noexcept
  {
    AdServer::UserInfoSvcs::UserBindOperationDistributor::
      ControllerRefList controller_groups;

    for(UserBindControllerGroupSeq::const_iterator cg_it =
        user_bind_controller_group.begin();
        cg_it != user_bind_controller_group.end();
        ++cg_it)
    {
      AdServer::UserInfoSvcs::UserBindOperationDistributor::
        ControllerRef controller_ref_group;

      Config::CorbaConfigReader::read_multi_corba_ref(
        *cg_it,
        controller_ref_group);

      controller_groups.push_back(controller_ref_group);
    }

    AdServer::UserInfoSvcs::UserBindOperationDistributor_var distributor =
      new AdServer::UserInfoSvcs::UserBindOperationDistributor(
        logger,
        controller_groups,
        corba_client_adapter);
    user_bind_mapper_ = ReferenceCounting::add_ref(distributor);
    add_child_object(distributor);
  }

  void
  UserBindCorbaClient::get_bind_request(
    const adserver::user_info_svcs::user_bind::GetBindRequestRequest& request,
    GetBindRequestCallback callback)
  {
    adserver::user_info_svcs::user_bind::GetBindRequestResponse response;
    try
    {
      AdServer::UserInfoSvcs::UserBindMapper::BindRequestInfo_var result =
        user_bind_mapper_->get_bind_request(
          request.request_id().c_str(),
          unpack_timestamp_(request.timestamp()));

      for (CORBA::ULong i = 0; i < result->bind_user_ids.length(); ++i)
      {
        response.add_bind_user_ids(result->bind_user_ids[i].in());
      }
      callback(grpc::Status::OK, response);
    }
    catch (const AdServer::UserInfoSvcs::UserBindMapper::NotReady& ex)
    {
      callback(status_from_exception_(ex), response);
    }
    catch (const AdServer::UserInfoSvcs::UserBindMapper::ChunkNotFound& ex)
    {
      callback(status_from_exception_(ex), response);
    }
    catch (const AdServer::UserInfoSvcs::UserBindMapper::ImplementationException& ex)
    {
      callback(status_from_exception_(ex), response);
    }
    catch (const CORBA::SystemException& ex)
    {
      callback(status_from_exception_(ex), response);
    }
    catch (const eh::Exception& ex)
    {
      callback(status_from_exception_(ex), response);
    }
  }

  void
  UserBindCorbaClient::add_bind_request(
    const adserver::user_info_svcs::user_bind::AddBindRequestRequest& request,
    AddBindRequestCallback callback)
  {
    adserver::user_info_svcs::user_bind::AddBindRequestResponse response;
    try
    {
      AdServer::UserInfoSvcs::UserBindServer::BindRequestInfo bind_request;
      bind_request.bind_user_ids.length(request.bind_user_ids_size());
      for (int i = 0; i < request.bind_user_ids_size(); ++i)
      {
        bind_request.bind_user_ids[i] = request.bind_user_ids(i).c_str();
      }

      user_bind_mapper_->add_bind_request(
        request.request_id().c_str(),
        bind_request,
        unpack_timestamp_(request.timestamp()));
      callback(grpc::Status::OK, response);
    }
    catch (const AdServer::UserInfoSvcs::UserBindMapper::NotReady& ex)
    {
      callback(status_from_exception_(ex), response);
    }
    catch (const AdServer::UserInfoSvcs::UserBindMapper::ChunkNotFound& ex)
    {
      callback(status_from_exception_(ex), response);
    }
    catch (const AdServer::UserInfoSvcs::UserBindMapper::ImplementationException& ex)
    {
      callback(status_from_exception_(ex), response);
    }
    catch (const CORBA::SystemException& ex)
    {
      callback(status_from_exception_(ex), response);
    }
    catch (const eh::Exception& ex)
    {
      callback(status_from_exception_(ex), response);
    }
  }

  void
  UserBindCorbaClient::get_user_id(
    const adserver::user_info_svcs::user_bind::GetUserIdRequest& request,
    GetUserIdCallback callback)
  {
    adserver::user_info_svcs::user_bind::GetUserIdResponse response;
    try
    {
      AdServer::UserInfoSvcs::UserBindMapper::GetUserRequestInfo request_info;
      request_info.id = request.id().c_str();
      fill_timestamp_(request_info.timestamp, request.timestamp());
      request_info.silent = request.silent();
      request_info.generate_user_id = request.generate_user_id();
      request_info.for_set_cookie = request.for_set_cookie();
      fill_timestamp_(request_info.create_timestamp, request.create_timestamp());
      fill_user_id_(request_info.current_user_id, request.current_user_id());

      AdServer::UserInfoSvcs::UserBindMapper::GetUserResponseInfo_var result =
        user_bind_mapper_->get_user_id(request_info);
      response.set_user_id(GrpcAlgs::pack_user_id(
        CorbaAlgs::unpack_user_id(result->user_id)));
      response.set_min_age_reached(result->min_age_reached);
      response.set_created(result->created);
      response.set_invalid_operation(result->invalid_operation);
      response.set_user_found(result->user_found);
      callback(grpc::Status::OK, response);
    }
    catch (const AdServer::UserInfoSvcs::UserBindMapper::NotReady& ex)
    {
      callback(status_from_exception_(ex), response);
    }
    catch (const AdServer::UserInfoSvcs::UserBindMapper::ChunkNotFound& ex)
    {
      callback(status_from_exception_(ex), response);
    }
    catch (const AdServer::UserInfoSvcs::UserBindMapper::ImplementationException& ex)
    {
      callback(status_from_exception_(ex), response);
    }
    catch (const CORBA::SystemException& ex)
    {
      callback(status_from_exception_(ex), response);
    }
    catch (const eh::Exception& ex)
    {
      callback(status_from_exception_(ex), response);
    }
  }

  void
  UserBindCorbaClient::add_user_id(
    const adserver::user_info_svcs::user_bind::AddUserIdRequest& request,
    AddUserIdCallback callback)
  {
    adserver::user_info_svcs::user_bind::AddUserIdResponse response;
    try
    {
      AdServer::UserInfoSvcs::UserBindMapper::AddUserRequestInfo request_info;
      request_info.id = request.id().c_str();
      fill_timestamp_(request_info.timestamp, request.timestamp());
      fill_user_id_(request_info.user_id, request.user_id());

      AdServer::UserInfoSvcs::UserBindMapper::AddUserResponseInfo_var result =
        user_bind_mapper_->add_user_id(request_info);
      response.set_merge_user_id(GrpcAlgs::pack_user_id(
        CorbaAlgs::unpack_user_id(result->merge_user_id)));
      response.set_invalid_operation(result->invalid_operation);
      callback(grpc::Status::OK, response);
    }
    catch (const AdServer::UserInfoSvcs::UserBindMapper::NotReady& ex)
    {
      callback(status_from_exception_(ex), response);
    }
    catch (const AdServer::UserInfoSvcs::UserBindMapper::ChunkNotFound& ex)
    {
      callback(status_from_exception_(ex), response);
    }
    catch (const AdServer::UserInfoSvcs::UserBindMapper::ImplementationException& ex)
    {
      callback(status_from_exception_(ex), response);
    }
    catch (const CORBA::SystemException& ex)
    {
      callback(status_from_exception_(ex), response);
    }
    catch (const eh::Exception& ex)
    {
      callback(status_from_exception_(ex), response);
    }
  }

  void
  UserBindCorbaClient::get_source(
    const adserver::user_info_svcs::user_bind::GetSourceRequest&,
    GetSourceCallback callback)
  {
    adserver::user_info_svcs::user_bind::GetSourceResponse response;
    callback(
      grpc::Status(
        grpc::StatusCode::UNIMPLEMENTED,
        "get_source is not available through UserBindCorbaClient"),
      response);
  }

  AdServer::UserInfoSvcs::UserBindMapper::BindRequestInfo*
  UserBindCorbaClient::get_bind_request(
    const char* id,
    const CORBACommons::TimestampInfo& timestamp)
  {
    return user_bind_mapper_->get_bind_request(id, timestamp);
  }

  void
  UserBindCorbaClient::add_bind_request(
    const char* id,
    const AdServer::UserInfoSvcs::UserBindServer::BindRequestInfo& bind_request,
    const CORBACommons::TimestampInfo& timestamp)
  {
    user_bind_mapper_->add_bind_request(id, bind_request, timestamp);
  }

  AdServer::UserInfoSvcs::UserBindMapper::GetUserResponseInfo*
  UserBindCorbaClient::get_user_id(
    const AdServer::UserInfoSvcs::UserBindMapper::GetUserRequestInfo&
      request_info)
  {
    return user_bind_mapper_->get_user_id(request_info);
  }

  AdServer::UserInfoSvcs::UserBindMapper::AddUserResponseInfo*
  UserBindCorbaClient::add_user_id(
    const AdServer::UserInfoSvcs::UserBindMapper::AddUserRequestInfo&
      request_info)
  {
    return user_bind_mapper_->add_user_id(request_info);
  }
}
