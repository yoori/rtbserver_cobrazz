#include <Commons/DelegateActiveObject.hpp>
#include <Commons/DelegateTaskGoal.hpp>
#include <LogCommons/LogCommons.hpp>
#include <ProfilingCommons/PlainStorage3/FileReader.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/ConsiderMessages.hpp>

#include "ExpressionMatcherLogLoader.hpp"

namespace AdServer
{
  namespace RequestInfoSvcs
  {
    namespace
    {
      const std::size_t FILE_READER_BUFFER_SIZE = 1024 * 1024;
      const char DEFAULT_ERROR_DIR[] = "Error";
    }

    namespace Aspect
    {
      const char EXPRESSION_MATCHER_LOG_LOADER[] = "ExpressionMatcherLogLoader";
    }

    ExpressionMatcherLogLoader::ExpressionMatcherLogLoader(
      ConsiderInterface* consider_interface,
      RequestBasicChannelsProcessor* request_basic_channels_processor,
      Generics::TaskRunner* task_runner,
      Generics::Planner* scheduler,
      Logging::Logger* logger,
      Generics::ActiveObjectCallback* callback,
      unsigned threads_number,
      LogReadTraitsList log_read_traits,
      unsigned long check_logs_period)
      noexcept
      : consider_interface_(consider_interface),
        request_basic_channels_processor_(request_basic_channels_processor),
        task_runner_(ReferenceCounting::add_ref(task_runner)),
        scheduler_(ReferenceCounting::add_ref(scheduler)),
        logger_(ReferenceCounting::add_ref(logger)),
        log_read_traits_(log_read_traits)
    {
      const unsigned long max_files_in_memory = 50000;
      FileReceiverFacade::FileReceiversInitializer file_receivers;
      LogProcessing::FileReceiverInterrupter_var interruptor =
        new LogProcessing::FileReceiverInterrupter();
      add_child_object(interruptor);

      for (auto it = log_read_traits.begin(); it != log_read_traits.end(); ++it)
      {
        LogProcessing::FileReceiver_var file_receiver =
          new LogProcessing::FileReceiver(
            it->intermediate_fetch_dir.c_str(),
            max_files_in_memory,
            interruptor,
            logger_);

        file_receivers.emplace_back(it->log_type, file_receiver);

        task_runner_->enqueue_task(
          AdServer::Commons::make_delegate_task(
            std::bind(
              &ExpressionMatcherLogLoader::fetch_files_,
              this,
              *it,
              check_logs_period,
              file_receiver)));
      }

      file_receiver_facade_ = new FileReceiverFacade(file_receivers);
      add_child_object(file_receiver_facade_);

      Commons::DelegateActiveObject_var process_files_worker =
        Commons::make_delegate_active_object(
          std::bind(&ExpressionMatcherLogLoader::process_file_, this),
           callback,
           threads_number);
      add_child_object(process_files_worker);
    }

    void
    ExpressionMatcherLogLoader::fetch_files_(
      const LogReadTraits& log_traits,
      unsigned long check_logs_period,
      LogProcessing::FileReceiver_var file_receiver)
      noexcept
    {
      static const char* FUN = "ExpressionMatcherLogLoader::fetch_files_()";

      try
      {
        if (logger_->log_level() >= Logging::Logger::TRACE)
        {
          logger_->stream(Logging::Logger::TRACE,
            Aspect::EXPRESSION_MATCHER_LOG_LOADER) << FUN <<
            ": To check input logs in '" <<
            log_traits.in_path << "' for prefix '" << log_traits.prefix << "'";
        }

        file_receiver->fetch_files(
          log_traits.in_path.c_str(),
          log_traits.prefix.c_str());
      }
      catch(LogProcessing::FileReceiver::Interrupted&)
      {}
      catch(const eh::Exception& ex)
      {
        logger_->sstream(Logging::Logger::EMERGENCY,
          Aspect::EXPRESSION_MATCHER_LOG_LOADER,
          "ADS-IMPL-4014") << FUN <<
          "Can't process log files. Caught eh::Exception: " << ex.what();
      }

      try
      {
        const Generics::Time tm = Generics::Time::get_time_of_day() +
          check_logs_period;

        scheduler_->schedule(
          AdServer::Commons::make_delegate_goal_task(
            std::bind(
              &ExpressionMatcherLogLoader::fetch_files_,
              this,
              log_traits,
              check_logs_period,
              file_receiver),
            task_runner_),
          tm);
      }
      catch(const eh::Exception& ex)
      {
        logger_->sstream(Logging::Logger::EMERGENCY,
          Aspect::EXPRESSION_MATCHER_LOG_LOADER,
          "ADS-IMPL-4015") << FUN <<
          "Can't set processing task. Caught eh::Exception: " << ex.what();
      }

      logger_->log(String::SubString("Logs processed."),
        Logging::Logger::TRACE,
        Aspect::EXPRESSION_MATCHER_LOG_LOADER);
    }

