#ifndef _BENCHMARK_HPP
#define _BENCHMARK_HPP

#include <tests/PerformanceTests/Commons/HttpPoolPolicies.hpp>
#include "BenchmarkStorage.hpp"


/**
 * @class BenchmarkBase
 * @brief Benchmark base class
 */
class BenchmarkBase
{
  
public:

  DECLARE_EXCEPTION(NotSupported, eh::DescriptiveException);

  /**
   * @brief Constructor
   *
   * @param config
   * @param storage
   * @param HTTP pool
   * @param logger
   */
  BenchmarkBase(
    const BenchmarkConfig* config,
    BenchmarkStorage* storage,
    HttpInterface* pool,
    Logging::Logger* logger);

  /**
   * @brief Destructor
   *
   */
  virtual ~BenchmarkBase() noexcept;

  /**
   * @brief Benchmark implementation.
   */
  virtual bool run();

protected:

  /**
   * @brief Send request through client
   *
   * @param client
   */
  virtual void send_request(AdServerClientBase* client);

  /**
   * @brief Send request from storage
   */
  virtual void send_storage_request() = 0;


  /**
   * @brief Make client
   *
   * @param request ID
   * @return server client
   */
  AdServerClientBase* make_client(unsigned long client_id);

  /**
   * @brief Make benchmark request
   *
   * @param client
   * @param request URL
   */
  BaseRequest* make_request(AdServerClientBase* client,
                            const char* url = 0);

protected:
  const BenchmarkConfig* config_;     // benchmark config
  BenchmarkStorage* storage_;         // benchmarks storage
  HttpInterface* pool_;               // HTTP pool
  Logging::Logger* logger_;          // Logger
};


/**
 * @class Benchmark
 * @brief Benchmark with URL creation
 */
class Benchmark : public BenchmarkBase
{
public:

  /**
   * @brief Constructor
   *
   * @param config
   * @param storage
   * @param HTTP pool
   * @param logger
   */
  Benchmark(
    const BenchmarkConfig* config,
    BenchmarkStorage* storage,
    HttpInterface* pool,
    Logging::Logger* logger);
  
  /**
   * @brief Destructor
   *
   */
  virtual ~Benchmark() noexcept;

protected:

  /**
   * @brief Send request through client
   *
   * @param client
   */
  virtual void send_request(AdServerClientBase* client);

  /**
   * @brief Send request from storage
   */
  virtual void send_storage_request();
  
};


/**
 * @class GeneratedBenchmark
 * @brief Benchmark with generated URL
 */
class GeneratedBenchmark : public BenchmarkBase
{
public:

  /**
   * @brief Constructor
   *
   * @param Application
   * @param Benchmark size (in requests) 
   * @param Opted out benchmark
   * @param request config
   * @param logger
   */
  GeneratedBenchmark(
    const BenchmarkConfig* config,
    BenchmarkStorage* storage,
    HttpInterface* pool,
    Logging::Logger* logger);
  
  /**
   * @brief Destructor
   *
   */
  virtual ~GeneratedBenchmark() noexcept;

  /**
   * @brief Benchmark implementation.
   */
  virtual bool run();

protected:

  /**
   * @brief Send request from storage
   */
  virtual void send_storage_request();
};

typedef std::unique_ptr<BenchmarkBase> Benchmark_var;

#endif  // _BENCHMARK_HPP
