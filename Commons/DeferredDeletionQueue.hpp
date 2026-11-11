#pragma once

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
    DeferredDeletionQueue();
    ~DeferredDeletionQueue() noexcept;

    void enqueue(std::function<void()> task);
    void wait() noexcept;

  private:
    static void run_(const std::shared_ptr<State>& state) noexcept;

    const std::shared_ptr<State> state_;
    std::thread worker_;
  };

  template<typename T>
  struct ActiveObjectDeleter
  {
    void operator()(T* object) const noexcept;
  };

  template<typename T, typename Deleter = std::default_delete<T>>
  class DeferredDeleter
  {
  public:
    explicit DeferredDeleter(std::shared_ptr<DeferredDeletionQueue> queue, Deleter deleter = {});
    void operator()(T* object) const noexcept;
    void wait() const noexcept;

  private:
    std::shared_ptr<DeferredDeletionQueue> queue_;
    Deleter deleter_;
  };

  template<typename T>
  void ActiveObjectDeleter<T>::operator()(T* object) const noexcept
  {
    // Also finish clients deactivated explicitly but not yet waited for.
    object->deactivate_object();
    object->wait_object();
    delete object;
  }

  template<typename T, typename Deleter>
  DeferredDeleter<T, Deleter>::DeferredDeleter(
    std::shared_ptr<DeferredDeletionQueue> queue, Deleter deleter)
    : queue_(std::move(queue)), deleter_(std::move(deleter))
  {}

  template<typename T, typename Deleter>
  void DeferredDeleter<T, Deleter>::operator()(T* object) const noexcept
  {
    queue_->enqueue([object, deleter = deleter_] { deleter(object); });
  }

  template<typename T, typename Deleter>
  void DeferredDeleter<T, Deleter>::wait() const noexcept
  {
    queue_->wait();
  }

  template<typename T, typename Deleter = std::default_delete<T>, typename... Args>
  std::shared_ptr<T> make_shared_with_deleter(Deleter deleter, Args&&... args)
  {
    return std::shared_ptr<T>(new T(std::forward<Args>(args)...), std::move(deleter));
  }
}
