/**
 * Implements container that store chunks of mappings ID --> Profile,
 * each chunk have expiration date. And we able to locate freshest Profile.
 * Initiate clearance expired chunks. Profiles can be moved in more recent
 * chunks or be removed from container as out-of-date.
 * @file ProfilingCommons/ExpireProfileMap.hpp
 */
#ifndef PROFILING_COMMONS_EXPIRE_PROFILE_MAP_HPP
#define PROFILING_COMMONS_EXPIRE_PROFILE_MAP_HPP

#include <list>
#include <string>

#include <eh/Exception.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>

#include <Sync/SyncPolicy.hpp>
#include <Generics/Time.hpp>
#include <Generics/MemBuf.hpp>

#include <Commons/LockMap.hpp>
#include <ProfilingCommons/PlainStorage/LayerFactory.hpp>

#include "ProfileMap.hpp"
#include <ProfilingCommons/ProfileMap/MemIndexProfileMap.hpp>
#include <ProfilingCommons/ProfileMap/HashIndexProfileMap.hpp>

namespace AdServer
{
namespace ProfilingCommons
{
  template<typename PlainSubMapType>
  struct DefaultFileOpenStrategy
  {
    ReferenceCounting::SmartPtr<PlainSubMapType>
    operator()(const char* file)
      const
    {
      return new PlainSubMapType(file);
    }
  };

  template<typename PlainSubMapType>
  struct CacheFileOpenStrategy
  {
    CacheFileOpenStrategy(
      PlainStorage::LayerFactory<Sync::Policy::PosixThreadRW>::
        CacheT* cache)
      noexcept
        : cache_(ReferenceCounting::add_ref(cache))
    {}

    ReferenceCounting::SmartPtr<PlainSubMapType>
    operator()(const char* file)
      const
    {
      typedef PlainStorage::LayerFactory<Sync::Policy::PosixThreadRW>
        LayerFactoryT;

      ReferenceCounting::SmartPtr<LayerFactoryT::WriteRecordLayerT>
        layer = PlainStorage::LayerFactory<Sync::Policy::PosixThreadRW>::
        create_write_record_layer(file, cache_);

      return new PlainSubMapType(layer, layer);
    }

  protected:
    ReferenceCounting::SmartPtr<
      PlainStorage::LayerFactory<Sync::Policy::PosixThreadRW>::CacheT>
      cache_;
  };

  template<typename PlainSubMapType>
  struct CacheInterruptFileOpenStrategy
  {
    CacheInterruptFileOpenStrategy(
      PlainStorage::LayerFactory<Sync::Policy::PosixThreadRW>::
        CacheT* cache,
      typename PlainSubMapType::InterruptCallback* interrupt)
      noexcept
        : cache_(ReferenceCounting::add_ref(cache)),
          interrupt_(ReferenceCounting::add_ref(interrupt))
    {}

    ReferenceCounting::SmartPtr<PlainSubMapType>
    operator()(const char* file)
      const
    {
      typedef PlainStorage::LayerFactory<Sync::Policy::PosixThreadRW>
        LayerFactoryT;

      ReferenceCounting::SmartPtr<LayerFactoryT::WriteRecordLayerT>
        layer = PlainStorage::LayerFactory<Sync::Policy::PosixThreadRW>::
          create_write_record_layer(file);

      return new PlainSubMapType(layer, layer, interrupt_);
    }

  protected:
    ReferenceCounting::SmartPtr<
      PlainStorage::LayerFactory<Sync::Policy::PosixThreadRW>::CacheT>
      cache_;

    ReferenceCounting::SmartPtr<
      typename PlainSubMapType::InterruptCallback> interrupt_;
  };

