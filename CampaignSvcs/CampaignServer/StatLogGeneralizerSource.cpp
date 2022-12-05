#include <Commons/CorbaAlgs.hpp>

#include "StatLogGeneralizerSource.hpp"

namespace Aspect
{
  const char STAT_LOGGENERALIZER_SOURCE[] = "StatLogGeneralizerSource";
}

namespace AdServer
{
namespace CampaignSvcs
{
  StatLogGeneralizerSource::StatLogGeneralizerSource(
    Logging::Logger* logger,
    unsigned long server_id,
    const CORBACommons::CorbaObjectRefList& stat_provider_refs)
    noexcept
    : logger_(ReferenceCounting::add_ref(logger)),
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

  StatSource::Stat_var
  StatLogGeneralizerSource::update(
    StatSource::Stat* stat,
    bool& full_synch_required,
    const Generics::Time& now)
    /*throw(Exception)*/
  {
    static const char* FUN = "StatLogGeneralizerSource::update()";
    full_synch_required = false;

    StatSource::Stat_var sum_stat(new StatSource::Stat());
    sum_stat->check_time = now;

    try
    {
      for(LogGeneralizerRefList::iterator ref_it =
            stat_providers_.begin();
          ref_it != stat_providers_.end(); ++ref_it)
      {
        AdServer::LogProcessing::StatInfo_var stats_portion =
          (*ref_it)->get_stat_info(server_id_, true);

        StatSource::Stat_var stat_portion = convert_stats_update_(
          *stats_portion, now);

        sum_stat->add(*stat_portion);
      }
    }
    catch(const AdServer::LogProcessing::LogGeneralizer::CollectionNotStarted&)
    {
      logger_->sstream(
        Logging::Logger::TRACE,
        Aspect::STAT_LOGGENERALIZER_SOURCE) << FUN <<
        ": Caught LogGeneralizer::CollectionNotStarted";

      full_synch_required = true;
    }
    catch(const LogGeneralizerRef::Exception& ex)
    {
      logger_->sstream(
        Logging::Logger::ERROR,
        Aspect::STAT_LOGGENERALIZER_SOURCE,
        "ADS-ICON-6000") << FUN <<
        ": can't do LogGeneralizer::get_stat_info: " << ex.what();
    }
    catch(const CORBA::SystemException& ex)
    {
      logger_->sstream(
        Logging::Logger::ERROR,
        Aspect::STAT_LOGGENERALIZER_SOURCE,
        "ADS-ICON-6000") << FUN <<
        ": can't do LogGeneralizer::get_stat_info: " << ex;
    }

    if(logger_->log_level() >= Logging::Logger::DEBUG)
    {
      Stream::Dynamic ostr(4096);
      ostr << "Received from LogGeneralizer stats:" << std::endl;
      sum_stat->print(ostr, "  ");
      logger_->log(
        ostr.str(),
        Logging::Logger::DEBUG,
        Aspect::STAT_LOGGENERALIZER_SOURCE);
    }

    if(stat)
    {
      stat->add(*sum_stat);

      return StatSource::Stat_var(ReferenceCounting::add_ref(stat));
    }

    return sum_stat;
  }

