#ifndef USER_INFO_SVCS_USER_BIND_ADMIN_APPLICATION_HPP
#define USER_INFO_SVCS_USER_BIND_ADMIN_APPLICATION_HPP

#include <map>
#include <set>
#include <string>
#include <string_view>

#include <ReferenceCounting/ReferenceCounting.hpp>

#include <eh/Exception.hpp>

#include <Generics/Singleton.hpp>
#include <Generics/Time.hpp>

#include <UserInfoSvcs/UserBindController/UserBindOperationDistributor.hpp>
#include <UserInfoSvcs/UserBindServer/UserBindServerGrpc.grpc.pb.h>

using namespace AdServer::UserInfoSvcs;

class Application_
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  Application_() noexcept;

  virtual
  ~Application_() noexcept;

  void
  main(int& argc, char** argv) /*throw(eh::Exception)*/;

protected:
  static void
  print_bind_request_grpc_(
    adserver::user_info_svcs::user_bind::UserBindServerGrpc::Stub* user_bind_stub,
    const String::SubString& bind_request_id)
    noexcept;

  static void
  print_bind_request_(
    AdServer::UserInfoSvcs::UserBindMapper* user_bind_mapper,
    const String::SubString& bind_request_id)
    noexcept;

  static void
  add_user_id_grpc_(
    adserver::user_info_svcs::user_bind::UserBindServerGrpc::Stub* user_bind_stub,
    std::string_view external_id,
    const AdServer::Commons::UserId& user_id)
    noexcept;

  static void
  add_user_id_(
    AdServer::UserInfoSvcs::UserBindMapper* user_bind_mapper,
    std::string_view external_id,
    const AdServer::Commons::UserId& user_id)
    noexcept;

  static void
  get_user_id_grpc_(
    adserver::user_info_svcs::user_bind::UserBindServerGrpc::Stub* user_bind_stub,
    std::string_view external_id)
    noexcept;

  static void
  get_user_id_(
    AdServer::UserInfoSvcs::UserBindMapper* user_bind_mapper,
    std::string_view external_id)
    noexcept;
};

typedef Generics::Singleton<Application_> Application;

#endif /*USER_INFO_SVCS_USER_BIND_ADMIN_APPLICATION_HPP*/
