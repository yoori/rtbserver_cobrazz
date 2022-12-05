#ifndef LOGPROCESSING_SYNCLOGS_FEEDROUTEMOVER_HPP_
#define LOGPROCESSING_SYNCLOGS_FEEDROUTEMOVER_HPP_

#include <ReferenceCounting/AtomicImpl.hpp>
#include <Generics/ActiveObject.hpp>

#include "Utils.hpp"
#include "RouteProcessor.hpp"
#include "FileRouter.hpp"
#include "RouteHelpers.hpp"

namespace AdServer
{
namespace LogProcessing
{
  class FeedRouteMover:
    public ReferenceCounting::AtomicImpl,
    public Generics::SimpleActiveObject,
    protected InterruptCallback
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    FeedRouteMover(
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
      /*throw(Exception)*/;

    bool
    move(
      const char* src_path,
      const char* src_file,
      std::string* dst_host = nullptr)
      /*throw(Utils::UnlinkException)*/;

    bool
    interrupt() /*throw(eh::Exception)*/;

  protected:
    virtual
    ~FeedRouteMover() noexcept
    {}

  private:
    struct TraceLevel
    {
      enum
      {
        LOW = Logging::Logger::TRACE,
        MIDDLE,
        HIGH
      };
    };

  private:
    // const configuration
    Utils::ErrorPool* const error_logger_;

    const LocalInterfaceChecker& host_checker_;
    const std::string src_files_pattern_;
    std::string dst_dir_;

    const bool parse_source_;
    const bool unlink_source_;
    const bool interruptible_;

    const unsigned long tries_per_file_;

    FixedRouteBasicHelper_var route_helper_;
    FixedFileRouter_var local_file_router_;
    FixedFileRouter_var remote_file_router_;
    bool commit_mode_flag_;
  };

  typedef ReferenceCounting::SmartPtr<FeedRouteMover>
    FeedRouteMover_var;
  typedef ReferenceCounting::FixedPtr<FeedRouteMover>
    FixedFeedRouteMover_var;
}
}

#endif /* LOGPROCESSING_SYNCLOGS_FEEDROUTEMOVER_HPP_ */
