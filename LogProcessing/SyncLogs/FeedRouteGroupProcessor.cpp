#include <fstream>
#include <sstream>

#include <Generics/DirSelector.hpp>
#include <Commons/PathManip.hpp>
#include <Commons/DelegateTaskGoal.hpp>

#include "FileRouter.hpp"
#include "RSyncFileRouter.hpp"

#include "FeedRouteGroupProcessor.hpp"

namespace AdServer
{
namespace LogProcessing
{
  namespace Aspect
  {
    const char SYNC_LOGS[] = "SyncLogs";
  }

  namespace
  {
    const char COMMIT_FILE_PREFIX = '~';
    const char COMMIT_FILE_SUFFIX[] = ".commit";
    const char COMMIT_FILE_FIND_PATTERN[] = "\\.commit\\.(.*)$";
  }

  /*
   * Route
   */
  Route::Route(
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
    bool commit_mode_flag,
    Generics::ActiveObject* interrupter)
    /*throw(Exception)*/
    : commit_mode(commit_mode_flag)
  {
    static const char* FUN = "FeedRouteGroupProcessor::Route::Route";

    try
    {
      if(!parse_source && feed_type != ST_ROUND_ROBIN)
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
          new RSyncFileRouter(local_copy_command_templ);
      }
      else if(local_copy_command_type == CT_GENERIC)
      {
        local_file_router =
          new AppFileRouter(local_copy_command_templ);
      }

      FileRouter_var remote_file_router;

      if(remote_copy_command_type == CT_RSYNC)
      {
        remote_file_router =
          new RSyncFileRouter(remote_copy_command_templ);
      }
      else if(remote_copy_command_type == CT_GENERIC)
      {
        remote_file_router =
          new AppFileRouter(remote_copy_command_templ);
      }

      PathManip::split_path(
        src_files_pattern,
        &src_dir,
        &file_mask,
        !parse_source);

      if (parse_source)
      {
        if(file_mask.empty())
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
            file_mask,
            TemplateParams::MARKER,
            TemplateParams::MARKER);
          file_mask = templ.instantiate(templ_args);
        }
        catch(const eh::Exception& e)
        {
          Stream::Error ostr;
          ostr << FUN << ": eh::Exception on instancing HASH template. "
            "Pattern: " << file_mask  << " : " << e.what();
          throw Exception(ostr);
        }
      }

      if (commit_mode)
      {
        commit_src_dir = src_dir.substr(0, src_dir.size() - 1) + COMMIT_FILE_SUFFIX + '/';
        commit_file_mask = COMMIT_FILE_PREFIX + file_mask + '*';
      }

      intermediate_dir = src_dir + "Intermediate/";
      const std::size_t MAX_FILES_TO_STORE = 50000;

      file_receiver = new FileReceiver(
        intermediate_dir.c_str(),
        MAX_FILES_TO_STORE,
        interrupter,
        0); // logger

      mover = new FeedRouteMover(
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
        commit_mode);

      if (commit_mode)
      {
        FixedRouteBasicHelper_var host_from_name_route_helper =
          new RouteHostFromFileNameHelper(ST_FROM_FILE_NAME, COMMIT_FILE_FIND_PATTERN);
        commit_mover = new FeedRouteMover(
          error_logger,
          host_checker,
          dst_dir,
          tries_per_file,
          host_from_name_route_helper,
          local_file_router,
          remote_file_router,
          parse_source,
          unlink_source,
          interruptible,
          false /* do not add '.C' to commit files */);
      }
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

