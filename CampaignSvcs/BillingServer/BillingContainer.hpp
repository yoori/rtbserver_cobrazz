#ifndef CAMPAIGNSVCS_BILLINGCONTAINER_HPP
#define CAMPAIGNSVCS_BILLINGCONTAINER_HPP

#include <list>
#include <vector>
#include <string>
#include <unordered_map>

#include <ReferenceCounting/ReferenceCounting.hpp>
#include <ReferenceCounting/PtrHolder.hpp>

#include <Logger/Logger.hpp>
#include <Sync/SyncPolicy.hpp>

#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>
#include <CampaignSvcs/CampaignServer/BillStatSource.hpp>

#include "CTROptimizer.hpp"

namespace AdServer
{
namespace CampaignSvcs
{
  struct BillingContainerState;

  struct BillingProcessor: public virtual ReferenceCounting::Interface
  {
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    struct Bid
    {
      Generics::Time time;
      unsigned long account_id;
      unsigned long advertiser_id;
      unsigned long campaign_id;
      unsigned long ccg_id;
      RevenueDecimal ctr;
      bool optimize_campaign_ctr;

      void
      print(std::ostream& out) const noexcept;
    };

    struct BidResult
    {
      BidResult(bool available_val, const RevenueDecimal& goal_ctr_val)
        noexcept;

      bool available;
      RevenueDecimal goal_ctr;
    };

    // check_available_bid
    virtual BidResult
    check_available_bid(const Bid& bid)
      /*throw(Exception)*/ = 0;

    // confirm_bid
    virtual BidResult
    confirm_bid(
      RevenueDecimal& account_bid_amount, // inout, contains remind amount
      RevenueDecimal& bid_amount,
      ImpRevenueDecimal& imps,
      ImpRevenueDecimal& clicks,
      const Bid& bid,
      bool forced)
      /*throw(Exception)*/ = 0;

    // reserve_bid
    virtual bool
    reserve_bid(
      const Bid& bid,
      const RevenueDecimal& bid_amount)
      /*throw(Exception)*/ = 0;

  protected:
    virtual ~BillingProcessor() noexcept = default;
  };

  typedef ReferenceCounting::SmartPtr<BillingProcessor>
    BillingProcessor_var;

  class BillingContainer:
    public BillingProcessor,
    public ReferenceCounting::AtomicImpl
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    struct Config: public ReferenceCounting::AtomicImpl
    {
      struct Account
      {
        bool active;
        Generics::Time time_offset;
        RevenueDecimal budget;
      };

      typedef std::unordered_map<unsigned long, Account>
        AccountMap;

      struct CommonDeliveryLimits: public CampaignDeliveryLimits
      {
        bool active;
        Generics::Time time_offset;
        std::optional<ImpRevenueDecimal> imps_dec;
        std::optional<ImpRevenueDecimal> daily_imps_dec;
        std::optional<ImpRevenueDecimal> clicks_dec;
        std::optional<ImpRevenueDecimal> daily_clicks_dec;
      };

      struct Campaign: public CommonDeliveryLimits
      {
      };

      typedef std::unordered_map<unsigned long, Campaign>
        CampaignMap;

      struct CCG: public CommonDeliveryLimits
      {
        unsigned long campaign_id;
        RevenueDecimal imp_amount;
        RevenueDecimal click_amount;
      };

      typedef std::unordered_map<unsigned long, CCG>
        CCGMap;

      AccountMap accounts;
      CampaignMap campaigns;
      CCGMap ccgs;

    protected:
      virtual ~Config() noexcept = default;
    };

    typedef ReferenceCounting::SmartPtr<Config> Config_var;

  public:
    BillingContainer(
      Logging::Logger* logger,
      const String::SubString& storage_root,
      const Generics::Time& stat_delay,
      unsigned long limits_divider)
      /*throw(Exception)*/;

    virtual BidResult
    check_available_bid(const Bid& bid)
      /*throw(BillingProcessor::Exception)*/;

