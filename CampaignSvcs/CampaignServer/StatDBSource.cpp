#include "ExecutionTimeTracer.hpp"
#include "StatDBSource.hpp"

namespace Aspect
{
  const char STAT_DB_SOURCE[] = "StatDBSource";
}

namespace AdServer
{
namespace CampaignSvcs
{
  StatDBSource::StatDBSource(
    Logging::Logger* logger,
    Commons::Postgres::ConnectionPool* pool,
    unsigned long server_id,
    const CORBACommons::CorbaObjectRefList& stat_provider_refs)
    noexcept
    : logger_(ReferenceCounting::add_ref(logger)),
      pg_pool_(ReferenceCounting::add_ref(pool)),
      server_id_(server_id)
  {
    CORBACommons::CorbaClientAdapter_var corba_client_adapter(
      new CORBACommons::CorbaClientAdapter());

    for(CORBACommons::CorbaObjectRefList::const_iterator ref_it =
          stat_provider_refs.begin();
        ref_it != stat_provider_refs.end(); ++ref_it)
    {
      stat_providers_.push_back(LogGeneralizerRef(
        corba_client_adapter.in(), *ref_it));
    }
  }

  StatDBSource::Stat_var
  StatDBSource::update(
    Stat* /*stat*/,
    bool& full_synch_required,
    const Generics::Time& now)
    /*throw(Exception)*/
  {
    full_synch_required = false;
    Stat_var campaign_stat;

    try
    {
      for(LogGeneralizerRefList::iterator ref_it =
            stat_providers_.begin();
          ref_it != stat_providers_.end(); ++ref_it)
      {
        (*ref_it)->stop_stat_upload(server_id_);
      }
    }
    catch(const LogGeneralizerRef::Exception& ex)
    {
      full_synch_required = true;

      logger_->sstream(
        Logging::Logger::ERROR,
        Aspect::STAT_DB_SOURCE,
        "ADS-ICON-9") << __func__ <<
        ": can't do LogGeneralizer::stop_creative_stat_upload: " << ex.what();
    }
    catch(const CORBA::SystemException& ex)
    {
      full_synch_required = true;

      logger_->sstream(
        Logging::Logger::ERROR,
        Aspect::STAT_DB_SOURCE,
        "ADS-ICON-9") << __func__ <<
        ": can't do LogGeneralizer::stop_creative_stat_upload: " << ex;
    }

    bool db_update_success = false;

    try
    {
      campaign_stat = query_db_stats_(now);
      db_update_success = campaign_stat.in();
    }
    catch(const eh::Exception& ex)
    {
      logger_->sstream(
        Logging::Logger::ERROR,
        Aspect::STAT_DB_SOURCE,
        "ADS-DB-6000") << __func__ <<
        ": can't receive stats from DB: " << ex.what();
    }

    for(LogGeneralizerRefList::reverse_iterator ref_it =
          stat_providers_.rbegin();
        ref_it != stat_providers_.rend(); ++ref_it)
    {
      try
      {
        (*ref_it)->start_stat_upload(server_id_, db_update_success);
      }
      catch(const AdServer::LogProcessing::LogGeneralizer::CollectionNotStarted&)
      {
        full_synch_required = true;
      }
      catch(const LogGeneralizerRef::Exception& ex)
      {
        logger_->sstream(
          Logging::Logger::ERROR,
          Aspect::STAT_DB_SOURCE,
          "ADS-IMPL-6069") << __func__ <<
          ": can't do LogGeneralizer::start_creative_stat_upload: " << ex.what();
      }
      catch(const CORBA::SystemException& ex)
      {
        full_synch_required = true;

        logger_->sstream(
          Logging::Logger::ERROR,
          Aspect::STAT_DB_SOURCE,
          "ADS-ICON-9") << __func__ <<
          ": can't do LogGeneralizer::start_creative_stat_upload: " << ex;
      }
    }

    return campaign_stat;
  }

