// @file SyncLogs/FeedRouteProcessor.cpp

#include <Generics/DirSelector.hpp>
#include <Commons/PathManip.hpp>

#include "FileRouter.hpp"
#include "RSyncFileRouter.hpp"

#include "FeedRouteProcessor.hpp"

namespace Aspect
{
  const char SYNC_LOGS[] = "SyncLogs";
}

namespace AdServer
{
namespace LogProcessing
{
  namespace
  {
    const char INTERMEDIATE_DIR[] = "Intermediate";

    class FileEntryListCreator
    {
    public:
      typedef BasicFeedRouteProcessor::FileEntries Container;

    public:
      FileEntryListCreator(Container& container) noexcept
        : container_(container)
      {}

      void
      operator ()(
        const char* full_path,
        const struct stat&)
        /*throw(eh::Exception)*/
      {
        container_.push_back(new FileReceiver::FileGuard(full_path));
      }

    private:
      Container& container_;
    };

    class FileEntryLess :
      std::binary_function<
        BasicFeedRouteProcessor::FileEntry,
        BasicFeedRouteProcessor::FileEntry,
        bool>
    {
    public:
      bool operator() (
        const BasicFeedRouteProcessor::FileEntry& arg1,
        const BasicFeedRouteProcessor::FileEntry& arg2) const
        noexcept
      {
        return (arg1->file_name() < arg2->file_name());
      }
    };
  }

  /** BasicFeedRouteProcessor */
  BasicFeedRouteProcessor::BasicFeedRouteProcessor(
    Utils::ErrorPool* error_logger,
    const LocalInterfaceChecker& host_checker,
    const char* src_files_pattern,
    RouteBasicHelper* destination_host_router,
    const char* dst_dir,
    unsigned long tries_per_file,
    const char* local_copy_command_templ,
    CommandType local_copy_command_type,
    const char* remote_copy_command_templ,
    CommandType remote_copy_command_type,
    bool parse_source,
    bool unlink_source,
    bool interruptible,
    SchedType feed_type,
    const char* post_command,
    FetchType fetch_type)
    /*throw(Exception)*/
    : error_logger_(error_logger),
      parse_source_(parse_source)
  {
    static const char* FUN = "BasicFeedRouteProcessor::BasicFeedRouteProcessor";

    try
    {
      if(!parse_source_ && feed_type != ST_ROUND_ROBIN)
      {
        Stream::Error err;
        err << "Have to parse source for type ";
        switch(feed_type)
        {
        case ST_BY_NUMBER:
          err << "ST_BY_NUMBER";
          break;
        case ST_HASH:
          err << "ST_HASH";
          break;
        case ST_DEFINITEHASH:
          err << "ST_DEFINITEHASH";
          break;
        default:
          assert(0);
          break;
        }
        throw Exception(err);
      }

      FileRouter_var local_file_router;

      if(local_copy_command_type == CT_RSYNC)
      {
        local_file_router =
          new RSyncFileRouter(local_copy_command_templ, post_command);
      }
      else if(local_copy_command_type == CT_GENERIC)
      {
        local_file_router =
          new AppFileRouter(local_copy_command_templ, post_command);
      }

      FileRouter_var remote_file_router;

      if(remote_copy_command_type == CT_RSYNC)
      {
        remote_file_router =
          new RSyncFileRouter(remote_copy_command_templ, post_command);
      }
      else if(remote_copy_command_type == CT_GENERIC)
      {
        remote_file_router =
          new AppFileRouter(remote_copy_command_templ, post_command);
      }

      PathManip::split_path(
        src_files_pattern,
        &src_dir_,
        &file_mask_,
        !parse_source);

      if(parse_source)
      {
        if(file_mask_.empty())
        {
          Stream::Error ostr;
          ostr << FUN << ": '" << src_files_pattern << "' not a file name mask";
          throw Exception(ostr);
        }

        try
        {
          String::TextTemplate::Args templ_args;
          templ_args[TemplateParams::HASH] = "[0-9]*";
          String::TextTemplate::String templ(
            file_mask_,
            TemplateParams::MARKER,
            TemplateParams::MARKER);
          file_mask_ = templ.instantiate(templ_args);
        }
        catch(const eh::Exception& e)
        {
          Stream::Error ostr;
          ostr << FUN << ": eh::Exception on instancing HASH template. "
            "Pattern: " << file_mask_  << " : " << e.what();
          throw Exception(ostr);
        }
      }

      configure_fetch_mode_(fetch_type);

      const bool commit_mode_flag = post_command &&
        ::strstr(post_command, "touch") &&
        ::strstr(post_command, "commit");

      mover_ = new FeedRouteMover(
        error_logger,
        host_checker,
        dst_dir,
        tries_per_file,
        destination_host_router,
        local_file_router,
        remote_file_router,
        parse_source,
        unlink_source,
        interruptible,
        commit_mode_flag);

      add_child_object(mover_);
    }
    catch(const FileRouter::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught FileRouter::Exception: " << e.what();
      throw Exception(ostr);
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": eh::Exception. : " << e.what();
      throw Exception(ostr);
    }
  }

