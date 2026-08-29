#include <cstddef>
#include <cstdint>
#include <utility>

#include <Commons/Algs.hpp>
#include <CampaignSvcs/CampaignCommons/ExpressionChannel.hpp>
#include <LogCommons/ChannelHitStat.hpp>
#include <LogCommons/ChannelImpInventory.hpp>
#include <LogCommons/ChannelInventory.hpp>
#include <LogCommons/ChannelPriceRange.hpp>
#include <LogCommons/ChannelPerformance.hpp>
#include <LogCommons/ChannelTriggerImpStat.hpp>
#include <LogCommons/ChannelTriggerStat.hpp>

#include "ExpressionMatcherOutLogger.hpp"

namespace AdServer::RequestInfoSvcs
{
  namespace
  {
    template<
      typename Container,
      typename UnaryFunction>
    Algs::IteratorRange<Algs::TransformIterator<
      typename Container::const_iterator,
      UnaryFunction> >
    make_transform_range(const Container& cont, UnaryFunction func)
    {
      return Algs::IteratorRange<Algs::TransformIterator<typename Container::const_iterator, UnaryFunction> >(
        Algs::TransformIterator<typename Container::const_iterator, UnaryFunction>(cont.begin(), func),
        Algs::TransformIterator<typename Container::const_iterator, UnaryFunction>(cont.end(), func));
    }
  }

  /**
   * ExpressionMatcherOutLogger::ChannelInventoryLogger
   */
  class ExpressionMatcherOutLogger::ChannelInventoryLogger:
    public AdServer::LogProcessing::LogHolderSharded<
      AdServer::LogProcessing::ChannelInventoryTraits>
  {
  public:
    typedef AdServer::LogProcessing::ChannelInventoryInnerKey ChannelInventoryInnerKey;
    typedef AdServer::LogProcessing::ChannelInventoryInnerData ChannelInventoryInnerData;
    typedef AdServer::LogProcessing::ChannelInventoryKey ChannelInventoryKey;
    typedef AdServer::LogProcessing::ChannelInventoryData ChannelInventoryData;

  public:
    ChannelInventoryLogger(
      unsigned long placement_colo_id,
      const RevenueDecimal& simple_factor,
      const AdServer::LogProcessing::LogFlushTraits& flush_traits)
      /*throw(LoggerException)*/
      : AdServer::LogProcessing::LogHolderSharded<
          AdServer::LogProcessing::ChannelInventoryTraits>(flush_traits),
        placement_colo_id_(placement_colo_id),
        users_simpl_factor_(simple_factor)
    {}

    void
    add_inv_record_(
      const ChannelIdArray& channels,
      const ChannelInventoryInnerData& channel_props,
      ChannelInventoryData& ch_inv_data)
      /*throw(eh::Exception)*/
    {
      for (auto ch_it = channels.begin(); ch_it != channels.end(); ++ch_it)
      {
        ch_inv_data.add(ChannelInventoryInnerKey(*ch_it), channel_props);
      }
    }

    virtual void
    process_user(const InventoryUserInfo& inv_info)
      /*throw(eh::Exception)*/
    {
      if (!inv_info.total_appear_channels.empty())
      {
        const ChannelInventoryKey ch_inv_key(inv_info.placement_colo_time, placement_colo_id_);

        LogProcessing::ChannelInventoryCollector::DataT ch_inv_data;

        // fill appears
        add_inv_record_(
          inv_info.total_appear_channels,
          ChannelInventoryInnerData(
            RevenueDecimal::ZERO,
            RevenueDecimal::ZERO,
            users_simpl_factor_),
          ch_inv_data);

        add_record(ch_inv_key, ch_inv_data);
      }
    }

    virtual void
    process_request(const InventoryInfo& inv_info)
      /*throw(eh::Exception)*/
    {
      if (!inv_info.total_appear_channels.empty() || !inv_info.active_appear_channels.empty())
      {
        const ChannelInventoryKey ch_inv_key(inv_info.placement_colo_time, placement_colo_id_);
        ChannelInventoryData ch_inv_data;

        // fill appears
        add_inv_record_(
          inv_info.total_appear_channels,
          ChannelInventoryInnerData(
            RevenueDecimal::ZERO, RevenueDecimal::ZERO, users_simpl_factor_),
          ch_inv_data);

        add_inv_record_(
          inv_info.active_appear_channels,
          ChannelInventoryInnerData(
            RevenueDecimal::ZERO, users_simpl_factor_, RevenueDecimal::ZERO),
          ch_inv_data);

        add_record(ch_inv_key, ch_inv_data);
      }
    }

  protected:
    virtual
    ~ChannelInventoryLogger() noexcept = default;

  private:
    const unsigned long placement_colo_id_;
    const RevenueDecimal users_simpl_factor_;
  };

