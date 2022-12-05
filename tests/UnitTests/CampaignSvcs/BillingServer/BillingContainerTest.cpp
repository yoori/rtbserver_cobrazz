#include <fstream>
#include <Logger/StreamLogger.hpp>
#include <Generics/AppUtils.hpp>

#include <CampaignSvcs/BillingServer/BillingContainer.hpp>

using namespace AdServer::CampaignSvcs;

std::ostream&
print_bid_result(std::ostream& ostr, const BillingContainer::BidResult& bid_result)
{
  ostr << "available = " << bid_result.available <<
    ", goal_ctr = " << bid_result.goal_ctr;
  return ostr;
}

bool
imp_limits_test()
{
  Logging::Logger_var logger(
    new Logging::OStream::Logger(Logging::OStream::Config(std::cout)));

  std::string storage_root = "./tmp/BillingContainerTest";

  ::system((std::string("rm -rf ") + storage_root + " ; mkdir -p " + storage_root + "/").c_str());
  ::system((std::string("touch ") + storage_root + "/Accounts").c_str());
  ::system((std::string("touch ") + storage_root + "/Campaigns").c_str());
  ::system((std::string("touch ") + storage_root + "/Ccgs").c_str());

  BillingContainer::Config_var config = new BillingContainer::Config;

  {
    BillingContainer::Config::Account acc;
    acc.active = true;
    acc.time_offset = Generics::Time::ZERO;
    acc.budget = RevenueDecimal(false, 10000000, 0);
    config->accounts.emplace(1, acc);
  }

  {
    BillingContainer::Config::Campaign campaign;
    campaign.active = true;
    campaign.time_offset = Generics::Time::ZERO;
    campaign.imps_dec = AdServer::CampaignSvcs::ImpRevenueDecimal(false, 1800, 0);
    campaign.daily_imps_dec = AdServer::CampaignSvcs::ImpRevenueDecimal(false, 1000, 0);
    config->campaigns.emplace(1, campaign);
  }

  {
    BillingContainer::Config::CCG ccg;
    ccg.campaign_id = 1;
    ccg.active = true;
    ccg.time_offset = Generics::Time::ZERO;
    //ccg.daily_imps_dec = AdServer::CampaignSvcs::ImpRevenueDecimal(false, 1000, 0);
    config->ccgs.emplace(1, ccg);
  }

  BillingContainer_var billing_container = new BillingContainer(
    logger,
    storage_root,
    Generics::Time::ONE_DAY * 3,
    10);

  billing_container->config(config);

  BillingContainer::Bid bid;
  bid.time = Generics::Time::get_time_of_day();
  bid.account_id = 1;
  bid.advertiser_id = 1;
  bid.campaign_id = 1;
  bid.ccg_id = 1;
  bid.ctr = RevenueDecimal::ZERO;
  bid.optimize_campaign_ctr = false;

  {
    BillingContainer::BidResult check_available_bid_res = billing_container->check_available_bid(bid);
    std::cout << "check #1: ";
    print_bid_result(std::cout, check_available_bid_res) << std::endl;
  }

  for(int i = 0; i < 1001; ++i)
  {
    AdServer::CampaignSvcs::RevenueDecimal acc_amount(false, 10, 0);
    AdServer::CampaignSvcs::RevenueDecimal cmp_amount(false, 10, 0);
    AdServer::CampaignSvcs::ImpRevenueDecimal imp(false, 1, 0);
    AdServer::CampaignSvcs::ImpRevenueDecimal click(false, i % 2, 0);

    BillingContainer::BidResult confirm_bid_res = billing_container->confirm_bid(
      acc_amount,
      cmp_amount,
      imp,
      click,
      bid,
      false // forced
      );

    if(i == 0 || i >= 999)
    {
      std::cout << "confirm #" << i << ": ";
      print_bid_result(std::cout, confirm_bid_res) <<
        ", acc_amount(reminder) = " << acc_amount <<
        ", cmp_amount(reminder) = " << cmp_amount <<
        ", imp(reminder) = " << imp.str() <<
        ", click(reminder) = " << click.str() << std::endl;
    }
  }

  bid.time = bid.time + Generics::Time::ONE_DAY;

  for(int i = 1001; i < 1802; ++i)
  {
    AdServer::CampaignSvcs::RevenueDecimal acc_amount(false, 10, 0);
    AdServer::CampaignSvcs::RevenueDecimal cmp_amount(false, 10, 0);
    AdServer::CampaignSvcs::ImpRevenueDecimal imp(false, 1, 0);
    AdServer::CampaignSvcs::ImpRevenueDecimal click(false, i % 2, 0);

    BillingContainer::BidResult confirm_bid_res = billing_container->confirm_bid(
      acc_amount,
      cmp_amount,
      imp,
      click,
      bid,
      false // forced
      );

    if(i == 1001 || i >= 1800 - 1)
    {
      std::cout << "confirm #" << i << ": ";
      print_bid_result(std::cout, confirm_bid_res) <<
        ", acc_amount(reminder) = " << acc_amount <<
        ", cmp_amount(reminder) = " << cmp_amount <<
        ", imp(reminder) = " << imp.str() <<
        ", click(reminder) = " << click.str() << std::endl;
    }
  }

  //billing_container->dump();
  //return true;

  {
    AdServer::CampaignSvcs::RevenueDecimal acc_amount(true, 10, 0);
    AdServer::CampaignSvcs::RevenueDecimal cmp_amount(true, 10, 0);
    AdServer::CampaignSvcs::ImpRevenueDecimal imp(true, 1, 0);
    AdServer::CampaignSvcs::ImpRevenueDecimal click(true, 1, 0);

    // negative case
    BillingContainer::BidResult confirm_bid_res = billing_container->confirm_bid(
      acc_amount,
      cmp_amount,
      imp,
      click,
      bid,
      false // forced
      );
  }

  billing_container->dump();
}

int main(int argc, char** argv) noexcept
{
  int ret = true;
  ret &= imp_limits_test();

  return 0;
}
