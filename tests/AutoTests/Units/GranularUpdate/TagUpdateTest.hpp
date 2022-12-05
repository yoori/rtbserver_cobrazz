#ifndef AUTOTESTS_UNITS_TAGUPDATETEST_HPP
#define AUTOTESTS_UNITS_TAGUPDATETEST_HPP

#include <tests/AutoTests/Commons/Common.hpp>

/**
 * @class TagUpdateTest
 * @brief Test for tag updates
 */ 
class TagUpdateTest: public BaseDBUnit
{
public:
  TagUpdateTest(
    UnitStat& stat_var, 
    const char* task_name, 
    XsdParams params_var)
    : BaseDBUnit(stat_var, task_name, params_var)
  {};
 
  virtual ~TagUpdateTest() noexcept
  {};
 
private:

  unsigned long other_size_type_;
  unsigned int site_id_;
  unsigned int size_468x60_id_;
  std::string size_468x60_name_;
  unsigned int default_template_;

  void set_up();
  void tear_down();
  bool run();

  void tag_create_case();
  void tag_update_creative_category_exclusion_case();
  void tag_update_size_case();
  void tag_remove_case();
};

#endif /*AUTOTESTS_UNITS_TAGUPDATETEST_HPP*/
