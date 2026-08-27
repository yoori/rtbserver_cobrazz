#include <atomic>
#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <random>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#include <optional>

#include <sys/resource.h>

#include <grpcpp/grpcpp.h>
#include <grpc/impl/channel_arg_names.h>

#include <Generics/AppUtils.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Generics/Time.hpp>

#include <Commons/BoostAsioContextRunActiveObject.hpp>
#include <Commons/GrpcAlgs.hpp>
#include <Commons/Grpc/GrpcClient.hpp>
#include <Commons/UserInfoManip.hpp>

#include <UserInfoSvcs/UserBindServer/UserBindServerGrpc.grpc.pb.h>
#include <UserInfoSvcs/UserBindClient/UserBindDistributedGrpcClient.hpp>

#include "UserBindServerGrpc.grpc-client.hpp"

namespace
{
  using BatchClient = AdServer::UserInfoSvcs::UserBindServerGrpcAsyncBatchingClient;

  class NullActiveObjectCallback final:
    public virtual Generics::ActiveObjectCallback,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    void
    report_error(Severity, const String::SubString&, const char* = nullptr) throw () override
    {}

  protected:
    ~NullActiveObjectCallback() throw () override = default;
  };

  enum class Mode
  {
    AsyncBatch,
    DistributedGrpc
  };

  std::optional<Mode>
  parse_mode(const std::string& value)
  {
    if (value == "async-batch")
    {
      return Mode::AsyncBatch;
    }

    if (value == "distributed-grpc")
    {
      return Mode::DistributedGrpc;
    }

    return std::nullopt;
  }

  std::vector<std::uint32_t>
  parse_response_time_steps(const std::string& value)
  {
    std::vector<std::uint32_t> result;
    if (value.empty())
    {
      return result;
    }

    std::size_t begin = 0;
    while (begin < value.size())
    {
      const auto end = value.find(',', begin);
      const auto token = value.substr(
        begin,
        end == std::string::npos ? std::string::npos : end - begin);
      if (token.empty())
      {
        throw std::invalid_argument("--response-time-steps-us contains an empty value");
      }

      std::size_t parsed = 0;
      const auto step = std::stoull(token, &parsed);
      if (parsed != token.size() || step == 0 || step > 0xffffffffULL)
      {
        throw std::invalid_argument("--response-time-steps-us values must be positive uint32");
      }

      if (!result.empty() && step <= result.back())
      {
        throw std::invalid_argument("--response-time-steps-us values must be strictly increasing");
      }
      result.push_back(static_cast<std::uint32_t>(step));

      if (end == std::string::npos)
      {
        break;
      }
      begin = end + 1;
    }

    return result;
  }

  std::string
  random_external_id(std::mt19937& gen)
  {
    static constexpr std::size_t id_size = 10;
    static constexpr char first = 'a';
    static constexpr char last = 'z';
    std::uniform_int_distribution<int> dist(first, last);

    std::string id;
    id.reserve(id_size);
    for (std::size_t i = 0; i < id_size; ++i)
    {
      id.push_back(static_cast<char>(dist(gen)));
    }
    return id;
  }

  struct CpuTimes
  {
    double user = 0.0;
    double sys = 0.0;
  };

  std::string
  format_stat_float(double value)
  {
    std::ostringstream out;
    out << std::fixed << std::setprecision(6) << value;
    auto result = out.str();
    const auto dot_pos = result.find('.');
    if (dot_pos != std::string::npos)
    {
      while (!result.empty() && result.back() == '0')
      {
        result.pop_back();
      }

      if (!result.empty() && result.back() == '.')
      {
        result.pop_back();
      }
    }
    return result;
  }

  double
  timeval_seconds(const timeval& value)
  {
    return static_cast<double>(value.tv_sec) +
      static_cast<double>(value.tv_usec) / 1000000.0;
  }

  CpuTimes
  process_cpu_times()
  {
    rusage usage{};
    getrusage(RUSAGE_SELF, &usage);
    return CpuTimes{
      timeval_seconds(usage.ru_utime),
      timeval_seconds(usage.ru_stime)
    };
  }

  void
  print_async_batch_diagnostics(std::ostream& out, const AdServer::Grpc::Stats& stats)
  {
    out <<
      ", input_items=" << stats.input_items <<
      ", completed_items=" << stats.completed_items <<
      ", completed_error_items=" << stats.completed_error_items <<
      ", outstanding_items=" <<
        (stats.input_items - stats.completed_items) <<
      ", queue_items=" << stats.queue_items <<
      ", pending_batches=" << stats.pending_batches <<
      ", pending_batch_items=" << stats.pending_batch_items <<
      ", inflight_items=" << stats.inflight_items <<
      ", stream_inflight_items=" << stats.stream_inflight_items <<
      ", active_streams=" << stats.active_streams <<
      ", available_streams=" << stats.available_streams <<
      ", connecting_streams=" << stats.connecting_streams <<
      ", draining_streams=" << stats.draining_streams <<
      ", deferred_streams=" << stats.deferred_streams;

    if (stats.last_error.has_value())
    {
      out <<
        ", last_error_time=" <<
          stats.last_error->time.get_gm_time().format("%F %T") <<
        ", last_error_endpoint=" << stats.last_error->endpoint <<
        ", last_error_code=" << stats.last_error->code <<
        ", last_error_message=" << stats.last_error->message <<
        ", last_error_source=" << stats.last_error->source;
    }
  }

  class LatencyTopPercentile final
  {
  public:
    explicit LatencyTopPercentile(std::uint64_t max_samples)
      : max_samples_(std::max<std::uint64_t>(1, max_samples))
    {}

    void add(std::uint64_t latency_us)
    {
      if (heap_size_.load(std::memory_order_relaxed) >= max_samples_ &&
        latency_us <= threshold_.load(std::memory_order_relaxed))
      {
        return;
      }

      std::lock_guard<std::mutex> lock(lock_);
      if (slowest_.size() < max_samples_)
      {
        slowest_.push(latency_us);
      }
      else if (latency_us > slowest_.top())
      {
        slowest_.pop();
        slowest_.push(latency_us);
      }

      heap_size_.store(slowest_.size(), std::memory_order_relaxed);
      if (!slowest_.empty())
      {
        threshold_.store(slowest_.top(), std::memory_order_relaxed);
      }
    }

    std::uint64_t p99(std::uint64_t count) const
    {
      if (count == 0)
      {
        return 0;
      }

      std::vector<std::uint64_t> values;
      {
        std::lock_guard<std::mutex> lock(lock_);
        values = slowest_.values();
      }

      if (values.empty())
      {
        return 0;
      }

      auto top_count = std::max<std::uint64_t>(1, (count + 99) / 100);
      top_count = std::min<std::uint64_t>(top_count, values.size());
      auto position = values.begin() + static_cast<std::ptrdiff_t>(top_count - 1);
      std::nth_element(values.begin(), position, values.end(), std::greater<std::uint64_t>());
      return *position;
    }

  private:
    using SlowestHeap = std::priority_queue<
      std::uint64_t,
      std::vector<std::uint64_t>,
      std::greater<std::uint64_t>>;

    struct SlowestHeapAccessor : SlowestHeap
    {
      const std::vector<std::uint64_t>& values() const noexcept
      {
        return this->c;
      }
    };

    const std::uint64_t max_samples_;
    mutable std::mutex lock_;
    SlowestHeapAccessor slowest_;
    std::atomic<std::uint64_t> heap_size_{0};
    std::atomic<std::uint64_t> threshold_{0};
  };

  class ErrorAccumulator final
  {
  public:
    using ErrorMap = std::map<std::string, std::uint64_t>;

    void add(const grpc::Status& status)
    {
      std::ostringstream key;
      key << '[' << static_cast<int>(status.error_code()) << "] " << status.error_message();

      std::lock_guard<std::mutex> lock(lock_);
      ++errors_[key.str()];
    }

    ErrorMap snapshot() const
    {
      std::lock_guard<std::mutex> lock(lock_);
      return errors_;
    }

    static void print_delta(std::ostream& out, const ErrorMap& current, const ErrorMap& previous)
    {
      print_(out, current, &previous);
    }

    static void print_total(std::ostream& out, const ErrorMap& current)
    {
      print_(out, current, nullptr);
    }

  private:
    static void print_(std::ostream& out, const ErrorMap& current, const ErrorMap* previous)
    {
      bool first = true;
      for (const auto& [message, count] : current)
      {
        std::uint64_t previous_count = 0;
        if (previous)
        {
          const auto it = previous->find(message);
          if (it != previous->end())
          {
            previous_count = it->second;
          }
        }

        if (count <= previous_count)
        {
          continue;
        }

        if (first)
        {
          out << "{";
          first = false;
        }
        else
        {
          out << ", ";
        }

        out << message << ": " << (count - previous_count);
      }

      if (first)
      {
        out << "{}";
      }
      else
      {
        out << "}";
      }
    }

  private:
    mutable std::mutex lock_;
    ErrorMap errors_;
  };
}

