#include "BoostAsioContextRunActiveObject.hpp"

namespace AdServer
{
  // BoostAsioContextRunActiveObject implementation
  BoostAsioContextRunActiveObject::BoostAsioContextRunActiveObject(
    Generics::ActiveObjectCallback* callback,
    std::shared_ptr<boost::asio::io_service> io_service,
    unsigned long threads)
    /*throw(Gears::Exception)*/
    : AdServer::Commons::DelegateActiveObject(callback, threads),
      io_service_(std::move(io_service))
  {}

  BoostAsioContextRunActiveObject::~BoostAsioContextRunActiveObject() noexcept
  {}

  void
  BoostAsioContextRunActiveObject::work_() noexcept
  {
    //static const char* FNE = "Acceptor::AcceptActiveObject::work_()";

    while(active())
    {
      io_service_->run();
    }
  }

  void
  BoostAsioContextRunActiveObject::terminate_() noexcept
  {
    io_service_->stop();
  }
}
