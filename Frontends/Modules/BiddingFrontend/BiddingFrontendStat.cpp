/// @file BiddingFrontend/BiddingFrontendStat.cpp
#include "BiddingFrontendStat.hpp"
#include "GroupLogger.hpp"

#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>

namespace
{
  const Generics::Values::Key BF_REQ_COUNT        = "rtbRequestCount";
  const Generics::Values::Key BF_REQ_BIDS         = "rtbRequestBidCount";
  const Generics::Values::Key BF_REQ_GOOGLE       = "rtbRequestGoogleCount";
  const Generics::Values::Key BF_REQ_GOOGLE_BIDS  = "rtbRequestGoogleBidCount";
  const Generics::Values::Key BF_REQ_OPENRTB      = "rtbRequestOpenRTBCount";
  const Generics::Values::Key BF_REQ_OPENRTB_BIDS = "rtbRequestOpenRTBBidCount";
  const Generics::Values::Key BF_REQ_OTHER        = "rtbRequestOtherCount";
  const Generics::Values::Key BF_REQ_OTHER_BIDS   = "rtbRequestOtherBidCount";

  const Generics::Values::Key BF_SKIPPED          = "rtbRequestSkipCount";
  const Generics::Values::Key BF_TIMEOUTS         = "rtbRequestTimeoutCount";
  const Generics::Values::Key BF_TIME_COUNTER     = "rtbRequestTimeCounter";

  struct GapKey
  {
    Generics::Time t;
    Generics::Values::Key key;
    bool
    operator <(const Generics::Time& rhs) const noexcept
    {
      return t < rhs;
    }
  };

  const GapKey GAPS[] =
  {
    {Generics::Time(0, 10000), "timeoutCount.time10ms"},
    {Generics::Time(0, 20000), "timeoutCount.time20ms"},
    {Generics::Time(0, 30000), "timeoutCount.time30ms"},
    {Generics::Time(0, 40000), "timeoutCount.time40ms"},
    {Generics::Time(0, 50000), "timeoutCount.time50ms"},
    {Generics::Time(0, 60000), "timeoutCount.time60ms"},
    {Generics::Time(0, 70000), "timeoutCount.time70ms"},
    {Generics::Time(0, 80000), "timeoutCount.time80ms"},
    {Generics::Time(0, 90000), "timeoutCount.time90ms"},
    {Generics::Time(0, 100000), "timeoutCount.time100ms"},
    {Generics::Time(0, 200000), "timeoutCount.time200ms"},
    {Generics::Time(0, 300000), "timeoutCount.time300ms"},
    {Generics::Time(0, 400000), "timeoutCount.time400ms"},
    {Generics::Time(0, 500000), "timeoutCount.time500ms"},
    {Generics::Time(0, 600000), "timeoutCount.time600ms"},
    {Generics::Time(0, 700000), "timeoutCount.time700ms"},
    {Generics::Time(0, 800000), "timeoutCount.time800ms"},
    {Generics::Time(0, 900000), "timeoutCount.time900ms"},
    {Generics::Time(1, 0), "timeoutCount.time1sec"},
    {Generics::Time(2, 0), "timeoutCount.time2sec"},
    {Generics::Time(3, 0), "timeoutCount.time3sec"},
    {Generics::Time(4, 0), "timeoutCount.time4sec"},
    {Generics::Time(5, 0), "timeoutCount.time5sec"},
    {Generics::Time(6, 0), "timeoutCount.time6sec"},
    {Generics::Time(7, 0), "timeoutCount.time7sec"},
    {Generics::Time(8, 0), "timeoutCount.time8sec"},
    {Generics::Time(9, 0), "timeoutCount.time9sec"},
    {Generics::Time(10, 0), "timeoutCount.time10sec"},
  };

  const Generics::Values::Key&
  convert_to_key(const Generics::Time& value) noexcept
  {
    const GapKey* gap =
      std::lower_bound(GAPS,
        GAPS + sizeof(GAPS)/sizeof(GAPS[0]), value);
    std::size_t index = gap - GAPS;
    return index < sizeof(GAPS)/sizeof(GAPS[0]) ? GAPS[index].key :
      GAPS[sizeof(GAPS)/sizeof(GAPS[0]) - 1].key;
  }
}

namespace Aspect
{
  extern const char BIDDING_FRONTEND[];
}

