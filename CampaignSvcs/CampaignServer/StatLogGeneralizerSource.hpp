#ifndef CAMPAIGNSERVER_STATLOGGENERALIZERSOURCE_HPP
#define CAMPAIGNSERVER_STATLOGGENERALIZERSOURCE_HPP

#include <eh/Exception.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <Logger/Logger.hpp>
#include <CORBACommons/CorbaAdapters.hpp>

#include <Commons/CorbaObject.hpp>
#include <LogProcessing/LogGeneralizer/LogGeneralizer.hpp>

#include "StatSource.hpp"

namespace AdServer
{
namespace CampaignSvcs
{
  class StatLogGeneralizerSource:
    public StatSource,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    DECLARE_EXCEPTION(Exception, StatSource::Exception);

    StatLogGeneralizerSource(
      Logging::Logger* logger,
      unsigned long server_id,
      const CORBACommons::CorbaObjectRefList& stat_providers)
      noexcept;

    virtual Stat_var
    update(
      Stat* stat,
      bool& full_synch_required,
      const Generics::Time& now)
      /*throw(Exception)*/;

  protected:
    virtual ~StatLogGeneralizerSource() noexcept {}

  private:
    typedef AdServer::Commons::CorbaObject<
      AdServer::LogProcessing::LogGeneralizer> LogGeneralizerRef;
    typedef std::list<LogGeneralizerRef> LogGeneralizerRefList;

  private:
    StatSource::Stat_var
    convert_stats_update_(
      const AdServer::LogProcessing::StatInfo& update,
      const Generics::Time& now)
      /*throw(Exception)*/;

  private:
    Logging::Logger_var logger_;
    unsigned long server_id_;
    LogGeneralizerRefList stat_providers_;
  };
}
}

#endif /*CAMPAIGNSERVER_STATLOGGENERALIZERSOURCE_HPP*/
