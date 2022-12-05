#ifndef _RSYNCFILEROUTER_HPP_
#define _RSYNCFILEROUTER_HPP_

#include <eh/Exception.hpp>

#include "Utils.hpp"
#include "FileRouter.hpp"

namespace AdServer
{
  namespace LogProcessing
  {
    /**
     * wrapper for execute rsync command &
     * control its result
     */
    class RSyncFileRouter: public AppFileRouter
    {
    public:
      DECLARE_EXCEPTION(AlreadyExists, Exception);

    public:
      RSyncFileRouter(
        const char* command_template,
        const char* post_command_template = "")
        /*throw(Exception)*/;

      virtual ~RSyncFileRouter() noexcept {}

      virtual void
      move(
        const FileRouteParams& file_route_params,
        bool sync_mode = false,
        InterruptCallback* interrupter = 0)
        /*throw(Exception)*/;

    protected:
      /** check rsync output after execution */
      static void check_rsync_output_(
        const FileRouteParams& file_route_params,
        const char* output_str)
        /*throw(AlreadyExists, Exception)*/;

      /** Returns error message corresponding rsync exit code */
      static const char* rsync_err_msg_(int exit_code) noexcept;
    };
  }
}


#endif
