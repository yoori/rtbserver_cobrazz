#include "FileRWStats.hpp"

namespace AdServer
{
namespace UserInfoSvcs
{
  FileRWStats::FileRWStats(
    const Generics::Time interval,
    const std::size_t times)
    noexcept
    : interval_(interval), times_(times)
  {
    IntervalStat stat;
    stat.timestamp = Generics::Time::get_time_of_day();
    stats_.push_back(stat);
  }

  FileRWStats::IntervalStats
  FileRWStats::get_stats() const noexcept
  {
    SyncPolicy::ReadGuard lock(stats_lock_);
    return stats_;
  }

  void
  FileRWStats::add_read_time_(
    const Generics::Time& start,
    const Generics::Time& stop,
    unsigned long size)
    noexcept
  {
    SyncPolicy::WriteGuard lock(stats_lock_);
    check_for_new_interval_i_(stop);
    add_time_i_(start, stop, size, stats_.back().read);
  }

  void
  FileRWStats::add_write_time_(
    const Generics::Time& start,
    const Generics::Time& stop,
    unsigned long size)
    noexcept
  {
    SyncPolicy::WriteGuard lock(stats_lock_);
    check_for_new_interval_i_(stop);
    add_time_i_(start, stop, size, stats_.back().write);
  }

  void
  FileRWStats::check_for_new_interval_i_(const Generics::Time& stop) noexcept
  {
    if (stop > stats_.back().timestamp &&
        (stop - stats_.back().timestamp) > interval_)
    {
      IntervalStat stat;
      stat.timestamp = stop;
      stats_.push_back(stat);

      if (stats_.size() > times_)
      {
        stats_.pop_front();
      }
    }
  }

  void
  FileRWStats::add_time_i_(
    const Generics::Time& start,
    const Generics::Time& stop,
    unsigned long size,
    Counters& counters)
    noexcept
  {
    const Generics::Time time = stop - start;
    counters.max_time = std::max(counters.max_time, time);
    counters.sum_time += time;
    counters.sum_size += size;
    ++counters.count;
  }
}
}
