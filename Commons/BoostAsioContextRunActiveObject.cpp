#include "BoostAsioContextRunActiveObject.hpp"

#include <Commons/ThreadName.hpp>

#include <algorithm>
#include <chrono>
#include <utility>

namespace AdServer::Commons
{
  BoostAsioContextRunActiveObject::BoostAsioContextRunActiveObject(
    Generics::ActiveObjectCallback* callback,
    std::shared_ptr<IoService> io_service,
    unsigned long threads,
    unsigned long stack_size,
    std::string thread_name)
    /*throw(Gears::Exception)*/
    : AdServer::Commons::DelegateActiveObject(callback, threads, stack_size),
      io_service_(std::move(io_service)),
      io_work_(std::make_unique<Work>(*io_service_)),
      thread_name_(std::move(thread_name))
  {}

  BoostAsioContextRunActiveObject::~BoostAsioContextRunActiveObject() noexcept
  {}

  std::shared_ptr<BoostAsioContextRunActiveObject::IoService>
  BoostAsioContextRunActiveObject::io_service() const noexcept
  {
    return io_service_;
  }

  void
  BoostAsioContextRunActiveObject::post(std::function<void()> task)
  {
    io_service_->post(std::move(task));
  }

  void
  BoostAsioContextRunActiveObject::schedule(
    const Generics::Time& timeout,
    std::function<void()> task)
  {
    const auto timeout_us = timeout.microseconds();
    auto timer = std::make_shared<SteadyTimer>(*io_service_);
    timer->expires_after(std::chrono::microseconds(std::max<long long>(0, timeout_us)));
    timer->async_wait(
      [timer, task = std::move(task)](const boost::system::error_code& error) mutable
      {
        if (!error)
        {
          task();
        }
      });
  }

  void
  BoostAsioContextRunActiveObject::work_() noexcept
  {
    set_current_thread_name(thread_name_);

    while (active())
    {
      try
      {
        io_service_->run();
      }
      catch (...)
      {}
    }
  }

  void
  BoostAsioContextRunActiveObject::terminate_() noexcept
  {
    io_work_.reset();
    io_service_->stop();
  }
}
