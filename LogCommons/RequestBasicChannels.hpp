#ifndef AD_SERVER_LOG_PROCESSING_REQUEST_BASIC_CHANNELS_HPP
#define AD_SERVER_LOG_PROCESSING_REQUEST_BASIC_CHANNELS_HPP


#include <iosfwd>
#include <Generics/Time.hpp>
#include <Generics/Rand.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

#include <Commons/UserInfoManip.hpp>
#include <Commons/StringHolder.hpp>
#include <LogCommons/GenericLogIoImpl.hpp>
#include "LogCommons.hpp"
#include "StatCollector.hpp"
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>
#include <LogCommons/CsvUtils.hpp>

namespace AdServer {
namespace LogProcessing {

struct RequestBasicChannelsKey
{
  RequestBasicChannelsKey(): time_(), isp_time_(), colo_id_() {}

  RequestBasicChannelsKey(
    const SecondsTimestamp& time,
    const SecondsTimestamp& isp_time,
    unsigned long colo_id
  )
  :
    time_(time),
    isp_time_(isp_time),
    colo_id_(colo_id)
  {
  }

  bool operator==(const RequestBasicChannelsKey& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return time_ == rhs.time_ &&
      isp_time_ == rhs.isp_time_ &&
      colo_id_ == rhs.colo_id_;
  }

  bool operator<(const RequestBasicChannelsKey& rhs) const
  {
    return time_ < rhs.time_ ||
      (time_ == rhs.time_ && (isp_time_ < rhs.isp_time_ ||
      (isp_time_ == rhs.isp_time_ && colo_id_ < rhs.colo_id_)));
  }

  Generics::Time time() const
  {
    return time_.time();
  }

  Generics::Time isp_time() const
  {
    return isp_time_.time();
  }

  unsigned long colo_id() const
  {
    return colo_id_;
  }

  friend std::istream&
  operator>>(std::istream& is, RequestBasicChannelsKey& key);

  friend std::ostream&
  operator<<(std::ostream& os, const RequestBasicChannelsKey& key)
    /*throw(eh::Exception)*/;

private:
  SecondsTimestamp time_;
  SecondsTimestamp isp_time_;
  unsigned long colo_id_;
};

class RequestBasicChannelsInnerData_V_2_7
{
public:
  typedef AdServer::LogProcessing::FixedNumber FixedNum;

  struct TriggerMatch
  {
    TriggerMatch(): channel_id(), channel_trigger_id() {}
    TriggerMatch(unsigned long ch_id, unsigned long trig_id) noexcept
      : channel_id(ch_id), channel_trigger_id(trig_id)
    {}

    bool operator==(const TriggerMatch& match) const
    {
      if (this == &match)
      {
        return true;
      }
      return channel_id == match.channel_id &&
        channel_trigger_id == match.channel_trigger_id;
    }

    unsigned long channel_id;
    unsigned long channel_trigger_id;
  };

  typedef std::list<TriggerMatch> TriggerMatchList;

  struct PartlyHistoryMatch
  {
    PartlyHistoryMatch(): channel_id(), visits(), minimum_visits() {}

    bool operator==(const PartlyHistoryMatch& match) const
    {
      if (this == &match)
      {
        return true;
      }
      return channel_id == match.channel_id &&
        visits == match.visits &&
        minimum_visits == match.minimum_visits;
    }

    unsigned long channel_id;
    unsigned long visits;
    unsigned long minimum_visits;
  };

  typedef std::list<PartlyHistoryMatch> PartlyHistoryMatchList;

  class AdSlotImpression
  {
  private:
    class Data: public ReferenceCounting::AtomicImpl
    {
    public:
      Data(): revenue(FixedNum::ZERO), impression_channels() {}

      bool operator==(const Data& rhs) const
      {
        return &rhs == this ||
          (revenue == rhs.revenue &&
          impression_channels == rhs.impression_channels);
      }

      FixedNum revenue;
      NumberList impression_channels;

    protected:
      virtual
      ~Data() noexcept = default;
    };

    typedef ReferenceCounting::SmartPtr<Data> DataPtr;

  public:
    AdSlotImpression() noexcept {}

    template <class IMPRESSION_CHANNELS_CONTAINER_TYPE_>
    AdSlotImpression(
      const FixedNum& revenue,
      const IMPRESSION_CHANNELS_CONTAINER_TYPE_& impression_channels
    )
    :
      data_(new Data)
    {
      data_->revenue = revenue;
      data_->impression_channels.assign(impression_channels.begin(),
        impression_channels.end());
    }

    bool operator==(const AdSlotImpression& rhs) const
    {
      return &rhs == this || data_ == rhs.data_ || *data_ == *rhs.data_;
    }

    const FixedNum& revenue() const
    {
      return data_->revenue;
    }

    const NumberList& impression_channels() const
    {
      return data_->impression_channels;
    }

    friend FixedBufStream<TabCategory>&
    operator>>(FixedBufStream<TabCategory>&, AdSlotImpression&);

    friend std::ostream&
    operator<<(std::ostream& os, const AdSlotImpression& ad_imp);

  private:
    DataPtr data_;
  };

  typedef OptionalValue<AdSlotImpression> AdSlotImpressionOptional;

  class AdBidSlotImpression
  {
  private:
    class Data: public ReferenceCounting::AtomicImpl
    {
    public:
      Data()
      :
        revenue(FixedNum::ZERO),
        revenue_bid(FixedNum::ZERO),
        impression_channels()
      {
      }

      bool operator==(const Data& rhs) const
      {
        return &rhs == this ||
          (revenue == rhs.revenue &&
          revenue_bid == rhs.revenue_bid &&
          impression_channels == rhs.impression_channels);
      }

      FixedNum revenue;
      FixedNum revenue_bid;
      NumberList impression_channels;

    protected:
      virtual
      ~Data() noexcept = default;
    };

    typedef ReferenceCounting::SmartPtr<Data> DataPtr;

  public:
    AdBidSlotImpression() {}

    template <class IMPRESSION_CHANNELS_CONTAINER_TYPE_>
    AdBidSlotImpression(
      const FixedNum& revenue,
      const FixedNum& revenue_bid,
      const IMPRESSION_CHANNELS_CONTAINER_TYPE_& impression_channels
    )
    :
      data_(new Data)
    {
      data_->revenue = revenue;
      data_->revenue_bid = revenue_bid;
      data_->impression_channels.assign(impression_channels.begin(),
        impression_channels.end());
    }

    bool operator==(const AdBidSlotImpression& rhs) const
    {
      return &rhs == this || data_ == rhs.data_ || *data_ == *rhs.data_;
    }

    const FixedNum& revenue() const
    {
      return data_->revenue;
    }

    const FixedNum& revenue_bid() const
    {
      return data_->revenue_bid;
    }

    const NumberList& impression_channels() const
    {
      return data_->impression_channels;
    }

    friend FixedBufStream<SlashCategory>&
    operator>>(FixedBufStream<SlashCategory>&, AdBidSlotImpression&);

    friend std::ostream&
    operator<<(std::ostream& os, const AdBidSlotImpression& absi);

  private:
    DataPtr data_;
  };

  typedef std::list<AdBidSlotImpression> AdBidSlotImpressionList;

  class AdRequestProps
  {
    typedef StringIoWrapperOptional OptionalStringT;

    class Data: public ReferenceCounting::AtomicImpl
    {
    public:
      Data()
      :
        size(),
        country_code(),
        max_text_ads(),
        text_ad_cost_threshold(FixedNum::ZERO),
        display_ad_shown(),
        text_ad_shown()
      {
      }


      bool operator==(const Data& rhs) const
      {
        return &rhs == this ||
          (size == rhs.size &&
          country_code == rhs.country_code &&
          max_text_ads == rhs.max_text_ads &&
          text_ad_cost_threshold == rhs.text_ad_cost_threshold &&
          display_ad_shown == rhs.display_ad_shown &&
          text_ad_shown == rhs.text_ad_shown);
      }

      bool empty() const
      {
        return size.empty() &&
          !country_code.present() &&
          !max_text_ads &&
          text_ad_cost_threshold == FixedNum::ZERO &&
          !display_ad_shown.present() &&
          text_ad_shown.empty();
      }

      Aux_::StringIoWrapper size;
      OptionalStringT country_code;
      unsigned long max_text_ads;
      FixedNum text_ad_cost_threshold;
      AdSlotImpressionOptional display_ad_shown;
      AdBidSlotImpressionList text_ad_shown;

    protected:
      virtual
      ~Data() noexcept = default;
    };

    typedef ReferenceCounting::SmartPtr<Data> DataPtr;

  public:
    AdRequestProps() noexcept {}

