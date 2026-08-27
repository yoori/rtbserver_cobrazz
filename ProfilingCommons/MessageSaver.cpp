#include "MessageSaver.hpp"

#include <algorithm>

#include <Logger/ActiveObjectCallback.hpp>

#include <Commons/DelegateTaskGoal.hpp>
#include <LogCommons/LogCommons.hpp>

#include <utility>

namespace Aspect
{
  const char MESSAGE_SAVER[] = "MessageSaver";
}

namespace AdServer::ProfilingCommons
{
  namespace
  {
    unsigned long normalize_write_workers(
      unsigned long chunks_count,
      unsigned long threads_count) noexcept
    {
      if (chunks_count == 0)
      {
        return 1;
      }

      return std::min(chunks_count, std::max(1UL, threads_count));
    }
  }

  class MessageSaver::WriteWorkers final: public Generics::RefCountableSimpleActiveObject
  {
  public:
    explicit WriteWorkers(MessageSaver* owner) noexcept
      : owner_(owner)
    {}

  protected:
    ~WriteWorkers() noexcept override = default;

    void
    activate_object_() override
    {
      try
      {
        owner_->start_write_workers_();
      }
      catch(...)
      {
        state_ = AS_DEACTIVATING;
        owner_->notify_write_workers_();
        owner_->wait_write_workers_();
        throw;
      }
    }

    void
    deactivate_object_() override
    {
      owner_->notify_write_workers_();
    }

    void
    wait_object_() override
    {
      owner_->wait_write_workers_();
    }

  private:
    MessageSaver* const owner_;
  };

  MessageSaver::QueuedOperation::QueuedOperation(
    unsigned long chunk_i_val,
    std::uint32_t op_index_val,
    Generics::MemBuf&& membuf_val) noexcept
    : chunk_i(chunk_i_val),
      op_index(op_index_val),
      membuf(std::move(membuf_val))
  {}

  MessageSaver::FileHolderGuard::FileHolderGuard(FileHolder* file_holder)
    : file_holder_(ReferenceCounting::add_ref(file_holder)),
      guard_(file_holder_->lock)
  {}

  void
  MessageSaver::FileHolderGuard::write(const void* buf, unsigned long size)
    /*throw(eh::Exception)*/
  {
    file_holder_->file_writer->write(buf, size);
  }

  void
  MessageSaver::FileHolderGuard::write(const Generics::MemBuf& membuf)
    /*throw(eh::Exception)*/
  {
    uint32_t sz = membuf.size();
    file_holder_->file_writer->write(&sz, sizeof(sz));
    file_holder_->file_writer->write(membuf.data(), membuf.size());
  }

  // MessageSaver
  MessageSaver::MessageSaver(
    Logging::Logger* logger,
    const char* output_dir,
    const char* output_file_prefix,
    unsigned long chunks_count,
    const Generics::Time& flush_period,
    unsigned long threads_count)
    : logger_(ReferenceCounting::add_ref(logger)),
      output_dir_(output_dir),
      output_file_prefix_(output_file_prefix),
      flush_period_(flush_period),
      write_workers_count_(normalize_write_workers(chunks_count, threads_count)),
      callback_(new Logging::ActiveObjectCallbackImpl(
        logger,
        "ActiveObject",
        Aspect::MESSAGE_SAVER,
        "ADS-IMPL-?")),
      write_workers_active_object_(new WriteWorkers(this)),
      task_runner_(new Generics::TaskRunner(callback_, 1)),
      scheduler_(new Generics::Planner(callback_))
  {
    static const char* FUN = "MessageSaver::MessageSaver()";

    if (chunks_count == 0)
    {
      Stream::Error ostr;
      ostr << FUN << ": chunks_count is zero";
      throw Exception(ostr);
    }

    worker_conds_.reserve(write_workers_count_);
    for (unsigned long i = 0; i < write_workers_count_; ++i)
    {
      worker_conds_.emplace_back(std::make_shared<QueueCond>());
    }

    files_.reserve(chunks_count);
    for (unsigned long i = 0; i < chunks_count; ++i)
    {
      FileHolder* file_holder = new FileHolder();
      file_holder->queue_cond = worker_conds_[i % write_workers_count_];
      files_.push_back(file_holder);
    }

    try
    {
      add_child_object(write_workers_active_object_);
      add_child_object(task_runner_);
      add_child_object(scheduler_);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't init CompositeActiveObject: " << ex.what();
      throw Exception(ostr);
    }

    try
    {
      task_runner_->enqueue_task(
        Commons::make_delegate_goal_task(
          std::bind(&MessageSaver::flush_logs_, this),
          task_runner_));
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't schedule first flush task: " << ex.what();
      throw Exception(ostr);
    }
  }

  MessageSaver::~MessageSaver()
    noexcept
  {
    try
    {
      wait_pending_operations_();
      flush();
    }
    catch(const eh::Exception&)
    {}
  }

