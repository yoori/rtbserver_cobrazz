
#ifndef _AD_SERVER_LOG_PROCESSING_SYNCLOGS_IMPL_HPP_
#define _AD_SERVER_LOG_PROCESSING_SYNCLOGS_IMPL_HPP_

#include <string>
#include <list>

#include <eh/Exception.hpp>

#include <ReferenceCounting/ReferenceCounting.hpp>
#include <ReferenceCounting/List.hpp>

#include <Logger/Logger.hpp>
#include <Generics/ActiveObject.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/TaskRunner.hpp>

#include <xsd/LogProcessing/SyncLogsConfig.hpp>

#include "RouteProcessor.hpp"
#include "Utils.hpp"
#include "RouteHelpers.hpp"

namespace AdServer
{
  namespace LogProcessing
  {
    /** SyncLogs servant.
     * For each directory, configured in the Configure.xml file,
     * this service sends logs matching the configured pattern
     * from this directory to destination by running 'rsync'.
     */
    class SyncLogsImpl:
      public Generics::CompositeActiveObject,
      public ReferenceCounting::AtomicImpl
    {
    public:
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

      typedef xsd::AdServer::Configuration::SyncLogsConfigType Configuration;
      typedef std::unique_ptr<Configuration> ConfigurationPtr;

    public:
      /** Parametric constructor, provides SyncLogs the callback
       *  and logger pointers as well as database access parameters.
       *
       * @param callback Pointer to a callback object instance.
       * @param logger Pointer to a logger object instance.
       */
      SyncLogsImpl(
        Generics::ActiveObjectCallback *callback,
        Logging::Logger* logger,
        const Configuration &config)
        /*throw(Exception, eh::Exception)*/;

      virtual void
      wait_object() /*throw(ActiveObject::Exception, eh::Exception)*/;

    private:
      virtual ~SyncLogsImpl() noexcept;

      typedef Generics::TaskGoal TaskMessage;

      typedef ReferenceCounting::QualPtr<TaskMessage>
        TaskMessage_var;

      typedef ReferenceCounting::QualPtr<RouteProcessor> RouteProcessorPtr;
      typedef ReferenceCounting::List<RouteProcessorPtr> RouteProcessorList;

      class ProcessRouteTask : public virtual TaskMessage
      {
      public:
        ProcessRouteTask(
          RouteProcessor* processor,
          unsigned long check_period,
          Generics::TaskRunner* task_runner,
          SyncLogsImpl* sync_logs_impl) noexcept;

        virtual void execute() noexcept;

      private:
        RouteProcessorPtr route_processor_;
        unsigned long check_period_;
        SyncLogsImpl* sync_logs_impl_;
      };

      struct TraceLevel
      {
        enum
        {
          LOW = Logging::Logger::TRACE,
          MIDDLE,
          HIGH
        };
      };

      typedef xsd::AdServer::Configuration::SrcDestType HostsRoute;

    private:
      void schedule_route_processing_task_(
        RouteProcessor *processor,
        unsigned long timeout) noexcept;

      static SchedType get_feed_type_(
        const char* feed_type_name)
        /*throw(Exception)*/;

      const char* adapt_path_(
        const char* dir, std::string& res) const
        /*throw(Exception)*/;

      void parse_hosts_route_(
        const HostsRoute& hosts_route,
        StringList& src_hosts,
        StringList* dst_hosts);

      StringList::const_iterator
      check_hosts_(const StringList& hosts_list)
        /*throw(Exception)*/;

    private:
      Generics::ActiveObjectCallback_var callback_;
      Logging::Logger_var logger_;
      Utils::ErrorPool error_logger_;

      ConfigurationPtr configuration_;
      const LocalInterfaceChecker host_checker_;

      Generics::TaskRunner_var task_runner_;
      Generics::Planner_var scheduler_;

      RouteProcessorList route_processors_;
    };

    typedef ReferenceCounting::QualPtr<SyncLogsImpl>
      SyncLogsImpl_var;
  }
}

// Inlines
namespace AdServer
{
  namespace LogProcessing
  {
    //
    // SyncLogsImpl class
    //
    inline
    void
    SyncLogsImpl::wait_object() /*throw(ActiveObject::Exception, eh::Exception)*/
    {
      Generics::CompositeActiveObject::wait_object();
      error_logger_.dump(Logging::Logger::ERROR, "SyncLogs");
    }

    inline
    SyncLogsImpl::ProcessRouteTask::ProcessRouteTask(
      RouteProcessor* processor,
      unsigned long check_period,
      Generics::TaskRunner* task_runner,
      SyncLogsImpl* sync_logs_impl) noexcept
      : TaskMessage(task_runner),
        route_processor_(ReferenceCounting::add_ref(processor)),
        check_period_(check_period),
        sync_logs_impl_(sync_logs_impl)
    {}

    inline
    void SyncLogsImpl::ProcessRouteTask::execute() noexcept
    {
      route_processor_->process();
      sync_logs_impl_->schedule_route_processing_task_(route_processor_,
        check_period_);
    }
  }
}

#endif // _AD_SERVER_LOG_PROCESSING_SYNCLOGS_IMPL_HPP_