  /**
   * Base class for manage expiryable chunks of request ID profiles.
   */
  template <
    typename KeyType,
    typename PlainSubMapType,
    typename FileOpenStrategyType>
  class ExpireProfileMapBase:
    public AdServer::ProfilingCommons::ProfileMap<KeyType>,
    public ReferenceCounting::AtomicImpl
  {
  public:
    DECLARE_EXCEPTION(Exception, typename ProfileMap<KeyType>::Exception);

    /**
     * Default constructor do nothing
     */
    ExpireProfileMapBase() noexcept;

    /**
     * init creating object, open files and create and push SubMaps
     * in requests_ for each convenient file.
     * @param base_path Folder path to start from selection of files
     *   to be loaded. All files are loading that contain max keeping
     *   time information. Mask of files is '*', recursively pass folders.
     * @param prefix Files with this prefix loaded only
     * @param extend_time_period The delta of time, step of expiration for
     *   new chunks of profiles.
     */
    ExpireProfileMapBase(
      const char* base_path,
      const char* prefix,
      const Generics::Time& extend_time_period,
      FileOpenStrategyType file_open_strategy = FileOpenStrategyType())
      /*throw(Exception)*/;

    virtual
    bool
    check_profile(const KeyType& key) const
      /*throw(Exception)*/;

    /**
     * Block higher lock (requests), iterate through list of SubMaps,
     * and find profile by key on each step, if found, do lowest lock
     * (content). Under content lock do copy of profile and return it.
     * @return The profile with correspond key stored at most modern chunk
     */
    virtual
    Generics::ConstSmartMemBuf_var
    get_profile(
      const KeyType& key,
      Generics::Time* last_access_time = 0)
      /*throw(Exception)*/;

    /**
     * Search first chunk with specified profile key.
     * Remove record if chunk inactual for now data. For example,
     * freshest profile can be removed if you write in retrospect
     * (in older chunk)
     * Update profile in chunk actual for specified time.
     * @param key The ID of request that will be saved in this container
     *   of obsolescent chunks
     * @param mem_buf The memory buffer (user profile) to be saved
     *   by specified key
     * @param now The moment of time that you want to keep the memory
     *   buffer
     */
    virtual void
    save_profile(
      const KeyType& key,
      const Generics::ConstSmartMemBuf* mem_buf,
      const Generics::Time& now = Generics::Time::get_time_of_day(),
      OperationPriority op_priority = OP_RUNTIME)
      /*throw(Exception)*/;

    /**
     * Remove records with specified request ID from all chunks
     * @param key The request ID for records that will be erased from
     * all available chunks
     */
    virtual
    bool
    remove_profile(
      const KeyType& key,
      OperationPriority op_priority = OP_RUNTIME)
      /*throw(Exception)*/;

    /**
     * Mark all expired chunks that they should be removed
     * (remove chunk file from file system before destruction).
     * Free expired chunks, remove from container
     * See SubMap destructor
     */
    void
    clear_expired(const Generics::Time& expire_time)
      noexcept;

    /**
     * calculate cumulative size of all SubMaps stored in requests_
     * @return The sum of sizes of SubMap containers
     */
    virtual unsigned long
    size() const noexcept;

    /**
     * calculate cumulative area size of all SubMaps stored in requests_
     * @return The sum of area sizes of SubMap containers
     */
    virtual unsigned long
    area_size() const noexcept;

    /**
     * Copy all keys from all SubMaps in some output iterator
     * @param ins_it The output iterator - place for copied keys
     */
    void
    copy_keys(typename ProfileMap<KeyType>::KeyList& keys)
      /*throw(Exception)*/
    {
      static const char* FUN = "ExpireProfileMapBase::copy_keys()";

      try
      {
        SubMapListHolder_var sub_maps_holder = sub_maps_();

        for(typename SubMapList::const_iterator it = sub_maps_holder->maps.begin();
              it != sub_maps_holder->maps.end(); ++it)
        {
          (*it)->map->copy_keys(keys);
        }
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": Caught eh::Exception: " << ex.what();
        throw Exception(ostr);
      }
    }

    /**
     * debug method
     * @return The number of used chunks
     */
    unsigned long
    file_count_() const noexcept;

  protected:
    typedef Sync::Policy::PosixThread SyncPolicy;

    class UnlinkSubMapsHolder: public ReferenceCounting::AtomicImpl
    {
    public:
      SyncPolicy::Mutex lock;
      std::set<Generics::Time> unlink_maps;

    protected:
      virtual
      ~UnlinkSubMapsHolder() noexcept
      {
      }
    };

    typedef ReferenceCounting::SmartPtr<UnlinkSubMapsHolder>
      UnlinkSubMapsHolder_var;

    /**
     * Chunk is storing keys -> data mapping, actual before some time
     */
    class SubMap: public ReferenceCounting::AtomicImpl
    {
    public:
      /**
       * Open file by creating WriteRecordLayer
       */
      SubMap(const char* file,
        const Generics::Time& min_time_val,
        const Generics::Time& max_time_val,
        PlainSubMapType* map_val)
        /*throw(eh::Exception)*/;

      /**
       * Do not implemented
       */
      const Generics::Time& creation_time() const noexcept;

      /// The name of file that store chunk data
      const std::string filename;
      ///
      const Generics::Time min_time;
      /// The maximum time to be profile data actual
      const Generics::Time max_time;

      ReferenceCounting::SmartPtr<PlainSubMapType> map;

      // if != 0: file must be removed on object destruction,
      // and unregistered from holder
      UnlinkSubMapsHolder_var unlink_sub_maps_holder;

    protected:
      /**
       * If the flag to_delete=true, close and delete the file
       */
      virtual
      ~SubMap() noexcept;
    };

    typedef ReferenceCounting::SmartPtr<SubMap> SubMap_var;
    typedef std::list<SubMap_var> SubMapList;

    class SubMapListHolder: public ReferenceCounting::AtomicCopyImpl
    {
    public:
      SubMapList maps; // in order of max_time descending

    private:
      virtual
      ~SubMapListHolder() noexcept
      {
      }
    };

    typedef ReferenceCounting::SmartPtr<SubMapListHolder>
      SubMapListHolder_var;

    typedef AdServer::Commons::LockMap<KeyType> KeyLockMap;

  protected:
    virtual
    ~ExpireProfileMapBase() noexcept
    {
    }

    bool
    remove_profile_i_(const KeyType& key, SubMap* checked_sub_map)
      /*throw(Exception)*/;

    /**
     * @param now The time moment which should be returned chunk that
     * actual on this moment
     */
    SubMap_var
    find_or_create_chunk_(const Generics::Time& now)
      /*throw(Exception)*/;

    /**
     * Create new chunk file, expiry time encoded in the name of file
     * @param now The timestamp of maximum time for profiles stored in the
     *   chunk
     * @return The chunk with specified expiration
     */
    SubMap_var
    create_new_chunk_(
      const Generics::Time& min_time,
      const Generics::Time& max_time)
      /*throw(Exception)*/;

    typename SubMapList::iterator
    find_chunk_(
      SubMap_var& target_map,
      SubMapListHolder* sub_maps_holder,
      const Generics::Time& now)
      /*throw(Exception)*/;

    SubMapListHolder_var
    sub_maps_() const noexcept;

  protected:
    /// Root folder for searching and loading chunks
    const std::string chunk_file_root_;
    const std::string chunk_file_prefix_;
    /// The time step for chunks expiration moments
    const Generics::Time extend_time_period_;
    const FileOpenStrategyType file_open_strategy_;

    /// synchronize requests: get_profile, save_profile, etc.
    mutable SyncPolicy::Mutex extend_lock_;
    mutable SyncPolicy::Mutex sub_maps_lock_;
    mutable SyncPolicy::Mutex unlink_marking_lock_;

    /// Chunks stored in order of up-to-date --> to obsolescence
    SubMapListHolder_var sub_maps_holder_;
    const UnlinkSubMapsHolder_var unlink_sub_maps_holder_;
  };

