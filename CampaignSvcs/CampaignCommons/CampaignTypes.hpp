#ifndef CAMPAIGNCONFIGTYPES_HPP
#define CAMPAIGNCONFIGTYPES_HPP

#include <limits>
#include <set>
#include <map>
#include <vector>
#include <optional>
#include <Generics/Time.hpp>
#include <Generics/SimpleDecimal.hpp>
#include <Generics/Decimal.hpp>
#include <Commons/Algs.hpp>
#include <Commons/Containers.hpp>
#include <Commons/Interval.hpp>

namespace AdServer
{
  namespace CampaignSvcs
  {
    const unsigned long MAX_TARGET_USERS_GROUPS = 100;

    namespace AccountTypeFlags
    {
      const unsigned long USE_SELF_BUDGET = 0x02;
      const unsigned long GROSS = 0x10;
      const unsigned long INVOICE_COMMISION = 0x1000;
      const unsigned long AGENCY_PROFIT_BY_PUB_AMOUNT = 0x8000;
    };

    namespace StringDictionaries
    {
      const std::string WEB_APPLICATION = "web-app";
      const std::string WEB_APPLICATION_SOURCE = "web-app-source";
      const std::string WEB_APPLICATION_OPERATION = "web-app-operation";
    }

    namespace AdTokens
    {
      static const char FOOTER_URL[] = "AD_FOOTER_URL";
    }

    typedef std::vector<unsigned long> ChannelIdArray;
    typedef std::list<unsigned long> ChannelIdList;
    typedef std::set<unsigned long> ChannelIdSet;

    struct ChannelIdSortedArray: protected std::vector<unsigned long>
    {
      typedef std::vector<unsigned long>::const_iterator const_iterator;

      enum ConstructType
      {
        UNSORTED,
        SORTED
      };

      template<typename IteratorType>
      ChannelIdSortedArray(IteratorType begin, IteratorType end, ConstructType construct_type)
      {
        reserve(std::distance(begin, end));
        std::copy(begin, end, std::back_inserter(*this));
        if(construct_type == UNSORTED)
        {
          std::sort(this->begin(), this->end());
        }
      }

      const_iterator
      find(unsigned long val) const
      {
        return std::find(
          std::vector<unsigned long>::begin(), std::vector<unsigned long>::end(), val);
      }

      const_iterator
      begin() const
      {
        return this->begin();
      }

      const_iterator
      end() const
      {
        return this->end();
      }
    };

    enum NativeAdsImpressionTrackerType
    {
      NAITT_IMP,
      NAITT_JS,
      NAITT_RESOURCES
    };

    enum AdInstantiateType
    {
      AIT_URL = 1,
      AIT_BODY, // body
      AIT_URL_IN_BODY,
      AIT_SCRIPT_WITH_URL,
      AIT_IFRAME_WITH_URL,
      AIT_URL_PARAMS, // result is sequence of parameters 'a=b&...'
      AIT_NONSECURE_URL,
      AIT_DATA_URL_PARAM, // result is 'd=...'
      AIT_DATA_PARAM_VALUE, // result is value of d
      AIT_VIDEO_URL, // equal to AIT_URL but give link to video domain
      AIT_VIDEO_NONSECURE_URL, // equal to AIT_VIDEO_URL but give http link to video domain
      AIT_VIDEO_URL_IN_BODY //equal to AIT_VIDEO_URL, but has another response
    };

    enum RevenueType
    {
      RT_NONE,
      RT_ABSOLUTE,
      RT_SHARE
    };

    enum AdRequestType
    {
      // add new veriants only to end (used in cluster exchange protocol)
      AR_NORMAL = 1,
      AR_TANX,
      AR_OPENRTB,
      AR_OPENRTB_WITH_CLICKURL,
      AR_OPENX,
      AR_LIVERAIL,
      AR_ALLYES,
      AR_ADRIVER,
      AR_BAIDU,
      AR_BAIDU_STATIC,
      AR_APPNEXUS,
      AR_YANDEX,
      AR_GOOGLE
    };

