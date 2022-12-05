#include <String/TextTemplate.hpp>

#include <Commons/DelegateTaskGoal.hpp>
#include <LogProcessing/SyncLogs/Utils.hpp>

#include "NullUserOperationProcessor.hpp"
#include "UserOperationGeneratorImpl.hpp"

namespace AdServer
{
namespace UserInfoSvcs
{
  namespace Aspect
  {
    const char USER_OPERATION_GENERATOR[] = "UserOperationGenerator";
  }

  namespace TemplateParams
  {
    const String::SubString MARKER("##", 2);
    const char IN_PATH[] = "IN_PATH";
    const char TEMP_PATH[] = "TEMP_PATH";
  }

  namespace
  {
    const char USER_OP_OUT_DIR[] = "UserOp";

    class Interrupter :
      public ReferenceCounting::AtomicImpl,
      public Generics::SimpleActiveObject
    {
    private:
      virtual
      ~Interrupter() noexcept
      {}
    };
  }

  /*
   * UserOperationGeneratorImpl
   */
  UserOperationGeneratorImpl::UserOperationGeneratorImpl(
    const ConfigPtr& config,
    Generics::ActiveObjectCallback* callback,
    Logging::Logger* logger)
    /*throw(Exception)*/
    : config_(config),
      callback_(ReferenceCounting::add_ref(callback)),
      logger_(ReferenceCounting::add_ref(logger)),
      task_runner_(new Generics::TaskRunner(callback_, config_->work_threads())),
      scheduler_(new Generics::Planner(callback_))
  {
    UserOperationProcessor_var null_user_operation_processor =
      new NullUserOperationProcessor();
    const xsd::AdServer::Configuration::OutLogsType&
      out_logs_config = config_->OutLogs();

    user_operation_saver_ = new UserOperationSaver(
      callback,
      logger_,
      (out_logs_config.log_root() + USER_OP_OUT_DIR).c_str(),
      USER_OP_OUT_DIR,
      out_logs_config.common_chunks_number(),
      nullptr, // file controller
      null_user_operation_processor);

    add_child_object(user_operation_saver_);
    add_child_object(task_runner_);
    add_child_object(scheduler_);

    flush_user_operation_saver_();

    task_runner_->enqueue_task(
      AdServer::Commons::make_delegate_goal_task(
        std::bind(&UserOperationGeneratorImpl::load_and_sync_snapshot_task_, this),
        nullptr));

    interruper_ = new Interrupter();
    add_child_object(interruper_);
  }

  void
  UserOperationGeneratorImpl::sync_snapshot_() /*throw(Exception)*/
  {
    static const char* FUN = "UserOperationGeneratorImpl::sync_snapshot_";

    try
    {
      logger_->log(String::SubString("sync_snapshot_ started"),
        Logging::Logger::TRACE,
        Aspect::USER_OPERATION_GENERATOR);

      if (!current_snapshot_)
      {
        current_snapshot_ =
          new UidChannelSnapshot(config_->snapshot_path().c_str());
        current_snapshot_->load();
      }

      UidChannelSnapshot_var temp_snapshot =
        new UidChannelSnapshot(config_->temp_snapshot_path().c_str());
      temp_snapshot->load();

      const std::vector<UidChannelSnapshot::Operation> operations =
        current_snapshot_->sync(*temp_snapshot, config_->refresh_period(), interruper_);
      std::size_t processed_count = 0;

      for (auto op = operations.begin();
           op != operations.end() && interruper_->active(); ++op)
      {
        task_runner_->enqueue_task(
          AdServer::Commons::make_delegate_goal_task(
            std::bind(
              &UserOperationGeneratorImpl::execute_operation_,
              this,
              *op,
              std::ref(processed_count)),
            nullptr));
      }

      {
        Sync::ConditionalGuard lock(cond_);

        while (active() && processed_count < operations.size())
        {
          lock.wait();
        }
      }

      logger_->log(String::SubString("sync_snapshot_ finished"),
        Logging::Logger::TRACE,
        Aspect::USER_OPERATION_GENERATOR);
    }
    catch (eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": got eh::Exception: " << ex.what();
      throw Exception(ostr.str());
    }
  }