    AdRequestProps(
      const std::string& size,
      const std::string& country_code,
      unsigned long max_text_ads,
      const FixedNum& text_ad_cost_threshold,
      const AdSlotImpressionOptional& display_ad_shown,
      const AdBidSlotImpressionList& text_ad_shown
    )
    :
      data_(new Data)
    {
      data_->size = size;
      data_->country_code = country_code;
      data_->max_text_ads = max_text_ads;
      data_->text_ad_cost_threshold = text_ad_cost_threshold;
      data_->display_ad_shown = display_ad_shown;
      data_->text_ad_shown = text_ad_shown;
    }

    bool operator==(const AdRequestProps& rhs) const
    {
      return &rhs == this || data_ == rhs.data_ || *data_ == *rhs.data_;
    }

    bool empty() const
    {
      return data_->empty();
    }

    const std::string& size() const
    {
      return data_->size;
    }

    const std::string& country_code() const
    {
      return data_->country_code.get();
    }

    unsigned long max_text_ads() const
    {
      return data_->max_text_ads;
    }

    const FixedNum& text_ad_cost_threshold() const
    {
      return data_->text_ad_cost_threshold;
    }

    const AdSlotImpressionOptional& display_ad_shown() const
    {
      return data_->display_ad_shown;
    }

    const AdBidSlotImpressionList& text_ad_shown() const
    {
      return data_->text_ad_shown;
    }

    void normalize()
    {
      if (data_->country_code.get().size() > max_country_code_size_)
      {
        data_->country_code.get().resize(max_country_code_size_);
      }
    }

    friend FixedBufStream<TabCategory>&
    operator>>(FixedBufStream<TabCategory>& is, AdRequestProps& value);

    DataPtr data_;

  private:
    static const size_t max_country_code_size_ = 2;
  };

  typedef OptionalValue<AdRequestProps> AdRequestPropsOptional;

  RequestBasicChannelsInnerData_V_2_7() {}

  RequestBasicChannelsInnerData_V_2_7(
    char user_type,
    const UserId& user_id,
    const UserId& temporary_user_id,
    const NumberList& history_channels,
    const NumberList& platform_channels,
    const TriggerMatchList& page_trigger_channels,
    const TriggerMatchList& search_trigger_channels,
    const TriggerMatchList& url_trigger_channels,
    const PartlyHistoryMatchList& partly_from_to_history_channels,
    const PartlyHistoryMatchList& partly_now_to_history_channels,
    const AdRequestPropsOptional& ad_request
  )
  :
    holder_(
      new DataHolder(
        user_type,
        user_id,
        temporary_user_id,
        history_channels,
        platform_channels,
        page_trigger_channels,
        search_trigger_channels,
        url_trigger_channels,
        partly_from_to_history_channels,
        partly_now_to_history_channels,
        ad_request,
        Generics::safe_rand()
      )
    )
  {
  }

  bool operator==(const RequestBasicChannelsInnerData_V_2_7& data) const
  {
    return this == &data || holder_.in() == data.holder_.in() ||
      (holder_->user_type == data.holder_->user_type &&
      holder_->user_id == data.holder_->user_id &&
      holder_->temporary_user_id == data.holder_->temporary_user_id &&
      holder_->history_channels == data.holder_->history_channels &&
      holder_->platform_channels == data.holder_->platform_channels &&
      holder_->page_trigger_channels == data.holder_->page_trigger_channels &&
      holder_->search_trigger_channels ==
        data.holder_->search_trigger_channels &&
      holder_->url_trigger_channels == data.holder_->url_trigger_channels &&
      holder_->partly_from_to_history_channels ==
        data.holder_->partly_from_to_history_channels &&
      holder_->partly_now_to_history_channels ==
        data.holder_->partly_now_to_history_channels &&
      holder_->ad_request == data.holder_->ad_request);
  }

  char user_type() const
  {
    return holder_->user_type;
  }

  const UserId& user_id() const
  {
    return holder_->user_id;
  }

  const UserId& temporary_user_id() const
  {
    return holder_->temporary_user_id;
  }

  const NumberList& history_channels() const
  {
    return holder_->history_channels;
  }

  const NumberList& platform_channels() const
  {
    return holder_->platform_channels;
  }

  const TriggerMatchList& page_trigger_channels() const
  {
    return holder_->page_trigger_channels;
  }

  const TriggerMatchList& search_trigger_channels() const
  {
    return holder_->search_trigger_channels;
  }

  const TriggerMatchList& url_trigger_channels() const
  {
    return holder_->url_trigger_channels;
  }

  const PartlyHistoryMatchList& partly_from_to_history_channels() const
  {
    return holder_->partly_from_to_history_channels;
  }

  const PartlyHistoryMatchList& partly_now_to_history_channels() const
  {
    return holder_->partly_now_to_history_channels;
  }

  const AdRequestPropsOptional& ad_request() const
  {
    return holder_->ad_request;
  }

  unsigned long distrib_hash() const
  {
    if (holder_->user_id.is_null())
    {
      if (holder_->temporary_user_id.is_null())
      {
        return holder_->random;
      }
      return AdServer::Commons::uuid_distribution_hash(
        holder_->temporary_user_id);
    }
    return AdServer::Commons::uuid_distribution_hash(holder_->user_id);
  }

  friend std::istream&
  operator>>(std::istream& is, RequestBasicChannelsInnerData_V_2_7& data)
    /*throw(eh::Exception)*/;

private:
  class DataHolder: public ReferenceCounting::AtomicImpl
  {
  public:
    DataHolder()
    :
      user_type(),
      user_id(),
      temporary_user_id(),
      history_channels(),
      platform_channels(),
      page_trigger_channels(),
      search_trigger_channels(),
      url_trigger_channels(),
      partly_from_to_history_channels(),
      partly_now_to_history_channels(),
      ad_request(),
      random()
    {
    }

    DataHolder(
      char user_type_val,
      const UserId& user_id_val,
      const UserId& temporary_user_id_val,
      const NumberList& history_channels_val,
      const NumberList& platform_channels_val,
      const TriggerMatchList& page_trigger_channels_val,
      const TriggerMatchList& search_trigger_channels_val,
      const TriggerMatchList& url_trigger_channels_val,
      const PartlyHistoryMatchList& partly_from_to_history_channels_val,
      const PartlyHistoryMatchList& partly_now_to_history_channels_val,
      const AdRequestPropsOptional& ad_request_val,
      unsigned long random_val
    )
    :
      user_type(user_type_val),
      user_id(user_id_val),
      temporary_user_id(temporary_user_id_val),
      history_channels(history_channels_val),
      platform_channels(platform_channels_val),
      page_trigger_channels(page_trigger_channels_val),
      search_trigger_channels(search_trigger_channels_val),
      url_trigger_channels(url_trigger_channels_val),
      partly_from_to_history_channels(partly_from_to_history_channels_val),
      partly_now_to_history_channels(partly_now_to_history_channels_val),
      ad_request(ad_request_val),
      random(random_val)
    {
      if (ad_request.present())
      {
        ad_request.get().normalize();
      }
    }

    template <class ARCHIVE_>
    void serialize(ARCHIVE_& ar)
    {
      ar & user_type;
      ar & user_id;
      ar & temporary_user_id;
      ar & history_channels;
      ar & platform_channels;
      ar & page_trigger_channels;
      ar & search_trigger_channels;
      ar & url_trigger_channels;
      ar & partly_from_to_history_channels;
      ar & partly_now_to_history_channels;
      ar ^ ad_request;
    }

    void invariant() const /*throw(ConstraintViolation)*/
    {
      if (!user_type_is_valid_())
      {
        Stream::Error es;
        es << "RequestBasicChannelsInnerData_V_2_7::DataHolder::invariant(): "
          "user_type has invalid value '" << user_type << '\'';
        throw ConstraintViolation(es);
      }
    }

    char user_type;
    UserId user_id;
    UserId temporary_user_id;
    NumberList history_channels;
    NumberList platform_channels;
    TriggerMatchList page_trigger_channels;
    TriggerMatchList search_trigger_channels;
    TriggerMatchList url_trigger_channels;
    PartlyHistoryMatchList partly_from_to_history_channels;
    PartlyHistoryMatchList partly_now_to_history_channels;
    AdRequestPropsOptional ad_request;
    unsigned long random;

  private:
    virtual
    ~DataHolder() noexcept = default;

    bool user_type_is_valid_() const
    {
      return user_type == 'H' || user_type == 'P';
    }
  };

  typedef ReferenceCounting::SmartPtr<DataHolder> DataHolder_var;

  DataHolder_var holder_;
};


class RequestBasicChannelsInnerData_V_3_1
{
public:
  typedef RequestBasicChannelsInnerData_V_2_7::FixedNum FixedNum;

  typedef RequestBasicChannelsInnerData_V_2_7::TriggerMatch TriggerMatch;

  typedef RequestBasicChannelsInnerData_V_2_7::TriggerMatchList
    TriggerMatchList;

  typedef RequestBasicChannelsInnerData_V_2_7::AdSlotImpression
    AdSlotImpression;

  typedef RequestBasicChannelsInnerData_V_2_7::AdSlotImpressionOptional
    AdSlotImpressionOptional;

