#include "BufferWriter.hpp"

#include <ostream>

namespace AdServer::LogProcessing
{
  BufferWriter::BufferWriter(std::size_t reserve_size)
  {
    if (reserve_size != 0)
    {
      buffer_.reserve(reserve_size);
    }
  }

  BufferWriter::BufferWriter(const char* file_name, std::size_t buffer_limit)
    : file_(std::fopen(file_name, "wb")),
      buffer_limit_(buffer_limit)
  {
    good_ = file_ != nullptr;
    if (buffer_limit_ != 0)
    {
      buffer_.reserve(buffer_limit_);
    }
  }

  BufferWriter::~BufferWriter()
  {
    close();
  }

  void
  BufferWriter::clear() noexcept
  {
    buffer_.clear();
  }

  bool
  BufferWriter::empty() const noexcept
  {
    return buffer_.empty();
  }

  std::size_t
  BufferWriter::size() const noexcept
  {
    return buffer_.size();
  }

  const char*
  BufferWriter::data() const noexcept
  {
    return buffer_.data();
  }

  const std::string&
  BufferWriter::str() const noexcept
  {
    return buffer_;
  }

  void
  BufferWriter::reserve(std::size_t size)
  {
    buffer_.reserve(size);
  }

  void
  BufferWriter::append(const char* value)
  {
    append(std::string_view(value));
  }

  void
  BufferWriter::append(std::string_view value)
  {
    if (file_ && buffer_limit_ != 0 && value.size() >= buffer_limit_)
    {
      flush();
      write_direct_(value.data(), value.size());
      return;
    }

    flush_if_required_(value.size());
    buffer_.append(value.data(), value.size());
  }

  void
  BufferWriter::append(const std::string& value)
  {
    append(std::string_view(value));
  }

  void
  BufferWriter::write_to(std::ostream& out) const
  {
    out.write(buffer_.data(), buffer_.size());
  }

  void
  BufferWriter::flush()
  {
    if (!file_ || buffer_.empty())
    {
      return;
    }

    write_direct_(buffer_.data(), buffer_.size());
    buffer_.clear();
  }

  void
  BufferWriter::close()
  {
    if (!file_)
    {
      return;
    }

    flush();
    if (std::fclose(file_) != 0)
    {
      good_ = false;
    }
    file_ = nullptr;
  }

  bool
  BufferWriter::good() const noexcept
  {
    return good_;
  }

  void
  BufferWriter::write_direct_(const char* data, std::size_t size)
  {
    if (!file_)
    {
      good_ = false;
      return;
    }

    if (size != 0 && std::fwrite(data, 1, size, file_) != size)
    {
      good_ = false;
    }
  }

  BufferWriter&
  operator<<(BufferWriter& out, const char* value)
  {
    out.append(value);
    return out;
  }

  BufferWriter&
  operator<<(BufferWriter& out, std::string_view value)
  {
    out.append(value);
    return out;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const std::string& value)
  {
    out.append(value);
    return out;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const Generics::Uuid& value)
  {
    out << value.to_string(true);
    return out;
  }
}
