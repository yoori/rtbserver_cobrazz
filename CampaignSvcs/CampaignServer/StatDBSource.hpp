#ifndef CAMPAIGNSERVER_STATDBSOURCE_HPP
#define CAMPAIGNSERVER_STATDBSOURCE_HPP

#include <eh/Exception.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <Logger/Logger.hpp>

#include <Commons/Postgres/ConnectionPool.hpp>
#include <Commons/CorbaObject.hpp>
#include <LogProcessing/LogGeneralizer/LogGeneralizer.hpp>

#include "StatSource.hpp"

namespace AdServer
{
namespace CampaignSvcs
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
      const CORBACommons::CorbaObjectRefList& stat_providers)
      noexcept;

    virtual Stat_var
    update(
      Stat* stat,
      bool& full_synch_required,
      const Generics::Time& now)
      /*throw(Exception)*/;

  protected:
    virtual ~StatDBSource() noexcept {}

  private:
    typedef AdServer::Commons::CorbaObject<
      AdServer::LogProcessing::LogGeneralizer> LogGeneralizerRef;
    typedef std::list<LogGeneralizerRef> LogGeneralizerRefList;

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
}

#endif /*CAMPAIGNSERVER_STATDBSOURCE_HPP*/