  typedef RequestBasicChannelsInnerData_V_2_7::AdBidSlotImpression
    AdBidSlotImpression;

  typedef RequestBasicChannelsInnerData_V_2_7::AdBidSlotImpressionList
    AdBidSlotImpressionList;

  class AdSelectProps
  {
    class Data: public ReferenceCounting::AtomicImpl
    {
    public:
      Data() noexcept {}

      Data(
        unsigned long tag_id_val,
        const std::string& format_val,
        bool test_request_val,
        bool profiling_available_val
      )
        noexcept
      :
        tag_id(tag_id_val),
        format(format_val),
        test_request(test_request_val),
        profiling_available(profiling_available_val)
      {
      }

      bool
      operator==(const Data& rhs) const
      {
        return &rhs == this ||
          (tag_id == rhs.tag_id &&
          format == rhs.format &&
          test_request == rhs.test_request &&
          profiling_available == rhs.profiling_available &&
          full_freq_caps == rhs.full_freq_caps);
      }

      unsigned long tag_id;
      SpacesMarksString format;
      bool test_request;
      bool profiling_available;
      NumberList full_freq_caps;

    protected:
      virtual
      ~Data() noexcept = default;
    };

    typedef ReferenceCounting::SmartPtr<Data> DataPtr;

    friend FixedBufStream<TabCategory>&
    operator>>(FixedBufStream<TabCategory>&, AdSelectProps&)
      /*throw(eh::Exception)*/;

  public:
    AdSelectProps() noexcept {}

    template <typename FullFreqCapsContainerType>
    AdSelectProps(
      unsigned long tag_id,
      const std::string& format,
      bool test_request,
      bool profiling_available,
      const FullFreqCapsContainerType& full_freq_caps
    )
    :
      data_(new Data(tag_id, format, test_request, profiling_available))
    {
      data_->full_freq_caps.assign(full_freq_caps.begin(),
        full_freq_caps.end());
    }

    bool
    operator==(const AdSelectProps& rhs) const
    {
      return &rhs == this || data_ == rhs.data_ || *data_ == *rhs.data_;
    }

    unsigned long
    tag_id() const
    {
      return data_->tag_id;
    }

    const std::string&
    format() const
    {
      return data_->format;
    }

    bool
    test_request() const
    {
      return data_->test_request;
    }

    bool
    profiling_available() const
    {
      return data_->profiling_available;
    }

    const NumberList&
    full_freq_caps() const
    {
      return data_->full_freq_caps;
    }

  private:
    DataPtr data_;
  };

  typedef OptionalValue<AdSelectProps> AdSelectPropsOptional;

  class AdRequestProps
  {
  private:
    typedef StringIoWrapperOptional OptionalStringT;

    class Data: public ReferenceCounting::AtomicImpl
    {
    public:
      Data() noexcept
      :
        size(),
        country_code(),
        max_text_ads(),
        text_ad_cost_threshold(FixedNum::ZERO),
        display_ad_shown(),
        text_ad_shown(),
        ad_select()
      {
      }

      bool operator==(const Data& rhs) const
      {
        return &rhs == this ||
          (size == rhs.size &&
          country_code == rhs.country_code &&
          max_text_ads == rhs.max_text_ads &&
          text_ad_cost_threshold == rhs.text_ad_cost_threshold &&
          display_ad_shown == rhs.display_ad_shown &&
          text_ad_shown == rhs.text_ad_shown &&
          ad_select == rhs.ad_select);
      }

      bool empty() const
      {
        return size.empty() &&
          !country_code.present() &&
          !max_text_ads &&
          text_ad_cost_threshold == FixedNum::ZERO &&
          !display_ad_shown.present() &&
          text_ad_shown.empty() &&
          !ad_select.present();
      }

      Aux_::StringIoWrapper size;
      OptionalStringT country_code;
      unsigned long max_text_ads;
      FixedNum text_ad_cost_threshold;
      AdSlotImpressionOptional display_ad_shown;
      AdBidSlotImpressionList text_ad_shown;
      AdSelectPropsOptional ad_select;

    protected:
      virtual
      ~Data() noexcept = default;
    };

    typedef ReferenceCounting::SmartPtr<Data> DataPtr;

  public:
    AdRequestProps() noexcept {}

    AdRequestProps(
      const std::string& size,
      const std::string& country_code,
      unsigned long max_text_ads,
      const FixedNum& text_ad_cost_threshold,
      const AdSlotImpressionOptional& display_ad_shown,
      const AdBidSlotImpressionList& text_ad_shown,
      const AdSelectPropsOptional& ad_select
    )
    :
      data_(new Data)
    {
      data_->size = size;
      data_->country_code = country_code;
      data_->max_text_ads = max_text_ads;
      data_->text_ad_cost_threshold = text_ad_cost_threshold;
      data_->display_ad_shown = display_ad_shown;
      data_->text_ad_shown = text_ad_shown;
      data_->ad_select = ad_select;
    }

    AdRequestProps(
      const RequestBasicChannelsInnerData_V_2_7::AdRequestProps& data
    )
    :
      data_(new Data)
    {
      data_->size = data.data_->size;
      data_->country_code = data.data_->country_code;
      data_->max_text_ads = data.data_->max_text_ads;
      data_->text_ad_cost_threshold = data.data_->text_ad_cost_threshold;
      data_->display_ad_shown = data.data_->display_ad_shown;
      data_->text_ad_shown = data.data_->text_ad_shown;
    }

    bool operator==(const AdRequestProps& rhs) const
    {
      return &rhs == this || data_ == rhs.data_ || *data_ == *rhs.data_;
    }

    bool empty() const
    {
      return data_->empty();
    }

    const std::string& size() const
    {
      return data_->size;
    }

    const std::string& country_code() const
    {
      return data_->country_code.get();
    }

    unsigned long max_text_ads() const
    {
      return data_->max_text_ads;
    }

    const FixedNum& text_ad_cost_threshold() const
    {
      return data_->text_ad_cost_threshold;
    }

    const AdSlotImpressionOptional& display_ad_shown() const
    {
      return data_->display_ad_shown;
    }

    const AdBidSlotImpressionList& text_ad_shown() const
    {
      return data_->text_ad_shown;
    }

    const AdSelectPropsOptional& ad_select() const
    {
      return data_->ad_select;
    }

    void normalize()
    {
      if (data_->country_code.get().size() > max_country_code_size_)
      {
        data_->country_code.get().resize(max_country_code_size_);
      }
    }

    friend FixedBufStream<TabCategory>&
    operator>>(FixedBufStream<TabCategory>& is, AdRequestProps& value);

  private:
    static const size_t max_country_code_size_ = 2;

    DataPtr data_;
  };

  typedef OptionalValue<AdRequestProps> AdRequestPropsOptional;

  RequestBasicChannelsInnerData_V_3_1() noexcept {}

  RequestBasicChannelsInnerData_V_3_1(
    char user_type,
    const UserId& user_id,
    const UserId& temporary_user_id,
    const NumberList& history_channels,
    const TriggerMatchList& page_trigger_channels,
    const TriggerMatchList& search_trigger_channels,
    const TriggerMatchList& url_trigger_channels,
    const AdRequestPropsOptional& ad_request
  )
  :
    holder_(
      new DataHolder(
        user_type,
        user_id,
        temporary_user_id,
        history_channels,
        page_trigger_channels,
        search_trigger_channels,
        url_trigger_channels,
        ad_request,
        Generics::safe_rand()
      )
    )
  {
  }

  bool operator==(const RequestBasicChannelsInnerData_V_3_1& data) const
  {
    return this == &data || holder_.in() == data.holder_.in() ||
      (holder_->user_type == data.holder_->user_type &&
      holder_->user_id == data.holder_->user_id &&
      holder_->temporary_user_id == data.holder_->temporary_user_id &&
      holder_->history_channels == data.holder_->history_channels &&
      holder_->page_trigger_channels == data.holder_->page_trigger_channels &&
      holder_->search_trigger_channels ==
        data.holder_->search_trigger_channels &&
      holder_->url_trigger_channels == data.holder_->url_trigger_channels &&
      holder_->ad_request == data.holder_->ad_request);
  }

  char user_type() const
  {
    return holder_->user_type;
  }

  const UserId& user_id() const
  {
    return holder_->user_id;
  }

  const UserId& temporary_user_id() const
  {
    return holder_->temporary_user_id;
  }

  const NumberList& history_channels() const
  {
    return holder_->history_channels;
  }

  const TriggerMatchList& page_trigger_channels() const
  {
    return holder_->page_trigger_channels;
  }

  const TriggerMatchList& search_trigger_channels() const
  {
    return holder_->search_trigger_channels;
  }

  const TriggerMatchList& url_trigger_channels() const
  {
    return holder_->url_trigger_channels;
  }

  const AdRequestPropsOptional& ad_request() const
  {
    return holder_->ad_request;
  }

