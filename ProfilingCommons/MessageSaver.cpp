#include "MessageSaver.hpp"

#include <Logger/ActiveObjectCallback.hpp>

#include <Commons/DelegateTaskGoal.hpp>
#include <LogCommons/LogCommons.hpp>

namespace Aspect
{
  const char MESSAGE_SAVER[] = "MessageSaver";
}

namespace AdServer
{
namespace ProfilingCommons
{
  MessageSaver::FileHolderGuard::FileHolderGuard(
    FileHolder* file_holder)
    : guard_(file_holder->lock),
      file_holder_(ReferenceCounting::add_ref(file_holder))
  {}

  void
  MessageSaver::FileHolderGuard::write(
    const void* buf, unsigned long size)
    /*throw(eh::Exception)*/
  {
    file_holder_->file_writer->write(buf, size);
  }

  void
  MessageSaver::FileHolderGuard::write(
    const Generics::MemBuf& membuf)
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
    const Generics::Time& flush_period)
    : logger_(ReferenceCounting::add_ref(logger)),
      output_dir_(output_dir),
      output_file_prefix_(output_file_prefix),
      flush_period_(flush_period),
      callback_(new Logging::ActiveObjectCallbackImpl(
        logger,
        "ActiveObject",
        Aspect::MESSAGE_SAVER,
        "ADS-IMPL-?")),
      task_runner_(new Generics::TaskRunner(callback_, 1)),
      scheduler_(new Generics::Planner(callback_))
  {
    static const char* FUN = "MessageSaver::MessageSaver()";

    files_.reserve(chunks_count);
    for(unsigned long i = 0; i < chunks_count; ++i)
    {
      files_.push_back(new FileHolder());
    }

    try
    {
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
      flush();
    }
    catch(const eh::Exception&)
    {}
  }

  void
  MessageSaver::write(
    std::size_t distrib_hash,
    const Generics::MemBuf& membuf)
    /*throw(eh::Exception)*/
  {
    get_file_holder_(distrib_hash)->write(membuf);
  }

  void
  MessageSaver::write_operation(
    std::size_t distrib_hash,
    unsigned long op_index,
    const Generics::MemBuf& membuf)
    /*throw(eh::Exception)*/
  {
    const uint32_t op_index_val = op_index;

    FileHolderGuard_var file_holder_guard = get_file_holder_(distrib_hash);
    file_holder_guard->write(&op_index_val, sizeof(op_index_val));
    file_holder_guard->write(membuf);
  }

  MessageSaver::FileHolderGuard_var
  MessageSaver::get_file_holder_(std::size_t hash)
    /*throw(eh::Exception)*/
  {
    const unsigned long chunk_i = hash % files_.size();
      // % files_.size() ~ % distrib_count
    FileHolderGuard_var ret(new FileHolderGuard(files_[chunk_i]));
    FileHolder& file_holder = *files_[chunk_i];
    if(file_holder.tmp_file_name.empty())
    {
      init_file_holder_i_(file_holder, chunk_i);
    }
    return ret;
  }

  void
  MessageSaver::flush(
    FileNameList* dumped_files)
    /*throw(eh::Exception)*/
  {
    for(FileHolderArray::iterator it = files_.begin();
        it != files_.end(); ++it)
    {
      dump_file_holder_(**it, dumped_files);
    }
  }

  void
  MessageSaver::dump_file_holder_(
    FileHolder& file_holder,
    FileNameList* dumped_files)
    /*throw(eh::Exception)*/
  {
    static const char* FUN = "RequestOperationSaver::dump_()";

    std::string tmp_file_name;
    std::string file_name;
    std::unique_ptr<AdServer::ProfilingCommons::FileWriter> file_writer;

    {
      SyncPolicy::WriteGuard lock(file_holder.lock);
      tmp_file_name.swap(file_holder.tmp_file_name);
      file_name.swap(file_holder.file_name);
      file_writer.swap(file_holder.file_writer);
    }

    if(!tmp_file_name.empty())
    {
      file_writer->close();
      file_writer.reset(0);

      if(::rename(tmp_file_name.c_str(), file_name.c_str()) < 0)
      {
        Stream::Error ostr;
        ostr << FUN << ": can't rename '" << tmp_file_name <<
          "' to '" << file_name << "'";
        eh::throw_errno_exception<Exception>(ostr.str());
      }

      if(dumped_files)
      {
        dumped_files->push_back(file_name);
      }
    }
  }

  void
  MessageSaver::init_file_holder_i_(
    FileHolder& file_holder,
    unsigned long chunk_i)
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
      new AdServer::ProfilingCommons::FileWriter(
        file_holder.tmp_file_name.c_str(),
        1024*1024));
  }

  void
  MessageSaver::flush_logs_()
    noexcept
  {
    static const char* FUN = "MessageSaver::flush_logs_()";

    try
    {
      flush();
    }
    catch(const eh::Exception& ex)
    {
      logger_->sstream(
        Logging::Logger::EMERGENCY,
        Aspect::MESSAGE_SAVER,
        "ADS-IMPL-?") << FUN <<
        ": Can't flush request operations: " << ex.what();
    }

    try
    {
      scheduler_->schedule(
        Commons::make_delegate_goal_task(
          std::bind(&MessageSaver::flush_logs_, this),
          task_runner_),
        Generics::Time::get_time_of_day() + flush_period_);
    }
    catch(const eh::Exception& ex)
    {
      logger_->sstream(
        Logging::Logger::EMERGENCY,
        Aspect::MESSAGE_SAVER,
        "ADS-IMPL-?") << FUN <<
        ": Can't schedule next flush task: " << ex.what();
    }
  }
}
}
