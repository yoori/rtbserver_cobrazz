#include <Commons/FileManip.hpp>
#include <LogCommons/LogCommons.hpp>
#include <LogCommons/LogCommons.ipp>

#include "BillingContainer.hpp"
#include "BillingContainerState.hpp"

//#define DEBUG_OUTPUT

namespace AdServer
{
namespace CampaignSvcs
{
  namespace
  {
    const char ACCOUNTS_FILE[] = "Accounts";
    const char CAMPAIGNS_FILE[] = "Campaigns";
    const char CCGS_FILE[] = "Ccgs";
    const char CCG_RATE_AMOUNTS_FILE[] = "CcgRateAmounts";
    const char CAMPAIGN_RATE_OPTS_FILE[] = "CampaignRateOpts";

    const ImpRevenueDecimal UNLIMITED_IMPS(false, 1000000, 0);
  };

  // BillingContainer::InternalConfig
  struct BillingContainer::InternalConfig: public ReferenceCounting::AtomicImpl
  {
  public:
    typedef Config::Account Account;
    typedef Config::AccountMap AccountMap;

    struct DeliveryLimitsCalcHelper
    {
      Generics::Time adv_tz_date_end;
    };

    typedef std::vector<unsigned long> CCGIdArray;

    struct Campaign: public Config::Campaign,
      public DeliveryLimitsCalcHelper
    {
      CCGIdArray ccg_ids;
    };

    typedef std::unordered_map<unsigned long, Campaign>
      CampaignMap;

    struct CCG: public Config::CCG,
      public DeliveryLimitsCalcHelper
    {};

    typedef std::unordered_map<unsigned long, CCG>
      CCGMap;

    struct ResolveResult
    {
      ResolveResult()
        : account(nullptr),
          advertiser(nullptr),
          campaign(nullptr),
          ccg(nullptr)
      {}

      const Account* account;
      const Account* advertiser;
      const Campaign* campaign;
      const CCG* ccg;
    };

    ResolveResult
    resolve_bid(const Bid& bid)
      const noexcept;

    const BillingContainer::InternalConfig::Account*
    resolve_account(unsigned long account_id)
      const noexcept;

    static Account
    init_forced_account_config()
    {
      Account res;
      res.active = false;
      // use zero offset if need confirm bid and entity isn't loaded
      res.time_offset = Generics::Time::ZERO;
      return res;
    }

    template<typename EntityType>
    static EntityType
    init_forced_entity_config()
    {
      EntityType res;
      res.delivery_pacing = 'F';
      res.active = false;
      // use zero offset if need confirm bid and entity isn't loaded
      res.time_offset = Generics::Time::ZERO;
      return res;
    }

  public:
    AccountMap accounts;
    CampaignMap campaigns;
    CCGMap ccgs;

    static const Account DEFAULT_FORCED_ACCOUNT_TRAITS;
    static const Campaign DEFAULT_FORCED_CAMPAIGN_TRAITS;
    static const CCG DEFAULT_FORCED_CCG_TRAITS;

  protected:
    virtual
    ~InternalConfig() noexcept = default;
  };

  // unsafe class, because it keep ref to parameter
  // DeliveryLimitsType : CommonDeliveryLimits + DeliveryLimitsCalcHelper
  template<typename DeliveryLimitsType>
  class DeliveryLimitsChecker
  {
  public:
    DeliveryLimitsChecker(
      const DeliveryLimitsType& delivery_limits,
      const Generics::Time& now)
      : delivery_limits_(delivery_limits)
    {
      adv_tz_now_date_ = get_date_in_adv_tz(delivery_limits_, now);

#     ifdef DEBUG_OUTPUT
      std::cerr << "  DeliveryLimitsChecker::DeliveryLimitsChecker(): "
        "{now = " << now.gm_ft() << "} => {adv_tz_now_date_ = " << adv_tz_now_date_.gm_ft() << "}" <<
        ", delivery_limits_.time_offset = " << delivery_limits_.time_offset.gm_ft() <<
        std::endl;
#     endif

      if(delivery_limits_.budget ||
        delivery_limits_.daily_budget)
      {
        if(delivery_limits_.delivery_pacing == 'D' &&
          delivery_limits_.date_end != Generics::Time::ZERO)
        {
          remain_days_ = RevenueDecimal(
            (delivery_limits_.adv_tz_date_end - adv_tz_now_date_).tv_sec /
            Generics::Time::ONE_DAY.tv_sec + 1);
        }
      }
    }

    bool
    check_required() const
    {
      return delivery_limits_.budget ||
        delivery_limits_.daily_budget ||
        delivery_limits_.imps_dec ||
        delivery_limits_.daily_imps_dec ||
        delivery_limits_.clicks_dec ||
        delivery_limits_.daily_clicks_dec;
    };

    bool
    check(const BillingContainerState::AmountHolder* amount_holder)
    {
      return check_(
        amount_holder,
        nullptr, // allowed add amount
        nullptr,
        nullptr,
        true // positive
        );
    }

    bool
    confirm_amount(
      BillingContainerState::AmountHolder& amount_holder,
      RevenueDecimal& add_amount,
      ImpRevenueDecimal& add_imps,
      ImpRevenueDecimal& add_clicks,
      bool forced)
      noexcept
    {
      // check without additional amount
      // additional amount can generate much forced loops at last amount
      bool check_result;
      std::optional<RevenueDecimal> allowed_add_amount;
      std::optional<ImpRevenueDecimal> allowed_add_imps;
      std::optional<ImpRevenueDecimal> allowed_add_clicks;

      if(!check_required())
      {
        check_result = true;
      }
      else
      {
        *allowed_add_amount = add_amount;
        *allowed_add_imps = add_imps;
        *allowed_add_clicks = add_clicks;

        check_result = check_(
          &amount_holder,
          &*allowed_add_amount,
          &*allowed_add_imps,
          &*allowed_add_clicks,
          add_amount > RevenueDecimal::ZERO);
      }

#     ifdef DEBUG_OUTPUT
      std::cerr << "  confirm_amount(P1): check_result = " <<
        check_result << std::endl;
#     endif

      if(check_result || forced)
      {
        const RevenueDecimal& res_add_amount =
          allowed_add_amount ? *allowed_add_amount : add_amount;
        const ImpRevenueDecimal res_add_imps =
          allowed_add_imps.has_value() ? *allowed_add_imps : add_imps;
        const ImpRevenueDecimal res_add_clicks =
          allowed_add_clicks.has_value() ? *allowed_add_clicks : add_clicks;

        if(!amount_holder.add_amount(adv_tz_now_date_, res_add_amount, res_add_imps, res_add_clicks))
        {
          check_result = false;
        }

        add_amount -= res_add_amount;
        add_imps -= res_add_imps;
        add_clicks -= res_add_clicks;
      }

      return check_result;
    }

    static Generics::Time
    get_date_in_adv_tz(
      const DeliveryLimitsType& delivery_limits,
      const Generics::Time& now)
      noexcept
    {
      return (now + delivery_limits.time_offset).get_gm_time().get_date();
    }

    bool
    daily_budget_defined() const noexcept
    {
      return (delivery_limits_.delivery_pacing == 'F' &&
          delivery_limits_.daily_budget) ||
        (delivery_limits_.delivery_pacing == 'D' &&
          delivery_limits_.date_end != Generics::Time::ZERO);
    }

    bool
    today_budget(
      RevenueDecimal& res,
      const BillingContainerState::AmountHolder* amount_holder) const
      noexcept
    {
      try
      {
        // check today amount
        if(delivery_limits_.delivery_pacing == 'F')
        {
          if(delivery_limits_.daily_budget)
          {
            res = *delivery_limits_.daily_budget;
            return true;
          }
        }
        else if(delivery_limits_.delivery_pacing == 'D' &&
          delivery_limits_.date_end != Generics::Time::ZERO)
        {
          const RevenueDecimal total_amount = (amount_holder ?
            amount_holder->get_total_amount() :
            RevenueDecimal::ZERO);

          const RevenueDecimal today_amount = (amount_holder ?
            amount_holder->get_day_amount(adv_tz_now_date_) :
            RevenueDecimal::ZERO);

          const RevenueDecimal dynamic_daily_budget =
            remain_days_ > RevenueDecimal::ZERO ?
            RevenueDecimal::div(
              *delivery_limits_.budget - total_amount + today_amount,
              remain_days_) :
            RevenueDecimal::ZERO;

          res = dynamic_daily_budget;

          return true;
        }
      }
      catch(const RevenueDecimal::Overflow&)
      {
        return true;
      }

      return false;
    }

  protected:
    bool
    check_(
      const BillingContainerState::AmountHolder* amount_holder,
      RevenueDecimal* allowed_amount,
      ImpRevenueDecimal* allowed_imps,
      ImpRevenueDecimal* allowed_clicks,
      bool positive) const
    {
      try
      {
        // check today amount
        const RevenueDecimal today_amount = (amount_holder ?
          amount_holder->get_day_amount(adv_tz_now_date_) :
          RevenueDecimal::ZERO);

        const ImpRevenueDecimal today_imps = (amount_holder ?
          amount_holder->get_day_imps(adv_tz_now_date_) :
          ImpRevenueDecimal::ZERO);

        const ImpRevenueDecimal today_clicks = (amount_holder ?
          amount_holder->get_day_clicks(adv_tz_now_date_) :
          ImpRevenueDecimal::ZERO);

        if(allowed_imps)
        {
          *allowed_imps = UNLIMITED_IMPS;
        }

        if(allowed_clicks)
        {
          *allowed_clicks = UNLIMITED_IMPS;
        }

#       ifdef DEBUG_OUTPUT
        std::cerr << "DeliveryLimitsChecker::check_(): input, today_amount = " << today_amount <<
          ", adv_tz_now_date_ = " << adv_tz_now_date_.gm_ft() <<
          ", positive = " << positive <<
          std::endl;
#       endif

        if(positive)
        {
          // fixed daily budget check

          if(delivery_limits_.delivery_pacing == 'F')
          {
            // check fixed daily budget
            if(delivery_limits_.daily_budget)
            {
              /*
#             ifdef DEBUG_OUTPUT
              std::cerr << "DeliveryLimitsChecker::check_(): today_amount = " << today_amount <<
                ", daily_budget = " << *delivery_limits_.daily_budget << std::endl;
#             endif
              */

              if(today_amount >= *delivery_limits_.daily_budget)
              {
                return false;
              }

              if(allowed_amount)
              {
                *allowed_amount = std::min(
                  *delivery_limits_.daily_budget - today_amount,
                  *allowed_amount);
              }
            }
          }

          // fixed daily imps check
          if(delivery_limits_.daily_imps_dec.has_value())
          {
            if(today_imps >= *delivery_limits_.daily_imps_dec)
            {
              return false;
            }

            if(allowed_imps)
            {
              *allowed_imps = std::min(
                *delivery_limits_.daily_imps_dec - today_imps,
                *allowed_imps);
            }
          }

          // daily clicks check
          if(delivery_limits_.daily_clicks_dec.has_value())
          {
            if(today_clicks >= *delivery_limits_.daily_clicks_dec)
            {
              return false;
            }

            if(allowed_clicks)
            {
              *allowed_clicks = std::min(
                *delivery_limits_.daily_clicks_dec - today_clicks,
                *allowed_clicks);
            }
          }
        }
        else // negative
        {
          if(today_amount <= RevenueDecimal::ZERO)
          {
            return false;
          }

          if(today_imps <= ImpRevenueDecimal::ZERO)
          {
            return false;
          }

          if(today_clicks <= ImpRevenueDecimal::ZERO)
          {
            return false;
          }

          // fill allowed values
          if(allowed_amount)
          {
            *allowed_amount = std::max(
              RevenueDecimal(today_amount).negate(),
              *allowed_amount);
          }

          if(allowed_imps)
          {
            *allowed_imps = std::max(
              ImpRevenueDecimal(today_imps).negate(),
              *allowed_imps);
          }

          if(allowed_clicks)
          {
            *allowed_clicks = std::max(
              ImpRevenueDecimal(today_clicks).negate(),
              *allowed_clicks);
          }
        }

        if(!positive || (
             // some total limit defined
             delivery_limits_.budget ||
             delivery_limits_.imps_dec ||
             delivery_limits_.clicks_dec))
        {
          const RevenueDecimal total_amount = (amount_holder ?
            amount_holder->get_total_amount() :
            RevenueDecimal::ZERO);

          const ImpRevenueDecimal total_imps = (amount_holder ?
            amount_holder->get_total_imps() :
            ImpRevenueDecimal::ZERO);

          const ImpRevenueDecimal total_clicks = (amount_holder ?
            amount_holder->get_total_clicks() :
            ImpRevenueDecimal::ZERO);

#         ifdef DEBUG_OUTPUT
          std::cerr << "DeliveryLimitsChecker::check_(): total_amount = " << total_amount <<
            ", total_budget = " << (
              delivery_limits_.budget ?
              *delivery_limits_.budget : RevenueDecimal::ZERO) << std::endl;
#         endif

          if(positive)
          {
            // check total budget
            if(total_amount >= *delivery_limits_.budget)
            {
              return false;
            }

            if(delivery_limits_.imps_dec && total_imps >= *delivery_limits_.imps_dec)
            {
              return false;
            }

            if(delivery_limits_.clicks_dec && total_clicks >= *delivery_limits_.clicks_dec)
            {
              return false;
            }

            if(allowed_amount)
            {
              *allowed_amount = std::min(
                *delivery_limits_.budget - total_amount,
                *allowed_amount);
            }

            if(allowed_imps)
            {
              *allowed_imps = delivery_limits_.imps_dec ?
                std::min(
                  *delivery_limits_.imps_dec - total_imps,
                  *allowed_imps) :
                UNLIMITED_IMPS;
            }

            if(allowed_clicks)
            {
              *allowed_clicks = delivery_limits_.clicks_dec.has_value() ?
                std::min(
                  *delivery_limits_.clicks_dec - total_clicks,
                  *allowed_clicks) :
                UNLIMITED_IMPS;
            }

            // check dynamic daily budget
            if(delivery_limits_.delivery_pacing == 'D' &&
              delivery_limits_.date_end != Generics::Time::ZERO)
            {
              const RevenueDecimal dynamic_daily_budget =
                remain_days_ > RevenueDecimal::ZERO ?
                RevenueDecimal::div(
                  *delivery_limits_.budget - total_amount + today_amount,
                  remain_days_) :
                RevenueDecimal::ZERO;

#             ifdef DEBUG_OUTPUT
              std::cerr << "DeliveryLimitsChecker::check_(): today_amount = " << today_amount <<
                ", dynamic_daily_budget = " << dynamic_daily_budget << std::endl;
#             endif

              if(today_amount >= dynamic_daily_budget)
              {
                return false;
              }

              if(allowed_amount)
              {
                *allowed_amount = std::min(
                  dynamic_daily_budget - today_amount,
                  *allowed_amount);
              }
            }
          }
          else // negative
          {
#           ifdef DEBUG_OUTPUT
            std::cerr << "DeliveryLimitsChecker::check_(): negative case, total_amount = " << total_amount <<
              ", allowed_amount = " << (
                allowed_amount ? allowed_amount->str() : "null") << std::endl;
#           endif

            if(total_amount <= RevenueDecimal::ZERO)
            {
              return false;
            }

            if(total_imps <= ImpRevenueDecimal::ZERO)
            {
              return false;
            }

            if(total_clicks <= ImpRevenueDecimal::ZERO)
            {
              return false;
            }

            if(allowed_amount)
            {
              *allowed_amount = std::max(
                RevenueDecimal(total_amount).negate(),
                *allowed_amount);
            }

            if(allowed_imps)
            {
              *allowed_imps = std::max(
                ImpRevenueDecimal(total_imps).negate(),
                *allowed_imps);
            }

            if(allowed_clicks)
            {
              *allowed_clicks = std::max(
                ImpRevenueDecimal(total_clicks).negate(),
                *allowed_clicks);
            }
          }
        } // !positive || delivery_limits_.budget

        // TODO: check imps/daily_imp/clicks/daily_clicks limits
      }
      catch(const RevenueDecimal::Overflow&)
      {
        return false;
      }

      return true;
    }

