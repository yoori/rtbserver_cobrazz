#ifndef _EXPRESSION_MATCHER_USER_INVENTORY_CONTAINER_HPP_
#define _EXPRESSION_MATCHER_USER_INVENTORY_CONTAINER_HPP_

#include <string>
#include <list>

#include <eh/Exception.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <ReferenceCounting/PtrHolder.hpp>

#include <Logger/Logger.hpp>
#include <Generics/Time.hpp>
#include <Generics/MemBuf.hpp>

#include <Commons/Algs.hpp>
#include <Commons/Containers.hpp>
#include <ProfilingCommons/ProfileMap/ProfileMapFactory.hpp>
#include <ProfilingCommons/PlainStorageAdapters.hpp>
#include <CampaignSvcs/CampaignCommons/ExpressionChannel.hpp>

#include "InventoryActionProcessor.hpp"
#include "ColoReachProcessor.hpp"

namespace AdServer
{
  namespace RequestInfoSvcs
  {
    typedef AdServer::Commons::UserId UserId;
    typedef std::list<UserId> UserIdList;

    const Generics::Time DEFAULT_EXPIRE_TIME(30*24*60*60); // one month

    /**
     * UserInfoventoryInfoContainer
     * contains logic of requests processing
     */
    class UserInventoryInfoContainer:
      public MatchRequestProcessor,
      public Generics::CompositeActiveObject,
      public ReferenceCounting::AtomicImpl
    {
    public:
      using RocksdbManagerPool = UServerUtils::Grpc::RocksDB::DataBaseManagerPool;
      using RocksdbManagerPoolPtr = std::shared_ptr<RocksdbManagerPool>;
      using RocksDBParams = AdServer::ProfilingCommons::RocksDB::RocksDBParams;

      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

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
      explicit UserInventoryInfoContainer(
        Logging::Logger* logger,
        const Generics::Time& days_to_keep,
        InventoryActionProcessor* inv_processor,
        ColoReachProcessor* colo_reach_processor,
        bool adrequest_anonymize,
        unsigned long common_chunks_number,
        const AdServer::ProfilingCommons::ProfileMapFactory::ChunkPathMap& chunk_folders,
        const char* invfile_prefix,
        ProfilingCommons::ProfileMapFactory::Cache* cache,
        const RocksDBParams& add_rocksdb_params)
        /*throw(Exception)*/;

      Generics::ConstSmartMemBuf_var
      get_profile(const AdServer::Commons::UserId& user_id) /*throw(Exception)*/;

      void
      clear_expired_users() /*throw(Exception)*/;

      void
      process_match_request(
        const MatchInfo& request_info)
        /*throw(MatchRequestProcessor::Exception)*/;

      bool
      get_last_daily_processing_time(
        const AdServer::Commons::UserId& user_id,
        Generics::Time& time)
        /*throw(Exception)*/;

      void
      process_user(
        const InventoryDailyMatchInfo& inv_daily_match_info)
        /*throw(Exception)*/;

      void
      all_users(UserIdList& users) /*throw(Exception)*/;

      void
      save_profile_(
        const AdServer::Commons::UserId& user_id,
        const Generics::ConstSmartMemBuf* profile,
        const Generics::Time& time)
        /*throw(Exception)*/;

    private:
      typedef Sync::Policy::PosixThread SyncPolicy;

      typedef AdServer::ProfilingCommons::ChunkedProfileMap<
        UserId,
        AdServer::ProfilingCommons::TransactionProfileMap<UserId>,
        unsigned long (*)(const Generics::Uuid& uuid) >
      UserInventoryInfoMap;

      typedef ReferenceCounting::SmartPtr<UserInventoryInfoMap>
        UserInventoryInfoMap_var;

      typedef std::list<ColoReachProcessor::ColoReachInfo>
        ColoReachInfoList;

    private:
      virtual
      ~UserInventoryInfoContainer() noexcept;

      void
      process_request_trans_(
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
      init_inv_info_(
        InventoryActionProcessor::InventoryInfo& inv_info,
        const MatchInfo& request_info)
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

    typedef ReferenceCounting::SmartPtr<UserInventoryInfoContainer>
      UserInventoryInfoContainer_var;

  } /* RequestInfoSvcs */
} /* AdServer */

namespace AdServer
{
namespace RequestInfoSvcs
{

  inline
  void
  UserInventoryInfoContainer::InventoryDailyMatchInfo::print(
    std::ostream& ostr, const char* offset) const
    noexcept
  {
    ostr << offset << "user_id: '" << user_id << "'" << std::endl <<
      offset << "time: " << time.get_gm_time() << std::endl <<
      offset << "channels: ";
    Algs::print(ostr,
      triggered_expression_channels.begin(), triggered_expression_channels.end());
    ostr << std::endl;
  }
}
}

#endif /*_EXPRESSION_MATCHER_USER_INVENTORY_CONTAINER_IMPL_HPP_*/
