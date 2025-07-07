#include <cstring>

#include <Generics/DirSelector.hpp>
#include <Commons/PathManip.hpp>

#include "LogCommons.hpp"
#include "FileReceiver.hpp"

namespace AdServer
{
  namespace LogProcessing
  {
    namespace
    {
      const char PATH_SEPARATOR = '/';
      const char REGULAR_FILE_IN_COMMIT_MODE_SUFFIX = 'C';
      const std::string UNDEFINED_STATE_MARK = "\x01";

      class DirSelectAdapter
      {
      public:
        typedef void (FileReceiver::*Handler)(const char*, const std::string&);

      public:
        DirSelectAdapter(
          FileReceiver* file_receiver,
          Handler handler, 
          Generics::ActiveObject* interrupter,
          const char* prefix = "")
          noexcept;

        void
        operator() (
          const char* file_name,
          const struct stat&)
          /*throw(eh::Exception, FileReceiver::Interrupted)*/;

      private:
        FileReceiver* file_receiver_;
        Handler handler_;
        Generics::ActiveObject* interrupter_;
        std::string prefix_;
      };

      DirSelectAdapter::DirSelectAdapter(
        FileReceiver* file_receiver,
        Handler handler, 
        Generics::ActiveObject* interrupter,
        const char* prefix)
        noexcept
        : file_receiver_(file_receiver),
          handler_(handler),
          interrupter_(interrupter),
          prefix_(prefix)
      {}

      void
      DirSelectAdapter::operator()(
        const char* file_name,
        const struct stat&)
        /*throw(eh::Exception, FileReceiver::Interrupted)*/
      {
        if (interrupter_ && !interrupter_->active())
        {
          throw FileReceiver::Interrupted("FileReceiver was interrupted");
        }

        (file_receiver_->*handler_)(file_name, prefix_);
      }

      struct FileNameInfo
      {
        enum Type
        {
          REGULAR_FILE,
          REGULAR_FILE_IN_COMMIT_MODE,
          COMMIT_FILE
        };

        Type type;
        std::string regular_file_name;

        FileNameInfo() noexcept : type(REGULAR_FILE)
        {}
      };

      void
      parse_log_file_name(
        const std::string& log_file_name,
        FileNameInfo& info)
        /*throw(eh::Exception)*/
      {
        if (log_file_name[log_file_name.size() - 1] ==
              REGULAR_FILE_IN_COMMIT_MODE_SUFFIX)
        {
          info.type = FileNameInfo::REGULAR_FILE_IN_COMMIT_MODE;
        }
        else if (log_file_name[0] == '~')
        {
            const std::size_t pos = log_file_name.find(".commit");

            if (pos == std::string::npos)
            {
              throw InvalidLogFileNameFormat("");
            }

            info.type = FileNameInfo::COMMIT_FILE;
            info.regular_file_name = log_file_name.substr(1, pos - 1);
        }
        else
        {
          info.type = FileNameInfo::REGULAR_FILE;
          info.regular_file_name = log_file_name;
        }
      }
    }

    //
    // FileReceiver::FileGuard
    //
    FileReceiver::FileGuard::FileGuard(
      const char* file_name)
      /*throw(eh::Exception)*/
      : file_name_(file_name),
        full_path_(file_name)
    {}

    FileReceiver::FileGuard::FileGuard(
      FileReceiver* file_receiver,
      const char* file_name)
      /*throw(eh::Exception)*/
      : file_receiver_(ReferenceCounting::add_ref(file_receiver)),
        file_name_(file_name),
        full_path_(file_receiver->intermediate_dir_ + PATH_SEPARATOR + file_name_)
    {}

    FileReceiver::FileGuard::~FileGuard() noexcept
    {
      if (file_receiver_)
      {
        file_receiver_->file_processed_(file_name_.c_str());
      }
    }

    void
    FileReceiver::FileGuard::revert() /*throw(eh::Exception)*/
    {
      if (file_receiver_)
      {
        file_receiver_->revert_(file_name_);
        file_receiver_.reset();
      }
    }

