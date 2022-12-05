#ifndef USERBINDOPERATIONSAVER_HPP
#define USERBINDOPERATIONSAVER_HPP

#include <string>
#include <ProfilingCommons/MessageSaver.hpp>

#include "UserBindProcessor.hpp"

namespace AdServer
{
namespace UserInfoSvcs
{
  class UserBindOperationSaver:
    public UserBindProcessor,
    public virtual Generics::ActiveObject,
    public virtual ReferenceCounting::AtomicImpl,
    public ProfilingCommons::MessageSaver
  {
  public:
    enum Operation
    {
      OP_ADD_USER_ID = 1,
      OP_GET_USER_ID
    };

  public:
    UserBindOperationSaver(
      Logging::Logger* logger,
      const char* output_files_path,
      const char* output_file_prefix,
      unsigned long chunks_number,
      const Generics::Time& flush_period,
      UserBindProcessor* next_processor);

    virtual UserInfo
    add_user_id(
      const String::SubString& external_id,
      const Commons::UserId& user_id,
      const Generics::Time& now,
      bool resave_if_exists,
      bool ignore_bad_event)
      /*throw(ChunkNotFound, UserBindProcessor::Exception)*/;

    virtual UserInfo
    get_user_id(
      const String::SubString& external_id,
      const Commons::UserId& current_user_id,
      const Generics::Time& now,
      bool silent,
      const Generics::Time& create_time,
      bool for_set_cookie)
      /*throw(ChunkNotFound, UserBindProcessor::Exception)*/;

    virtual void
    clear_expired(
      const Generics::Time& unbound_expire_time,
      const Generics::Time& bound_expire_time)
      /*throw(UserBindProcessor::Exception)*/;

    virtual void
    dump() /*throw(UserBindProcessor::Exception)*/;

  protected:
    virtual
    ~UserBindOperationSaver() noexcept = default;

    UserBindProcessor_var next_processor_;
  };

  typedef ReferenceCounting::SmartPtr<UserBindOperationSaver>
    UserBindOperationSaver_var;
}
}

#endif /*USERBINDOPERATIONSAVER_HPP*/
