#include <Commons/CorbaAlgs.hpp>

#include "BillStatServerSource.hpp"

namespace Aspect
{
  const char BILL_STAT_SERVER_SOURCE[] = "BillStatServerSource";
}

namespace AdServer
{
namespace CampaignSvcs
{
  BillStatServerSource::BillStatServerSource(
    Logging::Logger* logger,
    unsigned long server_id,
    const CORBACommons::CorbaObjectRefList& campaign_server_refs)
    noexcept
    : logger_(ReferenceCounting::add_ref(logger)),
      server_id_(server_id)
  {
    CORBACommons::CorbaClientAdapter_var corba_client_adapter(
      new CORBACommons::CorbaClientAdapter());

    campaign_servers_.reset(new CampaignServerPool(
      campaign_server_refs,
      corba_client_adapter,
      CORBACommons::ChoosePolicyType::PT_PERSISTENT,
      Generics::Time(10) // timeout
      ));
  }

  BillStatSource::Stat_var
  BillStatServerSource::update(
    BillStatSource::Stat* /*stat*/,
    const Generics::Time& now)
    /*throw(Exception)*/
  {
    static const char* FUN = "BillStatServerSource::update()";

    try
    {
      while(true)
      {
        CampaignServerPool::ObjectHandlerType campaign_server =
          campaign_servers_->get_object<Exception>(
            logger_,
            Logging::Logger::ERROR,
            Aspect::BILL_STAT_SERVER_SOURCE,
            "ADS_ECON-6",
            server_id_,
            server_id_);

        try
        {
          AdServer::CampaignSvcs::BillStatInfo_var bill_stat =
            campaign_server->get_bill_stat();

          return convert_update_(*bill_stat, now);
        }
        catch(const CORBA::SystemException& e)
        {
          Stream::Error ostr;
          ostr << FUN << ": caught CORBA exception on update config: " << e;

          campaign_server.release_bad(ostr.str());

          logger_->log(ostr.str(),
            Logging::Logger::ERROR,
            Aspect::BILL_STAT_SERVER_SOURCE,
            "ADS-ECON-6");
        }
        catch(const AdServer::CampaignSvcs::CampaignServer::NotReady& )
        {
          Stream::Error ostr;
          ostr << FUN << ": Caught CampaignServer::NotReady";
          logger_->log(ostr.str(),
            Logging::Logger::NOTICE,
            Aspect::BILL_STAT_SERVER_SOURCE,
            "ADS-IMPL-147");

          campaign_server.release_bad(ostr.str());
        }
        catch(const AdServer::CampaignSvcs::CampaignServer::ImplementationException&)
        {
          Stream::Error ostr;
          ostr << FUN << ": Proxy CampaignServer::ImplementationException";

          campaign_server.release_bad(ostr.str());

          logger_->log(ostr.str(),
            Logging::Logger::ERROR,
            Aspect::BILL_STAT_SERVER_SOURCE,
            "ADS-IMPL-147");
        }
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }

    return nullptr;
  }

  BillStatSource::Stat_var
  BillStatServerSource::convert_update_(
    const AdServer::CampaignSvcs::BillStatInfo& update,
    const Generics::Time& /*now*/)
    /*throw(Exception)*/
  {
    BillStatSource::Stat_var stat(new BillStatSource::Stat());

    for(CORBA::ULong i = 0; i < update.accounts.length(); ++i)
    {
      auto& account_info = update.accounts[i];
      BillStatSource::Stat::Account new_account;
      convert_amount_distribution_(
        new_account, account_info.amount_distribution);
      stat->accounts.insert(std::make_pair(
        account_info.account_id, new_account));
    }

    for(CORBA::ULong i = 0; i < update.campaigns.length(); ++i)
    {
      auto& campaign_info = update.campaigns[i];
      BillStatSource::Stat::Campaign new_campaign;
      convert_amount_count_distribution_(
        static_cast<BillStatSource::Stat::AmountCountDistribution&>(new_campaign), campaign_info.amount_count_distribution);
      stat->campaigns.insert(std::make_pair(
        campaign_info.campaign_id, new_campaign));
    }

    for(CORBA::ULong i = 0; i < update.ccgs.length(); ++i)
    {
      auto& ccg_info = update.ccgs[i];
      BillStatSource::Stat::CCG new_ccg;
      convert_amount_count_distribution_(
        new_ccg, ccg_info.amount_count_distribution);
      stat->ccgs.insert(std::make_pair(
        ccg_info.ccg_id, new_ccg));
    }

    return stat;
  }

  void
  BillStatServerSource::convert_amount_distribution_(
    Stat::AmountDistribution& amount_distribution,
    const AmountDistributionInfo& amount_distribution_info)
    noexcept
  {
    amount_distribution.prev_days_amount =
      CorbaAlgs::unpack_decimal<RevenueDecimal>(
        amount_distribution_info.prev_days_amount.amount);
    amount_distribution.prev_day = CorbaAlgs::unpack_time(
      amount_distribution_info.prev_days_amount.day);

    for(CORBA::ULong i = 0;
      i < amount_distribution_info.day_amounts.length(); ++i)
    {
      auto& day_info = amount_distribution_info.day_amounts[i];
      amount_distribution.day_amounts.insert(std::make_pair(
        CorbaAlgs::unpack_time(day_info.day),
        CorbaAlgs::unpack_decimal<RevenueDecimal>(day_info.amount)));
    }

    auto& day_hourly_amounts = amount_distribution.day_hourly_amounts;
    const auto& day_hourly_amounts_info = amount_distribution_info.day_hourly_amounts;
    for(CORBA::ULong i = 0; i < day_hourly_amounts_info.length(); ++i)
    {
      auto& day_info = day_hourly_amounts_info[i];
      if (day_info.hourly_amounts.length() != 24)
      {
        std::ostringstream stream;
        stream << FNS
               << "hourly_amounts size must be equal 24";
        logger_->log(
          stream.str(),
          Logging::Logger::ERROR,
          Aspect::BILL_STAT_SERVER_SOURCE,
          "ADS-IMPL-147");
        continue;
      }

      auto& hourly_amounts = day_hourly_amounts[CorbaAlgs::unpack_time(day_info.day)];
      const auto& hourly_amounts_info = day_info.hourly_amounts;
      hourly_amounts.reserve(hourly_amounts_info.length());
      for (CORBA::ULong j = 0; j < hourly_amounts_info.length(); ++j)
      {
        const auto& amount_info = hourly_amounts_info[j];
        hourly_amounts.emplace_back(
          CorbaAlgs::unpack_decimal<RevenueDecimal>(amount_info));
      }
    }
  }

  void
  BillStatServerSource::convert_amount_count_distribution_(
    Stat::AmountCountDistribution& amount_count_distribution,
    const AmountCountDistributionInfo& amount_count_distribution_info)
    noexcept
  {
    amount_count_distribution.prev_days_amount =
      CorbaAlgs::unpack_decimal<RevenueDecimal>(
        amount_count_distribution_info.prev_days_amount_count.amount);
    amount_count_distribution.prev_days_imps =
      CorbaAlgs::unpack_decimal<ImpRevenueDecimal>(
        amount_count_distribution_info.prev_days_amount_count.imps);
    amount_count_distribution.prev_days_clicks =
      CorbaAlgs::unpack_decimal<ImpRevenueDecimal>(
        amount_count_distribution_info.prev_days_amount_count.clicks);
    amount_count_distribution.prev_day = CorbaAlgs::unpack_time(
      amount_count_distribution_info.prev_days_amount_count.day);

    for(CORBA::ULong i = 0;
      i < amount_count_distribution_info.day_amount_counts.length(); ++i)
    {
      const auto& day_info = amount_count_distribution_info.day_amount_counts[i];
      amount_count_distribution.day_amount_counts.insert(std::make_pair(
        CorbaAlgs::unpack_time(day_info.day),
        Stat::AmountCount(
          CorbaAlgs::unpack_decimal<RevenueDecimal>(day_info.amount),
          CorbaAlgs::unpack_decimal<ImpRevenueDecimal>(day_info.imps),
          CorbaAlgs::unpack_decimal<ImpRevenueDecimal>(day_info.clicks)
          ))
        );
    }

    auto& day_hourly_amount_counts = amount_count_distribution.day_hourly_amount_counts;
    const auto& day_hourly_amount_counts_info =
      amount_count_distribution_info.day_hourly_amount_counts;
    for(CORBA::ULong i = 0; i < day_hourly_amount_counts_info.length(); ++i)
    {
      const auto& day_info = day_hourly_amount_counts_info[i];
      if (day_info.hourly_amount_counts.length() != 24)
      {
        std::ostringstream stream;
        stream << FNS
               << "hourly_amount_counts size must be equal 24";
        logger_->log(
          stream.str(),
          Logging::Logger::ERROR,
          Aspect::BILL_STAT_SERVER_SOURCE,
          "ADS-IMPL-147");
        continue;
      }

      auto& hourly_amount_counts = day_hourly_amount_counts[
        CorbaAlgs::unpack_time(day_info.day)];
      const auto& hourly_amount_counts_info = day_info.hourly_amount_counts;
      hourly_amount_counts.reserve(hourly_amount_counts_info.length());
      for (CORBA::ULong j = 0; j < hourly_amount_counts_info.length(); ++j)
      {
        const auto& amount_count_info = hourly_amount_counts_info[j];
        hourly_amount_counts.emplace_back(
          Stat::AmountCount{
            CorbaAlgs::unpack_decimal<RevenueDecimal>(amount_count_info.amount),
            CorbaAlgs::unpack_decimal<ImpRevenueDecimal>(amount_count_info.imps),
            CorbaAlgs::unpack_decimal<ImpRevenueDecimal>(amount_count_info.clicks)});
      }
    }
  }
}
}
