
#ifndef _AUTOTEST__TAGSANDCREATIVESSIZEMATCHING_
#define _AUTOTEST__TAGSANDCREATIVESSIZEMATCHING_
  
#include <tests/AutoTests/Commons/Common.hpp>

class TagsAndCreativesSizeMatching : public BaseUnit
{
public:
  TagsAndCreativesSizeMatching(
    UnitStat& stat_var,
    const char* task_name,
    XsdParams params_var) :
    BaseUnit(stat_var, task_name, params_var)
  { }

  virtual ~TagsAndCreativesSizeMatching() noexcept
  { }

private:

  virtual bool run_test();

  // Cases
  void same_type_diff_sizes();
  void actual_size();
  void diff_sizes();
  void max_ads();
  void zero_max_ads();
  void multi_creatives_ccg();
  void display_vs_text();
  void size_type_level();
  void rtb();
};

#endif // _AUTOTEST__TAGSANDCREATIVESSIZEMATCHING_
