#pragma once

#include <cstdint>
#include <fstream>
#include <mutex>
#include <string>

namespace AdServer::UserInfoSvcs::UserInfoManagerDump
{
  struct DumpRecord
  {
    std::uint64_t request_time_us = 0;
    std::string method;
    std::string request_bytes;
    std::uint64_t response_time_us = 0;
    std::string response_bytes;
  };

  class Writer final
  {
  public:
    explicit Writer(const std::string& path);

    void write(const DumpRecord& record);

  private:
    std::mutex mutex_;
    std::ofstream out_;
  };

  class Reader final
  {
  public:
    explicit Reader(const std::string& path);

    bool read(DumpRecord& record);

  private:
    std::ifstream in_;
  };

  std::uint64_t now_us() noexcept;
}
