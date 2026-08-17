#pragma once

#include <memory>

#include <eh/Exception.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>

#include <Logger/Logger.hpp>
#include <Generics/Time.hpp>

#include <Generics/MemBuf.hpp>
#include <Generics/CompositeActiveObject.hpp>

#include <ProfilingCommons/ProfileMap/TransactionProfileMap.hpp>
#include <String/SubString.hpp>

#include <Commons/Coro/StartableAwaitable.hpp>

#include "RequestActionProcessor.hpp"
#include "RequestOperationProcessor.hpp"
#include "RequestOperationSaver.hpp"

namespace AdServer
{
  namespace ProfilingCommons
  {
    class RocksDBProfileMapProcessor;
  }

  namespace RequestInfoSvcs
  {
    class RequestInfoProfileWriter;

    using RequestProfileMap = ProfilingCommons::TransactionProfileMap<
      AdServer::Commons::RequestId>;

    using RequestProfileMap_var = ReferenceCounting::SmartPtr<RequestProfileMap>;

    /**
     * RequestInfoContainer
     * contains logic of requests processing
     */
    class RequestInfoContainer:
      public Generics::RefCountableCompositeActiveObject,
      public RequestContainerProcessor
    {
    public:
      static const Generics::Time DEFAULT_EXPIRE_TIME; // 180 days

    public:
      DECLARE_EXCEPTION(Exception, RequestContainerProcessor::Exception);

      RequestInfoContainer(
        Logging::Logger* logger,
        RequestActionProcessor* request_processor,
        RequestOperationProcessor* request_operation_processor,
        const String::SubString& request_profile_path,
        const Generics::Time& expire_time = DEFAULT_EXPIRE_TIME,
        std::shared_ptr<ProfilingCommons::RocksDBProfileMapProcessor>
          rocksdb_processor = {})
        /*throw(Exception)*/;

      Generics::ConstSmartMemBuf_var
      get_profile(const AdServer::Commons::RequestId& request_id)
        /*throw(Exception)*/;

      void
      clear_expired_requests() /*throw(Exception)*/;

      /** RequestContainerProcessor interface */
      virtual void
      process_request(const RequestInfo& request_info)
        /*throw(RequestContainerProcessor::Exception)*/;

      virtual AdServer::Commons::Awaitable<void>
      co_process_request(const RequestInfo& request_info);

      virtual void
      process_impression(const ImpressionInfo& impression_info)
        /*throw(RequestContainerProcessor::Exception)*/;

      virtual AdServer::Commons::Awaitable<void>
      co_process_impression(const ImpressionInfo& impression_info);

      virtual void
      process_action(
        ActionType action_type,
        const Generics::Time& time,
        const AdServer::Commons::RequestId& request_id)
        /*throw(RequestContainerProcessor::Exception)*/;

      virtual AdServer::Commons::Awaitable<void>
      co_process_action(
        ActionType action_type,
        const Generics::Time& time,
        const AdServer::Commons::RequestId& request_id);

      virtual void
      process_custom_action(
        const AdServer::Commons::RequestId& request_id,
        const AdvCustomActionInfo& adv_custom_action_info)
        /*throw(RequestContainerProcessor::Exception)*/;

      virtual AdServer::Commons::Awaitable<void>
      co_process_custom_action(
        const AdServer::Commons::RequestId& request_id,
        const AdvCustomActionInfo& adv_custom_action_info);

      virtual void
      process_impression_post_action(
        const AdServer::Commons::RequestId& request_id,
        const RequestPostActionInfo& request_post_action_info)
        /*throw(RequestContainerProcessor::Exception)*/;

      virtual AdServer::Commons::Awaitable<void>
      co_process_impression_post_action(
        const AdServer::Commons::RequestId& request_id,
        const RequestPostActionInfo& request_post_action_info);

      RequestContainerProcessor_var
      proxy() noexcept;

      RequestOperationProcessor_var
      request_operation_proxy() noexcept;

    protected:
      class ProxyImpl;
      using ProxyImpl_var = ReferenceCounting::SmartPtr<ProxyImpl>;

      class RequestOperationProxy;
      using RequestOperationProxy_var =
        ReferenceCounting::SmartPtr<RequestOperationProxy>;

      struct MoveActionInfo
      {
        MoveActionInfo(
          ActionType action_type_val,
          const Generics::Time& time_val,
          const AdServer::Commons::RequestId& request_id_val)
          : action_type(action_type_val),
            time(time_val),
            request_id(request_id_val)
        {}

        ActionType action_type;
        Generics::Time time;
        AdServer::Commons::RequestId request_id;
      };

      struct MoveRequestPostActionInfo: public RequestPostActionInfo
      {
        MoveRequestPostActionInfo(
          const AdServer::Commons::RequestId& request_id_val,
          const RequestPostActionInfo& request_post_action_info)
          : RequestPostActionInfo(request_post_action_info),
            request_id(request_id_val)
        {}

        AdServer::Commons::RequestId request_id;
      };

      struct RequestProcessDelegate
      {
        RequestProcessDelegate()
          : process_impression(false),
            process_click(false),
            process_actions(0)
        {}

        AdServer::Commons::Optional<RequestInfo> request_info;
        AdServer::Commons::Optional<ImpressionInfo> impression_info;
        Commons::Optional<RequestInfo::RequestState> process_request;
        bool process_impression;
        bool process_click;
        unsigned long process_actions;

        // if rollback_request_info defined it will be used for rollback instead request_info
        AdServer::Commons::Optional<RequestInfo> rollback_request_info;
        Commons::Optional<RequestInfo::RequestState> process_fraud_request;
        std::list<RequestInfo::RequestState> process_rollback_impressions;
        std::list<RequestInfo::RequestState> process_rollback_clicks;

        AdvCustomActionInfoList custom_actions;
        RequestPostActionInfoList process_post_impression_actions;

        AdServer::Commons::UserId move_request_user_id;
        AdServer::Commons::RequestId move_request_id;
        Generics::ConstSmartMemBuf_var move_request_profile;

        AdServer::Commons::Optional<ImpressionInfo> move_notice_info;
        AdServer::Commons::Optional<ImpressionInfo> move_impression_info;
        std::vector<MoveActionInfo> move_actions; // AT_CLICK,AT_ACTION,AT_FRAUD_ROLLBACK
        std::vector<MoveRequestPostActionInfo> move_impression_post_actions;
      };

      class Transaction: public ReferenceCounting::AtomicImpl
      {
      public:
        explicit Transaction(RequestProfileMap::Transaction* transaction);

        virtual
        Generics::ConstSmartMemBuf_var
        get_profile(Generics::Time* last_access_time = 0);

        AdServer::Commons::StartableAwaitable<Generics::ConstSmartMemBuf_var>
        co_get_profile();

        virtual void
        save_profile(
          const Generics::ConstSmartMemBuf* mem_buf,
          const Generics::Time& now = Generics::Time::get_time_of_day());

        AdServer::Commons::StartableAwaitable<void>
        co_save_profile(
          const Generics::ConstSmartMemBuf* mem_buf,
          const Generics::Time& now = Generics::Time::get_time_of_day());

      protected:
        RequestProfileMap::Transaction_var transaction_;
      };

      using Transaction_var = ReferenceCounting::SmartPtr<Transaction>;

    protected:
      virtual
      ~RequestInfoContainer() noexcept;

      void
      delegate_processing_(
        const RequestProcessDelegate& request_process_gelegate)
        /*throw(Exception)*/;

      AdServer::Commons::Awaitable<void>
      co_delegate_processing_(
        const RequestProcessDelegate& request_process_gelegate);

      static void
      throw_request_processing_exception_(
        const char* fun,
        const AdServer::Commons::RequestId& request_id,
        const char* message)
        /*throw(Exception)*/;

      virtual void
      process_impression_(
        const ImpressionInfo& impression_info,
        bool move_enabled)
        /*throw(RequestContainerProcessor::Exception)*/;

      virtual AdServer::Commons::Awaitable<void>
      co_process_impression_(
        const ImpressionInfo& impression_info,
        bool move_enabled);

      virtual void
      process_action_(
        ActionType action_type,
        const Generics::Time& time,
        const AdServer::Commons::RequestId& request_id,
        bool move_enabled)
        /*throw(RequestContainerProcessor::Exception)*/;

      virtual AdServer::Commons::Awaitable<void>
      co_process_action_(
        ActionType action_type,
        const Generics::Time& time,
        const AdServer::Commons::RequestId& request_id,
        bool move_enabled);

      virtual void
      process_impression_post_action_(
        const AdServer::Commons::RequestId& request_id,
        const RequestPostActionInfo& request_post_action_info,
        bool move_enabled)
        /*throw(Exception)*/;

      virtual AdServer::Commons::Awaitable<void>
      co_process_impression_post_action_(
        const AdServer::Commons::RequestId& request_id,
        const RequestPostActionInfo& request_post_action_info,
        bool move_enabled);

      bool
      process_request_buf_(
        Generics::ConstSmartMemBuf_var& mem_buf,
        RequestProcessDelegate& request_process_delegate,
        Generics::Time* last_event_time,
        const RequestInfo& request_info,
        bool move_enabled)
        /*throw(Exception)*/;

      bool
      process_notice_buf_(
        Generics::ConstSmartMemBuf_var& mem_buf,
        RequestProcessDelegate& request_process_delegate,
        Generics::Time* last_event_time,
        const ImpressionInfo& notice_info,
        bool move_enabled)
        /*throw(Exception)*/;

      bool
      process_impression_buf_(
        Generics::ConstSmartMemBuf_var& mem_buf,
        RequestProcessDelegate& request_process_delegate,
        Generics::Time* last_event_time,
        const ImpressionInfo& impression_info,
        bool move_enabled)
        /*throw(Exception)*/;

      static bool
      process_click_buf_(
        Generics::ConstSmartMemBuf_var& mem_buf,
        RequestProcessDelegate& request_process_delegate,
        Generics::Time* last_event_time,
        const AdServer::Commons::RequestId& request_id,
        const Generics::Time& time,
        bool move_enabled)
        /*throw(Exception)*/;

      static bool
      process_action_buf_(
        Generics::ConstSmartMemBuf_var& mem_buf,
        RequestProcessDelegate& request_process_delegate,
        Generics::Time* last_event_time,
        const AdServer::Commons::RequestId& request_id,
        const Generics::Time& action_time,
        bool move_enabled)
        /*throw(Exception)*/;

      static
      bool
      process_fraud_rollback_buf_(
        Generics::ConstSmartMemBuf_var& mem_buf,
        RequestProcessDelegate& request_process_delegate,
        Generics::Time* last_event_time,
        const AdServer::Commons::RequestId& request_id,
        const Generics::Time& rollback_time)
        /*throw(Exception)*/;

      static
      void
      process_custom_action_buf_(
        const Generics::ConstSmartMemBuf* mem_buf,
        RequestProcessDelegate& request_process_delegate,
        const AdServer::Commons::RequestId& request_id,
        const AdvCustomActionInfo& adv_custom_action_info)
        /*throw(Exception)*/;

      static bool
      process_impression_post_action_buf_(
        Generics::ConstSmartMemBuf_var& mem_buf,
        RequestProcessDelegate& request_process_delegate,
        Generics::Time* last_event,
        const AdServer::Commons::RequestId& request_id,
        const RequestPostActionInfo& request_post_action_info,
        bool move_enabled)
        /*throw(Exception)*/;

      static Generics::ConstSmartMemBuf_var
      get_profile_(
        Transaction* transaction);

      static AdServer::Commons::Awaitable<Generics::ConstSmartMemBuf_var>
      co_get_profile_(
        Transaction* transaction);

      Generics::ConstSmartMemBuf_var
      get_profile_(const AdServer::Commons::RequestId& request_id);

      static void
      save_profile_(
        Transaction* transaction,
        const Generics::ConstSmartMemBuf* mem_buf,
        const Generics::Time& time);

      static AdServer::Commons::Awaitable<void>
      co_save_profile_(
        Transaction* transaction,
        const Generics::ConstSmartMemBuf* mem_buf,
        const Generics::Time& time);

      Transaction_var
      get_transaction_(
        const AdServer::Commons::RequestId& request_id);

      AdServer::Commons::Awaitable<Transaction_var>
      co_get_transaction_(
        const AdServer::Commons::RequestId& request_id);

      void
      change_request_user_id_(
        const AdServer::Commons::UserId& new_user_id,
        const AdServer::Commons::RequestId& request_id,
        const Generics::ConstSmartMemBuf* request_profile)
        /*throw(Exception)*/;

      AdServer::Commons::Awaitable<void>
      co_change_request_user_id_(
        const AdServer::Commons::UserId& new_user_id,
        const AdServer::Commons::RequestId& request_id,
        const Generics::ConstSmartMemBuf* request_profile);

      void
      convert_impression_info_to_request_writer(
        const ImpressionInfo& impression_info,
        RequestInfoProfileWriter& request_writer,
        RequestInfo* request_info,
        bool notice);

      void
      eval_revenues_on_impression_fin(
        RequestInfoProfileWriter& request_writer,
        RequestInfo* request_info);

      void
      eval_revenues_on_impression(
        RequestInfoProfileWriter& request_writer,
        RequestInfo* request_info,
        const RevenueDecimal& res_pub_revenue,
        const RequestInfo::Revenue& orig_pub_revenue);

      RevenueDecimal div(
        const RevenueDecimal& dividend,
        const RevenueDecimal& divisor,
        const char * error_message = 0,
        unsigned long account_id = 0) const
        /*throw(eh::Exception, RevenueDecimal::Overflow)*/;

      RevenueDecimal div(
        const RevenueDecimal& dividend,
        const std::string& divisor,
        const char * error_message = 0,
        unsigned long account_id = 0) const
        /*throw(eh::Exception, RevenueDecimal::Overflow)*/
      {
        return div(dividend, RevenueDecimal(divisor), error_message, account_id);
      }

    protected:
      Logging::Logger_var logger_;
      Generics::Time expire_time_;
      RequestProfileMap_var profile_map_;

      ProxyImpl_var proxy_;
      RequestOperationProxy_var request_operation_proxy_;
      RequestActionProcessor_var request_processor_;
      RequestOperationProcessor_var request_operation_processor_;
    };

    using RequestInfoContainer_var =
      ReferenceCounting::SmartPtr<RequestInfoContainer>;
  } /* RequestInfoSvcs */
} /* AdServer */

namespace Aspect
{
  const char REQUEST_INFO_CONTAINER[] = "RequestInfoContainer";
}

namespace AdServer
{
  namespace RequestInfoSvcs
  {
    inline
    RevenueDecimal RequestInfoContainer::div(
      const RevenueDecimal& dividend,
      const RevenueDecimal& divisor,
      const char * error_message,
      unsigned long account_id) const
      /*throw(eh::Exception, RevenueDecimal::Overflow)*/
    {
      if (divisor != RevenueDecimal::ZERO)
      {
        return
          RevenueDecimal::div(dividend, divisor, Generics::DDR_CEIL);
      }

      if (error_message)
      {
        logger_->sstream(
          Logging::Logger::ERROR,
          Aspect::REQUEST_INFO_CONTAINER) << error_message <<
          " (account id " << account_id << ")";
      }
      return RevenueDecimal::ZERO;
    }
  } /* RequestInfoSvcs */
} /* AdServer */
