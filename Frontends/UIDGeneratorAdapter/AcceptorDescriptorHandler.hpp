#ifndef ACCEPTORDESCRIPTORHANDLER_HPP_
#define ACCEPTORDESCRIPTORHANDLER_HPP_

#include "DescriptorHandlerPoller.hpp"

namespace AdServer
{
  class AcceptorDescriptorHandler: public DescriptorHandler
  {
  public:
    AcceptorDescriptorHandler(
      unsigned long port,
      DescriptorHandlerPoller::Proxy* poller_proxy)
      /*throw(Exception)*/;

    virtual int
    fd() const noexcept;

    // return false if need stop loop
    virtual unsigned long
    read() /*throw(Exception)*/;

    // return false if need stop loop
    virtual unsigned long
    write() /*throw(Exception)*/;

    virtual void
    stopped() noexcept;

  protected:
    virtual
    ~AcceptorDescriptorHandler() noexcept;

    void
    accept_() /*throw(Exception)*/;

    virtual DescriptorHandler_var
    create_descriptor_handler(int fd)
      noexcept = 0;

  protected:
    DescriptorHandlerPoller::Proxy_var proxy_;
    int fd_;
  };
}

namespace AdServer
{
}

#endif /*ACCEPTORDESCRIPTORHANDLER_HPP_*/
