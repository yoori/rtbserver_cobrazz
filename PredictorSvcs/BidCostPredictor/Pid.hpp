#ifndef BIDCOSTPREDICTOR_PID_HPP
#define BIDCOSTPREDICTOR_PID_HPP

// STD
#include <fstream>
#include <iterator>
#include <optional>
#include <string>

// POSIX
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

// THIS
#include <eh/Exception.hpp>
#include "Generics/Uncopyable.hpp"

namespace PredictorSvcs
{
namespace BidCostPredictor
{

class PidSetter : private Generics::Uncopyable
{
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
public:
  PidSetter(const std::string& file_path)
           : file_path_(file_path)
  {
  }

  ~PidSetter()
  {
    unlock();
    if (fd_ >= 0)
    {
      ftruncate(fd_, 0);
      close(fd_);
    }
  }

  bool set()
  {
    fd_ = open(
            file_path_.c_str(),
            O_RDWR|O_CREAT,
            S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if (fd_ < 0)
    {
      Stream::Error stream;
      stream << __PRETTY_FUNCTION__
             << "Can't open file="
             << file_path_;
      throw Exception(stream);
    }

    if (lockFile(fd_))
    {
      is_lock_ = true;
    }
    else
    {
      return false;
    }

    if (ftruncate(fd_, 0) == -1)
    {
      Stream::Error stream;
      stream << __PRETTY_FUNCTION__
             << " : Reason: ftruncate is failed";
      throw Exception(stream);
    }

    char buf[16];
    if (sprintf(buf, "%ld", static_cast<long>(getpid())) < 0)
    {
      Stream::Error ostr;
      ostr << __PRETTY_FUNCTION__
           << " : Reason: sprintf is failed";
      throw Exception(ostr);
    }

    if (write(fd_, buf, strlen(buf) + 1) == -1)
    {
      Stream::Error ostr;
      ostr << __PRETTY_FUNCTION__
           << " : Reason: write is failed";
      throw Exception(ostr);
    }

    return true;
  }

private:
  void unlock() noexcept
  {
    if (fd_ == -1 || !is_lock_)
      return;

    is_lock_ = false;
    unlockFile(fd_);
  }

  bool lockFile(const int fd) noexcept
  {
    struct flock fl;
    fl.l_type = F_WRLCK;
    fl.l_start = 0;
    fl.l_whence = SEEK_SET;
    fl.l_len = 0;
    if (fcntl(fd, F_SETLK, &fl) == -1)
    {
      return false;
    }
    else
    {
      return true;
    }
  }

  bool unlockFile(const int fd) noexcept
  {
    struct flock fl;
    fl.l_type = F_UNLCK;
    fl.l_start = 0;
    fl.l_whence = SEEK_SET;
    fl.l_len = 0;
    if (fcntl(fd, F_SETLK, &fl) == -1)
    {
      return false;
    }
    else
    {
      return true;
    }
  }

private:
  std::string file_path_;

  int fd_ = -1;

  bool is_lock_ = false;
};

class PidGetter : private Generics::Uncopyable
{
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
public:
  PidGetter(const std::string& file_path)
           : file_path_(file_path)
  {
  }

  std::optional<long> get()
  {
    std::ifstream fstream(file_path_);
    if (!fstream.is_open())
    {
      Stream::Error ostr;
      ostr << __PRETTY_FUNCTION__
           << " : Reason: Can't open file="
           << file_path_;
      throw Exception(ostr);
    }

    const std::string data{std::istreambuf_iterator<char>(fstream),
                           std::istreambuf_iterator<char>()};
    if (data.empty())
      return {};

    try
    {
      const long pid = std::stol(data);
      return pid;
    }
    catch (...)
    {
      return {};
    }
  }

private:
  std::string file_path_;

  int fd_ = -1;
};

} // namespace BidCostPredictor
} // namespace PredictorSvcs

#endif //BIDCOSTPREDICTOR_PID_HPP