  template<typename KeyType, typename KeyAccessorType>
  struct ExpireProfileMapTraits
  {
    typedef MemIndexProfileMap<
      KeyType,
      PlainStorage::FragmentBlockIndex,
      DefaultMapTraits<
        KeyType,
        KeyAccessorType,
        PlainStorage::FragmentBlockIndex,
        PlainStorage::FragmentBlockIndexSerializer> >
      SubMap;
  };

  /**
   * ExpireProfileMap expiry chunks container on
   * PlainStorage::Map containers
   */
  template<
    typename KeyType,
    typename KeyAccessorType,
    typename FileOpenStrategyType = DefaultFileOpenStrategy<
      typename ExpireProfileMapTraits<KeyType, KeyAccessorType>::SubMap> >
  class ExpireProfileMap:
    public ExpireProfileMapBase<
      KeyType,
      typename ExpireProfileMapTraits<KeyType, KeyAccessorType>::SubMap,
      FileOpenStrategyType>
  {
  public:
    typedef KeyType KeyTypeT;
    typedef KeyAccessorType KeyAccessorTypeT;
    typedef typename ExpireProfileMapTraits<
      KeyType, KeyAccessorType>::SubMap SubMap;

    typedef ExpireProfileMapBase<
      KeyType,
      SubMap,
      FileOpenStrategyType>
      ExpireProfileMapBaseClass;

    typedef typename ExpireProfileMapBaseClass::Exception Exception;

  public:
    ExpireProfileMap(
      const char* base_path,
      const char* prefix,
      const Generics::Time& extend_time_period,
      FileOpenStrategyType file_open_strategy = FileOpenStrategyType())
      /*throw(Exception)*/
      : ExpireProfileMapBaseClass(
         base_path,
         prefix,
         extend_time_period,
         file_open_strategy)
    {}

  protected:
    virtual
    ~ExpireProfileMap() noexcept
    {
    }
  };

