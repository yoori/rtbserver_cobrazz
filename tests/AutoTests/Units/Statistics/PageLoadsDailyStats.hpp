#ifndef _AUTOTEST__PAGELOADSDAILYSTATS_
#define _AUTOTEST__PAGELOADSDAILYSTATS_
  
#include <tests/AutoTests/Commons/Common.hpp>
  
namespace ORM = AutoTest::ORM;

class PageLoadsDailyStats : public BaseDBUnit
{

  typedef AutoTest::AdClient AdClient;
  
  struct TagGroupExpected
  {
    const char* site;
    const char* tag_group;
    int page_loads;
    int utilized_page_loads;
  };
  
public:
  PageLoadsDailyStats(
    UnitStat& stat_var,
    const char* task_name,
    XsdParams params_var) :
    BaseDBUnit(stat_var, task_name, params_var),
    pq_conn_(open_pq())
  {};

  virtual ~PageLoadsDailyStats() noexcept
  {};

private:

  AutoTest::DBC::Conn pq_conn_;
  
  virtual bool run();
  virtual void tear_down() {}

  template <size_t Count>
  void
  initialize_stats(
    const std::string& description,
    const TagGroupExpected (&expected)[Count]);

  std::string
  fetch_tag_group(
    const char* tags);
  
  void case01_multiple_tags_in_one_domain();
  void case02_multiple_tags_iframe_eq_page();
  void case03_multiple_tags_iframe_noteq_page();
  void case04_multiple_tags_iframes();
  void case05_merging_without_page_id();
  void case06_merging_on_the_same_tag();
  void case07_different_page_id_equal_referrer_time();
  void case08_equal_referrers_time_exceeds_2seconds();
  void case09_no_referrer_and_page_id();
  void case10_unconfirmed_impressions();
  void case11_different_sites();
  void case12_different_countries();
  void case13_inventory_mode_tags();
  void case14_reverse_logs_delivery_order_part_1(
    AdClient& client);
  void case14_reverse_logs_delivery_order_part_2(
    AdClient& client);
  void case15_user_statuses();
  
  ORM::StatsList<ORM::PageLoadsDaily> stats;
  std::list<ORM::PageLoadsDaily::Diffs> diffs;
  Generics::Time debug_time;
};

#endif // _AUTOTEST__PAGELOADSDAILYSTATS_