    //
    // FileReceiver
    //
    FileReceiver::FileReceiver(
      const char* intermediate_dir,
      size_t max_files_to_store,
      Generics::ActiveObject* interrupter,
      Logging::Logger* logger)
      /*throw(eh::Exception)*/
      : intermediate_dir_(intermediate_dir),
        max_files_to_store_(max_files_to_store),
        interrupter_(ReferenceCounting::add_ref(interrupter)),
        logger_(ReferenceCounting::add_ref(logger)),
        // set undefined state:
        // we don't know which files are locating in intermediate dir and must
        // force to refetch these.
        oldest_not_in_files_(UNDEFINED_STATE_MARK),
        fetch_internal_in_progress_(false)
    {}

    FileReceiver::FileGuard_var
    FileReceiver::get_eldest(std::string& new_top)
      /*throw(Interrupted, eh::Exception)*/
    {
      FileGuard_var file;
      const std::string res = get_eldest_(new_top);

      if (!res.empty())
      {
        file = new FileGuard(this, res.c_str());
      }

      return file;
    }

    std::string
    FileReceiver::get_eldest_(std::string& new_top)
      /*throw(Interrupted, eh::Exception)*/
    {
      {
        SyncPolicy::WriteGuard guard(files_lock_);

        if (!fetch_internal_in_progress_ &&
            (oldest_not_in_files_.empty() ||
              (!files_.empty() && oldest_not_in_files_ >= *files_.begin())))
        {
          return get_eldest_i_(new_top);
        }
      }

      fetch_intermediate_dir_();

      SyncPolicy::WriteGuard guard(files_lock_);
      return get_eldest_i_(new_top);
    }

    bool
    FileReceiver::fetch_files(
      const char* input_dir,
      const char* prefix)
      /*throw(Interrupted, eh::Exception)*/
    {
      dir_select_external_(input_dir, prefix);
      return !empty();
    }

    bool
    FileReceiver::empty() const noexcept
    {
      SyncPolicy::WriteGuard guard(files_lock_);
      return (oldest_not_in_files_.empty() && files_.empty());
    }

    FileReceiver::~FileReceiver() noexcept
    {}

    std::string
    FileReceiver::get_eldest_i_(std::string& new_top) /*throw(eh::Exception)*/
    {
      std::string res;

      if (!files_.empty())
      {
        res = *files_.begin();
        [[maybe_unused]] bool inserted = in_progress_files_.insert(res).second;
        assert(inserted);
        files_.erase(files_.begin());
      }

      new_top = (files_.empty() ||
        (!oldest_not_in_files_.empty() && oldest_not_in_files_ < *files_.begin()) ?
          oldest_not_in_files_ : *files_.begin());

      return res;
    }

    const std::string&
    FileReceiver::add_file_i_(
      const std::string& file_name)
      /*throw(eh::Exception)*/
    {
      files_.insert(file_name);

      if (files_.size() > max_files_to_store_)
      {
        if (oldest_not_in_files_.empty() || oldest_not_in_files_ > *files_.rbegin())
        {
          oldest_not_in_files_ = *files_.rbegin();
        }

        files_.erase(--(files_.end()));
      }

      return (oldest_not_in_files_.empty() || *files_.begin() < oldest_not_in_files_ ?
        *files_.begin() : oldest_not_in_files_);
    }

    void
    FileReceiver::fetch_intermediate_dir_() /*throw(Interrupted, eh::Exception)*/
    {
      std::string oldest_not_in_files_backup;
      SyncPolicy::WriteGuard guard(fetch_lock_);

      {
        SyncPolicy::WriteGuard files_guard(files_lock_);

        if (oldest_not_in_files_.empty() ||
            (!files_.empty() && oldest_not_in_files_ >= *files_.begin()))
        {
          return;
        }

        fetch_internal_in_progress_ = true;
        oldest_not_in_files_backup = oldest_not_in_files_;
        // set intermediate_dir_ content fully known state
        oldest_not_in_files_.clear();
      }

      try
      {
        dir_select_internal_(intermediate_dir_.c_str());
      }
      catch (const eh::Exception&)
      {
        // fetching isn't completed, revert to previous state
        SyncPolicy::WriteGuard files_guard(files_lock_);

        if (oldest_not_in_files_.empty() ||
            oldest_not_in_files_ > oldest_not_in_files_backup)
        {
          oldest_not_in_files_ = oldest_not_in_files_backup;
        }

        fetch_internal_in_progress_ = false;

        throw;
      }

      {
        SyncPolicy::WriteGuard files_guard(files_lock_);
        processed_files_.clear();
        fetch_internal_in_progress_ = false;
      }
    }