    void
    ExpressionMatcherLogLoader::process_file_() noexcept
    {
      static const char* FUN = "ExpressionMatcherLogLoader::process_file_()";

      FileReceiverFacade::FileEntity file{LogType::RequestBasicChannels,
        static_cast<LogProcessing::FileReceiver::FileGuard*>(0)};
      std::size_t processed_lines_count = 0;

      try
      {
        file = file_receiver_facade_->get_eldest();
        bool terminated = false;

        if (file.file_guard)
        {
          switch (file.type)
          {
          case LogType::RequestBasicChannels:
            terminated = request_basic_channels_processor_->process_requests(
              file.file_guard,
              processed_lines_count);
            break;

          case LogType::ConsiderClick:
            terminated = process_binary_file_(
              file.file_guard,
              &ExpressionMatcherLogLoader::consider_click_,
              processed_lines_count);
            break;

          case LogType::ConsiderImpression:
            terminated = process_binary_file_(
              file.file_guard,
              &ExpressionMatcherLogLoader::consider_impression_,
              processed_lines_count);
            break;
          }

          if (terminated)
          {
            AdServer::LogProcessing::LogFileNameInfo name_info;
            parse_log_file_name(file.file_guard->file_name().c_str(), name_info);

            if (processed_lines_count > name_info.processed_lines_count)
            {
              name_info.processed_lines_count = processed_lines_count;
            }

            file_move_back_to_input_dir_(name_info, file.file_guard->full_path().c_str());
          }
          else
          {
            if(::unlink(file.file_guard->full_path().c_str()) != 0)
            {
              eh::throw_errno_exception<Exception>(
                "Can't delete file '", file.file_guard->full_path().c_str(), "'");
            }
          }
        }
      }
      catch (FileReceiverFacade::Interrupted&)
      {}
      catch (eh::Exception& ex)
      {
        logger_->sstream(Logging::Logger::EMERGENCY,
          Aspect::EXPRESSION_MATCHER_LOG_LOADER,
          "ADS-IMPL-4011") << FUN <<
          ": Can't process file '" <<
          (file.file_guard ? file.file_guard->file_name().c_str() : "") <<
          "'. Caught eh::Exception: " << ex.what();

        if (file.file_guard)
        {
          try
          {
            const LogReadTraits& log_read_traits = find_log_read_traits_(file.type);

            AdServer::LogProcessing::FileStore(
              log_read_traits.in_path, DEFAULT_ERROR_DIR).store(
                file.file_guard->full_path(), processed_lines_count);
          }
          catch (const eh::Exception &ex)
          {
            logger_->sstream(Logging::Logger::EMERGENCY,
              Aspect::EXPRESSION_MATCHER_LOG_LOADER,
              "ADS-IMPL-4012") << FUN <<
              ": Can't save files. eh::Exception: " << ex.what();
          }
        }
      }
    }

