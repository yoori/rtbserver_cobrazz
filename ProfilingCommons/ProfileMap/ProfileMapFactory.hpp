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
      typedef ChunkedProfileMap<
        KeyType,
        AdServer::ProfilingCommons::TransactionProfileMap<KeyType>,
        KeyHashType> ProfileMapType;
      typedef RocksDBBatchingProfileMap<
        KeyType,
        KeyAccessorStringAdapter<KeyAccessorType> >
        RocksDBMap;

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
        ReferenceCounting::SmartPtr<RocksDBMap> rocksdb_map =
          new RocksDBMap(
            processor,
            String::SubString(rocksdb_path.c_str()),
            profile_map_traits.expire_time,
            128,
            Generics::Time::ZERO,
            disable_wal);

        ReferenceCounting::SmartPtr<
          AdServer::ProfilingCommons::TransactionProfileMap<KeyType> > base_map =
            new TransactionProfileMap<KeyType>(rocksdb_map.in(), max_waiters);

        composite_active_object->add_child_object(rocksdb_map.in());
        chunks.insert(std::make_pair(chunk_folder_it->first, base_map));
      }

      Generics::ActiveObject_var active_object = composite_active_object;

      ReferenceCounting::SmartPtr<ProfileMapType> profile_map =
        new ProfileMapType(common_chunks_number, chunks, key_hash);
      return std::make_pair(profile_map, active_object);
    }

  };
}
