#include <LogCommons/ConvLogger.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>

#include "ConversionProcessor.hpp"

namespace AdServer
{
namespace RequestInfoSvcs
{
  namespace
  {
    namespace Aspect
    {
      const char CONVERSION_PROCESSOR[] = "ActionProcessor";
    }
  }

  // ConversionProcessor::ConvLogger
  class ConversionProcessor::ConvLogger:
    public AdServer::LogProcessing::LogHolderPoolData<
      AdServer::LogProcessing::ConvTraits,
      AdServer::LogProcessing::DistributionSavePolicy<
        AdServer::LogProcessing::ConvTraits> > // log distribution enabled
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    ConvLogger(const AdServer::LogProcessing::LogFlushTraits& flush_traits)
      /*throw(eh::Exception)*/
      : AdServer::LogProcessing::LogHolderPoolData<
          AdServer::LogProcessing::ConvTraits,
          AdServer::LogProcessing::DistributionSavePolicy<
            AdServer::LogProcessing::ConvTraits> >(
              flush_traits)
    {}

    void
    process_request(
      const MatchRequestProcessor::MatchInfo& match_info,
      unsigned long action_id)
      /*throw(Exception)*/;

  protected:
    virtual
    ~ConvLogger() noexcept = default;
  };

  // ConversionProcessor::ConvLogger impl
  void
  ConversionProcessor::ConvLogger::
  process_request(
    const MatchRequestProcessor::MatchInfo& match_info,
    unsigned long action_id)
    /*throw(Exception)*/
  {
    static const char* FUN = "ConversionProcessor::ConvLogger::process_request()";

    try
    {
      CollectorT::DataT data(
        match_info.time,
        match_info.user_id,
        action_id
        );

      add_record(data);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": eh::Exception caught: " << ex.what();
      throw Exception(ostr);
    }
  }

  // ConversionProcessor::FlushLogsTaskMessage
  ConversionProcessor::FlushLogsTaskMessage::FlushLogsTaskMessage(
    ConversionProcessor* action_processor,
    Generics::TaskRunner* task_runner)
    /*throw(eh::Exception)*/
    : Generics::TaskGoal(task_runner),
      action_processor_(action_processor)
  {}

  void
  ConversionProcessor::FlushLogsTaskMessage::execute() noexcept
  {
    action_processor_->flush_logs_();
  }

  // ConversionProcessor impl
  ConversionProcessor::ConversionProcessor(
    Logging::Logger* logger,
    Generics::ActiveObjectCallback* callback,
    const AdServer::LogProcessing::LogFlushTraits& conv_flush_traits)
    /*throw(eh::Exception)*/
    : logger_(ReferenceCounting::add_ref(logger)),
      callback_(ReferenceCounting::add_ref(callback)),
      task_runner_(new Generics::TaskRunner(callback_, 1))
  {
    conv_logger_ = new ConvLogger(conv_flush_traits);
    add_child_log_holder(conv_logger_);

    Generics::Task_var task = new FlushLogsTaskMessage(this, task_runner_);
    task_runner_->enqueue_task(task);
  }

  void
  ConversionProcessor::process_request(const InventoryInfo& /*inv_info*/)
    /*throw(InventoryActionProcessor::Exception)*/
  {
    // delegate to next processor
    /*
    inv_processor_->process_request(
      match_info,
      inv_info,
      action_id,
      action_ccgs);
    */
  }

  void
  ConversionProcessor::process_user(const InventoryUserInfo&)
    /*throw(InventoryActionProcessor::Exception)*/
  {
    /*
    inv_processor_->process_user(inv_user_info);
    */
  }

  void
  ConversionProcessor::flush_logs_() noexcept
  {
    static const char* FUN = "ConversionProcessor::flush_logs_()";

    const Generics::Time LOGS_DUMP_ERROR_RESCHEDULE_PERIOD(2);

    logger_->log(
      String::SubString("flush logs"),
      Logging::Logger::TRACE,
      Aspect::CONVERSION_PROCESSOR);

    Generics::Time next_flush;

    try
    {
      next_flush = conv_logger_->flush_if_required(
        Generics::Time::get_time_of_day());
    }
    catch (const eh::Exception& ex)
    {
      next_flush = Generics::Time::get_time_of_day() +
        LOGS_DUMP_ERROR_RESCHEDULE_PERIOD;

      Stream::Error ostr;
      ostr << FUN << ": eh::Exception caught on flush logs:" << ex.what();

      logger_->log(ostr.str(),
        Logging::Logger::ERROR,
        Aspect::CONVERSION_PROCESSOR,
        "ADS-IMPL-250");
    }

    if(logger_->log_level() >= Logging::Logger::TRACE)
    {
      Stream::Error ostr;
      ostr << FUN << ": logs flushed, next flush at " << next_flush.get_gm_time();
      logger_->log(
        ostr.str(),
        Logging::Logger::ERROR,
        Aspect::CONVERSION_PROCESSOR);
    }

    if(next_flush != Generics::Time::ZERO)
    {
      try
      {
        Generics::Goal_var task =
          new FlushLogsTaskMessage(this, task_runner_);

        scheduler_->schedule(task, next_flush);
      }
      catch (const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": Can't schedule next flush task. "
          "eh::Exception caught:" << ex.what();

        logger_->log(ostr.str(),
          Logging::Logger::ERROR,
          Aspect::CONVERSION_PROCESSOR,
          "ADS-IMPL-251");
      }
    }
  }
}
}
