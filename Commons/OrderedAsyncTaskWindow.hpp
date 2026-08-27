#pragma once

#include <cstddef>
#include <memory>

#include <Commons/Coro/StartableAwaitable.hpp>

namespace AdServer::Commons
{
  class OrderedAsyncTaskWindow final
  {
  public:
    OrderedAsyncTaskWindow(
      std::size_t first_sequence,
      std::size_t window_size,
      unsigned int wake_percent = 10);

    OrderedAsyncTaskWindow(const OrderedAsyncTaskWindow&) = delete;
    OrderedAsyncTaskWindow& operator=(const OrderedAsyncTaskWindow&) = delete;

    ~OrderedAsyncTaskWindow();

    void
    start(std::size_t sequence, StartableAwaitable<void> operation);

    bool
    full() const;

    std::size_t
    wait_progress();

    std::size_t
    drain();

    void
    rethrow_exception();

  private:
    struct State;

    const std::size_t window_size_;
    std::shared_ptr<State> state_;
  };
}
