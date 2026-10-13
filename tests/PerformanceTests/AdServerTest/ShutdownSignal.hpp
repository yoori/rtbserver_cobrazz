#pragma once

#include <atomic>
#include <cerrno>
#include <csignal>
#include <system_error>

class ShutdownSignal
{
public:
  ShutdownSignal();
  ~ShutdownSignal() noexcept;

  ShutdownSignal(const ShutdownSignal&) = delete;
  ShutdownSignal& operator=(const ShutdownSignal&) = delete;

  static bool received() noexcept;

private:
  static void handle_(int signal) noexcept;

  static inline std::atomic<bool> received_{false};
  struct sigaction previous_int_{};
  struct sigaction previous_term_{};
};

inline ShutdownSignal::ShutdownSignal()
{
  static_assert(std::atomic<bool>::is_always_lock_free);
  received_.store(false, std::memory_order_relaxed);
  struct sigaction action{};
  action.sa_handler = handle_;
  sigemptyset(&action.sa_mask);
  action.sa_flags = SA_RESTART;
  if (sigaction(SIGINT, &action, &previous_int_) != 0)
  {
    throw std::system_error(errno, std::generic_category(), "sigaction(SIGINT)");
  }

  if (sigaction(SIGTERM, &action, &previous_term_) != 0)
  {
    const int error = errno;
    sigaction(SIGINT, &previous_int_, nullptr);
    throw std::system_error(error, std::generic_category(), "sigaction(SIGTERM)");
  }
}

inline ShutdownSignal::~ShutdownSignal() noexcept
{
  sigaction(SIGTERM, &previous_term_, nullptr);
  sigaction(SIGINT, &previous_int_, nullptr);
}

inline bool ShutdownSignal::received() noexcept
{
  return received_.load(std::memory_order_relaxed);
}

inline void ShutdownSignal::handle_(int) noexcept
{
  // Only a lock-free flag is touched in signal context; cleanup runs in the worker.
  received_.store(true, std::memory_order_relaxed);
}