    enum UserStatus
    {
      US_UNDEFINED = 1,
      US_OPTIN,
      US_OPTOUT,
      US_PROBE,
      US_EXTERNALPROBE,
      US_TEMPORARY,
      US_NOEXTERNALID,
      US_FOREIGN,
      US_BLACKLISTED,
      US_NONE // campaign targeting specific alias for non US_OPTIN, US_OPTOUT statuses
    };

    enum ColocationAdServingType
    {
      CS_ALL = 0,
      CS_ONLY_OPTIN,
      CS_NON_OPTOUT,
      CS_NONE
    };

    enum AuctionRateType
    {
      AR_GROSS = 0,
      AR_NET
    };

    enum MarginRuleType
    {
      MR_STOP,
      MR_SUM,
      MR_MAX
    };

    enum ExpandDirectionType
    {
      ED_UP = 1,
      ED_RIGHT = 2,
      ED_DOWN = 4,
      ED_LEFT = 8,
      ED_ALL = ED_UP | ED_RIGHT | ED_DOWN | ED_LEFT
    };

    enum CCGRateType
    {
      CR_CPM = 'M',
      CR_CPC = 'C',
      CR_CPA = 'A',
      CR_MAXBID = 'B',
      CR_ALL = '-'
    };

    enum CCGType
    {
      CT_DISPLAY = 'D',
      CT_TEXT = 'T',
      CT_ALL = '-'
    };

    enum CampaignMode
    {
      CM_RANDOM = 0,
      CM_NON_RANDOM
    };

    enum AuctionTypeChar
    {
      ATC_RANDOM = 'R',
      ATC_MAX_ECPM = 'M',
      ATC_PROPORTIONAL_PROBABILITY = 'P'
    };

    enum AuctionType
    {
      AT_RANDOM,
      AT_MAX_ECPM,
      AT_PROPORTIONAL_PROBABILITY
    };

    enum ProfilingType
    {
      PT_PROFILING_INFO     = 1,
      PT_OTHER              = 2,
      PT_ALL = PT_PROFILING_INFO | PT_OTHER
    };

    enum BidStrategy
    {
      BS_MAX_REACH = 0,
      BS_MIN_CTR_GOAL
    };

    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    inline
    AuctionType
    get_auction_type(AuctionTypeChar at)
      /*throw(eh::Exception)*/
    {
      switch (at)
      {
      case ATC_RANDOM:
        return AT_RANDOM;
      case ATC_MAX_ECPM:
        return AT_MAX_ECPM;
      case ATC_PROPORTIONAL_PROBABILITY:
        return AT_PROPORTIONAL_PROBABILITY;
      }
      Stream::Error ostr;
      ostr << "Wrong auction char: " << (char)at;
      throw Exception(ostr);
    }

    template <class Stream>
    inline
    AuctionType
    get_auction_type(Stream& from)
      /*throw(eh::Exception)*/
    {
      char value = '\0';
      from >> value;
      return get_auction_type(AuctionTypeChar(value));
    }

    inline
    char
    put_auction_type(AuctionType at)
      /*throw(eh::Exception)*/
    {
      switch (at)
      {
      case AT_RANDOM:
        return ATC_RANDOM;
      case AT_MAX_ECPM:
        return ATC_MAX_ECPM;
      case AT_PROPORTIONAL_PROBABILITY:
        return ATC_PROPORTIONAL_PROBABILITY;
      }
      Stream::Error ostr;
      ostr << "Wrong auction type: " << at;
      throw Exception(ostr);
    }

    template <class Stream> 
    inline
    Stream&
    operator>>(Stream& is, AuctionType& auction_type)
      /*throw(eh::Exception)*/
    {
      auction_type = get_auction_type(is);
      return is;
    }

    enum RequestVerificationType
    {
      RVT_NOTICE,
      RVT_IMPRESSION,
      RVT_CUSTOM
    };

    typedef std::set<unsigned long> AllowedDurationSet;