  protected:
    const DeliveryLimitsType& delivery_limits_;
    Generics::Time adv_tz_now_date_;
    RevenueDecimal remain_days_;
  };

  namespace
  {
    struct DeliveryLimitsCheckHelper
    {
      Generics::Time adv_tz_now_date;
      RevenueDecimal remain_days;
    };
  }

  // BillingContainer::InternalConfig impl
  const BillingContainer::InternalConfig::Account
  BillingContainer::InternalConfig::DEFAULT_FORCED_ACCOUNT_TRAITS =
    BillingContainer::InternalConfig::init_forced_account_config();

  const BillingContainer::InternalConfig::Campaign
  BillingContainer::InternalConfig::DEFAULT_FORCED_CAMPAIGN_TRAITS =
    BillingContainer::InternalConfig::init_forced_entity_config<
      BillingContainer::InternalConfig::Campaign>();

  const BillingContainer::InternalConfig::CCG
  BillingContainer::InternalConfig::DEFAULT_FORCED_CCG_TRAITS =
    BillingContainer::InternalConfig::init_forced_entity_config<
      BillingContainer::InternalConfig::CCG>();

  BillingContainer::InternalConfig::ResolveResult
  BillingContainer::InternalConfig::resolve_bid(const Bid& bid)
    const noexcept
  {
    BillingContainer::InternalConfig::ResolveResult resolve_result;
    resolve_result.account = resolve_account(bid.account_id);
    resolve_result.advertiser = bid.advertiser_id != 0 ?
      resolve_account(bid.advertiser_id) :
      nullptr;

    {
      auto campaign_it = campaigns.find(bid.campaign_id);
      resolve_result.campaign = campaign_it != campaigns.end() ?
        &campaign_it->second : nullptr;
    }

    {
      auto ccg_it = ccgs.find(bid.ccg_id);
      resolve_result.ccg = ccg_it != ccgs.end() ?
        &ccg_it->second : nullptr;
    }

    return resolve_result;
  }

  const BillingContainer::InternalConfig::Account*
  BillingContainer::InternalConfig::resolve_account(
    unsigned long account_id)
    const noexcept
  {
    auto acc_it = accounts.find(account_id);
    return acc_it != accounts.end() ? &acc_it->second : nullptr;
  }

  // BillingContainer impl
  BillingContainer::BillingContainer(
    Logging::Logger* logger,
    const String::SubString& storage_root,
    const Generics::Time& stat_delay,
    unsigned long limits_divider)
    /*throw(Exception)*/
    : logger_(ReferenceCounting::add_ref(logger)),
      storage_root_(storage_root.str()),
      stat_delay_(stat_delay),
      limit_coef_(RevenueDecimal::div(RevenueDecimal(1), RevenueDecimal(limits_divider))),
      SAVE_PORTION_SIZE_(100),
      REPLACE_PORTION_SIZE_(100),
      SAVE_RATES_PORTION_SIZE_(10),
      SAVE_RATE_OPTS_PORTION_SIZE_(10),
      // optimize coef's
      OPTIMIZE_CTR_FACTUAL_COEF_(0.9),
      OPTIMIZE_CTR_PREDICTED_COEF_(0.1),
      OPTIMIZE_CTR_IGNORE_NOISE_PART_(0.1),
      OPTIMIZE_CTR_RATE_MULTIPLIER_(0.95),
      ctr_optimizer_(
        new CTROptimizer(
          RevenueDecimal(0.1), // max_underdelivery_coef : 10 %
          RevenueDecimal(3), // max goal correct coef
          RevenueDecimal(1.1) // default safe goal daily budget multiplier
          ))
  {
    if(FileManip::dir_exists(storage_root_))
    {
      state_ = load_storage_(storage_root_);
    }
    else
    {
      state_ = new BillingContainerState();
    }
  }

  BillingContainer::BidResult
  BillingContainer::check_available_bid(const Bid& bid)
    /*throw(BillingProcessor::Exception)*/
  {
    const Generics::Time now = bid.time;

    State_var state = state_;
    CInternalConfig_var config = get_config_(true); // only bound

    InternalConfig::ResolveResult resolve_result = config->resolve_bid(bid);

    // for advertiser id we check only that it is active
    if(!resolve_result.account ||
      !resolve_result.account->active ||
      (bid.advertiser_id != 0 &&
       (!resolve_result.advertiser ||
        !resolve_result.advertiser->active)) ||
      !resolve_result.campaign ||
      !resolve_result.campaign->active ||
      !resolve_result.ccg ||
      !resolve_result.ccg->active)
    {
#     ifdef DEBUG_OUTPUT
      std::cerr << "check_available_bid(";
      bid.print(std::cerr);
      std::cerr << "): result = false, non active" << std::endl;
#     endif

      return BidResult(false, RevenueDecimal::ZERO);
    }

    if(!check_available_account_budget_(
         state->accounts,
         state->accounts_lock,
         bid.account_id,
         *resolve_result.account))
    {
#     ifdef DEBUG_OUTPUT
      std::cerr << "check_available_bid(";
      bid.print(std::cerr);
      std::cerr << "): result = false, blocked on account level" << std::endl;
#     endif

      return BidResult(false, RevenueDecimal::ZERO);
    }

    if(!check_available_budget_(
      state->campaigns,
      state->campaigns_lock,
      bid.campaign_id,
      *resolve_result.campaign,
      now))
    {
#     ifdef DEBUG_OUTPUT
      std::cerr << "check_available_bid(";
      bid.print(std::cerr);
      std::cerr << "): result = false, blocked on campaign level" << std::endl;
#     endif

      return BidResult(false, RevenueDecimal::ZERO);
    }

    if(!check_available_budget_(
      state->ccgs,
      state->ccgs_lock,
      bid.ccg_id,
      *resolve_result.ccg,
      now))
    {
#     ifdef DEBUG_OUTPUT
      std::cerr << "check_available_bid(";
      bid.print(std::cerr);
      std::cerr << "): result = false, blocked on ccg level" << std::endl;
#     endif

      return BidResult(false, RevenueDecimal::ZERO);
    }

    RevenueDecimal min_ctr_goal = RevenueDecimal::ZERO;

    if(bid.optimize_campaign_ctr)
    {
      if(!check_min_rate_goal_(
        min_ctr_goal,
        config,
        round_rate_(bid.ctr),
        state->campaign_rate_goals,
        state->campaign_rate_goals_lock,
        state->campaign_rate_opts,
        state->campaign_rate_opts_lock,
        state->ccg_rate_amounts,
        state->ccg_rate_amounts_lock,
        state->campaigns,
        state->campaigns_lock,
        bid.campaign_id,
        *resolve_result.campaign,
        now))
      {
#       ifdef DEBUG_OUTPUT
        std::cerr << "check_available_bid(";
        bid.print(std::cerr);
        std::cerr << "): result = false, blocked by min ctr goal" << std::endl;
#       endif
      }
    }

    return BidResult(true, min_ctr_goal);
  }