  /**
   * ExpressionMatcherOutLogger::ChannelImpInventoryLogger
   */
  template<bool SAMPLING_FLAG>
  class ExpressionMatcherOutLogger::ChannelImpInventoryLogger:
    public AdServer::LogProcessing::LogHolderSharded<
      AdServer::LogProcessing::ChannelImpInventoryTraits>,
    public IProcessRequest
  {
  public:
    typedef LogProcessing::ChannelImpInventoryCollector::DataT::KeyT KeyType;
    typedef typename KeyType::CCGType CCGType;

    typedef LogProcessing::ChannelImpInventoryInnerData::OneImpopUserAppearCounter
      OneImpopUserAppearCounter;
    typedef LogProcessing::ChannelImpInventoryInnerData::OneImpUserAppearCounter
      OneImpUserAppearCounter;
    typedef LogProcessing::ChannelImpInventoryInnerData::OneImpOtherUserAppearCounter
      OneImpOtherUserAppearCounter;
    typedef LogProcessing::ChannelImpInventoryInnerData::OneImpopNoImpUserAppearCounter
      OneImpopNoImpUserAppearCounter;
    typedef LogProcessing::ChannelImpInventoryInnerData::ImpCountRevenueCounter
      ImpCountRevenueCounter;
    typedef LogProcessing::ChannelImpInventoryInnerData::ImpOtherImpsAndRevenueCounter
      ImpOtherImpsAndRevenueCounter;
    typedef LogProcessing::ChannelImpInventoryInnerData::NoImpopsImpsAndRevenueCounter
      NoImpopsImpsAndRevenueCounter;

  public:
    ChannelImpInventoryLogger(
      unsigned long placement_colo_id,
      const RevenueDecimal& simplify_factor,
      const AdServer::LogProcessing::LogFlushTraits& flush_traits)
      /*throw(LoggerException)*/;

    virtual void
    process_request(const InventoryInfo& inv_info)
      /*throw(eh::Exception)*/;

  private:
    template<typename Mediator>
    class OneUserAppearCounter
    {
    public:
      OneUserAppearCounter(CCGType ccg_type, const Mediator& counter)
        : ccg_type_(ccg_type), counter_(counter)
      {}

      std::pair<KeyType, Mediator>
      operator() (unsigned long channel_id) const
      {
        return std::make_pair(KeyType(channel_id, ccg_type_), counter_);
      }

    private:
      const CCGType ccg_type_;
      const Mediator counter_;
    };

    class ImpChannelsCounter
    {
    public:
      ImpChannelsCounter(CCGType ccg_type, const RevenueDecimal& users_simpl_factor)
        : ccg_type_(ccg_type), users_simpl_factor_(users_simpl_factor)
      {}

      std::pair<KeyType, ImpCountRevenueCounter>
      operator() (const std::pair<unsigned long, RevenueDecimal>& val) const
      {
        return std::make_pair(
          KeyType(val.first, ccg_type_),
          SAMPLING_FLAG ?
            RevenueDecimal::mul(val.second, users_simpl_factor_, Generics::DMR_ROUND) :
            val.second);
      }

    private:
      const CCGType ccg_type_;
      const RevenueDecimal users_simpl_factor_;
    };

    template<typename Mediator>
    class ChannelImpCounter
    {
    public:
      ChannelImpCounter(CCGType ccg_type, const RevenueDecimal& users_simpl_factor)
        : ccg_type_(ccg_type), users_simpl_factor_(users_simpl_factor)
      {}

      std::pair<KeyType, Mediator>
      operator() (const std::pair<unsigned long, InventoryInfo::ChannelImpCounter>& val) const
      {
        return std::make_pair(
          KeyType(val.first, ccg_type_),
          Mediator(
            (SAMPLING_FLAG ?
              RevenueDecimal::mul(
                RevenueDecimal(false, val.second.imps, 0),
                  users_simpl_factor_,
                  Generics::DMR_ROUND)
              :
              RevenueDecimal(false, val.second.imps, 0)),
            (SAMPLING_FLAG ?
              RevenueDecimal::mul(val.second.revenue, users_simpl_factor_, Generics::DMR_ROUND)
              :
              val.second.revenue)));
      }

    private:
      const CCGType ccg_type_;
      const RevenueDecimal users_simpl_factor_;
    };

  private:
    const unsigned long placement_colo_id_;
    /// ccg_type = 'D'
    OneUserAppearCounter<OneImpopUserAppearCounter> DISPLAY_ONE_IMPOP_USER_APPEAR_COUNTER_;
    OneUserAppearCounter<OneImpUserAppearCounter> DISPLAY_ONE_IMP_USER_APPEAR_COUNTER_;
    OneUserAppearCounter<OneImpOtherUserAppearCounter> DISPLAY_ONE_IMP_OTHER_USER_APPEAR_COUNTER_;
    OneUserAppearCounter<OneImpopNoImpUserAppearCounter> DISPLAY_ONE_IMPOP_NO_IMP_USER_APPEAR_COUNTER_;
    ImpChannelsCounter DISPLAY_IMP_CHANNEL_COUNTER_;
    ChannelImpCounter<ImpOtherImpsAndRevenueCounter> DISPLAY_IMP_OTHER_IMP_AND_REVENUE_COUNTER_;
    ChannelImpCounter<NoImpopsImpsAndRevenueCounter> DISPLAY_NO_IMPOPS_IMP_AND_REVENUE_COUNTER_;

    /// ccg_type = 'T'
    OneUserAppearCounter<OneImpopUserAppearCounter> TEXT_ONE_IMPOP_USER_APPEAR_COUNTER_;
    OneUserAppearCounter<OneImpUserAppearCounter> TEXT_ONE_IMP_USER_APPEAR_COUNTER_;
    OneUserAppearCounter<OneImpOtherUserAppearCounter> TEXT_ONE_IMP_OTHER_USER_APPEAR_COUNTER_;
    OneUserAppearCounter<OneImpopNoImpUserAppearCounter> TEXT_ONE_IMPOP_NO_IMP_USER_APPEAR_COUNTER_;
    ImpChannelsCounter TEXT_IMP_CHANNEL_COUNTER_;
    ChannelImpCounter<ImpOtherImpsAndRevenueCounter> TEXT_IMP_OTHER_IMP_AND_REVENUE_COUNTER_;
    ChannelImpCounter<NoImpopsImpsAndRevenueCounter> TEXT_NO_IMPOPS_IMP_AND_REVENUE_COUNTER_;

  protected:
    virtual
    ~ChannelImpInventoryLogger() noexcept = default;

    void
    add_inv_typed_appear_record_(
      CollectorT& collector,
      const LogProcessing::ChannelImpInventoryCollector::KeyT& ch_inv_key,
      const InventoryInfo::ChannelImpAppearInfo& appears,
      const OneUserAppearCounter<OneImpopUserAppearCounter>& one_impop_user_appear_counter,
      const OneUserAppearCounter<OneImpUserAppearCounter>& one_imp_user_appear_counter,
      const OneUserAppearCounter<OneImpOtherUserAppearCounter>& one_imp_other_user_appear_counter,
      const OneUserAppearCounter<OneImpopNoImpUserAppearCounter>& one_impop_no_imp_user_appear_counter) const
      /*throw(eh::Exception)*/;

    void
    add_inv_typed_imps_record_(
      CollectorT& collector,
      const LogProcessing::ChannelImpInventoryCollector::KeyT& ch_inv_key,
      const InventoryInfo::ChannelImpInfo& imps,
      const ImpChannelsCounter& imp_channel_counter,
      const ChannelImpCounter<ImpOtherImpsAndRevenueCounter>& imp_other_imp_and_revenue_counter,
      const ChannelImpCounter<NoImpopsImpsAndRevenueCounter>& no_impops_imp_and_revenue_counter) const
      /*throw(eh::Exception)*/;
  };