    void
    FileReceiver::file_processed_(const char* file_name) noexcept
    {
      SyncPolicy::WriteGuard guard(files_lock_);
      [[maybe_unused]] const bool erased = in_progress_files_.erase(file_name);
      assert(erased);

      if (fetch_internal_in_progress_)
      {
        processed_files_.insert(file_name);
      }
    }

    void
    FileReceiver::revert_(const std::string& name) /*throw(eh::Exception)*/
    {
      std::string top;

      {
        SyncPolicy::WriteGuard guard(files_lock_);
        const bool erased = in_progress_files_.erase(name);
        assert(erased);
        top = add_file_i_(name);
      }

      check_top_changed_(top, name);
    }

    void
    FileReceiver::fetch_files_handler_(
      const char* full_path,
      const std::string& prefix)
      /*throw(eh::Exception)*/
    {
      static const char* FUN = "FileReceiver::fetch_files_handler_";

      if (interrupter_ && !interrupter_->active())
      {
        throw Interrupted("FileReceiver::fetch_files was interrupted");
      }

      std::string name;
      std::string path;
      PathManip::split_path(full_path, &path, &name, true);

      FileNameInfo file_info;
      parse_log_file_name(name, file_info);

      if (file_info.type == FileNameInfo::REGULAR_FILE_IN_COMMIT_MODE ||
          file_info.regular_file_name.compare(0, prefix.length(), prefix))
      {
        return;
      }

      if (file_info.type == FileNameInfo::COMMIT_FILE)
      {
        const std::string regular_full_path =
          path + PATH_SEPARATOR + file_info.regular_file_name +
            '.' + REGULAR_FILE_IN_COMMIT_MODE_SUFFIX;

        if (access_(regular_full_path.c_str()))
        {
          fetch_file_(
            file_info.regular_file_name,
            regular_full_path);
        }
        else if (logger_)
        {
          logger_->sstream(Logging::Logger::NOTICE) << FUN <<
            ": commit file without regular '" << full_path << "'";
        }

        unlink_(full_path);
      }
      else
      {
        fetch_file_(name, full_path);
      }
    }

    void
    FileReceiver::fetch_file_(
      const std::string& name,
      const std::string& full_path)
      /*throw(eh::Exception)*/
    {
      static const char* FUN = "FileReceiver::fetch_file_()";

      const std::string new_name = intermediate_dir_ + PATH_SEPARATOR + name;
      std::string top;

      {
        SyncPolicy::WriteGuard guard(files_lock_);

        if (files_.find(name) != files_.end() ||
            in_progress_files_.find(name) != in_progress_files_.end())
        {
          Stream::Error ostr;
          ostr << FUN << ": duplicated file name: '" << name << "'";
          throw Exception(ostr.str());
        }

        rename_(full_path.c_str(), new_name.c_str());
        top = add_file_i_(name);
      }

      check_top_changed_(top, name);
    }

    void
    FileReceiver::fetch_intermediate_dir_handler_(
      const char* full_path,
      const std::string&)
      /*throw(eh::Exception)*/
    {
      const std::string name = Generics::DirSelect::file_name(full_path);

      SyncPolicy::WriteGuard guard(files_lock_);

      if (in_progress_files_.find(name) != in_progress_files_.end() ||
          processed_files_.find(name) != processed_files_.end())
      {
        return;
      }

      add_file_i_(name);
    }

