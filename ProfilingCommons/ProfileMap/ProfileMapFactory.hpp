#ifndef PROFILEMAP_PROFILEMAPFACTORY_HPP
#define PROFILEMAP_PROFILEMAPFACTORY_HPP

#include <ProfilingCommons/PlainStorage3/LevelProfileMap.hpp>
#include <ProfilingCommons/ProfileMap/ExpireProfileMap.hpp>
#include <ProfilingCommons/ProfileMap/AdaptProfileMap.hpp>
#include <ProfilingCommons/ProfileMap/AsyncRocksDBProfileMap.hpp>
#include <ProfilingCommons/ProfileMap/RocksDBWhithLevelAlternativeProfileMap.hpp>
#include <ProfilingCommons/ProfileMap/RocksDBWhithAlternativeProfileMap.hpp>
#include <ProfilingCommons/ProfileMap/TransactionProfileMap.hpp>
#include <ProfilingCommons/ProfileMap/ChunkedExpireProfileMap.hpp>

namespace AdServer
{
namespace ProfilingCommons
{
  template<typename OptType>
  struct OptionalProfileAdapter
  {
    static const bool DEFINED = true;

    typedef OptType AdapterType;

    AdapterType adapter;
  };

  struct NullProfileAdapter
  {
    static const bool DEFINED = false;

    struct AdapterType
    {
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

      Generics::ConstSmartMemBuf_var
      operator()(const Generics::ConstSmartMemBuf*) /*throw(Exception)*/
      {
        assert(0);
        return Generics::ConstSmartMemBuf_var();
      }
    };

    AdapterType adapter;
  };

  struct ProfileMapFactory
  {
    typedef PlainStorage::LayerFactory<
      Sync::Policy::PosixThreadRW>::CacheT
      Cache;

    typedef ReferenceCounting::SmartPtr<Cache>
      Cache_var;

    typedef std::map<unsigned long, std::string> ChunkPathMap;

    static void
    fetch_chunk_folders(
      ChunkPathMap& chunks,
      const char* chunks_root,
      const char* chunks_prefix = "Chunk")
      /*throw(eh::Exception)*/;

    template<typename KeyType, typename KeyAccessorType>
    static
    typename ReferenceCounting::SmartPtr<
      AdServer::ProfilingCommons::TransactionProfileMap<KeyType> >
    open_transaction_expire_map(
      const char* root,
      const char* prefix,
      const Generics::Time& extend_time,
      Cache* cache = 0,
      unsigned long max_waiters = 0)
      /*throw(eh::Exception)*/
    {
      ReferenceCounting::SmartPtr<ProfileMap<KeyType> > base_map;

      if(cache)
      {
        typedef CacheFileOpenStrategy<
          typename ExpireProfileMapTraits<KeyType, KeyAccessorType>::SubMap>
          CacheFileOpenStrategyT;

        CacheFileOpenStrategyT file_open_strategy(cache);

        base_map = new ExpireProfileMap<KeyType,
          KeyAccessorType,
          CacheFileOpenStrategyT>(
            // (std::string(root) + "/" + prefix + "/").c_str(),
            root,
            prefix,
            extend_time,
            file_open_strategy);
      }
      else
      {
        base_map = new ExpireProfileMap<KeyType, KeyAccessorType>(
          // (std::string(root) + "/" + prefix + "/").c_str(),
          root,
          prefix,
          extend_time);
      }

      return new AdServer::ProfilingCommons::TransactionProfileMap<
        KeyType>(base_map, max_waiters);
    }

    template<typename Key,
      typename KeyAdapter>
    static ReferenceCounting::SmartPtr<
      AdServer::ProfilingCommons::TransactionProfileMap<Key>>
    open_transaction_rocksdb_map(
      Logging::Logger* logger,
      const std::shared_ptr<UServerUtils::Grpc::RocksDB::DataBaseManagerPool>& rocksdb_manager_pool,
      const std::string& rocksdb_path,
      const AdServer::ProfilingCommons::RocksDB::RocksDBParams& rocksdb_params,
      const unsigned long max_waiters,
      const KeyAdapter& key_adapter = KeyAdapter())
    {
      using RocksDBProfileMap = ProfilingCommons::RocksDB::RocksDBProfileMap<Key, KeyAdapter>;

      ReferenceCounting::SmartPtr<RocksDBProfileMap> base_map = new RocksDBProfileMap(
        logger,
        rocksdb_manager_pool,
        rocksdb_path,
        rocksdb_params,
        "profile_map",
        key_adapter);

      return new AdServer::ProfilingCommons::TransactionProfileMap<Key>(
        base_map,
        max_waiters);
    }

