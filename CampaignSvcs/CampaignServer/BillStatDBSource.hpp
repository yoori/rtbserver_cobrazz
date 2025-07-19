#ifndef CAMPAIGNSERVER_BILLSTATDBSOURCE_HPP_
#define CAMPAIGNSERVER_BILLSTATDBSOURCE_HPP_

#include <eh/Exception.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/PtrHolder.hpp>
#include <Logger/Logger.hpp>
#include <HTTP/UrlAddress.hpp>

#include <Commons/Postgres/ConnectionPool.hpp>

#include "BillStatSource.hpp"

namespace AdServer
{
namespace CampaignSvcs
{
  class BillStatDBSource:
    public BillStatSource,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    DECLARE_EXCEPTION(Exception, BillStatSource::Exception);

    BillStatDBSource(
      Logging::Logger* logger,
      Commons::Postgres::Environment* pg_env)
      /*throw(Exception)*/;

    virtual Stat_var
    update(Stat* stat,
      const Generics::Time& now)
      /*throw(Exception)*/;

  protected:
    virtual ~BillStatDBSource() noexcept {}

  private:
    Stat_var
    query_db_stats_(const Generics::Time& now)
      /*throw(Exception)*/;

    Stat_var query_day_db_stats_(
      const Generics::Time& now,
      Commons::Postgres::Connection* conn);

    void query_hour_db_stats_(
      const Generics::Time& now,
      Commons::Postgres::Connection* conn,
      Stat& stat);

  private:
    Logging::Logger_var logger_;
    Commons::Postgres::ConnectionPool_var pg_pool_;
  };

  typedef ReferenceCounting::QualPtr<BillStatDBSource>
    BillStatDBSource_var;
}
}

#endif /*CAMPAIGNSERVER_BILLSTATDBSOURCE_HPP_*/
