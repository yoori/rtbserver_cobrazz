#pragma once

#include <eh/Exception.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <Logger/Logger.hpp>

#include <Commons/Postgres/ConnectionPool.hpp>
#include <LogProcessing/LogGeneralizer/LogGeneralizerGrpc.hpp>

#include "StatSource.hpp"

namespace AdServer::CampaignSvcs
{
  class StatDBSource:
    public StatSource,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    DECLARE_EXCEPTION(Exception, StatSource::Exception);

    StatDBSource(
      Logging::Logger* logger,
      Commons::Postgres::ConnectionPool* pool,
      unsigned long server_id,
      const std::vector<std::string>& stat_providers);

    virtual Stat_var
    update(Stat* stat, bool& full_synch_required, const Generics::Time& now)
      /*throw(Exception)*/;

  protected:
    virtual ~StatDBSource() noexcept;

  private:
    using LogGeneralizerRefList =
      std::vector<std::unique_ptr<AdServer::LogProcessing::LogGeneralizerClient>>;

  private:
    Stat_var
    query_db_stats_(const Generics::Time& now)
      /*throw(Exception)*/;

  private:
    Logging::Logger_var logger_;
    Commons::Postgres::ConnectionPool_var pg_pool_;
    unsigned long server_id_;
    LogGeneralizerRefList stat_providers_;
  };
}
