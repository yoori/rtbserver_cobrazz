// @file SyncLogs/FetchRouteProcessor.cpp

#include <sstream>

#include <String/BasicAnalyzer.hpp>
#include <String/AnalyzerParams.hpp>
#include <String/Analyzer.hpp>
#include <Logger/ActiveObjectCallback.hpp>

#include <Commons/PathManip.hpp>
#include "RSyncFileRouter.hpp"

#include "FetchRouteProcessor.hpp"

namespace Aspect
{
  const char SYNC_LOGS[] = "SyncLogs";
}

namespace AdServer
{
namespace LogProcessing
{
  FetchRouteProcessor::FetchRouteProcessor(
    Utils::ErrorPool* error_logger,
    const LocalInterfaceChecker& host_checker,
    const char* host_name,
    const char* src_files_pattern,
    const StringList& src_hosts,
    const char* dst_dir,
    const char* local_copy_command_templ,
    CommandType local_copy_type,
    const char* remote_copy_command_templ,
    CommandType remote_copy_type,
    const char* ssh_command_templ,
    const char* remote_ls_command_templ,
    const char* remote_rm_command_templ,
    unsigned long /*check_period*/)
    /*throw(Exception)*/
    : interrupter_(0),
      error_logger_(error_logger),
      host_checker_(host_checker),
      host_name_(host_name),
      src_files_pattern_(src_files_pattern),
      src_hosts_(src_hosts),
      dst_dir_(dst_dir)
  {
    const char FUN[] = "FetchRouteProcessor::FetchRouteProcessor()";

    /* test ssh command template */
    try
    {
      Stream::Parser istr(ssh_command_templ);
      ssh_templ_.init(istr, TemplateParams::MARKER, TemplateParams::MARKER);

      String::TextTemplate::Args templ_args;
      templ_args[TemplateParams::HOST] = "";
      ssh_templ_.instantiate(templ_args);
      if(dst_dir_.empty() || *dst_dir_.rbegin() != '/')
      {
        dst_dir_ += '/';
      }
    }
    catch (const String::TextTemplate::UnknownName &ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": "
          << "String::TextTemplate::UnknownName caught while "
          "instantiating ssh_params. : " << ex.what();
      throw Exception(ostr);
    }
    catch (const String::TextTemplate::TextTemplException &ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": "
        "String::TextTemplate::Exception caught while instantiating "
        "ssh_params. : " << ex.what();
      throw Exception(ostr);
    }
    catch (const eh::Exception &ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": "
        "eh::Exception caught while instantiating "
        "ssh_params. : " << ex.what();
      throw Exception(ostr);
    }

    try
    {
      Stream::Parser istr(remote_ls_command_templ);
      remote_ls_templ_.init(istr, TemplateParams::MARKER, TemplateParams::MARKER);

      String::TextTemplate::Args templ_args;
      templ_args[TemplateParams::PATH] = "";
      templ_args[TemplateParams::PATTERN] = "";
      remote_ls_templ_.instantiate(templ_args);
    }
    catch (const String::TextTemplate::UnknownName &ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": "
        "String::TextTemplate::UnknownName caught while "
        "instantiating remote_ls_params. : " << ex.what();
      throw Exception(ostr);
    }
    catch (const String::TextTemplate::TextTemplException &ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": "
        "String::TextTemplate::Exception caught while instantiating "
        "remote_ls_params. : " << ex.what();
      throw Exception(ostr);
    }
    catch (const eh::Exception &ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": "
        "eh::Exception caught while instantiating "
        "remote_ls_params. : " << ex.what();
      throw Exception(ostr);
    }

    try
    {
      Stream::Parser istr(remote_rm_command_templ);
      remote_rm_templ_.init(
        istr,
        TemplateParams::MARKER,
        TemplateParams::MARKER);

      String::TextTemplate::Args templ_args;
      templ_args[TemplateParams::PATH] = "";
      templ_args[TemplateParams::FILE_NAME] = "";
      remote_rm_templ_.instantiate(templ_args);
    }
    catch (const String::TextTemplate::UnknownName &ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": "
        "String::TextTemplate::UnknownName caught while "
        "instantiating remote_rm_params. : " << ex.what();
      throw Exception(ostr);
    }
    catch (const String::TextTemplate::TextTemplException &ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": "
        "String::TextTemplate::Exception caught while instantiating "
        "remote_rm_params. : " << ex.what();
      throw Exception(ostr);
    }
    catch (const eh::Exception &ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": "
        "eh::Exception caught while instantiating "
        "remote_rm_params. : " << ex.what();
      throw Exception(ostr);
    }

    try
    {
      if(local_copy_type == CT_RSYNC)
      {
        local_file_router_ = new RSyncFileRouter(local_copy_command_templ);
      }
      else if(local_copy_type == CT_GENERIC)
      {
        local_file_router_ = new AppFileRouter(local_copy_command_templ);
      }

      if(remote_copy_type == CT_RSYNC)
      {
        remote_file_router_ = new RSyncFileRouter(remote_copy_command_templ);
      }
      else if(remote_copy_type == CT_GENERIC)
      {
        remote_file_router_ = new AppFileRouter(remote_copy_command_templ);
      }
    }
    catch(const FileRouter::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught FileRouter::Exception. : " << e.what();
      throw Exception(ostr);
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception. : " << e.what();
      throw Exception(ostr);
    }

    // init split_files_params_
    split_files_params_.shield_symbol = '\\';
    split_files_params_.main_separators = String::SequenceAnalyzer::CharSet("\n");
    split_files_params_.ignore_successive_separators = true;

    split_files_params_.regular_symbs =
      String::SequenceAnalyzer::CharSet("a-zA-Z0-9_.:-/");
    split_files_params_.regular_range_symbs =
      String::SequenceAnalyzer::CharSet("a-zA-Z0-9_.:");

    split_files_params_.allow_ignored_symbs = true;
    split_files_params_.ignored_symbs =
      String::SequenceAnalyzer::CharSet("\n");

    split_files_params_.allow_recursion = true;
    split_files_params_.recursion_max_depth = 10000;
    split_files_params_.allow_repeat = true;
    split_files_params_.num_retries_symb =
      String::SequenceAnalyzer::CharPair('{', '}');
    split_files_params_.retry_part_symb =
      String::SequenceAnalyzer::CharPair('(', ')');
    split_files_params_.allow_range = true;
    split_files_params_.immediate_range_mode = false;
    split_files_params_.range_part_symb =
      String::SequenceAnalyzer::CharPair('[', ']');
    split_files_params_.range_separators =
      String::SequenceAnalyzer::CharSet(", ");
    split_files_params_.range_symbol = '-';
    split_files_params_.allow_padding = true;
    split_files_params_.padding_symb = '0';
    split_files_params_.use_char_range = false;
    split_files_params_.use_int_range = true;
    split_files_params_.int_range_bounds.add(0, 1000);
    split_files_params_.default_int_range_start = 0;
    split_files_params_.use_str_range = false;
    split_files_params_.before_lexeme_out_str = "";
    split_files_params_.after_lexeme_out_str = " ";

    split_files_null_callback_ = new Logging::ActiveObjectCallbackImpl(
      Logging::Logger_var(new Logging::Null::Logger()));
  }

