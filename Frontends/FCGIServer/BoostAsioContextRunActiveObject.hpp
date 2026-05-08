#pragma once

#include <memory>
#include <Commons/DelegateActiveObject.hpp>

#include <boost/asio.hpp>

namespace AdServer::Frontends
{
  // BoostAsioContextRunActiveObject
  class BoostAsioContextRunActiveObject: public AdServer::Commons::DelegateActiveObject
  {
  public:
    BoostAsioContextRunActiveObject(
      Generics::ActiveObjectCallback* callback,
      std::shared_ptr<boost::asio::io_service> io_service,
      unsigned long threads)
      /*throw(Gears::Exception)*/;

    virtual
    ~BoostAsioContextRunActiveObject() noexcept;

  protected:
    void
    work_() noexcept;

    virtual void
    terminate_() noexcept;

  protected:
    std::shared_ptr<boost::asio::io_service> io_service_;
  };
}
