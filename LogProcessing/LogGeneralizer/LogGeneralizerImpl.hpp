#ifndef AD_SERVER_LOG_PROCESSING_LOG_GENERALIZER_IMPL_HPP
#define AD_SERVER_LOG_PROCESSING_LOG_GENERALIZER_IMPL_HPP


#include <eh/Exception.hpp>

#include <ReferenceCounting/ReferenceCounting.hpp>
#include <Logger/Logger.hpp>
#include <Sync/SyncPolicy.hpp>
#include <Sync/Condition.hpp>
#include <Stream/MemoryStream.hpp>
#include <Generics/Function.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/TaskRunner.hpp>
#include <CORBACommons/CorbaAdapters.hpp>
#include <CORBACommons/ServantImpl.hpp>
#include <HTTP/HttpAsync.hpp>

#include <Commons/ProcessControlVarsImpl.hpp>
#include <LogCommons/FileReceiver.hpp>

#include "LogGeneralizer.hpp"
#include "LogGeneralizer_s.hpp"

#include "ProcStatImpl.hpp"
#include "LogGeneralizerStatDef.hpp"
#include "LogProcessorDecl.hpp"
#include "DbConnectionFactory.hpp"
#include "ProcessingContexts.hpp"

namespace AdServer {
namespace LogProcessing {

template <typename LogProcessor, typename NoDBLogProcessor = LogProcessor>
struct ProcTraits : protected LogProcessor::Traits
{
  using LogProcessor::Traits::log_base_name;
  typedef LogProcessor LogProcessorType;

  struct Inactive //Use if DB disabled
  {
    typedef NoDBLogProcessor LogProcessorType;
  };
};

class CompositeActiveObjectImpl:
  public Generics::CompositeActiveObject,
  public ReferenceCounting::AtomicImpl
{
};

class LogGeneralizerImpl:
  public Generics::CompositeActiveObject,
  public CORBACommons::ReferenceCounting::
    ServantImpl<POA_AdServer::LogProcessing::LogGeneralizer>
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  typedef xsd::AdServer::Configuration::LogGeneralizerConfigType
    LogGeneralizerConfigType;

  LogGeneralizerImpl(
    Generics::ActiveObjectCallback *callback,
    Logging::Logger *logger,
    const LogGeneralizerConfigType &config,
    ProcStatImpl_var &proc_stat_impl
  ) /*throw(Exception, eh::Exception)*/;

public:
  virtual void change_db_state(bool enable);

  virtual bool get_db_state() /*throw(Commons::DbStateChanger::NotSupported)*/;

  virtual void stop_stat_upload(CORBA::ULong client_id)
    /*throw(AdServer::LogProcessing::LogGeneralizer::NotSupported,
          AdServer::LogProcessing::LogGeneralizer::ImplementationException)*/;

  virtual void start_stat_upload(CORBA::ULong client_id, CORBA::Boolean clear)
    /*throw(AdServer::LogProcessing::LogGeneralizer::NotSupported,
          AdServer::LogProcessing::LogGeneralizer::ImplementationException,
          AdServer::LogProcessing::LogGeneralizer::CollectionNotStarted)*/;

  virtual StatInfo* get_stat_info(CORBA::ULong client_id, CORBA::Boolean clear)
    /*throw(AdServer::LogProcessing::LogGeneralizer::NotSupported,
          AdServer::LogProcessing::LogGeneralizer::ImplementationException,
          AdServer::LogProcessing::LogGeneralizer::CollectionNotStarted)*/;

  virtual
  void
  deactivate_object()
    /*throw(Generics::CompositeActiveObject::Exception, eh::Exception)*/;

private:
  virtual
  ~LogGeneralizerImpl() noexcept;

  typedef xsd::AdServer::Configuration::LogProcessingType
    LogProcessingType;

  typedef xsd::AdServer::Configuration::LogProcessingParamsDeferrableType
    LogProcessingParamsDeferrableType;

  typedef xsd::AdServer::Configuration::LogProcessingParamsDeferrableCSVType
    LogProcessingParamsDeferrableCSVType;

