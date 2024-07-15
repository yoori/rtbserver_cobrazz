/* $Id: KafkaProducer.cpp 185978 2020-07-04 00:12:49Z jurij_kuznecov $
* @file KafkaProducer.cpp
* @author Artem V. Nikitin (artem_nikitin@ocslab.com)
* Kafka producer 
*/

#include "KafkaProducer.hpp"
#include <Commons/UserInfoManip.hpp>

namespace
{
  const Generics::Time RECONNECT_TIMEOUT(5l); // 5 seconds
  const Generics::Time STATS_TIMEOUT(10l);    // 10 seconds
  const Generics::Time WAIT_MSG_TIMEOUT(1l);  // 1 second
}

namespace AdServer
{
  namespace Commons
  {
    namespace Kafka
    {
      // class StatCounter
      
      StatCounter::StatCounter() :
        prev_(0),
        current_(0)
      { }
        
          
      StatCounter&
      StatCounter::operator+=(int val)
      {
        current_ += val;
        return *this;
      }
      
      StatCounter::operator int() const
      {
        return current_;
      }

      std::ostream&
      operator<<(
        std::ostream& os,
        const StatCounter& cnt)
      {
        int current = cnt.current_;
        int prev = cnt.prev_;
        cnt.prev_ = current;
        os << current - prev << "/" << current;
        return os;
      }
     
      // Producer::EventCb
      Producer::EventCallback::EventCallback(
        ProducerHandler* handler)
        noexcept :
        handler_(handler)
      {}
          
      void
      Producer::EventCallback::event_cb(
        RdKafka::Event& event)
      {
        static const char* FUN = "Kafka::Producer::EventCallback::event_cb";
        switch(event.type())
        {
        case RdKafka::Event::EVENT_ERROR:
            // ERR__ALL_BROKERS_DOWN, ...
            // do reconnect after RECONNECT_TIMEOUT seconds
        {
          handler_->process_error(
            FUN, event.str().c_str(),
            event.err() == RdKafka::ERR__ALL_BROKERS_DOWN);
        }
        
        break;

        // info events
        case RdKafka::Event::EVENT_STATS:
        case RdKafka::Event::EVENT_LOG:
        case RdKafka::Event::EVENT_THROTTLE:
        default:
            break;
        }
      }

      // Producer::DeliveryReportCallback
      Producer::DeliveryReportCallback::DeliveryReportCallback(
        ProducerHandler* handler)
        noexcept :
        handler_(handler)
      { }

      void
      Producer::DeliveryReportCallback::dr_cb(RdKafka::Message& message)
      {
        if (message.err() != RdKafka::ERR_NO_ERROR)
        {
          handler_->resend_message(
            message.topic_name(),
            message.key(),
            message.payload(),
            message.len());
        }
        else
        {
          handler_->notify_ok();
        }
      }

      // Producer::PartitionCallback
      int32_t
      Producer::PartitionCallback::partitioner_cb(
        __attribute__((unused)) const RdKafka::Topic* topic,
        const std::string* key,
        int32_t partition_cnt,
        void*)
      {
        if (key)
        {
          int32_t partition_number =
            AdServer::Commons::external_id_distribution_hash(*key) %
              partition_cnt;
          return partition_number;
          /*
          if (topic && topic->partition_available(partition_number))
          {
            return partition_number;
          }
          */
        }

        return RdKafka::Topic::PARTITION_UA;
      }

      // ProducerHandler

