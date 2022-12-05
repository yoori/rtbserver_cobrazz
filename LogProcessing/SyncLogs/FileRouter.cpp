#include <sstream>

#include <eh/Exception.hpp>
#include <String/TextTemplate.hpp>
#include <Commons/PathManip.hpp>

#include "Utils.hpp"
#include "FileRouter.hpp"

namespace AdServer
{
namespace LogProcessing
{
  AppFileRouter::AppFileRouter(
    const char* command_template,
    const char* post_command_template)
    /*throw(Exception)*/
  {
    check_and_set_command_template_(
      command_template,
      command_template_);

    check_and_set_command_template_(
      post_command_template,
      post_command_template_);
  }

  void
  AppFileRouter::check_and_set_command_template_(
    const char* str_command_template,
    String::TextTemplate::IStream& command_template)
    /*throw(Exception)*/
  {
    static const char* FUN =
      "AppFileRouter::check_and_set_command_template_()";

    try
    {
      Stream::Parser istr(str_command_template);
      command_template.init(
        istr,
        TemplateParams::MARKER,
        TemplateParams::MARKER);

      /* test template */
      String::TextTemplate::Args templ_args;
      templ_args[TemplateParams::SRC_HOST] = "";
      templ_args[TemplateParams::SRC_PATH] = "";
      templ_args[TemplateParams::SRC_DIR] = "";
      templ_args[TemplateParams::DST_HOST] = "";
      templ_args[TemplateParams::DST_PATH] = "";
      templ_args[TemplateParams::DST_DIR] = "";
      templ_args[TemplateParams::FILE_NAME] = "";
      command_template.instantiate(templ_args);
    }
    catch (const String::TextTemplate::UnknownName &ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": "
        "String::TextTemplate::UnknownName caught while "
        "instantiating local_feed_cmd. : " << ex.what();
      throw Exception(ostr);
    }
    catch (const String::TextTemplate::TextTemplException &ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": "
        "String::TextTemplate::Exception caught while instantiating "
        "local_feed_cmd. : " << ex.what();
      throw Exception(ostr);
    }
    catch (const eh::Exception &ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": "
        "eh::Exception caught while instantiating "
        "local_feed_cmd. : " << ex.what();
      throw Exception(ostr);
    }
  }

  void
  AppFileRouter::move(
    const FileRouteParams& file_route_params,
    bool sync_mode,
    InterruptCallback* interrupter)
    /*throw(Exception)*/
  {
    static const char* FUN = "AppFileRouter::move()";

    {
      std::string output;
      std::string command;

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
        ostr << FUN << ": can't move '" <<
          file_route_params.src_file_path << "' command '" <<
          command << "' return error (" << exit_code << ").";
        throw Exception(ostr);
      }
    }

    {
      std::string output;
      std::string command;

      const int exit_code = run_command_(
        post_command_template_,
        file_route_params,
        sync_mode,
        command,
        output,
        0);

      if(exit_code != 0)
      {
        Stream::Error ostr;
        ostr << FUN << ": post command failed '" <<
          file_route_params.src_file_path << "' command '" <<
          command << "' return error (" << exit_code << ").";
        throw Exception(ostr);
      }
    }
  }

  void
  AppFileRouter::init_command_(
    const String::TextTemplate::IStream& command_template,
    const FileRouteParams& file_route_params,
    std::string& command,
    bool sync_mode)
    const
    /*throw(Exception)*/
  {
    static const char* FUN = "AppFileRouter::init_command_()";

    try
    {
      String::TextTemplate::Args templ_args;

      templ_args[TemplateParams::FILE_NAME] = file_route_params.src_file_name;

      templ_args[TemplateParams::SRC_HOST] = file_route_params.src_host;
      templ_args[TemplateParams::SRC_PATH] = file_route_params.src_file_path;
      templ_args[TemplateParams::SRC_DIR] = file_route_params.src_dir;

      templ_args[TemplateParams::DST_HOST] = file_route_params.dst_host;
      templ_args[TemplateParams::DST_PATH] = file_route_params.dst_file_name;

      std::string temp;
      PathManip::split_path(
        file_route_params.dst_file_name.c_str(),
        &templ_args[TemplateParams::DST_DIR],
        sync_mode ? 0 : &temp);

      // if sync_mode == true then split_log_name() must not expect
      // a pattern in a file name
      if(!sync_mode && temp.empty())
      {
        Stream::Error ostr;
        ostr << "," << file_route_params.dst_file_name <<
            "' not a file name";
        throw Exception(ostr);
      }

      command = command_template.instantiate(templ_args);
    }
    catch (const String::TextTemplate::UnknownName &ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": "
        "String::TextTemplate::UnknownName caught while "
        "instantiating command_template. : " << ex.what();
      throw Exception(ostr);
    }
    catch (const String::TextTemplate::TextTemplException &ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": "
        "String::TextTemplate::Exception caught while instantiating "
        "command_template. : " << ex.what();
      throw Exception(ostr);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": "
        "Can't init command template. Caught eh::Exception. "
        ": " << ex.what();
      throw Exception(ostr);
    }
  }

  int
  AppFileRouter::run_command_(
    const String::TextTemplate::IStream& command_template,
    const FileRouteParams& file_route_params,
    bool sync_mode,
    std::string& command,
    std::string& output,
    InterruptCallback* interrupter)
    const
    /*throw(Exception)*/
  {
    static const char* FUN = "AppFileRouter::run_command_()";

    if (!command_template.empty())
    {
      init_command_(
        command_template,
        file_route_params,
        command,
        sync_mode);

      try
      {
        return Utils::run_cmd(command.c_str(), true, output, interrupter);
      }
      catch (const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN <<
          ": Can't execute command: '" << command << "'" <<
          " Caught eh::Exception. : " << ex.what();
        throw Exception(ostr);
      }
    }

    return 0;
  }
}
}
