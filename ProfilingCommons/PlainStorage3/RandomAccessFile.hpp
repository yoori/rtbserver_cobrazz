#ifndef RANDOMACCESSFILE_HPP
#define RANDOMACCESSFILE_HPP

// We support large files, use mmap64 instead mmap, etc
#ifndef PS_NOT_USE_LARGEFILES

#define _LARGEFILE64_SOURCE 1
#define _FILE_OFFSET_BITS 64

#endif

#include <sys/types.h>

#include <eh/Exception.hpp>
#include "FileController.hpp"

/**
 * Wrappers over low-level system functions
 */
namespace AdServer
{
namespace ProfilingCommons
{
  /**
   * Handle work with file, that contain user data. Allow open, close, resize,
   * map part of file to memory and unmap chunk of file from memory
   */
  class RandomAccessFile
  {
  public:
    DECLARE_EXCEPTION(PosixException, eh::DescriptiveException);

  public:
    RandomAccessFile(FileController* file_controller = 0)
      noexcept;

    /**
     * This constructor open file_name with specified flag
     */
    RandomAccessFile(
      const char* file_name,
      FileController* file_controller = 0)
      /*throw(PosixException)*/;

    /**
     * Destructor close file
     */
    ~RandomAccessFile() noexcept;

    void
    open(const char* file_name) /*throw(PosixException)*/;

    void
    close() /*throw(PosixException)*/;

    /**
     * @return information about a file - its size.
     */
    unsigned long
    size() /*throw(PosixException)*/;

    void
    pread(void* buf, unsigned long read_size, unsigned long pos)
      /*throw(PosixException)*/;

    int
    fd() const noexcept;

  protected:
    FileController_var file_controller_;
    /// Descriptor for file opening
    int file_handle_;
  };
}
}

#endif // RANDOMACCESSFILE_HPP
