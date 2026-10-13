#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>

#include <pthread.h>

#include <tests/PerformanceTests/AdServerTest/ShutdownSignal.hpp>

namespace
{
  void check(bool condition, const char* message)
  {
    if (!condition)
    {
      throw std::runtime_error(message);
    }
  }

  void test_signal(int signal)
  {
    struct sigaction previous{};
    check(sigaction(signal, nullptr, &previous) == 0, "Read previous signal handler");
    {
      ShutdownSignal handler;
      check(!ShutdownSignal::received(), "Signal flag must start clear");
      check(raise(signal) == 0, "Deliver shutdown signal");
      check(ShutdownSignal::received(), "Shutdown signal must set the flag");
      check(raise(signal) == 0, "Deliver repeated shutdown signal");
      check(ShutdownSignal::received(), "Repeated signal must preserve the flag");
    }
    struct sigaction restored{};
    check(sigaction(signal, nullptr, &restored) == 0, "Read restored signal handler");
    check(previous.sa_handler == restored.sa_handler, "Previous handler must be restored");
  }

  void test_worker_signal()
  {
    ShutdownSignal handler;
    std::atomic<bool> finish{false};
    std::thread worker([&finish]()
    {
      while (!finish.load())
      {
        std::this_thread::yield();
      }
    });
    const int error = pthread_kill(worker.native_handle(), SIGTERM);
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (!ShutdownSignal::received() && std::chrono::steady_clock::now() < deadline)
    {
      std::this_thread::yield();
    }
    finish.store(true);
    worker.join();
    check(error == 0, "Deliver signal to a worker");
    check(ShutdownSignal::received(), "Worker signal must be visible to the main thread");
  }
}

int main()
{
  try
  {
    test_signal(SIGINT);
    test_signal(SIGTERM);
    test_worker_signal();
    std::cout << "PASS: shutdown signals, repeated delivery, restoration and worker delivery\n";
    return 0;
  }
  catch (const std::exception& error)
  {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
