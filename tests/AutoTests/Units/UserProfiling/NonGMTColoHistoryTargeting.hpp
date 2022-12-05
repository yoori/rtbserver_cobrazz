
#ifndef _AUTOTEST__NONGMTCOLOHISTORYTARGETING_
#define _AUTOTEST__NONGMTCOLOHISTORYTARGETING_
  
#include <tests/AutoTests/Commons/Common.hpp>
  

class NonGMTColoHistoryTargeting : public BaseUnit
{
public:

  enum GMTvsTZEnum
  {
    GTE_TZ_MIDNIGHT,
    GTE_SAME_DAY,
    GTE_TZ_HIGH,
    GTE_GMT_HIGH
  };

  struct RequestInfo
  {
    GMTvsTZEnum date_type;
    int time_ofset;
    bool need_referer;
    AutoTest::SequenceCheckerEnum trigger_match;
    AutoTest::SequenceCheckerEnum history_match;
  };
  
public:
  NonGMTColoHistoryTargeting(
    UnitStat& stat_var,
    const char* task_name,
    XsdParams params_var) :
    BaseUnit(stat_var, task_name, params_var)
  {};

  virtual ~NonGMTColoHistoryTargeting() noexcept
  {};

private:

  AutoTest::Time base_time;

  virtual bool run_test();

  // Utils
  Generics::Time
  get_time(
    GMTvsTZEnum date_type,
    const AutoTest::Time& time);

  void check(
    const std::string& description,
    const std::string& prefix,
    const RequestInfo* requests,
    size_t requests_size);
};

#endif // _AUTOTEST__NONGMTCOLOHISTORYTARGETING_
