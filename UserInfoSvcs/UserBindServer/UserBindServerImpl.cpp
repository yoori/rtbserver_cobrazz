#include <list>
#include <vector>
#include <iterator>

#include <PrivacyFilter/Filter.hpp>

#include <Commons/CorbaAlgs.hpp>
#include <Commons/FreqCapManip.hpp>

#include <UserInfoSvcs/UserInfoCommons/Allocator.hpp>

#include "UserBindServerImpl.hpp"

namespace AdServer::UserInfoSvcs
{
  // UserBindServerImpl
  UserBindServerImpl::UserBindServerImpl(
    Generics::ActiveObjectCallback* callback,
    Logging::Logger* logger,
    UserBindServerCore* core)
    /*throw(Exception)*/
    : callback_(ReferenceCounting::add_ref(callback)),
      logger_(ReferenceCounting::add_ref(logger)),
      core_(ReferenceCounting::add_ref(core))
  {}

  UserBindServerImpl::~UserBindServerImpl()
  {}

  AdServer::UserInfoSvcs::UserBindMapper::GetUserResponseInfo*
  UserBindServerImpl::get_user_id(
    const AdServer::UserInfoSvcs::UserBindMapper::GetUserRequestInfo&
      request_info)
    /*throw(AdServer::UserInfoSvcs::UserBindServer::NotReady,
      AdServer::UserInfoSvcs::UserBindServer::ChunkNotFound)*/
  {
    static const char* FUN = "UserBindServerImpl::get_user_id()";

    try
    {
      UserBindServerCore::GetUserRequestInfo core_request;
      core_request.id = request_info.id.in();
      core_request.timestamp = CorbaAlgs::unpack_time(request_info.timestamp);
      core_request.silent = request_info.silent;
      core_request.generate_user_id = request_info.generate_user_id;
      core_request.for_set_cookie = request_info.for_set_cookie;
      core_request.create_timestamp = CorbaAlgs::unpack_time(request_info.create_timestamp);
      core_request.current_user_id =
        request_info.current_user_id.length() > 0 ?
          CorbaAlgs::unpack_user_id(request_info.current_user_id) :
          Commons::UserId();

      const UserBindServerCore::GetUserResponseInfo core_res =
        core_->get_user_id(core_request);

      AdServer::UserInfoSvcs::UserBindServer::GetUserResponseInfo_var res(
        new AdServer::UserInfoSvcs::UserBindServer::GetUserResponseInfo());
      res->user_id = CorbaAlgs::pack_user_id(core_res.user_id);
      res->min_age_reached = core_res.min_age_reached;
      res->created = core_res.created;
      res->invalid_operation = core_res.invalid_operation;
      res->user_found = core_res.user_found;
      return res._retn();
    }
    catch(const UserBindServerCore::NotReady& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught UserBindServerCore::NotReady: " << ex.what();
      CORBACommons::throw_desc<AdServer::UserInfoSvcs::UserBindServer::NotReady>(ostr.str());
    }
    catch(const UserBindServerCore::ChunkNotFound& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught UserBindServerCore::ChunkNotFound: " << ex.what();
      CORBACommons::throw_desc<AdServer::UserInfoSvcs::
        UserBindServer::ChunkNotFound>(
          ostr.str());
    }

    return 0;
  }

  AdServer::UserInfoSvcs::UserBindMapper::AddUserResponseInfo*
  UserBindServerImpl::add_user_id(
    const AdServer::UserInfoSvcs::UserBindMapper::AddUserRequestInfo&
      request_info)
    /*throw(AdServer::UserInfoSvcs::UserBindServer::NotReady,
      AdServer::UserInfoSvcs::UserBindServer::ChunkNotFound)*/
  {
    static const char* FUN = "UserBindServerImpl::add_user_id()";

    /*
    std::cerr << FUN << ": " << std::endl <<
      "  id = " << request_info.id.in() << std::endl <<
      "  user_id = " << CorbaAlgs::unpack_user_id(request_info.user_id).to_string() << std::endl <<
      std::endl;
    */

    try
    {
      UserBindServerCore::AddUserRequestInfo core_request;
      core_request.id = request_info.id.in();
      core_request.user_id = CorbaAlgs::unpack_user_id(request_info.user_id);
      core_request.timestamp = CorbaAlgs::unpack_time(request_info.timestamp);
      const UserBindServerCore::AddUserResponseInfo core_res =
        core_->add_user_id(core_request);

      AdServer::UserInfoSvcs::UserBindMapper::AddUserResponseInfo_var res(
        new AdServer::UserInfoSvcs::UserBindMapper::AddUserResponseInfo());
      res->merge_user_id = CorbaAlgs::pack_user_id(core_res.merge_user_id);
      res->invalid_operation = core_res.invalid_operation;
      //res->min_age_reached = user_info.min_age_reached;
      //res->created = true;
      return res._retn();
    }
    catch(const UserBindServerCore::NotReady& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught UserBindServerCore::NotReady: " << ex.what();
      CORBACommons::throw_desc<AdServer::UserInfoSvcs::UserBindServer::NotReady>(ostr.str());
    }
    catch(const UserBindServerCore::ChunkNotFound& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught UserBindServerCore::ChunkNotFound: " << ex.what();
      CORBACommons::throw_desc<AdServer::UserInfoSvcs::
        UserBindServer::ChunkNotFound>(
          ostr.str());
    }

    return 0;
  }

