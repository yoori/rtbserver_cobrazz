#pragma once

#include <list>

#include <Generics/CompositeActiveObject.hpp>
#include <Generics/Scheduler.hpp>

#include <Commons/Coro/StartableAwaitable.hpp>
#include <Commons/ExecutorPool.hpp>
#include <LogCommons/FileReceiverFacade.hpp>
#include <LogCommons/RequestBasicChannels.hpp>

#include "ConsiderInterface.hpp"

namespace AdServer::RequestInfoSvcs
{
  class RequestBasicChannelsProcessor
  {
  public:
    virtual
    ~RequestBasicChannelsProcessor() noexcept
    {}

    virtual AdServer::Commons::StartableAwaitable<void>
    co_process_request_basic_channels_record(
      const LogProcessing::RequestBasicChannelsCollector::KeyT& key,
      const LogProcessing::RequestBasicChannelsCollector::DataT::DataT& record) = 0;

    virtual void
    request_basic_channels_file_processed(const Generics::Time& timestamp) noexcept = 0;
  };

  class ExpressionMatcherLogLoader :
    public Generics::RefCountableCompositeActiveObject
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    enum class LogType
    {
      RequestBasicChannels,
      ConsiderClick,
      ConsiderImpression
    };

    struct LogReadTraits
    {
      LogType log_type;
      std::string in_path;
      std::string prefix;
      std::string intermediate_fetch_dir;

      LogReadTraits(
        LogType type,
        const std::string& path,
        const std::string& pref,
        const std::string& intermediate)
        noexcept
      : log_type(type), in_path(path), prefix(pref),
        intermediate_fetch_dir(intermediate)
      {}
    };

    typedef std::list<LogReadTraits> LogReadTraitsList;

  public:
    ExpressionMatcherLogLoader(
      ConsiderInterface* consider_interface_,
      RequestBasicChannelsProcessor* request_basic_channels_processor,
      Generics::TaskRunner* task_runner,
      Generics::Planner* scheduler,
      Logging::Logger* logger,
      Generics::ActiveObjectCallback* callback,
      unsigned file_threads_number,
      std::shared_ptr<Commons::ExecutorPool> processing_executor_pool,
      LogReadTraitsList log_read_traits,
      unsigned long check_logs_period)
      noexcept;

  private:
    typedef LogProcessing::FileReceiverFacade<
      LogProcessing::DefaultOrderStrategy<LogType> > FileReceiverFacade;
    typedef ReferenceCounting::SmartPtr<FileReceiverFacade>
      FileReceiverFacade_var;

    ConsiderInterface* consider_interface_;
    RequestBasicChannelsProcessor* request_basic_channels_processor_;
    Generics::TaskRunner_var task_runner_;
    Generics::Planner_var scheduler_;
    Logging::Logger_var logger_;
    FileReceiverFacade_var file_receiver_facade_;
    LogReadTraitsList log_read_traits_;
    std::shared_ptr<Commons::ExecutorPool> processing_executor_pool_;

  private:
    virtual
    ~ExpressionMatcherLogLoader() noexcept
    {}

    void
    fetch_files_(
      const LogReadTraits& log_traits,
      unsigned long check_logs_period,
      LogProcessing::FileReceiver_var file_receiver)
      noexcept;

    void
    process_file_() noexcept;

    void
    prepare_mem_buf_(Generics::MemBuf& membuf, unsigned long size)
      noexcept;

    bool
    process_request_basic_channels_file_(
      LogProcessing::FileReceiver::FileGuard* file_ptr,
      std::size_t& processed_lines_count)
      /*throw(eh::Exception)*/;

    bool
    process_binary_file_(
      LogProcessing::FileReceiver::FileGuard* file_ptr,
      LogType log_type,
      std::size_t& processed_lines_count)
      /*throw(eh::Exception)*/;

    const LogReadTraits&
    find_log_read_traits_(LogType log_type) /*throw(Exception)*/;

    void
    file_move_back_to_input_dir_(
      const AdServer::LogProcessing::LogFileNameInfo& info,
      const char* file_name) /*throw(eh::Exception)*/;
  };

  typedef ReferenceCounting::SmartPtr<ExpressionMatcherLogLoader>
    ExpressionMatcherLogLoader_var;
}
