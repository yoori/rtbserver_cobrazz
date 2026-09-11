#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

#include <eh/Exception.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Generics/MemBuf.hpp>
#include <Generics/Time.hpp>
#include <Logger/Logger.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>

#include <Commons/Algs.hpp>
#include <Commons/Coro/StartableAwaitable.hpp>
#include <ProfilingCommons/PlainStorageAdapters.hpp>
#include <ProfilingCommons/ProfileMap/ProfileMapFactory.hpp>
#include <ProfilingCommons/ProfileMap/RocksDBProfileMapProcessor.hpp>

namespace AdServer::RequestInfoSvcs
{
  class UserNavigationContainer final :
    public Generics::RefCountableCompositeActiveObject
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    struct RequestInfo
    {
      AdServer::Commons::UserId user_id;
      Generics::Time time;
      std::vector<std::string_view> urls;
    };

    UserNavigationContainer(
      Logging::Logger* logger,
      unsigned long common_chunks_number,
      const AdServer::ProfilingCommons::ProfileMapFactory::ChunkPathMap& chunk_folders,
      const char* file_prefix,
      const AdServer::ProfilingCommons::LevelMapTraits& user_level_map_traits,
      std::size_t user_navigations_limit,
      unsigned long navigation_period_days,
      std::shared_ptr<AdServer::ProfilingCommons::RocksDBProfileMapProcessor>
        rocksdb_processor = {});

    AdServer::Commons::Awaitable<Generics::ConstSmartMemBuf_var>
    co_get_profile(
      const AdServer::Commons::UserId& user_id,
      std::optional<std::uint32_t> date = std::nullopt);

    AdServer::Commons::StartableAwaitable<void>
    co_process_request(const RequestInfo& request_info);

    void
    clear_expired();

    unsigned long profile_size() const noexcept;

  private:
    ~UserNavigationContainer() noexcept override = default;

    AdServer::Commons::Awaitable<void>
    co_process_request_trans_(const RequestInfo& request_info);

    struct UserNavigationKey
    {
      static constexpr std::uint32_t LEGACY_PERIOD_ID = std::numeric_limits<std::uint32_t>::max();

      UserNavigationKey() = default;

      UserNavigationKey(const AdServer::Commons::UserId& user_id_val, std::uint32_t period_id_val)
        noexcept
        : user_id(user_id_val),
          period_id(period_id_val)
      {}

      bool operator<(const UserNavigationKey& right) const noexcept
      {
        if (user_id != right.user_id)
        {
          return user_id < right.user_id;
        }

        return period_id < right.period_id;
      }

      unsigned long hash() const noexcept
      {
        return AdServer::Commons::uuid_distribution_hash(user_id);
      }

      AdServer::Commons::UserId user_id;
      std::uint32_t period_id = LEGACY_PERIOD_ID;
    };

    struct UserNavigationKeyAccessor
    {
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

      static unsigned int size(const UserNavigationKey& key) noexcept;

      static void load(const void* buf, unsigned int size, UserNavigationKey& key);

      static void save(const UserNavigationKey& key, void* buf, unsigned int size);
    };

    struct UserNavigationKeyHash
    {
      unsigned long operator()(const UserNavigationKey& key) const noexcept
      {
        return key.hash();
      }
    };

    using UserNavigationMap = AdServer::ProfilingCommons::ChunkedProfileMap<
      UserNavigationKey,
      AdServer::ProfilingCommons::TransactionProfileMap<UserNavigationKey>,
      UserNavigationKeyHash>;

    using UserNavigationMap_var = ReferenceCounting::SmartPtr<UserNavigationMap>;

    AdServer::Commons::StartableAwaitable<void>
    co_get_profile_part_(UserNavigationKey key, Generics::ConstSmartMemBuf_var& profile);

    AdServer::Commons::StartableAwaitable<void>
    co_get_legacy_profile_part_(UserNavigationKey key, Generics::ConstSmartMemBuf_var& profile);

    AdServer::Commons::Awaitable<void>
    co_migrate_legacy_profile_(const AdServer::Commons::UserId& user_id);

    std::uint32_t navigation_period_id_(std::uint32_t date) const noexcept;

    Logging::Logger_var logger_;
    Generics::Time expire_time_;
    const std::size_t user_navigations_limit_;
    const std::uint64_t navigation_period_seconds_;
    UserNavigationMap_var user_map_;
  };

  using UserNavigationContainer_var = ReferenceCounting::SmartPtr<UserNavigationContainer>;
}