  /**
   * ChannelActivityLogger
   */
  class ExpressionMatcherOutLogger::ChannelActivityLogger:
    public AdServer::LogProcessing::LogHolderSharded<
      AdServer::LogProcessing::ChannelInventoryTraits>
  {
  public:
    ChannelActivityLogger(const AdServer::LogProcessing::LogFlushTraits& flush_traits)
      /*throw(LoggerException)*/
      : AdServer::LogProcessing::LogHolderSharded<
          AdServer::LogProcessing::ChannelInventoryTraits>(flush_traits)
    {};

    void
    process_channel_activity(
      const Generics::Time& date,
      unsigned long colo_id,
      const ChannelIdSet& channels)
      /*throw(eh::Exception)*/
    {
      if (!channels.empty())
      {
        LogProcessing::ChannelInventoryCollector::KeyT ch_inv_key(date, colo_id);
        LogProcessing::ChannelInventoryCollector::DataT::DataT
          channel_props(RevenueDecimal::ZERO, RevenueDecimal::ZERO, RevenueDecimal::ZERO);

        LogProcessing::ChannelInventoryCollector::DataT ch_inv_data;

        for (ChannelIdSet::const_iterator ch_it = channels.begin(); ch_it != channels.end();
          ++ch_it)
        {
          ch_inv_data.add(
            LogProcessing::ChannelInventoryCollector::DataT::KeyT(*ch_it),
            channel_props);
        }

        add_record(ch_inv_key, ch_inv_data);
      }
    }

  protected:
    virtual
    ~ChannelActivityLogger() noexcept = default;
  };

  /**
   * ExpressionMatcherOutLogger::ChannelPriceRangeLogger
   */
  class ExpressionMatcherOutLogger::ChannelPriceRangeLogger:
    public AdServer::LogProcessing::LogHolderSharded<
      AdServer::LogProcessing::ChannelPriceRangeTraits>
  {
  public:
    typedef LogProcessing::ChannelPriceRangeCollector::DataT::KeyT::EcpmT EcpmT;
    typedef LogProcessing::ChannelPriceRangeCollector::DataT::KeyT InnerKey;
    typedef LogProcessing::ChannelPriceRangeCollector::DataT::DataT InnerData;

    class AppearChannelCounter
    {
    public:
      AppearChannelCounter(
        const Commons::ImmutableString& country_code,
        const EcpmT& ecpm,
        unsigned long colo_id,
        const InnerData& counter)
        : country_code_(country_code),
          ecpm_(ecpm),
          colo_id_(colo_id),
          counter_(counter)
      {}

      std::pair<InnerKey, InnerData>
      operator() (const SizeChannel& ch) const
      {
        return std::make_pair(
          InnerKey(ch.size, country_code_, ch.channel_id, ecpm_, colo_id_),
          counter_);
      }

    private:
      const Commons::ImmutableString& country_code_;
      const EcpmT& ecpm_;
      const unsigned long colo_id_;
      const InnerData& counter_;
    };

    class DisappearChannelCounter
    {
    public:
      DisappearChannelCounter(
        const Commons::ImmutableString& country_code,
        unsigned long colo_id,
        const InnerData& counter)
        : country_code_(country_code),
          colo_id_(colo_id),
          counter_(counter)
      {}

      std::pair<InnerKey, InnerData>
      operator() (const ChannelECPM& ch) const
      {
        return std::make_pair(
          InnerKey(
            ch.size,
            country_code_,
            ch.channel_id,
            EcpmT::div(EcpmT(false, ch.ecpm, 0), CPM_PRECISION, Generics::DDR_CEIL),
            colo_id_),
          counter_);
      }

    private:
      const Commons::ImmutableString& country_code_;
      const unsigned long colo_id_;
      const InnerData& counter_;
    };

    class ChannelCounter
    {
    public:
      ChannelCounter(
        const Commons::ImmutableString& creative_size,
        const Commons::ImmutableString& country_code,
        const EcpmT& ecpm,
        unsigned long colo_id,
        const InnerData& counter)
        : creative_size_(creative_size),
          country_code_(country_code),
          ecpm_(ecpm),
          colo_id_(colo_id),
          counter_(counter)
      {}

      std::pair<InnerKey, InnerData>
      operator() (unsigned long channel_id) const
      {
        return std::make_pair(
          InnerKey(creative_size_, country_code_, channel_id, ecpm_, colo_id_),
          counter_);
      }

    private:
      const Commons::ImmutableString& creative_size_;
      const Commons::ImmutableString& country_code_;
      const EcpmT& ecpm_;
      const unsigned long colo_id_;
      const InnerData& counter_;
    };

  public:
    ChannelPriceRangeLogger(
      const RevenueDecimal& simplify_factor,
      const AdServer::LogProcessing::LogFlushTraits& flush_traits,
      unsigned long placement_colo_id)
      /*throw(LoggerException)*/
      : AdServer::LogProcessing::LogHolderSharded<
          AdServer::LogProcessing::ChannelPriceRangeTraits>(flush_traits),
        users_simpl_factor_(simplify_factor),
        double_users_simpl_factor_(static_cast<double>(
          RevenueDecimal::mul(
            users_simpl_factor_,
            RevenueDecimal(false, 100, 0),
            Generics::DMR_ROUND).ceil(0).
          integer<unsigned long>()) / 100),
        ulong_users_simpl_factor_(users_simpl_factor_.integer<unsigned long>()),
        placement_colo_id_(placement_colo_id)
    {}

    virtual void
    process_request(const InventoryActionProcessor::InventoryInfo& inv_info)
      /*throw(eh::Exception)*/
    {
      if (inv_info.sizes.empty())
      {
        return;
      }

      const EcpmT impop_ecpm = EcpmT::div(
        EcpmT(false, inv_info.impop_ecpm, 0),
        CPM_PRECISION,
        Generics::DDR_CEIL);

      if (!inv_info.appear_channel_ecpms.empty() || !inv_info.disappear_channel_ecpms.empty())
      {
        const AppearChannelCounter appear_channel_counter(
          inv_info.country_code,
          impop_ecpm,
          placement_colo_id_,
          InnerData(double_users_simpl_factor_, 0));

        const DisappearChannelCounter disappear_channel_counter(
          inv_info.country_code,
          placement_colo_id_,
          InnerData(-double_users_simpl_factor_, 0));

        add_records(
          [&](CollectorT& collector)
          {
            collector.add(
              inv_info.placement_colo_time,
              make_transform_range(inv_info.appear_channel_ecpms, appear_channel_counter));

            collector.add(
              inv_info.placement_colo_time,
              make_transform_range(inv_info.disappear_channel_ecpms, disappear_channel_counter));
          });
      }

      if (!inv_info.impop_channels.empty())
      {
        const LogProcessing::ChannelPriceRangeCollector::DataT::DataT impop_inner_data(
          0,
          ulong_users_simpl_factor_); // FIXME: double_users_simpl_factor_ should be used

        add_records(
          [&](CollectorT& collector)
          {
            for (auto size_it = inv_info.sizes.begin(); size_it != inv_info.sizes.end(); ++size_it)
            {
              const ChannelCounter channel_counter(
                *size_it,
                inv_info.country_code,
                impop_ecpm,
                inv_info.colo_id,
                impop_inner_data);

              collector.add(
                inv_info.isp_time,
                make_transform_range(inv_info.impop_channels, channel_counter));
            }
          });
      }
    }

  protected:
    virtual
    ~ChannelPriceRangeLogger() noexcept = default;

  private:
    static const EcpmT CPM_PRECISION;
    const RevenueDecimal users_simpl_factor_;
    const double double_users_simpl_factor_;
    const long ulong_users_simpl_factor_;
    const unsigned long placement_colo_id_;
  };

