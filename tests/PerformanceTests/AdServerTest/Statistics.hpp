
#ifndef __STATISTICS_HPP
#define __STATISTICS_HPP

#include <map>
#include <tests/PerformanceTests/Commons/Constraint.hpp>
#include <tests/PerformanceTests/Commons/StatCommons.hpp>
#include "Configuration.hpp"
#include <Generics/Time.hpp>


class Report;

class AdvertisingStatistics;
class FrontendStatistics;
class Statistics;

/**
 * @struct RequestCounterContainer
 * @brief Counters container for requests
 *        Contain total, opted in & opted out counters
 */
class RequestCounterContainer :
  public virtual ReferenceCounting::Interface,
  public virtual ReferenceCounting::AtomicImpl
{

  friend class AdvertisingStatistics;
  friend class FrontendStatistics;
  friend class Statistics;

public:

  /**
   * @brief Constructor                       .
   * @param master counter for total
   */
  RequestCounterContainer(Counter* master);

  /**
   * @brief Increment counter
   * @param request opted out flag
   */
  void incr(bool is_opted_out);

  /**
   * @brief Reset all container counters
   *
   * Using when start new second
   */
  void reset();

  /**
   * @brief Get total counter.
   * @return total counter.
   */
  const ConstraintableCounter& total() const;

  /**
   * @brief Get opted in counter.
   * @return opted in counter.
   */
  const SlaveCounter& opted_in() const;

  /**
   * @brief Get opted out counter.
   * @return opted out counter.
   */
  const SlaveCounter& opted_out() const;


private:
  ConstraintableCounter total_;
  SlaveCounter opted_in_;
  SlaveCounter opted_out_;
};

typedef
ReferenceCounting::SmartPtr<RequestCounterContainer>
RequestCounterContainer_var;

/**
 * @struct AdvertisingStatistics
 * @brief Storing advertising statistics
 */
class AdvertisingStatistics :
  public virtual ReferenceCounting::Interface,
  public virtual ReferenceCounting::AtomicImpl
{
public:
  typedef std::map <unsigned long, RequestCounterContainer_var>
    RequestCounterContainerList;

public:

  /**
   * @brief Constructor.
   * @param frontend name
   */
  AdvertisingStatistics(const char* frontend_name);

  /**
   * @brief Destructor.
   */
  ~AdvertisingStatistics() noexcept;

  /**
   * @brief push response.
   * @param CCID from response
   * @param request opted out flag
   */
  void push_response(unsigned long ccid,
                     bool optout);

  /**
   * @brief Get frontend name.
   * @return frontend name.
   */
  const std::string& frontend_name() const;

  /**
   * @brief Get total response counter container.
   * @return total responses.
   */
  const RequestCounterContainer& total_responses() const;

  /**
   * @brief Get creative response counter container.
   *        Response with no zero creatives
   * @return total responses.
   */
  const RequestCounterContainer& creative_responses() const;

  /**
   * @brief Get CCID statistic.
   * @return Counter dictionary ccid => counter.
   */
  const RequestCounterContainerList& ccid_stats() const;

private:
  std::string frontend_name_;
  RequestCounterContainer total_responses_;
  RequestCounterContainer creative_responses_;
  RequestCounterContainerList ccid_stats_;
};



/**
 * @struct FrontendStatistics
 * @brief Storing statistics by frontends
 */
class FrontendStatistics:
  public virtual ReferenceCounting::Interface,
  public virtual ReferenceCounting::AtomicImpl
{

public:

  /**
   * @brief Constructor.
   * @param frontend name
   * @param client config
   * @param reference for total request counter
   * @param constraints container
   */
  FrontendStatistics(const char* frontend_name,
                     const ClientConfig_var& config,
                     RequestCounterContainer& total_requests_,
                     ConstraintsContainer& constraints);

  /**
   * @brief Destructor.
   */
  ~FrontendStatistics() noexcept;

  /**
   * @brief push response.
   * @param request opted out flag
   */
  bool push_response(bool is_opted_out);

  /**
   * @brief push error.
   * @param request opted out flag
   */
  void push_error(bool is_opted_out);

  /**
   * @brief Get request counter.
   * @return request counter.
   */
  const RequestCounterContainer& requests() const;

  /**
   * @brief Get approved request counter.
   * @return approved counter.
   */
  const RequestCounterContainer& approved() const;

  /**
   * @brief Get error request counter.
   * @return error counter.
   */
  const RequestCounterContainer& errors() const;

  /**
   * @brief Get frontend name.
   * @return frontend name.
   */
  const std::string& frontend_name() const;

private:
  ConstraintsContainer& constraints_;
  RequestCounterContainer& total_requests_;
  std::string frontend_name_;
  const ConstraintConfig_var& constraint_;
  Constraint_var  error_constraint_;
  RequestCounterContainer requests_;
  RequestCounterContainer approved_;
  RequestCounterContainer errors_;
  time_t start_time_;

private:
  // Helper function, find constraint by name
  const ConstraintConfig_var& _get_constraint_by_name(const ClientConfig_var& config,
                                                      const char* frontend_name);
};


