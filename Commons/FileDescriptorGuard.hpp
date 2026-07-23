#pragma once

#include <cstddef>
#include <memory>

#include <unistd.h>

namespace AdServer::Commons
{
  struct FileDescriptor
  {
    FileDescriptor() noexcept = default;
    FileDescriptor(std::nullptr_t) noexcept;
    explicit FileDescriptor(int fd_val) noexcept;

    FileDescriptor&
    operator=(std::nullptr_t) noexcept;

    explicit
    operator bool() const noexcept;

    int fd = -1;
  };

  bool
  operator==(FileDescriptor left, FileDescriptor right) noexcept;

  bool
  operator!=(FileDescriptor left, FileDescriptor right) noexcept;

  bool
  operator==(FileDescriptor fd, std::nullptr_t) noexcept;

  bool
  operator!=(FileDescriptor fd, std::nullptr_t) noexcept;

  bool
  operator==(std::nullptr_t, FileDescriptor fd) noexcept;

  bool
  operator!=(std::nullptr_t, FileDescriptor fd) noexcept;

  struct FileDescriptorDeleter
  {
    using pointer = FileDescriptor;

    void
    operator()(FileDescriptor fd) const noexcept;
  };

  using FileDescriptorGuard = std::unique_ptr<void, FileDescriptorDeleter>;
}

namespace AdServer::Commons
{
  inline
  FileDescriptor::FileDescriptor(std::nullptr_t) noexcept
  {}

  inline
  FileDescriptor::FileDescriptor(int fd_val) noexcept
    : fd(fd_val)
  {}

  inline FileDescriptor&
  FileDescriptor::operator=(std::nullptr_t) noexcept
  {
    fd = -1;
    return *this;
  }

  inline
  FileDescriptor::operator bool() const noexcept
  {
    return fd >= 0;
  }

  inline bool
  operator==(FileDescriptor left, FileDescriptor right) noexcept
  {
    return left.fd == right.fd;
  }

  inline bool
  operator!=(FileDescriptor left, FileDescriptor right) noexcept
  {
    return !(left == right);
  }

  inline bool
  operator==(FileDescriptor fd, std::nullptr_t) noexcept
  {
    return fd.fd < 0;
  }

  inline bool
  operator!=(FileDescriptor fd, std::nullptr_t) noexcept
  {
    return !(fd == nullptr);
  }

  inline bool
  operator==(std::nullptr_t, FileDescriptor fd) noexcept
  {
    return fd == nullptr;
  }

  inline bool
  operator!=(std::nullptr_t, FileDescriptor fd) noexcept
  {
    return !(fd == nullptr);
  }

  inline void
  FileDescriptorDeleter::operator()(FileDescriptor fd) const noexcept
  {
    if (fd.fd >= 0)
    {
      ::close(fd.fd);
    }
  }
}