  const ExpressionMatcherOutLogger::ChannelPriceRangeLogger::EcpmT
    ExpressionMatcherOutLogger::ChannelPriceRangeLogger::CPM_PRECISION =
    ExpressionMatcherOutLogger::ChannelPriceRangeLogger::EcpmT(false, ChannelECPM::CPM_PRECISION, 0);

  /**
   * ExpressionMatcherOutLogger::ChannelPerformanceLogger
   */
  class ExpressionMatcherOutLogger::ChannelPerformanceLogger:
    public virtual MatchRequestProcessor,
    public AdServer::LogProcessing::LogHolderSharded<
      AdServer::LogProcessing::ChannelPerformanceTraits>
  {
  private:
    typedef LogProcessing::ChannelPerformanceInnerData::StatRequestOne StatRequestOne;

    class RequestCounter
    {
    public:
      RequestCounter(const StatRequestOne& counter, const Commons::ImmutableString& tag_size)
        : counter_(counter), tag_size_(tag_size)
      {}

      std::pair<CollectorT::DataT::KeyT, StatRequestOne>
      operator() (unsigned long ch) const
      {
        return std::make_pair(
          CollectorT::DataT::KeyT(ch, 0 /* ccg_id */, tag_size_),
          counter_);
      }

    private:
      const StatRequestOne& counter_;
      const Commons::ImmutableString& tag_size_;
    };

  public:
    ChannelPerformanceLogger(
      long users_simpl_factor,
      const AdServer::LogProcessing::LogFlushTraits& flush_traits)
      /*throw(LoggerException)*/
      : AdServer::LogProcessing::LogHolderSharded<
          AdServer::LogProcessing::ChannelPerformanceTraits>(flush_traits),
        STAT_REQUEST_ONE_(users_simpl_factor)
    {}

    virtual void
    process_match_request(const MatchInfo& match_info)
      /*throw(MatchRequestProcessor::Exception)*/
    {
      if (!match_info.tag_size.empty() &&
        match_info.triggered_expression_channels.present() &&
        !match_info.triggered_expression_channels->empty())
      {
        add_record(
          CollectorT::KeyT(match_info.isp_time, match_info.colo_id),
          make_transform_range(
            *match_info.triggered_expression_channels,
            RequestCounter(STAT_REQUEST_ONE_, match_info.tag_size)));
      }
    }

  protected:
    virtual
    ~ChannelPerformanceLogger() noexcept = default;

  private:
    const StatRequestOne STAT_REQUEST_ONE_;
  };

