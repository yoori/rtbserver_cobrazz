#pragma once

#include <string>

#include <Generics/ActiveObject.hpp>
#include <Generics/Time.hpp>
#include <Generics/Uuid.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <String/SubString.hpp>

#include <Commons/UserInfoManip.hpp>
#include <ProfilingCommons/ProfileMap/RocksDBBatchingProfileMap.hpp>
#include <ProfilingCommons/ProfileMap/TransactionProfileMap.hpp>

namespace AdServer::RequestInfoSvcs
{
  struct UuidKeyToString
  {
    template<typename KeyType>
    std::string
    operator()(const KeyType& key) const
    {
      return std::string(
        reinterpret_cast<const char*>(&*key.begin()),
        key.size());
    }

    template<typename KeyType>
    KeyType
    key_from_string(const std::string& value) const
    {
      return KeyType(Generics::Uuid(value.begin(), value.end()));
    }
  };

  using UserIdToString = UuidKeyToString;
  using RequestIdToString = UuidKeyToString;

  template<typename KeyType>
  struct RocksDBTransactionProfileMapHolder
  {
    using TransactionMap =
      ProfilingCommons::TransactionProfileMap<KeyType>;
    using TransactionMap_var =
      ReferenceCounting::SmartPtr<TransactionMap>;

    TransactionMap_var map;
    Generics::ActiveObject_var active_object;
  };

  template<typename KeyType, typename KeyAdapterType>
  RocksDBTransactionProfileMapHolder<KeyType>
  open_rocksdb_transaction_profile_map(
    const char* path,
    const Generics::Time& expire_time)
  {
    using RocksDBMap =
      ProfilingCommons::RocksDBBatchingProfileMap<KeyType, KeyAdapterType>;
    using TransactionMap = ProfilingCommons::TransactionProfileMap<KeyType>;

    ReferenceCounting::SmartPtr<RocksDBMap> rocksdb_map(
      new RocksDBMap(
        String::SubString(path),
        expire_time,
        2,
        128,
        Generics::Time::ZERO));

    RocksDBTransactionProfileMapHolder<KeyType> holder;
    holder.map = new TransactionMap(rocksdb_map.in());
    holder.active_object = rocksdb_map.in();
    return holder;
  }
}
