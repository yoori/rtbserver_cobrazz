#include "AdFrontendWorkers.hpp"

namespace AdServer
{
  AdFrontendWorkers::AdFrontendWorkers(
    Generics::ActiveObjectCallback* callback,
    unsigned long threads)
    : AdServer::Commons::DelegateActiveObject(callback, threads, 1024 * 1024),
      io_service_(std::make_shared<IoService>()),
      io_work_(std::make_unique<Work>(*io_service_))
  {}

  void
  AdFrontendWorkers::post(std::function<void()> task)
  {
    io_service_->post(std::move(task));
  }

  void
  AdFrontendWorkers::work_() noexcept
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
  AdFrontendWorkers::terminate_() noexcept
  {
    io_work_.reset();
    io_service_->stop();
  }
}
