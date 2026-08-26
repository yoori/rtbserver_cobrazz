#pragma once

#include <optional>
#include <string>
#include <unordered_set>
#include <Commons/Containers.hpp>
#include <Generics/MonoAllocator.hpp>
#include <Commons/UserInfoManip.hpp>

#include <ReferenceCounting/DefaultImpl.hpp>

#include "CampaignConfig.hpp"

namespace AdServer
{
namespace CampaignSvcs
{
  using namespace AdInstances;

  using FreqCapIdSet =
    Generics::MonoUnorderedSet<unsigned long>;

  struct SeqOrder
  {
    unsigned long set_id;
    unsigned long imps;
  };

  using SeqOrderMap =
    Generics::MonoMap<unsigned long, SeqOrder>;

  struct CampaignSelectParams: public ReferenceCounting::DefaultImpl<>
  {
    using CampaignImpsMap =
      Generics::MonoMap<unsigned long, unsigned long>;

    CampaignSelectParams(
      bool profiling_available_val,
      const FreqCapIdSet& full_freq_caps_val,
      const SeqOrderMap& seq_orders_val,
      const Colocation* colocation_val,
      const Tag* tag_val,
      const Tag::SizeMap& tag_sizes_val,
      bool filter_dest,
      int tag_visibility_val,
      int tag_predicted_viewability_val,
      Generics::MonoAllocatorArena* arena)
      : profiling_available(profiling_available_val),
        full_freq_caps(arena),
        seq_orders(seq_orders_val, arena),
        colocation(colocation_val),
        tag(tag_val),
        tag_sizes(tag_sizes_val),
        user_status(US_UNDEFINED),
        test_request(false),
        tag_delivery_factor(0),
        ccg_delivery_factor(0),
        up_expand_space(0),
        right_expand_space(0),
        down_expand_space(0),
        left_expand_space(0),
        video_min_duration(0),
        video_allow_skippable(true),
        video_allow_unskippable(true),
        random(0),
        random2(0),
        only_display_ad(false),
        need_debug_info(false),
        exclude_categories(arena),
        required_categories(arena),
        time_hour(0),
        time_week_day(0),
        channels(arena),
        last_platform_channel_id(0),
        secure(false),
        filter_empty_destination(filter_dest),
        allowed_durations(arena),
        campaign_imps(arena),
        tag_visibility(tag_visibility_val),
        tag_predicted_viewability(tag_predicted_viewability_val)
    {
      full_freq_caps.reserve(full_freq_caps_val.size());
      full_freq_caps.insert(full_freq_caps_val.begin(), full_freq_caps_val.end());
    }

    bool profiling_available;
    FreqCapIdSet full_freq_caps;
    SeqOrderMap seq_orders;
    const Colocation* colocation;
    const Tag* tag;
    Tag::SizeMap tag_sizes;
    AdServer::Commons::UserId user_id;
    std::string country_code;
    std::string format;
    std::string referer_hostname;
    std::string original_url;
    UserStatus user_status;
    bool test_request;
    Generics::Time time;
    unsigned long tag_delivery_factor;
    unsigned long ccg_delivery_factor;
    unsigned long up_expand_space;
    unsigned long right_expand_space;
    unsigned long down_expand_space;
    unsigned long left_expand_space;
    unsigned long video_min_duration;
    AdServer::Commons::Optional<unsigned long> video_max_duration;
    AdServer::Commons::Optional<unsigned long> video_skippable_max_duration;
    bool video_allow_skippable;
    bool video_allow_unskippable;
    unsigned long random;
    unsigned long random2;
    Generics::Time user_create_time;
    bool only_display_ad;
    bool need_debug_info;

    // min_ecpm in publisher currency
    RevenueDecimal min_pub_ecpm;
    RevenueDecimal min_ecpm;
    MonoCreativeCategoryIdSet exclude_categories;
    MonoCreativeCategoryIdSet required_categories;

    unsigned long time_hour;
    unsigned long time_week_day;
    std::string ext_tag_id;
    std::size_t short_referer_hash; // murmur3, see REQ-3637
    ChannelIdHashSet channels;
    //ChannelIdArray channels_array; // duplicate channels, but required for effective CTR eval
    unsigned long last_platform_channel_id;
    ChannelIdSet geo_channels;
    bool secure;
    bool filter_empty_destination;
    // Not empty only for AppNexus VAST
    AllowedDurationSet allowed_durations;
    CampaignImpsMap campaign_imps;

    int tag_visibility;
    int tag_predicted_viewability;

    std::string ssp_tag_id;
    std::optional<float> ssp_ctr;
    std::optional<float> ssp_viewability;
    std::optional<float> ssp_vtr;

  private:
    ~CampaignSelectParams() noexcept
    {}
  };

  typedef ReferenceCounting::SmartPtr<CampaignSelectParams> CampaignSelectParams_var;
  typedef ReferenceCounting::ConstPtr<CampaignSelectParams> CCampaignSelectParams_var;
}
}
