
#ifndef _AUTOTEST__ZEROCOLLECTORTEST_
#define _AUTOTEST__ZEROCOLLECTORTEST_
  
#include <tests/AutoTests/Commons/Common.hpp>
  

class ZeroCollectorTest :  public BaseDBUnit
{
public:
  typedef AutoTest::ORM::PQ::Channel Channel;
  typedef AutoTest::ORM::PQ::Channeltrigger ChannelTrigger;
  typedef AutoTest::ORM::PQ::Channelrate Rate;
  typedef std::list<Channel> ChannelList;
  typedef std::list<ChannelTrigger> TriggerList;

 
public:
  ZeroCollectorTest(
    UnitStat& stat_var,
    const char* task_name,
    XsdParams params_var) :
    BaseDBUnit(stat_var, task_name, params_var)
  { }

  virtual ~ZeroCollectorTest() noexcept
  { }

protected:

  void set_up();
  bool run();
  void tear_down();
  
private:
  ChannelList channels;
  TriggerList triggers;
  std::list<int> active_channels;
  std::list<int> wait_channels;
};

#endif // _AUTOTEST__ZEROCOLLECTORTEST_