  /**
   * ExpressionMatcherOutLogger::ChannelTriggerStatLogger
   */
  class ExpressionMatcherOutLogger::ChannelTriggerStatLogger:
    public AdServer::LogProcessing::LogHolderSharded<
      AdServer::LogProcessing::ChannelTriggerStatTraits>
  {
  public:
    ChannelTriggerStatLogger(const AdServer::LogProcessing::LogFlushTraits& flush_traits)
      /*throw(LoggerException)*/
      : AdServer::LogProcessing::LogHolderSharded<
          AdServer::LogProcessing::ChannelTriggerStatTraits>(flush_traits)
    {}

    void
    process_match_request(
      const Generics::Time& isp_time,
      unsigned long colo_id,
      const AdServer::LogProcessing::RequestBasicChannelsInnerData::Match& match_request)
      /*throw(eh::Exception)*/
    {
      const auto& page_triggers = match_request.page_trigger_channels();
      const auto& search_triggers = match_request.search_trigger_channels();
      const auto& url_triggers = match_request.url_trigger_channels();
      const auto& url_keyword_triggers = match_request.url_keyword_trigger_channels();

      const std::size_t triggers_count =
        page_triggers.size() +
        search_triggers.size() +
        url_triggers.size() +
        url_keyword_triggers.size();

      if (!triggers_count)
      {
        return;
      }

      const CollectorT::KeyT key(isp_time, colo_id);

      add_records(
        [&](CollectorT& collector)
        {
          add_hits_(collector, key, 'U', url_triggers);
          add_hits_(collector, key, 'P', page_triggers);
          add_hits_(collector, key, 'S', search_triggers);
          add_hits_(collector, key, 'R', url_keyword_triggers);
        });
    }

  protected:
    ~ChannelTriggerStatLogger() noexcept override = default;

  private:
    class HitCounter
    {
    public:
      explicit HitCounter(char type) noexcept
        : type_(type)
      {}

      std::pair<CollectorT::DataT::KeyT, CollectorT::DataT::DataT>
      operator()(
        const AdServer::LogProcessing::RequestBasicChannelsInnerData::TriggerMatch& trigger) const
      {
        return std::make_pair(
          CollectorT::DataT::KeyT(trigger.channel_trigger_id, trigger.channel_id, type_),
          CollectorT::DataT::DataT(1));
      }

    private:
      const char type_;
    };

    static void
    add_hits_(
      CollectorT& collector,
      const CollectorT::KeyT& key,
      char type,
      const AdServer::LogProcessing::RequestBasicChannelsInnerData::TriggerMatchArray& triggers)
      /*throw(eh::Exception)*/
    {
      if (!triggers.empty())
      {
        collector.add(key, make_transform_range(triggers, HitCounter(type)));
      }
    }
  };

  /**
   * ExpressionMatcherOutLogger::ChannelHitStatLogger
   */
  class ExpressionMatcherOutLogger::ChannelHitStatLogger:
    public AdServer::LogProcessing::LogHolderSharded<
      AdServer::LogProcessing::ChannelHitStatTraits>
  {
  public:
    ChannelHitStatLogger(const AdServer::LogProcessing::LogFlushTraits& flush_traits)
      /*throw(LoggerException)*/
      : AdServer::LogProcessing::LogHolderSharded<
          AdServer::LogProcessing::ChannelHitStatTraits>(flush_traits)
    {}

    void
    process_match_request(
      const Generics::Time& isp_time,
      unsigned long colo_id,
      const AdServer::LogProcessing::RequestBasicChannelsInnerData::Match& match_request)
      /*throw(eh::Exception)*/
    {
      const auto& page_triggers = match_request.page_trigger_channels();
      const auto& search_triggers = match_request.search_trigger_channels();
      const auto& url_triggers = match_request.url_trigger_channels();
      const auto& url_keyword_triggers = match_request.url_keyword_trigger_channels();

      const std::size_t triggers_count =
        page_triggers.size() +
        search_triggers.size() +
        url_triggers.size() +
        url_keyword_triggers.size();

      if (!triggers_count)
      {
        return;
      }

      Generics::MonoAllocatorArena arena;
      ChannelMaskMap channel_masks(&arena);
      channel_masks.reserve(triggers_count);

      add_channel_masks_(channel_masks, url_triggers, URL_MASK);
      add_channel_masks_(channel_masks, page_triggers, PAGE_MASK);
      add_channel_masks_(channel_masks, search_triggers, SEARCH_MASK);
      add_channel_masks_(channel_masks, url_keyword_triggers, URL_KEYWORD_MASK);

      add_record(
        CollectorT::KeyT(isp_time, colo_id),
        make_transform_range(channel_masks, ChannelCounter()));
    }

  protected:
    ~ChannelHitStatLogger() noexcept override = default;

  private:
    enum ChannelMask : std::uint8_t
    {
      URL_MASK = 1,
      PAGE_MASK = 2,
      SEARCH_MASK = 4,
      URL_KEYWORD_MASK = 8
    };

    using ChannelMaskMap = Generics::MonoUnorderedMap<std::uint32_t, std::uint8_t>;

    class ChannelCounter
    {
    public:
      std::pair<CollectorT::DataT::KeyT, CollectorT::DataT::DataT>
      operator()(const ChannelMaskMap::value_type& channel_mask) const
      {
        const auto& [channel_id, mask] = channel_mask;
        return std::make_pair(
          CollectorT::DataT::KeyT(channel_id),
          CollectorT::DataT::DataT(
            1,
            mask & URL_MASK ? 1 : 0,
            mask & PAGE_MASK ? 1 : 0,
            mask & SEARCH_MASK ? 1 : 0,
            mask & URL_KEYWORD_MASK ? 1 : 0));
      }
    };

    static void
    add_channel_masks_(
      ChannelMaskMap& channel_masks,
      const AdServer::LogProcessing::RequestBasicChannelsInnerData::TriggerMatchArray& triggers,
      std::uint8_t mask)
    {
      for (const auto& trigger : triggers)
      {
        auto [it, inserted] = channel_masks.try_emplace(trigger.channel_id, 0);
        static_cast<void>(inserted);
        it->second |= mask;
      }
    }
  };

