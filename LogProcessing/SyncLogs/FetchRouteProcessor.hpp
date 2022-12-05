// @file SyncLogs/FetchRouteProcessor.hpp

#ifndef _FETCHROUTEPROCESSOR_HPP_
#define _FETCHROUTEPROCESSOR_HPP_

#include <String/TextTemplate.hpp>
#include <Logger/Logger.hpp>
#include <Generics/Network.hpp>
#include <Generics/ActiveObject.hpp>

#include "Utils.hpp"
#include "FileRouter.hpp"
#include "RouteProcessor.hpp"

namespace AdServer
{
  namespace LogProcessing
  {
    /** */
    class FetchRouteProcessor:
      public RouteProcessor,
      public Generics::SimpleActiveObject,
      public ReferenceCounting::AtomicImpl,
      protected InterruptCallback
    {
    public:
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

      enum CommandType
      {
        CT_GENERIC,
        CT_RSYNC
      };

      FetchRouteProcessor(
        Utils::ErrorPool* error_logger,
        const LocalInterfaceChecker& host_checker,
        const char* host_name,
        const char* src_files_pattern,
        const StringList& src_hosts,
        const char* dst_dir,
        const char* local_copy_command_templ,
        CommandType local_copy_command_type,
        const char* remote_copy_command_templ,
        CommandType remote_copy_command_type,
        const char* ssh_command_templ,
        const char* remote_ls_command_templ,
        const char* remote_rm_command_templ,
        unsigned long check_period)
        /*throw(Exception)*/;

      virtual ~FetchRouteProcessor() noexcept {}

      void process() noexcept;

      bool interrupt() /*throw(eh::Exception)*/;

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
      bool
      make_file_list_for_fetch_(
        const char* src_files,
        StringList &files_in_dir,
        const String::SubString& src_host,
        const String::SubString& dst_host,
        const String::SubString& src_dir)
        /*throw(Exception)*/;

      void
      split_files_(
        std::istream &is,
        std::list<std::string> &files_in_dir)
        /*throw(Exception)*/;

      void
      unlink_remote_file_(
        const char* file,
        const char* host) /*throw(Exception)*/;

    private:
      InterruptCallback* interrupter_;
      Utils::ErrorPool* error_logger_;

      const LocalInterfaceChecker& host_checker_;
      std::string host_name_;
      std::string src_files_pattern_;
      StringList src_hosts_;
      std::string dst_dir_;

      FileRouter_var local_file_router_;
      FileRouter_var remote_file_router_;

      String::TextTemplate::IStream ssh_templ_;
      String::TextTemplate::IStream remote_ls_templ_;
      String::TextTemplate::IStream remote_rm_templ_;
      String::SequenceAnalyzer::AnalyzerParams split_files_params_;
      Generics::ActiveObjectCallback_var split_files_null_callback_;
    };

    typedef ReferenceCounting::QualPtr<FetchRouteProcessor>
      FetchRouteProcessor_var;
  }
}


#endif /*_FETCHROUTEPROCESSOR_HPP_*/