  void BasicFeedRouteProcessor::process() noexcept
  {
    static const char* FUN = "BasicFeedRouteProcessor::process(): ";

    for(StringList::iterator unlink_it = unlink_files_.begin();
        unlink_it != unlink_files_.end(); )
    {
      try
      {
        Utils::unlink_file(unlink_it->c_str());
        unlink_files_.erase(unlink_it++);
      }
      catch(const Utils::NotFound&)
      {
        unlink_files_.erase(unlink_it++);
      }
      catch(const eh::Exception& ex)
      {
        ++unlink_it;

        Stream::Error ostr;
        ostr << FUN << ": can't unlink source file: " << ex.what();
        error_logger_->log(
          ostr.str(),
          Logging::Logger::ERROR,
          Aspect::SYNC_LOGS,
          "ADS-IMPL-201");
      }
    }

    if(unlink_files_.empty())
    {
      try
      {
        if (error_logger_->log_level() >= Logging::Logger::TRACE)
        {
          Stream::Error ostr;
          ostr << FUN << ": searching for files matching the '" <<
            src_dir_ << "/" << file_mask_ << "' pattern.";
          error_logger_->log(ostr.str(),
            Logging::Logger::TRACE, Aspect::SYNC_LOGS);
        }

        FileEntries process_files;

        if (parse_source_)
        {
          make_file_list_for_feed_(process_files);
        }
        else
        {
          process_files.push_back(
            new FileReceiver::FileGuard(file_mask_.c_str()));
        }

        std::string src_dir = src_dir_;

        if (file_receiver_)
        {
          src_dir += INTERMEDIATE_DIR;
          src_dir += '/';
        }

        process_files_(unlink_files_, process_files, src_dir.c_str());
      }
      catch(const Utils::UnlinkException& ex)
      {
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": can't do processing: " << ex.what();
        error_logger_->log(ostr.str(),
          Logging::Logger::ERROR,
          Aspect::SYNC_LOGS,
          "ADS-IMPL-202");
      }
    }
  }

  BasicFeedRouteProcessor::FetchType
  BasicFeedRouteProcessor::get_fetch_type(const std::string& str)
    /*throw(Exception)*/
  {
    if (str == "full")
    {
      return FT_FULL;
    }
    else if (str == "new")
    {
      return FT_NEW;
    }
    else if (str == "commited")
    {
      return FT_COMMITED;
    }

    throw Exception(
      str + " is invalid value for fetch type of feed route");
  }

  void
  BasicFeedRouteProcessor::configure_fetch_mode_(FetchType fetch_type)
    /*throw(eh::Exception)*/
  {
    if (fetch_type == FT_NEW || fetch_type == FT_COMMITED)
    {
      const std::string intermediate_path = src_dir_ + INTERMEDIATE_DIR;
      const std::size_t MAX_FILES_TO_STORE = 50000;

      file_receiver_ = new FileReceiver(
        intermediate_path.c_str(),
        MAX_FILES_TO_STORE,
        0,  //interrupter
        0); // logger
    }
  }

  void
  BasicFeedRouteProcessor::make_file_list_for_feed_(
    FileEntries& files_in_dir) const
    /*throw(eh::Exception)*/
  {
    static const char* FUN = "FeedRouteProcessor::make_file_list_for_feed_()";

    if (file_receiver_)
    {
      file_receiver_->fetch_files(src_dir_.c_str(), file_mask_.c_str());

      while (!file_receiver_->empty())
      {
        std::string tmp;
        const FileReceiver::FileGuard_var file =
          file_receiver_->get_eldest(tmp);

        if (file)
        {
          files_in_dir.push_back(file);
        }
      }
    }
    else
    {
      FileEntryListCreator list_creator(files_in_dir);
      Generics::DirSelect::directory_selector(
        src_dir_.c_str(), list_creator, file_mask_.c_str(),
        Generics::DirSelect::DSF_FILE_NAME_ONLY);
      files_in_dir.sort(FileEntryLess());
    }

    if (error_logger_->log_level() >= Logging::Logger::TRACE)
    {
      Stream::Error ostr;
      ostr << FUN << ": " << files_in_dir.size() << " files have been found.";
      error_logger_->log(
        ostr.str(),
        Logging::Logger::TRACE,
        Aspect::SYNC_LOGS);
    }
  }

