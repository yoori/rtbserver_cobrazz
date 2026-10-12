#pragma once

#include <eh/Exception.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <Logger/Logger.hpp>

#include <LogProcessing/LogGeneralizer/LogGeneralizerGrpc.hpp>

#include "StatSource.hpp"

namespace AdServer::CampaignSvcs
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
      const std::vector<std::string>& stat_providers);

    virtual Stat_var
    update(Stat* stat, bool& full_synch_required, const Generics::Time& now)
      /*throw(Exception)*/;

  protected:
    virtual ~StatLogGeneralizerSource() noexcept;

  private:
    using LogGeneralizerRefList =
      std::vector<std::unique_ptr<AdServer::LogProcessing::LogGeneralizerClient>>;

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
