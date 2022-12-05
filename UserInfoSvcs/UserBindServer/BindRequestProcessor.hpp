#ifndef USERINFOSVCS_BINDREQUESTPROCESSOR_HPP
#define USERINFOSVCS_BINDREQUESTPROCESSOR_HPP

#include <ReferenceCounting/Interface.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

namespace AdServer
{
namespace UserInfoSvcs
{
  struct BindRequestProcessor:
    public virtual ReferenceCounting::Interface
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    DECLARE_EXCEPTION(ChunkNotFound, Exception);

    struct BindRequest
    {
      std::vector<std::string> bind_user_ids;
    };

  public:
    // return previous state
    virtual void
    add_bind_request(
      const String::SubString& id,
      const BindRequest& bind_request,
      const Generics::Time& now)
      /*throw(ChunkNotFound, Exception)*/ = 0;

    // create_time : time of user creation, will be used for min age check
    virtual BindRequest
    get_bind_request(
      const String::SubString& external_id,
      const Generics::Time& now)
      /*throw(ChunkNotFound, Exception)*/ = 0;

    virtual void
    clear_expired(const Generics::Time& expire_time)
      /*throw(Exception)*/ = 0;

    virtual void
    dump() /*throw(Exception)*/ = 0;
  };

  typedef ReferenceCounting::SmartPtr<BindRequestProcessor>
    BindRequestProcessor_var;
}
}

namespace AdServer
{
namespace UserInfoSvcs
{
}
}

#endif /*USERINFOSVCS_BINDREQUESTPROCESSOR_HPP*/