    virtual BidResult
    confirm_bid(
      RevenueDecimal& account_bid_amount,
      RevenueDecimal& bid_amount,
      ImpRevenueDecimal& imps,
      ImpRevenueDecimal& clicks,
      const Bid& bid,
      bool forced)
      /*throw(BillingProcessor::Exception)*/;

    virtual bool
    reserve_bid(
      const Bid& bid,
      const RevenueDecimal& bid_amount)
      /*throw(BillingProcessor::Exception)*/;

    void
    clear_expired_reservation(const Generics::Time& time)
      /*throw(BillingProcessor::Exception)*/;

    void
    config(Config* new_config)
      noexcept;

    void
    stat(BillStatSource::Stat* bill_stat)
      noexcept;

    // dump state to persistent storage
    void
    dump() /*throw(Exception)*/;

  protected:
    typedef Sync::Policy::PosixThreadRW SyncPolicy;
    typedef Sync::Policy::PosixThreadRW StateSyncPolicy;
    typedef Sync::Policy::PosixThread DumpSyncPolicy;

    typedef ReferenceCounting::QualPtr<BillingContainerState> State_var;

    struct InternalConfig;

    typedef ReferenceCounting::QualPtr<InternalConfig>
      InternalConfig_var;

    typedef ReferenceCounting::ConstPtr<InternalConfig>
      CInternalConfig_var;

    struct ConfirmAmountHolder
    {
      RevenueDecimal available_amount;
      RevenueDecimal confirmed_amount;
      RevenueDecimal revert_amount;
    };

    struct CampaignConfirmAmountHolder: public ConfirmAmountHolder
    {
      ImpRevenueDecimal available_imps;
      ImpRevenueDecimal confirmed_imps;
      ImpRevenueDecimal revert_imps;

      ImpRevenueDecimal available_clicks;
      ImpRevenueDecimal confirmed_clicks;
      ImpRevenueDecimal revert_clicks;
    };

    typedef Sync::Policy::PosixThreadRW RateOptimizationSyncPolicy;

    typedef std::set<unsigned long> IdSet;

  protected:
    virtual
    ~BillingContainer() noexcept = default;

    CInternalConfig_var
    get_config_(bool only_bound) const
      /*throw(BillingProcessor::Exception)*/;

    // processor operation helpers
    template<
      typename AmountMapType,
      typename AccountDeliveryLimitsType>
    bool
    check_available_account_budget_(
      const AmountMapType& amounts,
      StateSyncPolicy::Mutex& amounts_lock,
      unsigned long account_id,
      const AccountDeliveryLimitsType& delivery_limits)
      noexcept;

    template<typename AmountMapType, typename DeliveryLimitsType>
    static bool
    check_available_budget_(
      const AmountMapType& amounts,
      StateSyncPolicy::Mutex& amounts_lock,
      unsigned long object_id,
      const DeliveryLimitsType& delivery_limits,
      const Generics::Time& now)
      noexcept;

    template<
      typename RateGoalMapType,
      typename RateOptMapType,
      typename RateAmountMapType,
      typename DailyAmountMapType,
      typename DeliveryLimitsType>
    bool
    check_min_rate_goal_(
      RevenueDecimal& min_rate_goal,
      const InternalConfig* config,
      const RevenueDecimal& rate,
      RateGoalMapType& rate_goals, // goals
      StateSyncPolicy::Mutex& rate_goals_lock,
      RateOptMapType& rate_opts, // opts
      StateSyncPolicy::Mutex& rate_opts_lock,
      RateAmountMapType& rate_amounts, // amounts
      StateSyncPolicy::Mutex& rate_amounts_lock,
      const DailyAmountMapType& daily_amounts, // daily amounts
      StateSyncPolicy::Mutex& daily_amounts_lock,
      unsigned long object_id,
      const DeliveryLimitsType& delivery_limits,
      const Generics::Time& now)
      noexcept;

