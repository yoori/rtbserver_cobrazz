#pragma once

#include <ReferenceCounting/Interface.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <Commons/Coro/StartableAwaitable.hpp>

namespace AdServer::UserInfoSvcs
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

    // return previous bind request state
    virtual AdServer::Commons::StartableAwaitable<BindRequest>
    co_get_bind_request(
      const String::SubString& external_id,
      const Generics::Time& now)
      /*throw(ChunkNotFound, Exception)*/ = 0;

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

  using BindRequestProcessor_var =
    ReferenceCounting::SmartPtr<BindRequestProcessor>;
}
