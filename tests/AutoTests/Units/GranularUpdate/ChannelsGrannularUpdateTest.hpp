#ifndef _UNITTEST__ADDCHANNELSTEST_
#define _UNITTEST__ADDCHANNELSTEST_


#include <tests/AutoTests/Commons/Common.hpp>

namespace DB = ::AutoTest::DBC;
namespace ORM = ::AutoTest::ORM;

/**
 * @class ChannelsGrannularUpdateTest
 * @brief tests dynamic channels loading
 */ 
class ChannelsGrannularUpdateTest
  :public BaseDBUnit
{

public:

  ChannelsGrannularUpdateTest(
      UnitStat& stat_var,
      const char* task_name,
      XsdParams params_var):
    BaseDBUnit(stat_var, task_name, params_var),
    chp(pq_conn_),
    che(pq_conn_),
    chs(pq_conn_)
  { }

  virtual ~ChannelsGrannularUpdateTest() noexcept
  { }

private:
  
  /// Page channel
  ORM::PageChannel chp;

  /// Expression channel
  ORM::ExpressionChannel che;

  /// Search channel
  ORM::SearchChannel chs;

  // Triggers
  std::list<ORM::PQ::Triggers> triggers;


  // ChannelTrigger
  std::list<ORM::PQ::Channeltrigger> channeltriggers;
  
  void
  create_trigger(
    ORM::BehavioralChannel* channel,
    const char* kwd_name);

  void page_channel();
  void search_channel();
  void expression_channel();
  void delete_channel_expression();
  void channel_rate_change();
  
  void set_up();
  bool run();
  void tear_down();
  
  
};
 
#endif