    typedef Commons::Interval<unsigned long> PriceRange;
    typedef Commons::IntervalSet<unsigned long> PriceRangeSet;

    typedef Commons::Interval<unsigned long> WeeklyRunIntervalDef;
    typedef Commons::IntervalSet<unsigned long> WeeklyRunIntervalSet;

    typedef Generics::SimpleDecimal<uint64_t, 18, 8> RevenueDecimal;
    typedef Generics::SimpleDecimal<uint64_t, 18, 8> CTRDecimal;
    typedef Generics::Decimal<uint64_t, 36, 16> ExtRevenueDecimal;
    typedef Generics::SimpleDecimal<uint32_t, 6, 5> DeliveryThresholdDecimal;
    typedef Generics::Decimal<uint64_t, 36, 8> ImpRevenueDecimal;
    //typedef RevenueDecimal EcpmDecimal;
    typedef std::optional<RevenueDecimal> OptionalRevenueDecimal;

    const RevenueDecimal REVENUE_ONE(false, 1, 0);
    const ExtRevenueDecimal EXT_REVENUE_ONE(false, 1, 0);
    const RevenueDecimal NEGATIVE_REVENUE_ONE(true, 1, 0);

    typedef Generics::SimpleDecimal<uint64_t, 18, 8> CoordDecimal;
    typedef Generics::SimpleDecimal<uint64_t, 18, 8> AccuracyDecimal;

    struct CampaignDeliveryLimits
    {
      bool operator==(const CampaignDeliveryLimits& right) const noexcept;

      Generics::Time date_start;
      Generics::Time date_end;
      OptionalRevenueDecimal budget;
      OptionalRevenueDecimal daily_budget;
      char delivery_pacing;
      std::optional<unsigned long> imps;
      std::optional<unsigned long> daily_imps;
      std::optional<unsigned long> clicks;
      std::optional<unsigned long> daily_clicks;
    };

    class OptionValueMap: public std::map<unsigned long, std::string>
    {
    public:
      bool operator==(const OptionValueMap& right) const noexcept;
    };

    struct OptionValue
    {
      long option_id;
      std::string value;

      OptionValue(): option_id(0) {}

      OptionValue(long option_id_val, const char* val)
        : option_id(option_id_val),
          value(val)
      {}

      OptionValue(long option_id_val, const String::SubString& val)
        : option_id(option_id_val),
          value(val.str())
      {}

      bool operator==(const OptionValue& right) const noexcept;
    };

    const unsigned long RANDOM_PARAM_MAX = 10000000;
    const unsigned long TAG_DELIVERY_MAX = 10000;
    typedef std::map<unsigned long, unsigned long> TagDeliveryMap;

    const RevenueDecimal REVENUE_DECIMAL_THS = RevenueDecimal(false, 1000, 0);
    const ExtRevenueDecimal EXT_REVENUE_DECIMAL_THS = ExtRevenueDecimal(false, 1000, 0);
    const RevenueDecimal ECPM_FACTOR = RevenueDecimal(false, 100000, 0);

