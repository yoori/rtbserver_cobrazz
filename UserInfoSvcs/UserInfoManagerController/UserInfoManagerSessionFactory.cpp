#include <string>

//#include <Commons/UserInfoManip.hpp>

//#include <UserInfoSvcs/UserInfoManager/UserInfoManager.hpp>
//#include <UserInfoSvcs/UserInfoManagerController/UserInfoManagerController.hpp>

#include "UserInfoManagerSessionFactory.hpp"
#include "UserInfoManagerSessionImpl.h"

/**
 * UserInfomanagerSessionFactoryImpl
 * implementation of corba valuetype factory
 */
class UserInfoManagerSessionFactoryImpl :
  public virtual CORBA::ValueFactoryBase
{
public:
  virtual CORBA::ValueBase* create_for_unmarshal();
};

/**
 * UserInfoManagerSessionFactoryImpl
 */
CORBA::ValueBase*
UserInfoManagerSessionFactoryImpl::create_for_unmarshal()
{
  return new AdServer::UserInfoSvcs::UserInfoManagerSessionImpl();
}

/**
 * UserInfoManagerSessionFactory
 */
namespace AdServer{
namespace UserInfoSvcs{

  void
  UserInfoManagerSessionFactory::init(
    const CORBACommons::CorbaClientAdapter& corba_client_adapter)
    /*throw(eh::Exception)*/
  {
    static const char* FUN = "UserInfoManagerSessionFactory::init()";

    /* register UserInfomanagerSessionFactoryImpl
       for creating Session valuetype
     */
    try
    {
      CORBA::ValueFactoryBase_var factory =
        new UserInfoManagerSessionFactoryImpl();

      corba_client_adapter.register_value_factory(
        UserInfoManagerSessionImpl::_tao_obv_static_repository_id(),
        factory);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't init UserInfoManagerSessionFactory: " <<
        ex.what();
      throw Exception(ostr);
    }
    catch(const CORBA::SystemException& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't init UserInfoManagerSessionFactory. "
        "Caught CORBA::SystemException: " << e;
      throw Exception(ostr);
    }
  }

}
}