  bool
  FetchRouteProcessor::interrupt() /*throw(eh::Exception)*/
  {
    return !active();
  }

  void
  FetchRouteProcessor::process() noexcept
  {
    try
    {
      if(src_hosts_.empty())
      {
        return;
      }

      FileRouteParams file_route_params;
      std::string src_dir;
      PathManip::split_path(src_files_pattern_.c_str(), &src_dir, 0, false);
      file_route_params.dst_host = host_name_;

      for(StringList::const_iterator src_host_it = src_hosts_.begin();
          src_host_it != src_hosts_.end() &&
            (interrupter_ && !interrupter_->interrupt());
          ++src_host_it)
      {
        const char* src_host = src_host_it->c_str();
        StringList files_in_dir;

        if(make_file_list_for_fetch_(
            src_files_pattern_.c_str(),
            files_in_dir,
            *src_host_it,
            file_route_params.dst_host,
            src_dir))
        {
          for(StringList::const_iterator files_it = files_in_dir.begin();
              files_it != files_in_dir.end() &&
              (interrupter_ && !interrupter_->interrupt());
              ++files_it)
          {
            const char *current_file = files_it->c_str();

            std::string src_file;
            PathManip::split_path(current_file, 0, &src_file);

            if(src_file.empty())
            {
              Stream::Error ostr;
              ostr << "empty file name after split_path. Pattern: '" <<
                current_file << "'";
              error_logger_->add_error(*src_host_it,
                file_route_params.dst_host,
                src_dir,
                String::SubString("ADS-IMPL-203"),
                String::SubString("FetchRouteProcessor::process: "),
                ostr.str());
              /* don't interrupt, try other files */
              continue;
            }

            file_route_params.src_host = src_host;
            file_route_params.src_file_name = src_file;
            file_route_params.src_file_path = src_dir + src_file;
            file_route_params.dst_file_name = dst_dir_ + src_file;

            if(error_logger_->log_level() >= TraceLevel::HIGH)
            {
              Stream::Error ostr;
              ostr <<
                "SyncLogsImpl::fetch_logs(): preparing to send the '" <<
                current_file << "' file to '" <<
                file_route_params.dst_file_name << "'. ";
              error_logger_->log(ostr.str(),
                TraceLevel::HIGH, Aspect::SYNC_LOGS);
            }

            try
            {
              bool local_moving =
                host_checker_.check_host_name(file_route_params.src_host);

              if(local_moving)
              {
                local_file_router_->move(file_route_params);
                Utils::unlink_file(file_route_params.src_file_path.c_str());
              }
              else
              {
                remote_file_router_->move(file_route_params);
                unlink_remote_file_(
                  file_route_params.src_file_path.c_str(),
                  file_route_params.src_host.c_str());
              }
            }
            catch(const Utils::Exception& e)
            {
              Stream::Error ostr;
              ostr <<
                "Catch Utils::Exception on moving file. "
                "Route parameters:"
                " src host name = " << file_route_params.src_host <<
                " src file path = " << file_route_params.src_file_path <<
                " src file name = " << file_route_params.src_file_name <<
                " dst host name = " << file_route_params.dst_host <<
                " dst file name = " << file_route_params.dst_file_name <<
                " : " << e.what();

              error_logger_->add_error(*src_host_it,
                file_route_params.dst_host, src_dir,
                String::SubString("ADS-IMPL-203"),
                String::SubString("FetchRouteProcessor::process(): "),
                ostr.str());
              /* don't interrupt, try other files */
            }
            catch(const eh::Exception& e)
            {
              Stream::Error ostr;
              ostr <<
                "Catch eh::Exception on moving file. "
                "Route parameters:"
                " src host name = " << file_route_params.src_host <<
                " src file path = " << file_route_params.src_file_path <<
                " src file name = " << file_route_params.src_file_name <<
                " dst host name = " << file_route_params.dst_host <<
                " dst file name = " << file_route_params.dst_file_name <<
                " : " << e.what();
              error_logger_->add_error(*src_host_it,
                file_route_params.dst_host, src_dir,
                String::SubString("ADS-IMPL-203"),
                String::SubString("FetchRouteProcessor::process(): "),
                ostr.str());
              /* don't interrupt, try other files */
            }
          } /* files loop */
        }
      } /* hosts loop */
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << "FetchRouteProcessor::process(): "
        "Catch eh::Exception. : " << e.what();

      error_logger_->log(ostr.str(), Logging::Logger::ERROR,
        Aspect::SYNC_LOGS,
        "ADS-IMPL-202");
    }
  }

