#ifndef FREQCAPCONFIG_HPP
#define FREQCAPCONFIG_HPP

#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <Generics/GnuHashTable.hpp>
#include <Generics/HashTableAdapters.hpp>

#include <Commons/FreqCap.hpp>

namespace AdServer
{
namespace UserInfoSvcs
{
  struct FreqCapConfig: public virtual ReferenceCounting::AtomicImpl
  {
    typedef Generics::GnuHashTable<
      Generics::NumericHashAdapter<unsigned long>,
      Commons::FreqCap>
      FreqCapMap;

    typedef std::set<unsigned long> CampaignIds;

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
}

#endif /*FREQCAPCONFIG_HPP*/
