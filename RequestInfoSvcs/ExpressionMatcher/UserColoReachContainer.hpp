#pragma once

#include <eh/Exception.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>

#include <Logger/Logger.hpp>
#include <Generics/Time.hpp>
#include <Generics/MemBuf.hpp>
#include <Generics/MonoAllocator.hpp>

#include <Commons/Algs.hpp>
#include <Commons/Coro/StartableAwaitable.hpp>
#include <ProfilingCommons/ProfileMap/ProfileMapFactory.hpp>
#include <ProfilingCommons/ProfileMap/RocksDBProfileMapProcessor.hpp>
#include <ProfilingCommons/PlainStorageAdapters.hpp>

#include "ColoReachProcessor.hpp"

namespace AdServer::RequestInfoSvcs
{
  class UserColoReachContainer: public Generics::RefCountableCompositeActiveObject
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  public:
    struct RequestInfo
    {
      AdServer::Commons::UserId user_id;
      Generics::Time time;
      Generics::Time isp_time;
      unsigned long colo_id;
    };

    UserColoReachContainer(
      Logging::Logger* logger,
      ColoReachProcessor* colo_reach_processor,
      bool household,
      unsigned long common_chunks_number,
      const AdServer::ProfilingCommons::ProfileMapFactory::ChunkPathMap& chunk_folders,
      const char* file_prefix,
      const AdServer::ProfilingCommons::LevelMapTraits& user_level_map_traits,
      std::shared_ptr<AdServer::ProfilingCommons::RocksDBProfileMapProcessor>
        rocksdb_processor = {})
      /*throw(Exception)*/;

    AdServer::Commons::Awaitable<Generics::ConstSmartMemBuf_var>
    co_get_profile(const AdServer::Commons::UserId& user_id);

    AdServer::Commons::StartableAwaitable<void>
    co_process_request(const RequestInfo& request_info);

    void clear_expired() /*throw(Exception)*/;

  protected:
    using ColoReachInfoList = Generics::MonoList<ColoReachProcessor::ColoReachInfo>;

  protected:
    virtual ~UserColoReachContainer() noexcept
    {}

    AdServer::Commons::Awaitable<void>
    co_process_request_trans_(
      ColoReachInfoList& gmt_colo_reach_info_list,
      ColoReachInfoList& isp_colo_reach_info_list,
      const RequestInfo& request_info)
      /*throw(Exception)*/;

  private:
    using UserInfoMap = AdServer::ProfilingCommons::ChunkedProfileMap<
      AdServer::Commons::UserId,
      AdServer::ProfilingCommons::TransactionProfileMap<AdServer::Commons::UserId>,
      unsigned long (*)(const Generics::Uuid& uuid) >;

    using UserInfoMap_var = ReferenceCounting::SmartPtr<UserInfoMap>;

  private:
    Logging::Logger_var logger_;
    ColoReachProcessor_var colo_reach_processor_;
    const bool HOUSEHOLD_;

    Generics::Time expire_time_;
    UserInfoMap_var user_map_;
  };

  using UserColoReachContainer_var = ReferenceCounting::SmartPtr<UserColoReachContainer>;
}