  unsigned long distrib_hash() const
  {
    using AdServer::Commons::uuid_distribution_hash;
    if (holder_->user_id.is_null())
    {
      if (holder_->temporary_user_id.is_null())
      {
        return holder_->random;
      }
      return uuid_distribution_hash(holder_->temporary_user_id);
    }
    return uuid_distribution_hash(holder_->user_id);
  }

  friend std::istream&
  operator>>(std::istream& is, RequestBasicChannelsInnerData_V_3_1& data)
    /*throw(eh::Exception)*/;

private:
  class DataHolder: public ReferenceCounting::AtomicImpl
  {
  public:
    DataHolder()
    :
      user_type(),
      user_id(),
      temporary_user_id(),
      history_channels(),
      page_trigger_channels(),
      search_trigger_channels(),
      url_trigger_channels(),
      ad_request(),
      random()
    {
    }

    DataHolder(
      char user_type_val,
      const UserId& user_id_val,
      const UserId& temporary_user_id_val,
      const NumberList& history_channels_val,
      const TriggerMatchList& page_trigger_channels_val,
      const TriggerMatchList& search_trigger_channels_val,
      const TriggerMatchList& url_trigger_channels_val,
      const AdRequestPropsOptional& ad_request_val,
      unsigned long random_val
    )
    :
      user_type(user_type_val),
      user_id(user_id_val),
      temporary_user_id(temporary_user_id_val),
      history_channels(history_channels_val),
      page_trigger_channels(page_trigger_channels_val),
      search_trigger_channels(search_trigger_channels_val),
      url_trigger_channels(url_trigger_channels_val),
      ad_request(ad_request_val),
      random(random_val)
    {
      if (ad_request.present())
      {
        ad_request.get().normalize();
      }
    }

    template <class ARCHIVE_>
    void serialize(ARCHIVE_& ar)
    {
      ar & user_type;
      ar & user_id;
      ar & temporary_user_id;
      ar & history_channels;
      ar & page_trigger_channels;
      ar & search_trigger_channels;
      ar & url_trigger_channels;
      ar ^ ad_request;
    }

    void invariant() const /*throw(ConstraintViolation)*/
    {
      if (!user_type_is_valid_())
      {
        Stream::Error es;
        es << "RequestBasicChannelsInnerData_V_3_1::DataHolder::invariant(): "
          "user_type has invalid value '" << user_type << '\'';
        throw ConstraintViolation(es);
      }
    }

    char user_type;
    UserId user_id;
    UserId temporary_user_id;
    NumberList history_channels;
    TriggerMatchList page_trigger_channels;
    TriggerMatchList search_trigger_channels;
    TriggerMatchList url_trigger_channels;
    AdRequestPropsOptional ad_request;
    unsigned long random;

  private:
    virtual
    ~DataHolder() noexcept = default;

    bool user_type_is_valid_() const
    {
      return user_type == 'H' || user_type == 'P' || user_type == 'A';
    }
  };

  typedef ReferenceCounting::SmartPtr<DataHolder> DataHolder_var;

  DataHolder_var holder_;
};

class RequestBasicChannelsInnerData_V_3_3
{
public:
  typedef RequestBasicChannelsInnerData_V_3_1::FixedNum FixedNum;

  typedef RequestBasicChannelsInnerData_V_3_1::TriggerMatch TriggerMatch;

  typedef RequestBasicChannelsInnerData_V_3_1::TriggerMatchList
    TriggerMatchList;

  typedef RequestBasicChannelsInnerData_V_3_1::AdSlotImpression
    AdSlotImpression;

  typedef RequestBasicChannelsInnerData_V_3_1::AdSlotImpressionOptional
    AdSlotImpressionOptional;

  typedef RequestBasicChannelsInnerData_V_3_1::AdBidSlotImpression
    AdBidSlotImpression;

  typedef RequestBasicChannelsInnerData_V_3_1::AdBidSlotImpressionList
    AdBidSlotImpressionList;

  class AdSelectProps
  {
    class Data: public ReferenceCounting::AtomicImpl
    {
    public:
      Data() noexcept {}

      Data(
        unsigned long tag_id_val,
        const std::string& size_val,
        const std::string& format_val,
        bool test_request_val,
        bool profiling_available_val
      )
        noexcept
      :
        tag_id(tag_id_val),
        size(size_val),
        format(format_val),
        test_request(test_request_val),
        profiling_available(profiling_available_val)
      {
      }

      bool
      operator==(const Data& rhs) const
      {
        return &rhs == this ||
          (tag_id == rhs.tag_id &&
          size == rhs.size &&
          format == rhs.format &&
          test_request == rhs.test_request &&
          profiling_available == rhs.profiling_available &&
          full_freq_caps == rhs.full_freq_caps);
      }

      unsigned long tag_id;
      SpacesMarksString size;
      SpacesMarksString format;
      bool test_request;
      bool profiling_available;
      NumberList full_freq_caps;

    protected:
      virtual
      ~Data() noexcept = default;
    };

    typedef ReferenceCounting::SmartPtr<Data> DataPtr;

    friend FixedBufStream<TabCategory>&
    operator>>(FixedBufStream<TabCategory>&, AdSelectProps&)
      /*throw(eh::Exception)*/;

    friend std::ostream&
    operator<<(std::ostream& os, const AdSelectProps& ad_select_props)
      /*throw(eh::Exception)*/;

  public:
    AdSelectProps() noexcept {}

    template <typename FullFreqCapsContainerType>
    AdSelectProps(
      unsigned long tag_id,
      const std::string& size,
      const std::string& format,
      bool test_request,
      bool profiling_available,
      const FullFreqCapsContainerType& full_freq_caps
    )
    :
      data_(new Data(tag_id, size, format, test_request, profiling_available))
    {
      data_->full_freq_caps.assign(full_freq_caps.begin(),
        full_freq_caps.end());
    }

    bool
    operator==(const AdSelectProps& rhs) const
    {
      return &rhs == this || data_ == rhs.data_ || *data_ == *rhs.data_;
    }

    unsigned long
    tag_id() const
    {
      return data_->tag_id;
    }

    const std::string&
    size() const
    {
      return data_->size;
    }

    const std::string&
    format() const
    {
      return data_->format;
    }

    bool
    test_request() const
    {
      return data_->test_request;
    }

    bool
    profiling_available() const
    {
      return data_->profiling_available;
    }

    const NumberList&
    full_freq_caps() const
    {
      return data_->full_freq_caps;
    }

  private:
    DataPtr data_;
  };

  typedef OptionalValue<AdSelectProps> AdSelectPropsOptional;

  class AdRequestProps
  {
  private:
    typedef StringIoWrapperOptional OptionalStringT;

    typedef Aux_::StringIoWrapper SizeT;

    typedef std::list<SizeT> SizeListT;

    class Data: public ReferenceCounting::AtomicImpl
    {
    public:
      Data() noexcept
      :
        sizes(),
        country_code(),
        max_text_ads(),
        text_ad_cost_threshold(FixedNum::ZERO),
        display_ad_shown(),
        text_ad_shown(),
        ad_select()
      {
      }

      bool operator==(const Data& rhs) const
      {
        return &rhs == this ||
          (sizes == rhs.sizes &&
          country_code == rhs.country_code &&
          max_text_ads == rhs.max_text_ads &&
          text_ad_cost_threshold == rhs.text_ad_cost_threshold &&
          display_ad_shown == rhs.display_ad_shown &&
          text_ad_shown == rhs.text_ad_shown &&
          ad_select == rhs.ad_select);
      }

      bool empty() const
      {
        return sizes.empty() &&
          !country_code.present() &&
          !max_text_ads &&
          text_ad_cost_threshold == FixedNum::ZERO &&
          !display_ad_shown.present() &&
          text_ad_shown.empty() &&
          !ad_select.present();
      }

      SizeListT sizes;
      OptionalStringT country_code;
      unsigned long max_text_ads;
      FixedNum text_ad_cost_threshold;
      AdSlotImpressionOptional display_ad_shown;
      AdBidSlotImpressionList text_ad_shown;
      AdSelectPropsOptional ad_select;

    protected:
      virtual
      ~Data() noexcept = default;
    };

    typedef ReferenceCounting::SmartPtr<Data> DataPtr;

  public:
    AdRequestProps() noexcept {}

    AdRequestProps(
      const StringList& sizes,
      const std::string& country_code,
      unsigned long max_text_ads,
      const FixedNum& text_ad_cost_threshold,
      const AdSlotImpressionOptional& display_ad_shown,
      const AdBidSlotImpressionList& text_ad_shown,
      const AdSelectPropsOptional& ad_select
    )
    :
      data_(new Data)
    {
      data_->sizes.assign(sizes.begin(), sizes.end());
      data_->country_code = country_code;
      data_->max_text_ads = max_text_ads;
      data_->text_ad_cost_threshold = text_ad_cost_threshold;
      data_->display_ad_shown = display_ad_shown;
      data_->text_ad_shown = text_ad_shown;
      data_->ad_select = ad_select;
    }

