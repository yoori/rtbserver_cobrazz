#include <array>
#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>

#include <Commons/Coro/CallbackAwaiter.hpp>
#include <Commons/Coro/MonoStartableAwaitable.hpp>

namespace
{
  using namespace AdServer::Commons;
  using Arena = Generics::MonoAllocatorArena;
  using Task = MonoStartableAwaitable<void>;

  static_assert(sizeof(Task) == sizeof(StartableAwaitable<void>));
  static_assert(sizeof(Task::promise_type) == sizeof(StartableAwaitable<void>::promise_type));

  struct Lifetime
  {
    unsigned destroyed = 0;
    std::uintptr_t parameter_address = 0;
    std::uintptr_t nested_address = 0;
    std::thread::id destroyed_on;
  };

  struct Tracked
  {
    explicit Tracked(Lifetime& state) noexcept : state(&state)
    {}

    Tracked(Tracked&& other) noexcept : state(std::exchange(other.state, nullptr))
    {
      state->parameter_address = reinterpret_cast<std::uintptr_t>(this);
    }

    ~Tracked()
    {
      if (state)
      {
        ++state->destroyed;
        state->destroyed_on = std::this_thread::get_id();
      }
    }

    Lifetime* state;
  };

  StartableAwaitable<void> nested_heap_task(Lifetime* state)
  {
    state->nested_address = reinterpret_cast<std::uintptr_t>(&state);
    co_return;
  }

  Task suspended_task(Arena&, Tracked tracked, std::function<void()>& resume, bool fail)
  {
    co_await nested_heap_task(tracked.state);
    co_await CallbackAwaiter<>([&resume](auto callback) { resume = std::move(callback); });
    if (fail)
    {
      throw std::runtime_error("arena coroutine failure");
    }
  }

  struct Owner
  {
    MonoStartableAwaitable<int> result(Arena&, Tracked tracked) const
    {
      assert(tracked.state->destroyed == 0);
      co_return 42;
    }
  };

  MonoStartableAwaitable<int> two_arenas(Arena&, Arena&, Tracked tracked)
  {
    assert(tracked.state->destroyed == 0);
    co_return 42;
  }

  bool contains(const auto& buffer, std::uintptr_t address)
  {
    const auto begin = reinterpret_cast<std::uintptr_t>(buffer.data());
    return address >= begin && address < begin + buffer.size();
  }

  void test_unstarted_and_result()
  {
    alignas(std::max_align_t) std::array<unsigned char, 4096> buffer;
    Arena arena(buffer.data(), buffer.size());
    Lifetime unstarted;
    std::function<void()> resume;
    {
      auto operation = suspended_task(arena, Tracked(unstarted), resume, false);
      assert(contains(buffer, unstarted.parameter_address));
      assert(unstarted.destroyed == 0);
    }
    assert(unstarted.destroyed == 1 && !resume);

    Lifetime completed;
    const Owner owner;
    auto operation = owner.result(arena, Tracked(completed));
    assert(contains(buffer, completed.parameter_address));
    assert(sync_wait(std::move(operation)) == 42);
    assert(completed.destroyed == 1);

    Arena other_arena;
    Lifetime two_arguments;
    auto two_arena_operation = two_arenas(arena, other_arena, Tracked(two_arguments));
    assert(contains(buffer, two_arguments.parameter_address));
    assert(sync_wait(std::move(two_arena_operation)) == 42);
    assert(two_arguments.destroyed == 1);
  }

  void test_resume_and_destroy_on_worker(bool fail, bool detached)
  {
    alignas(std::max_align_t) std::array<unsigned char, 4096> buffer;
    Arena arena(buffer.data(), buffer.size());
    Lifetime state;
    std::function<void()> resume;
    bool completed = false;
    auto completion = [&](auto error)
    {
      assert(error.has_value() == fail);
      completed = true;
    };
    auto operation = suspended_task(arena, Tracked(state), resume, fail);
    assert(contains(buffer, state.parameter_address));
    if (detached)
    {
      operation.start_detached(completion);
    }
    else
    {
      operation.start(completion);
    }
    assert(resume && !completed && state.destroyed == 0);
    assert(!contains(buffer, state.nested_address));
    std::thread worker([operation = std::move(operation), &resume, &completed, fail, detached]()
      mutable
    {
      resume();
      assert(completed);
      if (!detached)
      {
        assert(operation.await_ready());
        bool caught = false;
        try
        {
          operation.await_resume();
        }
        catch (const std::runtime_error&)
        {
          caught = true;
        }
        assert(caught == fail);
      }
    });
    const auto worker_id = worker.get_id();
    worker.join();
    assert(state.destroyed == 1 && state.destroyed_on == worker_id);
  }

  struct ThrowOnFrameMove
  {
    ThrowOnFrameMove() = default;
    ThrowOnFrameMove(const ThrowOnFrameMove&) = default;
    ThrowOnFrameMove(ThrowOnFrameMove&&)
    {
      throw std::runtime_error("frame parameter move failure");
    }
  };

  Task failed_construction(Arena&, ThrowOnFrameMove)
  {
    co_return;
  }

  void test_failed_construction()
  {
    alignas(std::max_align_t) std::array<unsigned char, 4096> buffer;
    Arena arena(buffer.data(), buffer.size());
    const ThrowOnFrameMove argument;
    bool caught = false;
    try
    {
      auto operation = failed_construction(arena, argument);
    }
    catch (const std::runtime_error&)
    {
      caught = true;
    }
    assert(caught);

    Lifetime state;
    assert(sync_wait(Owner().result(arena, Tracked(state))) == 42);
    assert(state.destroyed == 1);
  }

  void test_parallel_completion()
  {
    constexpr unsigned count = 128;
    Arena arena;
    std::array<Lifetime, count> states;
    std::array<std::function<void()>, count> callbacks;
    std::vector<Task> operations;
    operations.reserve(count);
    for (unsigned i = 0; i < count; ++i)
    {
      operations.emplace_back(suspended_task(arena, Tracked(states[i]), callbacks[i], false));
    }
    std::vector<std::thread> workers;
    for (unsigned worker = 0; worker < 4; ++worker)
    {
      workers.emplace_back([&, worker]
      {
        for (unsigned i = worker; i < count; i += 4)
        {
          auto operation = std::move(operations[i]);
          bool completed = false;
          operation.start([&](auto error)
          {
            assert(!error);
            completed = true;
          });
          callbacks[i]();
          assert(completed);
        }
      });
    }
    for (auto& worker : workers)
    {
      worker.join();
    }
    for (const auto& state : states)
    {
      assert(state.destroyed == 1);
    }
  }
}

int main()
{
  test_unstarted_and_result();
  for (const bool fail : {false, true})
  {
    test_resume_and_destroy_on_worker(fail, false);
    test_resume_and_destroy_on_worker(fail, true);
  }
  test_failed_construction();
  test_parallel_completion();
  std::cout << "MonoStartableAwaitableTest passed" << std::endl;
}
