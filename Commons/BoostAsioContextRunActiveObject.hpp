#pragma once

#include <functional>
#include <memory>
#include <string>

#include <boost/asio.hpp>

#include <Commons/DelegateActiveObject.hpp>
#include <Generics/Time.hpp>

namespace AdServer::Commons
{
  class BoostAsioContextRunActiveObject:
    public AdServer::Commons::DelegateActiveObject
  {
  public:
    using IoService = boost::asio::io_service;

    BoostAsioContextRunActiveObject(
      Generics::ActiveObjectCallback* callback,
      std::shared_ptr<IoService> io_service,
      unsigned long threads,
      unsigned long stack_size = 128 * 1024,
      std::string thread_name = "grpc-asio-p")
      /*throw(Gears::Exception)*/;

    ~BoostAsioContextRunActiveObject() noexcept override;

    std::shared_ptr<IoService>
    io_service() const noexcept;

    void
    post(std::function<void()> task);

    void
    schedule(const Generics::Time& timeout, std::function<void()> task);

  protected:
    void
    work_() noexcept override;

    void
    terminate_() noexcept override;

  private:
    using Work = IoService::work;
    using SteadyTimer = boost::asio::steady_timer;

    std::shared_ptr<IoService> io_service_;
    std::unique_ptr<Work> io_work_;
    std::string thread_name_;
  };
}
