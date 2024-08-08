#ifndef _REQUEST_INFO_SVCS_REQUEST_INFO_CONTAINER_IMPL_HPP_
#define _REQUEST_INFO_SVCS_REQUEST_INFO_CONTAINER_IMPL_HPP_

#include <optional>

#include <eh/Exception.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>

#include <Logger/Logger.hpp>
#include <Generics/Time.hpp>

#include <Generics/MemBuf.hpp>

#include <ProfilingCommons/ProfileMap/ProfileMapFactory.hpp>
#include <ProfilingCommons/PlainStorageAdapters.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/RequestProfile.hpp>

#include "RequestActionProcessor.hpp"
#include "RequestOperationProcessor.hpp"
#include "RequestOperationSaver.hpp"

namespace AdServer
{
  namespace RequestInfoSvcs
  {
    typedef ProfilingCommons::TransactionProfileMap<
      ProfilingCommons::RequestIdPackHashAdapter>
      RequestInfoMap;

    typedef ReferenceCounting::SmartPtr<RequestInfoMap>
      RequestInfoMap_var;

    typedef ProfilingCommons::TransactionProfileMap<
      AdServer::Commons::RequestId>
      BidProfileMap;

    typedef ReferenceCounting::SmartPtr<BidProfileMap>
      BidProfileMap_var;

    /**
     * RequestInfoContainer
     * contains logic of requests processing
     */
    class RequestInfoContainer:
      public ReferenceCounting::AtomicImpl,
      public RequestContainerProcessor
    {
    public:
      using DataBaseManagerPool = UServerUtils::Grpc::RocksDB::DataBaseManagerPool;
      using DataBaseManagerPoolPtr = std::shared_ptr<DataBaseManagerPool>;
      using RocksDBParams = AdServer::ProfilingCommons::RocksDB::RocksDBParams;

      static const Generics::Time DEFAULT_EXPIRE_TIME; // 180 days

    public:
      DECLARE_EXCEPTION(Exception, RequestContainerProcessor::Exception);

      RequestInfoContainer(
        Logging::Logger* logger,
        RequestActionProcessor* request_processor,
        RequestOperationProcessor* request_operation_processor,
        const DataBaseManagerPoolPtr& rocksdb_manager_pool,
        const char* requestfile_base_path,
        const char* requestfile_prefix,
        const RocksDBParams& request_rocksdb_params,
        const String::SubString& bidfile_base_path,
        const String::SubString& bidfile_prefix,
        ProfilingCommons::ProfileMapFactory::Cache* cache,
        const Generics::Time& expire_time = DEFAULT_EXPIRE_TIME,
        const Generics::Time& extend_time_period = Generics::Time::ZERO)
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

      virtual void
      process_impression(const ImpressionInfo& impression_info)
        /*throw(RequestContainerProcessor::Exception)*/;

      virtual void
      process_action(
        ActionType action_type,
        const Generics::Time& time,
        const AdServer::Commons::RequestId& request_id)
        /*throw(RequestContainerProcessor::Exception)*/;

      virtual void
      process_custom_action(
        const AdServer::Commons::RequestId& request_id,
        const AdvCustomActionInfo& adv_custom_action_info)
        /*throw(RequestContainerProcessor::Exception)*/;

      virtual void
      process_impression_post_action(
        const AdServer::Commons::RequestId& request_id,
        const RequestPostActionInfo& request_post_action_info)
        /*throw(RequestContainerProcessor::Exception)*/;

      RequestContainerProcessor_var
      proxy() noexcept;

      RequestOperationProcessor_var
      request_operation_proxy() noexcept;

    protected:
      class ProxyImpl;
      typedef ReferenceCounting::SmartPtr<ProxyImpl> ProxyImpl_var;

      class RequestOperationProxy;
      typedef ReferenceCounting::SmartPtr<RequestOperationProxy>
        RequestOperationProxy_var;

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

        std::optional<RequestInfo> request_info;
        std::optional<ImpressionInfo> impression_info;
        std::optional<RequestInfo::RequestState> process_request;
        bool process_impression;
        bool process_click;
        unsigned long process_actions;

        // if rollback_request_info defined it will be used for rollback instead request_info
        std::optional<RequestInfo> rollback_request_info;
        std::optional<RequestInfo::RequestState> process_fraud_request;
        std::list<RequestInfo::RequestState> process_rollback_impressions;
        std::list<RequestInfo::RequestState> process_rollback_clicks;

        AdvCustomActionInfoList custom_actions;
        RequestPostActionInfoList process_post_impression_actions;

        AdServer::Commons::UserId move_request_user_id;
        AdServer::Commons::RequestId move_request_id;
        Generics::ConstSmartMemBuf_var move_request_profile;

        std::optional<ImpressionInfo> move_notice_info;
        std::optional<ImpressionInfo> move_impression_info;
        std::vector<MoveActionInfo> move_actions; // AT_CLICK,AT_ACTION,AT_FRAUD_ROLLBACK
        std::vector<MoveRequestPostActionInfo> move_impression_post_actions;
      };

      class Transaction: public ReferenceCounting::AtomicImpl
      {
      public:
        Transaction(
          BidProfileMap::Transaction* transaction,
          RequestInfoMap::Transaction* old_transaction);

        virtual
        Generics::ConstSmartMemBuf_var
        get_profile(Generics::Time* last_access_time = 0);

        virtual void
        save_profile(
          const Generics::ConstSmartMemBuf* mem_buf,
          const Generics::Time& now = Generics::Time::get_time_of_day());

      protected:
        BidProfileMap::Transaction_var transaction_;
        RequestInfoMap::Transaction_var old_transaction_;
      };

      typedef ReferenceCounting::SmartPtr<Transaction>
        Transaction_var;

    protected:
      virtual
      ~RequestInfoContainer() noexcept;

      void
      delegate_processing_(
        const RequestProcessDelegate& request_process_gelegate)
        /*throw(Exception)*/;

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

      virtual void
      process_action_(
        ActionType action_type,
        const Generics::Time& time,
        const AdServer::Commons::RequestId& request_id,
        bool move_enabled)
        /*throw(RequestContainerProcessor::Exception)*/;

      virtual void
      process_impression_post_action_(
        const AdServer::Commons::RequestId& request_id,
        const RequestPostActionInfo& request_post_action_info,
        bool move_enabled)
        /*throw(Exception)*/;

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

      Generics::ConstSmartMemBuf_var
      get_profile_(const AdServer::Commons::RequestId& request_id);

      static void
      save_profile_(
        Transaction* transaction,
        const Generics::ConstSmartMemBuf* mem_buf,
        const Generics::Time& time);

      Transaction_var
      get_transaction_(
        const AdServer::Commons::RequestId& request_id);

      void
      change_request_user_id_(
        const AdServer::Commons::UserId& new_user_id,
        const AdServer::Commons::RequestId& request_id,
        const Generics::ConstSmartMemBuf* request_profile)
        /*throw(Exception)*/;

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
      RequestInfoMap_var request_map_;
      BidProfileMap_var bid_profile_map_;

      ProxyImpl_var proxy_;
      RequestOperationProxy_var request_operation_proxy_;
      RequestActionProcessor_var request_processor_;
      RequestOperationProcessor_var request_operation_processor_;
    };

    typedef
      ReferenceCounting::SmartPtr<RequestInfoContainer>
      RequestInfoContainer_var;
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

#endif /*_REQUEST_INFO_SVCS_REQUEST_INFO_CONTAINER_IMPL_HPP_*/
