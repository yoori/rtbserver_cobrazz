#pragma once

#include <functional>
#include <memory>

#include <boost/asio.hpp>

#include <Commons/DelegateActiveObject.hpp>
#include <Generics/Time.hpp>

namespace AdServer::Bidding
{
  class BiddingFrontendWorkers:
    public AdServer::Commons::DelegateActiveObject
  {
  public:
    BiddingFrontendWorkers(
      Generics::ActiveObjectCallback* callback,
      unsigned long threads);

    void
    post(std::function<void()> task);

    void
    schedule(
      const Generics::Time& timeout,
      std::function<void()> task);

  protected:
    ~BiddingFrontendWorkers() noexcept override = default;

  private:
    void
    work_() noexcept override;

    void
    terminate_() noexcept override;

  private:
    using IoService = boost::asio::io_service;
    using Work = IoService::work;
    using SteadyTimer = boost::asio::steady_timer;

    std::shared_ptr<IoService> io_service_;
    std::unique_ptr<Work> io_work;
  };

  using BiddingFrontendWorkers_var =
    ReferenceCounting::SmartPtr<BiddingFrontendWorkers>;
}
