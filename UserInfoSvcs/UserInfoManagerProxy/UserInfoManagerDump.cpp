#include "UserInfoManagerDump.hpp"

#include <chrono>
#include <stdexcept>

namespace AdServer::UserInfoSvcs::UserInfoManagerDump
{
  namespace
  {
    constexpr char DUMP_MAGIC[] = "UIMDMP1";
    constexpr std::uint32_t DUMP_VERSION = 1;

    template<typename Integer>
    void write_integer(std::ostream& out, Integer value)
    {
      out.write(
        reinterpret_cast<const char*>(&value),
        static_cast<std::streamsize>(sizeof(value)));
      if (!out)
      {
        throw std::runtime_error("failed to write dump file");
      }
    }

    template<typename Integer>
    bool read_integer(std::istream& in, Integer& value)
    {
      in.read(
        reinterpret_cast<char*>(&value),
        static_cast<std::streamsize>(sizeof(value)));
      return static_cast<bool>(in);
    }

    void write_string(std::ostream& out, const std::string& value)
    {
      write_integer(out, static_cast<std::uint64_t>(value.size()));
      out.write(value.data(), static_cast<std::streamsize>(value.size()));
      if (!out)
      {
        throw std::runtime_error("failed to write dump file");
      }
    }

    bool read_string(std::istream& in, std::string& value)
    {
      std::uint64_t size = 0;
      if (!read_integer(in, size))
      {
        return false;
      }

      value.resize(static_cast<std::size_t>(size));
      in.read(value.data(), static_cast<std::streamsize>(value.size()));
      return static_cast<bool>(in);
    }
  }

  Writer::Writer(const std::string& path)
    : out_(path, std::ios::binary | std::ios::trunc)
  {
    if (!out_)
    {
      throw std::runtime_error("failed to open dump file for writing: " + path);
    }

    out_.write(DUMP_MAGIC, sizeof(DUMP_MAGIC));
    write_integer(out_, DUMP_VERSION);
  }

  void Writer::write(const DumpRecord& record)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    write_integer(out_, record.request_time_us);
    write_string(out_, record.method);
    write_string(out_, record.request_bytes);
    write_integer(out_, record.response_time_us);
    write_string(out_, record.response_bytes);
    out_.flush();
    if (!out_)
    {
      throw std::runtime_error("failed to flush dump file");
    }
  }

  Reader::Reader(const std::string& path)
    : in_(path, std::ios::binary)
  {
    if (!in_)
    {
      throw std::runtime_error("failed to open dump file for reading: " + path);
    }

    char magic[sizeof(DUMP_MAGIC)] = {};
    in_.read(magic, sizeof(magic));
    std::uint32_t version = 0;
    if (!read_integer(in_, version) ||
      std::string(magic, sizeof(magic)) != std::string(DUMP_MAGIC, sizeof(DUMP_MAGIC)) ||
      version != DUMP_VERSION)
    {
      throw std::runtime_error("invalid UserInfoManager dump file: " + path);
    }
  }

  bool Reader::read(DumpRecord& record)
  {
    if (in_.peek() == std::ifstream::traits_type::eof())
    {
      return false;
    }

    return
      read_integer(in_, record.request_time_us) &&
      read_string(in_, record.method) &&
      read_string(in_, record.request_bytes) &&
      read_integer(in_, record.response_time_us) &&
      read_string(in_, record.response_bytes);
  }

  std::uint64_t now_us() noexcept
  {
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::microseconds>(now).count());
  }
}
