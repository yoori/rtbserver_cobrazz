/**
 * @file UserCampaignReachContainer.hpp
 */
#ifndef REQUESTINFOMANAGER_USERCAMPAIGNREACHCONTAINER_HPP
#define REQUESTINFOMANAGER_USERCAMPAIGNREACHCONTAINER_HPP

#include <list>
#include <string>

#include <ReferenceCounting/ReferenceCounting.hpp>

#include <eh/Exception.hpp>

#include <Logger/Logger.hpp>
#include <Sync/SyncPolicy.hpp>
#include <Generics/Time.hpp>

#include <Generics/MemBuf.hpp>

#include <Commons/Algs.hpp>
#include <ProfilingCommons/ProfileMap/ProfileMapFactory.hpp>
#include <ProfilingCommons/PlainStorageAdapters.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/Algs.hpp>

#include "RequestActionProcessor.hpp"

namespace AdServer
{
  namespace RequestInfoSvcs
  {
    struct CampaignReachProcessor: public virtual ReferenceCounting::Interface
    {
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

      struct ReachInfo
      {
        IdAppearanceList campaigns;
        IdAppearanceList ccgs;
        IdAppearanceList creatives;
        IdAppearanceList advertisers;
        IdAppearanceList display_advertisers;
        IdAppearanceList text_advertisers;

        bool operator==(const ReachInfo& right) const;

        std::ostream& print(
          std::ostream& ostr, const char* offset) const;

      private:
        template<typename ContainerType>
        static
        bool compare_seq_(const ContainerType& left, const ContainerType& right);

/*        template<typename ContainerType>
        static
        std::ostream& print_seq_(
          std::ostream& ostr, const ContainerType& lst);*/
      };

      virtual void process_reach(const ReachInfo& reach_info)
        /*throw(Exception)*/ = 0;

    protected:
      virtual ~CampaignReachProcessor() noexcept {}
    };

    typedef ReferenceCounting::SmartPtr<CampaignReachProcessor>
      CampaignReachProcessor_var;

    const Generics::Time USER_CAMPAIGN_REACH_DEFAULT_EXPIRE_TIME =
      Generics::Time::ONE_DAY * 180; // 180 days

    /**
     * UserCampaignReachContainer
     * contains logic of campaign reach match processing
     * check appearing of cc_id, ccg_id, campaign_id
     *   delegate appear processing to CampaignReachProcessor
     * delegate request processing to request_processor
     */
    class UserCampaignReachContainer:
      public virtual ReferenceCounting::AtomicImpl,
      public virtual RequestActionProcessor
    {
    private:
      using RocksdbManagerPool = UServerUtils::Grpc::RocksDB::DataBaseManagerPool;
      using RocksdbManagerPoolPtr = std::shared_ptr<RocksdbManagerPool>;

    public:
      using RocksDBParams = AdServer::ProfilingCommons::RocksDB::RocksDBParams;
      DECLARE_EXCEPTION(Exception, RequestActionProcessor::Exception);

    public:
      UserCampaignReachContainer(
        Logging::Logger* logger,
        CampaignReachProcessor* campaign_reach_processor,
        const char* requestfile_base_path,
        const char* requestfile_prefix,
        const bool is_rocksdb_enable,
        const RocksdbManagerPoolPtr& rocksdb_manager_pool,
        const RocksDBParams& rocksdb_params,
        ProfilingCommons::ProfileMapFactory::Cache* cache,
        const Generics::Time& expire_time =
          Generics::Time(USER_CAMPAIGN_REACH_DEFAULT_EXPIRE_TIME),
        const Generics::Time& extend_time_period = Generics::Time::ZERO)
        /*throw(Exception)*/;

      Generics::ConstSmartMemBuf_var
      get_profile(const AdServer::Commons::UserId& user_id) /*throw(Exception)*/;

      void clear_expired_users() /*throw(Exception)*/;

      /** overlap CompositeRequestActionProcessor::process_impression */
      virtual void
      process_impression(
        const RequestInfo& request_info,
        const ImpressionInfo& imp_info,
        const ProcessingState& processing_state)
        /*throw(RequestActionProcessor::Exception)*/;

      virtual void
      process_request(
        const RequestInfo&,
        const ProcessingState&)
        /*throw(RequestActionProcessor::Exception)*/
      {};

      virtual void
      process_click(
        const RequestInfo&,
        const ProcessingState&)
        /*throw(RequestActionProcessor::Exception)*/
      {};

      virtual void
      process_action(const RequestInfo&)
        /*throw(RequestActionProcessor::Exception)*/
      {};

    protected:
      virtual ~UserCampaignReachContainer() noexcept;

    private:
      typedef ProfilingCommons::TransactionProfileMap<
        AdServer::Commons::UserId>
        UserCampaignReachMap;

      typedef ReferenceCounting::SmartPtr<UserCampaignReachMap>
        UserCampaignReachMap_var;

    private:
      void process_ad_impression_(
        const RequestInfo& request_info) /*throw(Exception)*/;

    private:
      Logging::Logger_var logger_;
      Generics::Time expire_time_;
      UserCampaignReachMap_var user_map_;
      CampaignReachProcessor_var campaign_reach_processor_;
    };

    typedef ReferenceCounting::SmartPtr<UserCampaignReachContainer>
      UserCampaignReachContainer_var;

  } /* RequestInfoSvcs */
} /* AdServer */

namespace AdServer
{
  namespace RequestInfoSvcs
  {
    template<typename ContainerType>
    inline
    bool
    CampaignReachProcessor::ReachInfo::compare_seq_(
      const ContainerType& left, const ContainerType& right)
    {
      return left.size() == right.size() &&
        std::equal(left.begin(), left.end(), right.begin());
    }

/*    template<typename ContainerType>
    inline
    std::ostream&
    CampaignReachProcessor::ReachInfo::print_seq_(
      std::ostream& ostr, const ContainerType& lst)
    {
      Algs::print(ostr, lst.begin(), lst.end());
      return ostr;
    }*/

    inline
    bool
    CampaignReachProcessor::ReachInfo::operator==(
      const CampaignReachProcessor::ReachInfo& right) const
    {
      return
        compare_seq_(campaigns, right.campaigns) &&
        compare_seq_(ccgs, right.ccgs) &&
        compare_seq_(creatives, right.creatives) &&
        compare_seq_(advertisers, right.advertisers) &&
        compare_seq_(display_advertisers, right.display_advertisers) &&
        compare_seq_(text_advertisers, right.text_advertisers);
    }

    inline
    std::ostream&
    CampaignReachProcessor::ReachInfo::print(
      std::ostream& ostr, const char* offset) const
    {
      ostr << offset << "campaigns: ";
      Algs::print(ostr, campaigns.begin(), campaigns.end());
      ostr << std::endl << offset << "ccgs: ";
      Algs::print(ostr, ccgs.begin(), ccgs.end());
      ostr << std::endl << offset << "creatives: ";
      Algs::print(ostr, creatives.begin(), creatives.end());
      ostr << std::endl << offset << "advertisers: ";
      Algs::print(ostr, advertisers.begin(), advertisers.end());
      ostr << std::endl << offset << "display_advertisers: " << std::endl;
      Algs::print(ostr, display_advertisers.begin(), display_advertisers.end());
      ostr << std::endl << offset << "text_advertisers: " << std::endl;
      Algs::print(ostr, text_advertisers.begin(), text_advertisers.end());

      return ostr;
    }
  }
}

#endif /*REQUESTINFOMANAGER_USERCAMPAIGNREACHCONTAINER_HPP*/
