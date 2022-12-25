// POSIX
#include <signal.h>
#include <sys/wait.h>

// STD
#include <vector>
#include <string>

// THIS
#include <Stream/MemoryStream.hpp>
#include "Process.hpp"

namespace PredictorSvcs
{
namespace BidCostPredictor
{

Process::Process(
  const std::string& bin_path,
  const std::vector<std::string>& options)
  : bin_path_(bin_path),
    options_(options)
{
}

Process::~Process()
{
}

void Process::launch()
{
  std::lock_guard lock(mutex_);
  if (is_stopped_)
    return;

  pid_ = fork();
  if (pid_ == -1)
  {
    Stream::Error ostr;
    ostr << __PRETTY_FUNCTION__
         << " : Reason : "
         << "fork is failed";
    throw Exception(ostr);
  }
  else if (pid_ == 0)
  {
    std::vector<char*> params;
    params.reserve(options_.size() + 2);
    params.emplace_back(const_cast<char*>(bin_path_.c_str()));
    for (const std::string& str : options_)
    {
      params.emplace_back(const_cast<char*>(str.c_str()));
    }
    params.emplace_back(nullptr);

    execvp(bin_path_.c_str(), params.data());
    exit(EXIT_FAILURE);
  }
}

int Process::wait() noexcept
{
  pid_t pid;
  {
    std::lock_guard guard(mutex_);
    if (pid_ == -1)
      return EXIT_SUCCESS;
    pid = pid_;
  }

  int status = 0;
  if (::waitpid(pid, &status, 0) == -1)
  {
    return EXIT_FAILURE;
  }

  if (WIFSIGNALED(status))
  {
    return EXIT_FAILURE;
  }
  else if (WIFEXITED(status))
  {
    const int exit_status = WEXITSTATUS(status);
    return exit_status;
  }
  else
  {
    return EXIT_FAILURE;
  }
}

void Process::stop() noexcept
{
  std::lock_guard guard(mutex_);
  is_stopped_ = true;
  if (pid_ != -1)
    kill(pid_, SIGINT);
}

} // namespace BidCostPredictor
} // namespace PredictorSvcs