    template<typename KeyType,
      typename KeyAccessorType,
      typename ProfileAdapterType>
    static
    typename ReferenceCounting::SmartPtr<
      AdServer::ProfilingCommons::TransactionProfileMap<KeyType> >
    open_transaction_expire_map(
      const char* root,
      const char* prefix,
      const Generics::Time& extend_time,
      const ProfileAdapterType& profile_adapter = ProfileAdapterType(),
      Cache* cache = 0,
      unsigned long max_waiters = 0)
      /*throw(eh::Exception)*/
    {
      ReferenceCounting::SmartPtr<ProfileMap<KeyType> > ex_map;

      if(cache)
      {
        typedef CacheFileOpenStrategy<
          typename ExpireProfileMapTraits<KeyType, KeyAccessorType>::SubMap>
          CacheFileOpenStrategyT;

        CacheFileOpenStrategyT file_open_strategy(cache);

        ex_map = new ExpireProfileMap<KeyType,
          KeyAccessorType,
          CacheFileOpenStrategyT>(
            // (std::string(root) + "/" + prefix + "/").c_str(),
            root,
            prefix,
            extend_time,
            file_open_strategy);
      }
      else
      {
        ex_map = new ExpireProfileMap<KeyType, KeyAccessorType>(
          // (std::string(root) + "/" + prefix + "/").c_str(),
          root,
          prefix,
          extend_time);
      }

      ReferenceCounting::SmartPtr<ProfileMap<KeyType> > base_map(
        new AdaptProfileMap<KeyType, ProfileAdapterType>(
          ex_map, profile_adapter));

      return new AdServer::ProfilingCommons::TransactionProfileMap<
        KeyType>(base_map, max_waiters);
    }

    template<typename Key,
      typename KeyAccessor,
      typename KeyAdapter,
      typename ProfileAdapter>
    static
    ReferenceCounting::SmartPtr<AdServer::ProfilingCommons::TransactionProfileMap<Key>>
    open_transaction_expire_map(
      Logging::Logger* logger,
      const char* root,
      const char* prefix,
      const bool is_level_enable,
      const Generics::Time& extend_time,
      const bool is_rocksdb_enable,
      const std::string& rocksdb_path,
      const std::shared_ptr<UServerUtils::Grpc::RocksDB::DataBaseManagerPool>& rocksdb_manager_pool,
      const AdServer::ProfilingCommons::RocksDB::RocksDBParams& rocksdb_params,
      const KeyAdapter& key_adapter = KeyAdapter(),
      const ProfileAdapter& profile_adapter = ProfileAdapter(),
      Cache* cache = 0,
      unsigned long max_waiters = 0)
    {
      ReferenceCounting::SmartPtr<ProfileMap<Key>> ex_map;
      if (is_level_enable)
      {
        if (cache)
        {
          typedef CacheFileOpenStrategy<
            typename ExpireProfileMapTraits<Key, KeyAccessor>::SubMap>
            CacheFileOpenStrategyT;

          CacheFileOpenStrategyT file_open_strategy(cache);

          ex_map = new ExpireProfileMap<Key,
            KeyAccessor,
            CacheFileOpenStrategyT>(
            // (std::string(root) + "/" + prefix + "/").c_str(),
            root,
            prefix,
            extend_time,
            file_open_strategy);
        }
        else
        {
          ex_map = new ExpireProfileMap<Key, KeyAccessor>(
            // (std::string(root) + "/" + prefix + "/").c_str(),
            root,
            prefix,
            extend_time);
        }
      }

      ReferenceCounting::SmartPtr<ProfileMap<Key>> rocksdb_whith_alternative_map =
        new AdServer::ProfilingCommons::RocksDBWhithAlternativeProfileMap(
          logger,
          rocksdb_manager_pool,
          is_rocksdb_enable,
          rocksdb_path,
          rocksdb_params,
          "profile_map",
          key_adapter,
          ex_map.in());

      ReferenceCounting::SmartPtr<ProfileMap<Key> > base_map(
        new AdaptProfileMap<Key, ProfileAdapter>(
          rocksdb_whith_alternative_map, profile_adapter));

      return new AdServer::ProfilingCommons::TransactionProfileMap<
        Key>(base_map, max_waiters);
    }

