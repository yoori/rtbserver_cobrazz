#pragma once

#include <cassert>
#include <functional>
#include <memory>
#include <thread>
#include <utility>

namespace AdServer::Commons
{
  class DeferredDeletionQueue final
  {
    struct State;

  public:
    class Registration final
    {
    public:
      ~Registration() noexcept;
      Registration(const Registration&) = delete;
      Registration& operator=(const Registration&) = delete;

      void complete() noexcept;

    private:
      friend class DeferredDeletionQueue;

      explicit Registration(std::shared_ptr<State> state);

      const std::shared_ptr<State> state_;
      bool pending_ = false; // Protected by state_->lock.
    };

    DeferredDeletionQueue();
    ~DeferredDeletionQueue() noexcept;

    std::shared_ptr<Registration> register_deletion();
    void enqueue(std::function<void()> task);
    void wait() noexcept;
    void drain() noexcept;

  private:
    static void run_(const std::shared_ptr<State>& state) noexcept;

    const std::shared_ptr<State> state_;
    std::thread worker_;
  };

  template<typename T>
  struct ActiveObjectDeleter
  {
    void operator()(T* object) const noexcept
    {
      // Explicit shutdown must keep an owning reference until wait_object() has completed.
      if (object->active())
      {
        object->deactivate_object();
        object->wait_object();
      }

      delete object;
    }
  };

  template<typename T, typename Deleter = std::default_delete<T>>
  class DeferredDeleter
  {
    struct BoundDeleter
    {
      std::shared_ptr<DeferredDeletionQueue> queue;
      Deleter deleter;
      std::shared_ptr<DeferredDeletionQueue::Registration> registration;

      void operator()(T* object) const noexcept
      {
        queue->enqueue([object, deleter = deleter, registration = registration]() mutable
          {
            deleter(object);
            registration->complete();
          });
      }
    };

  public:
    explicit DeferredDeleter(std::shared_ptr<DeferredDeletionQueue> queue, Deleter deleter = {})
      : queue_(std::move(queue)), deleter_(std::move(deleter))
    {}

    auto bind() const
    {
      return BoundDeleter{queue_, deleter_, queue_->register_deletion()};
    }

    // The caller has completed final shutdown while retaining an owning reference.
    static void complete(const std::shared_ptr<T>& object) noexcept
    {
      const auto* deleter = std::get_deleter<BoundDeleter>(object);
      assert(deleter);
      deleter->registration->complete();
    }

    void wait() const noexcept
    {
      queue_->wait();
    }

  private:
    std::shared_ptr<DeferredDeletionQueue> queue_;
    Deleter deleter_;
  };

  template<typename T, typename Deleter = std::default_delete<T>, typename... Args>
  std::shared_ptr<T> make_shared_with_deleter(Deleter deleter, Args&&... args)
  {
    if constexpr (requires { deleter.bind(); })
    {
      // Register before construction and before any owning reference can escape.
      auto bound_deleter = deleter.bind();
      return std::shared_ptr<T>(new T(std::forward<Args>(args)...), std::move(bound_deleter));
    }
    else
    {
      return std::shared_ptr<T>(new T(std::forward<Args>(args)...), std::move(deleter));
    }
  }
}
