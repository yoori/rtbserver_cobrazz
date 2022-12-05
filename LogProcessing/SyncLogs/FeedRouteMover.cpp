#include "FeedRouteMover.hpp"

#include "RSyncFileRouter.hpp"

namespace Aspect
{
  char SYNC_LOGS[] = "SyncLogs";
}

namespace AdServer
{
namespace LogProcessing
{
  namespace Aspect
  {
    const char SYNC_LOGS[] = "SyncLogs";
  }

  FeedRouteMover::FeedRouteMover(
    Utils::ErrorPool* error_logger,
    const LocalInterfaceChecker& host_checker,
    const char* dst_dir,
    unsigned long tries_per_file,
    RouteBasicHelper* route_helper,
    FileRouter* local_file_router,
    FileRouter* remote_file_router,
    bool parse_source,
    bool unlink_source,
    bool interruptible,
    bool commit_mode_flag)
    /*throw(Exception)*/
    : error_logger_(error_logger),
      host_checker_(host_checker),
      dst_dir_(dst_dir),
      parse_source_(parse_source),
      unlink_source_(unlink_source),
      interruptible_(interruptible),
      tries_per_file_(tries_per_file),
      route_helper_(ReferenceCounting::add_ref(route_helper)),
      local_file_router_(ReferenceCounting::add_ref(local_file_router)),
      remote_file_router_(ReferenceCounting::add_ref(remote_file_router)),
      commit_mode_flag_(commit_mode_flag)
  {
    if(dst_dir_.empty() || *dst_dir_.rbegin() != '/')
    {
      dst_dir_ += '/';
    }
  }

  bool
  FeedRouteMover::interrupt() /*throw(eh::Exception)*/
  {
    return !active();
  }

  bool
  FeedRouteMover::move(
    const char* src_path,
    const char* src_file,
    std::string* dst_host)
    /*throw(Utils::UnlinkException)*/
  {
    static const char* FUN = "FeedRouteMover::move()";

    FileRouteParams file_route_params;
    file_route_params.src_file_name = src_file;
    file_route_params.src_file_path = src_path;
    file_route_params.src_file_path += src_file;
    file_route_params.src_dir = src_path;
    file_route_params.dst_file_name =
      dst_dir_ + file_route_params.src_file_name;

    if (commit_mode_flag_)
    {
      file_route_params.dst_file_name += ".C";
    }

    bool moved = false;

    for(unsigned long try_number = 0;
        try_number < tries_per_file_ && !moved && active();
        ++try_number)
    {
      try
      {
        file_route_params.dst_host =
          route_helper_->get_dest_host(src_file);

        if (dst_host)
        {
          *dst_host = file_route_params.dst_host;
        }

        if(file_route_params.dst_host.empty())
        {
          // skip file
          return true;
        }
      }
      catch(const RouteBasicHelper::NotReady&)
      {
        return false;
      }
      catch(const RouteBasicHelper::NotAvailable& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": all hosts NotAvailable: " << ex.what();
        error_logger_->log(
          ostr.str(),
          Logging::Logger::ERROR,
          Aspect::SYNC_LOGS,
          "ADS-IMPL-201");

        return false;
      }

      try
      {
        if (error_logger_->log_level() >= TraceLevel::HIGH)
        {
          Stream::Dynamic ostr(4096);
          ostr << FUN << ": preparing to send the '" <<
            file_route_params.src_file_path <<
            "' to '" <<
            file_route_params.dst_file_name << "':";
          error_logger_->log(ostr.str(),
            TraceLevel::HIGH, Aspect::SYNC_LOGS);
        }

        bool local_moving =
          host_checker_.check_host_name(file_route_params.dst_host);

        if (local_moving)
        {
          if(file_route_params.src_file_path != file_route_params.dst_file_name)
          {
            local_file_router_->move(
              file_route_params,
              !parse_source_, // check log file name
              interruptible_ ? this : 0); // using sync mode
          }
        }
        else
        {
          remote_file_router_->move(
            file_route_params,
            !parse_source_, // check log file name
            interruptible_ ? this : 0); // using sync mode
        }

        moved = true;
      }
      catch (const eh::Exception& ex)
      {
        route_helper_->bad_host(file_route_params.dst_host);

        Stream::Error ostr;
        ostr << "Can't sync '" << file_route_params.src_file_path <<
          " - host '" << file_route_params.dst_host << "' or path"
          " '" << file_route_params.dst_file_name << "' unavailable: " <<
          ex.what();
        error_logger_->add_error(
          file_route_params.src_host,
          file_route_params.dst_host,
          String::SubString(src_path),
          String::SubString("ADS-IMPL-201"),
          String::SubString("FeedRouteMover::move(): "),
          ostr.str());
      }
    }

    if (moved && unlink_source_)
    {
      try
      {
        Utils::unlink_file(file_route_params.src_file_path.c_str());
      }
      catch(const Utils::NotFound& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": file not found: " << e.what();
        error_logger_->log(
          ostr.str(),
          Logging::Logger::ERROR,
          Aspect::SYNC_LOGS,
          "ADS-IMPL-201");
      }
      catch(const Utils::UnlinkException& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": can't remove file '" <<
          file_route_params.src_file_path << "': " << e.what();
        error_logger_->log(
          ostr.str(),
          Logging::Logger::ERROR,
          Aspect::SYNC_LOGS,
          "ADS-IMPL-201");
        throw e;
      }
    }

    return moved;
  }
}
}
