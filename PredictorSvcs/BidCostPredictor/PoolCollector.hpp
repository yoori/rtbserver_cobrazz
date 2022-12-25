#ifndef RTBSERVER_COBRAZZ_POOLCOLLECTOR_HPP
#define RTBSERVER_COBRAZZ_POOLCOLLECTOR_HPP

// STD
#include <list>
#include <mutex>

namespace PredictorSvcs
{
namespace BidCostPredictor
{

template<class Collector, std::size_t init_size = 500000>
class PoolCollector final
{
public:
  PoolCollector() = default;

  PoolCollector(const PoolCollector&) = delete;
  PoolCollector(PoolCollector&&) = delete;
  PoolCollector& operator=(const PoolCollector&) = delete;
  PoolCollector& operator=(PoolCollector&&) = delete;

  Collector getCollector()
  {
    std::unique_lock lock(mutex_);
    if (collectors_.empty())
    {
      lock.unlock();

      Collector collector;
      collector.prepare_adding(init_size);
      return collector;
    }
    else
    {
      Collector collector(std::move(collectors_.front()));
      collectors_.pop_front();
      return collector;
    }
  }

  void addCollector(Collector&& collector)
  {
    collector.clear();
    std::lock_guard lock(mutex_);
    collectors_.emplace_back(std::move(collector));
  }

private:
  std::mutex mutex_;

  std::list<Collector> collectors_;
};

} // namespace BidCostPredictor
} // namespace PredictorSvcs

#endif //RTBSERVER_COBRAZZ_POOLCOLLECTOR_HPP
