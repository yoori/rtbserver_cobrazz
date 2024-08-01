#ifndef EXPRESSIONMATCHER_INVENTORYACTIONPROCESSOR_HPP
#define EXPRESSIONMATCHER_INVENTORYACTIONPROCESSOR_HPP

#include <list>
#include <map>
#include <set>
#include <string>
#include <optional>

#include <ReferenceCounting/ReferenceCounting.hpp>
#include <eh/Exception.hpp>
#include <Generics/Time.hpp>
#include <Generics/SimpleDecimal.hpp>

#include <Commons/Algs.hpp>
#include <Commons/Containers.hpp>
#include <Commons/StringHolder.hpp>
#include <Commons/UserInfoManip.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>

namespace AdServer
{
  namespace RequestInfoSvcs
  {
    typedef Generics::SimpleDecimal<uint64_t, 18, 8> RevenueDecimal;

    typedef std::list<unsigned long> ChannelIdList;
    typedef std::vector<unsigned long> ChannelIdVector;
    typedef std::set<unsigned long> ChannelIdSet;
    typedef std::set<Commons::ImmutableString> StringSet;
    typedef std::map<unsigned long, unsigned long> ChannelActionMap;

    struct SizeChannel
    {
      SizeChannel(
        const Commons::ImmutableString& size_val,
        unsigned long channel_id_val)
        : size(size_val),
          channel_id(channel_id_val)
      {}

      bool operator==(const SizeChannel& right) const;

      Commons::ImmutableString size;
      unsigned long channel_id;
    };

    typedef std::vector<SizeChannel> SizeChannelList;

    struct ChannelECPM: public SizeChannel
    {
      static const unsigned long CPM_PRECISION = 100;

      ChannelECPM(
        const Commons::ImmutableString& size_val,
        unsigned long channel_id_val,
        unsigned long ecpm_val)
        : SizeChannel(size_val, channel_id_val),
          ecpm(ecpm_val)
      {}

      bool operator==(const ChannelECPM& right) const;

      unsigned long ecpm;
    };

    typedef std::vector<ChannelECPM> ChannelECPMList;

    struct ChannelLevel
    {
      ChannelLevel(unsigned long channel_id_val, unsigned long level_val)
        : channel_id(channel_id_val),
          level(level_val)
      {}

      bool operator==(const ChannelLevel& right) const
      {
        return channel_id == right.channel_id &&
          level == right.level;
      }

      unsigned long channel_id;
      unsigned long level;
    };

    typedef std::list<ChannelLevel> ChannelLevelList;

    /** MatchRequestProcessor */
    class MatchRequestProcessor:
      public virtual ReferenceCounting::Interface
    {
    public:
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

      struct MatchInfo
      {
        struct AdSlot
        {
          RevenueDecimal avg_revenue; // ecpm normalized to one action cost
          ChannelIdSet imp_channels;
        };

        struct AdBidSlot: public AdSlot
        {
          RevenueDecimal max_avg_revenue;
        };

        typedef std::list<AdBidSlot> AdBidSlotList;

        MatchInfo() noexcept
          : colo_id(0),
            merge_request(false),
            max_text_ads(0),
            auction_type(CampaignSvcs::AT_MAX_ECPM),
            ad_request(false)
        {}

        AdServer::Commons::UserId user_id;
        Generics::Time time;
        Generics::Time isp_time;
        Generics::Time placement_colo_time;
        unsigned long colo_id;

        typedef std::optional<ChannelIdVector> ChannelIdVectorOptional;
        ChannelIdVectorOptional triggered_expression_channels; // sorted
        ChannelIdSet triggered_cpm_expression_channels;
        ChannelActionMap channel_actions;

        bool merge_request;

        StringSet sizes;
        Commons::ImmutableString tag_size; // selected size, empty if no ad
        Commons::ImmutableString country_code;
        RevenueDecimal cost_threshold;
        unsigned long max_text_ads;
        std::optional<AdSlot> display_ad;
        AdBidSlotList text_ads;
        CampaignSvcs::AuctionType auction_type;
        bool ad_request;

        template<typename OStream>
        void print(OStream& ostr, const char*) const noexcept;
      };

    public:
      virtual void process_match_request(
        const MatchInfo& request_info)
        /*throw(Exception)*/ = 0;

    protected:
      virtual
      ~MatchRequestProcessor() noexcept = default;
    };