  /*
   * FeedRouteGroupProcessor
   */
  FeedRouteGroupProcessor::FeedRouteGroupProcessor(
    const std::vector<Route>& routes,
    SyncLog::FileReceiverFacade* file_receiver_facade,
    Utils::ErrorPool* error_logger,
    Generics::TaskRunner* task_runner,
    Generics::Planner* scheduler,
    Generics::ActiveObjectCallback* callback,
    unsigned threads_number,
    unsigned long timeout)
    /*throw(eh::Exception)*/
    : Commons::DelegateActiveObject(callback, threads_number),
      routes_(routes),
      file_receiver_facade_(ReferenceCounting::add_ref(file_receiver_facade)),
      error_logger_(error_logger),
      task_runner_(ReferenceCounting::add_ref(task_runner)),
      scheduler_(ReferenceCounting::add_ref(scheduler)),
      timeout_(timeout)
  {
    for (SyncLog::LogType log_type = 0; log_type < routes_.size(); ++ log_type)
    {
      if (routes_[log_type].commit_mode)
      {
        task_runner_->enqueue_task(
          AdServer::Commons::make_delegate_task(
            std::bind(&FeedRouteGroupProcessor::fetch_commit_files_, this, log_type)));
      }

      task_runner_->enqueue_task(
        AdServer::Commons::make_delegate_task(
          std::bind(&FeedRouteGroupProcessor::fetch_files_, this, log_type)));
    }

    task_runner_->enqueue_task(
      AdServer::Commons::make_delegate_task(
        std::bind(&FeedRouteGroupProcessor::try_unlink_files_, this)));
  }

  void
  FeedRouteGroupProcessor::work_() noexcept
  {
    static const char* FUN = "FeedRouteGroupProcessor::work_";

    while (active())
    {
      try
      {
        FileEntity file;

        {
          SyncPolicy::WriteGuard lock(pending_tasks_lock_);

          if (!pending_tasks_.empty())
          {
            const PendingTask& top_task = pending_tasks_.top();

            if (top_task.time < Generics::Time::get_time_of_day())
            {
              file = top_task.file;
              pending_tasks_.pop();
            }
          }
        }

        if (!file.file_guard)
        {
          file = file_receiver_facade_->get_eldest(timeout_);
        }

        if (file.file_guard)
        {
          process_file_(file);
        }
      }
      catch (eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << " : got exception: " << ex.what();
        error_logger_->log(ostr.str(),
          Logging::Logger::ERROR,
          Aspect::SYNC_LOGS,
          "ADS-IMPL-202");
      }
    }
  }

  void
  FeedRouteGroupProcessor::process_file_(const FileEntity& file) noexcept
  {
    static const char* FUN = "FeedRouteGroupProcessor::process_file_";

    try
    {
      Route& route = routes_[file.type];
      std::string file_path;
      std::string file_name;

      PathManip::split_path(
        file.file_guard->full_path().c_str(),
        &file_path,
        &file_name,
        false);
      const bool is_commit_file = (file_name[0] == COMMIT_FILE_PREFIX);

      FeedRouteMover_var mover = is_commit_file ? route.commit_mover : route.mover;

      std::string dst_host;

      if (!mover->move(
            file_path.c_str(),
            file_name.c_str(),
            &dst_host))
      {
        add_pending_task_(file, timeout_);
      }

      if (route.commit_mode && !is_commit_file)
      {
        std::ostringstream oss;
        oss << route.commit_src_dir << COMMIT_FILE_PREFIX << file.file_guard->file_name() <<
          COMMIT_FILE_SUFFIX << '.' << dst_host;
        const std::string commit_file_name = oss.str();

        std::ofstream commit_file(commit_file_name);

        if (!commit_file)
        {
          eh::throw_errno_exception<Exception>(
            "failed to create commit file :'", commit_file_name, "'");
        }

        FileEntity commit_file_entity;
        commit_file_entity.type = file.type;
        commit_file_entity.file_guard =
          new FileReceiver::FileGuard(commit_file_name.c_str());
        add_pending_task_(commit_file_entity, 0);
      }
    }
    catch(const Utils::UnlinkException& ex)
    {
      SyncPolicy::WriteGuard lock(unlink_files_lock_);
      unlink_files_.push_back(file.file_guard);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << " : can't do processing: " << ex.what();
      error_logger_->log(ostr.str(),
        Logging::Logger::ERROR,
        Aspect::SYNC_LOGS,
        "ADS-IMPL-202");
    }
  }