  /**
   * ExpressionMatcherOutLogger::ChannelTriggerImpLogger
   */
  class ExpressionMatcherOutLogger::ChannelTriggerImpLogger:
    public virtual TriggerActionProcessor,
    public virtual AdServer::LogProcessing::LogHolderSharded<
      AdServer::LogProcessing::ChannelTriggerImpStatTraits>
  {
  public:
    ChannelTriggerImpLogger(
      const AdServer::LogProcessing::LogFlushTraits& flush_traits,
      unsigned long colo_id)
      /*throw(LoggerException)*/
      : AdServer::LogProcessing::LogHolderSharded<
          AdServer::LogProcessing::ChannelTriggerImpStatTraits>(flush_traits),
        colo_id_(colo_id)
    {}

    virtual void
    process_triggers_impression(const TriggersMatchInfo& match_info)
      /*throw(TriggerActionProcessor::Exception)*/
    {
      Generics::Time date = match_info.time.get_gm_time().get_date();
      add_imp_records_(date, 'P', match_info.page_matches);
      add_imp_records_(date, 'S', match_info.search_matches);
      add_imp_records_(date, 'U', match_info.url_matches);
      add_imp_records_(date, 'R', match_info.url_keyword_matches);
    }

    virtual void
    process_triggers_click(const TriggersMatchInfo& match_info)
      /*throw(TriggerActionProcessor::Exception)*/
    {
      Generics::Time date = match_info.time.get_gm_time().get_date();
      add_click_records_(date, 'P', match_info.page_matches);
      add_click_records_(date, 'S', match_info.search_matches);
      add_click_records_(date, 'U', match_info.url_matches);
      add_click_records_(date, 'R', match_info.url_keyword_matches);
    }

  protected:
    virtual
    ~ChannelTriggerImpLogger() noexcept = default;

    void
    add_imp_records_(const Generics::Time& time, char type, const MatchCountMap& matches)
    {
      if (!matches.empty())
      {
        CollectorT::KeyT key(time, colo_id_);
        CollectorT::DataT add_data;

        for (MatchCountMap::const_iterator mit = matches.begin(); mit != matches.end(); ++mit)
        {
          add_data.add(
            CollectorT::DataT::KeyT(mit->first.channel_trigger_id, mit->first.channel_id, type),
            CollectorT::DataT::DataT(mit->second, RevenueDecimal::ZERO));
        }

        add_record(key, add_data);
      }
    }

    void add_click_records_(const Generics::Time& time, char type, const MatchCountMap& matches)
    {
      if (!matches.empty())
      {
        CollectorT::KeyT key(time, colo_id_);
        CollectorT::DataT add_data;

        for (MatchCountMap::const_iterator mit = matches.begin(); mit != matches.end(); ++mit)
        {
          add_data.add(
            CollectorT::DataT::KeyT(mit->first.channel_trigger_id, mit->first.channel_id, type),
            CollectorT::DataT::DataT(RevenueDecimal::ZERO, mit->second));
        }

        add_record(key, add_data);
      }
    }

  private:
    const unsigned long colo_id_;
  };

  /**
   * ExpressionMatcherOutLogger
   */
  ExpressionMatcherOutLogger::ExpressionMatcherOutLogger(
    Logging::Logger* logger,
    unsigned long colo_id,
    const RevenueDecimal& simplify_factor,
    const AdServer::LogProcessing::LogFlushTraits& channel_inventory_flush,
    const AdServer::LogProcessing::LogFlushTraits& channel_imp_inventory_flush,
    const AdServer::LogProcessing::LogFlushTraits& channel_price_range_flush,
    const AdServer::LogProcessing::LogFlushTraits& channel_activity_flush,
    const AdServer::LogProcessing::LogFlushTraits& channel_performance_flush,
    const AdServer::LogProcessing::LogFlushTraits& channel_hit_stat_flush,
    const AdServer::LogProcessing::LogFlushTraits& channel_trigger_stat_flush,
    const AdServer::LogProcessing::LogFlushTraits& channel_trigger_imp_flush,
    const AdServer::LogProcessing::LogFlushTraits& global_colo_user_stat_flush,
    const AdServer::LogProcessing::LogFlushTraits& colo_user_stat_flush)
    /*throw(Exception)*/
    : logger_(ReferenceCounting::add_ref(logger)), simplify_factor_(simplify_factor)
  {
    const RevenueDecimal NO_SAMPLIN(false, 1, 0);

    channel_inventory_logger_ = new ChannelInventoryLogger(
      colo_id,
      simplify_factor,
      channel_inventory_flush);
    add_child_log_holder(channel_inventory_logger_);

    if (simplify_factor_ == NO_SAMPLIN)
    {
      ReferenceCounting::SmartPtr<ChannelImpInventoryLogger<false> > logger =
        new ChannelImpInventoryLogger<false>(colo_id, simplify_factor, channel_imp_inventory_flush);
      channel_imp_inventory_logger_ = logger;
      add_child_log_holder(logger);
    }
    else
    {
      ReferenceCounting::SmartPtr<ChannelImpInventoryLogger<true> > logger =
        new ChannelImpInventoryLogger<true>(colo_id, simplify_factor, channel_imp_inventory_flush);
      channel_imp_inventory_logger_ = logger;
      add_child_log_holder(logger);
    }

    channel_price_range_logger_ = new ChannelPriceRangeLogger(
      simplify_factor,
      channel_price_range_flush,
      colo_id);
    add_child_log_holder(channel_price_range_logger_);

    channel_activity_logger_ = new ChannelActivityLogger(channel_activity_flush);
    add_child_log_holder(channel_activity_logger_);

    channel_performance_logger_ = new ChannelPerformanceLogger(
      simplify_factor.integer<unsigned long>(), // FIXME: change to double
      channel_performance_flush);
    add_child_log_holder(channel_performance_logger_);

    channel_hit_stat_logger_ = new ChannelHitStatLogger(channel_hit_stat_flush);
    add_child_log_holder(channel_hit_stat_logger_);

    channel_trigger_stat_logger_ = new ChannelTriggerStatLogger(channel_trigger_stat_flush);
    add_child_log_holder(channel_trigger_stat_logger_);

    channel_trigger_imp_logger_ = new ChannelTriggerImpLogger(channel_trigger_imp_flush, colo_id);
    add_child_log_holder(channel_trigger_imp_logger_);

    global_colo_user_stat_logger_ = new GlobalColoUserStatLogger(global_colo_user_stat_flush);
    add_child_log_holder(global_colo_user_stat_logger_);

    colo_user_stat_logger_ = new ColoUserStatLogger(colo_user_stat_flush);
    add_child_log_holder(colo_user_stat_logger_);
  }