    template<typename Key,
      typename KeyAdapter,
      typename ProfileAdapter>
    static ReferenceCounting::SmartPtr<
      AdServer::ProfilingCommons::TransactionProfileMap<Key>>
    open_transaction_rocksdb_map(
      Logging::Logger* logger,
      const std::shared_ptr<UServerUtils::Grpc::RocksDB::DataBaseManagerPool>& rocksdb_manager_pool,
      const std::string& rocksdb_path,
      const AdServer::ProfilingCommons::RocksDB::RocksDBParams& rocksdb_params,
      const unsigned long max_waiters,
      const KeyAdapter& key_adapter = KeyAdapter(),
      const ProfileAdapter& profile_adapter = ProfileAdapter())
    {
      using RocksDBProfileMap = ProfilingCommons::RocksDB::RocksDBProfileMap<Key, KeyAdapter>;

      ReferenceCounting::SmartPtr<RocksDBProfileMap> ex_map = new RocksDBProfileMap(
        logger,
        rocksdb_manager_pool,
        rocksdb_path,
        rocksdb_params,
        "profile_map",
        key_adapter);

      ReferenceCounting::SmartPtr<ProfileMap<Key>> base_map(
        new AdaptProfileMap<Key, ProfileAdapter>(
          ex_map, profile_adapter));

      return new AdServer::ProfilingCommons::TransactionProfileMap<Key>(
        base_map,
        max_waiters);
    }

    template<typename KeyType, typename KeyAccessorType>
    static
    typename ReferenceCounting::SmartPtr<
      AdServer::ProfilingCommons::TransactionProfileMap<KeyType> >
    open_transaction_packed_expire_map(
      const char* root,
      const char* prefix,
      const Generics::Time& extend_time,
      Cache* cache = 0)
      /*throw(eh::Exception)*/
    {
      ReferenceCounting::SmartPtr<ProfileMap<KeyType> > base_map;

      if(cache)
      {
        typedef CacheFileOpenStrategy<
          typename PackedExpireProfileMapTraits<KeyType, KeyAccessorType>::SubMap>
          CacheFileOpenStrategyT;

        CacheFileOpenStrategyT file_open_strategy(cache);

        base_map = new PackedExpireProfileMap<KeyType,
          KeyAccessorType,
          CacheFileOpenStrategyT>(
            // (std::string(root) + "/" + prefix + "/").c_str(),
            root,
            prefix,
            extend_time,
            file_open_strategy);
      }
      else
      {
        base_map = new PackedExpireProfileMap<KeyType,
          KeyAccessorType>(
            // (std::string(root) + "/" + prefix + "/").c_str(),
            root,
            prefix,
            extend_time);
      }

      return new AdServer::ProfilingCommons::TransactionProfileMap<
        KeyType>(base_map, 0, true);
    }

    template<typename KeyType,
      typename KeyAccessorType,
      typename ProfileAdapterType>
    static
    typename ReferenceCounting::SmartPtr<
      AdServer::ProfilingCommons::TransactionProfileMap<KeyType> >
    open_transaction_packed_expire_map(
      const char* root,
      const char* prefix,
      const Generics::Time& extend_time,
      Cache* cache = 0)
      /*throw(eh::Exception)*/
    {
      ReferenceCounting::SmartPtr<ProfileMap<KeyType> > ex_map;

      if(cache)
      {
        typedef CacheFileOpenStrategy<
          typename PackedExpireProfileMapTraits<KeyType, KeyAccessorType>::SubMap>
          CacheFileOpenStrategyT;

        CacheFileOpenStrategyT file_open_strategy(cache);

        ex_map = new PackedExpireProfileMap<KeyType,
          KeyAccessorType,
          CacheFileOpenStrategyT>(
            // (std::string(root) + "/" + prefix + "/").c_str(),
            root,
            prefix,
            extend_time,
            file_open_strategy);
      }
      else
      {
        ex_map = new PackedExpireProfileMap<KeyType, KeyAccessorType>(
          // (std::string(root) + "/" + prefix + "/").c_str(),
          root,
          prefix,
          extend_time);
      }

      ReferenceCounting::SmartPtr<ProfileMap<KeyType> > base_map(
        new AdaptProfileMap<KeyType, ProfileAdapterType>(ex_map));

      return new AdServer::ProfilingCommons::TransactionProfileMap<
        KeyType>(base_map, 0, true);
    }

