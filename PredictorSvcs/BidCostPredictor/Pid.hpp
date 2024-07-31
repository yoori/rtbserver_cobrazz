#ifndef BIDCOSTPREDICTOR_PID_HPP
#define BIDCOSTPREDICTOR_PID_HPP

// POSIX
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

// STD
#include <fstream>
#include <iterator>
#include <optional>
#include <string>

// UNIXCOMMONS
#include <eh/Exception.hpp>
#include <Generics/Uncopyable.hpp>

namespace PredictorSvcs::BidCostPredictor
{

class PidSetter final : private Generics::Uncopyable
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  explicit PidSetter(const std::string& file_path)
    : file_path_(file_path)
  {
  }

  ~PidSetter()
  {
    unlock();
    if (fd_ >= 0)
    {
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
      stream << FNS
             << "Can't open file="
             << file_path_;
      throw Exception(stream);
    }

    if (lock_file(fd_))
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
      stream << FNS
             << " Reason: ftruncate is failed";
      throw Exception(stream);
    }

    char buf[16];
    if (sprintf(buf, "%ld", static_cast<long>(getpid())) < 0)
    {
      Stream::Error stream;
      stream << FNS
             << "Reason: sprintf is failed";
      throw Exception(stream);
    }

    if (write(fd_, buf, strlen(buf) + 1) == -1)
    {
      Stream::Error stream;
      stream << FNS
             << "Reason: write is failed";
      throw Exception(stream);
    }

    return true;
  }

private:
  void unlock() noexcept
  {
    if (fd_ == -1 || !is_lock_)
      return;

    is_lock_ = false;
    ftruncate(fd_, 0);
    unlock_file(fd_);
  }

  bool lock_file(const int fd) noexcept
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

  bool unlock_file(const int fd) noexcept
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

class PidGetter final : private Generics::Uncopyable
{
public:
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
      Stream::Error stream;
      stream << FNS
             << "Reason: Can't open file="
             << file_path_;
      throw Exception(stream);
    }

    const std::string data{
      std::istreambuf_iterator<char>(fstream),
      std::istreambuf_iterator<char>()};
    if (data.empty())
    {
      return {};
    }

    try
    {
      const long pid = std::stol(data);
      return pid;
    }
    catch (...)
    {
    }

    return {};
  }

private:
  std::string file_path_;

  int fd_ = -1;
};

} // namespace PredictorSvcs::BidCostPredictor

#endif //BIDCOSTPREDICTOR_PID_HPP
