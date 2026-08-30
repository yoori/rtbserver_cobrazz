#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>

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
      std::string_view url;
    };

    UserNavigationContainer(
      Logging::Logger* logger,
      unsigned long common_chunks_number,
      const AdServer::ProfilingCommons::ProfileMapFactory::ChunkPathMap& chunk_folders,
      const char* file_prefix,
      const AdServer::ProfilingCommons::LevelMapTraits& user_level_map_traits,
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

  private:
    ~UserNavigationContainer() noexcept override = default;

    AdServer::Commons::Awaitable<void>
    co_process_request_trans_(const RequestInfo& request_info);

    using UserNavigationMap = AdServer::ProfilingCommons::ChunkedProfileMap<
      AdServer::Commons::UserId,
      AdServer::ProfilingCommons::TransactionProfileMap<AdServer::Commons::UserId>,
      unsigned long (*)(const Generics::Uuid&)>;

    using UserNavigationMap_var = ReferenceCounting::SmartPtr<UserNavigationMap>;

    Logging::Logger_var logger_;
    Generics::Time expire_time_;
    UserNavigationMap_var user_map_;
  };

  using UserNavigationContainer_var = ReferenceCounting::SmartPtr<UserNavigationContainer>;
}