    /** InventoryActionProcessor */
    class InventoryActionProcessor:
      public virtual ReferenceCounting::Interface
    {
    public:
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

      struct InventoryUserInfo
      {
        InventoryUserInfo() noexcept
          : colo_id(0)
        {}

        Generics::Time time;
        Generics::Time isp_time;
        Generics::Time placement_colo_time;
        unsigned long colo_id;
        ChannelIdVector total_appear_channels; // => +total_user_count
      };

      class InventoryInfo : public InventoryUserInfo
      {
      public:
        InventoryInfo() noexcept
          : impop_ecpm(0),
            auction_type(CampaignSvcs::AT_MAX_ECPM)
        {}

        InventoryInfo(const InventoryUserInfo& inv_user_info) noexcept
          : InventoryUserInfo(inv_user_info),
            impop_ecpm(0),
            auction_type(CampaignSvcs::AT_MAX_ECPM)
        {}

        struct ChannelImpCounter
        {
          ChannelImpCounter()
            : imps(0), revenue(RevenueDecimal::ZERO)
          {}

          ChannelImpCounter(unsigned long imps_val, const RevenueDecimal& revenue_val)
            : imps(imps_val), revenue(revenue_val)
          {}

          ChannelImpCounter&
          operator+=(const ChannelImpCounter& right)
          {
            imps += right.imps;
            revenue += right.revenue;
            return *this;
          }

          bool operator==(const ChannelImpCounter& right) const
          {
            return imps == right.imps && revenue == right.revenue;
          }

          unsigned long imps;
          RevenueDecimal revenue;
        };

        struct ChannelImpAppearInfo
        {
          ChannelIdVector impop_appear_channels; // => +impop_user_count
          ChannelIdVector imp_appear_channels; // => +imps.user_count
          ChannelIdVector imp_other_appear_channels; // => +imps_other.user_count
          ChannelIdVector impop_no_imp_appear_channels; // => +impop_no_imp.user_count

          bool operator==(const ChannelImpAppearInfo& right) const noexcept;

          bool empty() const noexcept;
        };

        typedef std::map<unsigned long, ChannelImpCounter> ChannelImpCounterMap;
        typedef std::vector<std::pair<unsigned long, ChannelImpCounter> > ChannelImpCounterVector;
        typedef std::vector<std::pair<unsigned long, RevenueDecimal> > ChannelAvgRevenueVector;

        struct ChannelImpInfo
        {
          ChannelAvgRevenueVector imp_channels; // => +imps.imps_value
          ChannelImpCounterVector imp_other_channels; // => +imps_other.imp
          ChannelImpCounterVector impop_no_imp_channels; // => +impop_no_imp.imp

          bool operator==(const ChannelImpInfo& right) const noexcept;

          bool empty() const noexcept;
        };

      public:
        /* channel appears (user counters) */
        /* this field isn't empty only at running of daily processing task */
        ChannelIdVector active_appear_channels; // => +active_user_count
        ChannelImpAppearInfo display_appears;
        ChannelImpAppearInfo text_appears;

        /* channel imps */
        ChannelImpInfo display_imps;
        ChannelImpInfo text_imps;

        /* channel by ecpm */
        Commons::ImmutableString country_code;
        unsigned long impop_ecpm;
        ChannelIdVector impop_channels;
        StringSet sizes;
        ChannelECPMList disappear_channel_ecpms;
        SizeChannelList appear_channel_ecpms;
        CampaignSvcs::AuctionType auction_type;

      public:
        template<typename OStream>
        void print(OStream& ostr, const char* offset) const noexcept;

        bool operator==(const InventoryInfo& right) const noexcept;

      private:
        static
        std::ostream&
        print_channels_(
          std::ostream& ostr,
          const ChannelIdList& lst)
          noexcept;

        template<typename OStream>
        static
        OStream&
        print_channels_(
          OStream& ostr,
          const ChannelIdVector& lst)
          noexcept;

        template<typename OStream>
        OStream&
        print_channel_counter_map_(
          OStream& ostr,
          const ChannelImpCounterVector& chs) const
          noexcept;

        template<typename OStream>
        OStream&
        print_channel_avg_revenue_map_(
          OStream& ostr,
          const ChannelAvgRevenueVector& chs) const
          noexcept;
      };

      virtual
      void process_request(const InventoryInfo&)
        /*throw(Exception)*/ = 0;

      virtual
      void process_user(const InventoryUserInfo&)
        /*throw(Exception)*/ = 0;

    protected:
      virtual
      ~InventoryActionProcessor() noexcept = default;
    };

    typedef ReferenceCounting::SmartPtr<InventoryActionProcessor>
      InventoryActionProcessor_var;
  }
}

namespace AdServer
{
  namespace RequestInfoSvcs
  {
    namespace
    {
      template<typename ContainerType>
      inline
      bool compare_seq_(
        const ContainerType& left,
        const ContainerType& right) noexcept
      {
        return left.size() == right.size() &&
          std::equal(left.begin(), left.end(), right.begin());
      }

      template<typename ContainerType>
      inline
      bool compare_maps_(
        const ContainerType& left,
        const ContainerType& right) noexcept
      {
        return left.size() == right.size() &&
          std::equal(left.begin(), left.end(), right.begin(), Algs::PairEqual());
      }
    }