  BillingContainer::BidResult
  BillingContainer::confirm_bid(
    RevenueDecimal& account_bid_amount,
    RevenueDecimal& bid_amount,
    ImpRevenueDecimal& imps,
    ImpRevenueDecimal& clicks,
    const Bid& bid,
    bool forced)
    /*throw(BillingProcessor::Exception)*/
  {
#   ifdef DEBUG_OUTPUT
    std::cerr << "confirm_bid(";
    bid.print(std::cerr);
    std::cerr << "): account_amount = " << account_bid_amount <<
      ", amount = " << bid_amount <<
      ", imps = " << imps.str() <<
      ", clicks = " << clicks <<
      ", forced = " << forced <<
      ", time = " << bid.time.gm_ft() << std::endl;
#   endif

    const Generics::Time now = bid.time;

    State_var state = state_;
    CInternalConfig_var config = get_config_(!forced);

    InternalConfig::ResolveResult resolve_result;

    if(config)
    {
      resolve_result = config->resolve_bid(bid);
    }

    if(forced)
    {
      if(!resolve_result.account)
      {
        resolve_result.account = &InternalConfig::DEFAULT_FORCED_ACCOUNT_TRAITS;
      }

      if(!resolve_result.campaign)
      {
        resolve_result.campaign = &InternalConfig::DEFAULT_FORCED_CAMPAIGN_TRAITS;
      }

      if(!resolve_result.ccg)
      {
        resolve_result.ccg = &InternalConfig::DEFAULT_FORCED_CCG_TRAITS;
      }
    }

    if(!resolve_result.account ||
      !resolve_result.campaign ||
      !resolve_result.ccg)
    {
      return BidResult(false, RevenueDecimal::ZERO);
    }

    ConfirmAmountHolder account_confirm_amount_holder;
    account_confirm_amount_holder.available_amount = account_bid_amount;
    account_confirm_amount_holder.confirmed_amount = RevenueDecimal::ZERO;
    account_confirm_amount_holder.revert_amount = RevenueDecimal::ZERO;

    CampaignConfirmAmountHolder campaign_confirm_amount_holder;
    campaign_confirm_amount_holder.available_amount = bid_amount;
    campaign_confirm_amount_holder.confirmed_amount = RevenueDecimal::ZERO;
    campaign_confirm_amount_holder.revert_amount = RevenueDecimal::ZERO;
    campaign_confirm_amount_holder.available_imps = imps;
    campaign_confirm_amount_holder.confirmed_imps = ImpRevenueDecimal::ZERO;
    campaign_confirm_amount_holder.revert_imps = ImpRevenueDecimal::ZERO;
    campaign_confirm_amount_holder.available_clicks = clicks;
    campaign_confirm_amount_holder.confirmed_clicks = ImpRevenueDecimal::ZERO;
    campaign_confirm_amount_holder.revert_clicks = ImpRevenueDecimal::ZERO;

    CampaignConfirmAmountHolder ccg_confirm_amount_holder;
    ccg_confirm_amount_holder.available_amount = bid_amount;
    ccg_confirm_amount_holder.confirmed_amount = RevenueDecimal::ZERO;
    ccg_confirm_amount_holder.revert_amount = RevenueDecimal::ZERO;
    ccg_confirm_amount_holder.available_imps = imps;
    ccg_confirm_amount_holder.confirmed_imps = ImpRevenueDecimal::ZERO;
    ccg_confirm_amount_holder.revert_imps = ImpRevenueDecimal::ZERO;
    ccg_confirm_amount_holder.available_clicks = clicks;
    ccg_confirm_amount_holder.confirmed_clicks = ImpRevenueDecimal::ZERO;
    ccg_confirm_amount_holder.revert_clicks = ImpRevenueDecimal::ZERO;

    bool res = true; // call result (false if some limit reached)
    bool stop_confirm = false;

    // confirm amount for account
    if(!stop_confirm &&
      account_confirm_amount_holder.available_amount != RevenueDecimal::ZERO &&
      !confirm_account_bid_(
        state->accounts,
        state->accounts_lock,
        account_confirm_amount_holder.confirmed_amount,
        account_confirm_amount_holder.available_amount,
        bid.account_id,
        *resolve_result.account,
        now,
        forced))
    {
      if(!forced)
      {
        // eval proportional part of bid_amount
        // only part of campaign amount can be confirmed
        // renew remind_campaign_bid_amount
        recalc_remind_amounts_(
          account_confirm_amount_holder,
          campaign_confirm_amount_holder,
          ccg_confirm_amount_holder,
          true,
          false,
          false,
          account_bid_amount,
          bid_amount);
      }

      res = false;
    }

    // don't check active flags - check only budget limits
#   ifdef DEBUG_OUTPUT
    std::cerr << "  confirm_bid(";
    bid.print(std::cerr);
    std::cerr << "): after account amount confirm"
      ", account_available_amount = " << account_confirm_amount_holder.available_amount <<
      ", account_confirmed_amount = " << account_confirm_amount_holder.confirmed_amount <<
      ", account_revert_amount = " << account_confirm_amount_holder.revert_amount <<
      ", campaign_available_amount = " << campaign_confirm_amount_holder.available_amount <<
      ", campaign_confirmed_amount = " << campaign_confirm_amount_holder.confirmed_amount <<
      ", campaign_revert_amount = " << campaign_confirm_amount_holder.revert_amount <<
      ", ccg_available_amount = " << ccg_confirm_amount_holder.available_amount <<
      ", ccg_confirmed_amount = " << ccg_confirm_amount_holder.confirmed_amount <<
      ", ccg_revert_amount = " << ccg_confirm_amount_holder.revert_amount <<
      ", res = " << res << std::endl;
#   endif

    if(!stop_confirm &&
      campaign_confirm_amount_holder.available_amount != RevenueDecimal::ZERO &&
      !confirm_bid_(
        state->campaigns,
        state->campaigns_lock,
        campaign_confirm_amount_holder.confirmed_amount,
        campaign_confirm_amount_holder.available_amount,
        campaign_confirm_amount_holder.confirmed_imps,
        campaign_confirm_amount_holder.available_imps,
        campaign_confirm_amount_holder.confirmed_clicks,
        campaign_confirm_amount_holder.available_clicks,
        bid.campaign_id,
        *resolve_result.campaign,
        now,
        forced))
    {
      if(!forced)
      {
        // only part of campaign amount confirmed
        // part of account amount should be reverted
        recalc_remind_amounts_(
          account_confirm_amount_holder,
          campaign_confirm_amount_holder,
          ccg_confirm_amount_holder,
          true,
          true,
          false,
          account_bid_amount,
          bid_amount);
      }

      res = false;
    }

#   ifdef DEBUG_OUTPUT
    std::cerr << "  confirm_bid(";
    bid.print(std::cerr);
    std::cerr << "): after campaign amount confirm"
      ", account_available_amount = " << account_confirm_amount_holder.available_amount <<
      ", account_confirmed_amount = " << account_confirm_amount_holder.confirmed_amount <<
      ", account_revert_amount = " << account_confirm_amount_holder.revert_amount <<
      ", campaign_available_amount = " << campaign_confirm_amount_holder.available_amount <<
      ", campaign_confirmed_amount = " << campaign_confirm_amount_holder.confirmed_amount <<
      ", campaign_revert_amount = " << campaign_confirm_amount_holder.revert_amount <<
      ", ccg_available_amount = " << ccg_confirm_amount_holder.available_amount <<
      ", ccg_confirmed_amount = " << ccg_confirm_amount_holder.confirmed_amount <<
      ", ccg_revert_amount = " << ccg_confirm_amount_holder.revert_amount <<
      ", res = " << res << std::endl;
#   endif

    if(!stop_confirm &&
      ccg_confirm_amount_holder.available_amount != RevenueDecimal::ZERO &&
      !confirm_bid_(
        state->ccgs,
        state->ccgs_lock,
        ccg_confirm_amount_holder.confirmed_amount,
        ccg_confirm_amount_holder.available_amount,
        ccg_confirm_amount_holder.confirmed_imps,
        ccg_confirm_amount_holder.available_imps,
        ccg_confirm_amount_holder.confirmed_clicks,
        ccg_confirm_amount_holder.available_clicks,
        bid.ccg_id,
        *resolve_result.ccg,
        now,
        forced))
    {
#     ifdef DEBUG_OUTPUT
      std::cerr << "  confirm_bid_(ccg): " <<
        "ccg_confirm_amount_holder.confirmed_amount = " << ccg_confirm_amount_holder.confirmed_amount <<
        "ccg_confirm_amount_holder.available_amount = " << ccg_confirm_amount_holder.available_amount <<
        "ccg_confirm_amount_holder.confirmed_imps = " << ccg_confirm_amount_holder.confirmed_imps <<
        "ccg_confirm_amount_holder.available_imps = " << ccg_confirm_amount_holder.available_imps <<
        "ccg_confirm_amount_holder.confirmed_clicks = " << ccg_confirm_amount_holder.confirmed_clicks <<
        "ccg_confirm_amount_holder.available_clicks = " << ccg_confirm_amount_holder.available_clicks <<
        std::endl;
#     endif

      if(!forced)
      {
        // only part of ccg amount confirmed
        recalc_remind_amounts_(
          account_confirm_amount_holder,
          campaign_confirm_amount_holder,
          ccg_confirm_amount_holder,
          true,
          true,
          true,
          account_bid_amount,
          bid_amount);
      }

      res = false;
    }

#   ifdef DEBUG_OUTPUT
    std::cerr << "  confirm_bid(";
    bid.print(std::cerr);
    std::cerr << "): after ccg amount confirm"
      ", account_available_amount = " << account_confirm_amount_holder.available_amount <<
      ", account_confirmed_amount = " << account_confirm_amount_holder.confirmed_amount <<
      ", account_revert_amount = " << account_confirm_amount_holder.revert_amount <<
      ", campaign_available_amount = " << campaign_confirm_amount_holder.available_amount <<
      ", campaign_confirmed_amount = " << campaign_confirm_amount_holder.confirmed_amount <<
      ", campaign_revert_amount = " << campaign_confirm_amount_holder.revert_amount <<
      ", ccg_available_amount = " << ccg_confirm_amount_holder.available_amount <<
      ", ccg_confirmed_amount = " << ccg_confirm_amount_holder.confirmed_amount <<
      ", ccg_revert_amount = " << ccg_confirm_amount_holder.revert_amount <<
      ", res = " << res << std::endl;
#   endif

    // REVIEW
    ImpRevenueDecimal confirmed_imps = imps;
    ImpRevenueDecimal confirmed_clicks = clicks;

    if(bid_amount != RevenueDecimal::ZERO)
    {
      const RevenueDecimal confirmed_ccg_share = RevenueDecimal::div(
        ccg_confirm_amount_holder.confirmed_amount,
        bid_amount);

      confirmed_imps = ImpRevenueDecimal::mul(
        ImpRevenueDecimal(confirmed_ccg_share),
        imps,
        Generics::DMR_FLOOR);
      confirmed_clicks = ImpRevenueDecimal::mul(
        ImpRevenueDecimal(confirmed_ccg_share),
        clicks,
        Generics::DMR_FLOOR);
    }

    // TODO: confirm imps,clicks propotional to confirmed_amount
    confirm_bid_rate_(
      state->campaign_rate_goals,
      state->campaign_rate_goals_lock,
      state->ccg_rate_amounts,
      state->ccg_rate_amounts_lock,
      round_rate_(bid.ctr),
      confirmed_imps, // normalize to confirmed amount
      confirmed_clicks,
      ccg_confirm_amount_holder.confirmed_amount,
      bid.campaign_id,
      bid.ccg_id,
      *resolve_result.campaign,
      now);

#   ifdef DEBUG_OUTPUT
    std::cerr << "  confirm_bid(";
    bid.print(std::cerr);
    std::cerr << "): finish"
      ", account_available_amount = " << account_confirm_amount_holder.available_amount <<
      ", account_confirmed_amount = " << account_confirm_amount_holder.confirmed_amount <<
      ", account_revert_amount = " << account_confirm_amount_holder.revert_amount <<
      ", campaign_available_amount = " << campaign_confirm_amount_holder.available_amount <<
      ", campaign_confirmed_amount = " << campaign_confirm_amount_holder.confirmed_amount <<
      ", campaign_revert_amount = " << campaign_confirm_amount_holder.revert_amount <<
      ", ccg_available_amount = " << ccg_confirm_amount_holder.available_amount <<
      ", ccg_confirmed_amount = " << ccg_confirm_amount_holder.confirmed_amount <<
      ", ccg_revert_amount = " << ccg_confirm_amount_holder.revert_amount <<
      ", res = " << res << std::endl;
#   endif

    // do required revert's
    if(ccg_confirm_amount_holder.revert_amount != RevenueDecimal::ZERO)
    {
      // for future, now no reach
      revert_confirmed_bid_(
        state->ccgs,
        state->ccgs_lock,
        bid.ccg_id,
        *resolve_result.ccg,
        now,
        ccg_confirm_amount_holder.revert_amount,
        ccg_confirm_amount_holder.revert_imps,
        ccg_confirm_amount_holder.revert_clicks);
    }

    if(campaign_confirm_amount_holder.revert_amount != RevenueDecimal::ZERO)
    {
      // for future, now no reach
      revert_confirmed_bid_(
        state->campaigns,
        state->campaigns_lock,
        bid.campaign_id,
        *resolve_result.campaign,
        now,
        campaign_confirm_amount_holder.revert_amount,
        campaign_confirm_amount_holder.revert_imps,
        campaign_confirm_amount_holder.revert_clicks);

      campaign_confirm_amount_holder.confirmed_amount -=
        campaign_confirm_amount_holder.revert_amount;
    }

    if(account_confirm_amount_holder.revert_amount != RevenueDecimal::ZERO)
    {
      revert_account_confirmed_bid_(
        state->accounts,
        state->accounts_lock,
        bid.account_id,
        *resolve_result.account,
        now,
        account_confirm_amount_holder.revert_amount);

      account_confirm_amount_holder.confirmed_amount -=
        account_confirm_amount_holder.revert_amount;
    }

    account_bid_amount -= account_confirm_amount_holder.confirmed_amount;
    bid_amount -= campaign_confirm_amount_holder.confirmed_amount;
    imps -= confirmed_imps;
    clicks -= confirmed_clicks;

    RevenueDecimal goal_ctr = RevenueDecimal::ZERO;
    return BidResult(res, goal_ctr); // some amount not confirmed
  }

  // not null remind parameters defined if confirm done
  void
  BillingContainer::recalc_remind_amounts_(
    ConfirmAmountHolder& account_confirm,
    CampaignConfirmAmountHolder& campaign_confirm,
    CampaignConfirmAmountHolder& ccg_confirm,
    bool account_bid_amount_confirmed,
    bool campaign_bid_amount_confirmed,
    bool ccg_bid_amount_confirmed,
    const RevenueDecimal& account_bid_amount,
    const RevenueDecimal& bid_amount)
    noexcept
  {
    // only part of ccg amount confirmed
    // part of account amount and campaign amount should be reverted
    RevenueDecimal min_confirmed_share = REVENUE_ONE;

    if(account_bid_amount_confirmed && account_bid_amount != RevenueDecimal::ZERO)
    {
      const RevenueDecimal confirmed_account_share = RevenueDecimal::div(
        account_confirm.confirmed_amount,
        account_bid_amount);
      min_confirmed_share = std::min(min_confirmed_share, confirmed_account_share);
    }

    if(campaign_bid_amount_confirmed && bid_amount != RevenueDecimal::ZERO)
    {
      const RevenueDecimal confirmed_campaign_share = RevenueDecimal::div(
        campaign_confirm.confirmed_amount,
        bid_amount);
      min_confirmed_share = std::min(min_confirmed_share, confirmed_campaign_share);
    }

    if(ccg_bid_amount_confirmed && bid_amount != RevenueDecimal::ZERO)
    {
      const RevenueDecimal confirmed_ccg_share = RevenueDecimal::div(
        ccg_confirm.confirmed_amount,
        bid_amount);
      min_confirmed_share = std::min(min_confirmed_share, confirmed_ccg_share);
    }

    {
      const RevenueDecimal target_account_bid_amount = RevenueDecimal::mul(
        min_confirmed_share,
        account_bid_amount,
        Generics::DMR_FLOOR);
      if(account_bid_amount_confirmed &&
        target_account_bid_amount != account_confirm.confirmed_amount)
      {
        account_confirm.revert_amount =
          account_confirm.confirmed_amount - target_account_bid_amount;
      }
      account_confirm.available_amount = !account_bid_amount_confirmed ?
        target_account_bid_amount :
        RevenueDecimal::ZERO;
    }

    {
      const RevenueDecimal target_campaign_bid_amount = RevenueDecimal::mul(
        min_confirmed_share,
        bid_amount,
        Generics::DMR_FLOOR);
      if(campaign_bid_amount_confirmed &&
        target_campaign_bid_amount != campaign_confirm.confirmed_amount)
      {
        campaign_confirm.revert_amount =
          campaign_confirm.confirmed_amount - target_campaign_bid_amount;
      }
      campaign_confirm.available_amount = !campaign_bid_amount_confirmed ?
        target_campaign_bid_amount :
        RevenueDecimal::ZERO;
    }

    {
      const RevenueDecimal target_ccg_bid_amount = RevenueDecimal::mul(
        min_confirmed_share,
        bid_amount,
        Generics::DMR_FLOOR);
      if(ccg_bid_amount_confirmed &&
        target_ccg_bid_amount != ccg_confirm.confirmed_amount)
      {
        ccg_confirm.revert_amount =
          ccg_confirm.confirmed_amount - target_ccg_bid_amount;
      }
      ccg_confirm.available_amount = !ccg_bid_amount_confirmed ?
        target_ccg_bid_amount :
        RevenueDecimal::ZERO;
    }

#   ifdef DEBUG_OUTPUT
    std::cerr << "recalc_remind_amounts_(): "
      "min_confirmed_share = " << min_confirmed_share <<
      ", account_available_amount = " << account_confirm.available_amount <<
      ", account_confirmed_amount = " << account_confirm.confirmed_amount <<
      ", account_revert_amount = " << account_confirm.revert_amount <<
      ", campaign_available_amount = " << campaign_confirm.available_amount <<
      ", campaign_confirmed_amount = " << campaign_confirm.confirmed_amount <<
      ", campaign_revert_amount = " << campaign_confirm.revert_amount <<
      ", ccg_available_amount = " << ccg_confirm.available_amount <<
      ", ccg_confirmed_amount = " << ccg_confirm.confirmed_amount <<
      ", ccg_revert_amount = " << ccg_confirm.revert_amount <<
      std::endl;
#   endif
  }

  bool
  BillingContainer::reserve_bid(
    const Bid& /*bid*/,
    const RevenueDecimal& /*bid_amount*/)
    /*throw(BillingProcessor::Exception)*/
  {
    // TODO
    return true;
  }

  void
  BillingContainer::clear_expired_reservation(
    const Generics::Time& /*time*/)
    /*throw(BillingProcessor::Exception)*/
  {
    // TODO
  }

