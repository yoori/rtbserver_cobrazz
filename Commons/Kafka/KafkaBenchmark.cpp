// STD
#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <unordered_map>
#include <thread>

// POSIX
#include <signal.h>

// THIS
#include <Generics/ActiveObject.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Commons/Kafka/KafkaProducer.hpp>
#include <Logger/ActiveObjectCallback.hpp>
#include <Logger/Logger.hpp>
#include <Logger/StreamLogger.hpp>

namespace Kafka = AdServer::Commons::Kafka;

namespace Aspect
{
const char* Benchmark = "BENCHMARK";
}

class ThreadGuard
{
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

  ~ThreadGuard()
  {
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

class Benchmark final
  : protected Kafka::ProducerObserver,
    protected Generics::CompositeActiveObject,
    public virtual ReferenceCounting::AtomicImpl
{
  using Time = std::chrono::time_point<std::chrono::high_resolution_clock>;
public:
  explicit Benchmark(
    const std::size_t number_threads,
    const std::size_t size_queue,
    const std::string& brokers,
    const std::string& topic_name,
    const std::size_t duration_print,
    const std::size_t initial_number_request,
    const std::string& key_message,
    const std::string& data_message)
    : duration_print_(duration_print),
      initial_number_request_(initial_number_request),
      key_message_(key_message),
      data_message_(data_message)
  {
    id_hash_.reserve(10000000);

    logger_ = Logging::Logger_var(
      new Logging::OStream::Logger(
        Logging::OStream::Config(std::cout)));

    std::stringstream stream;
    stream << "\n\n"
           << "Benchmark parameters: \n"
           << "Number threads = " << number_threads << "\n"
           << "Size queue = " << size_queue << "\n"
           << "Brokers = " << brokers << "\n"
           << "Topic name = " << topic_name << "\n"
           << "Duration print = " << duration_print << "\n"
           << "Initial number request = " << initial_number_request << "\n"
           << "Key message = " << key_message << "\n"
           << "Data message = " << data_message << "\n";
    logger_->info(stream.str(), Aspect::Benchmark);

    Logging::Logger_var logger_stub(
      new Logging::OStream::Logger(
        Logging::OStream::Config(
          std::cerr,
          Logging::Logger::ERROR)));

    Generics::ActiveObjectCallback_var callback(
      new Logging::ActiveObjectCallbackImpl(
        logger_stub));

    kafka_producer_ = Kafka::Producer_var(
      new Kafka::Producer(
        logger_stub,
        callback,
        number_threads,
        size_queue,
        brokers.c_str(),
        topic_name.c_str(),
        this));

    add_child_object(kafka_producer_);
  }

  ~Benchmark() override = default;

  void run()
  {
    try
    {
      Generics::SimpleActiveObject::activate_object();

      logger_->info(std::string("Start benchmark"), Aspect::Benchmark);
      is_running_.store(true, std::memory_order_relaxed);

      for (std::size_t i = 1; i <= initial_number_request_; ++i)
      {
        const auto id_message = kafka_producer_->push_data(key_message_, data_message_);
        if (id_message)
        {
          const auto time = std::chrono::high_resolution_clock::now();
          std::lock_guard lock(mutex_id_hash_);
          id_hash_.try_emplace(*id_message, time);
        }
        else
        {
          logger_->error(
            std::string("Initial push data is failed"),
            Aspect::Benchmark);
        }
      }

      ThreadGuard thread_guard([this] () {
        std::uint64_t count_success_send_request_old = 0;
        std::uint64_t count_failed_send_request_old = 0;
        std::uint64_t count_success_response_old = 0;
        std::uint64_t count_failed_response_old = 0;

        while(is_running_.load(std::memory_order_relaxed))
        {
          try
          {
            const auto time_start = std::chrono::high_resolution_clock::now();
            std::this_thread::sleep_for(std::chrono::seconds(duration_print_));
            const auto time_end = std::chrono::high_resolution_clock::now();

            const auto elapsed_time =
              std::chrono::duration<double, std::milli>(time_end - time_start).count();

            const std::uint64_t count_success_send_request_new =
              count_success_send_request_.load(std::memory_order_relaxed);
            const std::uint64_t count_failed_send_request_new =
              count_failed_send_request_.load(std::memory_order_relaxed);
            const std::uint64_t count_success_response_new =
              count_success_response_.load(std::memory_order_relaxed);
            const std::uint64_t count_failed_response_new =
              count_failed_response_.load(std::memory_order_relaxed);

            std::size_t max_duration = 0;
            std::uint64_t total_duration = 0;
            std::uint64_t count_duration = 0;
            {
              std::lock_guard lock(mutex_duration_);
              max_duration = max_duration_;
              total_duration = total_duration_;
              count_duration = count_duration_;
              max_duration_ = 0;
              total_duration_ = 0;
              count_duration_ = 0 ;
            }

            std::stringstream stream;
            stream << "\n"
                   << "Success send resuest [rq/s] = "
                   << (count_success_send_request_new - count_success_send_request_old) * 1000 / elapsed_time
                   << "\n"
                   << "Failed send request [rq/s] = "
                   << (count_failed_send_request_new - count_failed_send_request_old) * 1000 / elapsed_time
                   << "\n"
                   << "Success response [rs/s] = "
                   << (count_success_response_new - count_success_response_old) * 1000 / elapsed_time
                   << "\n"
                   << "Failed response [rs/s] = "
                   << (count_failed_response_new - count_failed_response_old) * 1000 / elapsed_time
                   << "\n"
                   << "Average request duration [ms] = "
                   << total_duration / (count_duration != 0 ? count_duration : 1)
                   << "\n"
                   << "Max request duration [ms] = "
                   << max_duration
                   << "\n";
            logger_->info(stream.str(), Aspect::Benchmark);

            count_success_send_request_old = count_success_send_request_new;
            count_failed_send_request_old = count_failed_send_request_new;
            count_success_response_old = count_success_response_new;
            count_failed_response_old = count_failed_response_new;
          }
          catch (const eh::Exception& exc)
          {
            std::stringstream stream;
            stream << __PRETTY_FUNCTION__
                   << " Reason: "
                   << exc.what();
            logger_->error(stream.str(), Aspect::Benchmark);

            count_success_send_request_old =
              count_success_send_request_.load(std::memory_order_relaxed);
            count_failed_send_request_old =
              count_failed_send_request_.load(std::memory_order_relaxed);
            count_success_response_old =
              count_success_response_.load(std::memory_order_relaxed);
            count_failed_response_old =
              count_failed_response_.load(std::memory_order_relaxed);
          }
        }
      });

      sigset_t mask;
      sigemptyset(&mask);
      sigaddset(&mask, SIGINT);
      sigaddset(&mask, SIGQUIT);
      sigaddset(&mask, SIGTERM);
      sigprocmask(SIG_BLOCK, &mask, nullptr);

      int signo = 0;
      if (sigwait(&mask, &signo) != 0)
      {
        std::stringstream stream;
        stream << "Sigwait is failed";
        logger_->critical(stream.str(), Aspect::Benchmark);
      }
      else
      {
        std::stringstream stream;
        stream << "Signal=";
        switch (signo) {
          case SIGINT:
            stream << "SIGINT";
            break;
          case SIGQUIT:
            stream << "SIGQUIT";
            break;
          case SIGTERM:
            stream << "SIGTERM";
            break;
          default:
            stream << "Unexpected signal";
        }
        stream << " interrupted benchmark";
        logger_->info(stream.str(), Aspect::Benchmark);
      }

      stop();

      logger_->info(
        std::string("Benchmark is stoped"),
        Aspect::Benchmark);
    }
    catch (const eh::Exception& exc)
    {
      stop();
      std::stringstream stream;
      stream << "Benchmark is failed. Reason: "
             << exc.what();
      logger_->critical(stream.str(), Aspect::Benchmark);
    }
  }

private:
  void stop() noexcept
  {
    try
    {
      Generics::CompositeActiveObject::deactivate_object();
      Generics::CompositeActiveObject::wait_object();
    }
    catch (...)
    {
    }

    is_running_.store(false, std::memory_order_relaxed);
  }

  std::optional<std::size_t> get_duration(const Kafka::IdMessage id_message) const
  {
    const auto current_time = std::chrono::high_resolution_clock::now();

    std::unique_lock lock(mutex_id_hash_);
    const auto it = id_hash_.find(id_message);
    if (it == id_hash_.end())
    {
      lock.unlock();
      std::stringstream stream;
      stream << __PRETTY_FUNCTION__
             << " : Reason: "
             << "Not existing id_message="
             << id_message;
      logger_->error(stream.str(), Aspect::Benchmark);
      return {};
    }
    else
    {
      return std::chrono::duration<double, std::milli>(current_time - it->second).count();
    }
  }

  void update_time(const Kafka::IdMessage id_message)
  {
    const auto current_time = std::chrono::high_resolution_clock::now();

    std::lock_guard lock(mutex_id_hash_);
    const auto it = id_hash_.find(id_message);
    if (it == id_hash_.end())
    {
      std::stringstream stream;
      stream << __PRETTY_FUNCTION__
             << " : Reason: "
             << "Logic error. Not existing id_message="
             << id_message;
      logger_->critical(stream.str(), Aspect::Benchmark);
      id_hash_.try_emplace(id_message, current_time);
    }
    else
    {
      it->second = current_time;
    }
  }

  void remove_message(const Kafka::IdMessage id_message)
  {
    std::lock_guard lock(mutex_id_hash_);
    id_hash_.erase(id_message);
  }

  void do_request()
  {
    const auto id_message = kafka_producer_->push_data(key_message_, data_message_);
    if (id_message)
    {
      const auto current_time = std::chrono::high_resolution_clock::now();
      std::lock_guard lock(mutex_id_hash_);
      const auto result = id_hash_.try_emplace(*id_message, current_time);
      if (!result.second)
      {
        std::stringstream stream;
        stream << __PRETTY_FUNCTION__
               << " : Reason: "
               << "Logic error. Existing id_message="
               << *id_message;
        logger_->critical(stream.str(), Aspect::Benchmark);
        result.first->second = current_time;
      }
    }
  }

  void on_event(
    const EventType event,
    const Kafka::IdMessage id_message,
    const std::string_view /*key*/,
    const std::string_view /*data*/) noexcept override
  {
    try
    {
      std::size_t duration = 0;
      switch (event)
      {
        case EventType::SEND_ERROR:
        {
          count_failed_send_request_.fetch_add(1, std::memory_order_relaxed);
          remove_message(id_message);
          do_request();
          break;
        }
        case EventType::SEND_SUCCESS:
        {
          count_success_send_request_.fetch_add(1, std::memory_order_relaxed);
          break;
        }
        case EventType::RECEIVE_ERROR:
        {
          count_failed_response_.fetch_add(1, std::memory_order_relaxed);
          const auto d = get_duration(id_message);
          if (d)
            duration = *d;
          update_time(id_message);
          break;
        }
        case EventType::RECEIVE_SUCCESS:
        {
          count_success_response_.fetch_add(1, std::memory_order_relaxed);
          const auto d = get_duration(id_message);
          if (d)
            duration = *d;
          remove_message(id_message);
          do_request();
          break;
        }
      };

      if (duration)
      {
        std::lock_guard lock(mutex_duration_);
        max_duration_ = std::max(max_duration_, duration);
        total_duration_ += duration;
        count_duration_ += 1;
      }
    }
    catch (const eh::Exception& exc)
    {
      std::stringstream stream;
      stream << __PRETTY_FUNCTION__
             << " Reason: "
             << exc.what();
      logger_->critical(stream.str(), Aspect::Benchmark);
    }
  }

private:
  const std::size_t duration_print_;

  const std::size_t initial_number_request_;

  const std::string key_message_;

  const std::string data_message_;

  Logging::Logger_var logger_;

  std::atomic<std::uint64_t> count_success_send_request_{0};

  std::atomic<std::uint64_t> count_failed_send_request_{0};

  std::atomic<std::uint64_t> count_success_response_{0};

  std::atomic<std::uint64_t> count_failed_response_{0};

  std::unordered_map<Kafka::IdMessage, Time> id_hash_;

  mutable std::mutex mutex_id_hash_;

  Kafka::Producer_var kafka_producer_;

  std::size_t max_duration_ = 0;

  std::uint64_t total_duration_ = 0;

  std::uint64_t count_duration_ = 0;

  std::mutex mutex_duration_;

  std::atomic<bool> is_running_{false};
};

using Benchmark_var = ReferenceCounting::SmartPtr<Benchmark>;

int main(int /*argc*/, char** /*argv*/)
{
  try
  {
    const std::size_t number_threads = 5;
    const std::size_t size_queue = 500000;
    const std::string brokers = "192.168.3.53:9092";
    const std::string topic_name = "test";
    const std::size_t duration_print = 10;
    const std::size_t initial_number_request = 50000;
    const std::string key_message = "key_message";
    const std::string& data_message = "data_message";

    Benchmark_var benchmark(
      new Benchmark(
        number_threads,
        size_queue,
        brokers,
        topic_name,
        duration_print,
        initial_number_request,
        key_message,
        data_message));
    benchmark->run();
  }
  catch (const eh::Exception& exc)
  {
    std::cerr << "Fatal error: "
              << exc.what();
  }
}