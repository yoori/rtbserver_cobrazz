
#ifndef _STATISTICS_HPP
#define _STATISTICS_HPP

#include <set>
#include <Sync/SyncPolicy.hpp>
#include <tests/PerformanceTests/Commons/StatCommons.hpp>

/**
 * @struct Statistics
 * @brief Benchmark statistics
 */
class Statistics
{

  typedef Sync::PosixRWLock Mutex_;
  typedef Sync::PosixRGuard ReadGuard_;
  typedef Sync::PosixWGuard WriteGuard_;
  typedef std::set<std::string> UidSet;

public:

  /**
   * @brief Constructor.
   */
  Statistics();

  /**
   * @brief Increment errors counter.
   */
  void incr_errors();

  /**
   * @brief Add response information.
   *
   * @param client UUID
   * @param advertising response
   */
  void add_response(const char* uid,
                    const AdvertiserResponse* ad_response);

  /**
   * @brief Get errors counter
   *
   * @return number of errors
   */
  unsigned long errors() const;

  /**
   * @brief Get creatives counter
   *
   * @return number of responses with no-zero creative
   */
  unsigned long creatives() const;

  /**
   * @brief Get response counter
   *
   * @return number of responses
   */
  unsigned long responses() const;

  /**
   * @brief Get benchmark duration
   *
   * @return duration
   */
  Generics::Time duration() const;

  /**
   * @brief Get clients size
   *
   * @return number of different clients in benchmark
   */
  size_t uids_size() const;

  /**
   * @brief Get using channels statistics (adrequest).
   * @return channels statistics as RangeStats.
   */
  const RangeStats& trigger_channels_stats() const;

  /**
   * @brief Get using channels statistics (adrequest).
   * @return channels statistics as RangeStats.
   */
  const RangeStats& history_channels_stats() const;

  /**
   * @brief Get advertising performance statistic.
   * @return advertising performance statistic.
   */
  const AdvPerformanceStats& adv_performance_stats() const;


  /**
   * @brief Reset statistics.
   */
  void reset();

private:
  unsigned long errors_;      // errors count
  unsigned long creatives_;   // creatives count
  unsigned long responses_;   // response count
  Generics::Time start_time_; // benchmark start time

  RangeStats trigger_channels_stats_;  // trigger channels usage stats
  RangeStats history_channels_stats_;  // history channels usage stats
  AdvPerformanceStats adv_performance_stats_; // advertising performance stats

  UidSet uids_;  // Client UUIDs set
  mutable Mutex_ lock_;
};


#endif  //_STATISTICS_HPP
