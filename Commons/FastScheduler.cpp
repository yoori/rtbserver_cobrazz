#include "FastScheduler.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

#include <Commons/ThreadName.hpp>

namespace AdServer::Commons
{
  class FastScheduler::Impl final
  {
  private:
    class Worker final
    {
    private:
      struct TaskCompare
      {
        bool operator()(const Task& lhs, const Task& rhs) const noexcept
        {
          if (lhs.deadline != rhs.deadline)
          {
            return lhs.deadline < rhs.deadline;
          }
          return std::addressof(lhs) < std::addressof(rhs);
        }
      };

      using TaskSet = boost::intrusive::multiset<
        Task,
        boost::intrusive::member_hook<
          Task,
          boost::intrusive::set_member_hook<>,
          &Task::timer_hook>,
        boost::intrusive::compare<TaskCompare>,
        boost::intrusive::constant_time_size<false>>;

    public:
      Worker()
        : thread_([this]() noexcept { work_(); })
      {}

      ~Worker() noexcept
      {
        {
          std::lock_guard<std::mutex> lock(wait_lock_);
          stopping_.store(true, std::memory_order_release);
        }
        wait_condition_.notify_one();
        if (thread_.joinable())
        {
          thread_.join();
        }
      }

      void schedule(
        Task& task,
        const Generics::Time& deadline,
        std::shared_ptr<void> owner,
        const Callback callback) noexcept
      {
        task.deadline = quantize_(deadline);
        task.callback = callback;
        task.owner = std::move(owner);

        const auto deadline_ticks = task.deadline.microseconds();
        Task* pending = pending_.load(std::memory_order_acquire);
        for (;;)
        {
          if (pending == stopped_task_())
          {
            release_task_(task);
            return;
          }

          task.pending_next = pending;
          if (pending_.compare_exchange_weak(
            pending,
            &task,
            std::memory_order_release,
            std::memory_order_acquire))
          {
            break;
          }
        }

        auto wakeup_ticks = wakeup_ticks_.load(std::memory_order_acquire);
        while ((wakeup_ticks == 0 || deadline_ticks < wakeup_ticks) &&
          !wakeup_ticks_.compare_exchange_weak(
            wakeup_ticks,
            deadline_ticks,
            std::memory_order_acq_rel,
            std::memory_order_acquire))
        {}

        if (wakeup_ticks == 0 || deadline_ticks < wakeup_ticks)
        {
          wait_condition_.notify_one();
        }
      }

    private:
      static Generics::Time quantize_(const Generics::Time& deadline) noexcept
      {
        constexpr long long quantum_us = 1000;
        const auto deadline_us = deadline.microseconds();
        const auto quantized_us =
          (deadline_us + quantum_us - 1) / quantum_us * quantum_us;
        return Generics::Time(
          quantized_us / Generics::Time::USEC_MAX,
          quantized_us % Generics::Time::USEC_MAX);
      }

      static void release_task_(Task& task) noexcept
      {
        auto owner = std::move(task.owner);
        task.pending_next = nullptr;
        task.callback = nullptr;
      }

      Task* stopped_task_() noexcept
      {
        return &stopped_task_storage_;
      }

      void drain_pending_() noexcept
      {
        Task* task = pending_.exchange(nullptr, std::memory_order_acquire);
        while (task)
        {
          Task* next = task->pending_next;
          task->pending_next = nullptr;
          tasks_.insert(*task);
          task = next;
        }
      }

      void process_ready_(const Generics::Time& now) noexcept
      {
        while (!tasks_.empty() && tasks_.begin()->deadline <= now)
        {
          Task& task = *tasks_.begin();
          tasks_.erase(tasks_.begin());

          std::optional<Generics::Time> next_deadline;
          if (task.callback && task.owner)
          {
            next_deadline = task.callback(task.owner.get());
          }

          if (next_deadline)
          {
            task.deadline = quantize_(*next_deadline);
            tasks_.insert(task);
          }
          else
          {
            release_task_(task);
          }
        }
      }

      void clear_() noexcept
      {
        Task* task = pending_.exchange(
          stopped_task_(),
          std::memory_order_acq_rel);

        while (task && task != stopped_task_())
        {
          Task* next = task->pending_next;
          release_task_(*task);
          task = next;
        }

        while (!tasks_.empty())
        {
          Task& scheduled_task = *tasks_.begin();
          tasks_.erase(tasks_.begin());
          release_task_(scheduled_task);
        }
      }

      void work_() noexcept
      {
        set_current_thread_name("fast-scheduler");

        while (!stopping_.load(std::memory_order_acquire))
        {
          wakeup_ticks_.store(0, std::memory_order_release);
          drain_pending_();

          if (stopping_.load(std::memory_order_acquire))
          {
            break;
          }

          if (tasks_.empty())
          {
            std::unique_lock<std::mutex> lock(wait_lock_);
            wait_condition_.wait(
              lock,
              [this]() noexcept
              {
                return stopping_.load(std::memory_order_acquire) ||
                  pending_.load(std::memory_order_acquire) != nullptr;
              });
            continue;
          }

          const auto deadline = tasks_.begin()->deadline;
          const auto now = Generics::Time::get_time_of_day();
          if (deadline <= now)
          {
            process_ready_(now);
            continue;
          }

          wakeup_ticks_.store(
            deadline.microseconds(),
            std::memory_order_release);
          if (pending_.load(std::memory_order_acquire))
          {
            continue;
          }

          std::unique_lock<std::mutex> lock(wait_lock_);
          wait_condition_.wait_for(
            lock,
            std::chrono::microseconds((deadline - now).microseconds()),
            [this]() noexcept
            {
              return stopping_.load(std::memory_order_acquire) ||
                pending_.load(std::memory_order_acquire) != nullptr;
            });
        }

        clear_();
      }

      std::atomic<Task*> pending_{nullptr};
      std::atomic<long long> wakeup_ticks_{0};
      std::atomic<bool> stopping_{false};
      Task stopped_task_storage_;
      TaskSet tasks_;
      std::mutex wait_lock_;
      std::condition_variable wait_condition_;
      std::thread thread_;
    };

  public:
    explicit Impl(const std::size_t threads)
    {
      if (threads == 0)
      {
        throw std::invalid_argument("FastScheduler: threads must be positive");
      }

      workers_.reserve(threads);
      for (std::size_t i = 0; i < threads; ++i)
      {
        workers_.emplace_back(std::make_unique<Worker>());
      }
    }

    void schedule(
      Task& task,
      const Generics::Time& deadline,
      std::shared_ptr<void> owner,
      const Callback callback) noexcept
    {
      const auto worker_index = next_worker_.fetch_add(
        1,
        std::memory_order_relaxed) % workers_.size();
      workers_[worker_index]->schedule(
        task,
        deadline,
        std::move(owner),
        callback);
    }

  private:
    std::vector<std::unique_ptr<Worker>> workers_;
    std::atomic<std::size_t> next_worker_{0};
  };

  FastScheduler::FastScheduler(const std::size_t threads)
    : impl_(std::make_unique<Impl>(threads))
  {}

  FastScheduler::~FastScheduler() noexcept = default;

  void
  FastScheduler::schedule(
    Task& task,
    const Generics::Time& deadline,
    std::shared_ptr<void> owner,
    const Callback callback) noexcept
  {
    impl_->schedule(task, deadline, std::move(owner), callback);
  }
}
