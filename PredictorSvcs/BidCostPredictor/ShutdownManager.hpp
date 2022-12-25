#ifndef RTBSERVER_COBRAZZ_BIDCOSTPREDICTOR_SHUTDOWNMANAGER_HPP
#define RTBSERVER_COBRAZZ_BIDCOSTPREDICTOR_SHUTDOWNMANAGER_HPP

// STD
#include <condition_variable>
#include <memory>
#include <mutex>

// THIS
#include <Generics/Uncopyable.hpp>

namespace PredictorSvcs
{
namespace BidCostPredictor
{

class ShutdownManager final : private Generics::Uncopyable
{
public:
  ShutdownManager() = default;

  ~ShutdownManager() = default;

  void stop() noexcept
  {
    try
    {
      {
        std::lock_guard lock(mutex_);
        is_stoped_ = true;
      }
      cv_.notify_all();
    }
    catch (...)
    {}
  }

  bool isStoped() noexcept
  {
    std::unique_lock lock(mutex_);
    return is_stoped_;
  }

  void wait() noexcept
  {
    try
    {
      std::unique_lock lock(mutex_);
      cv_.wait(lock, [this]{return is_stoped_;});
    }
    catch (...)
    {}
  }

private:
  std::condition_variable cv_;

  std::mutex mutex_;

  bool is_stoped_ = false;
};

} // namespace BidCostPredictor
} // namespace PredictorSvcs

#endif //RTBSERVER_COBRAZZ_BIDCOSTPREDICTOR_SHUTDOWNMANAGER_HPP
