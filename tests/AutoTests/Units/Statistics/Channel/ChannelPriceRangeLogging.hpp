#ifndef _UNITTEST__CHANNELPRICERANGELOGGING_
#define _UNITTEST__CHANNELPRICERANGELOGGING_

#include <tests/AutoTests/Commons/Common.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>

#include "UserSet.hpp"

namespace ORM = AutoTest::ORM;
namespace DB = AutoTest::DBC;

class ChannelPriceRangeLogging: public BaseDBUnit
{

  typedef AutoTest::AdClient AdClient;
  typedef ORM::ChannelInventoryByCPMStats Stat;
  typedef ORM::StatsList<Stat> Stats;
  typedef Stat::Diffs Diff;
  typedef std::list<Diff> Diffs;

public:
  
  struct UserRequest
  {
    const char* referer_kws;
    const char* tid;
    const char* country;
    const char* colo;
    const char* expected_channels;
    const char* expected_ccids;
  };

  struct StatKey
  {
    AutoTest::Time sdate;
    unsigned long colo_id;
    unsigned long channel_id;
    unsigned long creative_size;
    const char* country_code;
    double ecpm;
    int user_count;
    int impops;
  };
  
  
public:

  ChannelPriceRangeLogging(
    UnitStat& stat_var, 
    const char* task_name, 
    XsdParams params_var);
  
  virtual ~ChannelPriceRangeLogging() noexcept
  { }
 
private:
  AutoTest::Time today;
  unsigned long creative_size_id;
  unsigned long default_colo_;

  virtual bool run();

  virtual void tear_down();

  // cases

  void
  one_ecpm_group();

  void
  tag_cpm_part_1(
    AdClient& client);

  void
  tag_cpm_part_2(
    AdClient& client);
  
  void
  competitive_ecpm_groups(
    AutoTest::Statistics::UserSet& clients);
  
  void
  move_between_ecpm_groups(
    unsigned int index,
    AutoTest::Statistics::UserSet& clients);
  
  void
  new_day(
    AutoTest::Statistics::UserSet& clients);
  
  void
  day_switching_part_1(
    AdClient& client);

  void
  day_switching_part_2(
    AdClient& client);

  void
  day_switching_part_3(
    AdClient& client);

  void key_variation();

  void
  no_impression();

  void
  currency();

  void
  text_advertising();

  void
  tag_adjustment();


  // utils
  template<size_t Count>
  void
  process_requests(
    const UserRequest(&requests)[Count]);

  template<size_t Count>
  void
  add_stats(
    const StatKey(&stats)[Count],
    Stats& stats_container,
    Diffs& diffs_container);
    
};

#endif
