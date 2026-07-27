#pragma once

#include <optional>
#include <string>

#include <eh/Exception.hpp>

#include <ReferenceCounting/ReferenceCounting.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>

#include <Logger/Logger.hpp>
#include <Generics/Time.hpp>
#include <Generics/GnuHashTable.hpp>
#include <Generics/HashTableAdapters.hpp>
#include <Generics/Hash.hpp>

#include <Commons/UserInfoManip.hpp>
#include <Commons/LockMap.hpp>
#include <Commons/Containers.hpp>

#include "UserBindProcessor.hpp"

namespace AdServer::UserInfoSvcs
{
  class UserBindContainer:
    public UserBindProcessor,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    typedef std::map<unsigned long, std::string> ChunkPathMap;

  public:
    UserBindContainer(
      Logging::Logger* logger,
      unsigned long common_chunks_number,
      const ChunkPathMap& chunk_folders,
      const char* file_prefix,
      const char* bound_file_prefix,
      const Generics::Time& extend_time_period,
      const Generics::Time& bound_extend_time_period,
      std::optional<Generics::Time> bind_min_age,
      unsigned long max_bad_event,
      unsigned long portions_number,
      bool load_slave,
      unsigned long partition_index, // instance partition number (first or second part of cluster)
      unsigned long partitions_number)
      /*throw(Exception)*/;

    static void
    fetch_chunk_folders(
      ChunkPathMap& chunks,
      const char* chunks_root,
      const char* chunks_prefix)
      /*throw(eh::Exception)*/;

    // UserBindProcessor impl
    AdServer::Commons::StartableAwaitable<UserInfo>
    co_add_user_id(
      const String::SubString& external_id,
      const Commons::UserId& user_id,
      const Generics::Time& now,
      bool resave_if_exists,
      bool ignore_bad_event,
      bool set_cookie_flag) override;

    AdServer::Commons::StartableAwaitable<UserInfo>
    co_get_user_id(
      const String::SubString& external_id,
      const Commons::UserId& current_user_id,
      const Generics::Time& now,
      bool silent,
      const Generics::Time& create_time,
      bool for_set_cookie,
      bool generate_user_id) override;

    virtual void
    clear_expired(
      const Generics::Time& unbound_expire_time,
      const Generics::Time& bound_expire_time)
      /*throw(Exception)*/;

    virtual void
    dump() /*throw(Exception)*/;

  protected:
    typedef std::vector<UserBindProcessor_var> UserBindProcessorArray;

  protected:
    virtual ~UserBindContainer() noexcept;

    UserBindProcessor_var
    get_chunk_(const String::SubString& external_id)
      const /*throw(ChunkNotFound)*/;

  private:
    const Logging::Logger_var logger_;
    const unsigned long common_chunks_number_;
    UserBindProcessorArray chunks_;
  };

  typedef ReferenceCounting::SmartPtr<UserBindContainer>
    UserBindContainer_var;

} /* AdServer::UserInfoSvcs */
