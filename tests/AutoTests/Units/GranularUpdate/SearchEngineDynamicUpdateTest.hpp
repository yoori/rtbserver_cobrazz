#ifndef _AUTOTEST__SEARCHENGINEDYNAMICUPDATETEST_
#define _AUTOTEST__SEARCHENGINEDYNAMICUPDATETEST_
 
#include <tests/AutoTests/Commons/Common.hpp>

namespace ORM = ::AutoTest::ORM;

 
class SearchEngineDynamicUpdateTest: public BaseDBUnit
{
  typedef AutoTest::SearchEngineChecker::Expected Expected;
  typedef Expected& (Expected::*ExpectedSetter)(const std::string&);
  
public:

  SearchEngineDynamicUpdateTest(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var):
    BaseDBUnit(stat_var, task_name, params_var)
  {};
 
  virtual ~SearchEngineDynamicUpdateTest() noexcept
  {};
 
private:


  void add_scenario();

  template<typename AdminField, typename ValueType>
  void
  update_scenario(
    const std::string& description,
    unsigned long engine_id,
    AdminField admin_field,
    ExpectedSetter diff_setter,
    const ValueType& old_value,
    const ValueType& new_value);
  
  void delete_scenario();
  
  bool run();
  
  void set_up();

  void tear_down();
 
};

#endif //_AUTOTEST__SEARCHENGINEDYNAMICUPDATETEST_