  typedef xsd::AdServer::Configuration::LogProcessingParamsXSearchType
    LogProcessingParamsXSearchType;

  typedef xsd::cxx::tree::optional<LogProcessingParamsDeferrableType>
    LogProcessingParamsDeferrableTypeOptional;

  typedef xsd::cxx::tree::optional<LogProcessingParamsDeferrableCSVType>
    LogProcessingParamsDeferrableCSVTypeOptional;

  typedef xsd::cxx::tree::optional<LogProcessingParamsXSearchType>
    LogProcessingParamsXSearchTypeOptional;

  typedef xsd::AdServer::Configuration::HitsFilteringParamsType
    HitsFilteringParamsType;

  typedef xsd::AdServer::Configuration::LogProcessingParamsHFType
    LogProcessingParamsHFType;

  typedef xsd::cxx::tree::optional<LogProcessingParamsHFType>
    LogProcessingParamsHFTypeOptional;

  bool db_enabled() const;

  bool db_active();

  void
  apply_log_proc_config_(
    const LogProcessingType& config,
    PostgresConnectionFactoryImpl_var& pg_conn_factory)
    /*throw(eh::Exception)*/;

  void
  apply_log_proc_config_part2_(const LogProcessingType& config)
    /*throw(eh::Exception)*/;

  template <class LOG_PROC_PARAMS_TYPE_>
  static unsigned
  calc_max_number_of_threads(
    const xsd::cxx::tree::optional<LOG_PROC_PARAMS_TYPE_> &log_proc_params
  );

  unsigned
  calc_required_number_of_threads(const LogProcessingType& config);

  template <typename LogTraits>
  static void
  disable_upload_() noexcept;

  template <typename LogTraits>
  static void
  enable_upload_() noexcept;

  struct LogProcessorParams
  {
    const CollectorBundleParams bundle_params;
    std::string in_dir;
    std::string out_dir;
    mutable LogProcThreadInfo_var log_proc_thread_info;
  };

  template <typename Traits, typename LogProcessingParams>
  LogProcessor_var
  make_log_processor_(
    const LogProcessorParams& p,
    const xsd::cxx::tree::optional<LogProcessingParams>& log_proc_params)
    /*throw(eh::Exception)*/;

  template <typename Traits>
  LogProcessor_var
  make_log_processor_(
    const LogProcessorParams& p,
    const LogProcessingParamsXSearchTypeOptional& log_proc_params)
    /*throw(Exception, eh::Exception)*/;

  template <typename Traits>
  LogProcessor_var
  make_hf_log_processor_(
    const LogProcessorParams& p,
    const LogProcessingParamsHFTypeOptional& log_proc_params
  )
    /*throw(Exception, eh::Exception)*/;

  template <typename LogProcTraits, typename LogProcessingParams>
  void
  init_log_proc_info_(
    const xsd::cxx::tree::optional<LogProcessingParams>& log_proc_params
  )
    /*throw(Exception, eh::Exception)*/;

  template <typename LogProcTraits>
  void
  init_hf_log_proc_info_(
    const LogProcessingParamsHFTypeOptional& log_proc_params
  )
    /*throw(Exception, eh::Exception)*/;

  template <class LOG_PROC_TRAITS_>
  void
  init_deferrable_log_proc_info(
    const PostgresConnectionFactoryImpl_var& pg_conn_factory,
    const LogProcessingParamsDeferrableCSVTypeOptional& log_proc_params
  )
    /*throw(eh::Exception)*/;

  class CheckLogsCondition
  {
  public:
    CheckLogsCondition(LogGeneralizerImpl* log_generalizer) noexcept
      : log_generalizer_(log_generalizer)
    {}

    bool
    operator ()() const noexcept
    {
      return !log_generalizer_->db_enabled() || log_generalizer_->db_active();
    }
  private:
    LogGeneralizerImpl* log_generalizer_;
  };

  class CheckLogsAlways
  {
  public:
    CheckLogsAlways(LogGeneralizerImpl*) noexcept
    {}

    bool
    operator ()() const noexcept
    {
      return true;
    }
  };