    const ExpressionMatcherLogLoader::LogReadTraits&
    ExpressionMatcherLogLoader::find_log_read_traits_(LogType log_type)
      /*throw(Exception)*/
    {
      for (auto it = log_read_traits_.begin();
           it != log_read_traits_.end(); ++it)
      {
        if (it->log_type == log_type)
        {
          return *it;
        }
      }

      throw Exception("ExpressionMatcherLogLoader::find_log_read_traits_: Unknown log type");
    }

    void
    ExpressionMatcherLogLoader::prepare_mem_buf_(
      Generics::MemBuf& membuf,
      unsigned long size)
      noexcept
    {
      if(size > membuf.capacity())
      {
        membuf.alloc(2 * size);
      }
      else if(size > membuf.size())
      {
        membuf.resize(size);
      }
    }

    bool
    ExpressionMatcherLogLoader::process_binary_file_(
      LogProcessing::FileReceiver::FileGuard* file_ptr,
      ConsiderAction action,
      std::size_t& processed_lines_count)
      /*throw(eh::Exception)*/
    {
      static const char* FUN = "ExpressionMatcherLogLoader::process_binary_file_()";

      LogProcessing::FileReceiver::FileGuard_var file(
        ReferenceCounting::add_ref(file_ptr));

      AdServer::LogProcessing::LogFileNameInfo name_info;
      LogProcessing::parse_log_file_name(file->file_name().c_str(), name_info);

      Generics::MemBuf membuf;
      ProfilingCommons::FileReader file_reader(
        file->full_path().c_str(),
        FILE_READER_BUFFER_SIZE);
      uint32_t message_size = 0;

      while(file_reader.read(&message_size, sizeof(message_size)) != 0)
      {
        if (!active())
        {
          return true;
        }

        prepare_mem_buf_(membuf, message_size);

        if(file_reader.read(membuf.data(), message_size) != message_size)
        {
          Stream::Error ostr;
          ostr << FUN << ": unexpected end of file on 'change request' operation reading";
          throw Exception(ostr);
        }

        if (processed_lines_count >= name_info.processed_lines_count)
        {
          (this->*action)(membuf, message_size);
        }

        ++processed_lines_count;
      }

      return false;
    }

    void
    ExpressionMatcherLogLoader::consider_click_(
      const Generics::MemBuf& membuf,
      uint32_t size) noexcept
    {
      ConsiderClickReader reader(membuf.data(), size);

      consider_interface_->consider_click(
        AdServer::Commons::RequestId(reader.request_id()),
        Generics::Time(reader.time()));
    }

    void
    ExpressionMatcherLogLoader::consider_impression_(
      const Generics::MemBuf& membuf,
      uint32_t size) noexcept
    {
      ConsiderImpressionReader reader(membuf.data(), size);

      consider_interface_->consider_impression(
        AdServer::Commons::UserId(reader.user_id()),
        AdServer::Commons::RequestId(reader.request_id()),
        Generics::Time(reader.time()),
        ChannelIdSet(reader.channels().begin(), reader.channels().end()));
    }

    void
    ExpressionMatcherLogLoader::file_move_back_to_input_dir_(
      const AdServer::LogProcessing::LogFileNameInfo& info,
      const char* file_name) /*throw(eh::Exception)*/
    {
      static const char* FUN =
        "ExpressionMatcherLogLoader::file_move_back_to_input_dir_()";

      std::string path;
      const char* ptr = strrchr(file_name, '/');

      if (ptr)
      {
        path.assign(file_name, ptr + 1);
      }

      const std::string new_file_name =
        AdServer::LogProcessing::restore_log_file_name(info, path);

      if (::rename(file_name, new_file_name.c_str()))
      {
        eh::throw_errno_exception<Exception>(
          "Can't move '", file_name, "' to '", new_file_name.c_str(), "'");
      }

      logger_->sstream(Logging::Logger::INFO,
        Aspect::EXPRESSION_MATCHER_LOG_LOADER) << FUN <<
        ": service was stopped, move " <<
        file_name << " to " << new_file_name;
    }
  }
}
