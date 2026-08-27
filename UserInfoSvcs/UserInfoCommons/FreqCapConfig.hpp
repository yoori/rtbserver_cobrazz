#pragma once

#include <boost/unordered/unordered_flat_map.hpp>
#include <boost/unordered/unordered_flat_set.hpp>

#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

#include <Commons/FreqCap.hpp>

namespace AdServer::UserInfoSvcs
{
  struct FreqCapConfig: public virtual ReferenceCounting::AtomicImpl
  {
    using FreqCapMap = boost::unordered_flat_map<unsigned long, Commons::FreqCap>;

    using CampaignIds = boost::unordered_flat_set<unsigned long>;

    Generics::Time confirm_timeout;
    FreqCapMap freq_caps;
    CampaignIds campaign_ids;

  private:
    virtual
    ~FreqCapConfig() noexcept
    {}
  };

  typedef ReferenceCounting::SmartPtr<FreqCapConfig> FreqCapConfig_var;
}
