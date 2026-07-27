#pragma once

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

#include "BindRequestChunk.hpp"

namespace AdServer::UserInfoSvcs
{
  class BindRequestContainer:
    public BindRequestProcessor,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    using ChunkPathMap = std::map<unsigned long, std::string>;

  public:
    BindRequestContainer(
      Logging::Logger* logger,
      unsigned long common_chunks_number,
      const ChunkPathMap& chunk_folders,
      const char* file_prefix,
      const Generics::Time& extend_time_period,
      unsigned long portions_number)
      /*throw(Exception)*/;

    // BindRequestProcessor impl
    virtual void
    add_bind_request(
      const String::SubString& id,
      const BindRequest& bind_request,
      const Generics::Time& now)
      /*throw(ChunkNotFound, Exception)*/;

    AdServer::Commons::StartableAwaitable<BindRequest>
    co_get_bind_request(
      const String::SubString& external_id,
      const Generics::Time& now)
      /*throw(ChunkNotFound, Exception)*/ override;

    BindRequest
    get_bind_request(
      const String::SubString& external_id,
      const Generics::Time& now)
      /*throw(ChunkNotFound, Exception)*/ override;

    virtual void
    clear_expired(const Generics::Time& expire_time)
      /*throw(Exception)*/;

    virtual void
    dump() /*throw(Exception)*/;

  protected:
    using ChunkArray = std::vector<BindRequestChunk_var>;

  protected:
    virtual
    ~BindRequestContainer() noexcept;

    BindRequestChunk_var
    get_chunk_(const String::SubString& external_id)
      const /*throw(ChunkNotFound)*/;

  private:
    const Logging::Logger_var logger_;
    const unsigned long common_chunks_number_;
    ChunkArray chunks_;
  };

  using BindRequestContainer_var =
    ReferenceCounting::SmartPtr<BindRequestContainer>;

} /* namespace AdServer::UserInfoSvcs */