  StatSource::Stat_var
  StatLogGeneralizerSource::convert_stats_update_(
    const AdServer::LogProcessing::StatInfo& update,
    const Generics::Time& now)
    /*throw(Exception)*/
  {
    StatSource::Stat_var stat(new StatSource::Stat());
    stat->check_time = now;

    for(CORBA::ULong i = 0; i < update.campaign_stats.length(); ++i)
    {
      const AdServer::LogProcessing::CampaignStatInfo& campaign_stat_info =
        update.campaign_stats[i];

      Generics::Time sdate = CorbaAlgs::unpack_time(campaign_stat_info.sdate);
      Generics::Time adv_sdate = CorbaAlgs::unpack_time(campaign_stat_info.adv_sdate);
      Generics::Time tz_offset = adv_sdate - sdate;
      RevenueDecimal amount = CorbaAlgs::unpack_decimal<RevenueDecimal>(
        campaign_stat_info.adv_amount);
      RevenueDecimal comm_amount = CorbaAlgs::unpack_decimal<RevenueDecimal>(
        campaign_stat_info.adv_comm_amount);
      Generics::ExtendedTime acc_ex_now = (now + tz_offset).get_gm_time();
      Generics::ExtendedTime ex_adv_sdate = adv_sdate.get_gm_time();
      bool current_day = acc_ex_now.get_date() == ex_adv_sdate.get_date();
      bool current_hour = current_day && acc_ex_now.tm_hour == ex_adv_sdate.tm_hour;
      bool prev_hour = current_day && acc_ex_now.tm_hour == ex_adv_sdate.tm_hour + 1;

      // fill campaigns
      Stat::CCGStat ccg_stat;
      ccg_stat.prev_hour_amount = RevenueDecimal::ZERO;
      ccg_stat.prev_hour_comm_amount = RevenueDecimal::ZERO;
      ccg_stat.cur_hour_amount = RevenueDecimal::ZERO;
      ccg_stat.cur_hour_comm_amount = RevenueDecimal::ZERO;

      if(current_day)
      {
        ccg_stat.daily_amount = amount;
        ccg_stat.daily_comm_amount = comm_amount;
      }
      ccg_stat.amount = amount;
      ccg_stat.comm_amount = comm_amount;

      for(CORBA::ULong cc_i = 0;
          cc_i < campaign_stat_info.creative_stats.length(); ++cc_i)
      {
        const AdServer::LogProcessing::CreativeStatInfo& creative_stat_info =
          campaign_stat_info.creative_stats[cc_i];

        ccg_stat.impressions += creative_stat_info.impressions;
        ccg_stat.clicks += creative_stat_info.clicks;
        ccg_stat.actions += creative_stat_info.actions;

        Stat::CreativeStat creative_stat;
        creative_stat.impressions = creative_stat_info.impressions;
        creative_stat.clicks = creative_stat_info.clicks;
        creative_stat.actions = creative_stat_info.actions;
        ccg_stat.creatives[creative_stat_info.cc_id] += creative_stat;
      }

      for(CORBA::ULong pub_i = 0;
          pub_i < campaign_stat_info.publisher_amounts.length(); ++pub_i)
      {
        RevenueDecimal pub_amount = CorbaAlgs::unpack_decimal<RevenueDecimal>(
          campaign_stat_info.publisher_amounts[pub_i].adv_amount);

        Stat::CCGStat::PublisherStat pub_stat;

        if(current_day)
        {
          pub_stat.daily_amount += pub_amount;
        }
        pub_stat.amount += pub_amount;

        ccg_stat.publisher_amounts[
          campaign_stat_info.publisher_amounts[pub_i].publisher_account_id] +=
          pub_stat;
      }

      if(current_hour || prev_hour)
      {
        RevenueDecimal& sum_hour_amount =
          current_hour ? ccg_stat.cur_hour_amount :
          ccg_stat.prev_hour_amount;

        sum_hour_amount += CorbaAlgs::unpack_decimal<RevenueDecimal>(
          campaign_stat_info.adv_amount);

        RevenueDecimal& sum_hour_comm_amount =
          current_hour ? ccg_stat.cur_hour_comm_amount :
          ccg_stat.prev_hour_comm_amount;

        sum_hour_comm_amount += CorbaAlgs::unpack_decimal<RevenueDecimal>(
          campaign_stat_info.adv_comm_amount);

        for(CORBA::ULong tag_i = 0;
            tag_i < campaign_stat_info.tag_amounts.length(); ++tag_i)
        {
          const AdServer::LogProcessing::TagAmountInfo& tag_stat_info =
            campaign_stat_info.tag_amounts[tag_i];

          Stat::CCGStat::TagStat tag_stat;
          Stat::CCGStat::CCGStat::TagHourStat& hour_stat =
            current_hour ? tag_stat.current_hour_stat : tag_stat.prev_hour_stat;

          hour_stat.isp_pub_amount = CorbaAlgs::unpack_decimal<RevenueDecimal>(
            tag_stat_info.pub_isp_amount);
          hour_stat.adv_amount = CorbaAlgs::unpack_decimal<RevenueDecimal>(
            tag_stat_info.adv_amount);
          hour_stat.adv_comm_amount = CorbaAlgs::unpack_decimal<RevenueDecimal>(
            tag_stat_info.adv_comm_amount);

          ccg_stat.tag_stats[tag_stat_info.tag_id] += tag_stat;
        }

        for(CORBA::ULong ctr_reset_i = 0;
            ctr_reset_i < campaign_stat_info.ctr_reset_stats.length();
            ++ctr_reset_i)
        {
          const AdServer::LogProcessing::CtrResetStatInfo& ctr_reset_stat_info =
            campaign_stat_info.ctr_reset_stats[ctr_reset_i];

          Stat::CCGStat::CtrResetStat ctr_reset_stat;
          ctr_reset_stat.impressions = ctr_reset_stat_info.impressions;
          ccg_stat.ctr_reset_stats[
            ctr_reset_stat_info.ctr_reset_id] += ctr_reset_stat;
        }
      }

      Stat::CampaignStat& campaign_stat = stat->campaign_stats[campaign_stat_info.campaign_id];
      campaign_stat.ccgs[campaign_stat_info.ccg_id] += ccg_stat;
      campaign_stat.amount += amount;
      campaign_stat.comm_amount += comm_amount;
      if(current_day)
      {
        campaign_stat.daily_amount += amount;
        campaign_stat.daily_comm_amount += comm_amount;
      }

      // fill account_amounts
      Stat::Amount account_amount;
      account_amount.amount = CorbaAlgs::unpack_decimal<RevenueDecimal>(
        campaign_stat_info.adv_account_amount);
      account_amount.comm_amount = CorbaAlgs::unpack_decimal<RevenueDecimal>(
        campaign_stat_info.adv_payable_comm_amount);
      Stat::AccountAmountMap::iterator acc_it = stat->account_amounts.find(
        campaign_stat_info.adv_account_id);
      if(acc_it == stat->account_amounts.end())
      {
        stat->account_amounts.insert(std::make_pair(
          campaign_stat_info.adv_account_id, account_amount));
      }
      else
      {
        acc_it->second += account_amount;
      }
    }

    return stat;
  }
}
}