using namespace AdServer::CampaignSvcs;

namespace AdServer
{
  // StatHolder::StatData
  
  StatHolder::StatData::StatData()
    : request_google(0),
      request_google_bid(0),
      request_openrtb(0),
      request_openrtb_bid(0),
      request_other(0),
      request_other_bid(0),
      skipped(0)
  {}

  StatHolder::StatData::StatData(
    unsigned long request_google_,
    unsigned long request_google_bid_,
    unsigned long request_openrtb_,
    unsigned long request_openrtb_bid_,
    unsigned long request_other_,
    unsigned long request_other_bid_,
    const Generics::Time& processing_time_val)
    : request_google(request_google_),
      request_google_bid(request_google_bid_),
      request_openrtb(request_openrtb_),
      request_openrtb_bid(request_openrtb_bid_),
      request_other(request_other_),
      request_other_bid(request_other_bid_),
      processing_time(processing_time_val)
  {}

  StatHolder::StatData&
  StatHolder::StatData::operator +=(const StatHolder::StatData& rhs) noexcept
  {
    request_google += rhs.request_google;
    request_google_bid += rhs.request_google_bid;
    request_openrtb += rhs.request_openrtb;
    request_openrtb_bid += rhs.request_openrtb_bid;
    request_other += rhs.request_other;
    request_other_bid += rhs.request_other_bid;
    processing_time += rhs.processing_time;

    return *this;
  }

  // StatHolder
  void
  StatHolder::add_skipped() noexcept
  {
    Sync::PosixGuard lock(mutex_);
    ++stat_data_.skipped;
  }

  void
  StatHolder::add_timeout(const Generics::Time& timeout) noexcept
  {
    Generics::Time time_round = Bidding::CellsKey::round_timeout(timeout);
    Sync::PosixGuard lock(mutex_);
    ++stat_data_.timeout_counters[time_round];
  }

  Generics::Values_var
  StatHolder::dump_stats()
  {
    StatData d;

    {
      Sync::PosixGuard lock(mutex_);
      d += stat_data_;
    }

    Generics::Values_var v(new Generics::Values);

    std::size_t request_count =
        d.request_openrtb + d.request_other + d.request_google;

    v->set(BF_REQ_COUNT, request_count);
    v->set(
      BF_REQ_BIDS,
        d.request_openrtb_bid + d.request_other_bid +
          d.request_google_bid);
    v->set(BF_REQ_GOOGLE, d.request_google);
    v->set(BF_REQ_GOOGLE_BIDS, d.request_google_bid);
    v->set(BF_REQ_OPENRTB, d.request_openrtb);
    v->set(BF_REQ_OPENRTB_BIDS, d.request_openrtb_bid);
    v->set(BF_REQ_OTHER, d.request_other);
    v->set(BF_REQ_OTHER_BIDS, d.request_other_bid);

    v->set(BF_SKIPPED, d.skipped);
    std::size_t timeout_counter = 0;
    for (StatData::TimeoutsMap::const_iterator cit = d.timeout_counters.begin();
      cit != d.timeout_counters.end(); ++cit)
    {
      timeout_counter += cit->second;
      v->set(convert_to_key(cit->first), cit->second);
    }
    v->set(BF_TIMEOUTS, timeout_counter);
    v->set(BF_TIME_COUNTER, static_cast<unsigned long>(d.processing_time.microseconds()));

    return v;
  }

  // BiddingFrontendStatsImpl
  
  BiddingFrontendStatsImpl::BiddingFrontendStatsImpl(
    StatHolder* stat_holder)
    noexcept :
    stat_holder_(ReferenceCounting::add_ref(stat_holder))
  { }


  CORBACommons::StatsValueSeq*
  BiddingFrontendStatsImpl::get_stats()
    /*throw(CORBA::Exception,
      CORBACommons::ProcessStatsControl::ImplementationException)*/
  {
    static const char* FUN = "BiddingFrontendStatsImpl::get_stats()";

    CORBACommons::StatsValueSeq_var res;
    try
    {
      Generics::Values_var stat = stat_holder_->dump_stats(); 
      
      res = 
        CORBACommons::ValuesConverter::get_stats(*stat);
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << e.what();

      CORBACommons::throw_desc<
        CORBACommons::ProcessStatsControl::ImplementationException>(
          ostr.str());
    }
    return res._retn();
  }

}
