#ifndef CAMPAIGNSERVER_BILLSTATSERVERSOURCE_HPP
#define CAMPAIGNSERVER_BILLSTATSERVERSOURCE_HPP

#include <eh/Exception.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <Logger/Logger.hpp>
#include <CORBACommons/CorbaAdapters.hpp>

#include <Commons/CorbaObject.hpp>
#include "CampaignServerPool.hpp"
#include "BillStatSource.hpp"

namespace AdServer
{
namespace CampaignSvcs
{
  class BillStatServerSource:
    public BillStatSource,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    BillStatServerSource(
      Logging::Logger* logger,
      unsigned long server_id,
      const CORBACommons::CorbaObjectRefList& stat_providers)
      noexcept;

    virtual Stat_var
    update(
      Stat* stat,
      const Generics::Time& now)
      /*throw(BillStatSource::Exception)*/;

  protected:
    virtual
    ~BillStatServerSource() noexcept = default;

    Stat_var
    convert_update_(
      const AdServer::CampaignSvcs::BillStatInfo& bill_stat_info,
      const Generics::Time& now)
      /*throw(Exception)*/;

    void
    convert_amount_distribution_(
      Stat::AmountDistribution& amount_distribution,
      const AmountDistributionInfo& amount_distribution_info)
      noexcept;

    void
    convert_amount_count_distribution_(
      Stat::AmountCountDistribution& amount_count_distribution,
      const AmountCountDistributionInfo& amount_count_distribution_info)
      noexcept;

  private:
    Logging::Logger_var logger_;
    const unsigned long server_id_;
    CampaignServerPoolPtr campaign_servers_;
  };

  typedef ReferenceCounting::QualPtr<BillStatServerSource>
    BillStatServerSource_var;
}
}

#endif /*CAMPAIGNSERVER_BILLSTATSERVERSOURCE_HPP*/
