#ifndef CAMPAIGNSERVER_BILLSTATSOURCE_HPP
#define CAMPAIGNSERVER_BILLSTATSOURCE_HPP

#include <eh/Exception.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

#include <CORBACommons/CorbaAdapters.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>

namespace AdServer
{
namespace CampaignSvcs
{
  struct BillStatSource: public virtual ReferenceCounting::Interface
  {
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    struct Stat: public ReferenceCounting::AtomicCopyImpl
    {
    public:
      typedef std::map<Generics::Time, RevenueDecimal>
        DayToAmountMap;

      struct AmountDistribution
      {
        AmountDistribution()
          : prev_days_amount(RevenueDecimal::ZERO),
            prev_day(Generics::Time::ZERO)
        {}

        void
        print(std::ostream& out, const char* offset) const
          noexcept;

        RevenueDecimal prev_days_amount;
        Generics::Time prev_day;
        DayToAmountMap day_amounts;
      };

      struct Account: public AmountDistribution
      {};

      typedef std::map<unsigned long, Account> AccountMap;

      struct AmountCount
      {
        AmountCount()
          : amount(RevenueDecimal::ZERO),
            imps(ImpRevenueDecimal::ZERO),
            clicks(ImpRevenueDecimal::ZERO)
        {}

        AmountCount(
          const RevenueDecimal& amount_val,
          const ImpRevenueDecimal& imps_val,
          const ImpRevenueDecimal& clicks_val)
          : amount(amount_val),
            imps(imps_val),
            clicks(clicks_val)
        {}

        RevenueDecimal amount;
        ImpRevenueDecimal imps;
        ImpRevenueDecimal clicks;
      };

      typedef std::map<Generics::Time, AmountCount>
        DayToAmountCountMap;

      struct AmountCountDistribution
      {
        AmountCountDistribution()
          : prev_days_amount(RevenueDecimal::ZERO),
            prev_days_imps(ImpRevenueDecimal::ZERO),
            prev_days_clicks(ImpRevenueDecimal::ZERO),
            prev_day(Generics::Time::ZERO)
        {}

        void
        print(std::ostream& out, const char* offset) const
          noexcept;

        RevenueDecimal prev_days_amount;
        ImpRevenueDecimal prev_days_imps;
        ImpRevenueDecimal prev_days_clicks;
        Generics::Time prev_day;
        DayToAmountCountMap day_amount_counts;
      };

      struct Campaign: public AmountCountDistribution
      {};

      typedef std::map<unsigned long, Campaign> CampaignMap;

      struct CCG: public AmountCountDistribution
      {};

      typedef std::map<unsigned long, CCG> CCGMap;

    public:
      void
      print(std::ostream& out, const char* offset) const noexcept;

    public:
      Generics::Time timestamp;
      AccountMap accounts;
      CampaignMap campaigns;
      CCGMap ccgs;

    protected:
      virtual
      ~Stat() noexcept
      {}
    };

    typedef ReferenceCounting::QualPtr<Stat>
      Stat_var;

    typedef ReferenceCounting::ConstPtr<Stat>
      CStat_var;

    virtual Stat_var
    update(
      Stat* stat,
      const Generics::Time& now)
      /*throw(Exception)*/ = 0;

  protected:
    virtual
    ~BillStatSource() noexcept
    {}
  };

  typedef ReferenceCounting::QualPtr<BillStatSource>
    BillStatSource_var;

  typedef ReferenceCounting::FixedPtr<BillStatSource>
    FBillStatSource_var;
}
}

#endif /*CAMPAIGNSERVER_BILLSTATSOURCE_HPP*/
