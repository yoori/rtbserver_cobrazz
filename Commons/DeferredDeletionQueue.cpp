#include <Commons/DeferredDeletionQueue.hpp>

#include <condition_variable>
#include <mutex>
#include <stdexcept>
#include <vector>

#include <Commons/ThreadName.hpp>

namespace AdServer::Commons
{
  struct DeferredDeletionQueue::State
  {
    std::mutex lock;
    std::condition_variable condition;
    std::vector<std::function<void()>> tasks;
    std::size_t pending = 0;
    std::size_t registrations = 0;
    bool registrations_closed = false;
    bool stopping = false;
  };

  DeferredDeletionQueue::Registration::Registration(std::shared_ptr<State> state)
    : state_(std::move(state))
  {}

  DeferredDeletionQueue::Registration::~Registration() noexcept
  {
    complete();
  }

  void DeferredDeletionQueue::Registration::complete() noexcept
  {
    {
      std::lock_guard<std::mutex> lock(state_->lock);
      if (!pending_)
      {
        return;
      }

      pending_ = false;
      --state_->registrations;
    }

    state_->condition.notify_all();
  }

  DeferredDeletionQueue::DeferredDeletionQueue()
    : state_(std::make_shared<State>()), worker_([state = state_] { run_(state); })
  {}

  DeferredDeletionQueue::~DeferredDeletionQueue() noexcept
  {
    {
      std::lock_guard<std::mutex> lock(state_->lock);
      state_->stopping = true;
    }

    state_->condition.notify_all();
    if (worker_.get_id() == std::this_thread::get_id())
    {
      // A deleted object's last owner may also own this queue. State outlives the worker.
      worker_.detach();
    }
    else
    {
      worker_.join();
    }
  }

  void DeferredDeletionQueue::enqueue(std::function<void()> task)
  {
    {
      std::lock_guard<std::mutex> lock(state_->lock);
      state_->tasks.emplace_back(std::move(task));
      ++state_->pending;
    }

    state_->condition.notify_all();
  }

  std::shared_ptr<DeferredDeletionQueue::Registration> DeferredDeletionQueue::register_deletion()
  {
    auto registration = std::shared_ptr<Registration>(new Registration(state_));
    {
      std::lock_guard<std::mutex> lock(state_->lock);
      if (state_->registrations_closed)
      {
        throw std::logic_error("deletion queue is draining");
      }

      ++state_->registrations;
      registration->pending_ = true;
    }

    return registration;
  }

  void DeferredDeletionQueue::wait() noexcept
  {
    std::unique_lock<std::mutex> lock(state_->lock);
    state_->condition.wait(lock, [this] { return state_->pending == 0; });
  }

  void DeferredDeletionQueue::drain() noexcept
  {
    std::unique_lock<std::mutex> lock(state_->lock);
    state_->registrations_closed = true;
    state_->condition.wait(lock, [this]
      {
        return state_->registrations == 0 && state_->pending == 0;
      });
  }

  void DeferredDeletionQueue::run_(const std::shared_ptr<State>& state) noexcept
  {
    set_current_thread_name("deferred-delete");
    std::vector<std::function<void()>> tasks;
    for (;;)
    {
      {
        std::unique_lock<std::mutex> lock(state->lock);
        state->condition.wait(lock, [&] { return state->stopping || !state->tasks.empty(); });
        if (state->tasks.empty())
        {
          return;
        }

        tasks.swap(state->tasks);
      }

      const auto count = tasks.size();
      for (auto& task : tasks)
      {
        task();
      }

      // Captured owners may enqueue more deletions when the tasks are destroyed.
      tasks.clear();
      {
        std::lock_guard<std::mutex> lock(state->lock);
        state->pending -= count;
      }

      state->condition.notify_all();
    }
  }
}
