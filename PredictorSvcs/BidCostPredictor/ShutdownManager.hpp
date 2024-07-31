#ifndef BIDCOSTPREDICTOR_SHUTDOWNMANAGER_HPP
#define BIDCOSTPREDICTOR_SHUTDOWNMANAGER_HPP

// STD
#include <condition_variable>
#include <memory>
#include <mutex>

// UNIXCOMMONS
#include <Generics/Uncopyable.hpp>

namespace PredictorSvcs::BidCostPredictor
{

class ShutdownManager final : private Generics::Uncopyable
{
public:
  explicit ShutdownManager() = default;

  ~ShutdownManager() = default;

  void stop() noexcept
  {
    {
      std::lock_guard lock(mutex_);
      is_stoped_ = true;
    }
    cv_.notify_all();
  }

  bool is_stoped() const noexcept
  {
    std::unique_lock lock(mutex_);
    return is_stoped_;
  }

  void wait() noexcept
  {
    std::unique_lock lock(mutex_);
    cv_.wait(lock, [this] () {
      return is_stoped_;
    });
  }

private:
  std::condition_variable cv_;

  mutable std::mutex mutex_;

  bool is_stoped_ = false;
};

} // namespace PredictorSvcs::BidCostPredictor

#endif //BIDCOSTPREDICTOR_SHUTDOWNMANAGER_HPP
