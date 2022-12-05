#ifndef AD_SERVER_LOG_PROCESSING_PROC_STAT_IMPL_HPP
#define AD_SERVER_LOG_PROCESSING_PROC_STAT_IMPL_HPP


#include <string>

#include <ReferenceCounting/ReferenceCounting.hpp>
#include <CORBACommons/StatsImpl.hpp>
#include <Sync/SyncPolicy.hpp>
#include <Generics/Time.hpp>

namespace AdServer {
namespace LogProcessing {

class ProcStatImpl : public Generics::Values
{
  virtual
  ~ProcStatImpl() noexcept
  {}

  typedef const Floating& (*FloatFunction)(const Floating&,
    const Floating&);
  typedef const UnsignedInt& (*UIntFunction)(const UnsignedInt&,
    const UnsignedInt&);

public:
  void
  set_if_gt_time(const Key &key, const Generics::Time &value)
  {
    double time = value.as_double();
    Sync::PosixGuard guard(mutex_);
    func_or_set_<FloatFunction>(key, time, std::max);
  }

  template <typename ProcStatsValues>
  void
  reset()
  {
    double initial_time = Generics::Time().as_double();
    Sync::PosixGuard guard(mutex_);
    ProcStatsValues::total_dump_elapsed_time_ = Generics::Time();
    set_(ProcStatsValues::input_time_avg, initial_time);
    set_(ProcStatsValues::input_time_max, initial_time);
    set_(ProcStatsValues::input_time_total, initial_time);
    set_(ProcStatsValues::input_time_per_file_avg, initial_time);
    set_(ProcStatsValues::input_time_per_file_max, initial_time);
    set_<UnsignedInt>(ProcStatsValues::input_success_count, 0);
    set_<UnsignedInt>(ProcStatsValues::input_error_count, 0);
    set_(ProcStatsValues::output_time_avg, initial_time);
    set_(ProcStatsValues::output_time_max, initial_time);
    set_(ProcStatsValues::output_time_total, initial_time);
    set_(ProcStatsValues::output_records_avg, 0.);
    set_<UnsignedInt>(ProcStatsValues::output_records_max, 0);
    set_<UnsignedInt>(ProcStatsValues::output_records_total, 0);
    set_<UnsignedInt>(ProcStatsValues::output_success_count, 0);
    set_<UnsignedInt>(ProcStatsValues::output_error_count, 0);
  }

  template <typename ProcStatsValues>
  void
  stat_num_entries(Generics::Timer& dump_timer, size_t num_entries)
  {
    dump_timer.stop();
    Generics::Time elapsed_time = dump_timer.elapsed_time();
    double elapsed = elapsed_time.as_double();

    Sync::PosixGuard guard(mutex_);
    ProcStatsValues::total_dump_elapsed_time_ += elapsed_time;
    double total_elapsed =
      ProcStatsValues::total_dump_elapsed_time_.as_double();

    UnsignedInt total_dumps_count =
      func_or_set_(ProcStatsValues::output_success_count,
        1ul, std::plus<UnsignedInt>());

    set_(ProcStatsValues::output_time_avg,
      total_elapsed / total_dumps_count);

    func_or_set_<FloatFunction>(
      ProcStatsValues::output_time_max, elapsed, std::max);

    set_(ProcStatsValues::output_time_total, total_elapsed);

    UnsignedInt total_entries_count =
      func_or_set_(ProcStatsValues::output_records_total, num_entries,
        std::plus<UnsignedInt>());

    set_(ProcStatsValues::output_records_avg,
      double(total_entries_count) / total_dumps_count);

    func_or_set_<UIntFunction>(
      ProcStatsValues::output_records_max, num_entries, std::max);
  }

  template <typename ProcStatsValues>
  void
  stat_processed_files(
    Generics::Time& total_elapsed_time,
    const Generics::Time& total_file_proc_elapsed_time,
    const Generics::Time& start_time,
    unsigned& num_input_ops,
    size_t processed_files_count)
  {
    Generics::Time elapsed_time =
      Generics::Time::get_time_of_day() - start_time;
    total_elapsed_time += elapsed_time;
    double elapsed = elapsed_time.as_double();
    double total_elapsed = total_elapsed_time.as_double();
    double total_file_proc_elapsed = total_file_proc_elapsed_time.as_double();

    Sync::PosixGuard guard(mutex_);
    UnsignedInt total_processed_files_count =
      func_or_set_(ProcStatsValues::input_success_count,
        processed_files_count, std::plus<UnsignedInt>());

    set_(ProcStatsValues::input_time_avg,
      total_elapsed / ++num_input_ops);

    set_(ProcStatsValues::input_time_per_file_avg,
      total_file_proc_elapsed / total_processed_files_count);

    func_or_set_<FloatFunction>(ProcStatsValues::input_time_max, elapsed,
      std::max);

    set_(ProcStatsValues::input_time_total, total_elapsed);
  }
};
typedef ::ReferenceCounting::SmartPtr<ProcStatImpl>
  ProcStatImpl_var;

template <class LOG_TYPE_TRAITS_>
class ProcStatsValues
{
  friend class ProcStatImpl;

