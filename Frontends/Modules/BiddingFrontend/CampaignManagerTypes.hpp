#pragma once

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <ostream>
#include <string>
#include <vector>

#include <Commons/GrpcAlgs.hpp>
#include <String/SubString.hpp>

namespace AdServer::Bidding::CampaignManager
{
  struct ByteField
  {
    std::string value;

    std::size_t length() const noexcept
    {
      return value.size();
    }

    void length(std::size_t size)
    {
      value.resize(size);
    }

    unsigned char* get_buffer() noexcept
    {
      return reinterpret_cast<unsigned char*>(value.data());
    }

    const unsigned char* get_buffer() const noexcept
    {
      return reinterpret_cast<const unsigned char*>(value.data());
    }

    bool empty() const noexcept
    {
      return value.empty();
    }

    ByteField& operator=(const std::string& source)
    {
      value = source;
      return *this;
    }

    template<typename OctSeq>
    ByteField& operator=(const OctSeq& source)
    {
      value.assign(
        reinterpret_cast<const char*>(source.get_buffer()),
        reinterpret_cast<const char*>(source.get_buffer()) + source.length());
      return *this;
    }
  };

  struct StringField
  {
    std::string str;

    StringField() = default;
    StringField(const char* value) : str(value ? value : "") {}
    StringField(const std::string& value) : str(value) {}

    const char* in() const noexcept
    {
      return str.c_str();
    }

    char operator[](std::size_t index) const noexcept
    {
      return index < str.size() ? str[index] : '\0';
    }

    StringField& operator=(const char* value)
    {
      str = value ? value : "";
      return *this;
    }

    StringField& operator=(const std::string& value)
    {
      str = value;
      return *this;
    }

    operator const char*() const noexcept
    {
      return str.c_str();
    }

    bool empty() const noexcept
    {
      return str.empty();
    }
  };

  inline std::ostream&
  operator<<(std::ostream& out, const StringField& value)
  {
    return out << value.str;
  }

  inline std::string&
  operator+=(std::string& target, const StringField& value)
  {
    target += value.str;
    return target;
  }

  inline StringField&
  operator<<(StringField& target, const String::SubString& value)
  {
    target.str.assign(value.data(), value.size());
    return target;
  }

  inline StringField&
  operator<<(StringField& target, const char* value)
  {
    target = value;
    return target;
  }

  inline StringField&
  operator<<(StringField& target, const std::string& value)
  {
    target = value;
    return target;
  }

  template<typename T>
  class Sequence
  {
  public:
    using value_type = T;
    using iterator = typename std::vector<T>::iterator;
    using const_iterator = typename std::vector<T>::const_iterator;

    std::size_t length() const noexcept
    {
      return items_.size();
    }

    void length(std::size_t size)
    {
      items_.resize(size);
    }

    T* get_buffer() noexcept
    {
      return items_.data();
    }

    const T* get_buffer() const noexcept
    {
      return items_.data();
    }

    T& operator[](std::size_t index) noexcept
    {
      return items_[index];
    }

    const T& operator[](std::size_t index) const noexcept
    {
      return items_[index];
    }

    iterator begin() noexcept { return items_.begin(); }
    iterator end() noexcept { return items_.end(); }
    const_iterator begin() const noexcept { return items_.begin(); }
    const_iterator end() const noexcept { return items_.end(); }

    template<typename SourceSeq>
    void assign_from(const SourceSeq& source)
    {
      length(source.length());
      for(std::size_t i = 0; i < source.length(); ++i)
      {
        items_[i] = source[i];
      }
    }

    template<typename SourceSeq>
    void swap(SourceSeq& source)
    {
      assign_from(source);
      source.length(0);
    }

  private:
    std::vector<T> items_;
  };

  using IdSeq = Sequence<unsigned long>;
  using StringSeq = Sequence<StringField>;
  using ExternalCreativeCategoryIdSeq = StringSeq;
  using TimestampInfo = ByteField;
  using RequestIdInfo = ByteField;
  using UserIdInfo = ByteField;
  using DecimalInfo = ByteField;

  struct ChannelTriggerMatchInfo
  {
    unsigned long channel_trigger_id = 0;
    unsigned long channel_id = 0;
  };

  using ChannelTriggerMatchSeq = Sequence<ChannelTriggerMatchInfo>;

