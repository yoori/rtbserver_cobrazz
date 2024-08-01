#include <iostream>
#include <sstream>
#include <vector>

#include <eh/Errno.hpp>
#include <eh/Exception.hpp>
#include <Generics/Listener.hpp>
#include <String/TextTemplate.hpp>

#include "Utils.hpp"
#include "FeedRouteProcessor.hpp"

namespace
{
  template<typename OStream>
  void form_run_cmd_error_string(
    OStream& oss,
    const char* add_info,
    const char* command,
    int status)
    /*throw(eh::Exception)*/
  {
    oss << "SyncLogsImpl::run_cmd:" << (add_info ? add_info : "") <<
      "Status: " << status <<
      ", while trying to execute the command: " <<
      command;
  }

  class ArgvTokenizer
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    ArgvTokenizer(const String::SubString& str):
      str_(str),
      cur_(str_.begin())
    {}

    std::string get_token() /*throw(Exception)*/;

    virtual ~ArgvTokenizer() {}

  private:
    String::SubString str_;
    String::SubString::Pointer cur_;
  };

  std::string
  ArgvTokenizer::get_token() /*throw(Exception)*/
  {
    std::string token;
    bool opening_quote = false;
    while (cur_ != str_.end())
    {
      if (std::isspace(*cur_))
      {
        if (opening_quote)
        {
          token.push_back(*cur_);
        }
        else
        {
          cur_++;
          return token;
        }
      }
      else if (*cur_ == '\'')
      {
        opening_quote = !opening_quote;
      }
      else
      {
        token.push_back(*cur_);
      }
      cur_++;
    }
    if (opening_quote)
    {
      throw Exception("Unclosed quote");
    }

    return token;
  }

  class Callback :
    public Generics::ExecuteAndListenCallback,
    public ReferenceCounting::AtomicImpl
  {
  public:
    Callback(std::string* output, std::string& error,
      AdServer::LogProcessing::InterruptCallback* interrupter)
      /*throw(eh::Exception)*/;

    virtual void
    on_data_ready(int fd, std::size_t fd_index, const char* str,
      std::size_t size) noexcept;

    virtual void
    on_periodic() noexcept;

    virtual void
    report_error(Severity severity, const String::SubString& description,
      const char* error_code = 0) noexcept;

    virtual void
    set_pid(pid_t pid) noexcept;

  protected:
    virtual
    ~Callback() noexcept;

  private:
    std::string* output_;
    std::string& error_;
    AdServer::LogProcessing::InterruptCallback* interrupter_;
    pid_t pid_;
  };

  Callback::Callback(std::string* output, std::string& error,
    AdServer::LogProcessing::InterruptCallback* interrupter)
    /*throw(eh::Exception)*/
    : output_(output), error_(error), interrupter_(interrupter)
  {}

  Callback::~Callback() noexcept
  {}

  void
  Callback::on_data_ready(int /*fd*/, std::size_t fd_index, const char* str,
    std::size_t size) noexcept
  {
    try
    {
      (output_ && !fd_index ? *output_ : error_).append(str, size);
    }
    catch (...)
    {
      report_error(ERROR, String::SubString("Memory allocation error"));
    }
  }

  void
  Callback::on_periodic() noexcept
  {
    if (interrupter_ && interrupter_->interrupt())
    {
      kill(-pid_, SIGKILL);
    }
  }

  void
  Callback::report_error(Severity /*severity*/,
    const String::SubString& /*description*/,
    const char* /*error_code*/) noexcept
  {
    // FIXME
  }

  void
  Callback::set_pid(pid_t pid) noexcept
  {
    pid_ = pid;
  }
}

namespace AdServer
{
namespace LogProcessing
{
  namespace Utils
  {
    /* TODO: form_run_cmd_error_string */

