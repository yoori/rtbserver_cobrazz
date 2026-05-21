#include <Commons/Grpc/GrpcExecutor.hpp>

#include <Commons/ThreadName.hpp>

#include <algorithm>
#include <memory>
#include <thread>

namespace AdServer::Grpc
{
  GrpcExecutor::CQHolder::CQHolder(std::shared_ptr<CQ> queue_val)
    : queue(std::move(queue_val))
  {}

  GrpcExecutor::GrpcExecutor(std::size_t workers_number)
  {
    const auto safe_workers_number = std::max<std::size_t>(1, workers_number);
    holders_.reserve(safe_workers_number);
    for (std::size_t i = 0; i < safe_workers_number; ++i)
    {
      holders_.emplace_back(std::make_shared<CQ>());
    }
  }

  grpc::CompletionQueue&
  GrpcExecutor::CQ::completion_queue() noexcept
  {
    return completion_queue_;
  }

  void
  GrpcExecutor::CQ::run()
  {
    void* tag = nullptr;
    bool ok = false;
    while (completion_queue_.Next(&tag, &ok))
    {
      std::unique_ptr<GrpcCompletionEvent> event(
        static_cast<GrpcCompletionEvent*>(tag));
      event->proceed(ok);
    }
  }

  void
  GrpcExecutor::CQ::shutdown()
  {
    std::lock_guard<std::mutex> lock(lock_);
    shutdown_requested_ = true;
    completion_queue_.Shutdown();
  }

  std::shared_ptr<GrpcExecutor::CQ>
  GrpcExecutor::queue(unsigned int index) noexcept
  {
    return holders_[index % holders_.size()].queue;
  }

  void
  GrpcExecutor::activate_object_()
  {
    for (auto& holder : holders_)
    {
      auto queue = holder.queue;
      holder.thread = std::thread([queue]() {
        AdServer::Commons::set_current_thread_name("grpc-pool");
        queue->run();
      });
    }
  }

  void
  GrpcExecutor::deactivate_object_()
  {
    for (auto& holder : holders_)
    {
      holder.queue->shutdown();
    }
  }

  void
  GrpcExecutor::wait_object_()
  {
    for (auto& holder : holders_)
    {
      if (holder.thread.joinable())
      {
        holder.thread.join();
      }
    }
  }
}
