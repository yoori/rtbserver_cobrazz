#include "OrderedAsyncTaskWindow.hpp"

#include <algorithm>
#include <condition_variable>
#include <exception>
#include <mutex>
#include <optional>
#include <vector>

namespace AdServer::Commons
{
  struct OrderedAsyncTaskWindow::State
  {
    State(
      std::size_t first_sequence,
      std::size_t window_size,
      unsigned int wake_percent)
      : completed(window_size),
        committed(first_sequence),
        started(first_sequence),
        wake_threshold(std::max<std::size_t>(1, window_size * wake_percent / 100))
    {}

    std::mutex lock;
    std::condition_variable cond;
    std::optional<std::exception_ptr> exception;
    std::vector<unsigned char> completed;
    std::size_t committed;
    std::size_t started;
    std::size_t in_flight = 0;
    std::size_t completed_since_wait = 0;
    const std::size_t wake_threshold;
    bool failed = false;
  };

  OrderedAsyncTaskWindow::OrderedAsyncTaskWindow(
    std::size_t first_sequence,
    std::size_t window_size,
    unsigned int wake_percent)
    : window_size_(window_size ? window_size : 1),
      state_(std::make_shared<State>(first_sequence, window_size_, wake_percent))
  {}

  OrderedAsyncTaskWindow::~OrderedAsyncTaskWindow()
  {
    std::unique_lock<std::mutex> lock(state_->lock);
    state_->cond.wait(lock, [this] { return state_->in_flight == 0; });
  }

  void
  OrderedAsyncTaskWindow::start(
    std::size_t sequence,
    StartableAwaitable<void> operation)
  {
    {
      std::lock_guard<std::mutex> guard(state_->lock);
      state_->started = std::max(state_->started, sequence + 1);
      ++state_->in_flight;
    }

    operation.start_detached(
      [state = state_, sequence](std::optional<std::exception_ptr> exception) mutable noexcept
      {
        const bool has_exception = exception.has_value();
        std::lock_guard<std::mutex> guard(state->lock);

        if (exception)
        {
          if (!state->exception)
          {
            state->exception = std::move(exception);
          }
          state->failed = true;
        }
        else
        {
          state->completed[sequence % state->completed.size()] = 1;
          while (state->completed[state->committed % state->completed.size()] != 0)
          {
            state->completed[state->committed % state->completed.size()] = 0;
            ++state->committed;
          }
        }

        ++state->completed_since_wait;
        --state->in_flight;
        if (has_exception || state->in_flight == 0 ||
          state->completed_since_wait == state->wake_threshold)
        {
          state->cond.notify_one();
        }
      });
  }

  bool
  OrderedAsyncTaskWindow::full() const
  {
    std::lock_guard<std::mutex> guard(state_->lock);
    return state_->started - state_->committed >= window_size_;
  }

  std::size_t
  OrderedAsyncTaskWindow::wait_progress()
  {
    std::unique_lock<std::mutex> lock(state_->lock);
    state_->cond.wait(
      lock,
      [this]
      {
        return state_->failed || state_->in_flight == 0 ||
          state_->completed_since_wait >= state_->wake_threshold;
      });

    if (state_->failed)
    {
      state_->cond.wait(lock, [this] { return state_->in_flight == 0; });
    }

    state_->completed_since_wait = 0;
    return state_->committed;
  }

  std::size_t
  OrderedAsyncTaskWindow::drain()
  {
    std::unique_lock<std::mutex> lock(state_->lock);
    state_->cond.wait(lock, [this] { return state_->in_flight == 0; });
    return state_->committed;
  }

  void
  OrderedAsyncTaskWindow::rethrow_exception()
  {
    std::optional<std::exception_ptr> exception;
    {
      std::lock_guard<std::mutex> guard(state_->lock);
      exception = std::move(state_->exception);
    }

    if (exception)
    {
      std::rethrow_exception(std::move(*exception));
    }
  }
}
