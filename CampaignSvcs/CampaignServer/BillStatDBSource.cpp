#include <CampaignSvcs/CampaignCommons/CampaignCommons.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignSvcsVersionAdapter.hpp>

#include "ExecutionTimeTracer.hpp"
#include "BillStatDBSource.hpp"

namespace Aspect
{
  const char BILL_STAT_DB_SOURCE[] = "BillStatDBSource";
}

namespace AdServer
{
namespace CampaignSvcs
{
  namespace
  {
    typedef std::map<unsigned long, RevenueDecimal> TotalAmountMap;
    typedef std::map<unsigned long, BillStatSource::Stat::AmountCount> TotalAmountCountMap;

    template<typename AmountMapType>
    void
    add_account_day_amount(
      AmountMapType& amounts,
      const Generics::Time& day,
      unsigned long object_id,
      const RevenueDecimal& amount)
      noexcept
    {
      auto& obj = amounts[object_id];
      auto ins = obj.day_amounts.insert(std::make_pair(day, amount));
      if(!ins.second)
      {
        ins.first->second += amount;
      }
    }

    template<typename AmountMapType>
    void
    add_amount_count_day_amount(
      AmountMapType& amounts,
      const Generics::Time& day,
      unsigned long object_id,
      const RevenueDecimal& amount,
      const ImpRevenueDecimal& imps,
      const ImpRevenueDecimal& clicks)
      noexcept
    {
      auto& obj = amounts[object_id];
      auto ins = obj.day_amount_counts.insert(
        std::make_pair(day, BillStatSource::Stat::AmountCount(amount, imps, clicks)));
      if(!ins.second)
      {
        ins.first->second.amount += amount;
        ins.first->second.imps += imps;
        ins.first->second.clicks += clicks;
      }
    }

    void
    add_account_total_amount(
      TotalAmountMap& amounts,
      unsigned long object_id,
      const RevenueDecimal& total_amount)
      noexcept
    {
      auto ins = amounts.insert(std::make_pair(object_id, total_amount));
      if(!ins.second)
      {
        ins.first->second += total_amount;
      }
    }

    void
    add_amount_count_total_amount(
      TotalAmountCountMap& amounts,
      unsigned long object_id,
      const RevenueDecimal& total_amount,
      const ImpRevenueDecimal& total_imps,
      const ImpRevenueDecimal& total_clicks)
      noexcept
    {
      auto ins = amounts.insert(std::make_pair(
        object_id,
        BillStatSource::Stat::AmountCount(total_amount, total_imps, total_clicks)));
      if(!ins.second)
      {
        ins.first->second.amount += total_amount;
        ins.first->second.imps += total_imps;
        ins.first->second.clicks += total_clicks;
      }
    }

    template<typename AmountMapType>
    void
    set_prev_day_amount(
      AmountMapType& amounts,
      TotalAmountMap& total_amounts,
      const Generics::Time& before_date)
      noexcept
    {
      for(auto it = total_amounts.begin(); it != total_amounts.end(); ++it)
      {
        BillStatSource::Stat::AmountDistribution& res = amounts[it->first];
        RevenueDecimal sum_day_amount = RevenueDecimal::ZERO;
        for(auto day_it = res.day_amounts.begin();
          day_it != res.day_amounts.end(); ++day_it)
        {
          sum_day_amount += day_it->second;
        }
        res.prev_days_amount = it->second - sum_day_amount;
        res.prev_day = before_date;
      }
    }

    template<typename AmountMapType>
    void
    set_prev_day_amount_acount(
      AmountMapType& amounts,
      TotalAmountCountMap& total_amounts,
      const Generics::Time& before_date)
      noexcept
    {
      for(auto it = total_amounts.begin(); it != total_amounts.end(); ++it)
      {
        BillStatSource::Stat::AmountCountDistribution& res = amounts[it->first];
        RevenueDecimal sum_day_amount = RevenueDecimal::ZERO;
        ImpRevenueDecimal sum_day_imps = ImpRevenueDecimal::ZERO;
        ImpRevenueDecimal sum_day_clicks = ImpRevenueDecimal::ZERO;
        for(auto day_it = res.day_amount_counts.begin();
          day_it != res.day_amount_counts.end(); ++day_it)
        {
          sum_day_amount += day_it->second.amount;
          sum_day_imps += day_it->second.imps;
          sum_day_clicks += day_it->second.clicks;
        }
        res.prev_day = before_date;
        res.prev_days_amount = it->second.amount - sum_day_amount;
        res.prev_days_imps = it->second.imps - sum_day_imps;
        res.prev_days_clicks = it->second.clicks - sum_day_clicks;
      }
    }
  }

