#include <sys/statvfs.h>
#include <fcntl.h>
#include <unistd.h>

#include <eh/Errno.hpp>
#include <Stream/MemoryStream.hpp>

#include "FileController.hpp"

namespace AdServer
{
namespace ProfilingCommons
{
  // StatImpl
  StatImpl::Counters::Counters() noexcept
    : count(0)
  {}

  Generics::Time
  StatImpl::Counters::avg_time() const noexcept
  {
    return count > 0 ?
      sum_time / count : Generics::Time::ZERO;
  }

  StatImpl::Counters
  StatImpl::read_counters() const noexcept
  {
    SyncPolicy::ReadGuard lock(read_counters_lock_);
    return read_counters_;
  }

  StatImpl::Counters
  StatImpl::write_counters() const noexcept
  {
    SyncPolicy::ReadGuard lock(write_counters_lock_);
    return write_counters_;
  }

  ReferenceCounting::SmartPtr<StatImpl>
  StatImpl::reset()
    noexcept
  {
    ReferenceCounting::SmartPtr<StatImpl> res = new StatImpl();

    {
      SyncPolicy::WriteGuard lock(read_counters_lock_);
      std::swap(res->read_counters_, read_counters_);
    }

    {
      SyncPolicy::WriteGuard lock(write_counters_lock_);
      std::swap(res->write_counters_, write_counters_);
    }

    return res;
  }

  void
  StatImpl::add_read_time_(
    const Generics::Time& start,
    const Generics::Time& stop,
    unsigned long)
    noexcept
  {
    add_time_(start, stop, read_counters_lock_, read_counters_);
  }

  void
  StatImpl::add_write_time_(
    const Generics::Time& start,
    const Generics::Time& stop,
    unsigned long)
    noexcept
  {
    add_time_(start, stop, write_counters_lock_, write_counters_);
  }

  void
  StatImpl::add_time_(
    const Generics::Time& start,
    const Generics::Time& stop,
    SyncPolicy::Mutex& counters_lock,
    Counters& counters)
    noexcept
  {
    const Generics::Time time = stop - start;
    SyncPolicy::WriteGuard lock(counters_lock);
    counters.max_time = std::max(counters.max_time, time);
    counters.sum_time += time;
    ++counters.count;
  }

  // PosixFileController::Device
  inline
  PosixFileController::Device::Device() noexcept
    : write_size_meter(0)
  {}

  // PosixFileController
  PosixFileController::PosixFileController(
    Stat* pread_stat,
    uint64_t min_free_space,
    unsigned long free_space_check_size_period)
    noexcept
    : allocator_(new Generics::Allocator::Align()),
      min_free_space_(min_free_space),
      free_space_check_size_period_(
        free_space_check_size_period > 0 ?
        free_space_check_size_period :
        min_free_space / 100),
      stat_(ReferenceCounting::add_ref(pread_stat))
  {
  }

  Generics::SmartMemBuf_var
  PosixFileController::create_buffer() const noexcept
  {
    return new Generics::SmartMemBuf(allocator_.in());
  }

  int
  PosixFileController::open(
    const char* file_name, int flags, mode_t mode)
    /*throw(Exception)*/
  {
    static const char* FUN = "PosixFileController::open()";

    int fd = ::open64(file_name, flags, mode);

    if(fd < 0)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't open file '" << file_name << "'";
      eh::throw_errno_exception<Exception>(ostr.str());
    }

    if(control_devices_())
    {
      struct statvfs fs_stat;

      if(::fstatvfs(fd, &fs_stat) != 0)
      {
        Stream::Error ostr;
        ostr << "PosixFileController::write(): error on statvfs.";
        eh::throw_errno_exception<Exception>(ostr.str());
      }

      Device_var device;

      {
        DevicesSyncPolicy::WriteGuard dev_lock(devices_lock_);
        DeviceMap::iterator device_it = devices_.insert(
          std::make_pair(fs_stat.f_fsid, Device_var())).first;
        if(!device_it->second.in())
        {
          device_it->second = new Device();
        }
        device = device_it->second;
      }

      {
        FilesSyncPolicy::WriteGuard files_lock(files_lock_);
        files_.insert(std::make_pair(fd, device));
      }
    }

    return fd;
  }

  void
  PosixFileController::close(int fd) /*throw(Exception)*/
  {
    if(control_devices_())
    {
      FilesSyncPolicy::WriteGuard files_lock(files_lock_);
      files_.erase(fd);
    }

    ::close(fd);
  }

  ssize_t
  PosixFileController::pread(
    int fd, void* buf, unsigned long read_size, unsigned long fd_pos)
    /*throw(Exception)*/
  {
    if(stat_)
    {
      Generics::Timer timer;
      timer.start();
      const ssize_t res = pread_(fd, buf, read_size, fd_pos);
      timer.stop();
      stat_->add_read_time_(timer.start_time(), timer.stop_time(), read_size);
      return res;
    }

    return pread_(fd, buf, read_size, fd_pos);
  }

  ssize_t
  PosixFileController::read(
    int fd, void* buf, unsigned long read_size)
    /*throw(Exception)*/
  {
    if(stat_)
    {
      Generics::Timer timer;
      timer.start();
      const ssize_t res = read_(fd, buf, read_size);
      timer.stop();
      stat_->add_read_time_(timer.start_time(), timer.stop_time(), read_size);
      return res;
    }

    return read_(fd, buf, read_size);
  }

