#ifndef BIDCOSTPREDICTOR_POOLCOLLECTOR_HPP
#define BIDCOSTPREDICTOR_POOLCOLLECTOR_HPP

// STD
#include <list>
#include <mutex>

// UNIXCOMMONS
#include <Generics/Uncopyable.hpp>

namespace PredictorSvcs::BidCostPredictor
{

template<class Collector,
         std::size_t init_size = 500000>
class PoolCollector final : private Generics::Uncopyable
{
public:
  explicit PoolCollector() = default;

  ~PoolCollector() = default;

  Collector get_collector()
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

  void add_collector(Collector&& collector)
  {
    collector.clear();
    std::lock_guard lock(mutex_);
    collectors_.emplace_back(std::move(collector));
  }

private:
  std::mutex mutex_;

  std::list<Collector> collectors_;
};

} // namespace PredictorSvcs::BidCostPredictor

#endif //BIDCOSTPREDICTOR_POOLCOLLECTOR_HPP