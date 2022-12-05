
#ifndef _UNITTEST__TAGSTEST_
#define _UNITTEST__TAGSTEST_

#include <tests/AutoTests/Commons/Common.hpp>

typedef AutoTest::NSLookupRequest  NSLookupRequest;
typedef AutoTest::AdClient AdClient;

/**
* @class TagsTest
* @brief Test for checking tag pricing logic
*/

class TagsTest: 
  public BaseUnit
{
public:
 
  TagsTest(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var): 
    BaseUnit(stat_var, task_name, params_var)  {};
 
  virtual ~TagsTest() noexcept
  {};

private:
 
  virtual bool run_test();

  void set_up();

  // test cases
  void empty_country_case();
  void country_ru_case();
  void country_us_case();
  void unknown_country_case();
  void cpm_filter_case();
  
  NSLookupRequest request;
  std::string tag_id1;
  std::string tag_id2;
  std::string tag_id3;
  
  std::string siterate_id1;
  std::string siterate_id2;
  std::string siterate_id5;
  std::string siterate_id_def;

  std::string cc_id1;
  std::string cc_id2;
  std::string cc_id3;
  std::string cc_id4;
  std::string cc_id5;
  std::string cc_id6;
 
};

#endif  // _UNITTEST__TAGSTEST_