    template<typename OStream>
    void
    MatchRequestProcessor::MatchInfo::print(
      OStream& ostr, const char* offset) const
      noexcept
    {
      ostr << offset << "user_id: '" << user_id << "'" << std::endl <<
        offset << "time: " << time.get_gm_time() << std::endl <<
        offset << "colo_id: " << colo_id << std::endl <<
        offset << "tag_size: " << tag_size << std::endl <<
        offset << "country: " << country_code << std::endl <<
        offset << "triggered_expression_channels:";
      if(triggered_expression_channels)
      {
        Algs::print(ostr,
          triggered_expression_channels->begin(), triggered_expression_channels->end());
      }
      ostr << offset << "triggered_cpm_expression_channels:";
      Algs::print(ostr,
        triggered_cpm_expression_channels.begin(),
        triggered_cpm_expression_channels.end());
      ostr << offset << "display_ad:";
      if(display_ad)
      {
        ostr << " [ " << display_ad->avg_revenue << ": ";
        Algs::print(ostr, display_ad->imp_channels.begin(), display_ad->imp_channels.end());
        ostr << " ]";
      }
      ostr << std::endl << offset << "text_ad:";
      for(AdBidSlotList::const_iterator sit = text_ads.begin();
          sit != text_ads.end(); ++sit)
      {
        ostr << " [ " << sit->avg_revenue << ": ";
        Algs::print(ostr, sit->imp_channels.begin(), sit->imp_channels.end());
        ostr << " ]";
      }
      ostr << std::endl;
    }

    inline
    bool
    SizeChannel::operator==(const SizeChannel& right) const
    {
      return size == right.size &&
        channel_id == right.channel_id;
    }

    inline
    bool
    ChannelECPM::operator==(const ChannelECPM& right) const
    {
      return SizeChannel::operator==(right) &&
        ecpm == right.ecpm;
    }

    inline
    bool
    InventoryActionProcessor::InventoryInfo::
    ChannelImpInfo::operator==(
      const ChannelImpInfo& right) const noexcept
    {
      return compare_seq_(imp_channels, right.imp_channels) &&
        compare_seq_(imp_other_channels, right.imp_other_channels) &&
        compare_seq_(impop_no_imp_channels, right.impop_no_imp_channels);
    }

    inline
    bool
    InventoryActionProcessor::InventoryInfo::
    ChannelImpInfo::empty() const noexcept
    {
      return imp_channels.empty() &&
        imp_other_channels.empty() &&
        impop_no_imp_channels.empty();
    }

    inline
    bool
    InventoryActionProcessor::InventoryInfo::
    ChannelImpAppearInfo::operator==(
      const ChannelImpAppearInfo& right) const noexcept
    {
      return compare_seq_(impop_appear_channels, right.impop_appear_channels) &&
        compare_seq_(imp_appear_channels, right.imp_appear_channels) &&
        compare_seq_(imp_other_appear_channels, right.imp_other_appear_channels) &&
        compare_seq_(impop_no_imp_appear_channels, right.impop_no_imp_appear_channels);
    }

    inline
    bool
    InventoryActionProcessor::InventoryInfo::
    ChannelImpAppearInfo::empty() const noexcept
    {
      return impop_appear_channels.empty() &&
        imp_appear_channels.empty() &&
        imp_other_appear_channels.empty() &&
        impop_no_imp_appear_channels.empty();
    }

    inline
    std::ostream&
    InventoryActionProcessor::InventoryInfo::print_channels_(
      std::ostream& ostr,
      const ChannelIdList& lst)
      noexcept
    {
      Algs::print(ostr, lst.begin(), lst.end());
      return ostr;
    }

    template<typename OStream>
    OStream&
    InventoryActionProcessor::InventoryInfo::print_channels_(
      OStream& ostr,
      const ChannelIdVector& lst)
      noexcept
    {
      Algs::print(ostr, lst.begin(), lst.end());
      return ostr;
    }

    template<typename OStream>
    OStream&
    InventoryActionProcessor::InventoryInfo::print_channel_counter_map_(
      OStream& ostr,
      const ChannelImpCounterVector& chs) const noexcept
    {
      for(auto ch_it = chs.begin(); ch_it != chs.end(); ++ch_it)
      {
        ostr << "[" << ch_it->first << ":" << ch_it->second.imps << "," <<
          ch_it->second.revenue << "]";
      }

      return ostr;
    }

    template<typename OStream>
    OStream&
    InventoryActionProcessor::InventoryInfo::print_channel_avg_revenue_map_(
      OStream& ostr,
      const ChannelAvgRevenueVector& chs) const noexcept
    {
      for(auto ch_it = chs.begin(); ch_it != chs.end(); ++ch_it)
      {
        ostr << "[" << ch_it->first << ":" << ch_it->second << "]";
      }

      return ostr;
    }

