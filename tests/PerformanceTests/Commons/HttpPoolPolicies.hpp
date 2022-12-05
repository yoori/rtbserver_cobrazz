
#ifndef __HTTPPOOLPOLICIES_HPP
#define __HTTPPOOLPOLICIES_HPP

#include <HTTP/HttpAsyncPool.hpp>
#include <HTTP/HttpAsyncPolicies.hpp>
#include <Generics/Time.hpp>
#include <Logger/Logger.hpp>

using namespace HTTP;

class HttpPoolPolicy  :  public PoolPolicy,
                         public PoolPolicySimpleDecider,
                         public PoolPolicySimpleEmptyConnection,
                         public PoolPolicySimpleEmptyThread,
                         public PoolPolicyWaitRequests,
                         public PoolPolicySimpleTimeout
{

  static const unsigned REQUEST_QUEUE_SIZE = 10000;

  typedef Sync::Condition Condition_;
  
public:

  /**
   * @brief Constructor
   *
   * @param logger
   * @param connections per server
   * @param connections per thread
   * @param request's timeout
   */
  HttpPoolPolicy(
    Logging::Logger* logger,
    unsigned connections_per_server,
    unsigned connections_per_threads,
    const time_t timeout) /*throw(eh::Exception)*/;

  /**
   * @brief Destructor
   */
  virtual ~HttpPoolPolicy() noexcept;

  /**
   * @brief request constructing 
   */
  virtual void request_constructing() /*throw(eh::Exception)*/;
  
  /**
   * @brief request destroying
   */
  virtual void request_destroying() noexcept;

  /**
   * @brief wait for pool queue empty
   */
  void wait_empty() noexcept;

  /**
   * @brief Get pool requests queue size
   *
   * @return requests queue size
   */
  unsigned long get_request_queue_size();

  /**
   * @brief Get pool threads count
   *
   * @return threads count
   */
  unsigned long get_threads_count();

  /**
   * @brief Get pool connections count
   *
   * @return connections count
   */
  unsigned long get_connections_count();

  /**
   * @brief Get pool connections count
   * inherited from ActiveObject
   * @param severity
   * @param description
   */
  virtual void report_error(
    Generics::ActiveObjectCallback::Severity,
    const String::SubString& description,
    const char* ) noexcept;

private:
  Logging::Logger* logger_;                   // logger
  unsigned max_connections_;                  // max connections
  unsigned max_threads_;                      // max threads
  volatile _Atomic_word request_queue_size_;  // current queue size
  mutable Condition_ condition_;              // condition for waiting empty queue
};

typedef ReferenceCounting::SmartPtr<HttpPoolPolicy> HttpPoolPolicy_var;


#endif  // __HTTPPOOLPOLICIES_HPP