      Producer::ProducerHandler::ProducerHandler(Producer* owner)
        /*throw(ProducerError)*/
        : owner_(owner),
          event_callback_(this),
          delivery_callback_(this),
          producer_conf_(RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL)),
          topic_conf_(RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC))
      {
        static const char* FUN = "Kafka::Producer::ProducerHandler::ProducerHandler";
        
        try
        {
          std::string error_str;

          producer_conf_->set("metadata.broker.list", owner_->brokers_.c_str(), error_str);
          producer_conf_->set("event_cb", &event_callback_, error_str);
          producer_conf_->set("dr_cb", &delivery_callback_, error_str);
          //producer_conf_->set("dr_msg_cb", &delivery_callback_, error_str);
          producer_conf_->set("socket.keepalive.enable", "true", error_str);

          producer_.reset(RdKafka::Producer::create(producer_conf_.get(), error_str));

          if (!producer_)
          {
            Stream::Error ostr;
            ostr << FUN << " connection error: '" << error_str << "'";
            throw ProducerError(ostr);
          }

          topic_conf_->set(
            "partitioner_cb",
            &partition_callback_,
            error_str);

          topic_.reset(
            RdKafka::Topic::create(
              producer_.get(),
              owner_->topic_name_.c_str(),
              topic_conf_.get(),
              error_str));

          if(!topic_)
          {
            Stream::Error ostr;
            ostr << "Failed to create topic: '" << error_str << "'";
            throw ProducerError(ostr);
          }
        }
        catch (const eh::Exception& ex)
        {
          Stream::Error ostr;
          ostr << FUN << " connection error: '" << ex.what() << "'";
          throw ProducerError(ostr);
        }
      }

      void
      Producer::ProducerHandler::notify_ok()
      {
        owner_->stats_->consider_connect();
      }
            
      void
      Producer::ProducerHandler::process_error(
        const char* context,
        const char* error,
        bool disconnected)
      {
        if (disconnected)
        {
          owner_->stats_->consider_disconnect();
        }
        owner_->process_error_(context, error);
      }
      
      int
      Producer::ProducerHandler::poll(
        int timeout_ms)
      {
        return producer_->poll(timeout_ms);
      }
      
      int
      Producer::ProducerHandler::flush(
        int timeout_ms)
      {
        return producer_->flush(timeout_ms);
      }

      void
      Producer::ProducerHandler::produce(
        const ProducerPair& msg) /*throw(ProducerError)*/
      {
        RdKafka::ErrorCode code = producer_->produce(
          topic_.get(), // topic
          RdKafka::Topic::PARTITION_UA, // partition
          RdKafka::Producer::RK_MSG_COPY, // msgflags - copy payload
          const_cast<char*>(msg.second.data()), // payload
          msg.second.size(), // len
          &msg.first, // key
          NULL // msg_opaque
          );

        if (code != RdKafka::ERR_NO_ERROR)
        {
          Stream::Error ostr;
          ostr << "Produce message failed: '" <<
            RdKafka::err2str(code) << "'";
          throw ProducerError(ostr);
        }
      }

      void
      Producer::ProducerHandler::resend_message(
        const std::string& topic_name,
        const std::string* key,
        void* payload,
        size_t len)
      {
        if (owner_->topic_name_.compare(topic_name) == 0)
        {
          owner_->push_data(
            *key,
            std::string(static_cast<const char*>(payload), len));
        }
      }

      // class StatsObject
      Producer::StatsObject::StatsObject(
        Producer* owner,
        Generics::ActiveObjectCallback* callback) :
        Commons::DelegateActiveObject(callback, 1),
        disconnected(false),
        owner_(owner) 
      { }

      void
      Producer::StatsObject::work_() noexcept
      {
        static const char* FUN = "Kafka::Producer::StatsObject::work_";

        while(active())
        {
          {
            SyncPolicy::ReadGuard lock(lock_);
            condition_.timed_wait(lock_, &STATS_TIMEOUT, true);
            
            Stream::Error ostr;
            ostr << FUN << ": '" << owner_->topic_name_ <<
              "' producer stat: " << std::endl <<
              "  status: " << (disconnected?
                "Disconnected": "Connected") << std::endl <<
              "  sent: " << sent << std::endl <<
              "  reconnects: " << reconnect <<  std::endl <<
              "  overflows: " << error_overflow <<  std::endl <<
              "  exceptions: " << error_exception << std::endl <<
              "  message queue size: "  << owner_->messages_.size() << std::endl;
            owner_->logger_->log(ostr.str(), Logging::Logger::INFO);
          }
        }
      }

      void Producer::StatsObject::consider_connect()
      {
        if (disconnected)
        {
          SyncPolicy::WriteGuard lock(lock_);
          if (disconnected)
          {
            disconnected = false;
            reconnect += 1;
          }
        }
      }

