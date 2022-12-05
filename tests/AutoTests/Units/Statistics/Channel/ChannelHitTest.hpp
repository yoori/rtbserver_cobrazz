#ifndef AUTOTESTS_CHANNELHITTEST_H
#define AUTOTESTS_CHANNELHITTEST_H

#include <tests/AutoTests/Commons/Common.hpp>


class ChannelHitTest : public BaseDBUnit
{

  typedef AutoTest::ORM::ChannelInventoryStats Stat;
  typedef AutoTest::ORM::StatsList<Stat> Stats;
  typedef AutoTest::ORM::ChannelInventoryStats::Diffs Diff;
  typedef std::list<Diff> Diffs;

public:
  ChannelHitTest(UnitStat& stat_var, const char* task_name,
            XsdParams params_var)
  : BaseDBUnit(stat_var, task_name, params_var),
  default_colo_(fetch_int("DefaultColo")),
  base_time_()
  {}

  virtual ~ChannelHitTest() noexcept
  {}

private:

  const unsigned long default_colo_;
  AutoTest::Time base_time_;

  Stats stats_;
  Diffs diffs_;

  bool run();
  void tear_down();
  void run_case(const AutoTest::Time& date,
    unsigned long colo = 0);
};

#endif // AUTOTESTS_CHANNELHITTEST_H
