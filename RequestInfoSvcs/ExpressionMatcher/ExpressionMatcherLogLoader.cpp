#include <algorithm>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <vector>

#include <Commons/DelegateActiveObject.hpp>
#include <Commons/DelegateTaskGoal.hpp>
#include <LogCommons/LogCommons.hpp>
#include <ProfilingCommons/FileReader.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/ConsiderMessages.hpp>

#include "ExpressionMatcherLogLoader.hpp"

namespace AdServer::RequestInfoSvcs
{
  namespace
  {
    const std::size_t FILE_READER_BUFFER_SIZE = 1024 * 1024;
    const std::size_t RECORD_PROCESS_WINDOW_SIZE = 1024;
    const unsigned int RECORD_PROCESS_WAKE_PERCENT = 10;
    const char DEFAULT_ERROR_DIR[] = "Error";

    class WindowedRecordTasks
    {
    private:
      struct State
      {
        std::mutex lock;
        std::condition_variable cond;
        std::optional<std::exception_ptr> exception;
        std::vector<unsigned char> completed;
        std::size_t committed;
        std::size_t started = 0;
        std::size_t in_flight = 0;
        std::size_t completed_since_wait = 0;
        std::size_t wake_threshold;
        bool failed = false;
      };

    public:
      WindowedRecordTasks(std::size_t first_sequence, std::size_t window_size)
        : window_size_(window_size ? window_size : 1),
          state_(std::make_shared<State>())
      {
        state_->committed = first_sequence;
        state_->started = first_sequence;
        state_->completed.resize(window_size_);
        state_->wake_threshold = std::max<std::size_t>(
          1,
          window_size_ * RECORD_PROCESS_WAKE_PERCENT / 100);
      }

      ~WindowedRecordTasks()
      {
        std::unique_lock<std::mutex> lock(state_->lock);
        state_->cond.wait(lock, [this] { return state_->in_flight == 0; });
      }

      void
      start(
        std::size_t sequence,
        AdServer::Commons::StartableAwaitable<void> operation)
      {
        {
          std::lock_guard<std::mutex> guard(state_->lock);
          state_->started = std::max(state_->started, sequence + 1);
          ++state_->in_flight;
        }

        operation.start_detached(
          [state = state_, sequence](
            std::optional<std::exception_ptr> exception) mutable noexcept
          {
            std::lock_guard<std::mutex> guard(state->lock);
            if (exception)
            {
              if (!state->exception)
              {
                state->exception = std::move(exception);
              }
              state->failed = true;
            }
            else
            {
              state->completed[sequence % state->completed.size()] = 1;
              while (state->completed[
                state->committed % state->completed.size()] != 0)
              {
                state->completed[state->committed % state->completed.size()] = 0;
                ++state->committed;
              }
            }

            ++state->completed_since_wait;
            --state->in_flight;
            if (state->failed || state->in_flight == 0 ||
              state->completed_since_wait >= state->wake_threshold)
            {
              state->cond.notify_one();
            }
          });
      }

      bool
      full() const
      {
        std::lock_guard<std::mutex> guard(state_->lock);
        return state_->started - state_->committed >= window_size_;
      }

      std::size_t
      wait_progress()
      {
        std::unique_lock<std::mutex> lock(state_->lock);
        state_->cond.wait(
          lock,
          [this]
          {
            return state_->failed || state_->in_flight == 0 ||
              state_->completed_since_wait >= state_->wake_threshold;
          });

        if (state_->failed)
        {
          state_->cond.wait(lock, [this] { return state_->in_flight == 0; });
        }
        state_->completed_since_wait = 0;
        return state_->committed;
      }

      std::size_t
      drain()
      {
        std::unique_lock<std::mutex> lock(state_->lock);
        state_->cond.wait(lock, [this] { return state_->in_flight == 0; });
        return state_->committed;
      }

      void
      rethrow_exception()
      {
        std::optional<std::exception_ptr> exception;
        {
          std::lock_guard<std::mutex> guard(state_->lock);
          exception = std::move(state_->exception);
        }

        if (exception)
        {
          std::rethrow_exception(std::move(*exception));
        }
      }

