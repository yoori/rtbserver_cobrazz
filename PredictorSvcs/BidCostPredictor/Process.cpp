// POSIX
#include <signal.h>
#include <sys/wait.h>

// STD
#include <vector>
#include <string>

// UNIXCOMMONS
#include <Generics/Function.hpp>
#include <Stream/MemoryStream.hpp>

// THIS
#include "Process.hpp"

namespace PredictorSvcs::BidCostPredictor
{

Process::Process(
  const std::string& bin_path,
  const Options& options)
  : bin_path_(bin_path),
    options_(options)
{
}

void Process::launch()
{
  std::lock_guard lock(mutex_);
  if (pid_ != -1)
  {
    Stream::Error stream;
    stream << FNS
           << "Process has already launched";
    throw Exception(stream);
  }

  pid_ = fork();
  if (pid_ == -1)
  {
    Stream::Error stream;
    stream << FNS
            << "Reason : fork is failed";
    throw Exception(stream);
  }
  else if (pid_ == 0)
  {
    std::vector<char*> params;
    params.reserve(options_.size() + 2);
    params.emplace_back(const_cast<char*>(bin_path_.c_str()));
    for (const auto& str : options_)
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
    {
      return EXIT_SUCCESS;
    }
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
  if (pid_ != -1)
  {
    kill(pid_, SIGINT);
  }
}

} // namespace PredictorSvcs::BidCostPredictor