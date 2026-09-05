#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <Generics/CompositeActiveObject.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/TaskRunner.hpp>
#include <Logger/Logger.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

#include <ProfilingCommons/FileWriter.hpp>

namespace AdServer::ProfilingCommons
{
  /*
   * MessageSaver
   * wrapper for divide saving of operations by chunks (distrib hash)
   */
  class MessageSaver:
    public virtual Generics::RefCountableActiveObject,
    public virtual ReferenceCounting::AtomicImpl,
    public Generics::CompositeActiveObject
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    using FileNameList = std::list<std::string>;

  public:
    MessageSaver(
      Logging::Logger* logger,
      const char* output_dir,
      const char* file_prefix,
      unsigned long chunks_count,
      const Generics::Time& flush_period,
      unsigned long threads_count);

    void
    flush(FileNameList* dumped_files = 0)
      /*throw(eh::Exception)*/;

    // can be used only for files with single operation
    void
    write(std::size_t distrib_hash, const Generics::MemBuf& membuf)
      /*throw(eh::Exception)*/;

    // save operation in format:
    //   uint32_t op_index
    //   uint32_t size
    //   byte message[size]
    void
    write_operation(std::size_t distrib_hash, unsigned long op_index, Generics::MemBuf&& membuf)
      /*throw(eh::Exception)*/;

  protected:
    struct QueueCond
    {
      std::mutex mutex;
      std::condition_variable cond;
      std::atomic<unsigned long> queued_operations{0};
    };

    using QueueCondPtr = std::shared_ptr<QueueCond>;

    struct QueuedOperation
    {
      QueuedOperation(
        unsigned long chunk_i_val,
        std::uint32_t op_index_val,
        Generics::MemBuf&& membuf_val) noexcept;

      QueuedOperation(const QueuedOperation&) = delete;
      QueuedOperation& operator=(const QueuedOperation&) = delete;
      QueuedOperation(QueuedOperation&&) noexcept = default;
      QueuedOperation& operator=(QueuedOperation&&) = delete;

      unsigned long chunk_i;
      std::uint32_t op_index;
      Generics::MemBuf membuf;
    };

    struct FileHolder: public ReferenceCounting::AtomicImpl
    {
      std::mutex lock;
      std::mutex queue_lock;
      std::deque<QueuedOperation> queue;
      QueueCondPtr queue_cond;
      std::string file_name;
      std::string tmp_file_name;
      std::unique_ptr<AdServer::ProfilingCommons::FileWriter> file_writer;

    protected:
      virtual ~FileHolder() noexcept {}
    };

    using FileHolder_var = ReferenceCounting::QualPtr<FileHolder>;

    class FileHolderGuard: public ReferenceCounting::DefaultImpl<>
    {
    public:
      FileHolderGuard(FileHolder* file_holder);

      void
      write(const void* buf, unsigned long size)
        /*throw(eh::Exception)*/;

      void
      write(const Generics::MemBuf& membuf)
        /*throw(eh::Exception)*/;

    protected:
      FileHolder_var file_holder_;
      std::unique_lock<std::mutex> guard_;
    };

    using FileHolderGuard_var = ReferenceCounting::QualPtr<FileHolderGuard>;

    using FileHolderArray = std::vector<FileHolder_var>;

    class WriteWorkers;

    using WriteWorkers_var = ReferenceCounting::SmartPtr<WriteWorkers>;

  protected:
    virtual
    ~MessageSaver() noexcept;

    FileHolderGuard_var
    get_file_holder_(std::size_t hash)
      /*throw(eh::Exception)*/;

    FileHolderGuard_var
    get_file_holder_i_(unsigned long chunk_i)
      /*throw(eh::Exception)*/;

    void
    dump_file_holder_(FileHolder& file_holder, FileNameList* dumped_files)
      /*throw(eh::Exception)*/;

    void
    init_file_holder_i_(FileHolder& file_holder, unsigned long chunk_i)
      /*throw(eh::Exception)*/;

    void
    flush_logs_() noexcept;

    void
    start_write_workers_();

    void
    notify_write_workers_();

    void
    wait_write_workers_();

    void
    write_worker_loop_(unsigned long worker_i) noexcept;

    void
    process_queued_operation_(const QueuedOperation& operation)
      /*throw(eh::Exception)*/;

    void complete_pending_operations_(unsigned long operations_count) noexcept;

    void
    wait_pending_operations_() const noexcept;

    void
    set_background_error_(const char* error) noexcept;

    void
    check_background_error_() const;

  protected:
    Logging::Logger_var logger_;
    const std::string output_dir_;
    const std::string output_file_prefix_;
    const Generics::Time flush_period_;
    const unsigned long write_workers_count_;

    Generics::ActiveObjectCallback_var callback_;
    WriteWorkers_var write_workers_active_object_;
    Generics::TaskRunner_var task_runner_;
    Generics::Planner_var scheduler_;
    FileHolderArray files_;

    std::atomic<unsigned long> pending_operations_{0};
    mutable std::mutex pending_cond_lock_;
    mutable std::condition_variable pending_cond_;
    std::atomic<bool> background_error_set_{false};
    mutable std::mutex background_error_lock_;
    std::string background_error_;
    std::vector<QueueCondPtr> worker_conds_;
    std::vector<std::thread> write_workers_;
  };

  using MessageSaver_var = ReferenceCounting::SmartPtr<MessageSaver>;
}