    private:
      const std::size_t window_size_;
      std::shared_ptr<State> state_;
    };

    AdServer::Commons::StartableAwaitable<void>
    co_process_request_basic_channels_record(
      std::shared_ptr<AdServer::Commons::ExecutorPool> executor_pool,
      RequestBasicChannelsProcessor* processor,
      const LogProcessing::RequestBasicChannelsCollector::KeyT& key,
      const LogProcessing::RequestBasicChannelsCollector::DataT::DataT& record)
    {
      co_await AdServer::Commons::ExecutorPool::reschedule(std::move(executor_pool));
      co_await processor->co_process_request_basic_channels_record(key, record);
    }

    AdServer::Commons::StartableAwaitable<void>
    co_process_click(
      std::shared_ptr<AdServer::Commons::ExecutorPool> executor_pool,
      ConsiderInterface* processor,
      AdServer::Commons::RequestId request_id,
      Generics::Time time)
    {
      co_await AdServer::Commons::ExecutorPool::reschedule(std::move(executor_pool));
      co_await processor->co_consider_click(request_id, time);
    }

    AdServer::Commons::StartableAwaitable<void>
    co_process_impression(
      std::shared_ptr<AdServer::Commons::ExecutorPool> executor_pool,
      ConsiderInterface* processor,
      AdServer::Commons::UserId user_id,
      AdServer::Commons::RequestId request_id,
      Generics::Time time,
      ChannelIdSet channels)
    {
      co_await AdServer::Commons::ExecutorPool::reschedule(std::move(executor_pool));
      co_await processor->co_consider_impression(user_id, request_id, time, channels);
    }
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
    unsigned file_threads_number,
    std::shared_ptr<Commons::ExecutorPool> processing_executor_pool,
    LogReadTraitsList log_read_traits,
    unsigned long check_logs_period)
    noexcept
    : consider_interface_(consider_interface),
      request_basic_channels_processor_(request_basic_channels_processor),
      task_runner_(ReferenceCounting::add_ref(task_runner)),
      scheduler_(ReferenceCounting::add_ref(scheduler)),
      logger_(ReferenceCounting::add_ref(logger)),
      log_read_traits_(log_read_traits),
      processing_executor_pool_(std::move(processing_executor_pool))
  {
    const unsigned long max_files_in_memory = 50000;
    FileReceiverFacade::FileReceiversInitializer file_receivers;
    LogProcessing::FileReceiverInterrupter_var interruptor =
      new LogProcessing::FileReceiverInterrupter();
    add_child_object(interruptor.in());

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
    add_child_object(file_receiver_facade_.in());

    Commons::DelegateActiveObject_var process_files_worker =
      Commons::make_delegate_active_object(
        std::bind(&ExpressionMatcherLogLoader::process_file_, this),
        callback,
        file_threads_number);
    add_child_object(process_files_worker.in());
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
          terminated = process_request_basic_channels_file_(
            file.file_guard,
            processed_lines_count);
          break;

        case LogType::ConsiderClick:
          terminated = process_binary_file_(
            file.file_guard,
            file.type,
            processed_lines_count);
          break;

        case LogType::ConsiderImpression:
          terminated = process_binary_file_(
            file.file_guard,
            file.type,
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
          if (::unlink(file.file_guard->full_path().c_str()) != 0)
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
    for (auto it = log_read_traits_.begin(); it != log_read_traits_.end(); ++it)
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
    if (size > membuf.capacity())
    {
      membuf.alloc(2 * size);
    }
    else if (size > membuf.size())
    {
      membuf.resize(size);
    }
  }

  bool
  ExpressionMatcherLogLoader::process_request_basic_channels_file_(
    LogProcessing::FileReceiver::FileGuard* file_ptr,
    std::size_t& processed_lines_count)
    /*throw(eh::Exception)*/
  {
    LogProcessing::FileReceiver::FileGuard_var file(
      ReferenceCounting::add_ref(file_ptr));

    LogProcessing::LogFileNameInfo name_info;
    LogProcessing::parse_log_file_name(file->file_name().c_str(), name_info);

    LogProcessing::RequestBasicChannelsCollector collector;
    std::ifstream ifs(file->full_path().c_str());
    LogProcessing::RequestBasicChannelsTraits::IoHelperType(collector).load(ifs);

    std::size_t sequence = 0;
    processed_lines_count = name_info.processed_lines_count;
    WindowedRecordTasks tasks(name_info.processed_lines_count, RECORD_PROCESS_WINDOW_SIZE);
    bool terminated = false;

    for (auto coll_it = collector.begin(); coll_it != collector.end(); ++coll_it)
    {
      for (auto record_it = coll_it->second.begin(); record_it != coll_it->second.end();
        ++record_it, ++sequence)
      {
        if (sequence < name_info.processed_lines_count)
        {
          continue;
        }

        if (!active())
        {
          terminated = true;
          break;
        }

        tasks.start(
          sequence,
          co_process_request_basic_channels_record(
            processing_executor_pool_,
            request_basic_channels_processor_,
            coll_it->first,
            *record_it));

        if (tasks.full())
        {
          processed_lines_count = tasks.wait_progress();
          tasks.rethrow_exception();
        }
      }

      if (terminated)
      {
        break;
      }
    }

    processed_lines_count = tasks.drain();
    tasks.rethrow_exception();

    if (!terminated)
    {
      request_basic_channels_processor_->request_basic_channels_file_processed(
        name_info.timestamp);
    }

    return terminated;
  }

  bool
  ExpressionMatcherLogLoader::process_binary_file_(
    LogProcessing::FileReceiver::FileGuard* file_ptr,
    LogType log_type,
    std::size_t& processed_lines_count)
    /*throw(eh::Exception)*/
  {
    static const char* FUN = "ExpressionMatcherLogLoader::process_binary_file_()";

    LogProcessing::FileReceiver::FileGuard_var file(ReferenceCounting::add_ref(file_ptr));

    AdServer::LogProcessing::LogFileNameInfo name_info;
    LogProcessing::parse_log_file_name(file->file_name().c_str(), name_info);

    Generics::MemBuf membuf;
    ProfilingCommons::FileReader file_reader(file->full_path().c_str(), FILE_READER_BUFFER_SIZE);
    uint32_t message_size = 0;
    std::size_t sequence = 0;
    processed_lines_count = name_info.processed_lines_count;
    WindowedRecordTasks tasks(name_info.processed_lines_count, RECORD_PROCESS_WINDOW_SIZE);
    bool terminated = false;

    while (file_reader.read(&message_size, sizeof(message_size)) != 0)
    {
      if (!active())
      {
        terminated = true;
        break;
      }

      prepare_mem_buf_(membuf, message_size);

      if (file_reader.read(membuf.data(), message_size) != message_size)
      {
        Stream::Error ostr;
        ostr << FUN << ": unexpected end of file on 'change request' operation reading";
        throw Exception(ostr);
      }

      if (sequence >= name_info.processed_lines_count)
      {
        if (log_type == LogType::ConsiderClick)
        {
          ConsiderClickReader reader(membuf.data(), message_size);
          tasks.start(
            sequence,
            co_process_click(
              processing_executor_pool_,
              consider_interface_,
              AdServer::Commons::RequestId(reader.request_id()),
              Generics::Time(reader.time())));
        }
        else
        {
          ConsiderImpressionReader reader(membuf.data(), message_size);
          tasks.start(
            sequence,
            co_process_impression(
              processing_executor_pool_,
              consider_interface_,
              AdServer::Commons::UserId(reader.user_id()),
              AdServer::Commons::RequestId(reader.request_id()),
              Generics::Time(reader.time()),
              ChannelIdSet(reader.channels().begin(), reader.channels().end())));
        }

        if (tasks.full())
        {
          processed_lines_count = tasks.wait_progress();
          tasks.rethrow_exception();
        }
      }

      ++sequence;
    }

    processed_lines_count = tasks.drain();
    tasks.rethrow_exception();
    return terminated;
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