    AdRequestProps(
      const RequestBasicChannelsInnerData_V_2_7::AdRequestProps& data
    )
    :
      data_(new Data)
    {
      data_->sizes.push_back(data.data_->size);
      data_->country_code = data.data_->country_code;
      data_->max_text_ads = data.data_->max_text_ads;
      data_->text_ad_cost_threshold = data.data_->text_ad_cost_threshold;
      data_->display_ad_shown = data.data_->display_ad_shown;
      data_->text_ad_shown = data.data_->text_ad_shown;
    }

    AdRequestProps(
      const RequestBasicChannelsInnerData_V_3_1::AdRequestProps& data
    )
    :
      data_(new Data)
    {
      data_->sizes.push_back(data.size());
      data_->country_code = data.country_code();
      data_->max_text_ads = data.max_text_ads();
      data_->text_ad_cost_threshold = data.text_ad_cost_threshold();
      data_->display_ad_shown = data.display_ad_shown();
      data_->text_ad_shown = data.text_ad_shown();

      if (data.ad_select().present())
      {
        data_->ad_select = AdSelectProps(
          data.ad_select().get().tag_id(),
          data.size(), // NOTE: size value is taken from AdRequestProps::size
          data.ad_select().get().format(),
          data.ad_select().get().test_request(),
          data.ad_select().get().profiling_available(),
          data.ad_select().get().full_freq_caps()
        );
      }
    }

    bool operator==(const AdRequestProps& rhs) const
    {
      return &rhs == this || data_ == rhs.data_ || *data_ == *rhs.data_;
    }

    bool empty() const
    {
      return data_->empty();
    }

    const SizeListT& sizes() const
    {
      return data_->sizes;
    }

    const std::string& country_code() const
    {
      return data_->country_code.get();
    }

    unsigned long max_text_ads() const
    {
      return data_->max_text_ads;
    }

    const FixedNum& text_ad_cost_threshold() const
    {
      return data_->text_ad_cost_threshold;
    }

    const AdSlotImpressionOptional& display_ad_shown() const
    {
      return data_->display_ad_shown;
    }

    const AdBidSlotImpressionList& text_ad_shown() const
    {
      return data_->text_ad_shown;
    }

    const AdSelectPropsOptional& ad_select() const
    {
      return data_->ad_select;
    }

    void normalize()
    {
      if (data_->country_code.get().size() > max_country_code_size_)
      {
        data_->country_code.get().resize(max_country_code_size_);
      }
    }

    friend FixedBufStream<TabCategory>&
    operator>>(FixedBufStream<TabCategory>& is, AdRequestProps& value);

    friend std::ostream&
    operator<<(std::ostream& os, const AdRequestProps& ad_req);

  private:
    static const size_t max_country_code_size_ = 2;

    DataPtr data_;
  };

  typedef OptionalValue<AdRequestProps> AdRequestPropsOptional;

  class Match
  {
    class Data: public ReferenceCounting::AtomicImpl
    {
    public:
      Data()
      :
        history_channels(),
        page_trigger_channels(),
        search_trigger_channels(),
        url_trigger_channels(),
        url_keyword_trigger_channels()
      {
      }

      Data(
        const NumberList& history_channels_val,
        const TriggerMatchList& page_trigger_channels_val,
        const TriggerMatchList& search_trigger_channels_val,
        const TriggerMatchList& url_trigger_channels_val,
        const TriggerMatchList& url_keyword_trigger_channels_val
        )
        : history_channels(history_channels_val),
          page_trigger_channels(page_trigger_channels_val),
          search_trigger_channels(search_trigger_channels_val),
          url_trigger_channels(url_trigger_channels_val),
          url_keyword_trigger_channels(url_keyword_trigger_channels_val)
      {}

      bool operator==(const Data& rhs) const
      {
        return &rhs == this ||
          (history_channels == rhs.history_channels &&
          page_trigger_channels == rhs.page_trigger_channels &&
          search_trigger_channels == rhs.search_trigger_channels &&
          url_trigger_channels == rhs.url_trigger_channels &&
          url_keyword_trigger_channels == rhs.url_keyword_trigger_channels);
      }

      bool empty() const
      {
        return history_channels.empty() &&
          page_trigger_channels.empty() &&
          search_trigger_channels.empty() &&
          url_trigger_channels.empty() &&
          url_keyword_trigger_channels.empty();
      }

      NumberList history_channels;
      TriggerMatchList page_trigger_channels;
      TriggerMatchList search_trigger_channels;
      TriggerMatchList url_trigger_channels;
      TriggerMatchList url_keyword_trigger_channels;

    protected:
      virtual
      ~Data() noexcept = default;
    };

    typedef ReferenceCounting::SmartPtr<Data> DataPtr;

  public:
    Match() noexcept {}

    Match(
      const NumberList& history_channels,
      const TriggerMatchList& page_trigger_channels,
      const TriggerMatchList& search_trigger_channels,
      const TriggerMatchList& url_trigger_channels,
      const TriggerMatchList& url_keyword_trigger_channels
      )
      : data_(new Data(
          history_channels,
          page_trigger_channels,
          search_trigger_channels,
          url_trigger_channels,
          url_keyword_trigger_channels))
    {}

    bool operator==(const Match& rhs) const
    {
      return &rhs == this || data_ == rhs.data_ || *data_ == *rhs.data_;
    }

    bool empty() const
    {
      return data_->empty();
    }

    const NumberList& history_channels() const
    {
      return data_->history_channels;
    }

    const TriggerMatchList& page_trigger_channels() const
    {
      return data_->page_trigger_channels;
    }

    const TriggerMatchList& search_trigger_channels() const
    {
      return data_->search_trigger_channels;
    }

    const TriggerMatchList& url_trigger_channels() const
    {
      return data_->url_trigger_channels;
    }

    const TriggerMatchList& url_keyword_trigger_channels() const
    {
      return data_->url_keyword_trigger_channels;
    }

    friend FixedBufStream<TabCategory>&
    operator>>(FixedBufStream<TabCategory>& is, Match& match);

    friend std::ostream&
    operator<<(std::ostream& os, const Match& match);

  protected:
    friend class RequestBasicChannelsInnerData;
    DataPtr data_;
  };

  typedef OptionalValue<Match> MatchOptional;

  RequestBasicChannelsInnerData_V_3_3() noexcept {}

  RequestBasicChannelsInnerData_V_3_3(
    char user_type,
    const UserId& user_id,
    const UserId& temporary_user_id,
    const MatchOptional& match_request,
    const AdRequestPropsOptional& ad_request
  )
  :
    holder_(
      new DataHolder(
        user_type,
        user_id,
        temporary_user_id,
        match_request,
        ad_request,
        Generics::safe_rand()
      )
    )
  {
  }

  RequestBasicChannelsInnerData_V_3_3(
    const RequestBasicChannelsInnerData_V_2_7& data
  )
  :
    holder_(
      new DataHolder(
        data.user_type(),
        data.user_id(),
        data.temporary_user_id(),
        data.history_channels(),
        data.page_trigger_channels(),
        data.search_trigger_channels(),
        data.url_trigger_channels(),
        TriggerMatchList(), // url_keyword_trigger_channels
        data.ad_request(),
        Generics::safe_rand()
      )
    )
  {
  }

  RequestBasicChannelsInnerData_V_3_3(
    const RequestBasicChannelsInnerData_V_3_1& data
  )
  :
    holder_(
      new DataHolder(
        data.user_type(),
        data.user_id(),
        data.temporary_user_id(),
        NumberList(data.history_channels().begin(), data.history_channels().end()),
        data.page_trigger_channels(),
        data.search_trigger_channels(),
        data.url_trigger_channels(),
        TriggerMatchList(), // url_keyword_trigger_channels
        data.ad_request(),
        Generics::safe_rand()
      )
    )
  {
  }

  bool operator==(const RequestBasicChannelsInnerData_V_3_3& data) const
  {
    return this == &data || holder_.in() == data.holder_.in() ||
      (holder_->user_type == data.holder_->user_type &&
      holder_->user_id == data.holder_->user_id &&
      holder_->temporary_user_id == data.holder_->temporary_user_id &&
      holder_->match_request == data.holder_->match_request &&
      holder_->ad_request == data.holder_->ad_request);
  }

  char user_type() const
  {
    return holder_->user_type;
  }

  const UserId& user_id() const
  {
    return holder_->user_id;
  }

  const UserId& temporary_user_id() const
  {
    return holder_->temporary_user_id;
  }

  const MatchOptional& match_request() const
  {
    return holder_->match_request;
  }

  const AdRequestPropsOptional& ad_request() const
  {
    return holder_->ad_request;
  }

  unsigned long distrib_hash() const
  {
    using AdServer::Commons::uuid_distribution_hash;
    if (holder_->user_id.is_null())
    {
      if (holder_->temporary_user_id.is_null())
      {
        return holder_->random;
      }
      return uuid_distribution_hash(holder_->temporary_user_id);
    }
    return uuid_distribution_hash(holder_->user_id);
  }