  BillStatDBSource::BillStatDBSource(
    Logging::Logger* logger,
    Commons::Postgres::Environment* pg_env)
    /*throw(Exception)*/
  try
    : logger_(ReferenceCounting::add_ref(logger))
  {
    pg_pool_ = pg_env->create_connection_pool();
  }
  catch(const eh::Exception& ex)
  {
    Stream::Error ostr;
    ostr << "BillStatDBSource::BillStatDBSource(): caught eh::Exception: " <<
      ex.what();
    throw Exception(ostr);
  }

  BillStatDBSource::Stat_var
  BillStatDBSource::update(
    Stat* /*stat*/,
    const Generics::Time& now)
    /*throw(Exception)*/
  {
    Stat_var new_stat;

    try
    {
      new_stat = query_db_stats_(now);
    }
    catch(const eh::Exception& ex)
    {
      logger_->sstream(
        Logging::Logger::ERROR,
        Aspect::BILL_STAT_DB_SOURCE,
        "ADS-DB-6000") << __func__ <<
        ": can't receive stats from DB: " << ex.what();
    }

    return new_stat;
  }

  BillStatDBSource::Stat_var
  BillStatDBSource::query_db_stats_(const Generics::Time& now)
    /*throw(Exception)*/
  {
    static const char* FUN = "BillStatDBSource::query_db_stats_()";

    ExecutionTimeTracer db_stats_timer(
      FUN,
      Aspect::BILL_STAT_DB_SOURCE,
      logger_);

    Commons::Postgres::Connection_var connection;

    try
    {
      connection = pg_pool_->get_connection();
    }
    catch (const Commons::Postgres::NotActive&)
    {
      return Stat_var();
    }
    catch (const Commons::Postgres::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": Commons::Postgres::Exception: " << e.what();
      throw Exception(ostr);
    }

    const Generics::Time now_date = now.get_gm_time().get_date();
    const Generics::Time trunc_date = now_date - Generics::Time::ONE_DAY * 60;

    Stat_var result(new Stat());
    result->timestamp = now;

    try
    {
      ExecutionTimeTracer db_stats_timer(FUN,
        Aspect::BILL_STAT_DB_SOURCE,
        logger_,
        "amount stats");

      enum
      {
        POS_ADV_DATE = 1,
        POS_ACCOUNT_ID,
        POS_CAMPAIGN_ID,
        POS_CCG_ID,
        POS_ADV_AMOUNT,
        POS_ADV_COMM_AMOUNT,
        POS_IMPS,
        POS_CLICKS,
        POS_ACCOUNT_FLAGS
      };

      Commons::Postgres::Statement_var stmt = new Commons::Postgres::Statement(
        "SELECT "
          "adv_sdate,"
          "coalesce(case when not (at.flags & x'2'::int)::bool then "
            "ag.account_id end, ac.account_id) adv_account_id,"
          "s.campaign_id,"
          "s.ccg_id,"
          "s.adv_amount adv_amount, "
          "s.adv_comm_amount adv_comm_amount, "
          "s.imps imps, "
          "s.clicks clicks, "
          "at.flags flags "
        "FROM ("
          "SELECT sdate adv_sdate,"
            "campaign_id,"
            "ccg_id,"
            "SUM(adv_amount) adv_amount, "
            "SUM(adv_comm_amount) adv_comm_amount, "
            "SUM(imps) imps, "
            "SUM(clicks) clicks "
          "FROM adserver.snapshot_daily_cmp d "
          "GROUP BY sdate, campaign_id, ccg_id "
          "UNION ALL "
          "SELECT NULL adv_sdate,"
            "campaign_id,"
            "ccg_id,"
            "SUM(adv_amount) adv_amount, "
            "SUM(adv_comm_amount) adv_comm_amount, "
            "SUM(imps) imps, "
            "SUM(clicks) clicks "
          "FROM adserver.snapshot_total s "
          "GROUP BY campaign_id, ccg_id "
          ") s "
          "JOIN Campaign ON Campaign.campaign_id = s.campaign_id "
          "JOIN Account ac ON ac.account_id = campaign.account_id "
          "LEFT JOIN Account ag ON ag.account_id = ac.agency_account_id "
          "JOIN AccountType at on at.account_type_id = "
            "coalesce(ag.account_type_id, ac.account_type_id) "
        "WHERE (s.adv_amount <> 0 OR s.adv_comm_amount <> 0) AND "
        "Campaign.status <> 'D' AND "
        "ac.status <> 'D' AND "
        "COALESCE(ag.status, 'N') <> 'D' "
        "ORDER BY adv_sdate ASC NULLS LAST");

      TotalAmountMap account_total_amounts;
      TotalAmountCountMap campaign_total_amounts;
      TotalAmountCountMap ccg_total_amounts;

      Commons::Postgres::ResultSet_var rs = connection->execute_statement(stmt);

      while(rs->next())
      {
        const RevenueDecimal amount = rs->get_decimal<RevenueDecimal>(POS_ADV_AMOUNT);
        const RevenueDecimal comm_amount = rs->get_decimal<RevenueDecimal>(POS_ADV_COMM_AMOUNT);
        const unsigned long account_flags = rs->get_number<unsigned long>(POS_ACCOUNT_FLAGS);
        const ImpRevenueDecimal imps = rs->get_decimal<ImpRevenueDecimal>(POS_IMPS);
        const ImpRevenueDecimal clicks = rs->get_decimal<ImpRevenueDecimal>(POS_CLICKS);

        const RevenueDecimal account_amount =
          (account_flags & AccountTypeFlags::INVOICE_COMMISION ? amount + comm_amount : amount);
        const RevenueDecimal ccg_amount =
          (account_flags & AccountTypeFlags::GROSS ? amount + comm_amount : amount);

        if(amount != RevenueDecimal::ZERO ||
           comm_amount != RevenueDecimal::ZERO)
        {
          Generics::Time day = !rs->is_null(POS_ADV_DATE) ?
            rs->get_date(POS_ADV_DATE) :
            Generics::Time::ZERO;

          if(day > trunc_date)
          {
            add_account_day_amount(
              result->accounts,
              day,
              rs->get_number<unsigned long>(POS_ACCOUNT_ID),
              account_amount);

            add_amount_count_day_amount(
              result->campaigns,
              day,
              rs->get_number<unsigned long>(POS_CAMPAIGN_ID),
              ccg_amount,
              imps,
              clicks);

            add_amount_count_day_amount(
              result->ccgs,
              day,
              rs->get_number<unsigned long>(POS_CCG_ID),
              ccg_amount,
              imps,
              clicks);
          }
          else
          {
            add_account_total_amount(
              account_total_amounts,
              rs->get_number<unsigned long>(POS_ACCOUNT_ID),
              account_amount);

            add_amount_count_total_amount(
              campaign_total_amounts,
              rs->get_number<unsigned long>(POS_CAMPAIGN_ID),
              ccg_amount,
              imps,
              clicks);

            add_amount_count_total_amount(
              ccg_total_amounts,
              rs->get_number<unsigned long>(POS_CCG_ID),
              ccg_amount,
              imps,
              clicks);
          }
        }
      }

      // fill truncated by date amounts
      set_prev_day_amount(
        result->accounts,
        account_total_amounts,
        trunc_date);

      set_prev_day_amount_acount(
        result->campaigns,
        campaign_total_amounts,
        trunc_date);

      set_prev_day_amount_acount(
        result->ccgs,
        ccg_total_amounts,
        trunc_date);
    }
    catch(const eh::Exception& ex)
    {
      pg_pool_->bad_connection(connection);
      Stream::Error ostr;
      ostr << FUN << ": Can't query account stats, eh::Exception: " <<
        ex.what();
      throw Exception(ostr);
    }

    return result;
  }
}
}
