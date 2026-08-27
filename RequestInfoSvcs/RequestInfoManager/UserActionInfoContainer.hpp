/**
 * @file UserActionInfoContainer.hpp
 */
#pragma once

#include <memory>

#include <eh/Exception.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>

#include <Logger/Logger.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Generics/Time.hpp>
#include <Generics/MemBuf.hpp>

#include <ProfilingCommons/ProfileMap/TransactionProfileMap.hpp>

#include "RequestActionProcessor.hpp"

namespace AdServer
{
  namespace ProfilingCommons
  {
    class RocksDBProfileMapProcessor;
  }

  namespace RequestInfoSvcs
  {
    const Generics::Time DEFAULT_ACTION_IGNORE_TIME(30); // 30 seconds
    const Generics::Time DEFAULT_ACTION_PROFILE_EXPIRE_TIME(180*24*60*60); // 180 days

    /**
     * UserActionInfoContainer
     * contains logic of requests processing
     */
    class UserActionInfoContainer:
      public virtual Generics::RefCountableCompositeActiveObject,
      public virtual AdvActionProcessor
    {
    public:
      DECLARE_EXCEPTION(Exception, RequestContainerProcessor::Exception);

      UserActionInfoContainer(
        Logging::Logger* logger,
        RequestContainerProcessor* request_processor,
        const char* rocksdb_path,
        const Generics::Time& action_ignore_time,
        const Generics::Time& expire_time = DEFAULT_ACTION_PROFILE_EXPIRE_TIME,
        std::shared_ptr<ProfilingCommons::RocksDBProfileMapProcessor>
          rocksdb_processor = {})
        /*throw(Exception)*/;

      Generics::ConstSmartMemBuf_var
      get_profile(const AdServer::Commons::RequestId& request_id)
        /*throw(Exception)*/;

      AdServer::Commons::Awaitable<Generics::ConstSmartMemBuf_var>
      co_get_profile(const AdServer::Commons::RequestId& request_id);

      void clear_expired_actions() /*throw(Exception)*/;

      AdServer::Commons::Awaitable<void>
      co_clear_expired_actions();

      /** AdvActionProcessor interface */
      virtual void process_adv_action(
        const AdvActionInfo& adv_action_info)
        /*throw(AdvActionProcessor::Exception)*/;

      virtual AdServer::Commons::Awaitable<void>
      co_process_adv_action(
        const AdvActionInfo& adv_action_info);

      virtual void process_custom_action(
        const AdvExActionInfo& adv_custom_action_info)
        /*throw(AdvActionProcessor::Exception)*/;

      virtual AdServer::Commons::Awaitable<void>
      co_process_custom_action(
        const AdvExActionInfo& adv_custom_action_info);

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

      using RequestIdList = std::list<AdServer::Commons::RequestId>;

      using UserActionInfoMap = ProfilingCommons::TransactionProfileMap<
        AdServer::Commons::UserId>;

      using UserActionInfoMap_var =
        ReferenceCounting::SmartPtr<UserActionInfoMap>;

      struct DelegateCustomActionInfo:
        public AdvCustomActionInfo
      {
        AdServer::Commons::RequestId request_id;
      };

      using DelegateCustomActionInfoList =
        std::list<DelegateCustomActionInfo>;

    private:
      void
      process_click_trans_(
        unsigned long& delegate_process_actions,
        AdvCustomActionInfoList& delegate_process_custom_actions,
        const RequestInfo& request_info)
        /*throw(RequestActionProcessor::Exception)*/;

      AdServer::Commons::Awaitable<void>
      co_process_click_trans_(
        unsigned long& delegate_process_actions,
        AdvCustomActionInfoList& delegate_process_custom_actions,
        const RequestInfo& request_info);

      void process_impression_trans_(
        AdvCustomActionInfoList& delegate_process_custom_actions,
        const RequestInfo& request_info)
        /*throw(RequestActionProcessor::Exception)*/;

      AdServer::Commons::Awaitable<void>
      co_process_impression_trans_(
        AdvCustomActionInfoList& delegate_process_custom_actions,
        const RequestInfo& request_info);

      void process_adv_action_trans_(
        RequestIdList& delegate_process_actions,
        const AdvActionInfo& adv_action_info)
        /*throw(AdvActionProcessor::Exception)*/;

      AdServer::Commons::Awaitable<void>
      co_process_adv_action_trans_(
        RequestIdList& delegate_process_actions,
        const AdvActionInfo& adv_action_info);

      void process_custom_action_trans_(
        DelegateCustomActionInfoList& delegate_custom_actions,
        const AdvExActionInfo& adv_ex_action_info)
        /*throw(AdvActionProcessor::Exception)*/;

      AdServer::Commons::Awaitable<void>
      co_process_custom_action_trans_(
        DelegateCustomActionInfoList& delegate_custom_actions,
        const AdvExActionInfo& adv_ex_action_info);

      void process_impression_(
        const RequestInfo& request_info,
        const ImpressionInfo& imp_info,
        const RequestActionProcessor::ProcessingState& processing_state)
        /*throw(RequestActionProcessor::Exception)*/;

      AdServer::Commons::Awaitable<void>
      co_process_impression_(
        const RequestInfo& request_info,
        const ImpressionInfo& imp_info,
        const RequestActionProcessor::ProcessingState& processing_state);

      void process_click_(
        const RequestInfo& request_info,
        const RequestActionProcessor::ProcessingState& processing_state)
        /*throw(RequestActionProcessor::Exception)*/;

      AdServer::Commons::Awaitable<void>
      co_process_click_(
        const RequestInfo& request_info,
        const RequestActionProcessor::ProcessingState& processing_state);

    private:
      Logging::Logger_var logger_;
      Generics::Time action_ignore_time_;
      Generics::Time expire_time_;
      UserActionInfoMap_var user_map_;
      RequestContainerProcessor_var request_container_processor_;
      RequestActionProcessor_var request_processor_;
    };

    using UserActionInfoContainer_var =
      ReferenceCounting::SmartPtr<UserActionInfoContainer>;

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
