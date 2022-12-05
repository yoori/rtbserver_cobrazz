#ifndef _AUTOTEST__CHANNELIMPINVENTORYTEST_
#define _AUTOTEST__CHANNELIMPINVENTORYTEST_
 
#include <tests/AutoTests/Commons/Common.hpp>

namespace ORM = ::AutoTest::ORM;

class ChannelImpInventoryTest: public BaseDBUnit
{
  typedef std::vector<std::string> StringSeq;
  typedef std::vector<int> IntSeq;

  
public:
  ChannelImpInventoryTest(
    UnitStat& stat_var, 
    const char* task_name, 
    XsdParams params_var) :
    BaseDBUnit(stat_var, task_name, params_var)
  { }
 
  virtual ~ChannelImpInventoryTest() noexcept
  { }

protected:

  virtual bool run();

  virtual void set_up();
  
  virtual void tear_down();

private:

  enum ACTIONS2SEND
  {
    SEND_NO_ACTIONS = 0,
    SEND_TID = 1,
    SEND_DEFAULT = SEND_TID,
    SEND_FORMAT = 2,
    SEND_TRACK = 4,
    SEND_CLICK = 8,
    SEND_ACTION = 16,
    SEND_ALL_ACTIONS = 31,
    SEND_CLICKS = 32,
    SEND_NO_COOKIES = 64,
    SEND_OO_USER = 128,
    SEND_REFERER = 256
  };
  
  void
  do_case_requests_(
    const std::string& prefix,
    int flags = SEND_DEFAULT);

  unsigned long
  fetch_colo_(
    const std::string& prefix);

  void
  fetch_list_(
    StringSeq& seq, 
    const std::string& prefix);

  void
  fetch_list_(
    IntSeq& seq, 
    const std::string& prefix);
  
  void
  init_(
    ORM::StatsList<ORM::ChannelImpInventory>& stats,
    std::list<ORM::ChannelImpInventory::Diffs>& diffs,
    const std::string& prefix,
    size_t size);
  
  void
  check_cretives_(
    const std::string& prefix,
    AutoTest::AdClient& client);
  
  void
  check_channels_(
    const std::string& prefix,
    AutoTest::AdClient& client);

  void
  test_case(
    const char* prefix,
    size_t size,
    int flags = SEND_DEFAULT);
  
  void
  colo_case();
  
  unsigned int default_colo_;
  size_t count_;
};

#endif //_AUTOTEST__CHANNELIMPINVENTORYTEST_

