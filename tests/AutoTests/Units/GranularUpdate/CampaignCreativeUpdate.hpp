#ifndef AUTOTESTS_UNITS_GRANULARUPDATE_CAMPAIGNCREATIVEUPDATE
#define AUTOTESTS_UNITS_GRANULARUPDATE_CAMPAIGNCREATIVEUPDATE

#include <tests/AutoTests/Commons/Common.hpp>
 
namespace ORM = ::AutoTest::ORM;

/**
 * @class CampaignCreativeUpdate
 * @brief Test for creative updates
 */
class CampaignCreativeUpdate: public BaseDBUnit
{
public:
  CampaignCreativeUpdate(
    UnitStat& stat_var, 
    const char* task_name, 
    XsdParams params_var)
    : BaseDBUnit(stat_var, task_name, params_var)
  {};

  virtual ~CampaignCreativeUpdate() noexcept
  {};

private:
  
  void set_up();
  void tear_down();
  bool run();

  void add_creative_();
  void remove_creative_();
  void update_creative_();
  void update_option_value_();

};

#endif /*AUTOTESTS_UNITS_GRANULARUPDATE_CAMPAIGNCREATIVEUPDATE*/
