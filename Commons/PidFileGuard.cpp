#include "PidFileGuard.hpp"

#include <cerrno>
#include <cstring>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>

#include <Stream/MemoryStream.hpp>

namespace AdServer::Commons
{
  namespace
  {
    void create_directories(const std::string& path)
    {
      if (path.empty())
      {
        return;
      }

      std::string current;
      std::size_t pos = 0;

      if (path[0] == '/')
      {
        current = "/";
        pos = 1;
      }

      while (pos < path.size())
      {
        const std::size_t next = path.find('/', pos);
        const std::string part = path.substr(
          pos,
          next == std::string::npos ? std::string::npos : next - pos);
        pos = next == std::string::npos ? path.size() : next + 1;

        if (part.empty() || part == ".")
        {
          continue;
        }

        if (!current.empty() && current.back() != '/')
        {
          current += '/';
        }
        current += part;

        if (::mkdir(current.c_str(), 0755) == -1 && errno != EEXIST)
        {
          Stream::Error ostr;
          ostr << "Can't create pid file directory '" << current << "': " << std::strerror(errno);
          throw PidFileGuard::Exception(ostr);
        }
      }
    }

    std::string parent_path(const std::string& path)
    {
      const std::size_t pos = path.find_last_of('/');
      if (pos == std::string::npos)
      {
        return {};
      }

      return pos == 0 ? std::string("/") : path.substr(0, pos);
    }
  }

  PidFileGuard::PidFileGuard(std::string path)
    : path_(std::move(path)),
      pid_(std::to_string(::getpid()))
  {
    if (path_.empty())
    {
      throw Exception("pid file path is empty");
    }

    create_directories(parent_path(path_));

    const std::string tmp_path = path_ + "." + pid_ + ".tmp";
    std::ofstream stream(tmp_path, std::ios::out | std::ios::trunc);
    if (!stream)
    {
      Stream::Error ostr;
      ostr << "Can't open pid file '" << tmp_path << "'";
      throw Exception(ostr);
    }

    stream << pid_ << '\n';
    stream.close();
    if (!stream)
    {
      ::unlink(tmp_path.c_str());
      Stream::Error ostr;
      ostr << "Can't write pid file '" << tmp_path << "'";
      throw Exception(ostr);
    }

    if (::rename(tmp_path.c_str(), path_.c_str()) == -1)
    {
      const int error = errno;
      ::unlink(tmp_path.c_str());
      Stream::Error ostr;
      ostr << "Can't rename pid file '" << tmp_path << "' to '" <<
        path_ << "': " << std::strerror(error);
      throw Exception(ostr);
    }
  }

  PidFileGuard::~PidFileGuard() noexcept
  {
    std::ifstream stream(path_);
    std::string current_pid;
    stream >> current_pid;
    if (current_pid == pid_)
    {
      ::unlink(path_.c_str());
    }
  }
}
