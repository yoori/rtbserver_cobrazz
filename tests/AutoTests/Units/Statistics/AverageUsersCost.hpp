#ifndef _AUTOTEST__AVERAGEUSERSCOST_
#define _AUTOTEST__AVERAGEUSERSCOST_
  
#include <tests/AutoTests/Commons/Common.hpp>

class AverageUsersCost : public BaseUnit
{
public:
  AverageUsersCost(
    UnitStat& stat_var,
    const char* task_name,
    XsdParams params_var) :
    BaseUnit(stat_var, task_name, params_var),
    pq_conn_(open_pq())
  {};

  virtual ~AverageUsersCost() noexcept
  {};

  struct UserRequest
  {
    const char* tag;
    const char* ccid;
    const char* revenue;
    bool track_imp;
    unsigned char actions;
  };

private:

  AutoTest::DBC::Conn pq_conn_;
  AutoTest::Time today_;

  std::string request_keyword_;
  unsigned long request_channel_;
  unsigned long request_k_channel_;
  unsigned long default_colo_;

  std::list<std::string> uids_;

  void
  log_profile(
    std::string uid);

  template<size_t RequestsCount>
  double
  test_case(
    const std::string& description,
    unsigned long test_channel,
    const char* test_keyword,
    const UserRequest(&requests) [RequestsCount],
    unsigned long flags = 0);

  virtual
  bool
  run_test();

};

#endif // _AUTOTEST__AVERAGEUSERSCOST_