  ExpressionMatcherOutLogger::~ExpressionMatcherOutLogger() noexcept
  {}

  void
  ExpressionMatcherOutLogger::process_match_request(const MatchInfo& request_info)
    /*throw(MatchRequestProcessor::Exception)*/
  {
    channel_performance_logger_->process_match_request(request_info);
  }

  void
  ExpressionMatcherOutLogger::process_channel_hit_stat(
    const Generics::Time& isp_time,
    unsigned long colo_id,
    const AdServer::LogProcessing::RequestBasicChannelsInnerData::Match& match_request)
    /*throw(Exception)*/
  {
    channel_hit_stat_logger_->process_match_request(isp_time, colo_id, match_request);
  }

  void
  ExpressionMatcherOutLogger::process_channel_trigger_stat(
    const Generics::Time& isp_time,
    unsigned long colo_id,
    const AdServer::LogProcessing::RequestBasicChannelsInnerData::Match& match_request)
    /*throw(Exception)*/
  {
    channel_trigger_stat_logger_->process_match_request(isp_time, colo_id, match_request);
  }

  void
  ExpressionMatcherOutLogger::process_request(
    const InventoryActionProcessor::InventoryInfo& inv_info)
    /*throw(InventoryActionProcessor::Exception)*/
  {
    static const char* FUN = "ExpressionMatcherOutLogger::process_request()";

    try
    {
      if (simplify_factor_ != RevenueDecimal::ZERO)
      {
        channel_inventory_logger_->process_request(inv_info);
        channel_imp_inventory_logger_->process_request(inv_info);
        channel_price_range_logger_->process_request(inv_info);
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw InventoryActionProcessor::Exception(ostr);
    }
  }

  void
  ExpressionMatcherOutLogger::process_user(
    const InventoryActionProcessor::InventoryUserInfo& inv_info)
    /*throw(InventoryActionProcessor::Exception)*/
  {
    static const char* FUN = "ExpressionMatcherOutLogger::process_request()";

    try
    {
      if (simplify_factor_ != RevenueDecimal::ZERO)
      {
        channel_inventory_logger_->process_user(inv_info);
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw InventoryActionProcessor::Exception(ostr);
    }
  }

  void
  ExpressionMatcherOutLogger::process_channel_activity(
    const Generics::Time& date,
    unsigned long colo_id,
    const ChannelIdSet& channels)
    /*throw(InventoryActionProcessor::Exception)*/
  {
    channel_activity_logger_->process_channel_activity(date, colo_id, channels);
  }

  void
  ExpressionMatcherOutLogger::process_triggers_impression(const TriggersMatchInfo& match_info)
    /*throw(TriggerActionProcessor::Exception)*/
  {
    channel_trigger_imp_logger_->process_triggers_impression(match_info);
  }

  void
  ExpressionMatcherOutLogger::process_triggers_click(const TriggersMatchInfo& match_info)
    /*throw(TriggerActionProcessor::Exception)*/
  {
    channel_trigger_imp_logger_->process_triggers_click(match_info);
  }

  void
  ExpressionMatcherOutLogger::process_gmt_colo_reach(const ColoReachInfo& reach_info)
    /*throw(ColoReachProcessor::Exception)*/
  {
    global_colo_user_stat_logger_->process_gmt_colo_reach(reach_info);
  }

  void
  ExpressionMatcherOutLogger::process_isp_colo_reach(const ColoReachInfo& reach_info)
    /*throw(ColoReachProcessor::Exception)*/
  {
    colo_user_stat_logger_->process_isp_colo_reach(reach_info);
  }
}

namespace AdServer::RequestInfoSvcs
{
  /*
   * ExpressionMatcherOutLogger::ChannelImpInventoryLogger
   */
  template<bool SAMPLING_FLAG>
  ExpressionMatcherOutLogger::ChannelImpInventoryLogger<SAMPLING_FLAG>::ChannelImpInventoryLogger(
    unsigned long placement_colo_id,
    const RevenueDecimal& simplify_factor,
    const AdServer::LogProcessing::LogFlushTraits& flush_traits)
    /*throw(LoggerException)*/
    : AdServer::LogProcessing::LogHolderSharded<
        AdServer::LogProcessing::ChannelImpInventoryTraits>(flush_traits),
      placement_colo_id_(placement_colo_id),
      DISPLAY_ONE_IMPOP_USER_APPEAR_COUNTER_(CCGType::DISPLAY, simplify_factor),
      DISPLAY_ONE_IMP_USER_APPEAR_COUNTER_(CCGType::DISPLAY, simplify_factor),
      DISPLAY_ONE_IMP_OTHER_USER_APPEAR_COUNTER_(CCGType::DISPLAY, simplify_factor),
      DISPLAY_ONE_IMPOP_NO_IMP_USER_APPEAR_COUNTER_(CCGType::DISPLAY, simplify_factor),
      DISPLAY_IMP_CHANNEL_COUNTER_(CCGType::DISPLAY, simplify_factor),
      DISPLAY_IMP_OTHER_IMP_AND_REVENUE_COUNTER_(CCGType::DISPLAY, simplify_factor),
      DISPLAY_NO_IMPOPS_IMP_AND_REVENUE_COUNTER_(CCGType::DISPLAY, simplify_factor),
      TEXT_ONE_IMPOP_USER_APPEAR_COUNTER_(CCGType::TEXT, simplify_factor),
      TEXT_ONE_IMP_USER_APPEAR_COUNTER_(CCGType::TEXT, simplify_factor),
      TEXT_ONE_IMP_OTHER_USER_APPEAR_COUNTER_(CCGType::TEXT, simplify_factor),
      TEXT_ONE_IMPOP_NO_IMP_USER_APPEAR_COUNTER_(CCGType::TEXT, simplify_factor),
      TEXT_IMP_CHANNEL_COUNTER_(CCGType::TEXT, simplify_factor),
      TEXT_IMP_OTHER_IMP_AND_REVENUE_COUNTER_(CCGType::TEXT, simplify_factor),
      TEXT_NO_IMPOPS_IMP_AND_REVENUE_COUNTER_(CCGType::TEXT, simplify_factor)
  {}

  template<bool SAMPLING_FLAG>
  void
  ExpressionMatcherOutLogger::ChannelImpInventoryLogger<SAMPLING_FLAG>::process_request(
    const InventoryInfo& inv_info)
    /*throw(eh::Exception)*/
  {
    add_records(
      [&](CollectorT& collector)
      {
        if (!inv_info.display_appears.empty() || !inv_info.text_appears.empty())
        {
          // log user counters for placement colo id (colocation configuration)
          const LogProcessing::ChannelImpInventoryCollector::KeyT ch_inv_key(
            inv_info.placement_colo_time, placement_colo_id_);

          add_inv_typed_appear_record_(
            collector,
            ch_inv_key,
            inv_info.display_appears,
            DISPLAY_ONE_IMPOP_USER_APPEAR_COUNTER_,
            DISPLAY_ONE_IMP_USER_APPEAR_COUNTER_,
            DISPLAY_ONE_IMP_OTHER_USER_APPEAR_COUNTER_,
            DISPLAY_ONE_IMPOP_NO_IMP_USER_APPEAR_COUNTER_);

          add_inv_typed_appear_record_(
            collector,
            ch_inv_key,
            inv_info.text_appears,
            TEXT_ONE_IMPOP_USER_APPEAR_COUNTER_,
            TEXT_ONE_IMP_USER_APPEAR_COUNTER_,
            TEXT_ONE_IMP_OTHER_USER_APPEAR_COUNTER_,
            TEXT_ONE_IMPOP_NO_IMP_USER_APPEAR_COUNTER_);
        }

        if (!inv_info.display_imps.empty() || !inv_info.text_imps.empty())
        {
          // impressions for request defined colo id
          const LogProcessing::ChannelImpInventoryCollector::KeyT ch_inv_key(
            inv_info.isp_time, inv_info.colo_id);

          add_inv_typed_imps_record_(
            collector,
            ch_inv_key,
            inv_info.display_imps,
            DISPLAY_IMP_CHANNEL_COUNTER_,
            DISPLAY_IMP_OTHER_IMP_AND_REVENUE_COUNTER_,
            DISPLAY_NO_IMPOPS_IMP_AND_REVENUE_COUNTER_);

          add_inv_typed_imps_record_(
            collector,
            ch_inv_key,
            inv_info.text_imps,
            TEXT_IMP_CHANNEL_COUNTER_,
            TEXT_IMP_OTHER_IMP_AND_REVENUE_COUNTER_,
            TEXT_NO_IMPOPS_IMP_AND_REVENUE_COUNTER_);
        }
      });
  }

  template<bool SAMPLING_FLAG>
  void
  ExpressionMatcherOutLogger::ChannelImpInventoryLogger<SAMPLING_FLAG>::add_inv_typed_appear_record_(
    CollectorT& collector,
    const LogProcessing::ChannelImpInventoryCollector::KeyT& ch_inv_key,
    const InventoryInfo::ChannelImpAppearInfo& appears,
    const OneUserAppearCounter<OneImpopUserAppearCounter>& one_impop_user_appear_counter,
    const OneUserAppearCounter<OneImpUserAppearCounter>& one_imp_user_appear_counter,
    const OneUserAppearCounter<OneImpOtherUserAppearCounter>& one_imp_other_user_appear_counter,
    const OneUserAppearCounter<OneImpopNoImpUserAppearCounter>& one_impop_no_imp_user_appear_counter) const
    /*throw(eh::Exception)*/
  {
    collector.add(
      ch_inv_key,
      make_transform_range(appears.impop_appear_channels, one_impop_user_appear_counter));

    collector.add(
      ch_inv_key,
      make_transform_range(appears.imp_appear_channels, one_imp_user_appear_counter));

    collector.add(
      ch_inv_key,
      make_transform_range(appears.imp_other_appear_channels, one_imp_other_user_appear_counter));

    collector.add(
      ch_inv_key,
      make_transform_range(
        appears.impop_no_imp_appear_channels,
        one_impop_no_imp_user_appear_counter));
  }

  template<bool SAMPLING_FLAG>
  void
  ExpressionMatcherOutLogger::ChannelImpInventoryLogger<SAMPLING_FLAG>::add_inv_typed_imps_record_(
    CollectorT& collector,
    const LogProcessing::ChannelImpInventoryCollector::KeyT& ch_inv_key,
    const InventoryInfo::ChannelImpInfo& imps,
    const ImpChannelsCounter& imp_channel_counter,
    const ChannelImpCounter<ImpOtherImpsAndRevenueCounter>& imp_other_imp_and_revenue_counter,
    const ChannelImpCounter<NoImpopsImpsAndRevenueCounter>& no_impops_imp_and_revenue_counter) const
    /*throw(eh::Exception)*/
  {
    collector.add(
      ch_inv_key,
      make_transform_range(imps.imp_channels, imp_channel_counter));

    collector.add(
      ch_inv_key,
      make_transform_range(imps.imp_other_channels, imp_other_imp_and_revenue_counter));

    collector.add(
      ch_inv_key,
      make_transform_range(imps.impop_no_imp_channels, no_impops_imp_and_revenue_counter));
  }
}
