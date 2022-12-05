#ifndef USERINFOSVCS_USERINFOMANAGER_FILERWSTATS_HPP_
#define USERINFOSVCS_USERINFOMANAGER_FILERWSTATS_HPP_

#include <list>

#include <ProfilingCommons/PlainStorage3/FileController.hpp>

namespace AdServer
{
namespace UserInfoSvcs
{
  class FileRWStats : public ProfilingCommons::PosixFileController::Stat
  {
  public:
    typedef Sync::Policy::PosixThread SyncPolicy;

    struct Counters : public ProfilingCommons::StatImpl::Counters
    {
      unsigned long sum_size;

      Counters()
        : ProfilingCommons::StatImpl::Counters(), sum_size(0)
      {}
    };

    struct IntervalStat
    {
      Counters read;
      Counters write;
      Generics::Time timestamp;
    };

    typedef std::list<IntervalStat> IntervalStats;

  public:
    FileRWStats(
      const Generics::Time interval,
      const std::size_t times)
      noexcept;

    IntervalStats
    get_stats() const noexcept;

  protected:
    virtual
    ~FileRWStats() noexcept
    {}

    void
    add_read_time_(
      const Generics::Time& start,
      const Generics::Time& stop,
      unsigned long size)
      noexcept;

    void
    add_write_time_(
      const Generics::Time& start,
      const Generics::Time& stop,
      unsigned long size)
      noexcept;

    void
    check_for_new_interval_i_(const Generics::Time& stop) noexcept;

    void
    add_time_i_(
      const Generics::Time& start,
      const Generics::Time& stop,
      unsigned long size,
      Counters& counters)
      noexcept;

  private:
    const Generics::Time interval_;
    const std::size_t times_;
    mutable SyncPolicy::Mutex stats_lock_;
    IntervalStats stats_;
  };
}
}

#endif /* USERINFOSVCS_USERINFOMANAGER_FILERWSTATS_HPP_ */
