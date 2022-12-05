#ifndef USERINFOSVCS_USERBINDPROCESSOR_HPP
#define USERINFOSVCS_USERBINDPROCESSOR_HPP

#include <ReferenceCounting/Interface.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <Commons/UserInfoManip.hpp>
#include <Commons/UserIdBlackList.hpp>

namespace AdServer
{
namespace UserInfoSvcs
{
  struct UserBindProcessor:
    public virtual ReferenceCounting::Interface
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    DECLARE_EXCEPTION(ChunkNotFound, Exception);

    struct UserInfo
    {
      UserInfo();

      Commons::UserId user_id;
      bool min_age_reached;
      bool user_id_generated;
      bool created;
      bool invalid_operation;
      bool user_found;
      // TODO: expire_time_prolonged (true if created or time holder changed)

      std::ostream&
      print(std::ostream& out) const noexcept;
    };

  public:
    // return previous state
    virtual UserInfo
    add_user_id(
      const String::SubString& external_id,
      const Commons::UserId& user_id,
      const Generics::Time& now,
      bool resave_if_exists,
      bool ignore_bad_event)
      /*throw(ChunkNotFound, Exception)*/ = 0;

    // create_time : time of user creation, will be used for min age check
    virtual UserInfo
    get_user_id(
      const String::SubString& external_id,
      const Commons::UserId& current_user_id,
      const Generics::Time& now,
      bool silent,
      const Generics::Time& create_time,
      bool for_set_cookie)
      /*throw(ChunkNotFound, Exception)*/ = 0;

    virtual void
    clear_expired(
      const Generics::Time& unbound_expire_time,
      const Generics::Time& bound_expire_time)
      /*throw(Exception)*/ = 0;

    virtual void
    dump() /*throw(Exception)*/ = 0;

  protected:
    virtual
    ~UserBindProcessor() noexcept = default;
  };

  typedef ReferenceCounting::SmartPtr<UserBindProcessor>
    UserBindProcessor_var;
}
}

namespace AdServer
{
namespace UserInfoSvcs
{
  inline
  UserBindProcessor::UserInfo::UserInfo()
    : min_age_reached(false),
      user_id_generated(false),
      created(false),
      invalid_operation(false)
  {}

  inline
  std::ostream&
  UserBindProcessor::UserInfo::print(std::ostream& out) const
    noexcept
  {
    out << "user_id: " << user_id.to_string() <<
      ", min_age_reached: " << min_age_reached <<
      ", user_id_generated: " << user_id_generated <<
      ", created: " << created <<
      ", invalid_operation: " << invalid_operation <<
      ", user found: " << user_found;
    return out;
  }
}
}

#endif /*USERINFOSVCS_USERBINDPROCESSOR_HPP*/
