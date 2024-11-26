#ifndef USERINFOSVCS_USERBINDCONTAINER_HPP
#define USERINFOSVCS_USERBINDCONTAINER_HPP

#include <string>

#include <eh/Exception.hpp>

#include <ReferenceCounting/ReferenceCounting.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>

#include <Logger/Logger.hpp>
#include <Sync/SyncPolicy.hpp>
#include <Generics/Time.hpp>
#include <Generics/GnuHashTable.hpp>
#include <Generics/HashTableAdapters.hpp>
#include <Generics/Hash.hpp>

#include <Commons/UserInfoManip.hpp>
#include <Commons/LockMap.hpp>
#include <Commons/Containers.hpp>

#include <rocksdb/advanced_options.h>

#include "UserBindChunk.hpp"

namespace AdServer
{
namespace UserInfoSvcs
{
  class UserBindContainer:
    public UserBindProcessor,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    using ChunkPathMap = std::map<unsigned long, std::string>;
    using ExpireTime = std::uint16_t;
    using Source = std::string;
    using SourceExpireTime = std::pair<Source, ExpireTime>;
    using ListSourceExpireTime = std::vector<SourceExpireTime>;

  public:
    UserBindContainer(
      Logging::Logger* logger,
      const ExpireTime seen_default_expire_time,
      const ExpireTime bound_default_expire_time,
      const ListSourceExpireTime& bound_list_source_expire_time,
      const unsigned long common_chunks_number,
      const ChunkPathMap& chunk_folders,
      const char* file_prefix,
      const char* bound_file_prefix,
      const Generics::Time& extend_time_period,
      const Generics::Time& bound_extend_time_period,
      const Generics::Time& min_bind_age,
      const bool bind_at_min_age,
      const unsigned long max_bad_event,
      const unsigned long portions_number,
      const bool load_slave,
      const unsigned long partition_index, // instance partition number (first or second part of cluster)
      const unsigned long partitions_number,
      const bool enable_two_layer_mode,
      const std::size_t rocksdb_number_threads,
      const rocksdb::CompactionStyle rocksdb_compaction_style,
      const std::uint32_t rocksdb_block_сache_size_mb,
      const std::uint32_t rocksdb_ttl)
      /*throw(Exception)*/;

    static void
    fetch_chunk_folders(
      ChunkPathMap& chunks,
      const char* chunks_root,
      const char* chunks_prefix)
      /*throw(eh::Exception)*/;

    // UserBindProcessor impl
    virtual UserInfo
    add_user_id(
      const String::SubString& external_id,
      const Commons::UserId& user_id,
      const Generics::Time& now,
      bool resave_if_exists,
      bool ignore_bad_event)
      /*throw(ChunkNotFound, Exception)*/;

    virtual UserInfo
    get_user_id(
      const String::SubString& external_id,
      const Commons::UserId& current_user_id,
      const Generics::Time& now,
      bool silent,
      const Generics::Time& create_time,
      bool for_set_cookie)
      /*throw(ChunkNotFound, Exception)*/;

    virtual void
    clear_expired(
      const Generics::Time& unbound_expire_time,
      const Generics::Time& bound_expire_time)
      /*throw(Exception)*/;

    virtual void
    dump() /*throw(Exception)*/;

  protected:
    typedef std::vector<UserBindChunk_var> UserBindChunkArray;

  protected:
    virtual ~UserBindContainer() noexcept;

    UserBindChunk_var
    get_chunk_(const String::SubString& external_id)
      const /*throw(ChunkNotFound)*/;

  private:
    const Logging::Logger_var logger_;
    const unsigned long common_chunks_number_;
    UserBindChunkArray chunks_;
  };

  typedef ReferenceCounting::SmartPtr<UserBindContainer>
    UserBindContainer_var;

} /* UserInfoSvcs */
} /* AdServer */

#endif /*USERINFOSVCS_USERBINDCONTAINER_HPP*/