    const long MAX_ADS_PER_TAG_OPTION_ID = -2;
    const char AD_IMAGE_PATH_TOKEN[] = "ADIMAGE-PATH";
    const long AD_IMAGE_PATH_OPTION_ID = -3;
    const char AD_IMAGE_PATH_SUFFIX_TOKEN[] = "ADIMAGE-PATH-SUFFIX";
    const long AD_IMAGE_PATH_SUFFIX_OPTION_ID = -4;
    const char FOOTER_URL_TOKEN[] = "AD_FOOTER_URL";
    const long FOOTER_URL_OPTION_ID = -5;
    const char FOOTER_ENABLED_TOKEN[] = "AD_FOOTER_ENABLED";
    const long FOOTER_ENABLED_OPTION_ID = -6;
    const char DEFAULT_CREATIVE_WIDTH_TOKEN[] = "WIDTH";
    const long DEFAULT_CREATIVE_WIDTH_OPTION_ID = -7;
    const char DEFAULT_CREATIVE_HEIGHT_TOKEN[] = "HEIGHT";
    const long DEFAULT_CREATIVE_HEIGHT_OPTION_ID = -8;
    const char CREATIVE_MAX_WIDTH_TOKEN[] = "CRMAXWIDTH";
    const long CREATIVE_MAX_WIDTH_OPTION_ID = -9;
    const char CREATIVE_MAX_HEIGHT_TOKEN[] = "CRMAXHEIGHT";
    const long CREATIVE_MAX_HEIGHT_OPTION_ID = -10;
    const char CREATIVE_EXPAND_DIRECTION_TOKEN[] = "CREXPANDDIRECTION";
    const long CREATIVE_EXPAND_DIRECTION_OPTION_ID = -11;
    const long TAG_MIN_VISIBILITY_OPTION_ID = -12;
    const long PUBL_TAG_TRACK_PIXEL_OPTION_ID = -13;
    const char TAG_MIN_VISIBILITY_TOKEN[] = "TAG_MIN_VISIBILITY_";
    const char MAX_RANDOM_CPM_TOKEN[] = "MAX_RANDOM_CPM_";
    const long MAX_RANDOM_CPM_OPTION_ID = -14;

    const long TAG_PASSBACK_URL_OPTION_ID = -14;
    const char TAG_PASSBACK_URL_TOKEN[] = "PASSBACK_URL";
    const long TAG_PASSBACK_TYPE_OPTION_ID = -15;
    const char TAG_PASSBACK_TYPE_TOKEN[] = "PASSBACK_TYPE";
    const long TAG_PASSBACK_CODE_OPTION_ID = -16;
    const char TAG_PASSBACK_CODE_TOKEN[] = "PASSBACK_CODE";

    const char CRVBASE_TOKEN[] = "CRVBASE";
    const long CRVBASE_OPTION_ID = -17;

    const char MP4_DURATION_HFORMAT_TOKEN[] = "MP4_DURATION_HFORMAT";
    const long MP4_DURATION_HFORMAT_OPTION_ID = -18;

    const char SKIP_MIN_ECPM_TOKEN[] = "SKIP_MIN_ECPM";
    const long SKIP_MIN_ECPM_OPTION_ID = -19;

    const unsigned long UNDEFINED_PUB_POSITION_BOTTOM = 0x100;

    inline
    void
    narrow_decimal(
      ExtRevenueDecimal& result,
      const RevenueDecimal& source)
    {
      result = ExtRevenueDecimal(source.str());
    }

    inline
    void
    narrow_decimal(
      RevenueDecimal& result,
      const ExtRevenueDecimal& source)
    {
      result = RevenueDecimal(
        ExtRevenueDecimal(source).floor(
          RevenueDecimal::FRACTION_RANK).str());
    }

    const char*
    to_str(UserStatus status) noexcept;

    const char*
    to_str(CampaignMode mode) noexcept;

    const char*
    to_str(AuctionType type) noexcept;
  }
}

namespace AdServer
{
  namespace CampaignSvcs
  {
    inline
    bool CampaignDeliveryLimits::operator==(
      const CampaignDeliveryLimits& right) const
      noexcept
    {
      return date_start == right.date_start &&
        date_end == right.date_end &&
        budget == right.budget &&
        daily_budget == right.daily_budget &&
        delivery_pacing == right.delivery_pacing &&
        imps == right.imps &&
        daily_imps == right.daily_imps &&
        clicks == right.clicks &&
        daily_clicks == right.daily_clicks;
    }

    inline
    bool
    OptionValueMap::operator==(const OptionValueMap& right) const noexcept
    {
      return size() == right.size() &&
        std::equal(begin(), end(), right.begin(), Algs::PairEqual());
    }

    inline
    bool
    OptionValue::operator==(const OptionValue& right) const noexcept
    {
      return option_id == right.option_id &&
        value == right.value;
    }
  }
}

#endif
