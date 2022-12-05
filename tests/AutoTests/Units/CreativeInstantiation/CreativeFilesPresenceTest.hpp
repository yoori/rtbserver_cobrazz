#ifndef _UNITTEST__CREATIVEFILESPRESENCETEST_
#define _UNITTEST__CREATIVEFILESPRESENCETEST_

#include <tests/AutoTests/Commons/Common.hpp>
 
class CreativeFilesPresenceTest: public BaseUnit
{
public:
 
  CreativeFilesPresenceTest(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var):
    BaseUnit(stat_var, task_name, params_var)
  {};
 
  virtual ~CreativeFilesPresenceTest() noexcept
  {};
 
private:
 
  virtual bool run_test();
  void file_not_present_case();
  void normal_case();
  void url_not_present_case();
  void file_url_present_case();
  void template_file_not_present_case();
  void w_notblock_a_case();
  void w_notblock_a_size_format_case();
  void text_2campaign_case();
  void text_campaign_case();
  void ADSC_8367(); // 'Dynamic File' type option
};

#endif
