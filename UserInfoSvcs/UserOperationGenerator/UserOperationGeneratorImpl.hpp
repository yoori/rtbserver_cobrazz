#ifndef USERINFOSVCS_USEROPERATIONGENERATOR_USEROPERATIONGENERATORIMPL_HPP_
#define USERINFOSVCS_USEROPERATIONGENERATOR_USEROPERATIONGENERATORIMPL_HPP_

#include <string>

#include <eh/Exception.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/TaskRunner.hpp>
#include <Logger/Logger.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

#include <xsd/UserInfoSvcs/UserOperationGeneratorConfig.hpp>
#include <UserInfoSvcs/UserInfoManager/UserOperationSaver.hpp>

#include "UidChannelSnapshot.hpp"

namespace AdServer
{
namespace UserInfoSvcs
{
  class UserOperationGeneratorImpl :
    public Generics::CompositeActiveObject,
    public ReferenceCounting::AtomicImpl
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    typedef xsd::AdServer::Configuration::UserOperationGeneratorConfigType
      ConfigType;

    typedef std::shared_ptr<ConfigType> ConfigPtr;

  public:
    UserOperationGeneratorImpl(
      const ConfigPtr& config,
      Generics::ActiveObjectCallback* callback,
      Logging::Logger* logger)
      /*throw(Exception)*/;

  protected:
    virtual
    ~UserOperationGeneratorImpl() noexcept
    {}

    void
    sync_snapshot_() /*throw(Exception)*/;

    void
    load_temp_snapshot_(const char* str_command_template) /*throw(Exception)*/;

    void
    load_and_sync_snapshot_task_() noexcept;

    void
    execute_operation_(
      const UidChannelSnapshot::Operation& op,
      std::size_t& processed_count)
      noexcept;

    void
    flush_user_operation_saver_() noexcept;

  protected:
    ConfigPtr config_;
    Generics::ActiveObjectCallback_var callback_;
    Logging::Logger_var logger_;

    Generics::TaskRunner_var task_runner_;
    Generics::Planner_var scheduler_;

    UserOperationSaver_var user_operation_saver_;
    UidChannelSnapshot_var current_snapshot_;

    Generics::ActiveObject_var interruper_;
  };

  typedef ReferenceCounting::AssertPtr<UserOperationGeneratorImpl>::Ptr
    UserOperationGeneratorImpl_var;
}
}

#endif /* USERINFOSVCS_USEROPERATIONGENERATOR_USEROPERATIONGENERATORIMPL_HPP_ */
