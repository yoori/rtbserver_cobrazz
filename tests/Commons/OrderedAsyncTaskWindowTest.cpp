#include <coroutine>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <utility>

#include <Commons/Coro/Utils.hpp>
#include <Commons/OrderedAsyncTaskWindow.hpp>

namespace
{
  class ManualEvent
  {
  private:
    struct State
    {
      std::mutex lock;
      bool ready = false;
      std::coroutine_handle<> waiter;
    };

  public:
    class Awaiter
    {
    public:
      explicit Awaiter(std::shared_ptr<State> state)
        : state_(std::move(state))
      {}

      bool
      await_ready() const noexcept
      {
        std::lock_guard<std::mutex> guard(state_->lock);
        return state_->ready;
      }

      bool
      await_suspend(std::coroutine_handle<> handle) noexcept
      {
        std::lock_guard<std::mutex> guard(state_->lock);
        if (state_->ready)
        {
          return false;
        }

        state_->waiter = handle;
        return true;
      }

      void
      await_resume() const noexcept
      {}

    private:
      std::shared_ptr<State> state_;
    };

    ManualEvent()
      : state_(std::make_shared<State>())
    {}

    Awaiter
    wait() const
    {
      return Awaiter(state_);
    }

    void
    set()
    {
      std::coroutine_handle<> waiter;
      {
        std::lock_guard<std::mutex> guard(state_->lock);
        state_->ready = true;
        waiter = std::exchange(state_->waiter, {});
      }

      if (waiter)
      {
        AdServer::Commons::resume_coroutine(waiter);
      }
    }

  private:
    std::shared_ptr<State> state_;
  };

  AdServer::Commons::StartableAwaitable<void>
  operation(std::shared_ptr<ManualEvent> event, bool fail = false)
  {
    co_await event->wait();
    if (fail)
    {
      throw std::runtime_error("operation failed");
    }
  }

  bool
  check_equal(std::size_t actual, std::size_t expected, const char* message)
  {
    if (actual == expected)
    {
      return true;
    }

    std::cerr << message << ": expected=" << expected << ", actual=" << actual << std::endl;
    return false;
  }
}

int
main()
{
  {
    AdServer::Commons::OrderedAsyncTaskWindow window(10, 4, 25);
    auto event0 = std::make_shared<ManualEvent>();
    auto event1 = std::make_shared<ManualEvent>();
    auto event2 = std::make_shared<ManualEvent>();
    auto event3 = std::make_shared<ManualEvent>();

    window.start(10, operation(event0));
    window.start(11, operation(event1));
    window.start(12, operation(event2));
    window.start(13, operation(event3));

    if (!window.full())
    {
      std::cerr << "window must be full" << std::endl;
      return 1;
    }

    event1->set();
    if (!check_equal(window.wait_progress(), 10, "out-of-order completion was committed"))
    {
      return 1;
    }

    event0->set();
    if (!check_equal(window.wait_progress(), 12, "contiguous prefix was not committed"))
    {
      return 1;
    }

    event3->set();
    if (!check_equal(window.wait_progress(), 12, "gap in completion sequence was ignored"))
    {
      return 1;
    }

    event2->set();
    if (!check_equal(window.drain(), 14, "final contiguous prefix was not committed"))
    {
      return 1;
    }
  }

  {
    AdServer::Commons::OrderedAsyncTaskWindow window(0, 2, 50);
    auto failed_event = std::make_shared<ManualEvent>();
    auto successful_event = std::make_shared<ManualEvent>();

    window.start(0, operation(failed_event, true));
    window.start(1, operation(successful_event));
    successful_event->set();
    failed_event->set();

    if (!check_equal(window.wait_progress(), 0, "failed operation was committed"))
    {
      return 1;
    }

    try
    {
      window.rethrow_exception();
      std::cerr << "operation exception was not rethrown" << std::endl;
      return 1;
    }
    catch (const std::runtime_error&)
    {}
  }

  return 0;
}