      void Producer::StatsObject::consider_disconnect()
      {
        if (!disconnected)
        {
          disconnected = true;
        }
      }
      
      void
      Producer::StatsObject::terminate_() noexcept
      {
        SyncPolicy::WriteGuard lock(lock_);
        condition_.broadcast();
      }
      
      // class Producer
      
      Producer::Producer(
        Logging::Logger* logger,
        Generics::ActiveObjectCallback* callback,
        const KafkaTopicConfig& config)
        : Commons::DelegateActiveObject(callback, config.threads()),
          logger_(ReferenceCounting::add_ref(logger)),
          brokers_(config.brokers()),
          topic_name_(config.topic()),
          messages_(config.message_queue_size()),
          stats_(new StatsObject(this, callback))
      {}
      
      Producer::Producer(
        Logging::Logger* logger,
        Generics::ActiveObjectCallback* callback,
        unsigned long threads_number,
        unsigned long queue_size,
        const char* brokers,
        const char* topic_name)
        : Commons::DelegateActiveObject(callback, threads_number),
          logger_(ReferenceCounting::add_ref(logger)),
          brokers_(brokers),
          topic_name_(topic_name),
          messages_(queue_size),
          stats_(new StatsObject(this, callback))
      {}

      void
      Producer::activate_object()
        /*throw(Exception, eh::Exception)*/
      {
        Commons::DelegateActiveObject::activate_object();
        stats_->activate_object();
      }

      void
      Producer::work_() noexcept
      {
        static const char* FUN = "Kafka::Producer::work_";

        while(active())
        {
          try
          {
            // (Re)create handler
            ProducerHandler handler(this);

            // Produce event
            while(active())
            {
              produce_(handler);
            }

            // wait flush before destroy,
            // otherwise possible crash on topic call in partitioner_cb
            handler.flush();
          }
          catch (const ProducerQueue::AlreadyClosed&)
          {
            break;
          }
          catch (const ProducerError& ex)
          {
            process_error_(FUN, ex.what());
          }

          {
            SyncPolicy::WriteGuard lock(reconnect_lock_);
            reconnect_cond_.timed_wait(
              reconnect_lock_,
              &RECONNECT_TIMEOUT,
              true);
          }
          
          stats_->reconnect += 1;
        }
      }

      void
      Producer::process_error_(
        const char* context,
        const char* error)
      {
        if (!stats_->disconnected)
        {
          stats_->error_exception += 1;
          Stream::Error ostr;
          ostr << context << ": '" << topic_name_ <<
            "' producer error: " << error;
          logger_->log(ostr.str(), Logging::Logger::ERROR);
        }
      }
        
      void
      Producer::produce_(ProducerHandler& handler)
      {
        std::list<ProducerPair> msg_array;
        messages_.pop_all(msg_array, WAIT_MSG_TIMEOUT);

        if (!msg_array.empty())
        {
          unsigned long sent_bytes = 0;
          for(auto msg_it = msg_array.begin(); msg_it != msg_array.end(); ++msg_it)
          {
            handler.produce(*msg_it);
            sent_bytes += msg_it->second.size();
          }

          stats_->sent_bytes += sent_bytes;
          stats_->sent += msg_array.size();

          msg_array.clear();
        }

        handler.poll();
      }
      
      void
      Producer::terminate_() noexcept
      {
        {
          SyncPolicy::WriteGuard lock(reconnect_lock_);
          reconnect_cond_.broadcast();
        }

        stats_->deactivate_object();
        stats_->wait_object();
        messages_.close();
      }

      void Producer::push_data(
        const std::string& key,
        const std::string& data) noexcept
      {
        if (!messages_.try_emplace(key, data))
        {
          stats_->error_overflow += 1;
        }
      }

      unsigned long
      Producer::errors() const
      {
        return stats_->error_overflow +
          stats_->error_exception;
      }
      
      unsigned long
      Producer::sent() const
      {
        return stats_->sent;
      }
      
      unsigned long
      Producer::sent_bytes() const
      {
        return stats_->sent_bytes;
      }
    }
  }
}


