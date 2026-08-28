#pragma once

#include <memory>
#include <string>
#include <utility>
#include <map>

#include <Generics/CompositeActiveObject.hpp>
#include <ProfilingCommons/ProfileMap/LevelMapTraits.hpp>
#include <ProfilingCommons/ProfileMap/TransactionProfileMap.hpp>
#include <ProfilingCommons/ProfileMap/ChunkedExpireProfileMap.hpp>
#include <ProfilingCommons/ProfileMap/RocksDBBatchingProfileMap.hpp>
#include <ProfilingCommons/ProfileMap/RocksDBProfileMapProcessor.hpp>

namespace AdServer::ProfilingCommons
{
  template<typename KeyAccessorType>
  struct KeyAccessorStringAdapter
  {
    template<typename KeyType>
    std::string
    operator()(const KeyType& key) const
    {
      const unsigned long size = KeyAccessorType::size(key);
      std::string result(size, '\0');
      if (size)
      {
        KeyAccessorType::save(key, result.data(), size);
      }
      return result;
    }

    template<typename KeyType>
    KeyType
    key_from_string(const std::string& value) const
    {
      KeyType key;
      KeyAccessorType::load(value.data(), value.size(), key);
      return key;
    }
  };

  struct ProfileMapFactory
  {
    struct ProfileMapTraits
    {
      explicit ProfileMapTraits(const Generics::Time& expire_time_val) noexcept
        : expire_time(expire_time_val)
      {}

      Generics::Time expire_time;
    };

    typedef std::map<unsigned long, std::string> ChunkPathMap;

    static void
    fetch_chunk_folders(
      ChunkPathMap& chunks,
      const char* chunks_root,
      const char* chunks_prefix = "Chunk")
      /*throw(eh::Exception)*/;

    template<typename KeyType, typename KeyAdapterType>
    static
    std::pair<
      ReferenceCounting::SmartPtr<TransactionProfileMap<KeyType>>,
      Generics::ActiveObject_var>
    open_rocksdb_map(
      const String::SubString& rocksdb_path,
      const ProfileMapTraits& profile_map_traits,
      unsigned long max_waiters = 0,
      bool disable_wal = false,
      unsigned long workers_count = 2,
      std::shared_ptr<RocksDBProfileMapProcessor> processor = {})
      /*throw(eh::Exception)*/
    {
      using RocksDBMap = RocksDBBatchingProfileMap<KeyType, KeyAdapterType>;
      using TransactionMap = TransactionProfileMap<KeyType>;

      ReferenceCounting::SmartPtr<RocksDBMap> rocksdb_map;
      if (processor)
      {
        rocksdb_map = new RocksDBMap(
          std::move(processor),
          rocksdb_path,
          profile_map_traits.expire_time,
          128,
          Generics::Time::ZERO,
          disable_wal);
      }
      else
      {
        rocksdb_map = new RocksDBMap(
          rocksdb_path,
          profile_map_traits.expire_time,
          workers_count,
          128,
          Generics::Time::ZERO,
          disable_wal);
      }

      ReferenceCounting::SmartPtr<TransactionMap> profile_map =
        new TransactionMap(rocksdb_map.in(), max_waiters);
      Generics::ActiveObject_var active_object = rocksdb_map.retn();

      return std::make_pair(std::move(profile_map), std::move(active_object));
    }

    template<typename KeyType,
      typename KeyAccessorType,
      typename KeyHashType>
    static
    std::pair<
      ReferenceCounting::SmartPtr<
        AdServer::ProfilingCommons::ChunkedProfileMap<
          KeyType, AdServer::ProfilingCommons::TransactionProfileMap<KeyType>, KeyHashType> >,
      Generics::ActiveObject_var>
    open_rocksdb_chunked_map(
      unsigned long common_chunks_number,
      const ChunkPathMap& chunk_folders,
      const char* chunk_prefix,
      const ProfileMapTraits& profile_map_traits,
      KeyHashType key_hash,
      unsigned long max_waiters = 0,
      bool disable_wal = false,
      const char* rocksdb_path_suffix = ".rocksdb",
      unsigned long workers_count = 2,
      std::shared_ptr<RocksDBProfileMapProcessor> processor = {})
      /*throw(eh::Exception)*/
    {
      using ProfileMapType = ChunkedProfileMap<
        KeyType,
        AdServer::ProfilingCommons::TransactionProfileMap<KeyType>,
        KeyHashType>;

      typename ProfileMapType::ChunkIdToProfileMap chunks;
      Generics::CompositeActiveObject_var composite_active_object =
        new Generics::RefCountableCompositeActiveObject(false, false);

      if (!processor)
      {
        processor = std::make_shared<RocksDBProfileMapProcessor>(workers_count);
        composite_active_object->add_child_object(processor);
      }

      for (ChunkPathMap::const_iterator chunk_folder_it = chunk_folders.begin();
          chunk_folder_it != chunk_folders.end(); ++chunk_folder_it)
      {
        std::string rocksdb_path = chunk_folder_it->second + "/" + chunk_prefix;
        if (rocksdb_path_suffix)
        {
          rocksdb_path += rocksdb_path_suffix;
        }

        auto profile_map = open_rocksdb_map<KeyType, KeyAccessorStringAdapter<KeyAccessorType>>(
          String::SubString(rocksdb_path),
          profile_map_traits,
          max_waiters,
          disable_wal,
          workers_count,
          processor);

        chunks.emplace(chunk_folder_it->first, profile_map.first);
        composite_active_object->add_child_object(profile_map.second);
      }

      Generics::ActiveObject_var active_object = composite_active_object;

      ReferenceCounting::SmartPtr<ProfileMapType> profile_map =
        new ProfileMapType(common_chunks_number, chunks, key_hash);
      return std::make_pair(profile_map, active_object);
    }

  };
}