  void
  BillingContainer::config(Config* new_config)
    noexcept
  {
    InternalConfig_var new_internal_config(new InternalConfig());

    new_internal_config->accounts = new_config->accounts;

    for(auto acc_it = new_internal_config->accounts.begin();
        acc_it != new_internal_config->accounts.end(); ++acc_it)
    {
      Config::Account& account = acc_it->second;

      account.budget = RevenueDecimal::mul(
        account.budget,
        limit_coef_,
        Generics::DMR_FLOOR);
    }

    for(Config::CampaignMap::const_iterator cmp_it =
          new_config->campaigns.begin();
        cmp_it != new_config->campaigns.end(); ++cmp_it)
    {
      InternalConfig::Campaign campaign;
      static_cast<Config::Campaign&>(campaign) = cmp_it->second;
      adapt_delivery_limits_(campaign);

      new_internal_config->campaigns.insert(
        std::make_pair(cmp_it->first, campaign));
    }

    for(Config::CCGMap::const_iterator ccg_it =
          new_config->ccgs.begin();
        ccg_it != new_config->ccgs.end(); ++ccg_it)
    {
      InternalConfig::CCG ccg;
      static_cast<Config::CCG&>(ccg) = ccg_it->second;
      adapt_delivery_limits_(ccg);

      new_internal_config->ccgs.insert(
        std::make_pair(ccg_it->first, ccg));
    }

    config_ = new_internal_config;
  }

  void
  BillingContainer::stat(
    BillStatSource::Stat* bill_stat)
    noexcept
  {
    stat_ = ReferenceCounting::add_ref(bill_stat);

    State_var state = state_;
    assert(state.in());

    BillingContainer::CInternalConfig_var config = get_config_(false);

    // replace local collected values by bill stat - only data that older
    // then stat_delay
    replace_account_amounts_(
      state->accounts,
      state->accounts_lock,
      bill_stat->accounts,
      config ? &config->accounts : nullptr);
    //std::cerr << "=============================== accounts replaced" << std::endl;
    replace_amounts_(
      state->campaigns,
      state->campaigns_lock,
      bill_stat->campaigns,
      config ? &config->campaigns : nullptr);
    //std::cerr << "=============================== campaigns replaced" << std::endl;
    replace_amounts_(
      state->ccgs,
      state->ccgs_lock,
      bill_stat->ccgs,
      config ? &config->ccgs : nullptr);
    //std::cerr << "=============================== ccgs replaced" << std::endl;
  }

  template<
    typename AmountMapType,
    typename AccountDeliveryLimitsType>
  bool
  BillingContainer::check_available_account_budget_(
    const AmountMapType& amounts,
    StateSyncPolicy::Mutex& amounts_lock,
    unsigned long account_id,
    const AccountDeliveryLimitsType& delivery_limits)
    noexcept
  {
    // check amounts
    StateSyncPolicy::ReadGuard lock(amounts_lock);
    auto account_it = amounts.find(account_id);

    const RevenueDecimal account_total_amount = (
      account_it != amounts.end() ?
      account_it->second->get_total_amount() :
      RevenueDecimal::ZERO);

#   ifdef DEBUG_OUTPUT
    std::cerr << "check_available_account_budget_(): for account_id = " << account_id <<
      ": account_total_amount = " << account_total_amount <<
      ", budget = " << delivery_limits.budget << std::endl;
#   endif

    return account_total_amount < delivery_limits.budget;
  }

  template<
    typename AmountMapType,
    typename DeliveryLimitsType // CommonDeliveryLimits + DeliveryLimitsCalcHelper
    >
  bool
  BillingContainer::check_available_budget_(
    const AmountMapType& amounts,
    StateSyncPolicy::Mutex& amounts_lock,
    unsigned long object_id,
    const DeliveryLimitsType& delivery_limits,
    const Generics::Time& now)
    noexcept
  {
    DeliveryLimitsChecker<DeliveryLimitsType> delivery_limits_checker(
      delivery_limits,
      now);

    if(!delivery_limits_checker.check_required())
    {
      return true;
    }

    // check amounts
    StateSyncPolicy::ReadGuard lock(amounts_lock);
    auto it = amounts.find(object_id);
    const BillingContainerState::AmountHolder* amount_holder =
      it != amounts.end() ? it->second : nullptr;

    return delivery_limits_checker.check(amount_holder);
  }

  template<
    typename RateOptMapType,
    typename RateAmountMapType,
    typename DailyAmountMapType,
    typename DeliveryLimitsType>
  void
  BillingContainer::make_min_rate_goal_optimization_(
    RevenueDecimal& min_rate_goal,
    CTROptimizer::HourBudgetDistribution& free_budget_distribution,
    const InternalConfig* config,
    RateOptMapType& rate_opts, // opts
    StateSyncPolicy::Mutex& rate_opts_lock,
    const RateAmountMapType& rate_amounts, // amounts by rate
    StateSyncPolicy::Mutex& rate_amounts_lock,
    const DailyAmountMapType& daily_amounts, // daily amounts
    StateSyncPolicy::Mutex& daily_amounts_lock,
    unsigned long object_id,
    const DeliveryLimitsType& delivery_limits,
    const Generics::Time& now)
    noexcept
  {
    RevenueDecimal today_budget = RevenueDecimal::ZERO;

    {
      // eval today budget
      DeliveryLimitsChecker<DeliveryLimitsType> delivery_limits_checker(
        delivery_limits,
        now);

      if(delivery_limits_checker.daily_budget_defined())
      {
        StateSyncPolicy::ReadGuard lock(daily_amounts_lock);
        auto it = daily_amounts.find(object_id);
        const BillingContainerState::AmountHolder* amount_holder =
          it != daily_amounts.end() ? it->second : nullptr;

        delivery_limits_checker.today_budget(today_budget, amount_holder);
      }
    }

    if(today_budget != RevenueDecimal::ZERO)
    {
      Generics::Time adv_tz_now_date = DeliveryLimitsChecker<
        DeliveryLimitsType>::get_date_in_adv_tz(
          delivery_limits,
          now);

      // do optimization
      std::list<std::pair<unsigned long, BillingContainerState::RateDistributionHolder> > past_amount_holders;
      std::list<std::pair<unsigned long, BillingContainerState::RateDistributionHolder> > actual_amount_holders;

      //Generics::Time prev_last_eval_min_rate_goal_time;

      {
        // get rated amounts
        SyncPolicy::ReadGuard lock(rate_amounts_lock);
        //
        auto it = rate_amounts.lower_bound(BillingContainerState::CampaignCCGId(object_id, 0));
        for(; it != rate_amounts.end() && it->first.campaign_id == object_id; ++it)
        {
          auto today_it = it->second.dates.find(adv_tz_now_date);
          if(today_it != it->second.dates.end())
          {
            actual_amount_holders.push_back(std::make_pair(it->first.ccg_id, today_it->second));
          }

          auto past_it = it->second.dates.find(adv_tz_now_date - Generics::Time::ONE_DAY);
          if(past_it != it->second.dates.end())
          {
            past_amount_holders.push_back(std::make_pair(it->first.ccg_id, past_it->second));
          }
        }
      }

      BillingContainerState::RateOptimizationHolder::DateHolder actual_opt_holder;

      {
        // get opt info
        SyncPolicy::ReadGuard lock(rate_opts_lock);
        //
        auto it = rate_opts.find(object_id);
        if(it != rate_opts.end())
        {
          auto today_it = it->second.dates.find(adv_tz_now_date);
          if(today_it != it->second.dates.end())
          {
            actual_opt_holder = today_it->second;
          }
        }
      }

      const unsigned long cur_hour = (
        now + delivery_limits.time_offset).get_gm_time().tm_hour;

      // 
      // convert imps/clicks to amount by actual cost's
      CTROptimizer::RateAmountDistribution past_goaled_amount_distribution;
      CTROptimizer::RateAmountDistribution past_free_amount_distribution;
      CTROptimizer::RateAmountDistribution actual_goaled_amount_distribution;
      CTROptimizer::RateAmountDistribution actual_free_amount_distribution;

      RevenueDecimal ctr_factual_coef;
      RevenueDecimal ctr_predicted_coef;

      if((now + delivery_limits.time_offset).get_gm_time().tm_hour < 20)
      {
        ctr_factual_coef = OPTIMIZE_CTR_FACTUAL_COEF_;
        ctr_predicted_coef = OPTIMIZE_CTR_PREDICTED_COEF_;
      }
      else
      {
        ctr_factual_coef = RevenueDecimal(false, 1, 0);
        ctr_predicted_coef = RevenueDecimal::ZERO;
      }

      for(auto past_amount_holder_it = past_amount_holders.begin();
        past_amount_holder_it != past_amount_holders.end();
        ++past_amount_holder_it)
      {
        auto ccg_it = config->ccgs.find(past_amount_holder_it->first);

        if(ccg_it != config->ccgs.end() && ccg_it->second.active)
        {
          BillingContainerState::convert_rate_amount_distribution(
            past_free_amount_distribution,
            past_goaled_amount_distribution,
            past_amount_holder_it->second,
            ccg_it->second.imp_amount,
            ccg_it->second.click_amount,
            ctr_factual_coef,
            ctr_predicted_coef,
            OPTIMIZE_CTR_IGNORE_NOISE_PART_,
            OPTIMIZE_CTR_RATE_MULTIPLIER_);
        }
      }

      for(auto actual_amount_holder_it = actual_amount_holders.begin();
        actual_amount_holder_it != actual_amount_holders.end();
        ++actual_amount_holder_it)
      {
        auto ccg_it = config->ccgs.find(actual_amount_holder_it->first);

        if(ccg_it != config->ccgs.end() && ccg_it->second.active)
        {
          BillingContainerState::convert_rate_amount_distribution(
            actual_free_amount_distribution,
            actual_goaled_amount_distribution,
            actual_amount_holder_it->second,
            ccg_it->second.imp_amount,
            ccg_it->second.click_amount,
            ctr_factual_coef,
            ctr_predicted_coef,
            OPTIMIZE_CTR_IGNORE_NOISE_PART_,
            OPTIMIZE_CTR_RATE_MULTIPLIER_);
        }
      }

      RevenueDecimal goal_budget;

      ctr_optimizer_->recalculate_rate_goal(
        min_rate_goal,
        goal_budget,
        free_budget_distribution,
        past_goaled_amount_distribution, // past amount distribution
        past_free_amount_distribution,
        actual_goaled_amount_distribution, // actual amount distribution
        actual_free_amount_distribution,
        actual_opt_holder.min_rates,
        today_budget,
        now + delivery_limits.time_offset);

      actual_opt_holder.min_rates[cur_hour] = min_rate_goal;

#     ifdef DEBUG_OUTPUT
      {
        Stream::Error ostr;
        ostr << "BillingContainer::make_min_rate_goal_optimization_(): "
          "campaign_id = " << object_id <<
          ", goal_budget = " << goal_budget <<
          ", today_budget = " << today_budget <<
          ", free_budget_distribution = ";
        free_budget_distribution.print(ostr);
        ostr << std::endl;
        std::cerr << ostr.str();
      }
#     endif

      {
        // commit opt info
        SyncPolicy::WriteGuard lock(rate_opts_lock);
        rate_opts[object_id].dates[adv_tz_now_date] = actual_opt_holder;
      }
    }
    else // today_budget == RevenueDecimal::ZERO
    {
      min_rate_goal = RevenueDecimal::ZERO;
    }
  }

  template<
    typename RateGoalMapType,
    typename RateOptMapType,
    typename RateAmountMapType,
    typename DailyAmountMapType,
    typename DeliveryLimitsType>
  bool
  BillingContainer::check_min_rate_goal_(
    RevenueDecimal& min_rate_goal,
    const InternalConfig* config,
    const RevenueDecimal& rate,
    RateGoalMapType& rate_goals, // goals
    StateSyncPolicy::Mutex& rate_goals_lock,
    RateOptMapType& rate_opts, // opts
    StateSyncPolicy::Mutex& rate_opts_lock,
    RateAmountMapType& rate_amounts, // amounts by rate
    StateSyncPolicy::Mutex& rate_amounts_lock,
    const DailyAmountMapType& daily_amounts, // daily amounts
    StateSyncPolicy::Mutex& daily_amounts_lock,
    unsigned long object_id,
    const DeliveryLimitsType& delivery_limits,
    const Generics::Time& now)
    noexcept
  {
    // check min rate
    const Generics::Time ASYNCH_PERIOD(10);

    const unsigned long hour = (
      now + delivery_limits.time_offset).get_gm_time().tm_hour;
    const Generics::Time round_time = Generics::Time(
      (now - ASYNCH_PERIOD).tv_sec / Generics::Time::ONE_HOUR.tv_sec *
      Generics::Time::ONE_HOUR.tv_sec);
    /*
    const Generics::Time adv_tz_now_date = DeliveryLimitsChecker<
      DeliveryLimitsType>::get_date_in_adv_tz(
        delivery_limits,
        now);
    */

    min_rate_goal = RevenueDecimal::ZERO;

    Generics::Time last_eval_min_rate_goal_time;
    bool found = false;
    bool result = false;

    {
      // check by goal
      SyncPolicy::ReadGuard lock(rate_amounts_lock);
      auto it = rate_goals.find(object_id);

      if(it != rate_goals.end())
      {
        last_eval_min_rate_goal_time = it->second.last_eval_min_rate_goal_time;

        if(it->second.last_eval_min_rate_goal_time == round_time)
        {
          min_rate_goal = it->second.min_rate_goal;
          found = true;
          result = rate >= it->second.min_rate_goal ||
            it->second.free_amount_distribution.amounts[hour].amount <
              it->second.free_budget_distribution[hour];
        }
      }
    }

    if(found)
    {
#     ifdef DEBUG_OUTPUT
      std::cerr << "check_min_rate_goal_(): result = " <<
        (result ? "true" : "false") << ", already optimized"
        ", rate = " << rate <<
        ", min_rate_goal = " << min_rate_goal << std::endl;
#     endif

      return result;
    }

    // reoptimize and check rate and free budget availability
    // only one thread make optimization
    // other return false
    bool make_opt = false;

    {
      RateOptimizationSyncPolicy::WriteGuard lock(campaign_min_ctr_goal_lock_);
      make_opt = campaign_ctr_optimization_ids_.insert(object_id).second;
    }

    if(make_opt)
    {
#     ifdef DEBUG_OUTPUT
      std::cerr << "check_min_rate_goal_(): do optimization"
        ", last_eval_min_rate_goal_time = " << last_eval_min_rate_goal_time.gm_ft() <<
        ", round_time = " << round_time.gm_ft() << std::endl;
#     endif

      CTROptimizer::HourBudgetDistribution free_budget_distribution;
      CTROptimizer::HourRateDistribution min_rates;

      make_min_rate_goal_optimization_(
        min_rate_goal,
        free_budget_distribution,
        config,
        rate_opts, // opts
        rate_opts_lock,
        rate_amounts, // amounts by rate
        rate_amounts_lock,
        daily_amounts, // daily amounts
        daily_amounts_lock,
        object_id,
        delivery_limits,
        now);

      {
        // commit goal
        SyncPolicy::WriteGuard lock(rate_goals_lock);
        auto& min_rate_holder = rate_goals[object_id];
        min_rate_holder.last_eval_min_rate_goal_time = round_time;
        min_rate_holder.min_rate_goal = min_rate_goal;
        min_rate_holder.free_budget_distribution.swap(free_budget_distribution);

        result = rate >= min_rate_holder.min_rate_goal ||
          min_rate_holder.free_amount_distribution.amounts[hour].amount <
            min_rate_holder.free_budget_distribution[hour];
      }

      RateOptimizationSyncPolicy::WriteGuard lock(campaign_min_ctr_goal_lock_);
      campaign_ctr_optimization_ids_.erase(object_id);
    }

#   ifdef DEBUG_OUTPUT
    std::cerr << "check_min_rate_goal_(): result = " <<
      (result ? "true" : "false") << ", after optimization"
      ", rate = " << rate <<
      ", min_rate_goal = " << min_rate_goal << std::endl;
#   endif

    return result;
  }

