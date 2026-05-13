#pragma once

#include <functional>
#include <memory>

#include <boost/asio.hpp>

#include <Commons/DelegateActiveObject.hpp>

namespace AdServer::ImprTrack
{
  class ImprTrackFrontendWorkers:
    public AdServer::Commons::DelegateActiveObject
  {
  public:
    ImprTrackFrontendWorkers(
      Generics::ActiveObjectCallback* callback,
      unsigned long threads);

    void
    post(std::function<void()> task);

  protected:
    ~ImprTrackFrontendWorkers() noexcept override = default;

  private:
    void
    work_() noexcept override;

    void
    terminate_() noexcept override;

  private:
    using IoService = boost::asio::io_service;
    using Work = IoService::work;

    std::shared_ptr<IoService> io_service_;
    std::unique_ptr<Work> io_work_;
  };

  using ImprTrackFrontendWorkers_var =
    ReferenceCounting::SmartPtr<ImprTrackFrontendWorkers>;
}