    template<typename KeyType,
      typename KeyAccessorType,
      typename ProfileAdapterType>
    static
    typename ReferenceCounting::SmartPtr<
      AdServer::ProfilingCommons::TransactionProfileMap<KeyType> >
    open_adapt_transaction_level_map(
      Generics::ActiveObject_var& active_object,
      Generics::ActiveObjectCallback* callback,
      const char* root,
      const char* prefix,
      const LevelMapTraits& level_map_traits,
      const ProfileAdapterType& profile_adapter = ProfileAdapterType(),
      unsigned long max_waiters = 0,
      LoadingProgressCallbackBase* progress_checker_parent = nullptr)
      /*throw(eh::Exception)*/
    {
      ReferenceCounting::SmartPtr<LevelProfileMap<KeyType, KeyAccessorType> > ex_map =
        new LevelProfileMap<KeyType, KeyAccessorType>(
          callback,
          root,
          prefix,
          level_map_traits,
          progress_checker_parent);

      active_object = ex_map;

      ReferenceCounting::SmartPtr<ProfileMap<KeyType> > base_map(
        new AdaptProfileMap<KeyType, ProfileAdapterType>(
          ex_map, profile_adapter));

      return new AdServer::ProfilingCommons::TransactionProfileMap<
        KeyType>(base_map, max_waiters);
    }

    template<
      typename Key,
      typename KeyAccessor,
      typename ProfileAdapter,
      typename KeyAdapter>
    static
    ReferenceCounting::SmartPtr<
      AdServer::ProfilingCommons::TransactionProfileMap<Key>>
    open_adapt_transaction_profile_map(
      Generics::ActiveObject_var& active_object,
      Generics::ActiveObjectCallback* callback,
      Logging::Logger* logger,
      const std::shared_ptr<UServerUtils::Grpc::RocksDB::DataBaseManagerPool>& rocksdb_manager_pool,
      const bool is_rocksdb_enable,
      const std::string& rocksdb_path,
      const AdServer::ProfilingCommons::RocksDB::RocksDBParams& rocksdb_params,
      const bool is_level_enable,
      const char* level_root,
      const char* level_prefix,
      const LevelMapTraits& level_map_traits,
      const KeyAdapter& key_adapter = KeyAdapter(),
      const ProfileAdapter& profile_adapter = ProfileAdapter(),
      unsigned long max_waiters = 0,
      LoadingProgressCallbackBase* progress_checker_parent = nullptr)
    {
      ReferenceCounting::SmartPtr<RocksDBWhithLevelAlternativeProfileMap<Key, KeyAdapter, KeyAccessor>> ex_map =
        new RocksDBWhithLevelAlternativeProfileMap<Key, KeyAdapter, KeyAccessor>(
          logger,
          callback,
          rocksdb_manager_pool,
          is_rocksdb_enable,
          rocksdb_path,
          rocksdb_params,
          "profile",
          key_adapter,
          is_level_enable,
          level_root,
          level_prefix,
          level_map_traits,
          progress_checker_parent);

      active_object = ex_map;

      ReferenceCounting::SmartPtr<ProfileMap<Key>> base_map(
        new AdaptProfileMap<Key, ProfileAdapter>(
          ex_map.in(), profile_adapter));

      return new AdServer::ProfilingCommons::TransactionProfileMap<Key>(
        base_map,
        max_waiters);
    }