  template<
    typename RateGoalMapType,
    typename RateAmountMapType,
    typename DeliveryLimitsType>
  void
  BillingContainer::confirm_bid_rate_(
    RateGoalMapType& rate_goals, // goals
    StateSyncPolicy::Mutex& rate_goals_lock,
    RateAmountMapType& rate_amounts,
    StateSyncPolicy::Mutex& rate_amounts_lock,
    const RevenueDecimal& rate,
    const ImpRevenueDecimal& confirm_imps,
    const ImpRevenueDecimal& confirm_clicks,
    const RevenueDecimal& amount,
    unsigned long campaign_id,
    unsigned long ccg_id,
    const DeliveryLimitsType& delivery_limits,
    const Generics::Time& now)
    noexcept
  {
#   ifdef DEBUG_OUTPUT
    std::cerr << "BillingContainer::confirm_bid_rate_(): campaign_id = " << campaign_id <<
      ", ccg_id = " << ccg_id <<
      ", imps = " << confirm_imps.str() <<
      ", clicks = " << confirm_clicks.str() << std::endl;
#   endif

    const Generics::Time ASYNCH_PERIOD = Generics::Time::ZERO;

    const Generics::Time round_time = Generics::Time(
      (now - ASYNCH_PERIOD).tv_sec / Generics::Time::ONE_HOUR.tv_sec *
      Generics::Time::ONE_HOUR.tv_sec);

    const Generics::Time adv_tz_now = now + delivery_limits.time_offset;
    const Generics::Time date = adv_tz_now.get_gm_time().get_date();
    const unsigned long hour = adv_tz_now.get_gm_time().tm_hour;

    const Generics::Time hour_time((
      adv_tz_now - date - Generics::Time::ONE_HOUR * hour +
      Generics::Time::ONE_SECOND).tv_sec);

    const bool positive = confirm_imps > ImpRevenueDecimal::ZERO ||
      confirm_clicks > ImpRevenueDecimal::ZERO;

    bool free_amount = true;

    {
      SyncPolicy::WriteGuard lock(rate_goals_lock);
      auto& rate_goal_holder = rate_goals[campaign_id];

      free_amount =
        rate_goal_holder.free_amount_distribution.amounts[hour].amount <
        rate_goal_holder.free_budget_distribution[hour];

      if(free_amount)
      {
        rate_goal_holder.free_amount_distribution.amounts[hour].amount += amount;
      }
    }

    {
      // push into rated amounts
      // check by goal
      SyncPolicy::WriteGuard lock(rate_amounts_lock);

      auto& rate_amount_holder = rate_amounts[BillingContainerState::CampaignCCGId(campaign_id, ccg_id)];

      rate_amount_holder.dates[date].add(
        hour,
        rate,
        free_amount,
        confirm_imps,
        confirm_clicks,
        free_amount ? hour_time : (
          positive ? Generics::Time::ONE_HOUR : Generics::Time::ZERO));
    }
  }

  template<
    typename AmountMapType,
    typename AccountDeliveryLimitsType
    >
  bool
  BillingContainer::confirm_account_bid_(
    AmountMapType& amounts,
    StateSyncPolicy::Mutex& amounts_lock,
    RevenueDecimal& confirmed_amount,
    const RevenueDecimal& confirm_amount,
    unsigned long account_id,
    const AccountDeliveryLimitsType& account_config,
    const Generics::Time& now,
    bool forced)
    noexcept
  {
    confirmed_amount = RevenueDecimal::ZERO;

    BillingContainerState::AccountAmountHolder_var new_amount_holder =
      new BillingContainerState::AccountAmountHolder();

    const Generics::Time adv_tz_now_date =
      DeliveryLimitsChecker<AccountDeliveryLimitsType>::get_date_in_adv_tz(
        account_config, now);

    StateSyncPolicy::WriteGuard lock(amounts_lock);

    auto account_it = amounts.find(account_id);

    const RevenueDecimal account_total_amount = (
      account_it != amounts.end() ?
      account_it->second->get_total_amount() :
      RevenueDecimal::ZERO);

    bool res = account_total_amount < account_config.budget;

    if(res || forced)
    {
      if(account_it == amounts.end())
      {
        account_it = amounts.insert(std::make_pair(account_id, new_amount_holder)).first;
      }

      if(forced || account_config.budget - account_total_amount > confirm_amount)
      {
        account_it->second->add_amount(adv_tz_now_date, confirm_amount);
        confirmed_amount = confirm_amount;
      }
      else // not full confirm
      {
        RevenueDecimal allowed_add_amount = account_config.budget - account_total_amount;
        account_it->second->add_amount(adv_tz_now_date, allowed_add_amount);
        confirmed_amount = allowed_add_amount;
        res = false;
      }
    }

    return res;
  }

  template<
    typename AmountMapType,
    typename DeliveryLimitsType // CommonDeliveryLimits + DeliveryLimitsCalcHelper
    >
  bool
  BillingContainer::confirm_bid_(
    AmountMapType& amounts,
    StateSyncPolicy::Mutex& amounts_lock,
    RevenueDecimal& confirmed_amount,
    const RevenueDecimal& confirm_amount,
    ImpRevenueDecimal& confirmed_imps,
    const ImpRevenueDecimal& confirm_imps,
    ImpRevenueDecimal& confirmed_clicks,
    const ImpRevenueDecimal& confirm_clicks,
    unsigned long object_id,
    const DeliveryLimitsType& delivery_limits,
    const Generics::Time& now,
    bool forced)
    noexcept
  {
    RevenueDecimal remind_confirm_amount = confirm_amount;
    ImpRevenueDecimal remind_confirm_imps = confirm_imps;
    ImpRevenueDecimal remind_confirm_clicks = confirm_clicks;

    DeliveryLimitsChecker<DeliveryLimitsType> delivery_limits_checker(
      delivery_limits,
      now);

    BillingContainerState::AmountHolder_var new_amount_holder =
      new BillingContainerState::AmountHolder();

    bool res;

    {
      StateSyncPolicy::WriteGuard lock(amounts_lock);
      auto it = amounts.find(object_id);
      BillingContainerState::AmountHolder* res_amount_holder = (
        it != amounts.end() ? it->second : new_amount_holder);

      res = delivery_limits_checker.confirm_amount(
        *res_amount_holder,
        remind_confirm_amount,
        remind_confirm_imps,
        remind_confirm_clicks,
        forced);

#     ifdef DEBUG_OUTPUT
      std::cerr << "  delivery_limits_checker.confirm_amount(): output"
        ", remind_confirm_amount = " << remind_confirm_amount <<
        ", now = " << now.gm_ft() <<
        std::endl;
#     endif

      if(it == amounts.end() && (res || forced))
      {
        amounts.insert(std::make_pair(object_id, new_amount_holder));
      }
    }

    confirmed_amount = confirm_amount - remind_confirm_amount;
    confirmed_imps = confirm_imps - remind_confirm_imps;
    confirmed_clicks = confirm_clicks - remind_confirm_clicks;    

    return res;
  }

  template<
    typename AmountMapType,
    typename DeliveryLimitsType // CommonDeliveryLimits + DeliveryLimitsCalcHelper
    >
  void
  BillingContainer::revert_account_confirmed_bid_(
    const AmountMapType& amounts,
    StateSyncPolicy::Mutex& amounts_lock,
    unsigned long object_id,
    const DeliveryLimitsType& delivery_limits,
    const Generics::Time& now,
    const RevenueDecimal& confirm_amount)
    noexcept
  {
    RevenueDecimal revert_amount = confirm_amount;
    revert_amount.negate();    

    Generics::Time adv_tz_now_date = DeliveryLimitsChecker<
      DeliveryLimitsType>::get_date_in_adv_tz(
        delivery_limits,
        now);

    bool add_amount_result;

    {
      StateSyncPolicy::WriteGuard lock(amounts_lock);
      auto it = amounts.find(object_id);
      assert(it != amounts.end());

      add_amount_result = it->second->add_amount(
        adv_tz_now_date,
        revert_amount);
    }

    assert(add_amount_result); // overflow unexpected
  }

  template<
    typename AmountMapType,
    typename DeliveryLimitsType // CommonDeliveryLimits + DeliveryLimitsCalcHelper
    >
  void
  BillingContainer::revert_confirmed_bid_(
    const AmountMapType& amounts,
    StateSyncPolicy::Mutex& amounts_lock,
    unsigned long object_id,
    const DeliveryLimitsType& delivery_limits,
    const Generics::Time& now,
    const RevenueDecimal& confirm_amount,
    const ImpRevenueDecimal& imps,
    const ImpRevenueDecimal& clicks)
    noexcept
  {
    RevenueDecimal revert_amount = confirm_amount;
    ImpRevenueDecimal revert_imps = imps;
    ImpRevenueDecimal revert_clicks = clicks;
    revert_amount.negate();
    revert_imps.negate();
    revert_clicks.negate();
    

    Generics::Time adv_tz_now_date = DeliveryLimitsChecker<
      DeliveryLimitsType>::get_date_in_adv_tz(
        delivery_limits,
        now);

    bool add_amount_result;

    {
      StateSyncPolicy::WriteGuard lock(amounts_lock);
      auto it = amounts.find(object_id);
      assert(it != amounts.end());

      add_amount_result = it->second->add_amount(
        adv_tz_now_date,
        revert_amount,
        imps,
        clicks);
    }

    assert(add_amount_result); // overflow unexpected
  }

  template<
    typename AmountMapType,
    typename AmountDistributionMapType,
    typename ConfigMapType>
  void
  BillingContainer::replace_account_amounts_(
    AmountMapType& day_amounts,
    StateSyncPolicy::Mutex& amounts_lock,
    const AmountDistributionMapType& source_amounts,
    const ConfigMapType* entities_config)
    noexcept
  {
    //std::cerr << "DEBUG: entities_config = " << entities_config << std::endl;

    const Generics::Time base_use_source_end_time = Generics::Time(
      Generics::Time::get_time_of_day() - stat_delay_);
    const Generics::Time def_use_source_end_date = (
      base_use_source_end_time - Generics::Time::ONE_HOUR * 14
      // use max possible time offset (UTC + 14)
        ).get_gm_time().get_date();

    bool end_reached = false;
    bool last_key_inited = false;
    bool target_end_reached = false;
    typename AmountMapType::key_type last_key =
      typename AmountMapType::key_type();

    auto source_it = source_amounts.begin();

    while(!end_reached)
    {
      StateSyncPolicy::WriteGuard lock(amounts_lock);
      typename AmountMapType::iterator cur_it;
      if(target_end_reached)
      {
        cur_it = day_amounts.end();
      }
      else if(last_key_inited)
      {
        cur_it = day_amounts.lower_bound(last_key);
      }
      else
      {
        cur_it = day_amounts.begin();
      }

      // merge local storage keys with source keys
      unsigned long cur_i = 0;
      while((cur_it != day_amounts.end() || source_it != source_amounts.end()) &&
        cur_i < REPLACE_PORTION_SIZE_)
      {
        assert(source_it == source_amounts.end() ||
          !source_it->second.day_amounts.empty() ||
          source_it->second.prev_day != Generics::Time::ZERO);

        // eval correct use_source_end_date that include timezone offset
        Generics::Time use_source_end_date = def_use_source_end_date;
        if(entities_config)
        {
          const unsigned long entity_id = cur_it != day_amounts.end() ?
            cur_it->first : source_it->first;
          auto it = entities_config->find(entity_id);
          if(it != entities_config->end())
          {
            use_source_end_date = (
              base_use_source_end_time + it->second.time_offset).get_gm_time().get_date();
          }
        }

        if(cur_it == day_amounts.end() ||
          (source_it != source_amounts.end() &&
            source_it->first < cur_it->first))
        {
          // source defined, local not
          // convert source to local (use only data before use_source_end_date)
          // left container should not invalidate iterators after insert
          BillingContainerState::AccountAmountHolder_var new_amount_holder =
            new BillingContainerState::AccountAmountHolder();

          if(replace_account_amount_(
               *new_amount_holder,
               &source_it->second,
               use_source_end_date))
          {
            day_amounts.insert(std::make_pair(source_it->first, new_amount_holder));
            assert(!new_amount_holder->days.empty());
          }

          ++source_it;
        }
        else
        {
          if(source_it == source_amounts.end() ||
            cur_it->first < source_it->first)
          {
            assert(!cur_it->second->days.empty());

            replace_account_amount_(
              *(cur_it->second),
              nullptr,
              use_source_end_date);
          }
          else
          {
            // source and local defined
            assert(source_it->first == cur_it->first);

            replace_account_amount_(
              *(cur_it->second),
              &source_it->second,
              use_source_end_date);

            ++source_it;
          }

          if(cur_it->second->days.empty())
          {
            // all days removed - this is possible if all local days are before
            // use_source_end_date
            // and source not defined or don't have information for these days
            day_amounts.erase(cur_it++);
          }
          else
          {
            ++cur_it;
          }
        }

        ++cur_i;
      }

      target_end_reached = (cur_it == day_amounts.end());
      end_reached = (target_end_reached && source_it == source_amounts.end());

      if(!target_end_reached)
      {
        last_key_inited = true;
        last_key = cur_it->first;
      }

      // unlock amounts_lock
    }
  }

