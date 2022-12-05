#ifndef FILEREADER_HPP
#define FILEREADER_HPP

#include <eh/Exception.hpp>
#include <Generics/MemBuf.hpp>

#include "FileController.hpp"

namespace AdServer
{
namespace ProfilingCommons
{
  class FileReader
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    FileReader(
      int fd,
      bool exclusive,
      unsigned long buffer_size,
      FileController* file_controller = 0)
      /*throw(Exception)*/;

    FileReader(
      const char* file_name,
      unsigned long buffer_size,
      bool disable_caching = false,
      FileController* file_controller = 0)
      /*throw(Exception)*/;

    ~FileReader() noexcept;

    // result can be less then read_size only if reached eof
    unsigned long
    read(void* val, unsigned long read_size)
      /*throw(Exception)*/;

    // result can be less then skip_size only if reached eof
    unsigned long
    skip(unsigned long skip_size)
      /*throw(Exception)*/;

    bool eof() const noexcept;

    unsigned long
    pos() const noexcept;

    unsigned long
    file_size() const noexcept;

  private:
    unsigned long
    read_(void* val, unsigned long read_size)
      /*throw(Exception)*/;

    unsigned long
    read_mem_buf_()
      /*throw(Exception)*/;

  private:
    const bool exclusive_;
    const unsigned long buffer_size_;
    const unsigned long direct_read_min_size_;
    const unsigned long seek_min_size_;
    FileController_var file_controller_;

    int fd_;
    bool fd_own_;
    unsigned long file_size_;
    unsigned long fd_pos_;
    bool eof_;

    Generics::SmartMemBuf_var mem_buf_;
    unsigned long mem_buf_pos_;
  };
}
}

#endif /*FILEREADER_HPP*/
