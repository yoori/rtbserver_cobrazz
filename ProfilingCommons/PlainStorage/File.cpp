#include "File.hpp"

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <iostream>
#include <Stream/MemoryStream.hpp>
#include <eh/Errno.hpp>

namespace
{
  /// The size of shared memory portion. Data block hold into some portions
  const std::size_t SYSTEM_PAGE_SIZE = ::getpagesize();
}

namespace Sys
{
  File::File() noexcept: file_handle_(-1)
  {}

  File::File(const char* file_name, FileOpening flag) /*throw(PosixException)*/
  {
    open(file_name, flag);
  }

  File::~File() noexcept
  {
    try
    {
      close();
    }
    catch(...)
    {}
  }

  void File::open(const char* file_name, FileOpening flag) /*throw(PosixException)*/
  {
    // std::cout << "File::open(" << file_name << ")" << std::endl;

    int mode = 0;
    int o_flags = 0;

    switch(flag)
    {
    case FO_READ: o_flags = O_RDONLY; break;
    case FO_RW: o_flags = O_RDWR; break;
    case FO_RW_CREATE: o_flags = O_RDWR | O_CREAT; mode = S_IWRITE | S_IREAD; break;
    };

    file_handle_ = ::open64(file_name, o_flags, mode);

    if(file_handle_ == -1)
    {
      eh::throw_errno_exception<File::PosixException>(
        "Can't open file '", file_name, "'");
    }
  }

  void File::close() /*throw(PosixException)*/
  {
    if(file_handle_ != -1)
    {
      int file_handle = file_handle_;
      file_handle_ = -1;

      if(::close(file_handle) == -1)
      {
        eh::throw_errno_exception<File::PosixException>(
          "Can't close file");
      }
    }
  }

  OffsetType File::size() /*throw(PosixException)*/
  {
    struct stat64 f_stat;

    if(::fstat64(file_handle_, &f_stat))
    {
      eh::throw_errno_exception<File::PosixException>(
        "Can't do fstat64");
    }

    return f_stat.st_size;
  }

  void File::alloc(OffsetType pos, OffsetType size)
    /*throw(PosixException)*/
  {
    static const char* FUN = "File::alloc()";

    if(::posix_fallocate(file_handle_, pos, size) != 0)
    {
      Stream::Stack<1024> ostr;
      ostr << FUN << ": Can't do fallocate: pos = " << pos <<
        ", size = " << size;
      eh::throw_errno_exception<PosixException>(ostr.str());
    }
  }

  void File::resize(OffsetType new_size) /*throw(PosixException)*/
  {
    static const char* FUN = "File::resize()";

    if(::ftruncate64(file_handle_, new_size) != 0)
    {
      Stream::Stack<1024> ostr;
      ostr << FUN << ": Can't do ftruncate64. Requested size is " <<
        new_size;
      eh::throw_errno_exception<PosixException>(ostr.str());
    }
  }

  void*
  File::mmap(
    OffsetType pos, OffsetType size, MemoryMapping flag)
    /*throw(PosixException)*/
  {
    int mode = 0;
    switch(flag)
    {
      case MM_READ: mode = PROT_READ; break;
      case MM_RW: mode = PROT_READ | PROT_WRITE; break;
    };

    void* mem_ptr =
      ::mmap64(
        0,
        size,
        mode,
        MAP_SHARED,
        file_handle_,
        pos);

    if(mem_ptr == 0 || mem_ptr == MAP_FAILED)
    {
      Stream::Stack<1024> ostr;
      ostr << "File::mmap(): Can't map to memory file block "
        "with pos = " << pos << " and size = " << size << ".";
      eh::throw_errno_exception<PosixException>(ostr.str());
    }

    return mem_ptr;
  }

  void
  File::msync(void* mem_ptr, OffsetType size)
    /*throw(PosixException)*/
  {
    if(::msync(mem_ptr, size, MS_SYNC))
    {
      Stream::Stack<1024> ostr;
      ostr << "File::msync(): Can't msync memory block with size = " <<
        size << ".";
      eh::throw_errno_exception<PosixException>(ostr.str());
    }
  }

  void
  File::munmap(void* mem_ptr, OffsetType size)
    /*throw(PosixException)*/
  {
    /*
    if(::msync(mem_ptr, size, MS_ASYNC))
    {
      eh::throw_errno_exception<PosixException>(
        "File::munmap(): Can't msync memory block.");
    }
    */
    
    if(::munmap(mem_ptr, size))
    {
      eh::throw_errno_exception<PosixException>(
        "File::munmap(): Can't unmap memory block.");
    }
  }

  void
  File::pread(void* buf, unsigned long read_size, unsigned long pos)
    /*throw(PosixException)*/
  {
    if(::pread(file_handle_, buf, read_size, pos))
    {
      eh::throw_errno_exception<PosixException>(
        "File::pread(): Can't do pread.");
    }
  }

  void
  File::pwrite(const void* buf, unsigned long read_size, unsigned long pos)
    /*throw(PosixException)*/
  {
    if(::pwrite(file_handle_, buf, read_size, pos))
    {
      eh::throw_errno_exception<PosixException>(
        "File::pread(): Can't do pwrite.");
    }
  }

  int
  File::get_page_size() noexcept
  {
    return SYSTEM_PAGE_SIZE;
  }

} // namespace Sys