  template<
    typename AmountMapType,
    typename AmountDistributionMapType,
    typename ConfigMapType>
  void
  BillingContainer::replace_amounts_(
    AmountMapType& day_amounts,
    StateSyncPolicy::Mutex& amounts_lock,
    const AmountDistributionMapType& source_amounts,
    const ConfigMapType* entities_config)
    noexcept
  {
    //std::cerr << "DEBUG: entities_config = " << entities_config << std::endl;

    const Generics::Time base_use_source_end_time = Generics::Time(
      Generics::Time::get_time_of_day() - stat_delay_);
    const Generics::Time def_use_source_end_date = (
      base_use_source_end_time - Generics::Time::ONE_HOUR * 14
      // use max possible time offset (UTC + 14)
        ).get_gm_time().get_date();

    bool end_reached = false;
    bool last_key_inited = false;
    bool target_end_reached = false;
    typename AmountMapType::key_type last_key =
      typename AmountMapType::key_type();

    auto source_it = source_amounts.begin();

    while(!end_reached)
    {
      StateSyncPolicy::WriteGuard lock(amounts_lock);
      typename AmountMapType::iterator cur_it;
      if(target_end_reached)
      {
        cur_it = day_amounts.end();
      }
      else if(last_key_inited)
      {
        cur_it = day_amounts.lower_bound(last_key);
      }
      else
      {
        cur_it = day_amounts.begin();
      }

      // merge local storage keys with source keys
      unsigned long cur_i = 0;
      while((cur_it != day_amounts.end() || source_it != source_amounts.end()) &&
        cur_i < REPLACE_PORTION_SIZE_)
      {
        assert(source_it == source_amounts.end() ||
          !source_it->second.day_amount_counts.empty() ||
          source_it->second.prev_day != Generics::Time::ZERO);

        // eval correct use_source_end_date that include timezone offset
        Generics::Time use_source_end_date = def_use_source_end_date;
        if(entities_config)
        {
          const unsigned long entity_id = cur_it != day_amounts.end() ?
            cur_it->first : source_it->first;
          auto it = entities_config->find(entity_id);
          if(it != entities_config->end())
          {
            use_source_end_date = (
              base_use_source_end_time + it->second.time_offset).get_gm_time().get_date();
          }
        }

        if(cur_it == day_amounts.end() ||
          (source_it != source_amounts.end() &&
            source_it->first < cur_it->first))
        {
          // source defined, local not
          // convert source to local (use only data before use_source_end_date)
          // left container should not invalidate iterators after insert
          BillingContainerState::AmountHolder_var new_amount_holder =
            new BillingContainerState::AmountHolder();

          /*
          std::cout << ">>>>> REPLACE #1 : " << source_it->first << std::endl <<
            "  use_source_end_date = " << use_source_end_date.gm_ft() << std::endl <<
            "  source:" << std::endl;
          source_it->second.print(std::cout, "    ");
          */
          if(replace_amount_(
               *new_amount_holder,
               &source_it->second,
               use_source_end_date))
          {
            day_amounts.insert(std::make_pair(source_it->first, new_amount_holder));
            assert(!new_amount_holder->days.empty());

            /*
            std::cout << "  result:" << std::endl;
            new_amount_holder->print(std::cout, "    ");
            std::cout << "<<<<< REPLACE #1 (use)" << std::endl;
            */
          }
          /*
          else
          {
            std::cout << "<<<<< REPLACE #1 (not use)" << std::endl;
          }
          */

          ++source_it;
        }
        else
        {
          if(source_it == source_amounts.end() ||
            cur_it->first < source_it->first)
          {
            assert(!cur_it->second->days.empty());

            // local defined, source not
            /*
            std::cout << ">>>>> REPLACE #2 : " << cur_it->first << std::endl <<
              "  use_source_end_date = " << use_source_end_date.gm_ft() << std::endl <<
              "  target:" << std::endl;
            cur_it->second->print(std::cout, "    ");
            */

            replace_amount_(
              *(cur_it->second),
              nullptr,
              use_source_end_date);

            /*
            std::cout << "  result:" << std::endl;
            cur_it->second->print(std::cout, "    ");
            std::cout << "<<<<< REPLACE #2" << std::endl;
            */
          }
          else
          {
            // source and local defined
            assert(source_it->first == cur_it->first);

            /*
            std::cout << ">>>>> REPLACE #3 : " << source_it->first << std::endl <<
              "  use_source_end_date = " << use_source_end_date.gm_ft() << std::endl <<
              "  source:" << std::endl;
            source_it->second.print(std::cout, "    ");
            std::cout << "  target:" << std::endl;
            cur_it->second->print(std::cout, "    ");
            */

            replace_amount_(
              *(cur_it->second),
              &source_it->second,
              use_source_end_date);

            /*
            std::cout << "  result:" << std::endl;
            cur_it->second->print(std::cout, "    ");
            std::cout << "<<<<< REPLACE #3" << std::endl;
            */
            ++source_it;
          }

          if(cur_it->second->days.empty())
          {
            // all days removed - this is possible if all local days are before
            // use_source_end_date
            // and source not defined or don't have information for these days
            day_amounts.erase(cur_it++);
          }
          else
          {
            ++cur_it;
          }
        }

        ++cur_i;
      }

      target_end_reached = (cur_it == day_amounts.end());
      end_reached = (target_end_reached && source_it == source_amounts.end());

      if(!target_end_reached)
      {
        last_key_inited = true;
        last_key = cur_it->first;
      }

      // unlock amounts_lock
    }
  }

  // replace amount days by source amount info for days < use_source_end_date
  template<typename AmountHolderType>
  bool
  BillingContainer::replace_account_amount_(
    AmountHolderType& amount_holder, // State::DayAmount
    const BillStatSource::Stat::AmountDistribution* source_amount,
    const Generics::Time& use_source_end_date)
    const
    noexcept
  {
    if(!source_amount || source_amount->prev_day < use_source_end_date)
    {
      {
        // erase replace days
        auto replace_end_day_it =
          amount_holder.days.lower_bound(use_source_end_date);

        RevenueDecimal amount_before = RevenueDecimal::ZERO;
        if(replace_end_day_it != amount_holder.days.end())
        {
          amount_before = replace_end_day_it->second.amount_before;
        }

        amount_holder.days.erase(amount_holder.days.begin(), replace_end_day_it);
        for(auto day_it = amount_holder.days.begin();
          day_it != amount_holder.days.end(); ++day_it)
        {
          day_it->second.amount_before -= amount_before;
        }
      }

      if(source_amount)
      {
        auto source_day_it = source_amount->day_amounts.lower_bound(
          use_source_end_date);      

        {
          // collect sum amount for replace days and add it to target days
          RevenueDecimal amount_before = RevenueDecimal::mul(
            source_amount->prev_days_amount,
            limit_coef_,
            Generics::DMR_FLOOR);

          for(auto src_day_it = source_amount->day_amounts.begin();
            src_day_it != source_day_it; ++src_day_it)
          {
            amount_before += RevenueDecimal::mul(
              src_day_it->second,
              limit_coef_,
              Generics::DMR_FLOOR);
          }

          for(auto day_it = amount_holder.days.begin();
            day_it != amount_holder.days.end(); ++day_it)
          {
            day_it->second.amount_before += amount_before;
          }
        }

        {
          // insert replace days
          RevenueDecimal amount_before = RevenueDecimal::mul(
            source_amount->prev_days_amount,
            limit_coef_,
            Generics::DMR_FLOOR);

          if(source_amount->prev_day != Generics::Time::ZERO)
          {
            BillingContainerState::AccountDayAmount prev_day;
            prev_day.amount_before = amount_before;

            amount_holder.days.insert(std::make_pair(
              source_amount->prev_day, prev_day));
          }

          assert(source_amount->day_amounts.empty() ||
            source_amount->prev_day < source_amount->day_amounts.begin()->first);

          for(auto src_day_it = source_amount->day_amounts.begin();
            src_day_it != source_day_it;
            ++src_day_it)
          {
            RevenueDecimal divided_day_amount = RevenueDecimal::mul(
              src_day_it->second,
              limit_coef_,
              Generics::DMR_FLOOR);

            BillingContainerState::AccountDayAmount new_day;
            new_day.amount_before = amount_before;
            new_day.amount = divided_day_amount;

            amount_holder.days.insert(std::make_pair(src_day_it->first, new_day));

            amount_before += divided_day_amount;
          }
        }
      } // if(source_amount)      

      return !amount_holder.days.empty(); // not all local data expired
    }

    return false;
  }

  // replace amount days by source amount info for days < use_source_end_date
  template<typename AmountHolderType>
  bool
  BillingContainer::replace_amount_(
    AmountHolderType& amount_holder, // State::DayAmount
    const BillStatSource::Stat::AmountCountDistribution* source_amount,
    const Generics::Time& use_source_end_date)
    const
    noexcept
  {
    if(!source_amount || source_amount->prev_day < use_source_end_date)
    {
      {
        // erase replace days
        auto replace_end_day_it =
          amount_holder.days.lower_bound(use_source_end_date);

        RevenueDecimal amount_before = RevenueDecimal::ZERO;
        ImpRevenueDecimal imps_before = ImpRevenueDecimal::ZERO;
        ImpRevenueDecimal clicks_before = ImpRevenueDecimal::ZERO;
        if(replace_end_day_it != amount_holder.days.end())
        {
          amount_before = replace_end_day_it->second.amount_before;
          imps_before = replace_end_day_it->second.imps_before;
          clicks_before = replace_end_day_it->second.clicks_before;
        }

        amount_holder.days.erase(amount_holder.days.begin(), replace_end_day_it);
        for(auto day_it = amount_holder.days.begin();
          day_it != amount_holder.days.end(); ++day_it)
        {
          day_it->second.amount_before -= amount_before;
          day_it->second.imps_before -= imps_before;
          day_it->second.clicks_before -= clicks_before;
        }
      }

      if(source_amount)
      {
        auto source_day_it = source_amount->day_amount_counts.lower_bound(
          use_source_end_date);      

        {
          // collect sum amount for replace days and add it to target days
          RevenueDecimal amount_before = RevenueDecimal::mul(
            source_amount->prev_days_amount,
            limit_coef_,
            Generics::DMR_FLOOR);
          ImpRevenueDecimal imps_before = ImpRevenueDecimal::mul(
            source_amount->prev_days_imps,
            ImpRevenueDecimal(limit_coef_),
            Generics::DMR_FLOOR);
          ImpRevenueDecimal clicks_before = ImpRevenueDecimal::mul(
            source_amount->prev_days_clicks,
            ImpRevenueDecimal(limit_coef_),
            Generics::DMR_FLOOR);

          for(auto src_day_it = source_amount->day_amount_counts.begin();
            src_day_it != source_day_it; ++src_day_it)
          {
            amount_before += RevenueDecimal::mul(
              src_day_it->second.amount,
              limit_coef_,
              Generics::DMR_FLOOR);
            imps_before += ImpRevenueDecimal::mul(
              src_day_it->second.imps,
              ImpRevenueDecimal(limit_coef_),
              Generics::DMR_FLOOR);
            clicks_before += ImpRevenueDecimal::mul(
              src_day_it->second.clicks,
              ImpRevenueDecimal(limit_coef_),
              Generics::DMR_FLOOR);
          }

          for(auto day_it = amount_holder.days.begin();
            day_it != amount_holder.days.end(); ++day_it)
          {
            day_it->second.amount_before += amount_before;
            day_it->second.imps_before += imps_before;
            day_it->second.clicks_before += clicks_before;
          }
        }

        {
          // insert replace days
          RevenueDecimal amount_before = RevenueDecimal::mul(
            source_amount->prev_days_amount,
            limit_coef_,
            Generics::DMR_FLOOR);
          ImpRevenueDecimal imps_before = ImpRevenueDecimal::mul(
            source_amount->prev_days_imps,
            ImpRevenueDecimal(limit_coef_),
            Generics::DMR_FLOOR);
          ImpRevenueDecimal clicks_before = ImpRevenueDecimal::mul(
            source_amount->prev_days_clicks,
            ImpRevenueDecimal(limit_coef_),
            Generics::DMR_FLOOR);

          if(source_amount->prev_day != Generics::Time::ZERO)
          {
            BillingContainerState::DayAmount prev_day;
            prev_day.amount_before = amount_before;
            prev_day.imps_before = imps_before;
            prev_day.clicks_before = clicks_before;

            amount_holder.days.insert(std::make_pair(
              source_amount->prev_day, prev_day));
          }

          assert(source_amount->day_amount_counts.empty() ||
            source_amount->prev_day < source_amount->day_amount_counts.begin()->first);

          for(auto src_day_it = source_amount->day_amount_counts.begin();
            src_day_it != source_day_it;
            ++src_day_it)
          {
            RevenueDecimal divided_day_amount = RevenueDecimal::mul(
              src_day_it->second.amount,
              limit_coef_,
              Generics::DMR_FLOOR);
            ImpRevenueDecimal divided_day_imps = ImpRevenueDecimal::mul(
              src_day_it->second.imps,
              ImpRevenueDecimal(limit_coef_),
              Generics::DMR_FLOOR);
            ImpRevenueDecimal divided_day_clicks = ImpRevenueDecimal::mul(
              src_day_it->second.clicks,
              ImpRevenueDecimal(limit_coef_),
              Generics::DMR_FLOOR);

            BillingContainerState::DayAmount new_day;
            new_day.amount_before = amount_before;
            new_day.amount = divided_day_amount;
            new_day.imps_before = imps_before;
            new_day.imps = divided_day_imps;
            new_day.clicks_before = clicks_before;
            new_day.clicks = divided_day_clicks;

            amount_holder.days.insert(std::make_pair(src_day_it->first, new_day));

            amount_before += divided_day_amount;
            imps_before += divided_day_imps;
            clicks_before += divided_day_clicks;
          }
        }
      } // if(source_amount)      

      return !amount_holder.days.empty(); // not all local data expired
    }

    return false;
  }