  friend std::istream&
  operator>>(std::istream& is, RequestBasicChannelsInnerData_V_3_3& data)
    /*throw(eh::Exception)*/;

  friend std::ostream&
  operator<<(std::ostream& os, const RequestBasicChannelsInnerData_V_3_3& data)
    /*throw(eh::Exception)*/;

private:
  class DataHolder: public ReferenceCounting::AtomicImpl
  {
  public:
    DataHolder()
    :
      user_type(),
      user_id(),
      temporary_user_id(),
      match_request(),
      ad_request(),
      random()
    {
    }

    DataHolder(
      char user_type_val,
      const UserId& user_id_val,
      const UserId& temporary_user_id_val,
      const MatchOptional& match_request_val,
      const AdRequestPropsOptional& ad_request_val,
      unsigned long random_val
    )
    :
      user_type(user_type_val),
      user_id(user_id_val),
      temporary_user_id(temporary_user_id_val),
      match_request(match_request_val),
      ad_request(ad_request_val),
      random(random_val)
    {
      if (ad_request.present())
      {
        ad_request.get().normalize();
      }
    }

    DataHolder(
      char user_type_val,
      const UserId& user_id_val,
      const UserId& temporary_user_id_val,
      const NumberList& history_channels_val,
      const TriggerMatchList& page_trigger_channels_val,
      const TriggerMatchList& search_trigger_channels_val,
      const TriggerMatchList& url_trigger_channels_val,
      const TriggerMatchList& url_keyword_trigger_channels_val,
      const AdRequestPropsOptional& ad_request_val,
      unsigned long random_val
    )
    :
      user_type(user_type_val),
      user_id(user_id_val),
      temporary_user_id(temporary_user_id_val),
      match_request(Match(history_channels_val, page_trigger_channels_val,
        search_trigger_channels_val, url_trigger_channels_val,
          url_keyword_trigger_channels_val)),
      ad_request(ad_request_val),
      random(random_val)
    {
      if (ad_request.present())
      {
        ad_request.get().normalize();
      }
    }

    DataHolder(
      char user_type_val,
      const UserId& user_id_val,
      const UserId& temporary_user_id_val,
      const NumberList& history_channels_val,
      const TriggerMatchList& page_trigger_channels_val,
      const TriggerMatchList& search_trigger_channels_val,
      const TriggerMatchList& url_trigger_channels_val,
      const TriggerMatchList& url_keyword_trigger_channels_val,
      const RequestBasicChannelsInnerData_V_2_7::AdRequestPropsOptional&
        ad_request_val,
      unsigned long random_val
    )
    :
      user_type(user_type_val),
      user_id(user_id_val),
      temporary_user_id(temporary_user_id_val),
      match_request(Match(history_channels_val, page_trigger_channels_val,
        search_trigger_channels_val, url_trigger_channels_val,
          url_keyword_trigger_channels_val)),
      ad_request(ad_request_val),
      random(random_val)
    {
      if (ad_request.present())
      {
        ad_request.get().normalize();
      }
    }

    DataHolder(
      char user_type_val,
      const UserId& user_id_val,
      const UserId& temporary_user_id_val,
      const NumberList& history_channels_val,
      const TriggerMatchList& page_trigger_channels_val,
      const TriggerMatchList& search_trigger_channels_val,
      const TriggerMatchList& url_trigger_channels_val,
      const TriggerMatchList& url_keyword_trigger_channels_val,
      const RequestBasicChannelsInnerData_V_3_1::AdRequestPropsOptional&
        ad_request_val,
      unsigned long random_val
    )
    :
      user_type(user_type_val),
      user_id(user_id_val),
      temporary_user_id(temporary_user_id_val),
      match_request(Match(history_channels_val, page_trigger_channels_val,
        search_trigger_channels_val, url_trigger_channels_val,
          url_keyword_trigger_channels_val)),
      ad_request(ad_request_val),
      random(random_val)
    {
      if (ad_request.present())
      {
        ad_request.get().normalize();
      }
    }

    template <class ARCHIVE_>
    void serialize(ARCHIVE_& ar)
    {
      ar & user_type;
      ar & user_id;
      ar & temporary_user_id;
      ar & match_request;
      ar ^ ad_request;
    }

    void invariant() const /*throw(ConstraintViolation)*/
    {
      if (!user_type_is_valid_())
      {
        Stream::Error es;
        es << "RequestBasicChannelsInnerData::DataHolder::invariant(): "
          "user_type has invalid value '" << user_type << '\'';
        throw ConstraintViolation(es);
      }
    }

    char user_type;
    UserId user_id;
    UserId temporary_user_id;
    MatchOptional match_request;
    AdRequestPropsOptional ad_request;
    unsigned long random;

  private:
    virtual
    ~DataHolder() noexcept = default;

    bool user_type_is_valid_() const
    {
      return user_type == 'H' || user_type == 'P' || user_type == 'A';
    }
  };

  typedef ReferenceCounting::SmartPtr<DataHolder> DataHolder_var;

  DataHolder_var holder_;
};


class RequestBasicChannelsInnerData
{
public:
  typedef RequestBasicChannelsInnerData_V_3_3::FixedNum FixedNum;

  typedef RequestBasicChannelsInnerData_V_3_3::TriggerMatch TriggerMatch;

  typedef RequestBasicChannelsInnerData_V_3_3::TriggerMatchList
    TriggerMatchList;

  typedef RequestBasicChannelsInnerData_V_3_3::AdSlotImpression
    AdSlotImpression;

  typedef RequestBasicChannelsInnerData_V_3_3::AdSlotImpressionOptional
    AdSlotImpressionOptional;

  typedef RequestBasicChannelsInnerData_V_3_3::AdBidSlotImpression
    AdBidSlotImpression;

  typedef RequestBasicChannelsInnerData_V_3_3::AdBidSlotImpressionList
    AdBidSlotImpressionList;

  class AdSelectProps
  {
    class Data: public ReferenceCounting::AtomicImpl
    {
    public:
      Data() noexcept {}

      Data(
        unsigned long tag_id_val,
        const std::string& size_val,
        const std::string& format_val,
        bool test_request_val,
        bool profiling_available_val
      )
        noexcept
      :
        tag_id(tag_id_val),
        size(size_val),
        format(format_val),
        test_request(test_request_val),
        profiling_available(profiling_available_val)
      {
      }

      bool
      operator==(const Data& rhs) const
      {
        return &rhs == this ||
          (tag_id == rhs.tag_id &&
          size == rhs.size &&
          format == rhs.format &&
          test_request == rhs.test_request &&
          profiling_available == rhs.profiling_available &&
          full_freq_caps == rhs.full_freq_caps);
      }

      unsigned long tag_id;
      SpacesMarksString size;
      SpacesMarksString format;
      bool test_request;
      bool profiling_available;
      NumberList full_freq_caps;

    protected:
      virtual
      ~Data() noexcept = default;
    };

    typedef ReferenceCounting::SmartPtr<Data> DataPtr;

    friend FixedBufStream<TabCategory>&
    operator>>(FixedBufStream<TabCategory>&, AdSelectProps&)
      /*throw(eh::Exception)*/;

    friend std::ostream&
    operator<<(std::ostream& os, const AdSelectProps& ad_select_props)
      /*throw(eh::Exception)*/;

  public:
    AdSelectProps() noexcept {}

    template <typename FullFreqCapsContainerType>
    AdSelectProps(
      unsigned long tag_id,
      const std::string& size,
      const std::string& format,
      bool test_request,
      bool profiling_available,
      const FullFreqCapsContainerType& full_freq_caps
    )
    :
      data_(new Data(tag_id, size, format, test_request, profiling_available))
    {
      data_->full_freq_caps.assign(full_freq_caps.begin(),
        full_freq_caps.end());
    }

    bool
    operator==(const AdSelectProps& rhs) const
    {
      return &rhs == this || data_ == rhs.data_ || *data_ == *rhs.data_;
    }

    unsigned long
    tag_id() const
    {
      return data_->tag_id;
    }

    const std::string&
    size() const
    {
      return data_->size;
    }

    const std::string&
    format() const
    {
      return data_->format;
    }

    bool
    test_request() const
    {
      return data_->test_request;
    }

    bool
    profiling_available() const
    {
      return data_->profiling_available;
    }

    const NumberList&
    full_freq_caps() const
    {
      return data_->full_freq_caps;
    }

  private:
    DataPtr data_;
  };

  typedef OptionalValue<AdSelectProps> AdSelectPropsOptional;

  class AdRequestProps
  {
  private:
    typedef StringIoWrapperOptional OptionalStringT;

    typedef Aux_::StringIoWrapper SizeT;

    typedef std::list<SizeT> SizeListT;

    class Data: public ReferenceCounting::AtomicImpl
    {
    public:
      Data() noexcept
      :
        sizes(),
        country_code(),
        max_text_ads(),
        text_ad_cost_threshold(FixedNum::ZERO),
        display_ad_shown(),
        text_ad_shown(),
        ad_select(),
        auction_type(CampaignSvcs::AT_MAX_ECPM)
      {
      }

