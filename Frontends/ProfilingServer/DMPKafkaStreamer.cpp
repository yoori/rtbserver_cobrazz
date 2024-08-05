/* $Id: DMPKafkaStreamer.cpp 185930 2019-08-23 09:10:46Z jurij_kuznecov $
* @file DMPKafkaStreamer.cpp
* @author Artem V. Nikitin (artem_nikitin@ocslab.com)
* DMP Kafka streamer
*/
#include <iomanip>
#include <numeric> 
#include "DMPKafkaStreamer.hpp"
#include <LogCommons/CsvUtils.hpp>


namespace
{
  const char* PRODUCER_NAMES[] =
  {
    "UID generator",
    "click stream"
  };

  using namespace AdServer::Profiling;
  using namespace AdServer::Commons;

  void dmp_profile_to_stream_helper(
    const Generics::Time& now,
    std::ostream& ostr)
  {
    ostr << now.tv_sec << "." <<  std::setfill('0') <<
      std::setw(6) << now.tv_usec << ",";
  }

  template<typename OStream>
  void dmp_profile_to_stream_helper(
    const Generics::Time& now,
    OStream& ostr)
  {
    ostr << now.tv_sec << "." << 
      Stream::MemoryStream::width_out(now.tv_usec, 6, '0') << ",";
  }

  template<typename OStream>
  void dmp_profile_to_stream(
    const DMPProfilingInfoReader& dmp_profiling_info,
    const ExternalUserIdArray& bind_user_ids,
    const Generics::Time& now,
    OStream& ostr)
  {    
    dmp_profile_to_stream_helper(now, ostr);

    // Skip beeline user_id
    auto bind_user_it = bind_user_ids.begin();
    if ( !bind_user_ids.empty() )
    {
      bind_user_it++;
    }

    // dmp profile without keywords    
    ostr <<
      dmp_profiling_info.version() << "," <<
      dmp_profiling_info.time() << "," <<
      dmp_profiling_info.source() << "," <<
      dmp_profiling_info.external_user_id() << ",";
    AdServer::LogProcessing::write_list_as_csv(
      ostr,
      ExternalUserIdArray(
        bind_user_it,
        bind_user_ids.end()));
    ostr << ",";
    AdServer::LogProcessing::write_string_as_csv(
      ostr, dmp_profiling_info.url());
    ostr << "," <<
      dmp_profiling_info.latitude() << "," <<
      dmp_profiling_info.longitude() << ",";
  }


  typedef unsigned long (
    Kafka::Producer::* GetStatFunc)() const;
  
  template <GetStatFunc Member>
  struct Sum
  {
    typedef DMPKafkaStreamer::ProducerTable::value_type Value;
    
    unsigned long operator()(unsigned long left, const Value& right)
    {
      return left + (right.second->*Member)();
    }
  };

}

namespace AdServer
{
  namespace Profiling
  {
    // DMPKafkaStreamer

    const DMPKafkaStreamer::KafkaProducerDescriptor
      DMPKafkaStreamer::PRODUCER_DESCRIPTORS[] =
    {
      {
        PT_UID_GENERATOR,
        &KafkaStorageConfig::UidGeneratorTopic
      },
      {
        PT_CLICK_STREAM,
        &KafkaStorageConfig::ClickStreamTopic
      },
      {
        PT_GEO,
        &KafkaStorageConfig::GeoTopic
      }
    };

    void DMPKafkaStreamer::register_producers(
      Logging::Logger* logger,
      Generics::ActiveObjectCallback* callback,
      Generics::CompositeActiveObject* owner,
      const KafkaStorageConfig& config)
    {
      size_t producers_size =
        sizeof(PRODUCER_DESCRIPTORS) / sizeof(*PRODUCER_DESCRIPTORS);
      
      for (size_t i = 0; i < producers_size; ++i)
      {
        const KafkaProducerDescriptor& descriptor =
          PRODUCER_DESCRIPTORS[i];

        const KafkaTopicConfig_optional& topic_config =
          (config.*(descriptor.topic_func))();

        if (topic_config.present())
        {
          Commons::Kafka::Producer* producer =
            new Commons::Kafka::Producer(
              logger,
              callback,
              topic_config.get());
          
          producers_.insert(
            std::make_pair(
              descriptor.producer_type,
              producer));
          
          owner->add_child_object(producer);
        }
      }
    }

    void DMPKafkaStreamer::write_dmp_profiling_info(
      const DMPProfilingInfoReader& dmp_profiling_info,
      const AdServer::Commons::ExternalUserIdArray& bind_user_ids,
      const Generics::Time& now)
    {
      static const char* FUN = "DMPKafkaStreamer::write_dmp_profiling_info";

      ProducerType producer_type = dmp_profiling_info.keywords()[0] ?
        PT_UID_GENERATOR:
          dmp_profiling_info.latitude() || dmp_profiling_info.longitude() ?
            PT_GEO: PT_CLICK_STREAM;

      auto producer = producers_.find(producer_type);

      if (producer != producers_.end())
      {
        Stream::MemoryStream::OutputMemoryStream<char> ostr(1024);
        
        dmp_profile_to_stream(
          dmp_profiling_info,
          bind_user_ids,
          now,
          ostr);

        if (dmp_profiling_info.keywords()[0])
        {
          AdServer::LogProcessing::write_not_empty_string_as_csv(
            ostr, dmp_profiling_info.keywords());
        }
        
        producer->second->push_data(
          dmp_profiling_info.external_user_id(),
          ostr.str().str());
      }
      else
      {
        Stream::Error ostr;
        ostr << FUN << "'" << PRODUCER_NAMES[producer_type] << "' producer not found";
        throw ProducerNotFound(ostr);
      }
    }

    unsigned long
    DMPKafkaStreamer::sent_messages() const
    {
      return std::accumulate(
        producers_.begin(),
        producers_.end(),
        0ul,
        Sum<&Commons::Kafka::Producer::sent>());
    }
    
    unsigned long
    DMPKafkaStreamer::sent_bytes() const
    {
      return std::accumulate(
        producers_.begin(),
        producers_.end(),
        0ul,
        Sum<&Commons::Kafka::Producer::sent_bytes>());
    }
    
    unsigned long
    DMPKafkaStreamer::errors() const
    {
      return std::accumulate(
        producers_.begin(),
        producers_.end(),
        0ul,
        Sum<&Commons::Kafka::Producer::errors>());
    }
  }
}
    
