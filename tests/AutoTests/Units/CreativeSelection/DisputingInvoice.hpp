
#ifndef _AUTOTEST__DISPUTINGINVOICE_
#define _AUTOTEST__DISPUTINGINVOICE_
  
#include <tests/AutoTests/Commons/Common.hpp>

namespace ORM = ::AutoTest::ORM;
namespace DB  = ::AutoTest::DBC;

class DisputingInvoice : public BaseDBUnit
{
public:
  DisputingInvoice(
    UnitStat& stat_var,
    const char* task_name,
    XsdParams params_var) :
    BaseDBUnit(stat_var, task_name, params_var)
  { }

  virtual ~DisputingInvoice() noexcept
  { }

private:

  int account_;
  int ccgid_;
  int ccid_;
  unsigned long tid_;
  std::string keyword_;
  ORM::HourlyStats stat;

  void
  set_up();

  void
  tear_down();

  bool
  run();

  // Cases
  void
  create_invoice_(
    ORM::PQ::Accountfinancialdata* acc_data);
  
  void
  edit_invoice_(
    ORM::PQ::Accountfinancialdata* acc_data);

  // Utils
  void
  clear_stats_();
  
};

#endif // _AUTOTEST__DISPUTINGINVOICE_