      bool operator==(const Data& rhs) const
      {
        return &rhs == this ||
          (sizes == rhs.sizes &&
          country_code == rhs.country_code &&
          max_text_ads == rhs.max_text_ads &&
          text_ad_cost_threshold == rhs.text_ad_cost_threshold &&
          display_ad_shown == rhs.display_ad_shown &&
          text_ad_shown == rhs.text_ad_shown &&
          ad_select == rhs.ad_select &&
          auction_type == rhs.auction_type);
      }

      bool empty() const
      {
        return sizes.empty() &&
          !country_code.present() &&
          !max_text_ads &&
          text_ad_cost_threshold == FixedNum::ZERO &&
          !display_ad_shown.present() &&
          text_ad_shown.empty() &&
          !ad_select.present();
      }

      SizeListT sizes;
      OptionalStringT country_code;
      unsigned long max_text_ads;
      FixedNum text_ad_cost_threshold;
      AdSlotImpressionOptional display_ad_shown;
      AdBidSlotImpressionList text_ad_shown;
      AdSelectPropsOptional ad_select;
      CampaignSvcs::AuctionType auction_type;

    protected:
      virtual
      ~Data() noexcept = default;
    };

    typedef ReferenceCounting::SmartPtr<Data> DataPtr;

  public:
    AdRequestProps() noexcept {}

    AdRequestProps(
      const StringList& sizes,
      const std::string& country_code,
      unsigned long max_text_ads,
      const FixedNum& text_ad_cost_threshold,
      const AdSlotImpressionOptional& display_ad_shown,
      const AdBidSlotImpressionList& text_ad_shown,
      const AdSelectPropsOptional& ad_select,
      CampaignSvcs::AuctionType auction_type
    )
    :
      data_(new Data)
    {
      data_->sizes.assign(sizes.begin(), sizes.end());
      data_->country_code = country_code;
      data_->max_text_ads = max_text_ads;
      data_->text_ad_cost_threshold = text_ad_cost_threshold;
      data_->display_ad_shown = display_ad_shown;
      data_->text_ad_shown = text_ad_shown;
      data_->ad_select = ad_select;
      data_->auction_type = auction_type;
    }

    AdRequestProps(
      const RequestBasicChannelsInnerData_V_2_7::AdRequestProps& data
    )
    :
      data_(new Data)
    {
      data_->sizes.push_back(data.data_->size);
      data_->country_code = data.data_->country_code;
      data_->max_text_ads = data.data_->max_text_ads;
      data_->text_ad_cost_threshold = data.data_->text_ad_cost_threshold;
      data_->display_ad_shown = data.data_->display_ad_shown;
      data_->text_ad_shown = data.data_->text_ad_shown;
      data_->auction_type = CampaignSvcs::AT_MAX_ECPM;
    }

    AdRequestProps(
      const RequestBasicChannelsInnerData_V_3_1::AdRequestProps& data
    )
    :
      data_(new Data)
    {
      data_->sizes.push_back(data.size());
      data_->country_code = data.country_code();
      data_->max_text_ads = data.max_text_ads();
      data_->text_ad_cost_threshold = data.text_ad_cost_threshold();
      data_->display_ad_shown = data.display_ad_shown();
      data_->text_ad_shown = data.text_ad_shown();
      data_->auction_type = CampaignSvcs::AT_MAX_ECPM;

      if (data.ad_select().present())
      {
        data_->ad_select = AdSelectProps(
          data.ad_select().get().tag_id(),
          data.size(), // NOTE: size value is taken from AdRequestProps::size
          data.ad_select().get().format(),
          data.ad_select().get().test_request(),
          data.ad_select().get().profiling_available(),
          data.ad_select().get().full_freq_caps()
        );
      }
    }

    AdRequestProps(
      const RequestBasicChannelsInnerData_V_3_3::AdRequestProps& data
    )
    :
      data_(new Data)
    {
      data_->sizes.insert(data_->sizes.end(), data.sizes().begin(), data.sizes().end());
      data_->country_code = data.country_code();
      data_->max_text_ads = data.max_text_ads();
      data_->text_ad_cost_threshold = data.text_ad_cost_threshold();
      data_->display_ad_shown = data.display_ad_shown();
      data_->text_ad_shown = data.text_ad_shown();
      data_->auction_type = CampaignSvcs::AT_MAX_ECPM;

      if (data.ad_select().present())
      {
        data_->ad_select = AdSelectProps(
          data.ad_select().get().tag_id(),
          data.ad_select().get().size(),
          data.ad_select().get().format(),
          data.ad_select().get().test_request(),
          data.ad_select().get().profiling_available(),
          data.ad_select().get().full_freq_caps()
        );
      }
    }

    bool operator==(const AdRequestProps& rhs) const
    {
      return &rhs == this || data_ == rhs.data_ || *data_ == *rhs.data_;
    }

    bool empty() const
    {
      return data_->empty();
    }

    const SizeListT& sizes() const
    {
      return data_->sizes;
    }

    const std::string& country_code() const
    {
      return data_->country_code.get();
    }

    unsigned long max_text_ads() const
    {
      return data_->max_text_ads;
    }

    const FixedNum& text_ad_cost_threshold() const
    {
      return data_->text_ad_cost_threshold;
    }

    const AdSlotImpressionOptional& display_ad_shown() const
    {
      return data_->display_ad_shown;
    }

    const AdBidSlotImpressionList& text_ad_shown() const
    {
      return data_->text_ad_shown;
    }

    const AdSelectPropsOptional& ad_select() const
    {
      return data_->ad_select;
    }

    CampaignSvcs::AuctionType auction_type() const
    {
      return data_->auction_type;
    }

    void normalize()
    {
      if (data_->country_code.get().size() > max_country_code_size_)
      {
        data_->country_code.get().resize(max_country_code_size_);
      }
    }

    friend FixedBufStream<TabCategory>&
    operator>>(FixedBufStream<TabCategory>& is, AdRequestProps& value);

    friend std::ostream&
    operator<<(std::ostream& os, const AdRequestProps& ad_req);

  private:
    static const size_t max_country_code_size_ = 2;

    DataPtr data_;
  };

  typedef OptionalValue<AdRequestProps> AdRequestPropsOptional;

  class Match
  {
    class Data: public ReferenceCounting::AtomicImpl
    {
    public:
      Data()
      :
        history_channels(),
        page_trigger_channels(),
        search_trigger_channels(),
        url_trigger_channels(),
        url_keyword_trigger_channels()
      {
      }

      Data(
        NumberArray history_channels_val,
        TriggerMatchList page_trigger_channels_val,
        TriggerMatchList search_trigger_channels_val,
        TriggerMatchList url_trigger_channels_val,
        TriggerMatchList url_keyword_trigger_channels_val
        )
        : history_channels(std::move(history_channels_val)),
          page_trigger_channels(std::move(page_trigger_channels_val)),
          search_trigger_channels(std::move(search_trigger_channels_val)),
          url_trigger_channels(std::move(url_trigger_channels_val)),
          url_keyword_trigger_channels(std::move(url_keyword_trigger_channels_val))
      {}

      bool operator==(const Data& rhs) const
      {
        return &rhs == this ||
          (history_channels == rhs.history_channels &&
          page_trigger_channels == rhs.page_trigger_channels &&
          search_trigger_channels == rhs.search_trigger_channels &&
          url_trigger_channels == rhs.url_trigger_channels &&
          url_keyword_trigger_channels == rhs.url_keyword_trigger_channels);
      }

      bool empty() const
      {
        return history_channels.empty() &&
          page_trigger_channels.empty() &&
          search_trigger_channels.empty() &&
          url_trigger_channels.empty() &&
          url_keyword_trigger_channels.empty();
      }

      NumberArray history_channels;
      TriggerMatchList page_trigger_channels;
      TriggerMatchList search_trigger_channels;
      TriggerMatchList url_trigger_channels;
      TriggerMatchList url_keyword_trigger_channels;

    protected:
      virtual
      ~Data() noexcept = default;
    };

    typedef ReferenceCounting::SmartPtr<Data> DataPtr;

  public:
    Match() noexcept {}

    Match(
      NumberArray history_channels,
      TriggerMatchList page_trigger_channels,
      TriggerMatchList search_trigger_channels,
      TriggerMatchList url_trigger_channels,
      TriggerMatchList url_keyword_trigger_channels
      )
      : data_(
          new Data(
            std::move(history_channels),
            std::move(page_trigger_channels),
            std::move(search_trigger_channels),
            std::move(url_trigger_channels),
            std::move(url_keyword_trigger_channels)))
    {}

    Match(const RequestBasicChannelsInnerData_V_3_3::Match& match)
    :
      data_(
        new Data(
          NumberArray(match.data_->history_channels.begin(), match.data_->history_channels.end()),
          match.data_->page_trigger_channels,
          match.data_->search_trigger_channels,
          match.data_->url_trigger_channels,
          match.data_->url_keyword_trigger_channels))
    {
    }