  void
  BillingContainer::dump()
    /*throw(Exception)*/
  {
    static const char* FUN = "BillingContainer::dump()";

    try
    {
      const std::string tmp_root = storage_root_ + "~";
      const std::string old_storage_tmp_root = storage_root_ + ".bak";

      DumpSyncPolicy::WriteGuard flush_lock(dump_lock_);

      remove_storage_(tmp_root);
      remove_storage_(old_storage_tmp_root);

      if(mkdir(tmp_root.c_str(), 0777) == -1)
      {
        // folder must not exist
        eh::throw_errno_exception<Exception>(
          FUN,
          ": failed to create folder '",
          tmp_root.c_str(),
          "'");
      }

      dump_storage_(tmp_root);

      FileManip::rename(storage_root_, old_storage_tmp_root, true);
      FileManip::rename(tmp_root, storage_root_, false);

      remove_storage_(old_storage_tmp_root);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  BillingContainer::CInternalConfig_var
  BillingContainer::get_config_(bool only_bound) const
    /*throw(BillingProcessor::Exception)*/
  {
    BillingContainer::CInternalConfig_var res = config_.get();
    if(only_bound && !res)
    {
      Stream::Error ostr;
      ostr << "config not bound";
      throw Exception(ostr);
    }
    return res;
  }

  void
  BillingContainer::remove_storage_(const String::SubString& path)
    /*throw(Exception)*/
  {
    static const char* FUN = "BillingContainer::remove_storage_()";

    const std::string path_str = path.str();

    ::unlink((path_str + "/" + ACCOUNTS_FILE).c_str());
    ::unlink((path_str + "/" + CAMPAIGNS_FILE).c_str());
    ::unlink((path_str + "/" + CCGS_FILE).c_str());
    ::unlink((path_str + "/" + CCG_RATE_AMOUNTS_FILE).c_str());
    ::unlink((path_str + "/" + CAMPAIGN_RATE_OPTS_FILE).c_str());

    if(::rmdir(path_str.c_str()) == -1 && errno != ENOENT)
    {
      eh::throw_errno_exception<Exception>(
        FUN,
        ": failed to remove '",
        path_str.c_str(),
        "'");
    }
  }

  void
  BillingContainer::dump_storage_(const String::SubString& path)
    /*throw(Exception)*/
  {
    //static const char* FUN = "BillingContainer::dump_storage_()";

    State_var state = state_;
    assert(state.in());

    save_account_amounts_(path.str() + "/" + ACCOUNTS_FILE,
      state->accounts,
      state->accounts_lock);
    save_amounts_(path.str() + "/" + CAMPAIGNS_FILE,
      state->campaigns,
      state->campaigns_lock);
    save_amounts_(path.str() + "/" + CCGS_FILE,
      state->ccgs,
      state->ccgs_lock);
    save_ccg_rates_(path.str() + "/" + CCG_RATE_AMOUNTS_FILE,
      state->ccg_rate_amounts,
      state->ccg_rate_amounts_lock);
    save_rate_opts_(path.str() + "/" + CAMPAIGN_RATE_OPTS_FILE,
      state->campaign_rate_opts,
      state->campaign_rate_opts_lock);
  }

  BillingContainer::State_var
  BillingContainer::load_storage_(const String::SubString& path)
    /*throw(Exception)*/
  {
    //static const char* FUN = "BillingContainer::dump_storage_()";

    State_var state = new BillingContainerState();
    assert(state.in());

    load_account_amounts_(state->accounts, path.str() + "/" + ACCOUNTS_FILE);
    load_amounts_(state->campaigns, path.str() + "/" + CAMPAIGNS_FILE);
    load_amounts_(state->ccgs, path.str() + "/" + CCGS_FILE);

    const std::string ccg_rate_amounts_file = path.str() + "/" + CCG_RATE_AMOUNTS_FILE;
    if(FileManip::file_exists(ccg_rate_amounts_file))
    {
      load_rates_(state->ccg_rate_amounts, ccg_rate_amounts_file);
    }

    const std::string campaign_rate_opts_file = path.str() + "/" + CAMPAIGN_RATE_OPTS_FILE;
    if(FileManip::file_exists(campaign_rate_opts_file))
    {
      load_rate_opts_(state->campaign_rate_opts, campaign_rate_opts_file);
    }

    return state;
  }

  template<typename AmountMapType>
  void
  BillingContainer::save_account_amounts_(
    const String::SubString& file_path,
    const AmountMapType& amounts,
    StateSyncPolicy::Mutex& amounts_lock)
    /*throw(Exception)*/
  {
    static const char* FUN = "BillingContainer::save_account_amounts_()";

    typedef std::list<typename AmountMapType::value_type> ValueList;

    std::ofstream file(
      file_path.str().c_str(),
      std::ios_base::out | std::ios_base::trunc);

    if(!file.is_open())
    {
      Stream::Error ostr;
      ostr << FUN << ": can't open file '" << file_path << "'";
      throw Exception(ostr);
    }

    const std::string zero_time_str =
      Generics::Time::ZERO.get_gm_time().format("%F");

    bool end_reached = false;
    bool last_key_inited = false;
    bool save_started = false;
    typename AmountMapType::key_type last_key =
      typename AmountMapType::key_type();

    while(!end_reached)
    {
      ValueList save_amounts;

      {
        StateSyncPolicy::ReadGuard lock(amounts_lock);
        typename AmountMapType::const_iterator cur_it;
        if(last_key_inited)
        {
          cur_it = amounts.lower_bound(last_key);
        }
        else
        {
          cur_it = amounts.begin();
        }

        unsigned long cur_i = 0;
        for(; cur_it != amounts.end() && cur_i < SAVE_PORTION_SIZE_;
          ++cur_it, ++cur_i)
        {
          // do deep copy inside lock
          save_amounts.push_back(typename AmountMapType::value_type(
            cur_it->first,
            new BillingContainerState::AccountAmountHolder(*(cur_it->second))));
        }

        if(cur_it != amounts.end())
        {
          last_key_inited = true;
          last_key = cur_it->first;
        }
        else
        {
          end_reached = true;
        }
      }

      // save collected portion
      for(auto it = save_amounts.begin(); it != save_amounts.end(); ++it)
      {
        if(save_started)
        {
          file << std::endl;
        }
        else
        {
          save_started = true;
        }

        assert(!it->second->days.empty());

        auto first_day_it = it->second->days.begin();

        for(auto day_it = first_day_it;
          day_it != it->second->days.end(); ++day_it)
        {
          file << it->first << '\t' <<
            day_it->first.get_gm_time().format("%F") << '\t' <<
            day_it->second.amount <<
            std::endl;
        }

        // storage should garantee that zero amount saved after other
        file << it->first << '\t' << zero_time_str << '\t' <<
          first_day_it->second.amount_before
          ;
      }
    }

    file.close();
  }

  template<typename AmountMapType>
  void
  BillingContainer::save_amounts_(
    const String::SubString& file_path,
    const AmountMapType& amounts,
    StateSyncPolicy::Mutex& amounts_lock)
    /*throw(Exception)*/
  {
    static const char* FUN = "BillingContainer::save_amounts_()";

    typedef std::list<typename AmountMapType::value_type> ValueList;

    std::ofstream file(
      file_path.str().c_str(),
      std::ios_base::out | std::ios_base::trunc);

    if(!file.is_open())
    {
      Stream::Error ostr;
      ostr << FUN << ": can't open file '" << file_path << "'";
      throw Exception(ostr);
    }

    const std::string zero_time_str =
      Generics::Time::ZERO.get_gm_time().format("%F");

    bool end_reached = false;
    bool last_key_inited = false;
    bool save_started = false;
    typename AmountMapType::key_type last_key =
      typename AmountMapType::key_type();

    while(!end_reached)
    {
      ValueList save_amounts;

      {
        StateSyncPolicy::ReadGuard lock(amounts_lock);
        typename AmountMapType::const_iterator cur_it;
        if(last_key_inited)
        {
          cur_it = amounts.lower_bound(last_key);
        }
        else
        {
          cur_it = amounts.begin();
        }

        unsigned long cur_i = 0;
        for(; cur_it != amounts.end() && cur_i < SAVE_PORTION_SIZE_;
          ++cur_it, ++cur_i)
        {
          // do deep copy inside lock
          save_amounts.push_back(typename AmountMapType::value_type(
            cur_it->first,
            new BillingContainerState::AmountHolder(*(cur_it->second))));
        }

        if(cur_it != amounts.end())
        {
          last_key_inited = true;
          last_key = cur_it->first;
        }
        else
        {
          end_reached = true;
        }
      }

      // save collected portion
      for(auto it = save_amounts.begin(); it != save_amounts.end(); ++it)
      {
        if(save_started)
        {
          file << std::endl;
        }
        else
        {
          save_started = true;
        }

        assert(!it->second->days.empty());

        auto first_day_it = it->second->days.begin();

        for(auto day_it = first_day_it;
          day_it != it->second->days.end(); ++day_it)
        {
          file << it->first << '\t' <<
            day_it->first.get_gm_time().format("%F") << '\t' <<
            day_it->second.amount << '\t' <<
            day_it->second.imps.str() << '\t' <<
            day_it->second.clicks.str() <<
            std::endl;
        }

        // storage should garantee that zero amount saved after other
        file << it->first << '\t' << zero_time_str << '\t' <<
          first_day_it->second.amount_before << '\t' <<
          first_day_it->second.imps_before.str() << '\t' <<
          first_day_it->second.clicks_before.str()
          ;
      }
    }

    file.close();
  }

  template<typename RateMapType>
  void
  BillingContainer::save_ccg_rates_(
    const String::SubString& file_path,
    const RateMapType& rates,
    StateSyncPolicy::Mutex& rates_lock)
    /*throw(Exception)*/
  {
    static const char* FUN = "BillingContainer::save_ccg_rates_()";

    typedef std::list<typename RateMapType::value_type> ValueList;

    std::ofstream file(
      file_path.str().c_str(),
      std::ios_base::out | std::ios_base::trunc);

    if(!file.is_open())
    {
      Stream::Error ostr;
      ostr << FUN << ": can't open file '" << file_path << "'";
      throw Exception(ostr);
    }

    const std::string zero_time_str =
      Generics::Time::ZERO.get_gm_time().format("%F");

    bool end_reached = false;
    bool last_key_inited = false;
    bool save_started = false;
    typename RateMapType::key_type last_key =
      typename RateMapType::key_type();

    while(!end_reached)
    {
      ValueList save_rates;

      {
        StateSyncPolicy::ReadGuard lock(rates_lock);
        typename RateMapType::const_iterator cur_it;
        if(last_key_inited)
        {
          cur_it = rates.lower_bound(last_key);
        }
        else
        {
          cur_it = rates.begin();
        }

        unsigned long cur_i = 0;
        for(; cur_it != rates.end() && cur_i < SAVE_RATES_PORTION_SIZE_;
          ++cur_it, ++cur_i)
        {
          // do deep copy inside lock
          save_rates.push_back(*cur_it);
        }

        if(cur_it != rates.end())
        {
          last_key_inited = true;
          last_key = cur_it->first;
        }
        else
        {
          end_reached = true;
        }
      }

      // save collected portion
      for(auto it = save_rates.begin(); it != save_rates.end(); ++it)
      {
        for(auto ti_it = it->second.dates.begin();
          ti_it != it->second.dates.end(); ++ti_it)
        {
          if(!ti_it->second.empty())
          {
            if(save_started)
            {
              file << std::endl;
            }
            else
            {
              save_started = true;
            }

            file <<
              // key
              it->first.campaign_id << '\t' <<
              it->first.ccg_id << '\t' <<
              ti_it->first.get_gm_time().format("%F") << '\t';
            ti_it->second.save(file);
          }
        }
      }
    }

    file.close();
  }

  template<typename RateOptMapType>
  void
  BillingContainer::save_rate_opts_(
    const String::SubString& file_path,
    const RateOptMapType& rate_opts,
    StateSyncPolicy::Mutex& rate_opts_lock)
    /*throw(Exception)*/
  {
    static const char* FUN = "BillingContainer::save_rate_opts_()";

    typedef std::list<typename RateOptMapType::value_type> ValueList;

    std::ofstream file(
      file_path.str().c_str(),
      std::ios_base::out | std::ios_base::trunc);

    if(!file.is_open())
    {
      Stream::Error ostr;
      ostr << FUN << ": can't open file '" << file_path << "'";
      throw Exception(ostr);
    }

    const std::string zero_time_str =
      Generics::Time::ZERO.get_gm_time().format("%F");

    bool end_reached = false;
    bool last_key_inited = false;
    bool save_started = false;
    typename RateOptMapType::key_type last_key = typename RateOptMapType::key_type();

    while(!end_reached)
    {
      ValueList save_rates;

      {
        StateSyncPolicy::ReadGuard lock(rate_opts_lock);
        typename RateOptMapType::const_iterator cur_it;
        if(last_key_inited)
        {
          cur_it = rate_opts.lower_bound(last_key);
        }
        else
        {
          cur_it = rate_opts.begin();
        }

        unsigned long cur_i = 0;
        for(; cur_it != rate_opts.end() && cur_i < SAVE_RATE_OPTS_PORTION_SIZE_;
          ++cur_it, ++cur_i)
        {
          // do deep copy inside lock
          save_rates.push_back(*cur_it);
        }

        if(cur_it != rate_opts.end())
        {
          last_key_inited = true;
          last_key = cur_it->first;
        }
        else
        {
          end_reached = true;
        }
      }

      // save collected portion
      for(auto it = save_rates.begin(); it != save_rates.end(); ++it)
      {
        for(auto ti_it = it->second.dates.begin();
          ti_it != it->second.dates.end(); ++ti_it)
        {
          if(!ti_it->second.empty())
          {
            if(save_started)
            {
              file << std::endl;
            }
            else
            {
              save_started = true;
            }

            file << it->first << '\t' <<
              ti_it->first.get_gm_time().format("%F") << '\t';
            ti_it->second.save(file);
          }
        }
      }
    }

    file.close();
  }

  template<typename AmountMapType>
  void
  BillingContainer::load_account_amounts_(
    AmountMapType& amounts,
    const String::SubString& file_path)
    /*throw(Exception)*/
  {
    static const char* FUN = "BillingContainer::load_account_amounts_()";

    std::ifstream file(
      file_path.str().c_str(),
      std::ios_base::in);

    if(!file.is_open())
    {
      Stream::Error ostr;
      ostr << FUN << ": can't open file '" << file_path << "'";
      throw Exception(ostr);
    }

    unsigned long line_i = 0;

    {
      // empty file allowed
      char sym;
      file.get(sym);
      if(!file.eof())
      {
        file.putback(sym);
      }
    }

    while(!file.eof())
    {
      std::string line;

      try
      {
        AdServer::LogProcessing::read_until_eol(file, line);
        if(file.fail())
        {
          throw Exception("read failed or empty line");
        }

        String::StringManip::SplitTab tokenizer(line);

        String::SubString id_str;
        if(!tokenizer.get_token(id_str))
        {
          throw Exception("no id");
        }

        String::SubString date_str;
        if(!tokenizer.get_token(date_str))
        {
          throw Exception("no date");
        }

        String::SubString amount_str;
        if(!tokenizer.get_token(amount_str))
        {
          throw Exception("no amount");
        }

        if(amount_str.end() != &line[0] + line.size())
        {
          throw Exception("unexpected content after amount");
        }

        unsigned long id;
        if(!String::StringManip::str_to_int(id_str, id))
        {
          Stream::Error ostr;
          ostr << "invalid id value: '" << id_str << "'";
          throw Exception(ostr);
        }

        {
          char eol;
          file.get(eol);
          if(!file.eof() && (file.fail() || eol != '\n'))
          {
            Stream::Error ostr;
            ostr << "line isn't closed when expected";
            throw Exception(ostr);
          }
        }

        Generics::Time date(date_str, "%Y-%m-%d"); // %F
        RevenueDecimal amount(amount_str);

        BillingContainerState::AccountAmountHolder_var& amount_holder = amounts[id];

        if(!amount_holder)
        {
          amount_holder = new BillingContainerState::AccountAmountHolder();
        }

        if(date == Generics::Time::ZERO)
        {
          assert(!amount_holder->days.empty());
          // storage currupted, logic don't allow this
          amount_holder->add_prev_days_amount(amount);
        }
        else
        {
          amount_holder->add_amount(date, amount);
        }
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": can't read '" << file_path <<
          "', invalid line #" << line_i << " '" << line <<
          "': " << ex.what();
        throw Exception(ostr);
      }

      ++line_i;
    }

    file.close();
  }

  template<typename AmountMapType>
  void
  BillingContainer::load_amounts_(
    AmountMapType& amounts,
    const String::SubString& file_path)
    /*throw(Exception)*/
  {
    static const char* FUN = "BillingContainer::load_amounts_()";

    std::ifstream file(
      file_path.str().c_str(),
      std::ios_base::in);

    if(!file.is_open())
    {
      Stream::Error ostr;
      ostr << FUN << ": can't open file '" << file_path << "'";
      throw Exception(ostr);
    }

    unsigned long line_i = 0;

    {
      // empty file allowed
      char sym;
      file.get(sym);
      if(!file.eof())
      {
        file.putback(sym);
      }
    }

    while(!file.eof())
    {
      std::string line;

      try
      {
        // load: id,date,amount,imps,clicks
        AdServer::LogProcessing::read_until_eol(file, line);
        if(file.fail())
        {
          throw Exception("read failed or empty line");
        }

        String::StringManip::SplitTab tokenizer(line);

        String::SubString id_str;
        if(!tokenizer.get_token(id_str))
        {
          throw Exception("no id");
        }

        String::SubString date_str;
        if(!tokenizer.get_token(date_str))
        {
          throw Exception("no date");
        }

        String::SubString amount_str;
        if(!tokenizer.get_token(amount_str))
        {
          throw Exception("no amount");
        }

        /*
        if(amount_str.end() != &line[0] + line.size())
        {
          throw Exception("unexpected content after amount");
        }
        */

        String::SubString imps_str;
        String::SubString clicks_str;

        if(tokenizer.get_token(imps_str))
        {
          if(!tokenizer.get_token(clicks_str))
          {
            throw Exception("no amount");
          }
          else if(clicks_str.end() != &line[0] + line.size())
          {
            throw Exception("unexpected content after amount");
          }
        }

        unsigned long id;
        if(!String::StringManip::str_to_int(id_str, id))
        {
          Stream::Error ostr;
          ostr << "invalid id value: '" << id_str << "'";
          throw Exception(ostr);
        }

        {
          char eol;
          file.get(eol);
          if(!file.eof() && (file.fail() || eol != '\n'))
          {
            Stream::Error ostr;
            ostr << "line isn't closed when expected";
            throw Exception(ostr);
          }
        }

        ImpRevenueDecimal imps = ImpRevenueDecimal::ZERO;
        if(!imps_str.empty())
        {
          imps = ImpRevenueDecimal(imps_str);
        }

        ImpRevenueDecimal clicks = ImpRevenueDecimal::ZERO;
        if(!clicks_str.empty())
        {
          clicks = ImpRevenueDecimal(clicks_str);
        }

        Generics::Time date(date_str, "%Y-%m-%d"); // %F
        RevenueDecimal amount(amount_str);

        BillingContainerState::AmountHolder_var& amount_holder = amounts[id];

        if(!amount_holder)
        {
          amount_holder = new BillingContainerState::AmountHolder();
        }

        if(date == Generics::Time::ZERO)
        {
          assert(!amount_holder->days.empty());
          // storage currupted, logic don't allow this
          amount_holder->add_prev_days_amount(amount, imps, clicks);
        }
        else
        {
          amount_holder->add_amount(date, amount, imps, clicks);
        }
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": can't read '" << file_path <<
          "', invalid line #" << line_i << " '" << line <<
          "': " << ex.what();
        throw Exception(ostr);
      }

      ++line_i;
    }

    file.close();
  }

  template<typename RateMapType>
  void
  BillingContainer::load_rates_(
    RateMapType& rates,
    const String::SubString& file_path)
    /*throw(Exception)*/
  {
    // rates file format:
    // <id>\t<date>\t<hour:ctr:imps:clicks,...>
    static const char* FUN = "BillingContainer::load_rates_()";

    std::ifstream file(file_path.str().c_str(), std::ios_base::in);

    if(!file.is_open())
    {
      Stream::Error ostr;
      ostr << FUN << ": can't open file '" << file_path << "'";
      throw Exception(ostr);
    }

    unsigned long line_i = 0;

    {
      // empty file allowed
      char sym;
      file.get(sym);
      if(!file.eof())
      {
        file.putback(sym);
      }
    }

    while(!file.eof())
    {
      std::string line;

      try
      {
        AdServer::LogProcessing::read_until_eol(file, line);
        if(file.fail())
        {
          throw Exception("read failed or empty line");
        }

        String::StringManip::SplitTab tokenizer(line);

        String::SubString campaign_id_str;
        if(!tokenizer.get_token(campaign_id_str))
        {
          throw Exception("no campaign_id");
        }

        String::SubString ccg_id_str;
        if(!tokenizer.get_token(ccg_id_str))
        {
          throw Exception("no ccg_id");
        }

        String::SubString date_str;
        if(!tokenizer.get_token(date_str))
        {
          throw Exception("no date");
        }

        String::SubString ctrs_str;
        if(!tokenizer.get_token(ctrs_str))
        {
          throw Exception("no ctr");
        }

        if(ctrs_str.end() != &line[0] + line.size())
        {
          throw Exception("unexpected content after amounts");
        }

        unsigned long campaign_id;
        if(!String::StringManip::str_to_int(campaign_id_str, campaign_id))
        {
          Stream::Error ostr;
          ostr << "invalid campaign_id value: '" << campaign_id_str << "'";
          throw Exception(ostr);
        }

        unsigned long ccg_id;
        if(!String::StringManip::str_to_int(ccg_id_str, ccg_id))
        {
          Stream::Error ostr;
          ostr << "invalid ccg_id value: '" << ccg_id_str << "'";
          throw Exception(ostr);
        }

        {
          char eol;
          file.get(eol);
          if(!file.eof() && (file.fail() || eol != '\n'))
          {
            Stream::Error ostr;
            ostr << "line isn't closed when expected";
            throw Exception(ostr);
          }
        }

        Generics::Time date(date_str, "%Y-%m-%d"); // %F

        BillingContainerState::RateAmountHolder& rate_holder = rates[
          BillingContainerState::CampaignCCGId(campaign_id, ccg_id)];

        BillingContainerState::RateDistributionHolder rate_distribution_holder;
        rate_distribution_holder.load(ctrs_str);

        rate_holder.dates.insert(std::make_pair(date, rate_distribution_holder));
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": can't read '" << file_path <<
          "', invalid line #" << line_i << " '" << line <<
          "': " << ex.what();
        throw Exception(ostr);
      }

      ++line_i;
    }

    file.close();
  }

  template<typename RateOptMapType>
  void
  BillingContainer::load_rate_opts_(
    RateOptMapType& rate_opts,
    const String::SubString& file_path)
    /*throw(Exception)*/
  {
    // rates file format:
    // <id>\t<date>\t<min_rate,...>
    static const char* FUN = "BillingContainer::load_rates_()";

    std::ifstream file(file_path.str().c_str(), std::ios_base::in);

    if(!file.is_open())
    {
      Stream::Error ostr;
      ostr << FUN << ": can't open file '" << file_path << "'";
      throw Exception(ostr);
    }

    unsigned long line_i = 0;

    {
      // empty file allowed
      char sym;
      file.get(sym);
      if(!file.eof())
      {
        file.putback(sym);
      }
    }

    while(!file.eof())
    {
      std::string line;

      try
      {
        AdServer::LogProcessing::read_until_eol(file, line);
        if(file.fail())
        {
          throw Exception("read failed or empty line");
        }

        String::StringManip::SplitTab tokenizer(line);

        String::SubString campaign_id_str;
        if(!tokenizer.get_token(campaign_id_str))
        {
          throw Exception("no campaign_id");
        }

        String::SubString date_str;
        if(!tokenizer.get_token(date_str))
        {
          throw Exception("no date");
        }

        String::SubString min_rates_str;
        if(!tokenizer.get_token(min_rates_str))
        {
          throw Exception("no min rates");
        }

        if(min_rates_str.end() != &line[0] + line.size())
        {
          throw Exception("unexpected content after min rates");
        }

        unsigned long campaign_id;
        if(!String::StringManip::str_to_int(campaign_id_str, campaign_id))
        {
          Stream::Error ostr;
          ostr << "invalid campaign_id value: '" << campaign_id_str << "'";
          throw Exception(ostr);
        }

        {
          char eol;
          file.get(eol);
          if(!file.eof() && (file.fail() || eol != '\n'))
          {
            Stream::Error ostr;
            ostr << "line isn't closed when expected";
            throw Exception(ostr);
          }
        }

        Generics::Time date(date_str, "%Y-%m-%d"); // %F

        BillingContainerState::RateOptimizationHolder::DateHolder& rate_holder =
          rate_opts[campaign_id].dates[date];

        rate_holder.load(min_rates_str);
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": can't read '" << file_path <<
          "', invalid line #" << line_i << " '" << line <<
          "': " << ex.what();
        throw Exception(ostr);
      }

      ++line_i;
    }

    file.close();
  }

  template<typename DeliveryLimitsType>
    // CommonDeliveryLimits + DeliveryLimitsCalcHelper
  void
  BillingContainer::adapt_delivery_limits_(
    DeliveryLimitsType& delivery_limits)
    const noexcept
  {
    if(delivery_limits.daily_budget)
    {
      delivery_limits.daily_budget = RevenueDecimal::mul(
        *delivery_limits.daily_budget,
        limit_coef_,
        Generics::DMR_FLOOR);
    }

    if(delivery_limits.budget)
    {
      delivery_limits.budget = RevenueDecimal::mul(
        *delivery_limits.budget,
        limit_coef_,
        Generics::DMR_FLOOR);
    }

    if(delivery_limits.daily_imps.has_value())
    {
      delivery_limits.daily_imps_dec = ImpRevenueDecimal::mul(
        ImpRevenueDecimal(false, *delivery_limits.daily_imps, 0),
        ImpRevenueDecimal(limit_coef_),
        Generics::DMR_FLOOR);
    }

    if(delivery_limits.imps.has_value())
    {
      delivery_limits.imps_dec = ImpRevenueDecimal::mul(
        ImpRevenueDecimal(false, *delivery_limits.imps, 0),
        ImpRevenueDecimal(limit_coef_),
        Generics::DMR_FLOOR);
    }

    if(delivery_limits.daily_clicks.has_value())
    {
      delivery_limits.daily_clicks_dec = ImpRevenueDecimal::mul(
        ImpRevenueDecimal(false, *delivery_limits.daily_clicks, 0),
        ImpRevenueDecimal(limit_coef_),
        Generics::DMR_FLOOR);
    }

    if(delivery_limits.clicks.has_value())
    {
      delivery_limits.clicks_dec = ImpRevenueDecimal::mul(
        ImpRevenueDecimal(false, *delivery_limits.clicks, 0),
        ImpRevenueDecimal(limit_coef_),
        Generics::DMR_FLOOR);
    }

    delivery_limits.adv_tz_date_end =
      delivery_limits.date_end != Generics::Time::ZERO ?
        (delivery_limits.date_end + delivery_limits.time_offset
          ).get_gm_time().get_date() :
        Generics::Time::ZERO;
  }

  RevenueDecimal
  BillingContainer::round_rate_(
    const RevenueDecimal& rate)
    noexcept
  {
    return RevenueDecimal(rate).floor(4);
  }
}
}
