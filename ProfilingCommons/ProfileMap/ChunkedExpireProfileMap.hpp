#pragma once

#include <atomic>
#include <memory>
#include <optional>
#include <vector>
#include <set>
#include <eh/Exception.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <Commons/Coro.hpp>

/**
 * ChunkedUserMap:
 *   ChunkedProfileMap -> TransactionProfileMap ->
 *   AdaptProfileMap -> ExpireProfileMap
 */
namespace AdServer
{
namespace ProfilingCommons
{
  typedef std::set<unsigned long> ChunkIdSet;

  template<typename KeyType,
    typename ProfileMapType,
    typename KeyHashType>
  class ChunkedProfileMap:
    public virtual ProfileMap<KeyType>,
    public virtual AsyncProfileMap<KeyType>,
    public virtual ReferenceCounting::AtomicImpl
  {
    /* requirements:
     * KeyType must be convertible to ProfileMapType::KeyTypeT */
  public:
    typedef ProfileMapType BaseProfileMap;
    typedef ReferenceCounting::SmartPtr<BaseProfileMap> BaseProfileMap_var;
    typedef typename ProfileMapType::Transaction_var Transaction_var;

    typedef std::map<unsigned long, ReferenceCounting::SmartPtr<ProfileMapType> >
      ChunkIdToProfileMap;

    typedef std::vector<BaseProfileMap_var> ChunkArray;

    DECLARE_EXCEPTION(Exception, typename ProfileMap<KeyType>::Exception);
    DECLARE_EXCEPTION(ChunkNotFound, Exception);

    ChunkedProfileMap(
      unsigned long common_chunks_number,
      const ChunkIdToProfileMap& chunks,
      const KeyHashType& key_hash = KeyHashType())
      /*throw(Exception, typename BaseProfileMap::Exception)*/;

    bool
    dispose_profile(const KeyType& key) const
      noexcept;

    virtual void
    wait_preconditions(
      const KeyType& key,
      OperationPriority priority) const
      /*throw(ChunkNotFound, Exception)*/;

    bool
    check_profile(const KeyType& key) const
      /*throw(ChunkNotFound, Exception)*/;

    void
    check_profile_async(
      const KeyType& key,
      typename AsyncProfileMap<KeyType>::CheckCallback callback) const
      override;

    Generics::ConstSmartMemBuf_var
    get_profile_async(
      const KeyType& key,
      typename AsyncProfileMap<KeyType>::GetCallback callback,
      std::optional<Generics::Time> last_access_time = std::nullopt)
      override;

    void
    save_profile_async(
      const KeyType& key,
      const Generics::ConstSmartMemBuf* mem_buf,
      const Generics::Time& now = Generics::Time::get_time_of_day(),
      typename AsyncProfileMap<KeyType>::SaveCallback callback =
        typename AsyncProfileMap<KeyType>::SaveCallback())
      override;

    void
    remove_profile_async(
      const KeyType& key,
      OperationPriority priority = OP_RUNTIME,
      typename AsyncProfileMap<KeyType>::RemoveCallback callback =
        typename AsyncProfileMap<KeyType>::RemoveCallback())
      override;

    void
    clear_expired_async(
      const Generics::Time& expire_time,
      typename AsyncProfileMap<KeyType>::CompleteCallback complete =
        typename AsyncProfileMap<KeyType>::CompleteCallback())
      override;

    Generics::ConstSmartMemBuf_var
    get_profile(
      const KeyType& key,
      Generics::Time* last_access_time = 0)
      /*throw(ChunkNotFound, Exception)*/;

    void
    save_profile(
      const KeyType& key,
      const Generics::ConstSmartMemBuf* mem_buf,
      const Generics::Time& now = Generics::Time::get_time_of_day(),
      OperationPriority priority = OP_RUNTIME)
      /*throw(ChunkNotFound, Exception)*/;

    bool
    remove_profile(
      const KeyType& key,
      OperationPriority priority = OP_RUNTIME)
      /*throw(ChunkNotFound, Exception)*/;

    Transaction_var
    get_transaction(
      const KeyType& key,
      bool check_max_waiters = true,
      OperationPriority op_priority = ProfilingCommons::OP_RUNTIME)
      /*throw(ChunkNotFound,
        typename BaseProfileMap::MaxWaitersReached,
        typename BaseProfileMap::Exception)*/;

    AdServer::Commons::Task<Transaction_var>
    co_get_transaction(
      const KeyType& key,
      bool check_max_waiters = true,
      OperationPriority op_priority = ProfilingCommons::OP_RUNTIME);

    virtual void
    process_keys(
      std::function<void(const KeyType&)> process_key,
      std::function<void(void)> process_complete)
      /*throw(Exception)*/;

    void clear_expired(const Generics::Time& expire_time)
      /*throw(Exception)*/;

    unsigned long size() const noexcept;

    unsigned long area_size() const noexcept;

    const ChunkIdSet& chunk_ids() const noexcept;

    BaseProfileMap_var get_chunk(unsigned long chunk_id) const
      /*throw(ChunkNotFound)*/;

    const ChunkArray& chunks() const noexcept;

  protected:
    virtual ~ChunkedProfileMap() noexcept {}

    BaseProfileMap_var get_chunk_(unsigned long chunk_id) const
      noexcept;

  private:
    const KeyHashType key_hash_;
    unsigned long common_chunks_number_;
    ChunkIdSet chunk_ids_;
    ChunkArray chunks_;
  };
}
}

#include "ChunkedExpireProfileMap.tpp"
