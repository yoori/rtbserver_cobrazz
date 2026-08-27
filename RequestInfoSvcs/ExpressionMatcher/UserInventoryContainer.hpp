#pragma once

#include <string>
#include <list>

#include <eh/Exception.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <ReferenceCounting/PtrHolder.hpp>

#include <Logger/Logger.hpp>
#include <Generics/Time.hpp>
#include <Generics/MemBuf.hpp>
#include <Generics/MonoAllocator.hpp>

#include <Commons/Algs.hpp>
#include <Commons/Coro/StartableAwaitable.hpp>
#include <Commons/Containers.hpp>
#include <ProfilingCommons/ProfileMap/ProfileMapFactory.hpp>
#include <ProfilingCommons/ProfileMap/RocksDBProfileMapProcessor.hpp>
#include <ProfilingCommons/PlainStorageAdapters.hpp>
#include <CampaignSvcs/CampaignCommons/ExpressionChannel.hpp>

#include "InventoryActionProcessor.hpp"
#include "ColoReachProcessor.hpp"

namespace AdServer::RequestInfoSvcs
{
  typedef AdServer::Commons::UserId UserId;
  typedef std::list<UserId> UserIdList;

  const Generics::Time DEFAULT_EXPIRE_TIME(30*24*60*60); // one month

  /**
   * UserInfoventoryInfoContainer
   * contains logic of requests processing
   */
  class UserInventoryInfoContainer: public Generics::RefCountableCompositeActiveObject
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    using MatchInfo = MatchRequestProcessor::MatchInfo;

    struct InventoryDailyMatchInfo
    {
      AdServer::Commons::UserId user_id;
      Generics::Time time;
      unsigned long colo_id;
      ChannelIdList triggered_expression_channels;

      void
      print(std::ostream& ostr, const char*) const noexcept;
    };

  public:
    UserInventoryInfoContainer(
      Logging::Logger* logger,
      const Generics::Time& days_to_keep,
      InventoryActionProcessor* inv_processor,
      ColoReachProcessor* colo_reach_processor,
      bool adrequest_anonymize,
      unsigned long common_chunks_number,
      const AdServer::ProfilingCommons::ProfileMapFactory::ChunkPathMap& chunk_folders,
      const char* invfile_prefix,
      const AdServer::ProfilingCommons::LevelMapTraits& user_level_map_traits,
      std::shared_ptr<AdServer::ProfilingCommons::RocksDBProfileMapProcessor>
        rocksdb_processor = {})
      /*throw(Exception)*/;

    AdServer::Commons::Awaitable<Generics::ConstSmartMemBuf_var>
    co_get_profile(const AdServer::Commons::UserId& user_id);

    void
    clear_expired_users() /*throw(Exception)*/;

    AdServer::Commons::StartableAwaitable<void>
    co_process_match_request(const MatchInfo& request_info);

    bool
    get_last_daily_processing_time(const AdServer::Commons::UserId& user_id, Generics::Time& time)
      /*throw(Exception)*/;

    void
    process_user(const InventoryDailyMatchInfo& inv_daily_match_info)
      /*throw(Exception)*/;

    void
    all_users(UserIdList& users) /*throw(Exception)*/;

  private:
    typedef Sync::Policy::PosixThread SyncPolicy;

    using UserInventoryInfoMap = AdServer::ProfilingCommons::ChunkedProfileMap<
      UserId,
      AdServer::ProfilingCommons::TransactionProfileMap<UserId>,
      unsigned long (*)(const Generics::Uuid& uuid) >;

    using UserInventoryInfoMap_var = ReferenceCounting::SmartPtr<UserInventoryInfoMap>;
    using ColoReachInfoList = Generics::MonoList<ColoReachProcessor::ColoReachInfo>;

  private:
    virtual
    ~UserInventoryInfoContainer() noexcept;

    AdServer::Commons::Awaitable<void>
    co_process_request_trans_(
      bool& delegate_processing,
      InventoryActionProcessor::InventoryInfo& inv_info,
      ColoReachInfoList& gmt_colo_reach_info_list,
      ColoReachInfoList& isp_colo_reach_info_list,
      const MatchInfo& request_info)
      /*throw(Exception)*/;

    void
    process_user_trans_(
      bool& delegate_processing,
      InventoryActionProcessor::InventoryUserInfo& inv_info,
      const InventoryDailyMatchInfo& inv_daily_match_info)
      /*throw(Exception)*/;

    static bool
    init_inv_info_(InventoryActionProcessor::InventoryInfo& inv_info, const MatchInfo& request_info)
      noexcept;

  private:
    Logging::Logger_var logger_;
    const Generics::Time days_to_keep_;
    const bool adrequest_anonymize_;
    Generics::Time expire_time_;

    UserInventoryInfoMap_var user_map_;
    InventoryActionProcessor_var inventory_processor_;
    ColoReachProcessor_var colo_reach_processor_;
  };

  using UserInventoryInfoContainer_var = ReferenceCounting::SmartPtr<UserInventoryInfoContainer>;

} /* AdServer::RequestInfoSvcs */

namespace AdServer::RequestInfoSvcs
{
  inline
  void
  UserInventoryInfoContainer::InventoryDailyMatchInfo::print(
    std::ostream& ostr, const char* offset) const
    noexcept
  {
    ostr << offset << "user_id: '" << user_id << "'" << std::endl <<
      offset << "time: " << time.get_gm_time() << std::endl << offset << "channels: ";
    Algs::print(ostr, triggered_expression_channels.begin(), triggered_expression_channels.end());
    ostr << std::endl;
  }
}
