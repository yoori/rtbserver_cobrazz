
#ifndef __COLOCATIONGRANULARUPDATETEST_HPP
#define __COLOCATIONGRANULARUPDATETEST_HPP

#include <tests/AutoTests/Commons/Common.hpp>

namespace ORM = ::AutoTest::ORM;


/**
* @class ColocationGranularUpdateTest
* @brief Test for colocation granular update
*/ 
 
class ColocationGranularUpdateTest:
  public BaseDBUnit
{
public:
 
  ColocationGranularUpdateTest(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var):
    BaseDBUnit(stat_var, task_name, params_var),
    colo_(create<ORM::RatedColocation>())
  {};
 
  virtual ~ColocationGranularUpdateTest() noexcept
  {};
 
  void set_up    ();
  void tear_down ();
  bool run();

private:

  ORM::ORMRestorer<ORM::RatedColocation>* colo_;

  void add_colocation();
  void add_colo_rate();
  void update_colo_account();
  void deactivate_colo();
};

#endif  // __COLOCATIONGRANULARUPDATETEST_HPP
