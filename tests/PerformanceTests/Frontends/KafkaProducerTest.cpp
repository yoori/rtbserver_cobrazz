#include <iostream>
#include <sstream>
#include <iomanip>

#include <Generics/AppUtils.hpp>
#include <Logger/StreamLogger.hpp>
#include <Logger/ActiveObjectCallback.hpp>

#include <Commons/Kafka/KafkaProducer.hpp>

namespace
{
}

int
main(int argc, char** argv)
{
  Generics::AppUtils::StringOption opt_broker;
  Generics::AppUtils::StringOption opt_topic;
  Generics::AppUtils::Option<unsigned long> opt_threads(100);
  Generics::AppUtils::Option<unsigned long> opt_queue_size(100000);
  Generics::AppUtils::Option<unsigned long> opt_sample_size(100000);
  Generics::AppUtils::Option<unsigned long> opt_repeat(0);

  Generics::AppUtils::Args args(-1);

  args.add(
    Generics::AppUtils::equal_name("broker") ||
    Generics::AppUtils::short_name("b"),
    opt_broker);

  args.add(
    Generics::AppUtils::equal_name("topic"),
    opt_topic);

  args.add(
    Generics::AppUtils::equal_name("threads"),
    opt_threads);

  args.add(
    Generics::AppUtils::equal_name("queue-size"),
    opt_queue_size);

  args.add(
    Generics::AppUtils::equal_name("sample-size"),
    opt_sample_size);

  args.add(
    Generics::AppUtils::equal_name("repeat"),
    opt_repeat);

  args.parse(argc - 1, argv + 1);

  Logging::Logger_var logger = new Logging::OStream::Logger(
    Logging::OStream::Config(std::cerr, Logging::Logger::CRITICAL));

  Logging::ActiveObjectCallbackImpl_var callback(
    new Logging::ActiveObjectCallbackImpl(
      logger,
      "KafkaProducerTest::main()",
      "KafkaProducerTest",
      ""));

  std::cerr << "brokers: " << *opt_broker << ", topic: " << *opt_topic << std::endl;

  try
  {
    AdServer::Commons::Kafka::Producer_var producer(
      new AdServer::Commons::Kafka::Producer(
        logger,
        callback,
        *opt_threads,
        *opt_queue_size,
        opt_broker->c_str(),
        opt_topic->c_str()));

    producer->activate_object();

    unsigned long i = 0;
    
    while(*opt_repeat? i < *opt_repeat: true)
    {
      Generics::Time start(Generics::Time::get_time_of_day());
      
      for(unsigned long j = 0; j < *opt_sample_size; ++j)
      {
        producer->push_data("key", "data");
      }

      sleep(1);

      Generics::Time duration =
        Generics::Time::get_time_of_day() - start;

      std::cerr << "#" << i << " messages sent (duration: " <<
        duration.tv_sec << "." <<  std::setfill('0') <<
        std::setw(6) << duration.tv_usec << "): " <<
        producer->sent() << ", errors:" <<
        producer->errors() <<  std::endl;

      i++;
    }

    //sleep(1000);

    producer->deactivate_object();
    producer->wait_object();
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << ex.what() << std::endl;
  }

  return 0;
}