    void
    FileReceiver::dir_select_external_(
      const char* input_dir,
      const char* prefix)
      /*throw(Interrupted, eh::Exception)*/
    {
      DirSelectAdapter dir_select_adapter(
        this,
        &FileReceiver::fetch_files_handler_,
        interrupter_,
        prefix);

      Generics::DirSelect::directory_selector(
        input_dir,
        dir_select_adapter,
        "[A-Z~]*",
        Generics::DirSelect::DSF_DONT_RESOLVE_LINKS |
        Generics::DirSelect::DSF_EXCEPTION_ON_OPEN);
    }

    void
    FileReceiver::dir_select_internal_(const char* dir)
      /*throw(Interrupted, eh::Exception)*/
    {
      DirSelectAdapter dir_select_adapter(
        this,
        &FileReceiver::fetch_intermediate_dir_handler_,
        interrupter_);

      try
      {
        Generics::DirSelect::directory_selector(
          dir,
          dir_select_adapter,
          "[A-Z]*",
          Generics::DirSelect::DSF_DONT_RESOLVE_LINKS |
          Generics::DirSelect::DSF_EXCEPTION_ON_OPEN);
      }
      catch (Generics::DirSelect::FailedToOpenDirectory&)
      {
        // skip it - corresponds to an empty Intermediate
      }
    }

    void
    FileReceiver::rename_(
      const char* old_name,
      const char* new_name)
      /*throw(eh::Exception)*/
    {
      if (::rename(old_name, new_name))
      {
        eh::throw_errno_exception<Exception>(FNE,
          "failed to rename file '", old_name, "' to '", new_name, "'");
      }
    }

    void
    FileReceiver::unlink_(const char* name)
      /*throw(eh::Exception)*/
    {
      if (::unlink(name))
      {
        eh::throw_errno_exception<Exception>(FNE,
          "failed to unlink file '", name, "'");
      }
    }

    bool
    FileReceiver::access_(const char* name) noexcept
    {
      return (::access(name, F_OK) != -1);
    }

    void
    FileReceiver::add_callback(Callback* callback) /*throw(eh::Exception)*/
    {
      if (callback)
      {
        callbacks_.insert(callback);
      }
    }

    void
    FileReceiver::move(const char* full_path) /*throw(eh::Exception)*/
    {
      std::string name;
      PathManip::split_path(full_path, 0, &name, true);
      fetch_file_(name, full_path);
    }

    void
    FileReceiver::check_top_changed_(
      const std::string& top,
      const std::string& file_name)
      noexcept
    {
      if (top == file_name)
      {
        for (Callbacks::iterator i = callbacks_.begin();
             i != callbacks_.end(); ++i)
        {
          (*i)->on_top_changed(file_name);
        }
      }
    }

    namespace FileReceiverConfig
    {
      std::string
      make_path(const char* log_root, const char* path) /*throw(eh::Exception)*/
      {
        std::string full_path(path);

        if (full_path[0] != '/')
        {
          full_path = std::string(log_root) + '/' + full_path;
        }

        return full_path;
      }
    }

    //
    // FileThreadProcessorPool
    //
    class FileThreadProcessorPool :
      public Generics::ActiveObjectCommonImpl
    {
    public:
      FileThreadProcessorPool(
        FileProcessor* file_processor,
        FileReceiver* file_receiver,
        Generics::ActiveObjectCallback* callback,
        unsigned threads_number)
        /*throw(eh::Exception)*/
          : Generics::ActiveObjectCommonImpl(
              SingleJob_var(new ProcessJob(file_processor, file_receiver, callback)),
              threads_number)
      {}

    private:
      class ProcessJob : public SingleJob, private FileReceiver::Callback
      {
      public:
        ProcessJob(
          FileProcessor* file_processor,
          FileReceiver* file_receiver,
          Generics::ActiveObjectCallback* callback)
          /*throw(eh::Exception)*/
          : SingleJob(callback),
            file_processor_(ReferenceCounting::add_ref(file_processor)),
            file_receiver_(ReferenceCounting::add_ref(file_receiver))
        {
          file_receiver->add_callback(this);
        }

        virtual void
        work() noexcept
        {
          while (!is_terminating())
          {
            FileReceiver::FileGuard_var file;

            {
              Sync::ConditionalGuard guard(cond_);
              std::string tmp;
              file = file_receiver_->get_eldest(tmp);

              if (!file && !is_terminating())
              {
                guard.wait();
              }
            }

            if (file)
            {
              file_processor_->process(file);
            }
          }
        }

        virtual void
        terminate() noexcept
        {
          Sync::ConditionalGuard guard(cond_);
          cond_.broadcast();
        }

        virtual void
        on_top_changed(const std::string&) noexcept
        {
          Sync::ConditionalGuard guard(cond_);
          cond_.broadcast();
        }

      private:
        FileProcessor_var file_processor_;
        FileReceiver_var file_receiver_;
        mutable Sync::Condition cond_;

      protected:
        virtual
        ~ProcessJob() noexcept
        {}
      };
    };