int
main(int argc, char** argv)
{
  try
  {
    using namespace Generics::AppUtils;

    StringOption opt_user_bind_grpc_endpoint("localhost:25728");
    StringOption opt_user_bind_controller_grpc_endpoint;
    Generics::AppUtils::Option<unsigned long> opt_count(10000000);
    Generics::AppUtils::Option<unsigned int> opt_threads(16);
    Generics::AppUtils::Option<unsigned int> opt_client_threads(4);
    Generics::AppUtils::Option<unsigned int> opt_max_streams(0);
    StringOption opt_mode("async-batch");
    Generics::AppUtils::Option<unsigned long> opt_max_inflight(0);
    Generics::AppUtils::Option<unsigned int> opt_error_on_inflight_reaching(0);
    Generics::AppUtils::Option<unsigned long> opt_max_batch_size(1024);
    Generics::AppUtils::Option<unsigned long> opt_max_batch_delay_us(3000);
    Generics::AppUtils::Option<unsigned long> opt_max_queue_wait_us(0);
    Generics::AppUtils::Option<unsigned long> opt_stream_start_timeout_us(0);
    Generics::AppUtils::Option<unsigned long> opt_hot_buckets_count(1);
    Generics::AppUtils::Option<unsigned int> opt_local_subchannel_pool(1);
    Generics::AppUtils::Option<unsigned int> opt_grpc_compression(1);
    Generics::AppUtils::Option<unsigned int> opt_print_errors(0);
    Generics::AppUtils::Option<unsigned int> opt_reconnect_per_request(0);
    Generics::AppUtils::Option<unsigned long> opt_rpc_timeout_ms(5000);
    Generics::AppUtils::Option<unsigned int> opt_delayed_percent(0);
    StringOption opt_response_time_steps_us;
    StringOption opt_user_id;

    Args args(-1);
    args.add(equal_name("grpc-endpoint") || short_name("g"), opt_user_bind_grpc_endpoint);
    args.add(
      equal_name("user-bind-controller-grpc-endpoint"),
      opt_user_bind_controller_grpc_endpoint);
    args.add(equal_name("count") || short_name("c"), opt_count);
    args.add(equal_name("threads") || short_name("t"), opt_threads);
    args.add(equal_name("client-threads"), opt_client_threads);
    args.add(equal_name("max-streams"), opt_max_streams);
    args.add(equal_name("mode"), opt_mode);
    args.add(equal_name("max-inflight"), opt_max_inflight);
    args.add(equal_name("error-on-inflight-reaching"), opt_error_on_inflight_reaching);
    args.add(equal_name("max-batch-size"), opt_max_batch_size);
    args.add(equal_name("max-batch-delay-us"), opt_max_batch_delay_us);
    args.add(equal_name("max-queue-wait-us"), opt_max_queue_wait_us);
    args.add(equal_name("stream-start-timeout-us"), opt_stream_start_timeout_us);
    args.add(equal_name("hot-buckets-count"), opt_hot_buckets_count);
    args.add(equal_name("local-subchannel-pool"), opt_local_subchannel_pool);
    args.add(equal_name("grpc-compression"), opt_grpc_compression);
    args.add(equal_name("print-errors"), opt_print_errors);
    args.add(equal_name("reconnect-per-request"), opt_reconnect_per_request);
    args.add(equal_name("rpc-timeout-ms"), opt_rpc_timeout_ms);
    args.add(equal_name("delayed-percent"), opt_delayed_percent);
    args.add(equal_name("response-time-steps-us"), opt_response_time_steps_us);
    args.add(equal_name("user-id"), opt_user_id);

    args.parse(argc - 1, argv + 1);

    if (*opt_threads == 0)
    {
      std::cerr << "--threads must be > 0" << std::endl;
      return 1;
    }

    if (*opt_client_threads == 0)
    {
      std::cerr << "--client-threads must be > 0" << std::endl;
      return 1;
    }

    const auto mode = parse_mode(*opt_mode);
    if (!mode.has_value())
    {
      std::cerr << "--mode must be one of: async-batch, distributed-grpc" << std::endl;
      return 1;
    }

    if (*mode == Mode::DistributedGrpc && opt_user_bind_controller_grpc_endpoint->empty())
    {
      std::cerr
        << "--user-bind-controller-grpc-endpoint is required for distributed-grpc mode"
        << std::endl;
      return 1;
    }

    if (*opt_max_batch_size == 0)
    {
      std::cerr << "--max-batch-size must be > 0" << std::endl;
      return 1;
    }

    if (*opt_hot_buckets_count == 0)
    {
      std::cerr << "--hot-buckets-count must be > 0" << std::endl;
      return 1;
    }

    if (*opt_delayed_percent > 100)
    {
      std::cerr << "--delayed-percent must be in range 0..100" << std::endl;
      return 1;
    }

    std::vector<std::uint32_t> response_time_steps_us;
    try
    {
      response_time_steps_us = parse_response_time_steps(*opt_response_time_steps_us);
    }
    catch (const std::exception& e)
    {
      std::cerr << e.what() << std::endl;
      return 1;
    }

    const bool reconnect_per_request = *opt_reconnect_per_request != 0;
    if (*opt_delayed_percent != 0 && response_time_steps_us.empty())
    {
      std::cerr << "--response-time-steps-us is required with "
        "--delayed-percent" << std::endl;
      return 1;
    }

    if (!response_time_steps_us.empty() && *mode != Mode::AsyncBatch)
    {
      std::cerr << "response time steps are supported only in async-batch mode" << std::endl;
      return 1;
    }

    if (!response_time_steps_us.empty() && reconnect_per_request)
    {
      std::cerr << "response time steps require persistent async-batch clients" << std::endl;
      return 1;
    }

    const auto client_threads = *opt_client_threads;
    const auto max_streams = *opt_max_streams == 0 ? client_threads : *opt_max_streams;
    const auto max_inflight =
      *opt_max_inflight > 0 ? std::make_optional<std::size_t>(*opt_max_inflight) : std::nullopt;
    const auto make_time_option =
      [](unsigned long microseconds) -> std::optional<Generics::Time>
      {
        if (microseconds == 0)
        {
          return std::nullopt;
        }

        return Generics::Time(
          microseconds / Generics::Time::USEC_MAX,
          microseconds % Generics::Time::USEC_MAX);
      };

    std::atomic<std::uint64_t> sent_count{0};
    std::atomic<std::uint64_t> done_count{0};
    std::atomic<std::uint64_t> error_count{0};
    std::atomic<std::uint64_t> latency_count{0};
    std::atomic<std::uint64_t> latency_sum_us{0};
    std::atomic<std::uint64_t> latency_max_us{0};
    LatencyTopPercentile latency_p99((*opt_count + 99) / 100);
    ErrorAccumulator errors;

    Generics::ActiveObjectSet_var async_batch_active_objects = new Generics::ActiveObjectSet();
    std::shared_ptr<BatchClient> batch_client;
    std::shared_ptr<BatchClient> delayed_batch_client;
    std::shared_ptr<AdServer::UserInfoSvcs::UserBindDistributedGrpcClient>
      distributed_client;
    AdServer::UserInfoSvcs::UserBindServerGrpcAsyncClient* client = nullptr;
    if (!reconnect_per_request && *mode == Mode::AsyncBatch)
    {
      AdServer::Grpc::BatchingOptions options;
      options.channels_number = max_streams;
      options.max_batch_size = *opt_max_batch_size;
      options.max_inflight = max_inflight;
      options.error_on_inflight_reaching = *opt_error_on_inflight_reaching != 0;
      options.workers_number = client_threads;
      options.hot_buckets_count = *opt_hot_buckets_count;
      options.max_batch_delay = make_time_option(*opt_max_batch_delay_us);
      options.max_queue_wait = make_time_option(*opt_max_queue_wait_us);
      options.stream_start_timeout = make_time_option(*opt_stream_start_timeout_us);
      options.enable_grpc_compression = *opt_grpc_compression != 0;
      options.use_local_subchannel_pool = *opt_local_subchannel_pool != 0;
      if (!response_time_steps_us.empty() && *opt_delayed_percent == 100)
      {
        options.response_time_steps_us = response_time_steps_us;
      }
      std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor =
        std::make_shared<AdServer::Grpc::GrpcExecutor>(client_threads);
      auto coalesce_runner =
        std::make_shared<AdServer::Commons::BoostAsioContextRunActiveObject>(
          Generics::ActiveObjectCallback_var(new NullActiveObjectCallback()),
          std::make_shared<boost::asio::io_service>(),
          client_threads);
      batch_client = std::make_shared<BatchClient>(
          *opt_user_bind_grpc_endpoint,
          grpc_executor,
          coalesce_runner,
          options);
      async_batch_active_objects->add_child_object(grpc_executor);
      async_batch_active_objects->add_child_object(coalesce_runner);
      async_batch_active_objects->add_child_object(batch_client);
      client = batch_client.get();

      if (!response_time_steps_us.empty() && *opt_delayed_percent != 100)
      {
        options.response_time_steps_us = response_time_steps_us;
        delayed_batch_client = std::make_shared<BatchClient>(
          *opt_user_bind_grpc_endpoint,
          grpc_executor,
          coalesce_runner,
          options);
        async_batch_active_objects->add_child_object(delayed_batch_client);
      }

      if (delayed_batch_client && *opt_delayed_percent == 100)
      {
        client = delayed_batch_client.get();
      }
    }
    else if (!reconnect_per_request && *mode == Mode::DistributedGrpc)
    {
      AdServer::Grpc::BatchingOptions options;
      options.channels_number = max_streams;
      options.max_batch_size = *opt_max_batch_size;
      options.max_inflight = max_inflight;
      options.error_on_inflight_reaching = *opt_error_on_inflight_reaching != 0;
      options.workers_number = client_threads;
      options.hot_buckets_count = *opt_hot_buckets_count;
      options.max_batch_delay = make_time_option(*opt_max_batch_delay_us);
      options.max_queue_wait = make_time_option(*opt_max_queue_wait_us);
      options.stream_start_timeout = make_time_option(*opt_stream_start_timeout_us);
      options.enable_grpc_compression = *opt_grpc_compression != 0;
      options.use_local_subchannel_pool = *opt_local_subchannel_pool != 0;

      std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor =
        std::make_shared<AdServer::Grpc::GrpcExecutor>(client_threads);
      auto coalesce_runner =
        std::make_shared<AdServer::Commons::BoostAsioContextRunActiveObject>(
          Generics::ActiveObjectCallback_var(new NullActiveObjectCallback()),
          std::make_shared<boost::asio::io_service>(),
          client_threads);
      distributed_client =
        std::make_shared<AdServer::UserInfoSvcs::UserBindDistributedGrpcClient>(
          AdServer::UserInfoSvcs::UserBindDistributedGrpcClient::
            UserBindControllerRefs{{*opt_user_bind_controller_grpc_endpoint}},
          options,
          grpc_executor,
          nullptr,
          coalesce_runner);
      async_batch_active_objects->add_child_object(grpc_executor);
      async_batch_active_objects->add_child_object(coalesce_runner);
      async_batch_active_objects->add_child_object(distributed_client);
      client = distributed_client.get();
    }

    if (!reconnect_per_request)
    {
      async_batch_active_objects->activate_object();
    }

    std::thread reporter_thread([&]() {
      std::uint64_t prev = 0;
      std::uint64_t prev_errors = 0;
      ErrorAccumulator::ErrorMap prev_error_details;
      std::uint64_t prev_write_batches = 0;
      std::uint64_t prev_write_items = 0;
      std::uint64_t prev_timing_coalesce_items = 0;
      std::uint64_t prev_queue_wait_count = 0;
      std::uint64_t prev_queue_wait_sum_us = 0;
      std::uint64_t prev_queue_timeout_count = 0;
      std::uint64_t prev_response_wait_count = 0;
      std::uint64_t prev_response_wait_sum_us = 0;
      std::uint64_t prev_consumer_stream_write_count = 0;
      std::uint64_t prev_consumer_stream_write_sum_us = 0;
      std::uint64_t prev_latency_count = 0;
      std::uint64_t prev_latency_sum_us = 0;
      while (done_count.load(std::memory_order_relaxed) < *opt_count)
      {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        const auto now = std::chrono::system_clock::now();
        const std::time_t now_tt = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
        localtime_r(&now_tt, &tm);

        const auto current = done_count.load(std::memory_order_relaxed);
        const auto per_second = current - prev;
        prev = current;
        const auto current_errors = error_count.load(std::memory_order_relaxed);
        const auto errors_delta = current_errors - prev_errors;
        prev_errors = current_errors;
        const auto current_latency_count = latency_count.load(std::memory_order_relaxed);
        const auto current_latency_sum_us = latency_sum_us.load(std::memory_order_relaxed);
        const auto latency_count_delta = current_latency_count - prev_latency_count;
        const auto latency_sum_delta_us = current_latency_sum_us - prev_latency_sum_us;
        prev_latency_count = current_latency_count;
        prev_latency_sum_us = current_latency_sum_us;
        const auto avg_latency_us = latency_count_delta == 0 ?
          0.0 :
          static_cast<double>(latency_sum_delta_us) /
            static_cast<double>(latency_count_delta);

        std::cout << std::put_time(&tm, "%T") << ": " << per_second << ", errors=" << errors_delta;
        if (*opt_print_errors != 0 && errors_delta != 0)
        {
          const auto current_error_details = errors.snapshot();
          std::cout << ", error_details=";
          ErrorAccumulator::print_delta(std::cout, current_error_details, prev_error_details);
          prev_error_details = std::move(current_error_details);
        }
        std::cout << ", avg_latency=" << format_stat_float(avg_latency_us) << "us" <<
          ", p99_latency=" << latency_p99.p99(current_latency_count) << "us" <<
          ", max_latency=" << latency_max_us.load(std::memory_order_relaxed) << "us";
        if (client)
        {
          const auto stats = client->stats();
          const auto write_batches = stats.write_batches - prev_write_batches;
          const auto write_items = stats.write_items - prev_write_items;
          const auto timing_coalesce_items =
            stats.timing_coalesce_items - prev_timing_coalesce_items;
          const auto queue_wait_count = stats.queue_wait_count - prev_queue_wait_count;
          const auto queue_wait_sum_us = stats.queue_wait_sum_us - prev_queue_wait_sum_us;
          const auto response_wait_count = stats.response_wait_count - prev_response_wait_count;
          const auto response_wait_sum_us = stats.response_wait_sum_us - prev_response_wait_sum_us;
          const auto queue_timeout_count = stats.queue_timeout_count - prev_queue_timeout_count;
          prev_write_batches = stats.write_batches;
          prev_write_items = stats.write_items;
          prev_timing_coalesce_items = stats.timing_coalesce_items;
          prev_queue_wait_count = stats.queue_wait_count;
          prev_queue_wait_sum_us = stats.queue_wait_sum_us;
          prev_queue_timeout_count = stats.queue_timeout_count;
          prev_response_wait_count = stats.response_wait_count;
          prev_response_wait_sum_us = stats.response_wait_sum_us;

          const auto avg_batch = write_batches == 0 ?
            0.0 :
            static_cast<double>(write_items) / static_cast<double>(write_batches);
          const auto total_avg_batch = stats.write_batches == 0 ?
            0.0 :
            static_cast<double>(stats.write_items) /
              static_cast<double>(stats.write_batches);

          std::cout << ", writes=" << write_batches <<
            ", avg_batch=" << format_stat_float(avg_batch) <<
            ", total_avg_batch=" << format_stat_float(total_avg_batch) <<
            ", timing_items=" << timing_coalesce_items <<
            ", total_timing_items=" << stats.timing_coalesce_items <<
            ", max_streams=" << stats.max_streams;
          print_async_batch_diagnostics(std::cout, stats);
          if (queue_wait_count != 0)
          {
            std::cout << ", avg_queue_wait=" <<
              format_stat_float(
                static_cast<double>(queue_wait_sum_us) /
                  static_cast<double>(queue_wait_count)) <<
              "us, max_queue_wait=" << stats.queue_wait_max_us << "us";
          }

          if (queue_timeout_count != 0)
          {
            std::cout << ", queue_timeouts=" << queue_timeout_count <<
              ", total_queue_timeouts=" << stats.queue_timeout_count;
          }

          if (response_wait_count != 0)
          {
            std::cout << ", avg_response_wait=" <<
              format_stat_float(
                static_cast<double>(response_wait_sum_us) /
                  static_cast<double>(response_wait_count)) <<
              "us, max_response_wait=" << stats.response_wait_max_us << "us";
          }

          if (stats.consumer_stream_write.has_value())
          {
            const auto consumer_stream_write_count =
              stats.consumer_stream_write->count -
                prev_consumer_stream_write_count;
            const auto consumer_stream_write_sum_us =
              stats.consumer_stream_write->sum_us -
                prev_consumer_stream_write_sum_us;
            prev_consumer_stream_write_count = stats.consumer_stream_write->count;
            prev_consumer_stream_write_sum_us = stats.consumer_stream_write->sum_us;
            if (consumer_stream_write_count != 0)
            {
              std::cout << ", avg_consumer_stream_write=" <<
                format_stat_float(
                  static_cast<double>(consumer_stream_write_sum_us) /
                    static_cast<double>(consumer_stream_write_count)) <<
                "us, max_consumer_stream_write=" << stats.consumer_stream_write->max_us << "us";
            }
          }
        }
        std::cout << std::endl;
      }
    });

    const auto run_start = std::chrono::steady_clock::now();
    const auto cpu_start = process_cpu_times();

    std::vector<std::thread> sender_threads;
    sender_threads.reserve(*opt_threads);

    for (unsigned long i = 0; i < *opt_threads; ++i)
    {
      sender_threads.emplace_back([&]() {
        std::mt19937 gen(std::random_device{}());

        while (true)
        {
          const auto req_index = sent_count.fetch_add(1, std::memory_order_relaxed);
          if (req_index >= *opt_count)
          {
            return;
          }

          adserver::user_info_svcs::user_bind::GetUserIdRequest request;
          if (opt_user_id.installed())
          {
            request.set_id(*opt_user_id);
          }
          else
          {
            request.set_id(random_external_id(gen));
          }

          auto now = Generics::Time::get_time_of_day();
          request.set_timestamp(GrpcAlgs::pack_time(now));
          request.set_silent(true);
          request.set_generate_user_id(false);
          request.set_for_set_cookie(false);
          request.set_create_timestamp(GrpcAlgs::pack_time(now));
          request.set_current_user_id(GrpcAlgs::pack_user_id(AdServer::Commons::UserId()));

          auto* request_client = client;
          if (delayed_batch_client && *opt_delayed_percent != 0 && *opt_delayed_percent != 100)
          {
            std::uniform_int_distribution<unsigned int> delayed_dist(1, 100);
            if (delayed_dist(gen) <= *opt_delayed_percent)
            {
              request_client = delayed_batch_client.get();
            }
          }

          const auto start = std::chrono::steady_clock::now();
          const auto complete_request =
            [
              &error_count,
              &errors,
              &done_count,
              &latency_count,
              &latency_sum_us,
              &latency_max_us,
              &latency_p99,
              start
            ](const grpc::Status& status) {
              const auto finish = std::chrono::steady_clock::now();
              const auto latency_us = static_cast<std::uint64_t>(
                std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count());
              latency_count.fetch_add(1, std::memory_order_relaxed);
              latency_sum_us.fetch_add(latency_us, std::memory_order_relaxed);
              latency_p99.add(latency_us);
              auto current_max = latency_max_us.load(std::memory_order_relaxed);
              while (current_max < latency_us &&
                !latency_max_us.compare_exchange_weak(
                  current_max,
                  latency_us,
                  std::memory_order_relaxed))
              {}
              if (!status.ok())
              {
                errors.add(status);
                error_count.fetch_add(1, std::memory_order_relaxed);
              }
              done_count.fetch_add(1, std::memory_order_relaxed);
            };

          if (reconnect_per_request)
          {
            grpc::ChannelArguments channel_args;
            channel_args.SetMaxReceiveMessageSize(-1);
            channel_args.SetMaxSendMessageSize(-1);
            channel_args.SetInt(GRPC_ARG_ENABLE_HTTP_PROXY, 0);
            channel_args.SetInt(
              GRPC_ARG_USE_LOCAL_SUBCHANNEL_POOL,
              *opt_local_subchannel_pool != 0 ? 1 : 0);
            if (*opt_grpc_compression == 0)
            {
              channel_args.SetCompressionAlgorithm(GRPC_COMPRESS_NONE);
            }

            auto channel = AdServer::Grpc::create_custom_channel(
              *opt_user_bind_grpc_endpoint,
              grpc::InsecureChannelCredentials(),
              channel_args);
            auto stub = adserver::user_info_svcs::user_bind::UserBindServerGrpc::NewStub(channel);
            grpc::ClientContext context;
            context.set_deadline(
              std::chrono::system_clock::now() +
              std::chrono::milliseconds(*opt_rpc_timeout_ms));
            adserver::user_info_svcs::user_bind::GetUserIdResponse response;
            complete_request(stub->get_user_id(&context, request, &response));
          }
          else
          {
            request_client->get_user_id(
              request,
              [complete_request](const grpc::Status& status, const auto&) {
                complete_request(status);
              });
          }
        }
      });
    }

    for (auto& sender : sender_threads)
    {
      sender.join();
    }

    while (done_count.load(std::memory_order_relaxed) < *opt_count)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    const auto run_finish = std::chrono::steady_clock::now();
    const auto cpu_finish = process_cpu_times();

    if (!reconnect_per_request)
    {
      async_batch_active_objects->deactivate_object();
      async_batch_active_objects->wait_object();
    }

    reporter_thread.join();

    const auto stats = client ? client->stats() : AdServer::Grpc::Stats();
    const auto total_avg_batch = stats.write_batches == 0 ?
      0.0 :
      static_cast<double>(stats.write_items) /
        static_cast<double>(stats.write_batches);
    const auto total_latency_count = latency_count.load(std::memory_order_relaxed);
    const auto total_avg_latency_us = total_latency_count == 0 ?
      0.0 :
      static_cast<double>(latency_sum_us.load(std::memory_order_relaxed)) /
        static_cast<double>(total_latency_count);
    const auto run_seconds = std::chrono::duration<double>(run_finish - run_start).count();
    const auto rps = run_seconds == 0.0 ?
      0.0 :
      static_cast<double>(done_count.load(std::memory_order_relaxed)) /
        run_seconds;
    const auto user_cpu_seconds = cpu_finish.user - cpu_start.user;
    const auto sys_cpu_seconds = cpu_finish.sys - cpu_start.sys;
    const auto cpu_seconds = user_cpu_seconds + sys_cpu_seconds;
    std::cout << "completed: " << done_count.load(std::memory_order_relaxed) <<
      ", errors: " << error_count.load(std::memory_order_relaxed) <<
      ", rps: " << format_stat_float(rps) <<
      ", cpu_time: " << format_stat_float(cpu_seconds) << "s" <<
      ", user_cpu_time: " << format_stat_float(user_cpu_seconds) << "s" <<
      ", sys_cpu_time: " << format_stat_float(sys_cpu_seconds) << "s" <<
      ", writes: " << stats.write_batches <<
      ", write_items: " << stats.write_items <<
      ", avg_batch: " << format_stat_float(total_avg_batch) <<
      ", timing_items: " << stats.timing_coalesce_items << ", max_streams: " << stats.max_streams;
    print_async_batch_diagnostics(std::cout, stats);
    std::cout <<
      ", avg_latency: " << format_stat_float(total_avg_latency_us) << "us" <<
      ", p99_latency: " << latency_p99.p99(total_latency_count) << "us" <<
      ", max_latency: " << latency_max_us.load(std::memory_order_relaxed) << "us";
    if (stats.queue_wait_count != 0)
    {
      std::cout << ", avg_queue_wait: " <<
        format_stat_float(
          static_cast<double>(stats.queue_wait_sum_us) /
            static_cast<double>(stats.queue_wait_count)) <<
        "us, max_queue_wait: " << stats.queue_wait_max_us << "us";
    }

    if (stats.queue_timeout_count != 0)
    {
      std::cout << ", queue_timeouts: " << stats.queue_timeout_count;
    }

    if (stats.response_wait_count != 0)
    {
      std::cout << ", avg_response_wait: " <<
        format_stat_float(
          static_cast<double>(stats.response_wait_sum_us) /
            static_cast<double>(stats.response_wait_count)) <<
        "us, max_response_wait: " << stats.response_wait_max_us << "us";
    }

    if (stats.consumer_stream_write.has_value() && stats.consumer_stream_write->count != 0)
    {
      std::cout << ", avg_consumer_stream_write: " <<
        format_stat_float(
          static_cast<double>(stats.consumer_stream_write->sum_us) /
            static_cast<double>(stats.consumer_stream_write->count)) <<
        "us, max_consumer_stream_write: " << stats.consumer_stream_write->max_us << "us";
    }

    if (*opt_print_errors != 0 && error_count.load(std::memory_order_relaxed) != 0)
    {
      std::cout << ", error_details=";
      ErrorAccumulator::print_total(std::cout, errors.snapshot());
    }
    std::cout << std::endl;

    distributed_client.reset();
    batch_client.reset();

    return 0;
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << "Caught eh::Exception: " << ex.what() << std::endl;
  }

  return 1;
}