  bool
  FetchRouteProcessor::make_file_list_for_fetch_(
    const char* src_files,
    StringList &files_in_dir,
    const String::SubString& src_host,
    const String::SubString& dst_host,
    const String::SubString& src_dir)
    /*throw(Exception)*/
  {
    bool ret_val = false;

    // We are about to perform ssh remote_host "ls src_files"

    if (ssh_templ_.empty() || remote_ls_templ_.empty())
    {
      throw Exception(
        "AdServer::LogProcessing::SyncLogsImpl::make_file_list_for_fetch(): "
        "configuration parameters for remote operation not specified"
        );
    }

    try
    {
      std::ostringstream ssh_cmd_oss;
      String::TextTemplate::Args templ_args;
      templ_args[TemplateParams::HOST] = src_host.data();
      {
        std::string path, pattern;
        PathManip::split_path(src_files, &path, &pattern);
        templ_args[TemplateParams::PATH] = path;
        templ_args[TemplateParams::PATTERN] = pattern;
      }

      ssh_cmd_oss <<
        ssh_templ_.instantiate(templ_args) << ' ' <<
        remote_ls_templ_.instantiate(templ_args);

      bool read_output = true;
      std::string output_str = "";

      int exit_code =
        Utils::run_cmd(ssh_cmd_oss.str().c_str(), read_output, output_str);

      if (exit_code == 0)
      {
        if (!output_str.empty())
        {
          std::stringstream ss;
          ss << output_str;
          split_files_(ss, files_in_dir);

          for (StringList::iterator files_it = files_in_dir.begin();
               files_it != files_in_dir.end(); ++files_it)
          {
            if (files_it->empty())
            {
              // happens at end of input
              files_it = files_in_dir.erase(files_it);
            }
          }

          if (!files_in_dir.empty())
          {
            ret_val = true;
          }
        }
      }
      else if (exit_code == 255)
      {
        Stream::Error ostr;
        ostr << "error occurred while running the 'ssh' command. "
          "Command is '" << ssh_cmd_oss.str() << "' "
          "ssh exit code is 255.";

        error_logger_->add_error(src_host, dst_host, src_dir,
          String::SubString("ADS-IMPL-204"),
          String::SubString("SyncLogsImpl::make_file_list_for_fetch(): "),
          ostr.str());
      }
      else if (exit_code > 0)
      {
        Stream::Error ostr;
        ostr << "remote ls terminated with error. "
          "Command is '" << ssh_cmd_oss.str() << "' "
          "ls exit code is " << exit_code << ".";

        error_logger_->add_error(src_host, dst_host, src_dir,
          String::SubString("ADS-IMPL-204"),
          String::SubString("SyncLogsImpl::make_file_list_for_fetch(): "),
          ostr.str());
      }

      if (error_logger_->log_level() >= TraceLevel::MIDDLE)
      {
        Stream::Error ostr;
        ostr << "SyncLogsImpl::make_file_list_for_fetch(): " <<
          files_in_dir.size() << " files have been found.";
        error_logger_->log(ostr.str(), TraceLevel::MIDDLE,
          Aspect::SYNC_LOGS);
      }

      return ret_val;
    }
    catch (const String::TextTemplate::UnknownName &ex)
    {
      Stream::Error ostr;
      ostr << "FetchRouteProcessor::make_file_list_for_fetch_: "
        "String::TextTemplate::UnknownName caught. "
        ": " << ex.what();
      throw Exception(ostr);
    }
    catch (const String::TextTemplate::TextTemplException &ex)
    {
      Stream::Error ostr;
      ostr << "FetchRouteProcessor::make_file_list_for_fetch_: "
        "String::TextTemplate::Exception caught. "
        ": " << ex.what();
      throw Exception(ostr);
    }
    catch (const eh::Exception &ex)
    {
      Stream::Error ostr;
      ostr << "FetchRouteProcessor::make_file_list_for_fetch_: "
        "eh::Exception caught. : " << ex.what();
      throw Exception(ostr);
    }
  }