    template<typename KeyType,
      typename KeyAccessorType>
    static
    typename ReferenceCounting::SmartPtr<
      AdServer::ProfilingCommons::TransactionProfileMap<KeyType> >
    open_transaction_level_map(
      Generics::ActiveObject_var& active_object,
      Generics::ActiveObjectCallback* callback,
      const char* root,
      const char* prefix,
      const LevelMapTraits& level_map_traits,
      unsigned long max_waiters = 0,
      LoadingProgressCallbackBase_var progress_checker_parent = nullptr)
      /*throw(eh::Exception)*/
    {
      ReferenceCounting::SmartPtr<LevelProfileMap<KeyType, KeyAccessorType> > ex_map =
        new LevelProfileMap<KeyType, KeyAccessorType>(
          callback,
          root,
          prefix,
          level_map_traits,
          progress_checker_parent);

      active_object = ex_map;

      return new AdServer::ProfilingCommons::TransactionProfileMap<
        KeyType>(ex_map, max_waiters);
    }

    template<
      typename Key,
      typename KeyAccessor,
      typename KeyAdapter>
    static
    typename ReferenceCounting::SmartPtr<
      AdServer::ProfilingCommons::TransactionProfileMap<Key>>
    open_transaction_profile_map(
      Generics::ActiveObject_var& active_object,
      Generics::ActiveObjectCallback* callback,
      Logging::Logger* logger,
      const std::shared_ptr<UServerUtils::Grpc::RocksDB::DataBaseManagerPool>& rocksdb_manager_pool,
      const bool is_rocksdb_enable,
      const std::string& rocksdb_path,
      const AdServer::ProfilingCommons::RocksDB::RocksDBParams& rocksdb_params,
      const bool is_level_enable,
      const char* level_root,
      const char* level_prefix,
      const LevelMapTraits& level_map_traits,
      const KeyAdapter& key_adapter = KeyAdapter(),
      unsigned long max_waiters = 0,
      LoadingProgressCallbackBase* progress_checker_parent = nullptr)
    {
      ReferenceCounting::SmartPtr<
        RocksDBWhithLevelAlternativeProfileMap<Key, KeyAdapter, KeyAccessor>> ex_map =
          new RocksDBWhithLevelAlternativeProfileMap<Key, KeyAdapter, KeyAccessor>(
            logger,
            callback,
            rocksdb_manager_pool,
            is_rocksdb_enable,
            rocksdb_path,
            rocksdb_params,
            "profile",
            key_adapter,
            is_level_enable,
            level_root,
            level_prefix,
            level_map_traits,
            progress_checker_parent);

      active_object = ex_map;

      return new AdServer::ProfilingCommons::TransactionProfileMap<Key>(
        ex_map,
        max_waiters);
    }

    template<typename KeyType,
      typename KeyAccessorType,
      typename KeyHashType,
      typename AdapterOptionalType>
    static
    ReferenceCounting::SmartPtr<
      AdServer::ProfilingCommons::ChunkedProfileMap<
        KeyType, AdServer::ProfilingCommons::TransactionProfileMap<KeyType>, KeyHashType> >
    open_chunked_map(
      unsigned long common_chunks_number,
      const ChunkPathMap& chunk_folders,
      const char* chunk_prefix,
      const AdServer::ProfilingCommons::LevelMapTraits& user_level_map_traits,
      Generics::CompositeActiveObject& parent_container,
      Generics::ActiveObjectCallback_var callback,
      KeyHashType key_hash,
      LoadingProgressCallbackBase_var progress_checker_parent = nullptr,
      unsigned long max_waiters = 0,
      const AdapterOptionalType& optional_adapter = AdapterOptionalType())
      /*throw(eh::Exception)*/
    {
      typedef ChunkedProfileMap<
        KeyType,
        AdServer::ProfilingCommons::TransactionProfileMap<KeyType>,
        KeyHashType> ProfileMapType;

      typename ProfileMapType::ChunkIdToProfileMap chunks;

      for(ChunkPathMap::const_iterator chunk_folder_it =
            chunk_folders.begin();
          chunk_folder_it != chunk_folders.end(); ++chunk_folder_it)
      {
        ReferenceCounting::SmartPtr<
          AdServer::ProfilingCommons::TransactionProfileMap<KeyType> > base_map;

        Generics::ActiveObject_var active_object;

        if(AdapterOptionalType::DEFINED)
        {
          base_map = AdServer::ProfilingCommons::ProfileMapFactory::
            open_adapt_transaction_level_map<
              KeyType,
              KeyAccessorType,
              typename AdapterOptionalType::AdapterType>(
                active_object,
                callback,
                chunk_folder_it->second.c_str(),
                chunk_prefix,
                user_level_map_traits,
                optional_adapter.adapter,
                max_waiters,
                progress_checker_parent);
        }
        else
        {
          base_map = AdServer::ProfilingCommons::ProfileMapFactory::
            open_transaction_level_map<
              KeyType,
              KeyAccessorType>(
                active_object,
                callback,
                chunk_folder_it->second.c_str(),
                chunk_prefix,
                user_level_map_traits,
                max_waiters,
                progress_checker_parent);
        }

        parent_container.add_child_object(active_object);
        chunks.insert(std::make_pair(chunk_folder_it->first, base_map));
      }

      return new ProfileMapType(
        common_chunks_number,
        chunks,
        key_hash);
    }