  struct TriggerMatchResult
  {
    ChannelTriggerMatchSeq url_channels;
    ChannelTriggerMatchSeq pkw_channels;
    ChannelTriggerMatchSeq skw_channels;
    ChannelTriggerMatchSeq ukw_channels;
    IdSeq uid_channels;
  };

  struct CCGKeywordInfo
  {
    unsigned long ccg_keyword_id = 0;
    unsigned long ccg_id = 0;
    unsigned long channel_id = 0;
    DecimalInfo max_cpc;
    DecimalInfo ctr;
    StringField click_url;
    StringField original_keyword;
  };

  using CCGKeywordSeq = Sequence<CCGKeywordInfo>;
  using FreqCapIdSeq = IdSeq;
  using PublisherAccountIdSeq = IdSeq;

  struct TokenInfo
  {
    StringField name;
    StringField value;
  };

  using TokenSeq = Sequence<TokenInfo>;

  struct TokenImageInfo
  {
    StringField name;
    StringField value;
    unsigned long width = 0;
    unsigned long height = 0;
  };

  using TokenImageSeq = Sequence<TokenImageInfo>;

  struct GeoInfo
  {
    StringField country;
    StringField region;
    StringField city;
  };

  using GeoInfoSeq = Sequence<GeoInfo>;

  struct GeoCoordInfo
  {
    DecimalInfo longitude;
    DecimalInfo latitude;
    DecimalInfo accuracy;
  };

  using GeoCoordInfoSeq = Sequence<GeoCoordInfo>;

  struct SeqOrderInfo
  {
    unsigned long ccg_id = 0;
    unsigned long set_id = 0;
    unsigned long imps = 0;
  };

  using SeqOrderSeq = Sequence<SeqOrderInfo>;

  struct CampaignFreq
  {
    unsigned long campaign_id = 0;
    unsigned long imps = 0;

    template<typename Source>
    CampaignFreq& operator=(const Source& source)
    {
      campaign_id = source.campaign_id;
      imps = source.imps;
      return *this;
    }
  };

  using CampaignFreqSeq = Sequence<CampaignFreq>;

  struct NativeDataToken
  {
    StringField name;
    bool required = false;
  };

  using NativeDataTokens = Sequence<NativeDataToken>;

  struct NativeImageToken
  {
    StringField name;
    bool required = false;
    unsigned long width = 0;
    unsigned long height = 0;
  };

  using NativeImageTokens = Sequence<NativeImageToken>;

  struct AdSlotInfo
  {
    unsigned long ad_slot_id = 0;
    StringField format;
    unsigned long tag_id = 0;
    StringSeq sizes;
    StringField ext_tag_id;
    DecimalInfo min_ecpm;
    StringField min_ecpm_currency_code;
    StringSeq currency_codes;
    bool passback = false;
    long up_expand_space = -1;
    long right_expand_space = -1;
    long left_expand_space = -1;
    long tag_visibility = -1;
    long tag_predicted_viewability = -1;
    long down_expand_space = -1;
    unsigned long video_min_duration = 0;
    long video_max_duration = -1;
    long video_skippable_max_duration = -1;
    long video_allow_skippable = 1;
    long video_allow_unskippable = 1;
    unsigned long video_width = 0;
    unsigned long video_height = 0;
    StringSeq exclude_categories;
    StringSeq required_categories;
    unsigned long debug_ccg = 0;
    IdSeq allowed_durations;
    NativeDataTokens native_data_tokens;
    NativeImageTokens native_image_tokens;
    unsigned long native_ads_impression_tracker_type = 0;
    bool fill_track_html = false;
    TokenSeq tokens;
  };

  using AdSlotSeq = Sequence<AdSlotInfo>;

  struct CommonAdRequestInfo
  {
    TimestampInfo time;
    RequestIdInfo request_id;
    StringField creative_instantiate_type;
    unsigned long request_type = 0;
    unsigned long random = 0;
    bool test_request = false;
    bool log_as_test = false;
    unsigned long colo_id = 0;
    StringField external_user_id;
    StringField source_id;
    GeoInfoSeq location;
    GeoCoordInfoSeq coord_location;
    StringField full_referer;
    StringField referer;
    StringSeq urls;
    StringField security_token;
    StringField pub_impr_track_url;
    StringField pub_param;
    StringField preclick_url;
    StringField click_prefix_url;
    StringField original_url;
    UserIdInfo track_user_id;
    UserIdInfo user_id;
    unsigned long user_status = 0;
    StringField signed_user_id;
    StringField peer_ip;
    StringField user_agent;
    StringField cohort;
    unsigned long hpos = 0;
    StringField ext_track_params;
    TokenSeq tokens;
    bool set_cookie = false;
    StringField passback_type;
    StringField passback_url;
  };

