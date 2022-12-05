#ifndef AUTOTESTS_COMMONS_CHECKER_STATSDIFFCHECKER_IPP
#define AUTOTESTS_COMMONS_CHECKER_STATSDIFFCHECKER_IPP

#include <tests/AutoTests/Commons/Stats/ORMStats.hpp>

namespace AutoTest
{
  // StatsDiffChecker
  template<typename StatsType, typename StatsDiffType>
  StatsDiffChecker<StatsType, StatsDiffType>::StatsDiffChecker(
    DBC::IConn& connection,
    const StatsDiffType& diff,
    const StatsType& initial)
    : connection_(connection),
      diff_(diff),
      initial_(initial),
      real_(initial)
  {}
  
  template<typename StatsType, typename StatsDiffType>
  StatsDiffChecker<StatsType, StatsDiffType>::~StatsDiffChecker()
    noexcept
  {}
  
  template<typename StatsType, typename StatsDiffType>
  bool
  StatsDiffChecker<StatsType, StatsDiffType>::check(
    bool throw_error) /*throw (CheckFailed, eh::Exception)*/
  {
    real_.select(connection_, false);
    int at = check_difference(real_, initial_, diff_);

    // Debug
    std::ostringstream out;

    out << "changes: at(" << at << ") ";
    out << '{';
    StatsType::print_diff(out, diff_);
    out << '}' << std::endl;
    initial_.print_diff(out, diff_, real_);
    out << std::endl;

    Logger::thlog().debug_trace(out.str());

    if(at == -1)
    {
      return true;
    }

    if(throw_error)
    {
      Stream::Error error;
      error << "can't get expected changes: at(" << at << ") ";
      error << '{';
      StatsType::print_diff(error, diff_);
      error << '}' << std::endl;
      initial_.print_diff(error, diff_, real_);
      error << std::endl;
      throw CheckFailed(error);
    }

    return false;
  }

  template<typename StatsType, typename StatsDiffType>
  StatsDiffChecker<StatsType, StatsDiffType>
  stats_diff_checker(
    DBC::IConn& connection,
    const StatsDiffType& diff,
    const StatsType& initial)
  {
    return StatsDiffChecker<StatsType, StatsDiffType>(
      connection, diff, initial);
  }

  template<typename StatsType, typename Diff>
  StatsDiffChecker<StatsType, std::list<Diff> >
  stats_diff_checker(
    DBC::IConn& connection,
    const std::initializer_list<Diff>& diff,
    const StatsType& initial)
  {
    std::list<Diff> diff_(diff);
    
    return
      StatsDiffChecker<StatsType, std::list<Diff> >(
        connection, diff_, initial);
  }

  // StatsEachDiffChecker
  
  template<typename StatsType, typename StatsDiffType>
  StatsEachDiffChecker<StatsType, StatsDiffType>::StatsEachDiffChecker(
    DBC::IConn& connection,
    const StatsDiffType& diff,
    const StatsType& initial)
    : connection_(connection),
      diff_(diff),
      initial_(initial),
      real_(initial)
  { }
  
  template<typename StatsType, typename StatsDiffType>
  StatsEachDiffChecker<StatsType, StatsDiffType>::~StatsEachDiffChecker() noexcept
  { }
   
  template<typename StatsType, typename StatsDiffType>
  bool
  StatsEachDiffChecker<StatsType, StatsDiffType>::check(
    bool throw_error) /*throw (CheckFailed, eh::Exception)*/
  {
    real_.select(connection_, false);
    int at = check_each_difference(real_, initial_, diff_);

    // Debug
    std::ostringstream out;

    out << "changes: at(" << at << ") ";
    out << '{';
    StatsType::print_each_diff(out, diff_);
    out << '}' << std::endl;
    initial_.print_each_diff(out, diff_, real_);
    out << std::endl;

    Logger::thlog().debug_trace(out.str());

    if(at == -1)
    {
      return true;
    }

    if(throw_error)
    {
      Stream::Error error;
      error << "can't get expected changes: at(" << at << ") ";
      error << '{';
      StatsType::print_each_diff(error, diff_);
      error << '}' << std::endl;
      initial_.print_each_diff(error, diff_, real_);
      error << std::endl;
      throw CheckFailed(error);
    }

    return false;
  }

  template<typename StatsType, typename StatsDiffType>
  StatsEachDiffChecker<StatsType, StatsDiffType>
  stats_each_diff_checker(
    DBC::IConn& connection,
    const StatsDiffType& diff,
    const StatsType& initial)
  {
    return StatsEachDiffChecker<StatsType, StatsDiffType>(
      connection, diff, initial);
  }
}

#endif /*AUTOTESTS_COMMONS_CHECKER_STATSDIFFCHECKER_IPP*/
