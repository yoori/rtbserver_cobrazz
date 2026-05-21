#pragma once

#include <string>

#include <eh/Exception.hpp>

namespace AdServer::Commons
{
  class PidFileGuard final
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    explicit PidFileGuard(std::string path);
    ~PidFileGuard() noexcept;

    PidFileGuard(const PidFileGuard&) = delete;
    PidFileGuard& operator=(const PidFileGuard&) = delete;

  private:
    std::string path_;
    std::string pid_;
  };
}
