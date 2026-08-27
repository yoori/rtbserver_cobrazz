
#pragma once

#include <tests/AutoTests/Commons/Common.hpp>


class RTBCategoriesMappingTest : public BaseUnit
{

  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::OpenRTBRequest OpenRTBRequest;
  typedef AutoTest::OpenRTBResponseChecker OpenRTBResponseChecker;

public:
  RTBCategoriesMappingTest(UnitStat& stat_var, const char* task_name, XsdParams params_var) :
    BaseUnit(stat_var, task_name, params_var)
  { }

  virtual ~RTBCategoriesMappingTest() noexcept
  { }

  struct RequestBanner
  {
    const char* size;
    const char* battr;
  };

  struct ResponseBid
  {
    const char* cc_id;
    const char* visual_categories;
    const char* content_categories;
  };

  struct AllyesBid
  {
    const char* cc_id;
    unsigned long fmt;
    const char* cat;
  };

  template<size_t BannersCount = 1>
  struct RTBTestCase
  {
    const char* description;
    // Request params
    const char* source;
    RequestBanner banners[BannersCount];
    const char* bcat;
    // Response params
    ResponseBid bids[BannersCount];
  };

  template<size_t BannersCount = 1>
  struct AllyesTestCase
  {
    const char* description;
    // Request params
    const char* source;
    RequestBanner banners[BannersCount];
    const char* bcat;
    // Response params
    AllyesBid bids[BannersCount];
  };

private:


  template<typename Object, typename Param>
  void
  set_category(Object& obj, Param Object::* setter, const std::string& cat);

  template<typename Object, typename RetVal, typename T>
  void
  set_category(Object& obj, RetVal (Object::*setter)(T), const std::string& cat);

  template<typename Object, typename Setter>
  void set_categories(const char* cat_list, Object& obj, Setter setter);

  template<typename Object, typename Setter, typename Touch>
  void set_expected_categories(const char* cat_list, Object& obj, Setter setter, Touch touch);

  template<typename TestCase>
  void
  prepare_request(OpenRTBRequest& request, const TestCase& test_case);

  template<size_t Slots>
  void
  prepare_checker(
    OpenRTBResponseChecker::ExpectedList& expected,
    const RTBTestCase<Slots>& test_case);

  template<size_t Slots>
  void
  prepare_checker(
    OpenRTBResponseChecker::ExpectedList& expected,
    const AllyesTestCase<Slots>& test_case);

  template<typename Traits, typename CaseType, size_t Cases>
  void
  perform_case_(const CaseType (&cases)[Cases]);

  bool run_test();
};