    template<
      typename Key,
      typename KeyAccessor,
      typename KeyHash,
      typename KeyAdapter,
      typename AdapterOptional>
    static ReferenceCounting::SmartPtr<
      AdServer::ProfilingCommons::ChunkedProfileMap<
        Key, AdServer::ProfilingCommons::TransactionProfileMap<Key>, KeyHash>>
    open_chunked_map(
      Generics::CompositeActiveObject& parent_container,
      Generics::ActiveObjectCallback_var callback,
      Logging::Logger* logger,
      const std::shared_ptr<UServerUtils::Grpc::RocksDB::DataBaseManagerPool>& rocksdb_manager_pool,
      unsigned long common_chunks_number,
      const ChunkPathMap& chunk_folders,
      const char* chunk_prefix,
      const bool is_rocksdb_enable,
      const AdServer::ProfilingCommons::RocksDB::RocksDBParams& rocksdb_params,
      const bool is_level_enable,
      const LevelMapTraits& level_map_traits,
      const KeyHash& key_hash,
      unsigned long max_waiters = 0,
      const KeyAdapter& key_adapter = KeyAdapter(),
      const AdapterOptional& optional_adapter = AdapterOptional(),
      LoadingProgressCallbackBase* progress_checker_parent = nullptr)
    {
      using ProfileMapType = ChunkedProfileMap<
        Key,
        AdServer::ProfilingCommons::TransactionProfileMap<Key>,
        KeyHash>;
      using ChunkIdToProfileMap = ProfileMapType::ChunkIdToProfileMap;
      ChunkIdToProfileMap chunks;

      for (const auto& [chunk_id, chunk_directory] : chunk_folders)
      {
        ReferenceCounting::SmartPtr<
          AdServer::ProfilingCommons::TransactionProfileMap<Key>> base_map;

        const std::string rocksdb_path = chunk_directory + "/RocksDB_" + chunk_prefix;
        Generics::ActiveObject_var active_object;

        if (AdapterOptional::DEFINED)
        {
          base_map = open_adapt_transaction_profile_map<Key, KeyAccessor>(
            active_object,
            callback.in(),
            logger,
            rocksdb_manager_pool,
            is_rocksdb_enable,
            rocksdb_path,
            rocksdb_params,
            is_level_enable,
            chunk_directory.c_str(),
            chunk_prefix,
            level_map_traits,
            key_adapter,
            optional_adapter.adapter,
            max_waiters,
            progress_checker_parent);
        }
        else
        {
          base_map = open_transaction_profile_map<Key, KeyAccessor>(
            active_object,
            callback.in(),
            logger,
            rocksdb_manager_pool,
            is_rocksdb_enable,
            rocksdb_path,
            rocksdb_params,
            is_level_enable,
            chunk_directory.c_str(),
            chunk_prefix,
            level_map_traits,
            key_adapter,
            max_waiters,
            progress_checker_parent);
        }

        parent_container.add_child_object(active_object);
        chunks.try_emplace(chunk_id, base_map);
      }

      return new ProfileMapType(
        common_chunks_number,
        chunks,
        key_hash);
    }
  };
} // namespace ProfilingCommons
} // namespace AdServer

#endif /*PROFILEMAP_PROFILEMAPFACTORY_HPP*/