    int
    run_cmd(const char *cmdline, bool read_output, std::string& output_str,
      InterruptCallback* interrupter) /*throw(eh::Exception)*/
    {
      std::vector<std::string> arg_str_vec;
      try
      {
        ArgvTokenizer tokenizer = String::SubString(cmdline);
        std::string token;
        while (!(token = tokenizer.get_token()).empty())
        {
          arg_str_vec.push_back(token);
        }
      }
      catch (const ArgvTokenizer::Exception& ex)
      {
        Stream::Error ostr;
        std::ostringstream token_oss;
        token_oss << "Auxiliary::StringTokenizer::Exception caught: " <<
          ex.what() << ' ';
        form_run_cmd_error_string(ostr, token_oss.str().c_str(), cmdline, 0);
        throw Exception(ostr);
      }

      if (arg_str_vec.empty())
      {
        throw Exception("parsed args is empty");
      }

      std::string cmd = arg_str_vec[0];
      std::string::size_type pos = arg_str_vec[0].find_last_of('/');
      if (pos != std::string::npos)
      {
        arg_str_vec[0].erase(
          arg_str_vec[0].begin(),
          arg_str_vec[0].begin() + pos + 1);
      }

      std::vector<const char*> arg_ptr_vec;
      arg_ptr_vec.reserve(arg_str_vec.size() + 1);

      for (std::vector<std::string>::const_iterator it = arg_str_vec.begin();
        it != arg_str_vec.end(); ++it)
      {
        arg_ptr_vec.push_back(it->c_str());
      }

      arg_ptr_vec.push_back(0);

      static const int LISTEN[] = { STDOUT_FILENO };
      static const int REDIRECT[] = { STDIN_FILENO, STDERR_FILENO };
      std::string error;
      Generics::ExecuteAndListenCallback_var callback(
        new Callback(read_output ? &output_str : 0, error, interrupter));
      int status = Generics::execute_and_listen(callback, cmd.c_str(),
        const_cast<char**>(&arg_ptr_vec[0]), read_output ? 1 : 0,
        LISTEN, 2, REDIRECT, 4096, false, true);
      if (!error.empty())
      {
        Stream::Error ostr;
        form_run_cmd_error_string(ostr, error.c_str(), cmdline, status);
        throw Exception(ostr);
      }

      if (!WIFEXITED(status))
      {
        Stream::Error ostr;
        form_run_cmd_error_string(ostr, " child finished abnormally.",
          cmdline, status);
        throw Exception(ostr);
      }

      return WEXITSTATUS(status);
    }

    unsigned int find_host_num(
      const char *file_name,
      unsigned int dest_hosts_size)
      /*throw(Exception, eh::Exception)*/
    {
      static const char* FUN = "Utils::find_host_num()";

      // NEED REVIEW
      std::string fname(file_name);
      std::string::size_type pos1 = fname.find_first_of('_');
      std::string::size_type pos2 = fname.find_last_of('_');
      std::string::size_type pos_point = fname.find_first_of('.', pos1);
      if (pos1 + 4 > pos2 || !(pos1 < pos_point && pos_point < pos2) ||
        pos2 == std::string::npos)
      {
        Stream::Error ostr;
        ostr << FUN << ": file name '" << file_name <<
          "' does not match the requested pattern.";
        throw Exception(ostr);
      }

      std::string dest_pattern_str(fname, pos1 + 1, pos2 - pos1 - 1);
      const char *dest_pattern = dest_pattern_str.c_str();

      unsigned int num;
      unsigned int from_num;

      if (std::sscanf(dest_pattern, "%10u.%10u", &num, &from_num) != 2)
      {
        Stream::Error ostr;
        ostr << FUN << ": file name '" << file_name <<
          "' does not match the requested pattern.";
        throw Exception(ostr);
      }

      if (from_num != dest_hosts_size)
      {
        Stream::Error ostr;
        ostr << FUN << ": "
          "the number of destination hosts in the '" <<
          file_name << "' file name does not match "
          "the number of listed destination hosts. "
          "Should be " << dest_hosts_size << " instead "
          "of " << from_num << ".";
        throw Exception(ostr);
      }

      if (num == 0 || num > from_num)
      {
        Stream::Error ostr;
        ostr << FUN << ": "
          "the number of destination host in the '" <<
          file_name << "' file name is not valid.";
        throw Exception(ostr);
      }

      return num - 1;
    }

    void unlink_file(const char* file) /*throw(NotFound, UnlinkException)*/
    {
      if (::unlink(file) == -1)
      {
        if(errno == ENOENT)
        {
          eh::throw_errno_exception<NotFound>("Utils::unlink_file(",
            file, ") failed.");
        }
        else
        {
          char string[sizeof(eh::Exception)];
          eh::ErrnoHelper::compose_safe(
            string, sizeof(string), errno,
            "Utils::unlink_file(", file, ") failed.");
          UnlinkException ex(string);
          ex.file_name = file;
          throw ex;
        }
      }
    }
  } /* Utils */
} /* LogProcessing */
} /* AdServer */
