#include "FrontendWorkers.hpp"

namespace FrontendCommons
{
  FrontendWorkers::FrontendWorkers(
    Generics::ActiveObjectCallback* callback,
    unsigned long threads)
    : AdServer::Commons::DelegateActiveObject(callback, threads, 1024 * 1024),
      io_service_(std::make_shared<IoService>()),
      io_work_(std::make_unique<Work>(*io_service_))
  {}

  void
  FrontendWorkers::post(std::function<void()> task)
  {
    io_service_->post(std::move(task));
  }

  void
  FrontendWorkers::schedule(
    const Generics::Time& timeout,
    std::function<void()> task)
  {
    const auto timeout_us = timeout.microseconds();
    auto timer = std::make_shared<SteadyTimer>(*io_service_);
    timer->expires_after(
      std::chrono::microseconds(timeout_us > 0 ? timeout_us : 0));
    timer->async_wait(
      [timer, task = std::move(task)](const boost::system::error_code& error)
      {
        if(!error)
        {
          task();
        }
      });
  }

  void
  FrontendWorkers::work_() noexcept
  {
    while(active())
    {
      try
      {
        io_service_->run();
      }
      catch(...)
      {}
    }
  }

  void
  FrontendWorkers::terminate_() noexcept
  {
    io_work_.reset();
    io_service_->stop();
  }
}