  AdServer::UserInfoSvcs::UserBindMapper::BindRequestInfo*
  UserBindServerImpl::get_bind_request(
    const char* id,
    const CORBACommons::TimestampInfo& now)
    /*throw(AdServer::UserInfoSvcs::UserBindServer::NotReady,
      AdServer::UserInfoSvcs::UserBindServer::ChunkNotFound)*/
  {
    static const char* FUN = "UserBindServerImpl::get_bind_request()";

    try
    {
      const UserBindServerCore::BindRequestInfo bind_request =
        core_->get_bind_request(id, CorbaAlgs::unpack_time(now));

      AdServer::UserInfoSvcs::UserBindServer::BindRequestInfo_var res(
        new AdServer::UserInfoSvcs::UserBindServer::BindRequestInfo());

      res->bind_user_ids.length(bind_request.bind_user_ids.size());

      CORBA::ULong bind_i = 0;
      for(auto bind_it = bind_request.bind_user_ids.begin();
          bind_it != bind_request.bind_user_ids.end(); ++bind_it, ++bind_i)
      {
        res->bind_user_ids[bind_i] << *bind_it;
      }

      return res._retn();
    }
    catch(const UserBindServerCore::NotReady& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught UserBindServerCore::NotReady: " << ex.what();
      CORBACommons::throw_desc<AdServer::UserInfoSvcs::UserBindServer::NotReady>(ostr.str());
    }
    catch(const UserBindServerCore::ChunkNotFound& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught UserBindServerCore::ChunkNotFound: " << ex.what();
      CORBACommons::throw_desc<AdServer::UserInfoSvcs::
        UserBindServer::ChunkNotFound>(
          ostr.str());
    }

    return 0;
  }

  void
  UserBindServerImpl::add_bind_request(
    const char* id,
    const AdServer::UserInfoSvcs::UserBindServer::BindRequestInfo& bind_request_info,
    const CORBACommons::TimestampInfo& timestamp)
    /*throw(AdServer::UserInfoSvcs::UserBindServer::NotReady,
      AdServer::UserInfoSvcs::UserBindServer::ChunkNotFound)*/
  {
    static const char* FUN = "UserBindServerImpl::add_bind_request()";

    try
    {
      UserBindServerCore::BindRequestInfo bind_request;
      bind_request.bind_user_ids.reserve(bind_request_info.bind_user_ids.length());
      for(CORBA::ULong bind_i = 0; bind_i < bind_request_info.bind_user_ids.length(); ++bind_i)
      {
        bind_request.bind_user_ids.push_back(bind_request_info.bind_user_ids[bind_i].in());
      }

      core_->add_bind_request(id, bind_request, CorbaAlgs::unpack_time(timestamp));
    }
    catch(const UserBindServerCore::NotReady& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught UserBindServerCore::NotReady: " << ex.what();
      CORBACommons::throw_desc<AdServer::UserInfoSvcs::UserBindServer::NotReady>(ostr.str());
    }
    catch(const UserBindServerCore::ChunkNotFound& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught UserBindServerCore::ChunkNotFound: " << ex.what();
      CORBACommons::throw_desc<AdServer::UserInfoSvcs::
        UserBindServer::ChunkNotFound>(
          ostr.str());
    }
  }

  AdServer::UserInfoSvcs::UserBindServer::Source*
  UserBindServerImpl::get_source()
    /*throw(AdServer::UserInfoSvcs::UserBindServer::NotReady)*/
  {
    const UserBindServerCore::Source core_source = core_->get_source();
    AdServer::UserInfoSvcs::UserBindServer::Source_var res =
      new AdServer::UserInfoSvcs::UserBindServer::Source();
    res->chunks.length(core_source.chunks.size());
    for(CORBA::ULong i = 0; i < core_source.chunks.size(); ++i)
    {
      res->chunks[i] = core_source.chunks[i];
    }
    res->chunks_number = core_source.chunks_number;

    return res._retn();
  }
} // UserInfoSvcs::AdServer
