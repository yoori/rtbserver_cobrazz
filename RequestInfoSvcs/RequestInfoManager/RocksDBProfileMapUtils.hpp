#pragma once

#include <string>

#include <Generics/Time.hpp>
#include <Generics/Uuid.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <String/SubString.hpp>

#include <Commons/UserInfoManip.hpp>
#include <ProfilingCommons/ProfileMap/RocksDBProfileMap.hpp>
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

  template<typename KeyType, typename KeyAdapterType>
  ReferenceCounting::SmartPtr<
    ProfilingCommons::TransactionProfileMap<KeyType>>
  open_rocksdb_transaction_profile_map(
    const char* path,
    const Generics::Time& expire_time)
  {
    using RocksDBMap =
      ProfilingCommons::RocksDBProfileMap<KeyType, KeyAdapterType>;
    using TransactionMap = ProfilingCommons::TransactionProfileMap<KeyType>;

    ReferenceCounting::SmartPtr<RocksDBMap> rocksdb_map(
      new RocksDBMap(String::SubString(path), expire_time));

    return ReferenceCounting::SmartPtr<TransactionMap>(
      new TransactionMap(rocksdb_map.in()));
  }
}