    template<typename OStream>
    void
    InventoryActionProcessor::InventoryInfo::print(
      OStream& ostr, const char* offset) const noexcept
    {
      ostr << offset << "time: " << time.get_gm_time() << std::endl <<
        offset << "country_code: " << country_code << std::endl <<
        offset << "impop_ecpm: " << impop_ecpm << std::endl <<
        offset << "auction_type: " << AdServer::CampaignSvcs::to_str(auction_type) << std::endl <<
        offset << "sizes: ";

      // appear fields
      Algs::print(ostr, sizes.begin(), sizes.end());

      ostr << std::endl << offset << "total appear channels: ";
      print_channels_(ostr, total_appear_channels) << std::endl;

      ostr << offset << "active appear channels: ";
      print_channels_(ostr, active_appear_channels) << std::endl;

      ostr << offset << "display impop appear channels: ";
      print_channels_(ostr, display_appears.impop_appear_channels) << std::endl;

      ostr << offset << "display imp appear channels: ";
      print_channels_(ostr, display_appears.imp_appear_channels) << std::endl;

      ostr << offset << "display imp other appear channels: ";
      print_channels_(ostr, display_appears.imp_other_appear_channels) << std::endl;

      ostr << offset << "display impop no imp appear channels: ";
      print_channels_(ostr, display_appears.impop_no_imp_appear_channels) << std::endl;

      ostr << offset << "text impop appear channels: ";
      print_channels_(ostr, text_appears.impop_appear_channels) << std::endl;

      ostr << offset << "text imp appear channels: ";
      print_channels_(ostr, text_appears.imp_appear_channels) << std::endl;

      ostr << offset << "text imp other appear channels: ";
      print_channels_(ostr, text_appears.imp_other_appear_channels) << std::endl;

      ostr << offset << "text impop no imp appear channels: ";
      print_channels_(ostr, text_appears.impop_no_imp_appear_channels) << std::endl;

      // imp fields
      ostr << offset << "display channels avg revenues: ";
      print_channel_avg_revenue_map_(ostr, display_imps.imp_channels) << std::endl;

      ostr << offset << "display imp other channels: ";
      print_channel_counter_map_(ostr, display_imps.imp_other_channels) << std::endl;

      ostr << offset << "display impop no imp channels: ";
      print_channel_counter_map_(ostr, display_imps.impop_no_imp_channels) << std::endl;

      ostr << offset << "text channels avg revenues: ";
      print_channel_avg_revenue_map_(ostr, text_imps.imp_channels) << std::endl;

      ostr << offset << "text imp other channels: ";
      print_channel_counter_map_(ostr, text_imps.imp_other_channels) << std::endl;

      ostr << offset << "text impop no imp channels: ";
      print_channel_counter_map_(ostr, text_imps.impop_no_imp_channels) << std::endl;

      // channel by cpm fields
      ostr << offset << "appear channel ecpms: ";

      for(SizeChannelList::const_iterator size_channel_it =
            appear_channel_ecpms.begin();
          size_channel_it != appear_channel_ecpms.end();
          ++size_channel_it)
      {
        ostr << "[" << size_channel_it->size <<
          ", " << size_channel_it->channel_id << "] ";
      }

      ostr << std::endl;
      ostr << offset << "disappear channel ecpms: ";

      for(ChannelECPMList::const_iterator ecpm_it =
            disappear_channel_ecpms.begin();
          ecpm_it != disappear_channel_ecpms.end();
          ++ecpm_it)
      {
        ostr << "[" << ecpm_it->size <<
          ", " << ecpm_it->ecpm <<
          ", " << ecpm_it->channel_id << "] ";
      }

      ostr << std::endl;
    }

    inline
    bool
    InventoryActionProcessor::InventoryInfo::operator==(
      const InventoryInfo& right) const noexcept
    {
      return
        time == right.time &&
        compare_seq_(total_appear_channels, right.total_appear_channels) &&
        compare_seq_(active_appear_channels, right.active_appear_channels) &&
        display_appears == right.display_appears &&
        text_appears == right.text_appears &&
        display_imps == right.display_imps &&
        text_imps == right.text_imps &&
        country_code == right.country_code &&
        impop_ecpm == right.impop_ecpm &&
        auction_type == right.auction_type &&
        compare_seq_(sizes, right.sizes) &&
        compare_seq_(appear_channel_ecpms, right.appear_channel_ecpms) &&
        compare_seq_(disappear_channel_ecpms, right.disappear_channel_ecpms);
    }
  }
}

#endif /*EXPRESSIONMATCHER_INVENTORYACTIONPROCESSOR_HPP*/
