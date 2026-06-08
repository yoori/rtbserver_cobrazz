#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include <Generics/CompositeActiveObject.hpp>
#include <Logger/Logger.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

#include <Commons/Grpc/GrpcServer.hpp>

#include "CampaignManagerCore.hpp"

namespace AdServer::CampaignSvcs
{
  class CampaignManagerGrpc:
    public Generics::CompositeActiveObject,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    struct Stats
    {
      std::uint64_t call_in_progress = 0;
      std::uint64_t call_total = 0;
      std::uint64_t call_time = 0;
      std::uint64_t ready_in_progress = 0;
      std::uint64_t ready_total = 0;
      std::uint64_t ready_time = 0;
      std::uint64_t progress_comment_in_progress = 0;
      std::uint64_t progress_comment_total = 0;
      std::uint64_t progress_comment_time = 0;
      std::uint64_t match_geo_channels_in_progress = 0;
      std::uint64_t match_geo_channels_total = 0;
      std::uint64_t match_geo_channels_time = 0;
      std::uint64_t get_file_in_progress = 0;
      std::uint64_t get_file_total = 0;
      std::uint64_t get_file_time = 0;
      std::uint64_t get_campaign_creative_in_progress = 0;
      std::uint64_t get_campaign_creative_total = 0;
      std::uint64_t get_campaign_creative_time = 0;
      std::uint64_t process_match_request_in_progress = 0;
      std::uint64_t process_match_request_total = 0;
      std::uint64_t process_match_request_time = 0;
      std::uint64_t process_anonymous_request_in_progress = 0;
      std::uint64_t process_anonymous_request_total = 0;
      std::uint64_t process_anonymous_request_time = 0;
      std::uint64_t instantiate_ad_in_progress = 0;
      std::uint64_t instantiate_ad_total = 0;
      std::uint64_t instantiate_ad_time = 0;
      std::uint64_t trace_campaign_selection_index_in_progress = 0;
      std::uint64_t trace_campaign_selection_index_total = 0;
      std::uint64_t trace_campaign_selection_index_time = 0;
      std::uint64_t trace_campaign_selection_in_progress = 0;
      std::uint64_t trace_campaign_selection_total = 0;
      std::uint64_t trace_campaign_selection_time = 0;
      std::uint64_t get_campaign_creative_by_ccid_in_progress = 0;
      std::uint64_t get_campaign_creative_by_ccid_total = 0;
      std::uint64_t get_campaign_creative_by_ccid_time = 0;
      std::uint64_t get_channel_links_in_progress = 0;
      std::uint64_t get_channel_links_total = 0;
      std::uint64_t get_channel_links_time = 0;
      std::uint64_t get_discover_channels_in_progress = 0;
      std::uint64_t get_discover_channels_total = 0;
      std::uint64_t get_discover_channels_time = 0;
      std::uint64_t get_category_channels_in_progress = 0;
      std::uint64_t get_category_channels_total = 0;
      std::uint64_t get_category_channels_time = 0;
      std::uint64_t get_colocation_flags_in_progress = 0;
      std::uint64_t get_colocation_flags_total = 0;
      std::uint64_t get_colocation_flags_time = 0;
      std::uint64_t get_pub_pixels_in_progress = 0;
      std::uint64_t get_pub_pixels_total = 0;
      std::uint64_t get_pub_pixels_time = 0;
      std::uint64_t consider_passback_in_progress = 0;
      std::uint64_t consider_passback_total = 0;
      std::uint64_t consider_passback_time = 0;
      std::uint64_t consider_passback_track_in_progress = 0;
      std::uint64_t consider_passback_track_total = 0;
      std::uint64_t consider_passback_track_time = 0;
      std::uint64_t get_click_url_in_progress = 0;
      std::uint64_t get_click_url_total = 0;
      std::uint64_t get_click_url_time = 0;
      std::uint64_t verify_impression_in_progress = 0;
      std::uint64_t verify_impression_total = 0;
      std::uint64_t verify_impression_time = 0;
      std::uint64_t action_taken_in_progress = 0;
      std::uint64_t action_taken_total = 0;
      std::uint64_t action_taken_time = 0;
      std::uint64_t verify_opt_operation_in_progress = 0;
      std::uint64_t verify_opt_operation_total = 0;
      std::uint64_t verify_opt_operation_time = 0;
      std::uint64_t consider_web_operation_in_progress = 0;
      std::uint64_t consider_web_operation_total = 0;
      std::uint64_t consider_web_operation_time = 0;
      std::uint64_t get_config_in_progress = 0;
      std::uint64_t get_config_total = 0;
      std::uint64_t get_config_time = 0;
    };

    CampaignManagerGrpc(
      CampaignManagerCore* core,
      Logging::Logger* logger,
      std::string_view bind_address,
      unsigned int bind_port,
      std::size_t grpc_threads);

    Stats stats() const noexcept;

  protected:
    class ServiceImpl;
    using Impl = AdServer::Grpc::GrpcServer<ServiceImpl>;

  protected:
    ~CampaignManagerGrpc() noexcept override;

  private:
    struct AtomicStats;

    const std::string bind_address_;
    const std::shared_ptr<AtomicStats> stats_;
    const std::shared_ptr<Impl> impl_;
  };

  using CampaignManagerGrpc_var =
    ReferenceCounting::SmartPtr<CampaignManagerGrpc>;
}
