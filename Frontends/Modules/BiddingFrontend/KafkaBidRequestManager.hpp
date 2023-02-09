#ifndef ADSERVER_KAFKAMANAGER_HPP
#define ADSERVER_KAFKAMANAGER_HPP

// STD
#include <list>
#include <memory>
#include <shared_mutex>
#include <unordered_map>

// THIS
#include <Commons/Kafka/KafkaProducer.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Logger/Logger.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/Interface.hpp>
#include <xsd/AdServerCommons/AdServerCommons.hpp>

namespace AdServer
{
namespace Bidding
{

class KafkaBidRequestManager final:
  public Generics::CompositeActiveObject,
  public virtual ReferenceCounting::AtomicImpl
{
  struct SamplingCounter
  {
    SamplingCounter()
    : counter(0)
    {}
    std::atomic<int> counter;
  };
  using SamplingCounter_var = std::shared_ptr<SamplingCounter>;
  using Source = std::string;
  using SamplingBySource = std::unordered_map<Source, SamplingCounter_var>;

public:
  using KafkaTopicConfig = ::xsd::AdServer::Configuration::KafkaTopic;

public:
  explicit KafkaBidRequestManager(
    Logging::Logger* logger,
    Generics::ActiveObjectCallback* callback,
    const KafkaTopicConfig& config,
    const std::size_t sample_div,
    const std::size_t sample_part);

  ~KafkaBidRequestManager() override = default;

  void push_data(
    const String::SubString& source,
    const String::SubString& key,
    const String::SubString& data);

private:
  Commons::Kafka::Producer_var kafka_;

  Logging::Logger_var logger_;

  const std::size_t sample_div_;

  const std::size_t sample_part_;

  SamplingBySource sampling_by_source_;

  std::shared_mutex shared_mutex_;
};

using KafkaBidRequestManager_var = ReferenceCounting::SmartPtr<KafkaBidRequestManager>;

} // namespace Bidding
} // namespace AdServer

#endif //ADSERVER_KAFKAMANAGER_HPP