    //
    // FileThreadProcessor
    //
    FileThreadProcessor::FileThreadProcessor(
      FileProcessor* file_processor,
      Generics::ActiveObjectCallback* callback,
      unsigned threads_number,
      const char* intermediate_dir,
      std::size_t max_files_to_store,
      const char* input_dir,
      const char* prefix,
      const Generics::Time& fetch_period,
      Logging::Logger* logger)
      /*throw(eh::Exception)*/
      : scheduler_(new Generics::Planner(callback)),
        input_dir_(input_dir),
        prefix_(prefix),
        fetch_period_(fetch_period),
        logger_(ReferenceCounting::add_ref(logger))
    {
      add_child_object(scheduler_);

      Generics::ActiveObject_var interrupter = new FileReceiverInterrupter();
      add_child_object(interrupter);

      file_receiver_ = new FileReceiver(
        intermediate_dir,
        max_files_to_store,
        interrupter,
        logger);

      add_child_object(Generics::ActiveObject_var(
        new FileThreadProcessorPool(
          file_processor,
          file_receiver_,
          callback,
          threads_number)));

      scheduler_->schedule(
        Generics::Goal_var(new FetchGoal(*this)),
        Generics::Time::get_time_of_day() + fetch_period_);
    }

    FileThreadProcessor::FileThreadProcessor(
      FileProcessor* file_processor,
      Generics::ActiveObjectCallback* callback,
      unsigned threads_number,
      FileReceiver* file_receiver,
      const char* input_dir,
      const char* prefix,
      const Generics::Time& fetch_period,
      Logging::Logger* logger)
      /*throw(eh::Exception)*/
      : scheduler_(new Generics::Planner(callback)),
        input_dir_(input_dir),
        prefix_(prefix),
        fetch_period_(fetch_period),
        logger_(ReferenceCounting::add_ref(logger))
    {
      add_child_object(scheduler_);

      Generics::ActiveObject_var interrupter = new FileReceiverInterrupter();
      add_child_object(interrupter);

      file_receiver_ = ReferenceCounting::add_ref(file_receiver);

      add_child_object(Generics::ActiveObject_var(
        new FileThreadProcessorPool(
          file_processor,
          file_receiver_,
          callback,
          threads_number)));

      scheduler_->schedule(
        Generics::Goal_var(new FetchGoal(*this)),
        Generics::Time::get_time_of_day() + fetch_period_);
    }

    void
    FileThreadProcessor::fetch_files_() noexcept
    {
      static const char* FUN = "FileThreadProcessor::fetch_files_()";

      try
      {
        file_receiver_->fetch_files(input_dir_.c_str(), prefix_.c_str());
      }
      catch(const eh::Exception& ex)
      {
        if (logger_)
        {
          logger_->sstream(Logging::Logger::EMERGENCY) << FUN <<
            "Can't fetch_files. Caught eh::Exception: " << ex.what();
        }
      }

      try
      {
         scheduler_->schedule(
          Generics::Goal_var(new FetchGoal(*this)),
          Generics::Time::get_time_of_day() + fetch_period_);
      }
      catch(const eh::Exception& ex)
      {
        if (logger_)
        {
          logger_->sstream(Logging::Logger::EMERGENCY) << FUN <<
            "Can't schedule goal. Caught eh::Exception: " << ex.what();
        }
      }
    }
  }
}
