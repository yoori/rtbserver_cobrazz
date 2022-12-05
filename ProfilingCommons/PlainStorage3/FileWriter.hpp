#ifndef FILEWRITER_HPP
#define FILEWRITER_HPP

#include <eh/Exception.hpp>
#include <Generics/MemBuf.hpp>

#include "FileController.hpp"

namespace AdServer
{
namespace ProfilingCommons
{
  class FileWriter
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    FileWriter(
      const char* file,
      unsigned long buffer_size,
      bool append = false,
      bool disable_caching = false,
      FileController* file_controller = 0)
      /*throw(Exception)*/;

    FileWriter(
      int fd,
      unsigned long buffer_size,
      FileController* file_controller = 0)
      /*throw(Exception)*/;

    ~FileWriter() noexcept;

    // result can be less then read_size only if reached eof
    void
    write(const void* val, unsigned long read_size)
      /*throw(Exception)*/;

    void
    flush() /*throw(Exception)*/;

    unsigned long
    size() const noexcept;

    void
    close() /*throw(Exception)*/;

  private:
    void
    write_(const void* val, unsigned long read_size)
      /*throw(Exception)*/;

  private:
    const unsigned long buffer_size_;
    const unsigned long direct_write_min_size_;
    FileController_var file_controller_;

    bool fd_own_;
    int fd_;
    unsigned long fd_pos_;

    Generics::SmartMemBuf_var mem_buf_;
  };
}
}

#endif /*FILEWRITER_HPP*/