  template<typename KeyType, typename KeyAccessorType>
  struct PackedExpireProfileMapTraits
  {
    typedef HashIndexProfileMap<
      KeyType,
      PlainStorage::FragmentBlockIndex,
      DefaultMapTraits<
        KeyType,
        KeyAccessorType,
        PlainStorage::FragmentBlockIndex,
        PlainStorage::FragmentBlockIndexSerializer> >
      SubMap;
  };

  /**
   * PackedExpireProfileMap expiry chunks container on
   * PlainStorage::HashMap containers
   */
  template<
    typename KeyType,
    typename KeyAccessorType,
    typename FileOpenStrategyType = DefaultFileOpenStrategy<
      typename PackedExpireProfileMapTraits<KeyType, KeyAccessorType>::SubMap> >
  class PackedExpireProfileMap:
    public ExpireProfileMapBase<
      KeyType,
      typename PackedExpireProfileMapTraits<KeyType, KeyAccessorType>::SubMap,
      FileOpenStrategyType>
  {
  public:
    typedef KeyType KeyTypeT;
    typedef KeyAccessorType KeyAccessorTypeT;
    typedef typename PackedExpireProfileMapTraits<
      KeyType, KeyAccessorType>::SubMap SubMap;

    typedef ExpireProfileMapBase<
      KeyType,
      SubMap,
      FileOpenStrategyType>
      ExpireProfileMapBaseClass;

  public:
    PackedExpireProfileMap(
      const char* base_path,
      const char* prefix,
      const Generics::Time& extend_time_period,
      FileOpenStrategyType file_open_strategy = FileOpenStrategyType())
      /*throw(typename ExpireProfileMapBaseClass::Exception)*/
      : ExpireProfileMapBaseClass(
          base_path,
          prefix,
          extend_time_period,
          file_open_strategy)
    {}

  protected:
    virtual
    ~PackedExpireProfileMap() noexcept
    {
    }
  };
}
}

#include "ExpireProfileMap.tpp"

#endif
