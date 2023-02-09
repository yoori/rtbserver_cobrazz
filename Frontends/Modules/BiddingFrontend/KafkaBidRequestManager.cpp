// THIS
#include "KafkaBidRequestManager.hpp"
#include <iostream>
namespace AdServer
{
namespace Bidding
{

KafkaBidRequestManager::KafkaBidRequestManager(
  Logging::Logger* logger,
  Generics::ActiveObjectCallback* callback,
  const KafkaTopicConfig& config,
  const std::size_t sample_div,
  const std::size_t sample_part)
  : kafka_(new Commons::Kafka::Producer(
     logger,
     callback,
     config)),
    logger_(ReferenceCounting::add_ref(logger)),
    sample_div_(sample_div),
    sample_part_(sample_part)
{
  add_child_object(kafka_);
}

void KafkaBidRequestManager::push_data(
  const String::SubString& source,
  const String::SubString& key,
  const String::SubString& data)
{
  const std::string source_id = source.str();
  SamplingCounter_var sampling_counter;
  {
    std::shared_lock lock(shared_mutex_);
    auto it = sampling_by_source_.find(source_id);
    if (it != sampling_by_source_.end())
    {
      sampling_counter = it->second;
    }
  }

  if (!sampling_counter)
  {
    std::unique_lock lock(shared_mutex_);
    sampling_counter =
      sampling_by_source_.emplace(
        source_id,
        std::make_shared<SamplingCounter>()).first->second;
  }

  const auto count =
    sampling_counter->counter.fetch_add(1, std::memory_order_relaxed);
  if(count % sample_div_ < sample_part_)
  {
    kafka_->push_data(key.str(), data.str());
  }
}

} // namespace Bidding
} // namespace AdServer
