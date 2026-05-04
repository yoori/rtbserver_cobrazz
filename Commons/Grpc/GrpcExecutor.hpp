#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

#include <grpcpp/completion_queue.h>

#include <Generics/ActiveObject.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

namespace AdServer::Grpc
{
  struct GrpcCompletionEvent
  {
    virtual ~GrpcCompletionEvent() = default;
    virtual void proceed(bool ok) = 0;
  };

  class GrpcExecutor final
    : public Generics::SimpleActiveObject,
      public virtual ReferenceCounting::AtomicImpl
  {
  public:
    class CQ final
    {
    public:
      grpc::CompletionQueue& completion_queue() noexcept;

      template<typename Functor>
      bool execute(Functor&& functor);

    private:
      friend class GrpcExecutor;

      void run();
      void shutdown();

    private:
      grpc::CompletionQueue completion_queue_;
      std::mutex lock_;
      bool shutdown_requested_ = false;
    };

    explicit GrpcExecutor(std::size_t workers_number);

    std::shared_ptr<CQ> queue(unsigned int index) noexcept;

    template<typename Functor>
    bool execute(Functor&& functor);

  private:
    void activate_object_() override;
    void deactivate_object_() override;
    void wait_object_() override;

  private:
    struct CQHolder
    {
      explicit CQHolder(std::shared_ptr<CQ> queue);

      std::shared_ptr<CQ> queue;
      std::thread thread;
    };

    std::vector<CQHolder> holders_;
  };

  using GrpcExecutor_var = ReferenceCounting::SmartPtr<GrpcExecutor>;
}

template<typename Functor>
bool
AdServer::Grpc::GrpcExecutor::CQ::execute(Functor&& functor)
{
  std::lock_guard<std::mutex> lock(lock_);
  if (shutdown_requested_)
  {
    return false;
  }

  std::forward<Functor>(functor)();
  return true;
}

template<typename Functor>
bool
AdServer::Grpc::GrpcExecutor::execute(Functor&& functor)
{
  return queue(0)->execute(std::forward<Functor>(functor));
}