  /* FeedRouteProcessor */
  FeedRouteProcessor::FeedRouteProcessor(
    Utils::ErrorPool* error_logger,
    const LocalInterfaceChecker& host_checker,
    const char* src_files_pattern,
    RouteBasicHelper* dest_router,
    const char* dst_dir,
    unsigned long tries_per_file,
    const char* local_copy_command_templ,
    CommandType local_copy_command_type,
    const char* remote_copy_command_templ,
    CommandType remote_copy_command_type,
    bool parse_source,
    bool unlink_source,
    bool interruptible,
    SchedType feed_type,
    const char* post_command,
    FetchType fetch_type)
    /*throw(BasicFeedRouteProcessor::Exception)*/
    : BasicFeedRouteProcessor(
        error_logger,
        host_checker,
        src_files_pattern,
        dest_router,
        dst_dir,
        tries_per_file,
        local_copy_command_templ,
        local_copy_command_type,
        remote_copy_command_templ,
        remote_copy_command_type,
        parse_source,
        unlink_source,
        interruptible,
        feed_type,
        post_command,
        fetch_type)
  {}

  void
  FeedRouteProcessor::process_files_(
    StringList& unlink_files,
    const FileEntries& process_files,
    const char* src_dir)
    noexcept
  {
    static const char* FUN = "FeedRouteProcessor::process_files_(): ";
    FileEntries::const_iterator file_it = process_files.begin();

    try
    {
      while (file_it != process_files.end() && active())
      {
        if(!mover_->move(src_dir, (*file_it)->file_name().c_str()))
        {
          break;
        }

        ++file_it;
      } // End of directory files listing
    }
    catch(const Utils::UnlinkException& ex)
    {
      unlink_files.push_back(ex.file_name);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": can't do processing: " << ex.what();
      error_logger_->log(ostr.str(),
        Logging::Logger::ERROR,
        Aspect::SYNC_LOGS,
        "ADS-IMPL-202");
    }

    while (file_it != process_files.end() && active())
    {
      (*file_it)->revert();
      ++file_it;
    }
  }

  /* ThreadPoolFeedRouteProcessor */
  ThreadPoolFeedRouteProcessor::ThreadPoolFeedRouteProcessor(
    Utils::ErrorPool* error_logger,
    const LocalInterfaceChecker& host_checker,
    const char* src_files_pattern,
    RouteBasicHelper* dest_router,
    const char* dst_dir,
    unsigned long tries_per_file,
    const char* local_copy_command_templ,
    CommandType local_copy_command_type,
    const char* remote_copy_command_templ,
    CommandType remote_copy_command_type,
    bool parse_source,
    bool unlink_source,
    bool interruptible,
    SchedType feed_type,
    Generics::TaskRunner* task_runner,
    const char* post_command,
    FetchType fetch_type)
    /*throw(BasicFeedRouteProcessor::Exception)*/
    : BasicFeedRouteProcessor(
        error_logger,
        host_checker,
        src_files_pattern,
        dest_router,
        dst_dir,
        tries_per_file,
        local_copy_command_templ,
        local_copy_command_type,
        remote_copy_command_templ,
        remote_copy_command_type,
        parse_source,
        unlink_source,
        interruptible,
        feed_type,
        post_command,
        fetch_type),
      task_runner_(ReferenceCounting::add_ref(task_runner)),
      move_state_(new MoveState())
  {}

  void
  ThreadPoolFeedRouteProcessor::deactivate_object() noexcept
  {
    Generics::CompositeActiveObject::deactivate_object();

    move_state_->tasks_count.release();
  }

  void
  ThreadPoolFeedRouteProcessor::process_files_(
    StringList& unlink_files,
    const FileEntries& process_files,
    const char* src_dir)
    noexcept
  {
    static const char* FUN = "ThreadPoolFeedRouteProcessor::process_files_(): ";

    try
    {
      unsigned long tasks_enqueued = 0;

      for (FileEntries::const_iterator file_it = process_files.begin();
        file_it != process_files.end() && active();
        ++file_it)
      {
        task_runner_->enqueue_task(Generics::Task_var(
          new MoveTask(
            mover_,
            move_state_,
            src_dir,
            *file_it)));
        ++tasks_enqueued;
      }

      for(unsigned long i = 0; i < tasks_enqueued && active(); ++i)
      {
        move_state_->tasks_count.acquire();
      }
    }
    catch(const Utils::UnlinkException& ex)
    {
      unlink_files.push_back(ex.file_name);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": can't do processing: " << ex.what();
      error_logger_->log(ostr.str(),
        Logging::Logger::ERROR,
        Aspect::SYNC_LOGS,
        "ADS-IMPL-202");
    }
  }
}
}