  void
  UserOperationGeneratorImpl::load_temp_snapshot_(
    const char* str_command_template)
    /*throw(Exception)*/
  {
    static const char* FUN = "UserOperationGeneratorImpl::load_temp_snapshot_";

    try
    {
      String::TextTemplate::IStream command_template;

      Stream::Parser istr(str_command_template);
      command_template.init(
        istr,
        TemplateParams::MARKER,
        TemplateParams::MARKER);

      String::TextTemplate::Args command_args;
      command_args[TemplateParams::IN_PATH] = config_->in_snapshot_path();
      command_args[TemplateParams::TEMP_PATH] = config_->temp_snapshot_path();

      const std::string command = command_template.instantiate(command_args);
      std::string output = "";
      const int exit_code =
        LogProcessing::Utils::run_cmd(command.c_str(), true, output);

      if (exit_code != 0)
      {
        Stream::Error ostr;
        ostr << FUN << ": command: '" <<
          command << "' return error (" << exit_code << "), output: '" <<
          output << "'";
        throw Exception(ostr);
      }

      if (logger_->log_level() >= Logging::Logger::TRACE)
      {
        std::ostringstream ostr;
        ostr<< FUN << ": new snapshot loaded by '" << command << "'";
        logger_->log(
          ostr.str(),
          Logging::Logger::TRACE,
          Aspect::USER_OPERATION_GENERATOR);
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": got eh::Exception: " << ex.what();
      throw Exception(ostr.str());
    }
  }

  void
  UserOperationGeneratorImpl::load_and_sync_snapshot_task_() noexcept
  {
    static const char* FUN = "UserOperationGeneratorImpl::sync_and_load_snapshot_task_";

    try
    {
      load_temp_snapshot_(config_->load_command().c_str());
      sync_snapshot_();
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": got eh::Exception: " << ex.what();

      logger_->log(ostr.str(),
        Logging::Logger::EMERGENCY,
        Aspect::USER_OPERATION_GENERATOR);
    }

    try
    {
      scheduler_->schedule(
        AdServer::Commons::make_delegate_goal_task(
          std::bind(&UserOperationGeneratorImpl::load_and_sync_snapshot_task_, this),
          task_runner_),
        Generics::Time::get_time_of_day() + config_->load_period());
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": reschedule task eh::Exception caught: " << ex.what();

      logger_->log(ostr.str(),
        Logging::Logger::EMERGENCY,
        Aspect::USER_OPERATION_GENERATOR);
    }
  }

  void
  UserOperationGeneratorImpl::flush_user_operation_saver_() noexcept
  {
    static const char* FUN = "UserOperationGeneratorImpl::flush_user_operation_saver_";

    user_operation_saver_->rotate();

    try
    {
      scheduler_->schedule(
        AdServer::Commons::make_delegate_goal_task(
          std::bind(&UserOperationGeneratorImpl::flush_user_operation_saver_, this),
          task_runner_),
        Generics::Time::get_time_of_day() + config_->OutLogs().UserOp().period());
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": reschedule task eh::Exception caught: " << ex.what();

      logger_->log(ostr.str(),
        Logging::Logger::EMERGENCY,
        Aspect::USER_OPERATION_GENERATOR);
    }
  }

  void
  UserOperationGeneratorImpl::execute_operation_(
    const UidChannelSnapshot::Operation& op,
    std::size_t& processed_count)
    noexcept
  {
    static const char* FUN = "UserOperationGeneratorImpl::execute_operation_";

    try
    {
      Stream::Error ostr;
      ostr << FUN << ": " << op;

      logger_->log(ostr.str(),
        Logging::Logger::TRACE,
        Aspect::USER_OPERATION_GENERATOR);

      current_snapshot_->execute(op, *user_operation_saver_, interruper_);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": got eh::Exception: " << ex.what();

      logger_->log(ostr.str(),
        Logging::Logger::EMERGENCY,
        Aspect::USER_OPERATION_GENERATOR);
    }

    Sync::ConditionalGuard lock(cond_);
    ++processed_count;
    cond_.broadcast();
  }
}
}
