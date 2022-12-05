#include "RSyncFileRouter.hpp"

namespace
{
  int MAX_RENAME_TRY_COUNT = 3;

  /* Error messages corresponding rsync exit codes. */
  struct RSyncErr
  {
    int code;
    const char* msg_str;
  };

  const RSyncErr RSYNC_ERRORS[] =
  {
    { -1, "unknown exit code" },
    { 1, "syntax or usage error" },
    { 2, "protocol incompatibility" },
    { 3, "errors selecting input/output files, dirs" },
    { 4, "requested action not supported" },
    { 5, "error starting client-server protocol" },
    { 10, "error in socket IO" },
    { 11, "error in file IO" },
    { 12, "error in rsync protocol data stream" },
    { 13, "errors with program diagnostics" },
    { 14, "error in IPC code" },
    { 15, "sibling process crashed" },
    { 16, "sibling process terminated abnormally" },
    { 19, "received SIGUSR1" },
    { 20, "received SIGINT, SIGTERM, or SIGHUP" },
    { 21, "waitpid() failed" },
    { 22, "error allocating core memory buffers" },
    { 23, "some files could not be transferred" },
    { 24, "some files vanished before they could be transferred" },
    { 25, "the --max-delete limit stopped deletions" },
    { 30, "timeout in data send/receive" },
    { 124, "remote shell failed" },
    { 125, "remote shell killed" },
    { 126, "remote command could not be run" },
    { 127, "remote command not found" },
    { 0, 0 }
  };
}

namespace AdServer
{
  namespace LogProcessing
  {
    RSyncFileRouter::RSyncFileRouter(
      const char* command_template,
      const char* post_command_template)
      /*throw(Exception)*/
      : AppFileRouter(command_template, post_command_template)
    {}

    void
    RSyncFileRouter::move(
      const FileRouteParams& file_route_params,
      bool sync_mode,
      InterruptCallback* interrupter)
      /*throw(Exception)*/
    {
      static const char* FUN = "RSyncFileRouter::move()";

      int rename_try_index = MAX_RENAME_TRY_COUNT - 1;

      while(rename_try_index > 0)
      {
        std::string command;
        std::string output;

        const int exit_code = run_command_(
          command_template_,
          file_route_params,
          sync_mode,
          command,
          output,
          interrupter);

        if(exit_code != 0)
        {
          Stream::Error ostr;
          ostr << FUN << ": '" <<
            command << "' return error (" << exit_code <<
            ", '" << rsync_err_msg_(exit_code) << "').";
          throw Exception(ostr);
        }

        bool already_exists = false;

        try
        {
          check_rsync_output_(file_route_params, output.c_str());
        }
        catch(const AlreadyExists& )
        {
          already_exists = true;
        }

        std::string post_command;
        std::string post_output;

        const int post_exit_code = run_command_(
          post_command_template_,
          file_route_params,
          sync_mode,
          post_command,
          post_output,
          0);

        if(post_exit_code != 0)
        {
          Stream::Error ostr;
          ostr << FUN << ": '" <<
            post_command << "' return error (" << post_exit_code << ").";
          throw Exception(ostr);
        }

        if(!already_exists)
        {
          break;
        }

        /* TODO: rename file */
        --rename_try_index;
      }
    }

    void
    RSyncFileRouter::check_rsync_output_(
      const FileRouteParams& file_route_params,
      const char* output_str)
      /*throw(AlreadyExists, Exception)*/
    {
      static const char* FUN = "RSyncFileRouter::check_rsync_output()";

      std::string output_str_s(output_str);

      if(output_str_s.empty())
      {
        throw AlreadyExists("");
      }

      if(output_str_s.find(file_route_params.src_file_name) == std::string::npos)
      {
        Stream::Error ostr;
        ostr << FUN << ": "
          "rsync has printed: '" << output_str <<
          "', and it does not contain the '" <<
          file_route_params.src_file_name <<
          "' file name.";

        throw Exception(ostr);
      }
    }

    /** Returns error message corresponding rsync exit code */
    const char*
    RSyncFileRouter::rsync_err_msg_(int exit_code)
      noexcept
    {
      for(int i = 1; RSYNC_ERRORS[i].msg_str; ++i)
      {
        if (RSYNC_ERRORS[i].code == exit_code)
        {
          return RSYNC_ERRORS[i].msg_str;
        }
      }

      return RSYNC_ERRORS[0].msg_str;
    }
  }
}