    template<typename AmountMapType, typename AccountDeliveryLimitsType>
    bool
    confirm_account_bid_(
      AmountMapType& amounts,
      StateSyncPolicy::Mutex& amounts_lock,
      RevenueDecimal& confirmed_amount,
      const RevenueDecimal& confirm_amount,
      unsigned long account_id,
      const AccountDeliveryLimitsType& account_config,
      const Generics::Time& now,
      bool forced)
      noexcept;

    template<typename AmountMapType, typename DeliveryLimitsType>
    static bool
    confirm_bid_(
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
      noexcept;

    template<
      typename RateGoalMapType,
      typename RateAmountMapType,
      typename DeliveryLimitsType>
    void
    confirm_bid_rate_(
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
      noexcept;

    template<typename AmountMapType, typename DeliveryLimitsType>
    static void
    revert_account_confirmed_bid_(
      const AmountMapType& amounts,
      StateSyncPolicy::Mutex& amounts_lock,
      unsigned long object_id,
      const DeliveryLimitsType& delivery_limits,
      const Generics::Time& now,
      const RevenueDecimal& confirm_amount)
      noexcept;

    template<typename AmountMapType, typename DeliveryLimitsType>
    static void
    revert_confirmed_bid_(
      const AmountMapType& amounts,
      StateSyncPolicy::Mutex& amounts_lock,
      unsigned long object_id,
      const DeliveryLimitsType& delivery_limits,
      const Generics::Time& now,
      const RevenueDecimal& confirm_amount,
      const ImpRevenueDecimal& imps,
      const ImpRevenueDecimal& clicks)
      noexcept;

    // state merge operations
    template<
      typename AmountMapType,
      typename AmountDistributionMapType,
      typename ConfigMapType>
    void
    replace_account_amounts_(
      AmountMapType& amounts,
      StateSyncPolicy::Mutex& amounts_lock,
      const AmountDistributionMapType& source_amounts,
      const ConfigMapType* entities_config)
      noexcept;

    template<
      typename AmountMapType,
      typename AmountDistributionMapType,
      typename ConfigMapType>
    void
    replace_amounts_(
      AmountMapType& amounts,
      StateSyncPolicy::Mutex& amounts_lock,
      const AmountDistributionMapType& source_amounts,
      const ConfigMapType* entities_config)
      noexcept;

    template<typename DayAmountType>
    bool
    replace_account_amount_(
      DayAmountType& amounts,
      const BillStatSource::Stat::AmountDistribution* source_amount,
      const Generics::Time& use_source_end_date)
      const
      noexcept;

    template<typename DayAmountType>
    bool
    replace_amount_(
      DayAmountType& amounts,
      const BillStatSource::Stat::AmountCountDistribution* source_amount,
      const Generics::Time& use_source_end_date)
      const
      noexcept;

    template<
      typename RateOptMapType,
      typename RateAmountMapType,
      typename DailyAmountMapType,
      typename DeliveryLimitsType>
    void
    make_min_rate_goal_optimization_(
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
      noexcept;

    void
    recalc_remind_amounts_(
      ConfirmAmountHolder& account_confirm,
      CampaignConfirmAmountHolder& campaign_confirm,
      CampaignConfirmAmountHolder& ccg_confirm,
      bool account_bid_amount_confirmed,
      bool campaign_bid_amount_confirmed,
      bool ccg_bid_amount_confirmed,
      const RevenueDecimal& account_bid_amount,
      const RevenueDecimal& bid_amount)
      noexcept;

    // storage operations
    void
    remove_storage_(const String::SubString& path)
      /*throw(Exception)*/;

    // storage save methods
    void
    dump_storage_(const String::SubString& path)
      /*throw(Exception)*/;

    template<typename AmountMapType>
    void
    save_account_amounts_(
      const String::SubString& file_path,
      const AmountMapType& amounts,
      StateSyncPolicy::Mutex& amounts_lock);

    template<typename AmountMapType>
    void
    save_amounts_(const String::SubString& file,
      const AmountMapType& amounts,
      StateSyncPolicy::Mutex& amounts_lock)
      /*throw(Exception)*/;

    template<typename RateMapType>
    void
    save_ccg_rates_(
      const String::SubString& file_path,
      const RateMapType& amounts,
      StateSyncPolicy::Mutex& rates_lock)
      /*throw(Exception)*/;

    template<typename RateOptMapType>
    void
    save_rate_opts_(
      const String::SubString& file_path,
      const RateOptMapType& rate_opts,
      StateSyncPolicy::Mutex& rate_opts_lock)
      /*throw(Exception)*/;

    // storage load methods
    State_var
    load_storage_(const String::SubString& path)
      /*throw(Exception)*/;

    template<typename AmountMapType>
    void
    load_account_amounts_(
      AmountMapType& amounts,
      const String::SubString& file_path);

    template<typename AmountMapType>
    void
    load_amounts_(
      AmountMapType& amounts,
      const String::SubString& file_path)
      /*throw(Exception)*/;

    template<typename RateMapType>
    void
    load_rates_(
      RateMapType& amounts,
      const String::SubString& file_path)
      /*throw(Exception)*/;

    template<typename RateOptMapType>
    void
    load_rate_opts_(
      RateOptMapType& rate_opts,
      const String::SubString& file_path)
      /*throw(Exception)*/;

    // utils
    template<typename DeliveryLimitsType>
    void
    adapt_delivery_limits_(
      DeliveryLimitsType& delivery_limits)
      const noexcept;

    static RevenueDecimal
    round_rate_(const RevenueDecimal& rate)
      noexcept;

  protected:
    Logging::Logger_var logger_;
    const std::string storage_root_;
    const Generics::Time stat_delay_;
    const RevenueDecimal limit_coef_;
    const unsigned long SAVE_PORTION_SIZE_;
    const unsigned long REPLACE_PORTION_SIZE_;
    const unsigned long SAVE_RATES_PORTION_SIZE_;
    const unsigned long SAVE_RATE_OPTS_PORTION_SIZE_;

    const RevenueDecimal OPTIMIZE_CTR_FACTUAL_COEF_;
    const RevenueDecimal OPTIMIZE_CTR_PREDICTED_COEF_;
    const RevenueDecimal OPTIMIZE_CTR_IGNORE_NOISE_PART_;
    const RevenueDecimal OPTIMIZE_CTR_RATE_MULTIPLIER_;
    const std::unique_ptr<CTROptimizer> ctr_optimizer_;

    // campaign optimization locks
    RateOptimizationSyncPolicy::Mutex campaign_min_ctr_goal_lock_;
    IdSet campaign_ctr_optimization_ids_;

    State_var state_;
    ReferenceCounting::PtrHolder<BillStatSource::Stat_var> stat_;
    ReferenceCounting::PtrHolder<InternalConfig_var> config_;

    DumpSyncPolicy::Mutex dump_lock_;
  };

  typedef ReferenceCounting::SmartPtr<BillingContainer>
    BillingContainer_var;
}
}

namespace AdServer
{
namespace CampaignSvcs
{
  // BillingProcessor::Bid
  inline void
  BillingProcessor::Bid::print(std::ostream& out) const noexcept
  {
    out << "time = " << time.gm_ft() <<
      ", account_id = " << account_id <<
      ", advertiser_id = " << advertiser_id <<
      ", campaign_id = " << campaign_id <<
      ", ccg_id = " << ccg_id <<
      ", optimize_campaign_ctr = " << optimize_campaign_ctr;
  }

  // BillingProcessor::BidResult
  inline
  BillingProcessor::BidResult::BidResult(
    bool available_val,
    const RevenueDecimal& goal_ctr_val)
    noexcept
    : available(available_val),
      goal_ctr(goal_ctr_val)
  {}
}
}

#endif /*CAMPAIGNSVCS_BILLINGCONTAINER_HPP*/
