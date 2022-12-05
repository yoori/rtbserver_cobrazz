
#include <String/AsciiStringManip.hpp>

#include <Commons/Postgres/SqlStream.hpp>
#include <Commons/Postgres/Statement.hpp>

#include <LogCommons/LogCommons.ipp>

#include "LogDbSaverImpl.hpp"
#include "ErrorCode.hpp"

namespace AdServer {
namespace LogProcessing {

namespace {

const char DT_CREATIVESTATPGRECORD[] = "adserver.snapshot_rsh_t";
const char DT_CMPSTATPGRECORD[]      = "adserver.snapshot_cmp_t";

class CreativeStatPgRecord: public Commons::Postgres::Object
{
public:
  CreativeStatPgRecord(
    const CreativeStatKey& outer_key,
    const CreativeStatInnerKey& key,
    const CreativeStatInnerData& data
  ) noexcept;

  virtual ~CreativeStatPgRecord() noexcept {}

  const char* getSQLTypeName() const /*throw(eh::Exception)*/;

  virtual void writeSQL(Commons::Postgres::SqlStream& stream)
    /*throw(eh::Exception, Commons::Postgres::SqlException)*/;

  virtual void readSQL(Commons::Postgres::SqlStream& stream)
    /*throw(eh::Exception, Commons::Postgres::SqlException)*/;

private:
  const CreativeStatKey& outer_key_;
  const CreativeStatInnerKey& key_;
  const CreativeStatInnerData& data_;
};

const char*
CreativeStatPgRecord::getSQLTypeName() const /*throw(eh::Exception)*/
{
  return DT_CREATIVESTATPGRECORD;
}

inline
CreativeStatPgRecord::CreativeStatPgRecord(
  const CreativeStatKey& outer_key,
  const CreativeStatInnerKey& key,
  const CreativeStatInnerData& data
) noexcept
:
  outer_key_(outer_key),
  key_(key),
  data_(data)
{
}

void
CreativeStatPgRecord::writeSQL(Commons::Postgres::SqlStream& stream)
  /*throw(eh::Exception, Commons::Postgres::SqlException)*/
{
  Generics::ExtendedTime time(outer_key_.sdate().time().get_gm_time());

  stream.set_timestamp(time);
  stream.set_value(key_.adv_account_id());
  stream.set_value(key_.campaign_id());
  stream.set_value(key_.ccg_id());
  stream.set_value(key_.cc_id());
  stream.set_value(key_.publisher_account_id());
  stream.set_value(key_.tag_id());
  stream.set_value(key_.ctr_reset_id());

  stream.set_value(data_.imps());
  stream.set_value(data_.clicks());
  stream.set_value(data_.actions());
  stream.set_decimal(data_.adv_amount());
  stream.set_decimal(data_.adv_comm_amount());
  stream.set_decimal(data_.adv_payable_comm_amount());

  typedef FixedNumber FxNum;

  FxNum threshold(key_.delivery_threshold());
  FxNum pub_isp_amount = data_.pub_advcurrency_amount() +
    data_.isp_advcurrency_amount();

  stream.set_decimal(FxNum::div(pub_isp_amount, threshold));
  stream.set_decimal(FxNum::div(data_.adv_amount(), threshold));
  stream.set_decimal(FxNum::div(data_.adv_payable_comm_amount(), threshold));
}

void
CreativeStatPgRecord::readSQL(Commons::Postgres::SqlStream& /* stream */)
  /*throw(eh::Exception, Commons::Postgres::SqlException)*/
{
  throw Commons::Postgres::SqlException("CreativeStatPgRecord::readSQL(): "
    "not supported");
}

class CmpStatPgRecord: public Commons::Postgres::Object
{
public:
  CmpStatPgRecord(
    const CmpStatKey& outer_key,
    const CmpStatInnerKey& key,
    const CmpStatInnerData& data
  ) noexcept;

  virtual ~CmpStatPgRecord() noexcept {}

  const char* getSQLTypeName() const /*throw(eh::Exception)*/;

  virtual void writeSQL(Commons::Postgres::SqlStream& stream)
    /*throw(eh::Exception, Commons::Postgres::SqlException)*/;

