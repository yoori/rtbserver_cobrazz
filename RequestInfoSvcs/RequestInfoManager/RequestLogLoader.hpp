/**
 * @file RequestLogLoader.hpp
 */
#ifndef _REQUEST_LOG_LOADER_HPP_
#define _REQUEST_LOG_LOADER_HPP_

#include <Generics/TaskRunner.hpp>
#include <Generics/Values.hpp>
#include <Generics/CompositeActiveObject.hpp>

#include <Logger/ActiveObjectCallback.hpp>
#include <LogCommons/FileReceiverFacade.hpp>

#include "RequestActionProcessor.hpp"
#include "PassbackContainer.hpp"
#include "TagRequestProcessor.hpp"
#include "RequestOperationLoader.hpp"
#include "CompositeMetricsProviderRIM.hpp"

namespace AdServer
{
  namespace RequestInfoSvcs
  {
    class RequestInfoManagerStatsImpl;

    struct LogProcessingState: public ReferenceCounting::AtomicImpl
    {
      LogProcessingState()
        : interrupter(new LogProcessing::FileReceiverInterrupter())
      {}

      LogProcessing::FileReceiverInterrupter_var interrupter;

    protected:
      virtual
      ~LogProcessingState() noexcept
      {}
    };

    typedef ReferenceCounting::AssertPtr<LogProcessingState>::Ptr
      LogProcessingState_var;

    struct InLog
    {
      std::string dir;
      unsigned int priority;

      InLog() noexcept;
    };

    struct InLogs
    {
      InLog request;
      InLog impression;
      InLog click;
      InLog advertiser_action;
      InLog passback_impression;
      InLog tag_request;
      InLog request_operation;
    };

    class LogFetcherBase: public ReferenceCounting::AtomicImpl
    {
    public:
      LogFetcherBase(
        unsigned int priority,
        LogProcessing::FileReceiver* file_receiver,
        CompositeMetricsProviderRIM* cmprim)
        noexcept;

      virtual
      Generics::Time
      check_files() noexcept = 0;

      virtual void
      process(LogProcessing::FileReceiver::FileGuard* file_ptr) noexcept = 0;

      unsigned int
      priority() const noexcept;

      LogProcessing::FileReceiver_var
      file_receiver() noexcept;

    protected:
      virtual
      ~LogFetcherBase() noexcept
      {}

    protected:
      const unsigned int priority_;
      LogProcessing::FileReceiver_var file_receiver_;
      CompositeMetricsProviderRIM_var cmprim_;
    };

    typedef ReferenceCounting::AssertPtr<LogFetcherBase>::Ptr LogFetcher_var;

    class RequestLogLoader:
      public virtual ReferenceCounting::AtomicImpl,
      public virtual Generics::CompositeActiveObject
    {
    public:
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

      RequestLogLoader(
        Generics::ActiveObjectCallback* callback,
        const InLogs& in_logs,
        UnmergedClickProcessor* unmerged_click_processor,
        RequestContainerProcessor* request_container_processor,
        AdvActionProcessor* adv_action_processor,
        PassbackVerificationProcessor* passback_verification_processor,
        TagRequestProcessor* tag_request_processor,
        RequestOperationProcessor* request_operation_processor,
        const Generics::Time& check_period,
        const Generics::Time& max_process_time,
        std::size_t threads_count,
        RequestInfoManagerStatsImpl* process_stats_values,
        CompositeMetricsProviderRIM* cmprim)
        /*throw(Exception)*/;

    protected:
      virtual
      ~RequestLogLoader() noexcept
      {}

      void
      process_file_() noexcept;

    private:
      enum InLogType
      {
        RequestLogType = 0,
        ImpressionLogType,
        ClickLogType,
        AdvertiserActionLogType,
        PassbackImpressionLogType,
        TagRequestLogType,
        RequestOperationLogType,

        LogTypesCount
      };

      class OrderStrategy
      {
      public:
        typedef InLogType LogType;

        struct Key
        {
          std::size_t priority;
          Generics::Time time;

          friend bool
          operator< (
            const Key& arg1,
            const Key& arg2)
          {
            if (arg1.priority != arg2.priority)
            {
              return (arg1.priority > arg2.priority);
            }

            return (arg1.time < arg2.time);
          }
        };

      public:
        OrderStrategy(std::vector<std::size_t> priorities) noexcept
          : priorities_(priorities)
        {}

        Key
        key(
          LogType log_type,
          const std::string* log_file_name = nullptr) const
          noexcept
        {
          Key k;
          k.priority = priorities_[log_type];

          if (log_file_name)
          {
            LogProcessing::LogFileNameInfo name_info;
            LogProcessing::parse_log_file_name(*log_file_name, name_info);
            k.time = name_info.timestamp;
          }

          return k;
        }

      private:
        std::vector<std::size_t> priorities_;
      };

      typedef LogProcessing::FileReceiverFacade<OrderStrategy>
        FileReceiverFacade;

      typedef ReferenceCounting::SmartPtr<FileReceiverFacade>
        FileReceiverFacade_var;

      typedef std::map<InLogType, LogFetcher_var> LogFetchers;

    private:
      /// Callback for task_runner and logger for threads.
      Generics::ActiveObjectCallback_var log_errors_callback_;

      LogProcessingState_var processing_state_;

      Generics::Planner_var scheduler_;
      Generics::TaskRunner_var log_fetch_runner_;

      FileReceiverFacade_var file_receiver_facade_;
      LogFetchers log_fetchers_;
      CompositeMetricsProviderRIM_var cmprim_;
    };

    typedef ReferenceCounting::SmartPtr<RequestLogLoader> RequestLogLoader_var;
  }
}

#endif // _REQUEST_LOG_LOADER_HPP_
