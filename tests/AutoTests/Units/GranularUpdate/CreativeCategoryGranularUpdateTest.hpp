
#ifndef _UNITTEST__CREATIVECATEGORYGRANULARUPDATETEST_
#define _UNITTEST__CREATIVECATEGORYGRANULARUPDATETEST_

#include <tests/AutoTests/Commons/Common.hpp>

namespace ORM = ::AutoTest::ORM;
 
/**
* @class CreativeCategoryGranularUpdateTest
* @brief Test creative category granular update.
*        Attention for special rules for creative
*        categories with cct_id=2 (Tags)
*/
 
class CreativeCategoryGranularUpdateTest: public BaseDBUnit
{
public:
 
  CreativeCategoryGranularUpdateTest(
    UnitStat& stat_var, 
    const char* task_name, 
    XsdParams params_var):
    BaseDBUnit(stat_var, task_name, params_var)
  {};
 
  virtual ~CreativeCategoryGranularUpdateTest() noexcept
  {};

protected:
  void set_up();
  void tear_down();
  bool run();

private:
  void create_categories_();
  void unlink_category_();
  void add_tags_exclusion_();
  void del_tags_exclusion_();
};

#endif  // _UNITTEST__CREATIVECATEGORYGRANULARUPDATETEST_