  void
  MessageSaver::write(std::size_t distrib_hash, const Generics::MemBuf& membuf)
    /*throw(eh::Exception)*/
  {
    check_background_error_();
    get_file_holder_(distrib_hash)->write(membuf);
  }

  void
  MessageSaver::write_operation(
    std::size_t distrib_hash,
    unsigned long op_index,
    Generics::MemBuf&& membuf)
    /*throw(eh::Exception)*/
  {
    check_background_error_();

    const unsigned long chunk_i = distrib_hash % files_.size();
    const uint32_t op_index_val = op_index;
    bool notify_worker = false;
    QueueCond* queue_cond = nullptr;

    pending_operations_.fetch_add(1, std::memory_order_release);
    try
    {
      FileHolder& file_holder = *files_[chunk_i];
      std::lock_guard<std::mutex> guard(file_holder.queue_lock);
      queue_cond = file_holder.queue_cond.get();
      file_holder.queue.emplace_back(chunk_i, op_index_val, std::move(membuf));
      notify_worker = queue_cond->queued_operations.fetch_add(1, std::memory_order_acq_rel) == 0;
    }
    catch(...)
    {
      if (pending_operations_.fetch_sub(1, std::memory_order_acq_rel) == 1)
      {
        pending_cond_.notify_all();
      }
      throw;
    }

    if (notify_worker)
    {
      std::lock_guard<std::mutex> lock(queue_cond->mutex);
      queue_cond->cond.notify_one();
    }
  }

  MessageSaver::FileHolderGuard_var
  MessageSaver::get_file_holder_(std::size_t hash)
    /*throw(eh::Exception)*/
  {
    wait_pending_operations_();
    check_background_error_();
    return get_file_holder_i_(hash % files_.size());
  }

  MessageSaver::FileHolderGuard_var
  MessageSaver::get_file_holder_i_(unsigned long chunk_i)
    /*throw(eh::Exception)*/
  {
    FileHolderGuard_var ret(new FileHolderGuard(files_[chunk_i]));
    FileHolder& file_holder = *files_[chunk_i];
    if (file_holder.tmp_file_name.empty())
    {
      init_file_holder_i_(file_holder, chunk_i);
    }
    return ret;
  }

  void
  MessageSaver::flush(FileNameList* dumped_files)
    /*throw(eh::Exception)*/
  {
    wait_pending_operations_();
    check_background_error_();
    for (FileHolderArray::iterator it = files_.begin(); it != files_.end(); ++it)
    {
      dump_file_holder_(**it, dumped_files);
    }
  }

  void
  MessageSaver::dump_file_holder_(FileHolder& file_holder, FileNameList* dumped_files)
    /*throw(eh::Exception)*/
  {
    static const char* FUN = "RequestOperationSaver::dump_()";

    std::string tmp_file_name;
    std::string file_name;
    std::unique_ptr<AdServer::ProfilingCommons::FileWriter> file_writer;

    {
      std::lock_guard<std::mutex> lock(file_holder.lock);
      tmp_file_name.swap(file_holder.tmp_file_name);
      file_name.swap(file_holder.file_name);
      file_writer.swap(file_holder.file_writer);
    }

    if (!tmp_file_name.empty())
    {
      file_writer->close();
      file_writer.reset(0);

      if (::rename(tmp_file_name.c_str(), file_name.c_str()) < 0)
      {
        Stream::Error ostr;
        ostr << FUN << ": can't rename '" << tmp_file_name << "' to '" << file_name << "'";
        eh::throw_errno_exception<Exception>(ostr.str());
      }

      if (dumped_files)
      {
        dumped_files->push_back(file_name);
      }
    }
  }

  void
  MessageSaver::init_file_holder_i_(FileHolder& file_holder, unsigned long chunk_i)
    /*throw(eh::Exception)*/
  {
    LogProcessing::LogFileNameInfo file_name_info(output_file_prefix_);
    file_name_info.distrib_index = chunk_i;
    file_name_info.distrib_count = files_.size();
    LogProcessing::StringPair files =
      LogProcessing::make_log_file_name_pair(file_name_info, output_dir_);
    file_holder.file_name = files.first;
    file_holder.tmp_file_name = files.second;
    file_holder.file_writer.reset(
      new AdServer::ProfilingCommons::FileWriter(file_holder.tmp_file_name.c_str(), 1024*1024));
  }

  void
  MessageSaver::flush_logs_() noexcept
  {
    static const char* FUN = "MessageSaver::flush_logs_()";

    try
    {
      flush();
    }
    catch(const eh::Exception& ex)
    {
      logger_->sstream(Logging::Logger::EMERGENCY, Aspect::MESSAGE_SAVER, "ADS-IMPL-?") << FUN <<
        ": Can't flush request operations: " << ex.what();
    }

    try
    {
      scheduler_->schedule(
        Commons::make_delegate_goal_task(std::bind(&MessageSaver::flush_logs_, this), task_runner_),
        Generics::Time::get_time_of_day() + flush_period_);
    }
    catch(const eh::Exception& ex)
    {
      logger_->sstream(Logging::Logger::EMERGENCY, Aspect::MESSAGE_SAVER, "ADS-IMPL-?") << FUN <<
        ": Can't schedule next flush task: " << ex.what();
    }
  }

