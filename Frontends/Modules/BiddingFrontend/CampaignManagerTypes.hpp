#pragma once

#include <algorithm>
#include <cstddef>
#include <memory>
#include <memory_resource>
#include <iterator>
#include <ostream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace AdServer::Bidding::CampaignManager
{
  struct ResultTokenInfo
  {
    std::string name;
    std::string value;
  };

  using ResultTokenSeq = std::pmr::vector<ResultTokenInfo>;

  struct ResultTokenImageInfo
  {
    std::string name;
    std::string value;
    unsigned long width = 0;
    unsigned long height = 0;
  };

  using ResultTokenImageSeq = std::pmr::vector<ResultTokenImageInfo>;

  struct CreativeSelectResult
  {
    std::string request_id;
    unsigned long ccid = 0;
    unsigned long cmp_id = 0;
    unsigned long campaign_group_id = 0;
    unsigned long order_set_id = 0;
    unsigned long advertiser_id = 0;
    std::string advertiser_name;
    std::string creative_size;
    std::string revenue;
    std::string ecpm;
    std::string pub_ecpm;
    std::string click_url;
    std::string destination_url;
    std::string creative_version_id;
    unsigned long creative_id = 0;
    bool https_safe_flag = false;
    unsigned char expanding = 0;
  };

  using CreativeSelectResultSeq = std::pmr::vector<CreativeSelectResult>;

  struct CreativeSelectDebugInfo
  {
    std::string imp_revenue;
    std::string click_revenue;
    std::string action_revenue;
    std::string ecpm_bid;
    std::string action_adv_url;
    std::string html_url;
    std::string triggered_expression;
    std::string full_expression;
  };

  using CreativeSelectDebugInfoSeq = std::pmr::vector<CreativeSelectDebugInfo>;

  struct AdSlotDebugInfo
  {
    AdSlotDebugInfo(
      std::pmr::memory_resource* resource = std::pmr::get_default_resource())
      : selected_creatives(resource)
    {}

    unsigned long tag_id = 0;
    unsigned long tag_size_id = 0;
    unsigned long site_id = 0;
    unsigned long site_rate_id = 0;
    unsigned long min_no_adv_ecpm = 0;
    unsigned long min_text_ecpm = 0;
    unsigned long auction_type = 0;
    std::string track_pixel_url;
    std::string cpm_threshold;
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
    std::string timestamp;
  };

  struct ExtContractInfo
  {
    ContractInfo contract_info;
    std::string parent_contract_id;
  };

  using ExtContractInfoSeq = std::pmr::vector<ExtContractInfo>;

  struct AdSlotResult
  {
    AdSlotResult(
      std::pmr::memory_resource* resource = std::pmr::get_default_resource())
      : track_pixel_urls(resource),
        selected_creatives(resource),
        external_visual_categories(resource),
        external_content_categories(resource),
        tokens(resource),
        ext_tokens(resource),
        freq_caps(resource),
        uc_freq_caps(resource),
        debug_info(resource),
        native_data_tokens(resource),
        native_image_tokens(resource),
        contracts(resource)
    {}

    unsigned long ad_slot_id = 0;
    std::string request_id;
    bool passback = false;
    std::string passback_url;
    std::string creative_body;
    std::string notice_url;
    std::pmr::vector<std::string> track_pixel_urls;
    std::string yandex_track_params;
    std::string creative_url;
    std::string track_pixel_params;
    std::string click_params;
    std::string mime_format;
    std::string iurl;
    bool test_request = false;
    CreativeSelectResultSeq selected_creatives;
    std::pmr::vector<std::string> external_visual_categories;
    std::pmr::vector<std::string> external_content_categories;
    std::string pub_currency_code;
    unsigned long overlay_width = 0;
    unsigned long overlay_height = 0;
    ResultTokenSeq tokens;
    ResultTokenSeq ext_tokens;
    bool track_impr = false;
    std::string tag_size;
    std::pmr::vector<unsigned long> freq_caps;
    std::pmr::vector<unsigned long> uc_freq_caps;
    AdSlotDebugInfo debug_info;
    ResultTokenSeq native_data_tokens;
    ResultTokenImageSeq native_image_tokens;
    std::string track_html_body;
    std::string erid;
    ExtContractInfoSeq contracts;
  };

  using AdSlotResultSeq = std::pmr::vector<AdSlotResult>;

  struct AdRequestDebugInfo
  {
    AdRequestDebugInfo(
      std::pmr::memory_resource* resource = std::pmr::get_default_resource())
      : geo_channels(resource),
        platform_channels(resource)
    {}

    unsigned long colo_id = 0;
    std::pmr::vector<unsigned long> geo_channels;
    std::pmr::vector<unsigned long> platform_channels;
    unsigned long last_platform_channel_id = 0;
    unsigned long user_group_id = 0;
  };

  struct RequestCreativeResult
  {
    RequestCreativeResult(
      std::pmr::memory_resource* resource = std::pmr::get_default_resource())
      : arena_(std::make_unique<std::pmr::monotonic_buffer_resource>(resource)),
        ad_slots(arena_.get()),
        debug_info(arena_.get())
    {}

    RequestCreativeResult(const RequestCreativeResult&) = delete;
    RequestCreativeResult& operator=(const RequestCreativeResult&) = delete;
    RequestCreativeResult(RequestCreativeResult&&) noexcept = default;
    RequestCreativeResult& operator=(RequestCreativeResult&&) = delete;

    std::unique_ptr<std::pmr::monotonic_buffer_resource> arena_;
    AdSlotResultSeq ad_slots;
    std::string process_time;
    AdRequestDebugInfo debug_info;
  };

  template<typename TargetSeq, typename SourceIteratorType>
  void fill_sequence(
    const SourceIteratorType& begin,
    const SourceIteratorType& end,
    TargetSeq& target,
    bool add = false)
  {
    if(!add)
    {
      target.clear();
    }

    using IteratorCategory = typename std::iterator_traits<
      SourceIteratorType>::iterator_category;
    if constexpr(std::is_base_of_v<
      std::random_access_iterator_tag,
      IteratorCategory>)
    {
      const auto old_size = target.size();
      target.resize(old_size + (end - begin));

      std::size_t pos = old_size;
      for(auto it = begin; it != end; ++it, ++pos)
      {
        target[pos] = *it;
      }
    }
    else
    {
      for(auto it = begin; it != end; ++it)
      {
        target.emplace_back();
        target.back() = *it;
      }
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