  ssize_t
  PosixFileController::write(
    int fd, const void* buf, unsigned long write_size)
    /*throw(Exception)*/
  {
    static const char* FUN = "PosixFileController::write()";

    if(control_devices_())
    {
      Device_var device;

      {
        FilesSyncPolicy::ReadGuard files_lock(files_lock_);
        FileMap::const_iterator file_it = files_.find(fd);
        assert(file_it != files_.end() && file_it->second.in());
        device = file_it->second;
      }

      int old_size = device->write_size_meter.exchange_and_add(write_size);

      if(old_size + write_size > free_space_check_size_period_)
      {
        struct statvfs fs_stat;

        if(::fstatvfs(fd, &fs_stat) != 0)
        {
          Stream::Stack<1024> ostr;
          ostr << "PosixFileController::write(): error on statvfs.";
          eh::throw_errno_exception<Exception>(ostr.str());
        }

        // use super user available space (
        //   can be compensated by min_free_space_ value)
        const uint64_t device_free_space = static_cast<uint64_t>(
          fs_stat.f_bsize) * fs_stat.f_bfree;

        if(device_free_space < min_free_space_)
        {
          // recheck space on each write if here no space
          device->write_size_meter = free_space_check_size_period_ + 1;

          Stream::Error ostr;
          ostr << FUN << ": min free space reached"
            ", free space = " << device_free_space <<
            ", min_free_space = " << min_free_space_;
          throw Exception(ostr);
        }

        device->write_size_meter = 0;
      }
    }

    if (stat_)
    {
      Generics::Timer timer;
      timer.start();
      const ssize_t res = write_(fd, buf, write_size);;
      timer.stop();
      stat_->add_write_time_(timer.start_time(), timer.stop_time(), write_size);
      return res;
    }

    return write_(fd, buf, write_size);
  }

  ssize_t
  PosixFileController::pread_(
    int fd, void* buf, unsigned long read_size, unsigned long fd_pos)
    /*throw(Exception)*/
  {
    ssize_t res = ::pread64(fd, buf, read_size, fd_pos);

    if(res < 0)
    {
      Stream::Stack<1024> ostr;
      ostr << "PosixFileController::pread_(): error on file reading.";
      eh::throw_errno_exception<Exception>(ostr.str());
    }

    return res;
  }

  ssize_t
  PosixFileController::read_(
    int fd, void* buf, unsigned long read_size)
    /*throw(Exception)*/
  {
    ssize_t res = ::read(fd, buf, read_size);

    if(res < 0)
    {
      Stream::Stack<1024> ostr;
      ostr << "PosixFileController::read_(): error on file reading.";
      eh::throw_errno_exception<Exception>(ostr.str());
    }

    return res;
  }

  ssize_t
  PosixFileController::write_(
    int fd, const void* buf, unsigned long write_size)
    /*throw(Exception)*/
  {
    ssize_t res = ::write(fd, buf, write_size);

    if(res < 0 || static_cast<unsigned long>(res) != write_size) // only error if fd is in blocking state
    {
      Stream::Stack<1024> ostr;
      ostr << "PosixFileController::write_(): error on file writing.";
      eh::throw_errno_exception<Exception>(ostr.str());
    }

    return res;
  }

  inline
  bool
  PosixFileController::control_devices_() const noexcept
  {
    return min_free_space_ > 0;
  }

  // SSDFileController
  SSDFileController::SSDFileController(
    FileController* delegate_file_controller,
    unsigned long write_block_size)
    noexcept
    : delegate_file_controller_(ReferenceCounting::add_ref(delegate_file_controller)),
      write_block_size_(write_block_size)
  {}

  Generics::SmartMemBuf_var
  SSDFileController::create_buffer() const noexcept
  {
    return delegate_file_controller_->create_buffer();
  }

  int
  SSDFileController::open(
    const char* file_name, int flags, mode_t mode)
    /*throw(Exception)*/
  {
    return delegate_file_controller_->open(file_name, flags, mode);
  }

  void
  SSDFileController::close(int fd) /*throw(Exception)*/
  {
    delegate_file_controller_->close(fd);
  }

  ssize_t
  SSDFileController::pread(
    int fd, void* buf, unsigned long read_size, unsigned long fd_pos)
    /*throw(Exception)*/
  {
    SyncPolicy::ReadGuard lock(operations_lock_);
    return delegate_file_controller_->pread(fd, buf, read_size, fd_pos);
  }

  ssize_t
  SSDFileController::read(
    int fd, void* buf, unsigned long read_size)
    /*throw(Exception)*/
  {
    SyncPolicy::ReadGuard lock(operations_lock_);
    return delegate_file_controller_->read(fd, buf, read_size);
  }

  ssize_t
  SSDFileController::write(
    int fd, const void* buf, unsigned long write_size)
    /*throw(Exception)*/
  {
    const unsigned long blocks_count = write_size / write_block_size_;
    for(unsigned long i = 0; i < blocks_count; ++i)
    {
      SyncPolicy::WriteGuard lock(operations_lock_);
      delegate_file_controller_->write(
        fd,
        static_cast<const unsigned char*>(buf) + i * write_block_size_,
        write_block_size_);
    }

    const unsigned long rest_size = write_size - blocks_count * write_block_size_;
    if(rest_size > 0)
    {
      SyncPolicy::WriteGuard lock(operations_lock_);
      delegate_file_controller_->write(
        fd,
        static_cast<const unsigned char*>(buf) + blocks_count * write_block_size_,
        rest_size);
    }

    return write_size;
  }
}
}