  struct ContextAdRequestInfo
  {
    bool enabled_notice = false;
    StringField client;
    StringField client_version;
    IdSeq platform_ids;
    IdSeq geo_channels;
    StringField platform;
    StringField full_platform;
    StringField web_browser;
    StringField ip_hash;
    bool profile_referer = false;
    unsigned long page_load_id = 0;
    unsigned long full_referer_hash = 0;
    unsigned long short_referer_hash = 0;
  };

  struct RequestParams
  {
    CommonAdRequestInfo common_info;
    ContextAdRequestInfo context_info;
    unsigned long publisher_site_id = 0;
    IdSeq publisher_account_ids;
    bool fill_track_pixel = false;
    bool fill_iurl = false;
    unsigned long ad_instantiate_type = 0;
    bool only_display_ad = false;
    FreqCapIdSeq full_freq_caps;
    SeqOrderSeq seq_orders;
    CampaignFreqSeq campaign_freqs;
    UserIdInfo household_id;
    UserIdInfo merged_user_id;
    unsigned long search_engine_id = 0;
    StringField search_words;
    bool page_keywords_present = false;
    bool profiling_available = false;
    bool fraud = false;
    IdSeq channels;
    IdSeq hid_channels;
    CCGKeywordSeq ccg_keywords;
    CCGKeywordSeq hid_ccg_keywords;
    TriggerMatchResult trigger_match_result;
    TimestampInfo client_create_time;
    TimestampInfo session_start;
    PublisherAccountIdSeq exclude_pubpixel_accounts;
    unsigned long tag_delivery_factor = 0;
    unsigned long ccg_delivery_factor = 0;
    unsigned long preview_ccid = 0;
    AdSlotSeq ad_slots;
    bool required_passback = false;
    unsigned long profiling_type = 0;
    bool disable_fraud_detection = false;
    bool need_debug_info = false;
    StringField page_keywords;
    StringField url_keywords;
    StringField ssp_location;
    StringField additional_info;
  };

  struct CreativeSelectResult
  {
    RequestIdInfo request_id;
    unsigned long ccid = 0;
    unsigned long cmp_id = 0;
    unsigned long campaign_group_id = 0;
    unsigned long order_set_id = 0;
    unsigned long advertiser_id = 0;
    StringField advertiser_name;
    StringField creative_size;
    DecimalInfo revenue;
    DecimalInfo ecpm;
    DecimalInfo pub_ecpm;
    StringField click_url;
    StringField destination_url;
    StringField creative_version_id;
    unsigned long creative_id = 0;
    bool https_safe_flag = false;
    unsigned char expanding = 0;
  };

  using CreativeSelectResultSeq = Sequence<CreativeSelectResult>;

  struct CreativeSelectDebugInfo
  {
    DecimalInfo imp_revenue;
    DecimalInfo click_revenue;
    DecimalInfo action_revenue;
    DecimalInfo ecpm_bid;
    StringField action_adv_url;
    StringField html_url;
    StringField triggered_expression;
    StringField full_expression;
  };

  using CreativeSelectDebugInfoSeq = Sequence<CreativeSelectDebugInfo>;

  struct AdSlotDebugInfo
  {
    unsigned long tag_id = 0;
    unsigned long tag_size_id = 0;
    unsigned long site_id = 0;
    unsigned long site_rate_id = 0;
    unsigned long min_no_adv_ecpm = 0;
    unsigned long min_text_ecpm = 0;
    unsigned long auction_type = 0;
    StringField track_pixel_url;
    DecimalInfo cpm_threshold;
    bool walled_garden = false;
    CreativeSelectDebugInfoSeq selected_creatives;
    StringField trace_ccg;
  };

