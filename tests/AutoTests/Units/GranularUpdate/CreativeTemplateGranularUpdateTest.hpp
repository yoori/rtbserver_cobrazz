#ifndef _UNITTEST__CREATIVETEMPLATEGRANULARUPDATETEST_
#define _UNITTEST__CREATIVETEMPLATEGRANULARUPDATETEST_

#include <tests/AutoTests/Commons/Common.hpp>

namespace ORM = ::AutoTest::ORM;


/**
* @class CreativeTemplateGranularUpdateTest
* @brief Test for template+files granular update
*/ 
class CreativeTemplateGranularUpdateTest:
  public BaseDBUnit
{
public:
 
  CreativeTemplateGranularUpdateTest(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var):
    BaseDBUnit(stat_var, task_name, params_var)
  {};
 
  virtual ~CreativeTemplateGranularUpdateTest() noexcept
  {};
 
private:

  std::string file1_;
  std::string file2_;
  std::string file3_;
  int app_format_id_;
  std::string format_name_;
  int size_300x250_id_;
  std::string size_300x250_name_;
  int size_468x60_id_;
  std::string size_468x60_name_;

  void set_up    ();
  void tear_down ();
  bool run();

  void testcase_add_template();
  void testcase_change_template();
  void testcase_delete_template();
  void testcase_del_status_template();
};

#endif
