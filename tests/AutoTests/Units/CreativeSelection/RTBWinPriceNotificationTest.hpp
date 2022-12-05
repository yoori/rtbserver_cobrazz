
#ifndef _AUTOTEST__RTBWINPRICENOTIFICATIONTEST_
#define _AUTOTEST__RTBWINPRICENOTIFICATIONTEST_
  
#include <tests/AutoTests/Commons/Common.hpp>

class RTBWinPriceNotificationTest : public BaseDBUnit
{
  typedef AutoTest::ORM::HourlyStats Stat;
  typedef AutoTest::ORM::StatsList<Stat> Stats;
  typedef Stat::Diffs Diff;
  typedef std::list<Diff> Diffs;

  typedef std::list<std::string> StoredRequests;

  enum CaseFlags
  {
    CF_INST_REQ       = 0x01,
    CF_NURL_REQ       = 0x02,
    CF_STORE_NURL     = 0x04,
    CF_IMPTRACK_REQ   = 0x08,
    CF_STORE_IMPTRACK = 0x10,
    CF_CLICK_REQ      = 0x20,
    CF_STORE_CLICKS   = 0x40,
    CF_NO_TPARAM      = 0x80
  };

  struct CaseRequest
  {
    CaseRequest(const char* aid_,
                const char* src_,
                const char* size_,
                const char* referer_,
                const char* ad_id_,
                const int flags_,
                const char* win_price_,
                const char* referer_kw_ = nullptr):
      aid       (aid_       ),
      src       (src_       ),
      size      (size_      ),
      referer   (referer_   ),
      ad_id     (ad_id_     ),
      flags     (flags_     ),
      win_price (win_price_ ),
      referer_kw(referer_kw_) {}

    // Request params
    const char* aid;
    const char* src;
    const char* size;
    const char* referer;
    const char* ad_id;
    const int flags;
    const char* win_price;
    const char* referer_kw;
  };

  struct CaseStats
  {
    const char* pub_account_id;
    const char* tag_id;
    const char* cc_id;
    unsigned int requests;
    unsigned int imps;
    unsigned int clicks;
    double adv_amount;
    double pub_amount;
    double isp_amount;
  };

public:
  RTBWinPriceNotificationTest(
    UnitStat& stat_var,
    const char* task_name,
    XsdParams params_var) :
    BaseDBUnit(stat_var, task_name, params_var)
  { }

  virtual ~RTBWinPriceNotificationTest() noexcept
  { }

private:

  AutoTest::Time now_;
  std::string encoded_now_;

  StoredRequests allyes_stored_requests_;

  template<typename RTB, size_t Count>
  StoredRequests process_requests_(const CaseRequest (&requests)[Count]);

  template<size_t Count>
  void select_current_stats_(const CaseStats (&expected)[Count],
    Stats& stats,
    Diffs& diffs);

  // Cases
  void openx_();
  void tanx_();
  void allyes_();
  void allyes_final_();
  void liverail_();

  void pre_condition();
  void set_up();
  bool run();
  void tear_down();
  void post_condition();

};

#endif // _AUTOTEST__RTBWINPRICENOTIFICATIONTEST_
