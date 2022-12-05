#ifndef USERINFOSVCS_USEROPERATIONGENERATOR_USEROPERATIONGENERATORMAIN_HPP_
#define USERINFOSVCS_USEROPERATIONGENERATOR_USEROPERATIONGENERATORMAIN_HPP_

#include <memory>

#include <eh/Exception.hpp>

#include <Commons/ProcessControlVarsImpl.hpp>

#include "UserOperationGeneratorImpl.hpp"

namespace AdServer
{
namespace UserInfoSvcs
{
  class UserOperationGeneratorApp
    : public AdServer::Commons::ProcessControlVarsLoggerImpl
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    DECLARE_EXCEPTION(InvalidArgument, Exception);

  public:
    UserOperationGeneratorApp() /*throw(eh::Exception)*/;

    void
    main(
      int argc,
      char** argv)
      noexcept;

  protected:
    virtual
    void
    shutdown(CORBA::Boolean wait_for_completion)
      /*throw(CORBA::SystemException)*/;

  protected:


  protected:
    virtual
    ~UserOperationGeneratorApp() noexcept
    {};

  private:
    CORBACommons::CorbaServerAdapter_var corba_server_adapter_;
    CORBACommons::CorbaConfig corba_config_;
    UserOperationGeneratorImpl::ConfigPtr config_;
    Logging::Logger_var logger_;

    UserOperationGeneratorImpl_var user_operation_generator_impl_;

    Sync::PosixMutex shutdown_lock_;
  };

  typedef ReferenceCounting::SmartPtr<UserOperationGeneratorApp>
    UserOperationGeneratorApp_var;

  typedef Generics::Singleton<
    UserOperationGeneratorApp, UserOperationGeneratorApp_var>
      UserOperationGeneratorSingleton;
}
}

#endif /* USERINFOSVCS_USEROPERATIONGENERATOR_USEROPERATIONGENERATORMAIN_HPP_ */
