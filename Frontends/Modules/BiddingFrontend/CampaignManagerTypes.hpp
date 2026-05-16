#pragma once

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <Commons/GrpcAlgs.hpp>
#include <String/SubString.hpp>

namespace AdServer::Bidding::CampaignManager
{
  using IdSeq = std::vector<unsigned long>;
  using StringSeq = std::vector<std::string>;
  using ExternalCreativeCategoryIdSeq = StringSeq;
  using TimestampInfo = std::string;
  using RequestIdInfo = std::string;
  using UserIdInfo = std::string;
  using DecimalInfo = std::string;

  inline void assign_string(std::string& target, std::string_view value)
  {
    target.assign(value.data(), value.size());
  }

  inline void assign_string(std::string& target, const std::string& value)
  {
    target = value;
  }

  inline void assign_string(std::string& target, std::string&& value)
  {
    target = std::move(value);
  }

  inline void assign_string(std::string& target, const String::SubString& value)
  {
    target.assign(value.data(), value.size());
  }

  struct ChannelTriggerMatchInfo
  {
    unsigned long channel_trigger_id = 0;
    unsigned long channel_id = 0;
  };

  using ChannelTriggerMatchSeq = std::vector<ChannelTriggerMatchInfo>;

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
    std::string click_url;
    std::string original_keyword;
  };

  using CCGKeywordSeq = std::vector<CCGKeywordInfo>;
  using FreqCapIdSeq = IdSeq;
  using PublisherAccountIdSeq = IdSeq;

  struct TokenInfo
  {
    std::string name;
    std::string value;
  };

  using TokenSeq = std::vector<TokenInfo>;

  struct TokenImageInfo
  {
    std::string name;
    std::string value;
    unsigned long width = 0;
    unsigned long height = 0;
  };

  using TokenImageSeq = std::vector<TokenImageInfo>;

  struct GeoInfo
  {
    std::string country;
    std::string region;
    std::string city;
  };

  using GeoInfoSeq = std::vector<GeoInfo>;

  struct GeoCoordInfo
  {
    DecimalInfo longitude;
    DecimalInfo latitude;
    DecimalInfo accuracy;
  };

  using GeoCoordInfoSeq = std::vector<GeoCoordInfo>;

  struct SeqOrderInfo
  {
    unsigned long ccg_id = 0;
    unsigned long set_id = 0;
    unsigned long imps = 0;
  };

  using SeqOrderSeq = std::vector<SeqOrderInfo>;

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

  using CampaignFreqSeq = std::vector<CampaignFreq>;

  struct NativeDataToken
  {
    std::string name;
    bool required = false;
  };

  using NativeDataTokens = std::vector<NativeDataToken>;

  struct NativeImageToken
  {
    std::string name;
    bool required = false;
    unsigned long width = 0;
    unsigned long height = 0;
  };

  using NativeImageTokens = std::vector<NativeImageToken>;

  struct AdSlotInfo
  {
    unsigned long ad_slot_id = 0;
    std::string format;
    unsigned long tag_id = 0;
    StringSeq sizes;
    std::string ext_tag_id;
    DecimalInfo min_ecpm;
    std::string min_ecpm_currency_code;
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

  using AdSlotSeq = std::vector<AdSlotInfo>;

  struct CommonAdRequestInfo
  {
    TimestampInfo time;
    RequestIdInfo request_id;
    std::string creative_instantiate_type;
    unsigned long request_type = 0;
    unsigned long random = 0;
    bool test_request = false;
    bool log_as_test = false;
    unsigned long colo_id = 0;
    std::string external_user_id;
    std::string source_id;
    GeoInfoSeq location;
    GeoCoordInfoSeq coord_location;
    std::string full_referer;
    std::string referer;
    StringSeq urls;
    std::string security_token;
    std::string pub_impr_track_url;
    std::string pub_param;
    std::string preclick_url;
    std::string click_prefix_url;
    std::string original_url;
    UserIdInfo track_user_id;
    UserIdInfo user_id;
    unsigned long user_status = 0;
    std::string peer_ip;
    std::string user_agent;
    std::string cohort;
    unsigned long hpos = 0;
    std::string ext_track_params;
    TokenSeq tokens;
    bool set_cookie = false;
    std::string passback_type;
    std::string passback_url;
  };

  struct ContextAdRequestInfo
  {
    bool enabled_notice = false;
    std::string client;
    std::string client_version;
    IdSeq platform_ids;
    IdSeq geo_channels;
    std::string platform;
    std::string full_platform;
    std::string web_browser;
    std::string ip_hash;
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
    UserIdInfo merged_user_id;
    unsigned long search_engine_id = 0;
    std::string search_words;
    bool page_keywords_present = false;
    bool profiling_available = false;
    bool fraud = false;
    IdSeq channels;
    CCGKeywordSeq ccg_keywords;
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
    std::string page_keywords;
    std::string url_keywords;
    std::string ssp_location;
    std::string additional_info;
  };

  struct CreativeSelectResult
  {
    RequestIdInfo request_id;
    unsigned long ccid = 0;
    unsigned long cmp_id = 0;
    unsigned long campaign_group_id = 0;
    unsigned long order_set_id = 0;
    unsigned long advertiser_id = 0;
    std::string advertiser_name;
    std::string creative_size;
    DecimalInfo revenue;
    DecimalInfo ecpm;
    DecimalInfo pub_ecpm;
    std::string click_url;
    std::string destination_url;
    std::string creative_version_id;
    unsigned long creative_id = 0;
    bool https_safe_flag = false;
    unsigned char expanding = 0;
  };

  using CreativeSelectResultSeq = std::vector<CreativeSelectResult>;

  struct CreativeSelectDebugInfo
  {
    DecimalInfo imp_revenue;
    DecimalInfo click_revenue;
    DecimalInfo action_revenue;
    DecimalInfo ecpm_bid;
    std::string action_adv_url;
    std::string html_url;
    std::string triggered_expression;
    std::string full_expression;
  };

  using CreativeSelectDebugInfoSeq = std::vector<CreativeSelectDebugInfo>;

  struct AdSlotDebugInfo
  {
    unsigned long tag_id = 0;
    unsigned long tag_size_id = 0;
    unsigned long site_id = 0;
    unsigned long site_rate_id = 0;
    unsigned long min_no_adv_ecpm = 0;
    unsigned long min_text_ecpm = 0;
    unsigned long auction_type = 0;
    std::string track_pixel_url;
    DecimalInfo cpm_threshold;
    bool walled_garden = false;
    CreativeSelectDebugInfoSeq selected_creatives;
    std::string trace_ccg;
  };

  struct ContractInfo
  {
    unsigned long contract_id = 0;
    std::string number;
    std::string date;
    std::string type;
    bool vat_included = false;
    std::string ord_contract_id;
    std::string ord_ado_id;
    std::string subject_type;
    std::string action_type;
    bool agent_acting_for_publisher = false;
    unsigned long parent_contract_id = 0;
    std::string client_id;
    std::string client_name;
    std::string client_legal_form;
    std::string contractor_id;
    std::string contractor_name;
    std::string contractor_legal_form;
    TimestampInfo timestamp;
  };

  struct ExtContractInfo
  {
    ContractInfo contract_info;
    std::string parent_contract_id;
  };

  using ExtContractInfoSeq = std::vector<ExtContractInfo>;

  struct AdSlotResult
  {
    unsigned long ad_slot_id = 0;
    RequestIdInfo request_id;
    bool passback = false;
    std::string passback_url;
    std::string creative_body;
    std::string notice_url;
    StringSeq track_pixel_urls;
    std::string yandex_track_params;
    std::string creative_url;
    std::string track_pixel_params;
    std::string click_params;
    std::string mime_format;
    std::string iurl;
    bool test_request = false;
    CreativeSelectResultSeq selected_creatives;
    StringSeq external_visual_categories;
    StringSeq external_content_categories;
    std::string pub_currency_code;
    unsigned long overlay_width = 0;
    unsigned long overlay_height = 0;
    TokenSeq tokens;
    TokenSeq ext_tokens;
    bool track_impr = false;
    std::string tag_size;
    FreqCapIdSeq freq_caps;
    FreqCapIdSeq uc_freq_caps;
    AdSlotDebugInfo debug_info;
    TokenSeq native_data_tokens;
    TokenImageSeq native_image_tokens;
    std::string track_html_body;
    std::string erid;
    ExtContractInfoSeq contracts;
  };

  using AdSlotResultSeq = std::vector<AdSlotResult>;

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
    return GrpcAlgs::unpack_time(time);
  }

  inline UserIdInfo
  pack_user_id(const AdServer::Commons::UserId& user_id)
  {
    return UserIdInfo{GrpcAlgs::pack_user_id(user_id)};
  }

  inline AdServer::Commons::UserId
  unpack_user_id(const UserIdInfo& user_id)
  {
    return GrpcAlgs::unpack_user_id(user_id);
  }

  inline RequestIdInfo
  pack_request_id(const AdServer::Commons::RequestId& request_id)
  {
    return RequestIdInfo{GrpcAlgs::pack_request_id(request_id)};
  }

  inline AdServer::Commons::RequestId
  unpack_request_id(const RequestIdInfo& request_id)
  {
    return GrpcAlgs::unpack_request_id(request_id);
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
    return GrpcAlgs::unpack_decimal<DecimalType>(decimal);
  }

  template<typename TargetSeq, typename SourceIteratorType>
  void fill_sequence(
    const SourceIteratorType& begin,
    const SourceIteratorType& end,
    TargetSeq& target,
    bool add = false)
  {
    const auto old_size = add ? target.size() : 0;
    target.resize(old_size + std::distance(begin, end));

    std::size_t pos = old_size;
    for(auto it = begin; it != end; ++it, ++pos)
    {
      target[pos] = *it;
    }
  }

  template<typename SourceSeq, typename TargetSeq>
  void copy_sequence(const SourceSeq& source, TargetSeq& target)
  {
    target.resize(source.size());
    for(std::size_t i = 0; i < source.size(); ++i)
    {
      target[i] = source[i];
    }
  }
}