  void
  MessageSaver::start_write_workers_()
  {
    write_workers_.reserve(write_workers_count_);
    for (unsigned long i = 0; i < write_workers_count_; ++i)
    {
      write_workers_.emplace_back([this, i]() { write_worker_loop_(i); });
    }
  }

  void
  MessageSaver::notify_write_workers_()
  {
    for (const auto& queue_cond : worker_conds_)
    {
      std::lock_guard<std::mutex> lock(queue_cond->mutex);
      queue_cond->cond.notify_all();
    }
  }

  void
  MessageSaver::wait_write_workers_()
  {
    for (auto& worker : write_workers_)
    {
      if (worker.joinable())
      {
        worker.join();
      }
    }
    write_workers_.clear();
  }

  void
  MessageSaver::write_worker_loop_(unsigned long worker_i) noexcept
  {
    QueueCond* const queue_cond = worker_conds_[worker_i].get();
    std::deque<QueuedOperation> operations;

    while (true)
    {
      {
        std::unique_lock<std::mutex> lock(queue_cond->mutex);
        queue_cond->cond.wait(
          lock,
          [this, queue_cond]()
          {
            return !write_workers_active_object_->active() ||
              queue_cond->queued_operations.load(std::memory_order_acquire) > 0;
          });
      }

      unsigned long processed_operations = 0;

      for (std::size_t chunk_i = worker_i; chunk_i < files_.size(); chunk_i += write_workers_count_)
      {
        FileHolder& file_holder = *files_[chunk_i];
        {
          std::lock_guard<std::mutex> guard(file_holder.queue_lock);
          if (file_holder.queue.empty())
          {
            continue;
          }

          operations.swap(file_holder.queue);
        }

        const unsigned long operations_count = operations.size();
        processed_operations += operations_count;
        queue_cond->queued_operations.fetch_sub(operations_count, std::memory_order_acq_rel);

        while (!operations.empty())
        {
          const QueuedOperation& operation = operations.front();
          try
          {
            process_queued_operation_(operation);
          }
          catch(const eh::Exception& ex)
          {
            set_background_error_(ex.what());
            logger_->sstream(
              Logging::Logger::EMERGENCY,
              Aspect::MESSAGE_SAVER,
              "ADS-IMPL-?") << "MessageSaver::write_worker_loop_(): "
              "can't write operation: " << ex.what();
          }
          catch(...)
          {
            set_background_error_("unknown write operation error");
            logger_->sstream(
              Logging::Logger::EMERGENCY,
              Aspect::MESSAGE_SAVER,
              "ADS-IMPL-?") << "MessageSaver::write_worker_loop_(): "
              "can't write operation: unknown error";
          }

          operations.pop_front();
        }

        if (pending_operations_.fetch_sub(
             operations_count,
             std::memory_order_acq_rel) == operations_count)
        {
          pending_cond_.notify_all();
        }
      }

      if (!write_workers_active_object_->active() &&
        processed_operations == 0 &&
        queue_cond->queued_operations.load(std::memory_order_acquire) == 0)
      {
        break;
      }
    }
  }

  void
  MessageSaver::process_queued_operation_(const QueuedOperation& operation)
    /*throw(eh::Exception)*/
  {
    FileHolderGuard_var file_holder_guard = get_file_holder_i_(operation.chunk_i);
    file_holder_guard->write(&operation.op_index, sizeof(operation.op_index));
    file_holder_guard->write(operation.membuf);
  }

  void
  MessageSaver::wait_pending_operations_() const noexcept
  {
    if (pending_operations_.load(std::memory_order_acquire) == 0)
    {
      return;
    }

    std::unique_lock<std::mutex> lock(pending_cond_lock_);
    pending_cond_.wait(
      lock,
      [this]()
      {
        return pending_operations_.load(std::memory_order_acquire) == 0;
      });
  }

  void
  MessageSaver::set_background_error_(const char* error) noexcept
  {
    try
    {
      if (background_error_set_.load(std::memory_order_acquire))
      {
        return;
      }

      std::lock_guard<std::mutex> lock(background_error_lock_);
      if (background_error_.empty())
      {
        background_error_ = error;
        background_error_set_.store(true, std::memory_order_release);
      }
    }
    catch(...)
    {}
  }

  void
  MessageSaver::check_background_error_() const
  {
    if (!background_error_set_.load(std::memory_order_acquire))
    {
      return;
    }

    std::lock_guard<std::mutex> lock(background_error_lock_);
    Stream::Error ostr;
    ostr << "MessageSaver background error: " << background_error_;
    throw Exception(ostr);
  }
}
