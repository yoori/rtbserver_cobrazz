#ifndef USER_INFO_SVCS_USER_BIND_ADMIN_APPLICATION_HPP
#define USER_INFO_SVCS_USER_BIND_ADMIN_APPLICATION_HPP

#include <map>
#include <set>
#include <string>

#include <ReferenceCounting/ReferenceCounting.hpp>

#include <eh/Exception.hpp>

#include <Generics/Singleton.hpp>
#include <Generics/Time.hpp>

#include <UserInfoSvcs/UserBindController/UserBindOperationDistributor.hpp>

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
  print_bind_request_(
    AdServer::UserInfoSvcs::UserBindMapper* user_bind_mapper,
    const String::SubString& bind_request_id)
    noexcept;
};

typedef Generics::Singleton<Application_> Application;

#endif /*USER_INFO_SVCS_USER_BIND_ADMIN_APPLICATION_HPP*/
