#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <eh/Errno.hpp>

#include "FileWriter.hpp"

namespace AdServer
{
namespace ProfilingCommons
{
  FileWriter::FileWriter(
    int fd,
    unsigned long buffer_size,
    FileController* file_controller)
    /*throw(Exception)*/
    : buffer_size_(buffer_size),
      direct_write_min_size_(std::max(buffer_size_ / 4, 1ul)),
      file_controller_(
        file_controller ? ReferenceCounting::add_ref(file_controller) :
        new PosixFileController()),
      fd_own_(false),
      fd_(fd),
      fd_pos_(0)
  {
    mem_buf_ = file_controller_->create_buffer();
  }
 
  FileWriter::FileWriter(
    const char* file_name,
    unsigned long buffer_size,
    bool append,
    bool disable_caching,
    FileController* file_controller)
    /*throw(Exception)*/
    : buffer_size_(buffer_size),
      direct_write_min_size_(
        disable_caching ?
        // don't allow direct read if caching disabled, because result buffer must be aligned
        std::numeric_limits<unsigned long>::max() :
        std::max(buffer_size_ / 4, 1ul)),
      file_controller_(
        file_controller ? ReferenceCounting::add_ref(file_controller) :
        new PosixFileController()),
      fd_own_(true),
      fd_pos_(0)
  {
    static const char* FUN = "FileWriter::FileWriter()";

    mem_buf_ = file_controller_->create_buffer();

    try
    {
      fd_ = file_controller_->open(
        file_name,
        O_RDWR | O_CREAT | (append ? O_APPEND : 0) |
          0,
          //(disable_caching ? O_DIRECT : 0) // temporary disabled - need implement buffers alignment
        S_IREAD | S_IWRITE);
    }
    catch(const FileController::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught FileController::Exception: " << ex.what();
      throw Exception(ostr.str());
    }
  }

  FileWriter::~FileWriter() noexcept
  {
    try
    {
      close();
    }
    catch(...)
    {}

    mem_buf_ = Generics::SmartMemBuf_var();
  }

  void
  FileWriter::close() /*throw(Exception)*/
  {
    flush();

    if(fd_own_ && fd_ >= 0)
    {
      ::close(fd_);
      fd_ = -1;
    }
  }

  void
  FileWriter::write(const void* write_buf, unsigned long write_size)
    /*throw(Exception)*/
  {
    unsigned long cur_write_size = write_size;
    const unsigned char* cur_write_buf =
      static_cast<const unsigned char*>(write_buf);

    if(buffer_size_ == 0)
    {
      write_(write_buf, write_size);
    }
    else
    {
      if(mem_buf_->membuf().capacity() == 0)
      {
        mem_buf_->membuf().alloc(buffer_size_);
        mem_buf_->membuf().resize(0);
      }

      while(cur_write_size)
      {
        // push into buffer
        unsigned long fill_size = std::min(
          mem_buf_->membuf().capacity() - mem_buf_->membuf().size(),
          cur_write_size);

        if(fill_size != 0)
        {
          unsigned long old_mem_buf_size = mem_buf_->membuf().size();
          mem_buf_->membuf().resize(old_mem_buf_size + fill_size);
          ::memcpy(
            static_cast<unsigned char*>(mem_buf_->membuf().data()) + old_mem_buf_size,
            cur_write_buf,
            fill_size);

          cur_write_buf += fill_size;
          cur_write_size -= fill_size;
        }

        // flush mem buf
        if(mem_buf_->membuf().size() == mem_buf_->membuf().capacity())
        {
          flush();
        }
      }
    }
  }

  void
  FileWriter::flush()
    /*throw(Exception)*/
  {
    if(mem_buf_->membuf().size() > 0)
    {
      write_(mem_buf_->membuf().data(), mem_buf_->membuf().size());
      mem_buf_->membuf().resize(0);
    }
  }

  unsigned long
  FileWriter::size() const noexcept
  {
    return fd_pos_ + mem_buf_->membuf().size();
  }

  void
  FileWriter::write_(const void* buf, unsigned long size)
    /*throw(Exception)*/
  {
    ssize_t res;

    try
    {
      res = file_controller_->write(fd_, buf, size);
    }
    catch(const FileController::Exception& ex)
    {
      Stream::Stack<1024> ostr;
      ostr << "FileWriter::write_(): at writing to pos=" << fd_pos_ <<
        " caught eh::Exception: " << ex.what();
      throw Exception(ostr.str());
    }

    fd_pos_ += res;
  }
}
}
