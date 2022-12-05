#ifndef _AUTOTEST__GEOCHANNELSSTATS_
#define _AUTOTEST__GEOCHANNELSSTATS_
  
#include <tests/AutoTests/Commons/Common.hpp>

namespace DB = AutoTest::DBC;

class GEOChannelsStats : public BaseUnit
{
public:
  typedef AutoTest::AdClient AdClient;
  
public:
  GEOChannelsStats(
    UnitStat& stat_var,
    const char* task_name,
    XsdParams params_var) :
    BaseUnit(stat_var, task_name, params_var),
    pq_conn_(open_pq())
  { }

  virtual ~GEOChannelsStats() noexcept
  { }

private:

  virtual bool run_test();

  void make_requests();
  void make_location_request(
    AdClient& client,
    const char* tid,
    const std::string& location,
    const char* expected_ccids,
    unsigned short flags = 0);

  AutoTest::Time testtime;
  DB::Conn pq_conn_;
};

#endif // _AUTOTEST__GEOCHANNELSSTATS_