  void
  check_logs(const LogProcThreadInfo_var& context) noexcept;

  class DbDumpTimeoutGuard;

  void
  move_deferred_logs(
    const char* log_base_name,
    const FileReceiver_var& file_rcvr,
    const FileReceiver_var& def_file_rcvr
  )
    noexcept;

  typedef Generics::GoalTask BaseTask;
  typedef ReferenceCounting::SmartPtr<BaseTask> PlannerTask_var;

  template <typename WorkCondition>
  class GenericCheckTask : public BaseTask
  {
  public:
    GenericCheckTask(
      LogGeneralizerImpl* log_generalizer,
      const LogProcThreadInfo_var& log_proc_thread_info)
      : BaseTask(log_generalizer->scheduler_, log_generalizer->task_runner_),
        log_generalizer_(log_generalizer),
        log_proc_thread_info_(log_proc_thread_info),
        work_allowed_(log_generalizer)
    {}

    virtual void
    execute() noexcept
    {
      if (work_allowed_())
      {
        log_generalizer_->check_logs(log_proc_thread_info_);
      }
      try
      {
        schedule(Generics::Time::get_time_of_day() +
          log_proc_thread_info_->CHECK_LOGS_PERIOD);
      }
      catch (const eh::Exception& ex)
      {
        Stream::Error es;
        es << FNT << ": an eh::Exception has been caught while "
          "rescheduling the task: " << ex.what();
        log_generalizer_->callback_->critical(es.str());
      }
    }

  private:
    LogGeneralizerImpl* log_generalizer_;
    LogProcThreadInfo_var log_proc_thread_info_;
    WorkCondition work_allowed_;
  };

  typedef GenericCheckTask<CheckLogsCondition> CheckLogsTask;
  typedef GenericCheckTask<CheckLogsAlways> CheckDeferrableLogsTask;

  typedef Generics::TaskGoal TaskBase;
  typedef ReferenceCounting::SmartPtr<TaskBase> Task_var;

  class MoveLogsTask: public TaskBase
  {
  public:
    MoveLogsTask(
      LogGeneralizerImpl* log_generalizer,
      const LogProcThreadInfo_var& stat_proc_info,
      const LogProcThreadInfo_var& deferred_stat_proc_info)
      : Generics::TaskGoal(log_generalizer->task_runner_),
        log_generalizer_(log_generalizer),
        stat_proc_info_(stat_proc_info),
        deferred_stat_proc_info_(deferred_stat_proc_info)
    {}

    virtual void
    execute() noexcept;

  private:
    LogGeneralizerImpl* log_generalizer_;
    LogProcThreadInfo_var stat_proc_info_;
    LogProcThreadInfo_var deferred_stat_proc_info_;
  };

  typedef Generics::Goal MsgBase;
  typedef ReferenceCounting::SmartPtr<MsgBase> Msg_var;

  class ClearExpiredHitsFilterDataMsg:
    public MsgBase,
    public ReferenceCounting::AtomicImpl
  {
  public:
    ClearExpiredHitsFilterDataMsg(LogGeneralizerImpl* log_generalizer);

    virtual void deliver() /*throw(eh::Exception)*/;

  private:
    LogGeneralizerImpl* log_generalizer_;
  };

  void clear_expired_hits_data() noexcept;

  void schedule_clear_expired_msg() noexcept;

  typedef Sync::Policy::PosixThread SyncPolicy_;
  typedef SyncPolicy_::Mutex Lock_;
  typedef SyncPolicy_::WriteGuard Guard_;

  Generics::ActiveObjectCallback_var callback_;
  Logging::Logger_var logger_;

  Generics::TaskRunner_var task_runner_;
  Generics::Planner_var scheduler_;
  std::string in_logs_dir_;
  std::string out_logs_dir_;
  std::string xsearch_stat_url_;

  unsigned long days_to_keep_;

  HitsFilter_var hits_filter_;

  ProcStatImpl_var proc_stat_impl_;

  PostgresConnectionFactoryImpl_var pg_conn_factory_impl_;
  Generics::CompositeActiveObject_var db_conn_factory_composite_;