  StatDBSource::Stat_var
  StatDBSource::query_db_stats_(const Generics::Time& now)
    /*throw(Exception)*/
  {
    ExecutionTimeTracer db_stats_timer(
      __func__, Aspect::STAT_DB_SOURCE, logger_);

    Commons::Postgres::Connection_var connection;

    try
    {
      connection = pg_pool_->get_connection();
    }
    catch (const Commons::Postgres::NotActive&)
    {
      return StatDBSource::Stat_var();
    }
    catch (const Commons::Postgres::Exception& e)
    {
      Stream::Error ostr;
      ostr << __func__ << ": Commons::Postgres::Exception: " << e.what();
      throw Exception(ostr);
    }

    Stat_var result(new Stat());
    result->timestamp = now;
    result->check_time = now;

    // fill account level stats
    try
    {
      ExecutionTimeTracer db_stats_timer(__func__,
        Aspect::STAT_DB_SOURCE, logger_, "account stats");

      enum
      {
        POS_ACCOUNT_ID = 1,
        POS_ADV_AMOUNT
      };

      Commons::Postgres::Statement_var stmt = new Commons::Postgres::Statement(
        "SELECT "
          "account_id,"
          "adv_amount "
        "FROM adserver.get_snapshot_account_amounts()");

      Commons::Postgres::ResultSet_var rs = connection->execute_statement(stmt);

      while(rs->next())
      {
        Stat::Amount amount_stat;
        amount_stat.amount = rs->get_decimal<RevenueDecimal>(POS_ADV_AMOUNT);
        amount_stat.comm_amount = RevenueDecimal::ZERO;
        result->account_amounts.insert(std::make_pair(
          rs->get_number<unsigned int>(POS_ACCOUNT_ID), amount_stat));
      }
    }
    catch(const eh::Exception& ex)
    {
      pg_pool_->bad_connection(connection);
      Stream::Error ostr;
      ostr << __func__ << ": Can't query account stats, eh::Exception: " <<
        ex.what();
      throw Exception(ostr);
    }
    
    // fill campaign level stats
    try
    {
      ExecutionTimeTracer db_stats_timer(__func__,
        Aspect::STAT_DB_SOURCE, logger_, "campaign stats");

      enum
      {
        POS_CAMPAIGN_ID = 1,
        POS_ADV_AMOUNT,
        POS_ADV_COMM_AMOUNT,
        POS_DAILY_ADV_AMOUNT,
        POS_DAILY_ADV_COMM_AMOUNT
      };

      Commons::Postgres::Statement_var stmt = new Commons::Postgres::Statement(
        "SELECT campaign_id,"
          "adv_amount, "
          "adv_comm_amount,"
          "adv_daily_amount,"
          "adv_daily_comm_amount "
        "FROM adserver.get_snapshot_campaign_amounts($1)");

      stmt->set_timestamp(1, result->check_time);

      Commons::Postgres::ResultSet_var rs = connection->execute_statement(stmt);

      while(rs->next())
      {
        Stat::AmountStat amount_stat;
        amount_stat.amount = rs->get_decimal<RevenueDecimal>(POS_ADV_AMOUNT);
        amount_stat.comm_amount = rs->get_decimal<RevenueDecimal>(POS_ADV_COMM_AMOUNT);
        amount_stat.daily_amount = rs->get_decimal<RevenueDecimal>(
          POS_DAILY_ADV_AMOUNT);
        amount_stat.daily_comm_amount = rs->get_decimal<RevenueDecimal>(
          POS_DAILY_ADV_COMM_AMOUNT);
        static_cast<Stat::AmountStat&>(
          result->campaign_stats[rs->get_number<unsigned int>(POS_CAMPAIGN_ID)]) += amount_stat;
      }
    }
    catch(const eh::Exception& ex)
    {
      pg_pool_->bad_connection(connection);
      Stream::Error ostr;
      ostr << __func__ << ": Can't query campaign stats. Caught eh::Exception: " <<
        ex.what();
      throw Exception(ostr);
    }

    // fill CCG level stats
    try
    {
      ExecutionTimeTracer db_stats_timer(__func__,
        Aspect::STAT_DB_SOURCE, logger_, "ccg stats");

      enum
      {
        POS_CAMPAIGN_ID = 1,
        POS_CCG_ID,
        POS_IMPRESSIONS,
        POS_CLICKS,
        POS_ACTIONS,
        POS_ADV_AMOUNT,
        POS_ADV_COMM_AMOUNT,
        POS_DAILY_ADV_AMOUNT,
        POS_DAILY_ADV_COMM_AMOUNT
      };

      Commons::Postgres::Statement_var stmt = new Commons::Postgres::Statement(
        "SELECT "
          "ccg.campaign_id,"
          "s.ccg_id,"
          "s.imps,"
          "s.clicks,"
          "s.actions,"
          "s.adv_amount,"
          "s.adv_comm_amount,"
          "s.adv_daily_amount,"
          "s.adv_daily_comm_amount "
        "FROM adserver.get_snapshot_ccg_stats($1) s "
          "JOIN campaigncreativegroup ccg on ccg.ccg_id = s.ccg_id");

      stmt->set_timestamp(1, result->check_time);

      Commons::Postgres::ResultSet_var rs = connection->execute_statement(stmt);

      while(rs->next())
      {
        Stat::CCGStat ccg_stat;
        ccg_stat.cur_hour_amount = RevenueDecimal::ZERO;
        ccg_stat.cur_hour_comm_amount = RevenueDecimal::ZERO;
        ccg_stat.prev_hour_amount = RevenueDecimal::ZERO;
        ccg_stat.prev_hour_comm_amount = RevenueDecimal::ZERO;        
        ccg_stat.impressions = rs->get_decimal<ImpRevenueDecimal>(POS_IMPRESSIONS);
        ccg_stat.clicks = rs->get_decimal<ImpRevenueDecimal>(POS_CLICKS);
        ccg_stat.actions = rs->get_decimal<ImpRevenueDecimal>(POS_ACTIONS);
        ccg_stat.amount = rs->get_decimal<RevenueDecimal>(POS_ADV_AMOUNT);
        ccg_stat.comm_amount = rs->get_decimal<RevenueDecimal>(POS_ADV_COMM_AMOUNT);
        ccg_stat.daily_amount = rs->get_decimal<RevenueDecimal>(
          POS_DAILY_ADV_AMOUNT);
        ccg_stat.daily_comm_amount = rs->get_decimal<RevenueDecimal>(
          POS_DAILY_ADV_COMM_AMOUNT);
        result->campaign_stats[
          rs->get_number<unsigned int>(POS_CAMPAIGN_ID)].ccgs[
            rs->get_number<unsigned int>(POS_CCG_ID)] += ccg_stat;
      }
    }
    catch(const eh::Exception& ex)
    {
      pg_pool_->bad_connection(connection);
      Stream::Error ostr;
      ostr << __func__ << ": Can't query campaign stats. Caught eh::Exception: " <<
        ex.what();
      throw Exception(ostr);
    }

    // query creative stats
    try
    {
      ExecutionTimeTracer db_stats_timer(__func__,
        Aspect::STAT_DB_SOURCE, logger_, "creative stats");

      enum
      {
        POS_CAMPAIGN_ID = 1,
        POS_CCG_ID,
        POS_CC_ID,
        POS_IMPRESSIONS,
        POS_CLICKS,
        POS_ACTIONS
      };

      Commons::Postgres::Statement_var stmt = new Commons::Postgres::Statement(
        "SELECT "
          "ccg.campaign_id, "
          "s.ccg_id, "
          "s.cc_id, "
          "s.imps, "
          "s.clicks, "
          "s.actions "
        "FROM adserver.get_snapshot_cc_stats() s "
          "JOIN campaigncreativegroup ccg on ccg.ccg_id = s.ccg_id");

      Commons::Postgres::ResultSet_var rs = connection->execute_statement(stmt);

      while(rs->next())
      {
        Stat::CreativeStat creative_stat;
        creative_stat.impressions = rs->get_decimal<ImpRevenueDecimal>(POS_IMPRESSIONS);
        creative_stat.clicks = rs->get_decimal<ImpRevenueDecimal>(POS_CLICKS);
        creative_stat.actions = rs->get_decimal<ImpRevenueDecimal>(POS_ACTIONS);

        result->campaign_stats[
          rs->get_number<unsigned int>(POS_CAMPAIGN_ID)].ccgs[
            rs->get_number<unsigned int>(POS_CCG_ID)].creatives[
              rs->get_number<unsigned int>(POS_CC_ID)] = creative_stat;
      }
    }
    catch(const eh::Exception& ex)
    {
      pg_pool_->bad_connection(connection);
      Stream::Error ostr;
      ostr << __func__ << ": Can't query creative stats, eh::Exception: " <<
        ex.what();
      throw Exception(ostr);
    }

    // query publisher stats
    try
    {
      ExecutionTimeTracer db_stats_timer(__func__,
        Aspect::STAT_DB_SOURCE, logger_, "publisher stats");

      enum
      {
        POS_CAMPAIGN_ID = 1,
        POS_CCG_ID,
        POS_PUBLISHER_ACCOUNT_ID,
        POS_ADV_AMOUNT,
        POS_ADV_DAILY_AMOUNT
      };

      Commons::Postgres::Statement_var stmt = new Commons::Postgres::Statement(
        "SELECT "
          "ccg.campaign_id,"
          "s.o_ccg_id,"
          "s.o_pub_account_id,"
          "s.o_adv_amount,"
          "s.o_daily_adv_amount "
        "FROM adserver.get_snapshot_publisher_stats($1) s "
          "JOIN campaigncreativegroup ccg on ccg.ccg_id = s.o_ccg_id");

      stmt->set_timestamp(1, result->check_time);

      Commons::Postgres::ResultSet_var rs = connection->execute_statement(stmt);

      while(rs->next())
      {
        Stat::CCGStat::PublisherStat publisher_stat;
        publisher_stat.amount = rs->get_decimal<RevenueDecimal>(POS_ADV_AMOUNT);
        publisher_stat.daily_amount = rs->get_decimal<RevenueDecimal>(POS_ADV_DAILY_AMOUNT);

        result->campaign_stats[
          rs->get_number<unsigned int>(POS_CAMPAIGN_ID)].ccgs[
            rs->get_number<unsigned int>(POS_CCG_ID)].publisher_amounts[
              rs->get_number<unsigned int>(POS_PUBLISHER_ACCOUNT_ID)] = publisher_stat;
      }
    }
    catch(const eh::Exception& ex)
    {
      pg_pool_->bad_connection(connection);
      Stream::Error ostr;
      ostr << __func__ << ": Can't query publisher stats, eh::Exception: " <<
        ex.what();
      throw Exception(ostr);
    }

    // query ccg tag stats
    try
    {
      ExecutionTimeTracer db_stats_timer(__func__,
        Aspect::STAT_DB_SOURCE, logger_, "ccg tag stats");

      enum
      {
        POS_CAMPAIGN_ID = 1,
        POS_CCG_ID,
        POS_TAG_ID,
        POS_ADV_AMOUNT,
        POS_ADV_COMM_AMOUNT,
        POS_POT_ISP_PUB_AMOUNT,
        POS_POT_ADV_AMOUNT,
        POS_POT_ADV_COMM_AMOUNT,
        POS_PREV_ADV_AMOUNT,
        POS_PREV_ADV_COMM_AMOUNT,
        POS_POT_PREV_ISP_PUB_AMOUNT,
        POS_POT_PREV_ADV_AMOUNT,
        POS_POT_PREV_ADV_COMM_AMOUNT
      };

      Commons::Postgres::Statement_var stmt = new Commons::Postgres::Statement(
        "SELECT campaign_id,"
          "ccg_id,"
          "tag_id,"
          "adv_amount,"
          "adv_invoice_comm_amount,"
          "pot_pub_isp_amount,"
          "pot_adv_amount,"
          "pot_adv_invoice_comm_amount,"
          "pr_adv_amount,"
          "pr_adv_invoice_comm_amount,"
          "pr_pot_pub_isp_amount,"
          "pr_pot_adv_amount,"
          "pr_pot_adv_invoice_comm_amount "
        "FROM adserver.get_snapshot_ccg_tag_stats($1)"
        );

      stmt->set_timestamp(1, result->check_time);

      Commons::Postgres::ResultSet_var rs = connection->execute_statement(stmt);

      while(rs->next())
      {
        // fill potential hour amounts
        Stat::CCGStat::TagStat tag_stat;
        tag_stat.current_hour_stat.isp_pub_amount = rs->get_decimal<RevenueDecimal>(
          POS_POT_ISP_PUB_AMOUNT);
        tag_stat.current_hour_stat.adv_amount = rs->get_decimal<RevenueDecimal>(
          POS_POT_ADV_AMOUNT);
        tag_stat.current_hour_stat.adv_comm_amount = rs->get_decimal<RevenueDecimal>(
          POS_POT_ADV_COMM_AMOUNT);
        tag_stat.prev_hour_stat.isp_pub_amount = rs->get_decimal<RevenueDecimal>(
          POS_POT_PREV_ISP_PUB_AMOUNT);
        tag_stat.prev_hour_stat.adv_amount = rs->get_decimal<RevenueDecimal>(
          POS_POT_PREV_ADV_AMOUNT);
        tag_stat.prev_hour_stat.adv_comm_amount = rs->get_decimal<RevenueDecimal>(
          POS_POT_PREV_ADV_COMM_AMOUNT);

        Stat::CCGStat& ccg_stat = result->campaign_stats[
          rs->get_number<unsigned int>(POS_CAMPAIGN_ID)].ccgs[
            rs->get_number<unsigned int>(POS_CCG_ID)];

        ccg_stat.tag_stats[
          rs->get_number<unsigned int>(POS_TAG_ID)] = tag_stat;
        
        // fill real hour amounts
        ccg_stat.prev_hour_amount += rs->get_decimal<RevenueDecimal>(
          POS_PREV_ADV_AMOUNT);
        ccg_stat.prev_hour_comm_amount += rs->get_decimal<RevenueDecimal>(
          POS_PREV_ADV_COMM_AMOUNT);
        ccg_stat.cur_hour_amount += rs->get_decimal<RevenueDecimal>(
          POS_ADV_AMOUNT);
        ccg_stat.cur_hour_comm_amount += rs->get_decimal<RevenueDecimal>(
          POS_ADV_COMM_AMOUNT);
      }
    }
    catch(const eh::Exception& ex)
    {
      pg_pool_->bad_connection(connection);
      Stream::Error ostr;
      ostr << __func__ << ": Can't query ccg tag stats, eh::Exception: " <<
        ex.what();
      throw Exception(ostr);
    }

    // query ccg ctr reset stats
    try
    {
      ExecutionTimeTracer db_stats_timer(__func__,
        Aspect::STAT_DB_SOURCE, logger_, "ccg ctr reset stats");

      enum
      {
        POS_CAMPAIGN_ID = 1,
        POS_CCG_ID,
        POS_CTR_RESET_ID,
        POS_IMPRESSIONS
      };

      Commons::Postgres::Statement_var stmt = new Commons::Postgres::Statement(
        "SELECT ccg.campaign_id, s.ccg_id, s.ctr_reset_id, s.imps "
        "FROM adserver.get_snapshot_ccg_ctr_reset_stats() s "
          "JOIN campaigncreativegroup ccg on ccg.ccg_id = s.ccg_id");

      Commons::Postgres::ResultSet_var rs = connection->execute_statement(stmt);

      while(rs->next())
      {
        Stat::CCGStat::CtrResetStat ctr_reset_stat;
        ctr_reset_stat.impressions = rs->get_decimal<ImpRevenueDecimal>(POS_IMPRESSIONS);

        result->campaign_stats[
          rs->get_number<unsigned int>(POS_CAMPAIGN_ID)].ccgs[
            rs->get_number<unsigned int>(POS_CCG_ID)].ctr_reset_stats[
              rs->get_number<unsigned int>(POS_CTR_RESET_ID)] += ctr_reset_stat;
      }
    }
    catch(const eh::Exception& ex)
    {
      pg_pool_->bad_connection(connection);
      Stream::Error ostr;
      ostr << __func__ << ": Can't query ccg ctr reset stats, eh::Exception: " <<
        ex.what();
      throw Exception(ostr);
    }

    return result;
  }
}
}
