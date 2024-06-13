/**
 * @file UserActionInfoContainer.hpp
 */
#ifndef _REQUEST_INFO_SVCS_USER_ACTION_INFO_CONTAINER_IMPL_HPP_
#define _REQUEST_INFO_SVCS_USER_ACTION_INFO_CONTAINER_IMPL_HPP_

#include <eh/Exception.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>

#include <Logger/Logger.hpp>
#include <Generics/Time.hpp>
#include <Generics/MemBuf.hpp>

#include <ProfilingCommons/ProfileMap/ProfileMapFactory.hpp>
#include <ProfilingCommons/PlainStorageAdapters.hpp>

#include "RequestActionProcessor.hpp"

namespace AdServer
{
  namespace RequestInfoSvcs
  {
    const Generics::Time DEFAULT_ACTION_IGNORE_TIME(30); // 30 seconds
    const Generics::Time DEFAULT_ACTION_PROFILE_EXPIRE_TIME(180*24*60*60); // 180 days

    /**
     * UserActionInfoContainer
     * contains logic of requests processing
     */
    class UserActionInfoContainer:
      public virtual ReferenceCounting::AtomicImpl,
      public virtual AdvActionProcessor
    {
    public:
      using RocksdbManagerPool = UServerUtils::Grpc::RocksDB::DataBaseManagerPool;
      using RocksdbManagerPoolPtr = std::shared_ptr<RocksdbManagerPool>;
      using RocksDBParams = AdServer::ProfilingCommons::RocksDB::RocksDBParams;

      DECLARE_EXCEPTION(Exception, RequestContainerProcessor::Exception);

    public:
      UserActionInfoContainer(
        Logging::Logger* logger,
        RequestContainerProcessor* request_processor,
        const char* useractionfile_base_path,
        const char* useractionfile_prefix,
        const bool is_level_enable,
        const bool is_rocksdb_enable,
        const RocksdbManagerPoolPtr& rocksdb_manager_pool,
        const RocksDBParams& rocksdb_params,
        const Generics::Time& action_ignore_time,
        ProfilingCommons::ProfileMapFactory::Cache* cache,
        const Generics::Time& expire_time = DEFAULT_ACTION_PROFILE_EXPIRE_TIME,
        const Generics::Time& extend_time_period = Generics::Time::ZERO)
        /*throw(Exception)*/;

      Generics::ConstSmartMemBuf_var
      get_profile(const AdServer::Commons::RequestId& request_id)
        /*throw(Exception)*/;

      void clear_expired_actions() /*throw(Exception)*/;

      /** AdvActionProcessor interface */
      virtual void process_adv_action(
        const AdvActionInfo& adv_action_info)
        /*throw(AdvActionProcessor::Exception)*/;

      virtual void process_custom_action(
        const AdvExActionInfo& adv_custom_action_info)
        /*throw(AdvActionProcessor::Exception)*/;

      void
      request_container_processor(
        RequestContainerProcessor* request_container_processor)
        noexcept;

      RequestActionProcessor_var
      request_processor() noexcept;

    protected:
      virtual ~UserActionInfoContainer() noexcept;

    private:
      class RequestActionProcessorImpl;
      friend class RequestActionProcessorImpl;

      typedef std::list<AdServer::Commons::RequestId> RequestIdList;

      typedef ProfilingCommons::TransactionProfileMap<
        AdServer::Commons::UserId>
        UserActionInfoMap;

      typedef ReferenceCounting::SmartPtr<UserActionInfoMap>
        UserActionInfoMap_var;

      struct DelegateCustomActionInfo:
        public AdvCustomActionInfo
      {
        AdServer::Commons::RequestId request_id;
      };

      typedef std::list<DelegateCustomActionInfo>
        DelegateCustomActionInfoList;

    private:
      void
      process_click_trans_(
        unsigned long& delegate_process_actions,
        AdvCustomActionInfoList& delegate_process_custom_actions,
        const RequestInfo& request_info)
        /*throw(RequestActionProcessor::Exception)*/;

      void process_impression_trans_(
        AdvCustomActionInfoList& delegate_process_custom_actions,
        const RequestInfo& request_info)
        /*throw(RequestActionProcessor::Exception)*/;

      void process_adv_action_trans_(
        RequestIdList& delegate_process_actions,
        const AdvActionInfo& adv_action_info)
        /*throw(AdvActionProcessor::Exception)*/;

      void process_custom_action_trans_(
        DelegateCustomActionInfoList& delegate_custom_actions,
        const AdvExActionInfo& adv_ex_action_info)
        /*throw(AdvActionProcessor::Exception)*/;

      void process_impression_(
        const RequestInfo& request_info,
        const ImpressionInfo& imp_info,
        const RequestActionProcessor::ProcessingState& processing_state)
        /*throw(RequestActionProcessor::Exception)*/;

      void process_click_(
        const RequestInfo& request_info,
        const RequestActionProcessor::ProcessingState& processing_state)
        /*throw(RequestActionProcessor::Exception)*/;

    private:
      Logging::Logger_var logger_;
      Generics::Time action_ignore_time_;
      Generics::Time expire_time_;
      UserActionInfoMap_var user_map_;
      RequestContainerProcessor_var request_container_processor_;
      RequestActionProcessor_var request_processor_;
    };

    typedef ReferenceCounting::SmartPtr<UserActionInfoContainer>
      UserActionInfoContainer_var;

  } // RequestInfoSvcs
} // AdServer

namespace AdServer
{
namespace RequestInfoSvcs
{
  inline
  void
  UserActionInfoContainer::request_container_processor(
    RequestContainerProcessor* request_container_processor)
    noexcept
  {
    request_container_processor_ = ReferenceCounting::add_ref(
      request_container_processor);
  }
}
}

#endif // _REQUEST_INFO_SVCS_USER_ACTION_INFO_CONTAINER_IMPL_HPP_