typedef ReferenceCounting::SmartPtr<AdvertisingStatistics> AdvertisingStatistics_var;
typedef ReferenceCounting::SmartPtr<FrontendStatistics> FrontendStatistics_var;

typedef std::map <std::string, AdvertisingStatistics_var> AdvertisingStatList;
typedef std::map <std::string, FrontendStatistics_var> FrontendStatList;


/**
 * @struct Statistics
 * @brief Global performance test statistics (Model).
 */
class Statistics
{
  typedef Sync::PosixRWLock Mutex_;
  typedef Sync::PosixRGuard ReadGuard_;
  typedef Sync::PosixWGuard WriteGuard_;

public:
  // Reset counters enum
  enum ResetEnum {RST_NONE, RST_DISCOVER, RST_CHANNELS, RST_COUNTERS};

public:

  /**
   * @brief Constructor.
   * @param constraints container.
   * @param test configuration.
   */
  Statistics(ConstraintsContainer& constraints,
             const ClientConfig_var& config);

  /**
   * @brief Push response.
   * @param URL.
   * @param request opted out flag
   * @param CCID.
   * @param advertising response.
   */
  void push_response(const char* url,
                     bool is_opted_out,
                     unsigned long ccid,
                     const AdvertiserResponse* ad_response);

  /**
   * @brief Push error.
   * @param URL.
   * @param request opted out flag
   */
  void push_error(const char* url,
                  bool is_opted_out);

  /**
   * @brief Publish & refresh statistic.
   * @param Report object for publishing.
   * @param reset counters flag.
   */
  void refresh(Report* report, ResetEnum reset = RST_NONE);

  /**
   * @brief Get total requests.
   * @return total requests counter.
   */
  const RequestCounterContainer& total_requests() const;

  /**
   * @brief Get total approved requests.
   * @return total approved counter.
   */
  const RequestCounterContainer& total_approved() const;

  /**
   * @brief Get total errors requests.
   * @return total errors counters.
   */
  const RequestCounterContainer& total_errors() const;

  /**
   * @brief Get profiling requests.
   * @return profiling request counters.
   */
  const RequestCounterContainer& profiling_requests() const;

  /**
   * @brief Get using channels statistics (adrequest).
   * @return channels statistics as RangeStats.
   */
  const RangeStats& request_channels_stats() const;

  /**
   * @brief Get advertising performance statistic.
   * @return advertising performance statistic.
   */
  const AdvPerformanceStats& adv_performance_stats() const;

  /**
   * @brief Get advertising (CCID) statistic.
   * @return advertising statistics.
   */
  const AdvertisingStatList& adv_stats() const;


  /**
   * @brief Get frontend statistic.
   * @return frontend statistics.
   */
  const FrontendStatList& frontend_stats() const;

  /**
   * @brief Get nsloookup creative stats
   * @return advertising stats.
   */
  AdvertisingStatistics* get_nslookup_stats();


private:
  ConstraintsContainer& constraints_;
  const ClientConfig_var& config_;
  mutable Mutex_ lock_;
  RequestCounterContainer total_requests_;
  RequestCounterContainer total_approved_;
  RequestCounterContainer total_errors_;
  RequestCounterContainer profiling_requests_;
  RangeStats request_channels_stats_;
  // Frontend stats
  AdvPerformanceStats adv_performance_stats_;
  AdvertisingStatList adv_stats_;
  FrontendStatList frontend_stats_;

  FrontendStatistics_var _get_frontend_stat_by_name(const char* frontend_name);
  AdvertisingStatistics_var _get_advertising_stat_by_name(const char* frontend_name);
  void _get_frontend_name(const char* url, std::string& frontend_name);
  bool _is_profiling_req(const char* url);
};


#endif  // __STATISTICS_HPP
