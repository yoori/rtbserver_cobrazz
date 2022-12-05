#ifndef __AUTOTESTS_COMMONS_CHECKER_STATSDIFFCHECKER_HPP
#define __AUTOTESTS_COMMONS_CHECKER_STATSDIFFCHECKER_HPP

#include "Checker.hpp"

namespace AutoTest
{
  /**
   * @class StatsDiffChecker
   * Checks that initial[i] + diff[i] == real[i] for all i
   */
  template<typename StatsType, typename StatsDiffType>
  class StatsDiffChecker: public Checker
  {
  public:
    StatsDiffChecker(
      DBC::IConn& connection,
      const StatsDiffType& diff,
      const StatsType& initial);

    virtual ~StatsDiffChecker() noexcept;

    bool
    check(
      bool throw_error = true)
      /*throw(CheckFailed, eh::Exception)*/;

  protected:
    DBC::IConn& connection_;
    StatsDiffType diff_;
    StatsType initial_; 
    StatsType real_;
  };

  /**
   * @class StatsEachDiffChecker
   * Checks that initial[i] + diff == real[i] for all i
   */
  template<typename StatsType, typename StatsDiffType>
  class StatsEachDiffChecker: public Checker
  {
  public:
    StatsEachDiffChecker(
      DBC::IConn& connection,
      const StatsDiffType& diff,
      const StatsType& initial);

    virtual ~StatsEachDiffChecker() noexcept;

    bool
    check(
      bool throw_error = true)
      /*throw(CheckFailed, eh::Exception)*/;

  protected:
    DBC::IConn& connection_;
    StatsDiffType diff_;
    StatsType initial_;
    StatsType real_;
  };

  // helper functions
  template<typename StatsType, typename StatsDiffType>
  StatsDiffChecker<StatsType, StatsDiffType>
  stats_diff_checker(
    DBC::IConn& connection,
    const StatsDiffType& diff,
    const StatsType& initial);

  template<typename StatsType, typename Diff>
  StatsDiffChecker<StatsType, std::list<Diff> >
  stats_diff_checker(
    DBC::IConn& connection,
    const std::initializer_list<Diff>& diff,
    const StatsType& initial);

  template<typename StatsType, typename StatsDiffType>
  StatsEachDiffChecker<StatsType, StatsDiffType>
  stats_each_diff_checker(
    DBC::IConn& connection,
    const StatsDiffType& diff,
    const StatsType& initial);
} //namespace AutoTest

#include "StatsDiffChecker.tpp"

#endif  // __AUTOTESTS_COMMONS_CHECKER_STATSDIFFCHECKER_HPP
