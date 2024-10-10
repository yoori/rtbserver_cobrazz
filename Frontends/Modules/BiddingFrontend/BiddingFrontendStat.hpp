/// @file BiddingFrontend/BiddingFrontendStat.hpp
#ifndef BIDDINGFRONTENDSTAT_HPP
#define BIDDINGFRONTENDSTAT_HPP

#include <eh/Exception.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Controlling/StatsDumper/StatsDumper.hpp>
#include <CORBACommons/CorbaAdapters.hpp>
#include <Frontends/FrontendCommons/CampaignManagersPool.hpp>


namespace AdServer
{
  class BiddingStatInfo;

  class StatHolder : public ReferenceCounting::AtomicImpl
  {
  public:
    struct StatData
    {
      StatData();
      StatData(
        unsigned long request_google_,
        unsigned long request_google_bid_,
        unsigned long request_openrtb_,
        unsigned long request_openrtb_bid_,
        unsigned long request_other_,
        unsigned long request_other_bid_,
        const Generics::Time& processing_time);
      
      unsigned long request_google;
      unsigned long request_google_bid;
      unsigned long request_openrtb;
      unsigned long request_openrtb_bid;
      unsigned long request_other;
      unsigned long request_other_bid;
      unsigned long skipped;
      Generics::Time processing_time;
      typedef std::map<Generics::Time, std::size_t> TimeoutsMap;
      TimeoutsMap timeout_counters;
    
      StatData&
      operator +=(const StatData& rhs) noexcept;
    };

    void
    add_skipped() noexcept;

    void
    add_timeout(const Generics::Time& timeout) noexcept;

    virtual Generics::Values_var
    dump_stats();

  protected:
    virtual
    ~StatHolder() noexcept = default;

  private:
    StatData stat_data_;
    Sync::PosixMutex mutex_;
  };

  typedef ReferenceCounting::SmartPtr<StatHolder>
    StatHolder_var;
  
  class BiddingFrontendStatsImpl:
    public CORBACommons::ReferenceCounting::ServantImpl<
      POA_CORBACommons::ProcessStatsControl>,
      public Generics::CompositeActiveObject
  {
  public:
    BiddingFrontendStatsImpl(
      StatHolder* stat_holder)
      noexcept;
      
    virtual CORBACommons::StatsValueSeq*
    get_stats()
      /*throw(CORBA::Exception,
        CORBACommons::ProcessStatsControl::ImplementationException)*/;

  private:
    StatHolder_var stat_holder_;
  };

  typedef ReferenceCounting::SmartPtr<BiddingFrontendStatsImpl>
  BiddingFrontendStatsImpl_var;
}

#endif // BIDDINGFRONTENDSTAT_HPP
