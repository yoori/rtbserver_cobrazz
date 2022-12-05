/// @file ExpressionMatcherStats.hpp

#ifndef EXPRESSION_MATCHER_STATS_INCLUDED
#define EXPRESSION_MATCHER_STATS_INCLUDED

#include <Commons/AtomicInt.hpp>
#include <Generics/Values.hpp>
#include <Generics/Time.hpp>
#include <Sync/SyncPolicy.hpp>

namespace AdServer
{
  namespace RequestInfoSvcs
  {
    struct Stats
    {
      Stats() noexcept;

      Algs::AtomicInt processed_matches_optin_user;
      Algs::AtomicInt processed_matches_temporary_user;
      Algs::AtomicInt processed_matches_non_optin_user;
      Generics::Time last_processed_file_timestamp;
    };

    class StatsCounters : private Stats
    {
    public:
      void
      inc_not_optedin_user_processed() noexcept;

      void
      inc_temporary_user_processed() noexcept;

      void
      inc_persistent_user_processed() noexcept;

      void
      set_last_processed_timestamp(const Generics::Time& time)
        noexcept;

      const Stats&
      get_stats() const noexcept;
    };

    class ExpressionMatcherStatsImpl : public Generics::Values
    {
    public:
      void
      fill_values(const Stats& stats) /*throw(eh::Exception)*/;

    protected:
      virtual
      ~ExpressionMatcherStatsImpl() noexcept = default;
    };

    typedef ExpressionMatcherStatsImpl ProcStatImpl;
    typedef ReferenceCounting::SmartPtr<ProcStatImpl>
      ProcStatImpl_var;
  }
}

//
// Inlines impl's
//
namespace AdServer
{
  namespace RequestInfoSvcs
  {
    inline
    Stats::Stats() noexcept
      : processed_matches_optin_user(0),
        processed_matches_temporary_user(0),
        processed_matches_non_optin_user(0)
    {}

    inline void
    StatsCounters::inc_not_optedin_user_processed() noexcept
    {
      processed_matches_optin_user += 1;
    }

    inline void
    StatsCounters::inc_temporary_user_processed() noexcept
    {
      processed_matches_temporary_user += 1;
    }

    inline void
    StatsCounters::inc_persistent_user_processed() noexcept
    {
      processed_matches_non_optin_user += 1;
    }

    inline void
    StatsCounters::set_last_processed_timestamp(const Generics::Time& time)
      noexcept
    {
      last_processed_file_timestamp = time;
    }

    inline const Stats&
    StatsCounters::get_stats() const noexcept
    {
      return *this;
    }
  }
}

#endif // EXPRESSION_MATCHER_STATS_INCLUDED