  virtual void readSQL(Commons::Postgres::SqlStream& stream)
    /*throw(eh::Exception, Commons::Postgres::SqlException)*/;

private:
  const CmpStatKey& outer_key_;
  const CmpStatInnerKey& key_;
  const CmpStatInnerData& data_;
};

const char*
CmpStatPgRecord::getSQLTypeName() const /*throw(eh::Exception)*/
{
  return DT_CMPSTATPGRECORD;
}

inline
CmpStatPgRecord::CmpStatPgRecord(
  const CmpStatKey& outer_key,
  const CmpStatInnerKey& key,
  const CmpStatInnerData& data
) noexcept
:
  outer_key_(outer_key),
  key_(key),
  data_(data)
{
}

void
CmpStatPgRecord::writeSQL(Commons::Postgres::SqlStream& stream)
  /*throw(eh::Exception, Commons::Postgres::SqlException)*/
{
  Generics::ExtendedTime time(outer_key_.sdate().time().get_gm_time());

  stream.set_timestamp(time);
  stream.set_value(key_.campaign_id());
  stream.set_value(key_.ccg_id());
  stream.set_value(key_.cc_id());
  stream.set_value(key_.publisher_account_id());

  stream.set_decimal(data_.adv_amount_cmp());
}

void
CmpStatPgRecord::readSQL(Commons::Postgres::SqlStream& /* stream */)
  /*throw(eh::Exception, Commons::Postgres::SqlException)*/
{
  throw Commons::Postgres::SqlException("CmpStatPgRecord::readSQL(): "
    "not supported");
}

} // namespace

static
void
update_creative_log_gen_stat(
  CreativeStatTraits::CollectorType& collector,
  LogGeneralizerStatMapBundle& lgsm_bundle,
  const Generics::Time& intime = Generics::Time::ZERO,
  bool deferred = false
)
{
  typedef CreativeStatTraits::CollectorType CollectorT;

  for (CollectorT::const_iterator it = collector.begin();
    it != collector.end(); ++it)
  {
    const CollectorT::KeyT& key = it->first;
    const CollectorT::DataT& data = it->second;
    for (CollectorT::DataT::const_iterator data_it = data.begin();
      data_it != data.end(); ++data_it)
    {
      if (data_it->first.test() || !data_it->first.adv_account_id() ||
        !data_it->first.campaign_id() || !data_it->first.ccg_id())
      {
        continue;
      }

      CampaignStatKeyDef
        cmp_key(
          key.sdate().time(),
          key.adv_sdate().time(),
          data_it->first.adv_account_id(),
          data_it->first.campaign_id(),
          data_it->first.ccg_id()
        );

      CampaignStatValueDef cmp_value;

      cmp_value.adv_account_amount = data_it->second.adv_amount();

      if (data_it->first.walled_garden())
      {
        cmp_value.adv_account_amount -=
          data_it->second.pub_advcurrency_amount();
      }

      cmp_value.adv_amount = data_it->second.adv_amount();
      cmp_value.adv_comm_amount = data_it->second.adv_comm_amount();
      cmp_value.adv_payable_comm_amount =
        data_it->second.adv_payable_comm_amount();

      CreativeStatValueDef
        crtv_value(
          data_it->second.unverified_imps(),
          data_it->second.imps(),
          data_it->second.clicks(),
          data_it->second.actions()
        );

      if (data_it->first.cc_id())
      {
        cmp_value.creative_stats[data_it->first.cc_id()] += crtv_value;
      }

      if (data_it->first.publisher_account_id())
      {
        unsigned long pub_acc_id = data_it->first.publisher_account_id();
        cmp_value.publisher_amounts[pub_acc_id] +=
          data_it->second.adv_amount();
      }

      typedef CampaignTagStatValueDef::DecimalT DecimalT;

      DecimalT pub_isp_amount = data_it->second.pub_advcurrency_amount();
      pub_isp_amount += data_it->second.isp_advcurrency_amount();

      FixedNumber delivery_threshold(data_it->first.delivery_threshold()); //FIXME
      CampaignTagStatValueDef
        cmp_tag_stat_value(
          DecimalT::div(pub_isp_amount, delivery_threshold),
          DecimalT::div(data_it->second.adv_amount(), delivery_threshold),
          DecimalT::div(data_it->second.adv_comm_amount(), delivery_threshold)
        );

      if (data_it->first.tag_id())
      {
        cmp_value.tag_amounts[data_it->first.tag_id()] +=
          cmp_tag_stat_value;
      }

      if (data_it->first.ctr_reset_id())
      {
        cmp_value.ctr_reset_stats[data_it->first.ctr_reset_id()] +=
          CtrResetStatValueDef(data_it->second.imps());
      }

      LogGeneralizerStatMap& lgs_map = lgsm_bundle.map;
      for (LogGeneralizerStatMap::iterator lgsm_it = lgs_map.begin();
        lgsm_it != lgs_map.end(); ++lgsm_it)
      {
        LogGeneralizerStatValue::GuardT lgsv_guard(lgsm_it->second->lock);
        if (deferred && intime >= lgsm_it->second->start_clear_timestamp)
        {
          continue;
        }
        lgsm_it->second->map[cmp_key] += cmp_value;
      }
    }
  }
}

static
bool
do_creative_stat_save_(
  Commons::Postgres::Connection_var& connection,
  const CreativeStatTraits::CollectorType::KeyT& outer_key,
  const CreativeStatTraits::CollectorType::DataT& collector
)
{
  typedef CreativeStatTraits::CollectorType CollectorT;

  if (collector.empty())
  {
    return true;
  }

  bool successful = false;

  Commons::Postgres::ObjectVector records_to_db;
  records_to_db.reserve(collector.size());

  for (CollectorT::DataT::const_iterator it = collector.begin();
    it != collector.end(); ++it)
  {
    if (!it->first.test())
    {
      CreativeStatPgRecord *creative_stat_pg_record =
        new CreativeStatPgRecord(outer_key, it->first, it->second);
      records_to_db.push_back(creative_stat_pg_record);
    }
  }

  if (!records_to_db.empty())
  {
    const char sql_stmt[] =
      "SELECT adserver.fill_snapshot_rsh($1::adserver.snapshot_rsh_t[])";

    Commons::Postgres::Statement_var stmt =
      new Commons::Postgres::Statement(sql_stmt);

    stmt->set_array(1, records_to_db);

    AdServer::Commons::Postgres::ResultSet_var res =
      connection->execute_statement(stmt, true);
  }

  successful = true;

  return successful;
}

void
CreativeStatDbSaver::save_i_(
  CollectorT& collector,
  Generics::Time& outtime
)
{
  Commons::Postgres::Connection_var connection;

  try
  {
    {
      LogGeneralizerStatMapBundle& lgsm_bundle = *output_;

      LogGeneralizerStatMapBundle::ReadGuardT lgsm_guard(lgsm_bundle.lock);

      outtime = Generics::Time::get_time_of_day();

      update_creative_log_gen_stat(collector, lgsm_bundle);
    }

    std::ostringstream err_oss;

    try
    {
      connection = pg_conn_factory_->get_connection();

      for (CollectorT::const_iterator it = collector.begin();
        it != collector.end(); ++it)
      {
        if (!do_save_(connection, it->first, it->second))
        {
          err_oss << __PRETTY_FUNCTION__
            << ": Error: Can't save data to database (sdate = "
            << it->first << ')';
        }
      }
    }
    catch (const DbConnectionFactory::NotActive&)
    {
      Stream::Error es;
      es << __PRETTY_FUNCTION__
        << ": DB is not active. Operation will be deferred.";
      throw OperationDeferred(es);
    }

    if (!err_oss.str().empty())
    {
      throw Exception(err_oss.str(), LOG_GEN_DB_ERR_CODE_1);
    }
  }
  catch (const Exception&)
  {
    throw;
  }
  catch (const AdServer::Commons::Postgres::Exception& ex)
  {
    connection->bad();

    Stream::Error es;
    es << __PRETTY_FUNCTION__ << ": Caught AdServer::Commons::Postgres::"
       "Exception. : " << ex.what();
    throw Exception(es, LOG_GEN_DB_ERR_CODE_1);
  }
  catch (const eh::Exception& ex)
  {
    connection->bad();

    Stream::Error es;
    es << __PRETTY_FUNCTION__ << ": Caught eh::Exception. "
       ": " << ex.what();
    throw Exception(es, LOG_GEN_IMPL_ERR_CODE_3);
  }
}

bool
CreativeStatDbSaver::do_save_(
  Commons::Postgres::Connection_var& connection,
  const CollectorT::KeyT& outer_key,
  const CollectorT::DataT& collector
)
{
  return do_creative_stat_save_(connection, outer_key, collector);
}

void
DeferredCreativeStatDbSaver::save_i_(
  CollectorT& collector,
  const Generics::Time& intime,
  Generics::Time& outtime
)
{
  Commons::Postgres::Connection_var connection;

  try
  {
    {
      try
      {
        connection = pg_conn_factory_->get_connection();
      }
      catch (const DbConnectionFactory::NotActive&)
      {
        Stream::Error es;
        es << FNS << ": DB is not active. Operation will be deferred.";
        throw DbConnectionFactory::NotActive(es.str());
      }

      LogGeneralizerStatMapBundle& lgsm_bundle = *output_;

      LogGeneralizerStatMapBundle::ReadGuardT lgsm_guard(lgsm_bundle.lock);

      outtime = Generics::Time::get_time_of_day();

      update_creative_log_gen_stat(collector, lgsm_bundle, intime, true);
    }

    std::ostringstream err_oss;

    for (CollectorT::const_iterator it = collector.begin();
      it != collector.end(); ++it)
    {
      if (!do_save_(connection, it->first, it->second))
      {
        err_oss << __PRETTY_FUNCTION__
          << ": Error: Can't save data to database (sdate = "
          << it->first << ") ";
      }
    }

    if (!err_oss.str().empty())
    {
      throw Exception(err_oss.str(), LOG_GEN_DB_ERR_CODE_1);
    }
  }
  catch (const DbConnectionFactory::NotActive&)
  {
    throw;
  }
  catch (const Exception&)
  {
    throw;
  }
  catch (const AdServer::Commons::Postgres::Exception& ex)
  {
    connection->bad();

    Stream::Error es;
    es << __PRETTY_FUNCTION__ << ": Caught AdServer::Commons::Postgres::"
      "Exception. : " << ex.what();
    throw Exception(es, LOG_GEN_DB_ERR_CODE_1);
  }
  catch (const eh::Exception& ex)
  {
    connection->bad();

    Stream::Error es;
    es << __PRETTY_FUNCTION__ << ": Caught eh::Exception. "
      ": " << ex.what();
    throw Exception(es, LOG_GEN_IMPL_ERR_CODE_3);
  }
}

bool
DeferredCreativeStatDbSaver::do_save_(
  Commons::Postgres::Connection_var& connection,
  const CollectorT::KeyT& outer_key,
  const CollectorT::DataT& collector
)
{
  return do_creative_stat_save_(connection, outer_key, collector);
}

void
CreativeStatPgCsvSaver::pg_save_i_(
  CollectorT& collector,
  Generics::Time& outtime
)
{
  Commons::Postgres::Connection_var connection;

  try
  {
    {
      LogGeneralizerStatMapBundle& lgsm_bundle = *output_;

      LogGeneralizerStatMapBundle::ReadGuardT lgsm_guard(lgsm_bundle.lock);

      outtime = Generics::Time::get_time_of_day();

      update_creative_log_gen_stat(collector, lgsm_bundle);
    }

    std::ostringstream err_oss;

    try
    {
      connection = pg_conn_factory_->get_connection();

      for (CollectorT::const_iterator it = collector.begin();
        it != collector.end(); ++it)
      {
        if (!do_save_(connection, it->first, it->second))
        {
          err_oss << __PRETTY_FUNCTION__
            << ": Error: Can't save data to database (sdate = "
            << it->first << ')';
        }
      }
    }
    catch (const DbConnectionFactory::NotActive&)
    {
      Stream::Error es;
      es << __PRETTY_FUNCTION__
        << ": DB is not active. Operation will be deferred.";
      throw OperationDeferred(es);
    }

    if (!err_oss.str().empty())
    {
      throw PgException(err_oss.str(), LOG_GEN_DB_ERR_CODE_1);
    }
  }
  catch (const Exception&)
  {
    throw;
  }
  catch (const AdServer::Commons::Postgres::Exception& ex)
  {
    connection->bad();

    Stream::Error es;
    es << __PRETTY_FUNCTION__ << ": Caught AdServer::Commons::Postgres::"
      "Exception. : " << ex.what();
    throw PgException(es, LOG_GEN_DB_ERR_CODE_1);
  }
  catch (const eh::Exception& ex)
  {
    connection->bad();

    Stream::Error es;
    es << __PRETTY_FUNCTION__ << ": Caught eh::Exception : "
      << ex.what();
    throw PgException(es, LOG_GEN_IMPL_ERR_CODE_3);
  }
}

bool
CreativeStatPgCsvSaver::do_save_(
  Commons::Postgres::Connection_var& connection,
  const CollectorT::KeyT& outer_key,
  const CollectorT::DataT& collector
)
{
  return do_creative_stat_save_(connection, outer_key, collector);
}

void
DeferredCreativeStatPgCsvSaver::pg_save_i_(
  CollectorT& collector,
  const Generics::Time& intime,
  Generics::Time& outtime
)
{
  Commons::Postgres::Connection_var connection;

  try
  {
    {
      try
      {
        connection = pg_conn_factory_->get_connection();
      }
      catch (const DbConnectionFactory::NotActive&)
      {
        Stream::Error es;
        es << FNS << ": DB is not active. Operation will be deferred.";
        throw DbConnectionFactory::NotActive(es.str());
      }

      LogGeneralizerStatMapBundle& lgsm_bundle = *output_;

      LogGeneralizerStatMapBundle::ReadGuardT lgsm_guard(lgsm_bundle.lock);

      outtime = Generics::Time::get_time_of_day();

      update_creative_log_gen_stat(collector, lgsm_bundle, intime, true);
    }

    std::ostringstream err_oss;

    for (CollectorT::const_iterator it = collector.begin();
      it != collector.end(); ++it)
    {
      if (!do_save_(connection, it->first, it->second))
      {
        err_oss << __PRETTY_FUNCTION__ << ": Error: Can't save data to "
                << "database (sdate = " << it->first << ") ";
      }
    }

    if (!err_oss.str().empty())
    {
      throw PgException(err_oss.str(), LOG_GEN_DB_ERR_CODE_1);
    }
  }
  catch (const DbConnectionFactory::NotActive&)
  {
    throw;
  }
  catch (const Exception&)
  {
    throw;
  }
  catch (const AdServer::Commons::Postgres::Exception& ex)
  {
    connection->bad();

    Stream::Error es;
    es << __PRETTY_FUNCTION__ << ": Caught AdServer::Commons::Postgres::"
      "Exception. : " << ex.what();
    throw PgException(es, LOG_GEN_DB_ERR_CODE_1);
  }
  catch (const eh::Exception& ex)
  {
    connection->bad();

    Stream::Error es;
    es << __PRETTY_FUNCTION__ << ": Caught eh::Exception. "
      ": " << ex.what();
    throw PgException(es, LOG_GEN_IMPL_ERR_CODE_3);
  }
}

bool
DeferredCreativeStatPgCsvSaver::do_save_(
  Commons::Postgres::Connection_var& connection,
  const CollectorT::KeyT& outer_key,
  const CollectorT::DataT& collector
)
{
  return do_creative_stat_save_(connection, outer_key, collector);
}

static
void
update_cmp_log_gen_stat(
  CmpStatTraits::CollectorType& collector,
  LogGeneralizerStatMapBundle& lgsm_bundle,
  const Generics::Time& intime = Generics::Time::ZERO,
  bool deferred = false
)
{
  typedef CmpStatTraits::CollectorType CollectorT;

  for (CollectorT::const_iterator it = collector.begin();
    it != collector.end(); ++it)
  {
    const CollectorT::KeyT& key = it->first;
    const CollectorT::DataT& data = it->second;
    for (CollectorT::DataT::const_iterator data_it = data.begin();
      data_it != data.end(); ++data_it)
    {
      if (!data_it->first.adv_account_id() ||
        !data_it->first.campaign_id() || !data_it->first.ccg_id())
      {
        continue;
      }

      CampaignStatKeyDef
        cmp_key(
          key.sdate().time(),
          key.adv_sdate().time(),
          data_it->first.adv_account_id(),
          data_it->first.campaign_id(),
          data_it->first.ccg_id()
        );

      CampaignStatValueDef cmp_value;
      cmp_value.adv_account_amount = data_it->second.adv_amount_cmp();
      cmp_value.adv_amount = data_it->second.adv_amount_cmp();

      typedef CampaignTagStatValueDef::DecimalT DecimalT;

      CampaignTagStatValueDef
        cmp_tag_stat_value(
          DecimalT(), // ZERO
          DecimalT::div(data_it->second.adv_amount_cmp(),
            FixedNumber(data_it->first.delivery_threshold())), //FIXME
          DecimalT()  // ZERO
        );

      if (data_it->first.tag_id())
      {
        cmp_value.tag_amounts[data_it->first.tag_id()] +=
          cmp_tag_stat_value;
      }

      LogGeneralizerStatMap& lgs_map = lgsm_bundle.map;
      for (LogGeneralizerStatMap::iterator lgsm_it = lgs_map.begin();
        lgsm_it != lgs_map.end(); ++lgsm_it)
      {
        LogGeneralizerStatValue::GuardT lgsv_guard(lgsm_it->second->lock);
        if (deferred && intime >= lgsm_it->second->start_clear_timestamp)
        {
          continue;
        }
        lgsm_it->second->map[cmp_key] += cmp_value;
      }
    }
  }
}

static
bool
do_cmp_stat_save_(
  Commons::Postgres::Connection_var& connection,
  const CmpStatTraits::CollectorType::KeyT& outer_key,
  const CmpStatTraits::CollectorType::DataT& collector
)
{
  typedef CmpStatTraits::CollectorType CollectorT;

  if (collector.empty())
  {
    return true;
  }

  bool successful = false;

  const char sql_stmt[] =
    "SELECT adserver.fill_snapshot_cmp($1::adserver.snapshot_cmp_t[])";

  Commons::Postgres::Statement_var stmt =
    new Commons::Postgres::Statement(sql_stmt);

  Commons::Postgres::ObjectVector records_to_db;
  records_to_db.reserve(collector.size());

  for (CollectorT::DataT::const_iterator it = collector.begin();
    it != collector.end(); ++it)
  {
    CmpStatPgRecord *cmp_stat_pg_record =
      new CmpStatPgRecord(outer_key, it->first, it->second);
    records_to_db.push_back(cmp_stat_pg_record);
  }

  stmt->set_array(1, records_to_db);

  AdServer::Commons::Postgres::ResultSet_var res =
    connection->execute_statement(stmt, true);

  successful = true;

  return successful;
}

void
CmpStatDbSaver::save_i_(
  CollectorT& collector,
  Generics::Time& outtime
)
{
  Commons::Postgres::Connection_var connection;

  try
  {
    {
      LogGeneralizerStatMapBundle& lgsm_bundle = *output_;

      LogGeneralizerStatMapBundle::ReadGuardT lgsm_guard(lgsm_bundle.lock);

      outtime = Generics::Time::get_time_of_day();

      update_cmp_log_gen_stat(collector, lgsm_bundle);
    }

    std::ostringstream err_oss;

    try
    {
      connection = pg_conn_factory_->get_connection();

      for (CollectorT::const_iterator it = collector.begin();
        it != collector.end(); ++it)
      {
        if (!do_save_(connection, it->first, it->second))
        {
          err_oss << __PRETTY_FUNCTION__ << ": Error: Can't save data to "
                  << "database (sdate = " << it->first << ')';
        }
      }
    }
    catch (const DbConnectionFactory::NotActive&)
    {
      Stream::Error es;
      es << FNS << ": DB is not active. Operation will be deferred.";
      throw OperationDeferred(es);
    }

    if (!err_oss.str().empty())
    {
      throw Exception(err_oss.str(), LOG_GEN_DB_ERR_CODE_1);
    }
  }
  catch (const Exception&)
  {
    throw;
  }
  catch (const AdServer::Commons::Postgres::Exception& ex)
  {
    connection->bad();

    Stream::Error es;
    es << __PRETTY_FUNCTION__ << ": Caught AdServer::Commons::Postgres::"
      "Exception. : " << ex.what();
    throw Exception(es, LOG_GEN_DB_ERR_CODE_1);
  }
  catch (const eh::Exception& ex)
  {
    connection->bad();

    Stream::Error es;
    es << __PRETTY_FUNCTION__ << ": Caught eh::Exception. "
      ": " << ex.what();
    throw Exception(es, LOG_GEN_IMPL_ERR_CODE_3);
  }
}

bool
CmpStatDbSaver::do_save_(
  Commons::Postgres::Connection_var& connection,
  const CollectorT::KeyT& outer_key,
  const CollectorT::DataT& collector
)
{
  return do_cmp_stat_save_(connection, outer_key, collector);
}

void
DeferredCmpStatDbSaver::save_i_(
  CollectorT& collector,
  const Generics::Time& intime,
  Generics::Time& outtime
)
{
  Commons::Postgres::Connection_var connection;

  try
  {
    {
      try
      {
        connection = pg_conn_factory_->get_connection();
      }
      catch (const DbConnectionFactory::NotActive&)
      {
        Stream::Error es;
        es << FNS << ": DB is not active. Operation will be deferred.";
        throw DbConnectionFactory::NotActive(es.str());
      }

      LogGeneralizerStatMapBundle& lgsm_bundle = *output_;

      LogGeneralizerStatMapBundle::ReadGuardT lgsm_guard(lgsm_bundle.lock);

      outtime = Generics::Time::get_time_of_day();

      update_cmp_log_gen_stat(collector, lgsm_bundle, intime, true);
    }

    std::ostringstream err_oss;

    for (CollectorT::const_iterator it = collector.begin();
      it != collector.end(); ++it)
    {
      if (!do_save_(connection, it->first, it->second))
      {
        err_oss << __PRETTY_FUNCTION__ << ": Error: Can't save data to "
          "database (sdate = " << it->first << ") ";
      }
    }

    if (!err_oss.str().empty())
    {
      throw Exception(err_oss.str(), LOG_GEN_DB_ERR_CODE_1);
    }
  }
  catch (const DbConnectionFactory::NotActive&)
  {
    throw;
  }
  catch (const Exception&)
  {
    throw;
  }
  catch (const AdServer::Commons::Postgres::Exception& ex)
  {
    connection->bad();

    Stream::Error es;
    es << __PRETTY_FUNCTION__ << ": Caught AdServer::Commons::Postgres::"
      "Exception. : " << ex.what();
    throw Exception(es, LOG_GEN_DB_ERR_CODE_1);
  }
  catch (const eh::Exception& ex)
  {
    connection->bad();

    Stream::Error es;
    es << __PRETTY_FUNCTION__ << ": Caught eh::Exception. "
      ": " << ex.what();
    throw Exception(es, LOG_GEN_IMPL_ERR_CODE_3);
  }
}

bool
DeferredCmpStatDbSaver::do_save_(
  Commons::Postgres::Connection_var& connection,
  const CollectorT::KeyT& outer_key,
  const CollectorT::DataT& collector
)
{
  return do_cmp_stat_save_(connection, outer_key, collector);
}

void
CmpStatPgCsvSaver::pg_save_i_(
  CollectorT& collector,
  Generics::Time& outtime
)
{
  Commons::Postgres::Connection_var connection;

  try
  {
    {
      LogGeneralizerStatMapBundle& lgsm_bundle = *output_;

      LogGeneralizerStatMapBundle::ReadGuardT lgsm_guard(lgsm_bundle.lock);

      outtime = Generics::Time::get_time_of_day();

      update_cmp_log_gen_stat(collector, lgsm_bundle);
    }

    std::ostringstream err_oss;

    try
    {
      connection = pg_conn_factory_->get_connection();

      for (CollectorT::const_iterator it = collector.begin();
        it != collector.end(); ++it)
      {
        if (!do_save_(connection, it->first, it->second))
        {
          err_oss << __PRETTY_FUNCTION__ << ": Error: Can't save data to "
            "database (sdate = " << it->first << ')';
        }
      }
    }
    catch (const DbConnectionFactory::NotActive&)
    {
      Stream::Error es;
      es << FNS << ": DB is not active. Operation will be deferred.";
      throw OperationDeferred(es);
    }

    if (!err_oss.str().empty())
    {
      throw PgException(err_oss.str(), LOG_GEN_DB_ERR_CODE_1);
    }
  }
  catch (const Exception&)
  {
    throw;
  }
  catch (const AdServer::Commons::Postgres::Exception& ex)
  {
    connection->bad();

    Stream::Error es;
    es << __PRETTY_FUNCTION__ << ": Caught AdServer::Commons::Postgres::"
      "Exception. : " << ex.what();
    throw PgException(es, LOG_GEN_DB_ERR_CODE_1);
  }
  catch (const eh::Exception& ex)
  {
    connection->bad();

    Stream::Error es;
    es << __PRETTY_FUNCTION__ << ": Caught eh::Exception. "
      ": " << ex.what();
    throw PgException(es, LOG_GEN_IMPL_ERR_CODE_3);
  }
}

bool
CmpStatPgCsvSaver::do_save_(
  Commons::Postgres::Connection_var& connection,
  const CollectorT::KeyT& outer_key,
  const CollectorT::DataT& collector
)
{
  return do_cmp_stat_save_(connection, outer_key, collector);
}

void
DeferredCmpStatPgCsvSaver::pg_save_i_(
  CollectorT& collector,
  const Generics::Time& intime,
  Generics::Time& outtime
)
{
  Commons::Postgres::Connection_var connection;

  try
  {
    {
      try
      {
        connection = pg_conn_factory_->get_connection();
      }
      catch (const DbConnectionFactory::NotActive&)
      {
        Stream::Error es;
        es << FNS << ": DB is not active. Operation will be deferred.";
        throw DbConnectionFactory::NotActive(es.str());
      }

      LogGeneralizerStatMapBundle& lgsm_bundle = *output_;

      LogGeneralizerStatMapBundle::ReadGuardT lgsm_guard(lgsm_bundle.lock);

      outtime = Generics::Time::get_time_of_day();

      update_cmp_log_gen_stat(collector, lgsm_bundle, intime, true);
    }

    std::ostringstream err_oss;

    for (CollectorT::const_iterator it = collector.begin();
      it != collector.end(); ++it)
    {
      if (!do_save_(connection, it->first, it->second))
      {
        err_oss << __PRETTY_FUNCTION__ << ": Error: Can't save data to "
          "database (sdate = " << it->first << ") ";
      }
    }

    if (!err_oss.str().empty())
    {
      throw PgException(err_oss.str(), LOG_GEN_DB_ERR_CODE_1);
    }
  }
  catch (const DbConnectionFactory::NotActive&)
  {
    throw;
  }
  catch (const Exception&)
  {
    throw;
  }
  catch (const AdServer::Commons::Postgres::Exception& ex)
  {
    connection->bad();

    Stream::Error es;
    es << __PRETTY_FUNCTION__ << ": Caught AdServer::Commons::Postgres::"
      "Exception. : " << ex.what();
    throw PgException(es, LOG_GEN_DB_ERR_CODE_1);
  }
  catch (const eh::Exception& ex)
  {
    connection->bad();

    Stream::Error es;
    es << __PRETTY_FUNCTION__ << ": Caught eh::Exception. "
      ": " << ex.what();
    throw PgException(es, LOG_GEN_IMPL_ERR_CODE_3);
  }
}

bool
DeferredCmpStatPgCsvSaver::do_save_(
  Commons::Postgres::Connection_var& connection,
  const CollectorT::KeyT& outer_key,
  const CollectorT::DataT& collector
)
{
  return do_cmp_stat_save_(connection, outer_key, collector);
}

} // namespace LogProcessing
} // namespace AdServer