  struct ContractInfo
  {
    unsigned long contract_id = 0;
    StringField number;
    StringField date;
    StringField type;
    bool vat_included = false;
    StringField ord_contract_id;
    StringField ord_ado_id;
    StringField subject_type;
    StringField action_type;
    bool agent_acting_for_publisher = false;
    unsigned long parent_contract_id = 0;
    StringField client_id;
    StringField client_name;
    StringField client_legal_form;
    StringField contractor_id;
    StringField contractor_name;
    StringField contractor_legal_form;
    TimestampInfo timestamp;
  };

  struct ExtContractInfo
  {
    ContractInfo contract_info;
    StringField parent_contract_id;
  };

  using ExtContractInfoSeq = Sequence<ExtContractInfo>;

  struct AdSlotResult
  {
    unsigned long ad_slot_id = 0;
    RequestIdInfo request_id;
    bool passback = false;
    StringField passback_url;
    StringField creative_body;
    StringField notice_url;
    StringSeq track_pixel_urls;
    StringField yandex_track_params;
    StringField creative_url;
    StringField track_pixel_params;
    StringField click_params;
    StringField mime_format;
    StringField iurl;
    bool test_request = false;
    CreativeSelectResultSeq selected_creatives;
    StringSeq external_visual_categories;
    StringSeq external_content_categories;
    StringField pub_currency_code;
    unsigned long overlay_width = 0;
    unsigned long overlay_height = 0;
    TokenSeq tokens;
    TokenSeq ext_tokens;
    bool track_impr = false;
    StringField tag_size;
    FreqCapIdSeq freq_caps;
    FreqCapIdSeq uc_freq_caps;
    AdSlotDebugInfo debug_info;
    TokenSeq native_data_tokens;
    TokenImageSeq native_image_tokens;
    StringField track_html_body;
    StringField erid;
    ExtContractInfoSeq contracts;
  };

  using AdSlotResultSeq = Sequence<AdSlotResult>;

  struct AdRequestDebugInfo
  {
    unsigned long colo_id = 0;
    IdSeq geo_channels;
    IdSeq platform_channels;
    unsigned long last_platform_channel_id = 0;
    unsigned long user_group_id = 0;
  };

  struct RequestCreativeResult
  {
    AdSlotResultSeq ad_slots;
    TimestampInfo process_time;
    AdRequestDebugInfo debug_info;
  };

  inline TimestampInfo
  pack_time(const Generics::Time& time)
  {
    return TimestampInfo{GrpcAlgs::pack_time(time)};
  }

  inline Generics::Time
  unpack_time(const TimestampInfo& time)
  {
    return GrpcAlgs::unpack_time(time.value);
  }

  inline UserIdInfo
  pack_user_id(const AdServer::Commons::UserId& user_id)
  {
    return UserIdInfo{GrpcAlgs::pack_user_id(user_id)};
  }

  inline AdServer::Commons::UserId
  unpack_user_id(const UserIdInfo& user_id)
  {
    return GrpcAlgs::unpack_user_id(user_id.value);
  }

  inline RequestIdInfo
  pack_request_id(const AdServer::Commons::RequestId& request_id)
  {
    return RequestIdInfo{GrpcAlgs::pack_request_id(request_id)};
  }

  inline AdServer::Commons::RequestId
  unpack_request_id(const RequestIdInfo& request_id)
  {
    return GrpcAlgs::unpack_request_id(request_id.value);
  }

  template<typename DecimalType>
  DecimalInfo
  pack_decimal(const DecimalType& decimal)
  {
    return DecimalInfo{GrpcAlgs::pack_decimal(decimal)};
  }

  template<typename DecimalType>
  DecimalType
  unpack_decimal(const DecimalInfo& decimal)
  {
    return GrpcAlgs::unpack_decimal<DecimalType>(decimal.value);
  }

  template<typename TargetSeq, typename SourceIteratorType>
  void fill_sequence(
    const SourceIteratorType& begin,
    const SourceIteratorType& end,
    TargetSeq& target,
    bool add = false)
  {
    const auto old_size = add ? target.length() : 0;
    target.length(old_size + std::distance(begin, end));

    std::size_t pos = old_size;
    for(auto it = begin; it != end; ++it, ++pos)
    {
      target[pos] = *it;
    }
  }

  template<typename SourceSeq, typename TargetSeq>
  void copy_sequence(const SourceSeq& source, TargetSeq& target)
  {
    target.length(source.length());
    for(std::size_t i = 0; i < source.length(); ++i)
    {
      target[i] = source[i];
    }
  }
}