  void
  FetchRouteProcessor::unlink_remote_file_(
    const char* file, const char* host)
    /*throw(Exception)*/
  {
    try
    {
      String::TextTemplate::Args templ_args;
      templ_args[TemplateParams::HOST] = host;
      {
        std::string path, file_name;
        PathManip::split_path(file, &path, &file_name);
        templ_args[TemplateParams::PATH] = path;
        templ_args[TemplateParams::FILE_NAME] = file_name;
      }
      std::ostringstream ssh_cmd_oss;
      ssh_cmd_oss << ssh_templ_.instantiate(templ_args) << ' ' <<
        remote_rm_templ_.instantiate(templ_args);
      std::string output_str;

      int exit_code =
        Utils::run_cmd(ssh_cmd_oss.str().c_str(), false, output_str);

      if(exit_code == 255)
      {
        Stream::Error ostr;
        ostr << "SyncLogsImpl::unlink_file(" << file << "): "
          "error occured while running the 'ssh' command. "
          "Command is '" << ssh_cmd_oss.str() << "' "
          "ssh exit code is 255.";

        throw Exception(ostr);
      }
      else if (exit_code > 0)
      {
        Stream::Error ostr;
        ostr << "SyncLogsImpl::unlink_file(" << file << "): "
          "remote rm terminated with error."
          "Command is '" << ssh_cmd_oss.str() << "' "
          "rm exit code is " << exit_code << ".";

        throw Exception(ostr);
      }
    }
    catch (const String::TextTemplate::UnknownName &ex)
    {
      Stream::Error ostr;
      ostr << "FetchRouteProcessor::unlink_remote_file_: "
        "String::TextTemplate::UnknownName caught. "
        ": " << ex.what();
      throw Exception(ostr);
    }
    catch (const String::TextTemplate::TextTemplException &ex)
    {
      Stream::Error ostr;
      ostr << "FetchRouteProcessor::unlink_remote_file_: "
        "String::TextTemplate::Exception caught. "
        ": " << ex.what();
      throw Exception(ostr);
    }
    catch (const eh::Exception &ex)
    {
      Stream::Error ostr;
      ostr << "FetchRouteProcessor::unlink_remote_file_: "
        "eh::Exception caught. : " << ex.what();
      throw Exception(ostr);
    }
  }

  void
  FetchRouteProcessor::split_files_(
    std::istream& is,
    std::list<std::string>& files_in_dir)
    /*throw(Exception)*/
  {
    try
    {
      String::SequenceAnalyzer::Analyzer base_analyzer(
        split_files_params_, split_files_null_callback_);
      base_analyzer.process_char_sequence(is, files_in_dir);
    }
    catch (const String::SequenceAnalyzer::Analyzer::Exception &ex)
    {
      Stream::Error ostr;
      ostr << "Generics::interprete_base_sequence: "
        "Got Analyzer::Exception. : " <<
        ex.what();
      throw Exception(ostr);
    }
  }
}
}
