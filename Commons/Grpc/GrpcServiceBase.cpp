#include "GrpcServiceBase.hpp"

#include <chrono>
#include <iostream>

namespace AdServer::Grpc
{
#ifdef ADS_GRPC_BATCH_STREAM_DEBUG_TIMEOUT
  GrpcBatchStreamDebugTimerService&
  GrpcBatchStreamDebugTimerService::instance()
  {
    static auto* service = new GrpcBatchStreamDebugTimerService();
    return *service;
  }

  void
  GrpcBatchStreamDebugTimerService::schedule(
    std::shared_ptr<WatchdogState> state)
  {
    auto timer = std::make_shared<boost::asio::steady_timer>(io_service_);
    timer->expires_after(std::chrono::seconds(20));
    timer->async_wait(
      [
        timer = std::move(timer),
        state = std::move(state)
      ](const boost::system::error_code& error)
      {
        if (!error && state &&
          !state->done.load(std::memory_order_acquire))
        {
          std::cout
            << "ADS_GRPC_BATCH_STREAM_DEBUG_TIMEOUT: "
            << "response timeout after 20 sec"
            << ", peer=" << state->peer
            << ", batch_items=" << state->items_size;
          if (!state->first_method.empty())
          {
            std::cout << ", first_method=" << state->first_method;
          }
          std::cout << std::endl;
        }
      });
  }

  GrpcBatchStreamDebugTimerService::GrpcBatchStreamDebugTimerService()
    : work_(io_service_),
      thread_([this] { io_service_.run(); })
  {}
#endif

  std::uint64_t
  GrpcServiceBase::InprogressStats::add(
    const std::uint64_t call_inflight,
    const Generics::Time& read_time)
  {
    const auto receiver_id = next_receiver_id_.fetch_add(
      1,
      std::memory_order_relaxed);

    std::lock_guard<std::mutex> lock(lock_);
    requests_.emplace(receiver_id, Request{read_time, call_inflight});
    call_inflight_ += call_inflight;
    if (!min_time_of_request_in_progress_ ||
      read_time < *min_time_of_request_in_progress_)
    {
      min_time_of_request_in_progress_ = read_time;
    }

    return receiver_id;
  }

  void
  GrpcServiceBase::InprogressStats::remove(
    const std::uint64_t receiver_id) noexcept
  {
    std::lock_guard<std::mutex> lock(lock_);
    const auto it = requests_.find(receiver_id);
    if (it == requests_.end())
    {
      return;
    }

    call_inflight_ -= it->second.call_inflight;
    requests_.erase(it);
    recalculate_min_time_();
  }

  GrpcServiceBase::InprogressStatsSnapshot
  GrpcServiceBase::InprogressStats::snapshot() const
  {
    std::lock_guard<std::mutex> lock(lock_);
    return InprogressStatsSnapshot{
      call_inflight_,
      min_time_of_request_in_progress_};
  }

  void
  GrpcServiceBase::InprogressStats::recalculate_min_time_() noexcept
  {
    min_time_of_request_in_progress_.reset();
    for (const auto& [_, request] : requests_)
    {
      if (!min_time_of_request_in_progress_ ||
        request.read_time < *min_time_of_request_in_progress_)
      {
        min_time_of_request_in_progress_ = request.read_time;
      }
    }
  }

  GrpcServiceBase::~GrpcServiceBase() noexcept = default;

  void
  GrpcServiceBase::register_services(::grpc::ServerBuilder& builder)
  {
    for (auto* service : grpc_services_)
    {
      builder.RegisterService(service);
    }
  }

  void
  GrpcServiceBase::start(const CompletionQueues& completion_queues)
  {
    grpc_operation_gate_.activate_object();

    const auto registrations = registrations_per_queue();
    for (auto* completion_queue : completion_queues)
    {
      for (std::size_t i = 0; i < registrations; ++i)
      {
        register_in_queue(completion_queue);
      }
    }
  }

  void
  GrpcServiceBase::stop_accepting_requests() noexcept
  {
    grpc_operation_gate_.deactivate_object();
    grpc_operation_gate_.wait_object();
  }

  void
  GrpcServiceBase::stop_finishing_requests() noexcept
  {}

  GrpcServiceBase::InprogressStatsSnapshot
  GrpcServiceBase::inprogress_stats() const
  {
    return inprogress_stats_->snapshot();
  }

  std::size_t
  GrpcServiceBase::registrations_per_queue() const noexcept
  {
    return 64;
  }

  void
  GrpcServiceBase::add_grpc_service(::grpc::Service* service)
  {
    grpc_services_.push_back(service);
  }

  void
  GrpcServiceBase::handle_batch_request(
    const adserver::grpc::BatchRequest& batch_request,
    adserver::grpc::BatchResponse& batch_response) const
  {
    for (int i = 0; i < batch_request.items_size(); ++i)
    {
      const auto& request_item = batch_request.items(i);
      auto* response_item = batch_response.add_items();
      response_item->set_request_id(request_item.request_id());

      const auto it = batch_methods_.find(request_item.full_method());
      if (it == batch_methods_.end())
      {
        response_item->set_status_code(::grpc::StatusCode::UNIMPLEMENTED);
        response_item->set_status_message("Unknown method");
        continue;
      }

      try
      {
        it->second(request_item, *response_item);
      }
      catch (const std::exception& ex)
      {
        response_item->set_status_code(::grpc::StatusCode::INTERNAL);
        response_item->set_status_message(ex.what());
      }
      catch (...)
      {
        response_item->set_status_code(::grpc::StatusCode::INTERNAL);
        response_item->set_status_message("Unknown batch handler exception");
      }
    }
  }

  AdServer::Commons::ActivityGate::Guard
  GrpcServiceBase::enter_grpc_operation() noexcept
  {
    return grpc_operation_gate_.enter();
  }
}
