
#include "HttpPoolPolicies.hpp"

// class HttpPoolPolicy

HttpPoolPolicy::HttpPoolPolicy(
  Logging::Logger* logger,
  unsigned connections_per_server,
  unsigned connections_per_threads,
  const time_t  timeout) /*throw(eh::Exception)*/ :
  PoolPolicySimpleDecider(connections_per_server, connections_per_threads),
  PoolPolicyWaitRequests(REQUEST_QUEUE_SIZE),
  PoolPolicySimpleTimeout(timeout),
  logger_(logger),
  request_queue_size_(0)
{
}

HttpPoolPolicy::~HttpPoolPolicy() noexcept
{
}

void HttpPoolPolicy::request_constructing() /*throw(eh::Exception)*/
{
  __gnu_cxx::__atomic_add(&request_queue_size_, 1);
  condition_.broadcast();
  PoolPolicyWaitRequests::request_constructing();
}

void HttpPoolPolicy::request_destroying() noexcept
{
  __gnu_cxx::__atomic_add(&request_queue_size_, -1);
  condition_.broadcast();
  PoolPolicyWaitRequests::request_destroying();
}

void HttpPoolPolicy::wait_empty() noexcept
{
  while (request_queue_size_ > 0)
  {
    condition_.wait();
  }
}

unsigned long HttpPoolPolicy::get_request_queue_size()
{
  return request_queue_size_;
}

unsigned long HttpPoolPolicy::get_threads_count()
{
  return get_threads_().size();
}

unsigned long HttpPoolPolicy::get_connections_count()
{
  return get_connections_().size();
}
  
void HttpPoolPolicy::report_error(
  Generics::ActiveObjectCallback::Severity,
  const String::SubString& description,
  const char* ) noexcept
{
  logger_->stream(Logging::Logger::WARNING) <<
    "HttpPoolPolicies, error: " << description;
}