  HTTP::HttpInterface_var http_interface_;

  LogGeneralizerStatMapBundle_var log_generalizer_stat_map_bundle_;

  FileReceiverInterrupter_var fr_interrupter_;

  Lock_ start_stop_upload_lock_;
};

typedef ReferenceCounting::SmartPtr<LogGeneralizerImpl>
  LogGeneralizerImpl_var;

inline
bool
LogGeneralizerImpl::db_enabled() const
{
  return db_conn_factory_composite_.in();
}

inline
bool
LogGeneralizerImpl::db_active()
{
  return db_conn_factory_composite_->active();
}

inline
LogGeneralizerImpl::ClearExpiredHitsFilterDataMsg::
ClearExpiredHitsFilterDataMsg(
  LogGeneralizerImpl* log_generalizer
)
:
  log_generalizer_(log_generalizer)
{
}

inline
void
LogGeneralizerImpl::ClearExpiredHitsFilterDataMsg::deliver()
  /*throw(eh::Exception)*/
{
  log_generalizer_->clear_expired_hits_data();
  log_generalizer_->schedule_clear_expired_msg();
}

inline
bool
LogGeneralizerImpl::get_db_state()
  /*throw(Commons::DbStateChanger::NotSupported)*/
{
  if (db_enabled())
  {
    return db_active();
  }
  else
  {
    throw Commons::DbStateChanger::NotSupported("");
  }
}

template <class LOG_PROC_PARAMS_TYPE_>
inline
unsigned
LogGeneralizerImpl::calc_max_number_of_threads(
  const xsd::cxx::tree::optional<LOG_PROC_PARAMS_TYPE_> &log_proc_params
)
{
  return log_proc_params.present() ?
    log_proc_params.get().max_upload_task_count() + 1 : 0;
}

template <typename Traits, typename LogProcessingParams>
LogProcessor_var
LogGeneralizerImpl::make_log_processor_(
  const LogProcessorParams& p,
  const xsd::cxx::tree::optional<LogProcessingParams>& /*log_proc_params*/) //FIXME
  /*throw(eh::Exception)*/
{
  return
    new typename Traits::LogProcessorType(
      p.log_proc_thread_info, p.in_dir, p.out_dir,
      p.bundle_params, proc_stat_impl_, fr_interrupter_);
}

template <typename Traits>
LogProcessor_var
LogGeneralizerImpl::make_log_processor_(
  const LogProcessorParams& p,
  const LogProcessingParamsXSearchTypeOptional& log_proc_params)
  /*throw(Exception, eh::Exception)*/
{
  static const char UPLOAD_TYPE_CSV[] = "csv";

  if (log_proc_params.get().upload_type() == UPLOAD_TYPE_CSV)
  {
    return
      new typename Traits::LogCsvProcessorType(
        p.log_proc_thread_info, p.in_dir, p.out_dir,
        p.bundle_params, proc_stat_impl_, fr_interrupter_);
  }
  Stream::Error es;
  es << __PRETTY_FUNCTION__ << ": Unknown or unsupported upload_type: " <<
    log_proc_params.get().upload_type();
  throw Exception(es);
}

template <typename Traits>
LogProcessor_var
LogGeneralizerImpl::make_hf_log_processor_(
  const LogProcessorParams& p,
  const LogProcessingParamsHFTypeOptional& log_proc_params
)
  /*throw(Exception, eh::Exception)*/
{
  const HitsFilteringParamsType& hf_params =
    log_proc_params.get().HitsFiltering();

  unsigned char min_count = hf_params.min_count();
  const std::string& file_prefix = hf_params.file_prefix();
  unsigned long table_size = hf_params.table_size();
  days_to_keep_ = hf_params.days_to_keep();

  hits_filter_ = new HitsFilter(min_count, file_prefix.c_str(), table_size);

  return
    new typename Traits::LogProcessorType(
      p.log_proc_thread_info, p.in_dir, p.out_dir,
      p.bundle_params,
      hits_filter_, proc_stat_impl_, fr_interrupter_);
}

template <typename LogProcTraits, typename LogProcessingParams>
void
LogGeneralizerImpl::init_log_proc_info_(
  const xsd::cxx::tree::optional<LogProcessingParams>& log_proc_params)
  /*throw(Exception, eh::Exception)*/
{
  if (!log_proc_params.present())
  {
    return;
  }
  LogProcessorParams params =
  {
    {
      log_proc_params.get().max_size(),
    },
    in_logs_dir_ + LogProcTraits::log_base_name(),
    out_logs_dir_ + LogProcTraits::log_base_name(),
    ProcessingContexts::create<LogProcTraits>(
      log_proc_params, params.in_dir, logger_,
        task_runner_, scheduler_, callback_, proc_stat_impl_)
  };

  params.log_proc_thread_info->log_processor = db_enabled() ?
    make_log_processor_<LogProcTraits>(
      params, log_proc_params) :
    make_log_processor_<typename LogProcTraits::Inactive, LogProcessingParams>(
      params, log_proc_params);

  PlannerTask_var check_logs_task(
    new CheckLogsTask(this, params.log_proc_thread_info));
  check_logs_task->deliver();
}

template <typename LogProcTraits>
void
LogGeneralizerImpl::init_hf_log_proc_info_(
  const LogProcessingParamsHFTypeOptional& log_proc_params
)
  /*throw(Exception, eh::Exception)*/
{
  if (!log_proc_params.present())
  {
    return;
  }
  LogProcessorParams params =
  {
    {
      log_proc_params.get().max_size(),
    },
    in_logs_dir_ + LogProcTraits::log_base_name(),
    out_logs_dir_ + LogProcTraits::log_base_name(),
    ProcessingContexts::create<LogProcTraits>(
      log_proc_params, params.in_dir, logger_,
        task_runner_, scheduler_, callback_, proc_stat_impl_)
  };

  params.log_proc_thread_info->log_processor = db_enabled() ?
    make_hf_log_processor_<LogProcTraits>(params, log_proc_params) :
    make_hf_log_processor_<typename LogProcTraits::Inactive>(params,
      log_proc_params);

  PlannerTask_var
    check_logs_task(new CheckLogsTask(this, params.log_proc_thread_info));
  check_logs_task->deliver();

  clear_expired_hits_data();
  schedule_clear_expired_msg();
}

} // namespace LogProcessing
} // namespace AdServer