  void
  FeedRouteGroupProcessor::add_pending_task_(
    const FileEntity& file,
    unsigned long timeout)
    /*throw(eh::Exception)*/
  {
    SyncPolicy::WriteGuard lock(pending_tasks_lock_);
    pending_tasks_.emplace(file, Generics::Time::get_time_of_day() + timeout);
    // no need condition signal, because this function can be called
    // from worker only and this worker will continue processing
  }

  void
  FeedRouteGroupProcessor::try_unlink_files_() noexcept
  {
    static const char* FUN = "FeedRouteGroupProcessor::try_unlink_files_()";

    UnlinkedFiles unlink_files;

    {
      SyncPolicy::WriteGuard lock(unlink_files_lock_);
      unlink_files_.swap(unlink_files);
    }

    for(auto it = unlink_files.begin(); it != unlink_files.end(); ++it)
    {
      try
      {
        Utils::unlink_file((*it)->full_path().c_str());
      }
      catch(const Utils::NotFound&)
      {
        // nothing to do
      }
      catch(const eh::Exception& ex)
      {
        {
          SyncPolicy::WriteGuard lock(unlink_files_lock_);
          unlink_files_.push_back(*it);
        }

        Stream::Error ostr;
        ostr << FUN << " : can't unlink source file: " << ex.what();
        error_logger_->log(
          ostr.str(),
          Logging::Logger::ERROR,
          Aspect::SYNC_LOGS,
          "ADS-IMPL-201");
      }
    }

    scheduler_->schedule(
      AdServer::Commons::make_delegate_goal_task(
        std::bind(&FeedRouteGroupProcessor::try_unlink_files_, this),
        task_runner_),
      Generics::Time::get_time_of_day() + timeout_);
  }

  void
  FeedRouteGroupProcessor::fetch_files_(SyncLog::LogType log_type) noexcept
  {
    static const char* FUN = "FeedRouteGroupProcessor::fetch_files_";

    try
    {
      const Route& route = routes_[log_type];
      route.file_receiver->fetch_files(route.src_dir.c_str(), route.file_mask.c_str());
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": fetch files eh::Exception caught: " << ex.what();
      error_logger_->log(ostr.str(), Logging::Logger::CRITICAL);
    }

    try
    {
      scheduler_->schedule(
      AdServer::Commons::make_delegate_goal_task(
        std::bind(&FeedRouteGroupProcessor::fetch_files_, this, log_type),
        task_runner_),
      Generics::Time::get_time_of_day() + timeout_);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": reschedule task eh::Exception caught: " << ex.what();
      error_logger_->log(ostr.str(), Logging::Logger::CRITICAL);
    }
  }

  void
  FeedRouteGroupProcessor::fetch_commit_files_(SyncLog::LogType log_type)
    noexcept
  {
    static const char* FUN = "FeedRouteGroupProcessor::fetch_commit_files_";

    try
    {
      typedef std::list<std::string> StringList;
      const Route& route = routes_[log_type];
      StringList files;

      Generics::DirSelect::directory_selector(
        route.commit_src_dir.c_str(),
        Generics::DirSelect::list_creator(std::back_inserter(files)),
        route.commit_file_mask.c_str(),
        Generics::DirSelect::DSF_FULL_PATH);

      for (auto it = files.begin(); it != files.end(); ++it)
      {
        FileEntity file;
        file.type = log_type;
        file.file_guard = new FileReceiver::FileGuard(it->c_str());
        add_pending_task_(file, 0);
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": eh::Exception caught: " << ex.what();
      error_logger_->log(ostr.str(), Logging::Logger::CRITICAL);
    }
  }
}
}
