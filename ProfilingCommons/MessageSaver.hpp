#ifndef PROFILINGCOMMONS_MESSAGESAVER_HPP_
#define PROFILINGCOMMONS_MESSAGESAVER_HPP_

#include <list>
#include <memory>
#include <string>
#include <vector>

#include <Generics/CompositeActiveObject.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/TaskRunner.hpp>
#include <Logger/Logger.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <Sync/SyncPolicy.hpp>

#include <ProfilingCommons/PlainStorage3/FileWriter.hpp>

namespace AdServer
{
namespace ProfilingCommons
{
  /*
   * MessageSaver
   * wrapper for divide saving of operations by chunks (distrib hash)
   */
  class MessageSaver:
    public virtual ReferenceCounting::AtomicImpl,
    public Generics::CompositeActiveObject
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    typedef std::list<std::string> FileNameList;

  public:
    MessageSaver(
      Logging::Logger* logger,
      const char* output_dir,
      const char* file_prefix,
      unsigned long chunks_count,
      const Generics::Time& flush_period);

    void
    flush(FileNameList* dumped_files = 0)
      /*throw(eh::Exception)*/;

    // can be used only for files with single operation
    void
    write(
      std::size_t distrib_hash,
      const Generics::MemBuf& membuf)
      /*throw(eh::Exception)*/;

    // save operation in format:
    //   uint32_t op_index
    //   uint32_t size
    //   byte message[size]
    void
    write_operation(
      std::size_t distrib_hash,
      unsigned long op_index,
      const Generics::MemBuf& membuf)
      /*throw(eh::Exception)*/;

  protected:
    typedef Sync::Policy::PosixThread SyncPolicy;

    struct FileHolder: public ReferenceCounting::AtomicImpl
    {
      mutable SyncPolicy::Mutex lock;
      std::string file_name;
      std::string tmp_file_name;
      std::unique_ptr<AdServer::ProfilingCommons::FileWriter> file_writer;

    protected:
      virtual ~FileHolder() noexcept {}
    };

    typedef ReferenceCounting::QualPtr<FileHolder>
      FileHolder_var;

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
      SyncPolicy::WriteGuard guard_;
      FileHolder_var file_holder_;
    };

    typedef ReferenceCounting::QualPtr<FileHolderGuard>
      FileHolderGuard_var;

    typedef std::vector<FileHolder_var> FileHolderArray;

  protected:
    virtual
    ~MessageSaver()
      noexcept;

    FileHolderGuard_var
    get_file_holder_(std::size_t hash)
      /*throw(eh::Exception)*/;

    void
    dump_file_holder_(
      FileHolder& file_holder,
      FileNameList* dumped_files)
      /*throw(eh::Exception)*/;

    void
    init_file_holder_i_(
      FileHolder& file_holder, unsigned long chunk_i)
      /*throw(eh::Exception)*/;

    void
    flush_logs_() noexcept;

  protected:
    Logging::Logger_var logger_;
    const std::string output_dir_;
    const std::string output_file_prefix_;
    const Generics::Time flush_period_;

    Generics::ActiveObjectCallback_var callback_;
    Generics::TaskRunner_var task_runner_;
    Generics::Planner_var scheduler_;
    FileHolderArray files_;
  };

  typedef ReferenceCounting::SmartPtr<MessageSaver>
    MessageSaver_var;
}
}

#endif /* PROFILINGCOMMONS_MESSAGESAVER_HPP_ */