    bool operator==(const Match& rhs) const
    {
      return &rhs == this || data_ == rhs.data_ || *data_ == *rhs.data_;
    }

    bool empty() const
    {
      return data_->empty();
    }

    const NumberArray& history_channels() const
    {
      return data_->history_channels;
    }

    const TriggerMatchList& page_trigger_channels() const
    {
      return data_->page_trigger_channels;
    }

    const TriggerMatchList& search_trigger_channels() const
    {
      return data_->search_trigger_channels;
    }

    const TriggerMatchList& url_trigger_channels() const
    {
      return data_->url_trigger_channels;
    }

    const TriggerMatchList& url_keyword_trigger_channels() const
    {
      return data_->url_keyword_trigger_channels;
    }

    friend FixedBufStream<TabCategory>&
    operator>>(FixedBufStream<TabCategory>& is, Match& match);

    friend std::ostream&
    operator<<(std::ostream& os, const Match& match);

  private:
    DataPtr data_;
  };

  typedef OptionalValue<Match> MatchOptional;

  RequestBasicChannelsInnerData() noexcept {}

  RequestBasicChannelsInnerData(
    char user_type,
    const UserId& user_id,
    const UserId& temporary_user_id,
    MatchOptional&& match_request,
    AdRequestPropsOptional&& ad_request
    )
  :
    holder_(
      new DataHolder(
        user_type,
        user_id,
        temporary_user_id,
        match_request,
        ad_request,
        Generics::safe_rand()
      )
    )
  {
  }

  RequestBasicChannelsInnerData(
    const RequestBasicChannelsInnerData_V_3_3& data
  )
  :
    holder_(
      new DataHolder(
        data.user_type(),
        data.user_id(),
        data.temporary_user_id(),
        data.match_request(),
        data.ad_request(),
        Generics::safe_rand()
      )
    )
  {
  }

  template <class RequestBasicChannelsInnerData_ANY>
  RequestBasicChannelsInnerData(
    const RequestBasicChannelsInnerData_ANY& data
  )
  :
    holder_(
      new DataHolder(
        data.user_type(),
        data.user_id(),
        data.temporary_user_id(),
        NumberArray(data.history_channels().begin(), data.history_channels().end()),
        data.page_trigger_channels(),
        data.search_trigger_channels(),
        data.url_trigger_channels(),
        TriggerMatchList(), // url_keyword_trigger_channels
        data.ad_request(),
        Generics::safe_rand()
      )
    )
  {
  }

  bool operator==(const RequestBasicChannelsInnerData& data) const
  {
    return this == &data || holder_.in() == data.holder_.in() ||
      (holder_->user_type == data.holder_->user_type &&
      holder_->user_id == data.holder_->user_id &&
      holder_->temporary_user_id == data.holder_->temporary_user_id &&
      holder_->match_request == data.holder_->match_request &&
      holder_->ad_request == data.holder_->ad_request);
  }

  char user_type() const
  {
    return holder_->user_type;
  }

  const UserId& user_id() const
  {
    return holder_->user_id;
  }

  const UserId& temporary_user_id() const
  {
    return holder_->temporary_user_id;
  }

  const MatchOptional& match_request() const
  {
    return holder_->match_request;
  }

  const AdRequestPropsOptional& ad_request() const
  {
    return holder_->ad_request;
  }

  unsigned long distrib_hash() const
  {
    using AdServer::Commons::uuid_distribution_hash;
    if (holder_->user_id.is_null())
    {
      if (holder_->temporary_user_id.is_null())
      {
        return holder_->random;
      }
      return uuid_distribution_hash(holder_->temporary_user_id);
    }
    return uuid_distribution_hash(holder_->user_id);
  }

  // FixedBufStream, getline already used inside.No need optimize it here.
  // TODO: possible rewrite to use SeqCollector<TypeName, USE_BUF_STREAM_ = true>
  friend std::istream&
  operator>>(std::istream& is, RequestBasicChannelsInnerData& data)
    /*throw(eh::Exception)*/;

  friend std::ostream&
  operator<<(std::ostream& os, const RequestBasicChannelsInnerData& data)
    /*throw(eh::Exception)*/;

private:
  class DataHolder: public ReferenceCounting::AtomicImpl
  {
  public:
    DataHolder()
    :
      user_type(),
      user_id(),
      temporary_user_id(),
      match_request(),
      ad_request(),
      random()
    {
    }

    DataHolder(
      char user_type_val,
      const UserId& user_id_val,
      const UserId& temporary_user_id_val,
      const MatchOptional& match_request_val,
      const AdRequestPropsOptional& ad_request_val,
      unsigned long random_val
    )
    :
      user_type(user_type_val),
      user_id(user_id_val),
      temporary_user_id(temporary_user_id_val),
      match_request(match_request_val),
      ad_request(ad_request_val),
      random(random_val)
    {
      if (ad_request.present())
      {
        ad_request.get().normalize();
      }
    }

    template <class AdRequestPropsOptional_ANY>
    DataHolder(
      char user_type_val,
      const UserId& user_id_val,
      const UserId& temporary_user_id_val,
      const NumberArray& history_channels_val,
      const TriggerMatchList& page_trigger_channels_val,
      const TriggerMatchList& search_trigger_channels_val,
      const TriggerMatchList& url_trigger_channels_val,
      const TriggerMatchList& url_keyword_trigger_channels_val,
      const AdRequestPropsOptional_ANY& ad_request_val,
      unsigned long random_val
    )
    :
      user_type(user_type_val),
      user_id(user_id_val),
      temporary_user_id(temporary_user_id_val),
      match_request(Match(history_channels_val, page_trigger_channels_val,
        search_trigger_channels_val, url_trigger_channels_val,
          url_keyword_trigger_channels_val)),
      ad_request(ad_request_val),
      random(random_val)
    {
      if (ad_request.present())
      {
        ad_request.get().normalize();
      }
    }

    template <class ARCHIVE_>
    void serialize(ARCHIVE_& ar)
    {
      ar & user_type;
      ar & user_id;
      ar & temporary_user_id;
      ar & match_request;
      ar ^ ad_request;
    }

    void invariant() const /*throw(ConstraintViolation)*/
    {
      if (!user_type_is_valid_())
      {
        Stream::Error es;
        es << "RequestBasicChannelsInnerData::DataHolder::invariant(): "
          "user_type has invalid value '" << user_type << '\'';
        throw ConstraintViolation(es);
      }
    }

    char user_type;
    UserId user_id;
    UserId temporary_user_id;
    MatchOptional match_request;
    AdRequestPropsOptional ad_request;
    unsigned long random;

  private:
    virtual
    ~DataHolder() noexcept = default;

    bool user_type_is_valid_() const
    {
      return user_type == 'H' || user_type == 'P' || user_type == 'A';
    }
  };

  typedef ReferenceCounting::SmartPtr<DataHolder> DataHolder_var;

  DataHolder_var holder_;
};

template <typename Key, typename InnerData>
struct RBC
{
  typedef SeqCollector<InnerData> InnerCollectorType;
  typedef StatCollector<Key, InnerCollectorType, false, false, true, true>
    Type;
};

typedef RBC<RequestBasicChannelsKey, RequestBasicChannelsInnerData_V_2_7>::Type
  RequestBasicChannelsCollector_V_2_7;

typedef RBC<RequestBasicChannelsKey, RequestBasicChannelsInnerData_V_3_1>::Type
  RequestBasicChannelsCollector_V_3_1;

typedef RBC<RequestBasicChannelsKey, RequestBasicChannelsInnerData_V_3_3>::Type
  RequestBasicChannelsCollector_V_3_3;

typedef RBC<RequestBasicChannelsKey, RequestBasicChannelsInnerData> LastRBC;

typedef LastRBC::Type RequestBasicChannelsCollector;
typedef LastRBC::InnerCollectorType RequestBasicChannelsInnerCollector;

struct RequestBasicChannelsTraits:
  LogDefaultTraits<RequestBasicChannelsCollector, true, false>
{
  template <class FUNCTOR_>
  static void for_each_old(FUNCTOR_& f) /*throw(eh::Exception)*/
  {
    // second template parameter specifies whether to use a packed loader:
    // false - use non-packed loader implementation
    // true  - use packed loader implementation
    f.template operator()<RequestBasicChannelsCollector_V_2_7, true>("2.7");
    f.template operator()<RequestBasicChannelsCollector_V_3_1, true>("3.1");
    f.template operator()<RequestBasicChannelsCollector_V_3_3, true>("3.3");
  }

  typedef GenericLogIoHelperImpl<RequestBasicChannelsTraits> IoHelperType;
};

} // namespace LogProcessing
} // namespace AdServer

#endif /* AD_SERVER_LOG_PROCESSING_REQUEST_BASIC_CHANNELS_HPP */

