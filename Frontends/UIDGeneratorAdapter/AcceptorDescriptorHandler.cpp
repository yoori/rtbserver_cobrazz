#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>

#include <eh/Errno.hpp>

#include "AcceptorDescriptorHandler.hpp"

namespace AdServer
{
  AcceptorDescriptorHandler::AcceptorDescriptorHandler(
    unsigned long port,
    DescriptorHandlerPoller::Proxy* poller_proxy)
    /*throw(Exception)*/
    : proxy_(ReferenceCounting::add_ref(poller_proxy))
  {
    static const char* FUN = "AcceptorDescriptorHandler::AcceptorDescriptorHandler()";

    fd_ = socket(AF_INET, SOCK_STREAM, 0);

    if(fd_ < 0)
    {
      eh::throw_errno_exception<Exception>(errno, FUN, ": socket() failed");
    }

    DescriptorHandlerPoller::set_non_blocking(fd_);

    sockaddr_in addr;
    ::memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if(::bind(fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
    {
      Stream::Error ostr;
      ostr << ": bind on port = " << port << " failed";
      eh::throw_errno_exception<Exception>(errno, FUN, ostr.str());
    }

    if(::listen(fd_, 1024) < 0)
    {
      eh::throw_errno_exception<Exception>(errno, FUN, ": listen() failed");
    }
  }

  AcceptorDescriptorHandler::~AcceptorDescriptorHandler()
    noexcept
  {
    ::close(fd_);
  }

  int
  AcceptorDescriptorHandler::fd() const noexcept
  {
    return fd_;
  }

  unsigned long
  AcceptorDescriptorHandler::read() /*throw(Exception)*/
  {
    accept_();

    return DescriptorHandler::CONTINUE_HANDLE;
  }

  unsigned long
  AcceptorDescriptorHandler::write() /*throw(Exception)*/
  {
    accept_();

    return DescriptorHandler::CONTINUE_HANDLE;
  }

  void
  AcceptorDescriptorHandler::stopped() noexcept
  {}

  void
  AcceptorDescriptorHandler::accept_() /*throw(Exception)*/
  {
    static const char* FUN = "AcceptorDescriptorHandler::accept_()";

    sockaddr_in cli_addr;
    socklen_t cli_addr_len;
    ::memset(&cli_addr, 0, sizeof(cli_addr));
    ::memset(&cli_addr_len, 0, sizeof(cli_addr_len));

    while(true)
    {
      int accepted_fd = ::accept(
        fd_,
        reinterpret_cast<sockaddr*>(&cli_addr),
        &cli_addr_len);

      if(accepted_fd < 0)
      {
        if(errno != EAGAIN && errno != EWOULDBLOCK)
        {
          eh::throw_errno_exception<Exception>(errno, FUN, "accept() failed");
        }
        else
        {
          break;
        }
      }
      else
      {
        DescriptorHandler_var accepted_descriptor_handler =
          create_descriptor_handler(accepted_fd);

        if(accepted_descriptor_handler)
        {
          proxy_->add(accepted_descriptor_handler);
        }
      }
    }
  }
}