  typedef Generics::Values::Key Key;

  static Generics::Time total_dump_elapsed_time_;

  static const Key input_time_avg;
  static const Key input_time_max;
  static const Key input_time_total;

  static const Key input_time_per_file_avg;

  static const Key input_success_count;

  static const Key output_time_avg;
  static const Key output_time_max;
  static const Key output_time_total;

  static const Key output_records_avg;
  static const Key output_records_max;
  static const Key output_records_total;

  static const Key output_success_count;

  ProcStatImpl_var process_stats_values_;

public:

  static const Key input_time_per_file_max;
  static const Key input_error_count;
  static const Key output_error_count;

  ProcStatsValues(const ProcStatImpl_var& process_stats_values)
    : process_stats_values_(process_stats_values)
  {
    reset();
  }

  void
  reset()
  {
    process_stats_values_->reset<ProcStatsValues<LOG_TYPE_TRAITS_> >();
  }

  ProcStatImpl*
  operator ->() noexcept
  {
    return process_stats_values_.in();
  }

  void
  stat_num_entries(Generics::Timer& dump_timer, size_t num_entries)
  {
    process_stats_values_->
      stat_num_entries<ProcStatsValues<LOG_TYPE_TRAITS_> >(
        dump_timer, num_entries);
  }

};

template <class LogTraits>
Generics::Time ProcStatsValues<LogTraits>::total_dump_elapsed_time_;

template <class L_T_T_> const typename ProcStatsValues<L_T_T_>::Key
  ProcStatsValues<L_T_T_>::input_time_avg =
    L_T_T_::snmp_friendly_name() + ".inputTimeAvg";

template <class L_T_T_> const typename ProcStatsValues<L_T_T_>::Key
  ProcStatsValues<L_T_T_>::input_time_max =
    L_T_T_::snmp_friendly_name() + ".inputTimeMax";

template <class L_T_T_> const typename ProcStatsValues<L_T_T_>::Key
  ProcStatsValues<L_T_T_>::input_time_total =
    L_T_T_::snmp_friendly_name() + ".inputTimeTotal";

template <class L_T_T_> const typename ProcStatsValues<L_T_T_>::Key
  ProcStatsValues<L_T_T_>::input_time_per_file_avg =
    L_T_T_::snmp_friendly_name() + ".inputTimePerFileAvg";

template <class L_T_T_> const typename ProcStatsValues<L_T_T_>::Key
  ProcStatsValues<L_T_T_>::input_time_per_file_max =
    L_T_T_::snmp_friendly_name() + ".inputTimePerFileMax";

template <class L_T_T_> const typename ProcStatsValues<L_T_T_>::Key
  ProcStatsValues<L_T_T_>::input_success_count =
    L_T_T_::snmp_friendly_name() + ".inputSuccessCount";

template <class L_T_T_> const typename ProcStatsValues<L_T_T_>::Key
  ProcStatsValues<L_T_T_>::input_error_count =
    L_T_T_::snmp_friendly_name() + ".inputErrorCount";

template <class L_T_T_> const typename ProcStatsValues<L_T_T_>::Key
  ProcStatsValues<L_T_T_>::output_time_avg =
    L_T_T_::snmp_friendly_name() + ".outputTimeAvg";

template <class L_T_T_> const typename ProcStatsValues<L_T_T_>::Key
  ProcStatsValues<L_T_T_>::output_time_max =
    L_T_T_::snmp_friendly_name() + ".outputTimeMax";

template <class L_T_T_> const typename ProcStatsValues<L_T_T_>::Key
  ProcStatsValues<L_T_T_>::output_time_total =
    L_T_T_::snmp_friendly_name() + ".outputTimeTotal";

template <class L_T_T_> const typename ProcStatsValues<L_T_T_>::Key
  ProcStatsValues<L_T_T_>::output_records_avg =
    L_T_T_::snmp_friendly_name() + ".outputRecordsAvg";

template <class L_T_T_> const typename ProcStatsValues<L_T_T_>::Key
  ProcStatsValues<L_T_T_>::output_records_max =
    L_T_T_::snmp_friendly_name() + ".outputRecordsMax";

template <class L_T_T_> const typename ProcStatsValues<L_T_T_>::Key
  ProcStatsValues<L_T_T_>::output_records_total =
    L_T_T_::snmp_friendly_name() + ".outputRecordsTotal";

template <class L_T_T_> const typename ProcStatsValues<L_T_T_>::Key
  ProcStatsValues<L_T_T_>::output_success_count =
    L_T_T_::snmp_friendly_name() + ".outputSuccessCount";

template <class L_T_T_> const typename ProcStatsValues<L_T_T_>::Key
  ProcStatsValues<L_T_T_>::output_error_count =
    L_T_T_::snmp_friendly_name() + ".outputErrorCount";

} // namespace LogProcessing
} // namespace AdServer

#endif /* AD_SERVER_LOG_PROCESSING_PROC_STAT_IMPL_HPP */
