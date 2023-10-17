#ifndef BIDCOSTPREDICTOR_THREADGUARD_H
#define BIDCOSTPREDICTOR_THREADGUARD_H

// STD
#include <thread>

namespace PredictorSvcs
{
namespace BidCostPredictor
{

class ThreadGuard {
public:
  ThreadGuard(std::thread&& thread) noexcept
    : m_thread(std::move(thread))
  {
  }

  template<class Function, class... Args>
  ThreadGuard(Function&& function, Args&&... args)
    : m_thread(std::forward<Function>(function),
               std::forward<Args>(args)...)
  {
  }

  ~ThreadGuard() {
    try
    {
      if (m_thread.joinable())
        m_thread.join();
    }
    catch (...)
    {
    }
  }

  ThreadGuard(ThreadGuard&& threadGuard) = default;
  ThreadGuard& operator=(ThreadGuard&&) = default;
  ThreadGuard(const ThreadGuard&) = delete;
  ThreadGuard& operator=(const ThreadGuard&) = delete;

private:
  std::thread m_thread;
};

} // namespace BidCostPredictor
} // namespace PredictorSvcs

#endif //BIDCOSTPREDICTOR_THREADGUARD_H