namespace Stream::MemoryStream
{
  template<>
  struct ToCharsLenHelper<xsd::AdServer::Configuration::LogUploadType4>
  {
    size_t
    operator()(const xsd::AdServer::Configuration::LogUploadType4&) noexcept
    {
      // TODO
      return 0;
    }
  };

  template<>
  struct ToCharsHelper<xsd::AdServer::Configuration::LogUploadType4>
  {
    std::to_chars_result
    operator()(char*, char* last, const xsd::AdServer::Configuration::LogUploadType4&) noexcept
    {
      // TODO
      return {last, std::errc::value_too_large};
    }
  };

  template<>
  struct ToStringHelper<xsd::AdServer::Configuration::LogUploadType4>
  {
    std::string
    operator()(const xsd::AdServer::Configuration::LogUploadType4&) noexcept
    {
      // TODO
      return "";
    }
  };

  template<typename Elem, typename Traits, typename Allocator,
    typename AllocatorInitializer, const size_t SIZE>
  struct OutputMemoryStreamHelper<Elem, Traits, Allocator, AllocatorInitializer,
    SIZE, xsd::AdServer::Configuration::LogUploadType4>
  {
    OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>&
    operator()(OutputMemoryStream<Elem, Traits, Allocator,
      AllocatorInitializer, SIZE>& ostr, const xsd::AdServer::Configuration::LogUploadType4& arg)
    {
      typedef typename xsd::AdServer::Configuration::LogUploadType4 ArgT;
      return OutputMemoryStreamHelperImpl(ostr, arg,
        ToCharsLenHelper<ArgT>(), ToCharsHelper<ArgT>(), ToStringHelper<ArgT>());
    }
  };
}

#endif /* AD_SERVER_LOG_PROCESSING_LOG_GENERALIZER_IMPL_HPP */
