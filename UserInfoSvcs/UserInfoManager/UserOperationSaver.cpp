#include <UserInfoSvcs/UserInfoCommons/UserOperationProfiles.hpp>

#include <utility>

#include "Compatibility/UserOperationProfilesAdapter.hpp"
#include "UserOperationSaver.hpp"

namespace AdServer::UserInfoSvcs
{
  UserOperationSaver::UserOperationSaver(
    Logging::Logger* logger,
    const char* output_dir,
    const char* output_file_prefix,
    unsigned long chunks_number,
    const Generics::Time& flush_period,
    ProfilingCommons::FileController* file_controller,
    UserOperationProcessor* next_processor)
    /*throw(Exception)*/
    : MessageSaver(
        logger,
        output_dir,
        output_file_prefix,
        chunks_number,
        flush_period,
        1,
        file_controller,
        true),
      next_processor_(ReferenceCounting::add_ref(next_processor))
  {}

  AdServer::Commons::StartableAwaitable<bool>
  UserOperationSaver::co_remove_user_profile(const UserId& user_id)
    /*throw(ChunkNotFound, UserOperationProcessor::Exception)*/
  {
    co_return co_await next_processor_->co_remove_user_profile(user_id);
  }

  AdServer::Commons::StartableAwaitable<bool>
  UserOperationSaver::co_fraud_user(const UserId& user_id, const Generics::Time& now)
    /*throw(NotReady, ChunkNotFound, UserOperationProcessor::Exception)*/
  {
    {
      UserFraudOperationWriter fraud_operation_writer;
      fraud_operation_writer.operation_type() = UO_FRAUD;
      fraud_operation_writer.version() = FRAUD_OPERATION_PROFILE_VERSION;
      fraud_operation_writer.user_id() = user_id.to_string();
      fraud_operation_writer.fraud_time() = now.tv_sec;
      save_(user_id, UO_FRAUD, fraud_operation_writer);
    }

    co_return co_await next_processor_->co_fraud_user(user_id, now);
  }

  AdServer::Commons::StartableAwaitable<bool>
  UserOperationSaver::co_match(
    const RequestMatchParams& channel_match_info,
    long last_colo_id,
    long current_placement_colo_id,
    ColoUserId& colo_user_id,
    const ChannelIdPack& matched_channels,
    ChannelMatchMap& result_channels,
    UserAppearance& user_app,
    //PartlyMatchResult& partly_match_result,
    ProfileProperties& properties,
    AdServer::ProfilingCommons::OperationPriority op_priority,
    UserInfoManagerLogger::HistoryOptimizationInfo* ho_info,
    UniqueChannelsResult* pucr)
    /*throw(NotReady, ChunkNotFound, UserOperationProcessor::Exception)*/
  {
    if (!channel_match_info.silent_match)
    {
      UserMatchOperationWriter match_operation_writer;
      match_operation_writer.operation_type() = UO_MATCH;
      match_operation_writer.version() = MATCH_OPERATION_PROFILE_VERSION;
      match_operation_writer.user_id() = channel_match_info.user_id.to_string();
      match_operation_writer.temporary() = channel_match_info.temporary;
      match_operation_writer.time() = channel_match_info.current_time.tv_sec;
      match_operation_writer.request_colo_id() = channel_match_info.request_colo_id;
      match_operation_writer.last_colo_id() = last_colo_id;
      match_operation_writer.placement_colo_id() = current_placement_colo_id;
      match_operation_writer.change_last_request() = channel_match_info.change_last_request;
      match_operation_writer.household() = channel_match_info.household ? 1 : 0;
      match_operation_writer.cohort() = channel_match_info.cohort;
      match_operation_writer.repeat_trigger_timeout() =
        channel_match_info.repeat_trigger_timeout.tv_sec;
      match_operation_writer.filter_contextual_triggers() =
        channel_match_info.filter_contextual_triggers ? 1 : 0;

      ChannelTriggerMatchWriter cm_writer;

      for (const auto channel_id : matched_channels.page_channels)
      {
        cm_writer.channel_id() = channel_id;
        cm_writer.channel_trigger_id() = 0;
        match_operation_writer.page_channels().push_back(cm_writer);
      }

      for (const auto channel_id : matched_channels.search_channels)
      {
        cm_writer.channel_id() = channel_id;
        cm_writer.channel_trigger_id() = 0;
        match_operation_writer.search_channels().push_back(cm_writer);
      }

      for (const auto channel_id : matched_channels.url_channels)
      {
        cm_writer.channel_id() = channel_id;
        cm_writer.channel_trigger_id() = 0;
        match_operation_writer.url_channels().push_back(cm_writer);
      }

      for (const auto channel_id : matched_channels.url_keyword_channels)
      {
        cm_writer.channel_id() = channel_id;
        cm_writer.channel_trigger_id() = 0;
        match_operation_writer.url_keyword_channels().push_back(cm_writer);
      }

      std::copy(
        matched_channels.persistent_channels.begin(),
        matched_channels.persistent_channels.end(),
        std::back_inserter(match_operation_writer.persistent_channels()));

      if (channel_match_info.coord_data.defined)
      {
        CoordDataWriter cdw;

        cdw.latitude().alloc(AdServer::CampaignSvcs::CoordDecimal::PACK_SIZE);
        channel_match_info.coord_data.latitude.pack(cdw.latitude().data());

        cdw.longitude().alloc(AdServer::CampaignSvcs::CoordDecimal::PACK_SIZE);
        channel_match_info.coord_data.longitude.pack(cdw.longitude().data());

        cdw.accuracy().alloc(AdServer::CampaignSvcs::AccuracyDecimal::PACK_SIZE);
        channel_match_info.coord_data.accuracy.pack(cdw.accuracy().data());

        match_operation_writer.coord_data().push_back(cdw);
      }

      save_(channel_match_info.user_id, UO_MATCH, match_operation_writer);
    }

    co_return co_await next_processor_->co_match(
      channel_match_info,
      last_colo_id,
      current_placement_colo_id,
      colo_user_id,
      matched_channels,
      result_channels,
      user_app,
      properties,
      op_priority,
      ho_info,
      pucr);
  }

  AdServer::Commons::StartableAwaitable<bool>
  UserOperationSaver::co_merge(
    const RequestMatchParams& request_params,
    const Generics::MemBuf& merge_base_profile,
    Generics::MemBuf& merge_add_profile,
    const Generics::MemBuf& merge_history_profile,
    const Generics::MemBuf& merge_freq_cap_profile,
    UserAppearance& user_app,
    long last_colo_id,
    long current_placement_colo_id,
    AdServer::ProfilingCommons::OperationPriority op_priority,
    UserInfoManagerLogger::HistoryOptimizationInfo* ho_info)
    /*throw(NotReady, ChunkNotFound, UserOperationProcessor::Exception)*/
  {
    {
      UserMergeOperationWriter merge_operation_writer;
      merge_operation_writer.operation_type() = UO_MERGE;
      merge_operation_writer.version() = MERGE_OPERATION_PROFILE_VERSION;
      merge_operation_writer.user_id() = request_params.user_id.to_string();
      merge_operation_writer.time() = request_params.current_time.tv_sec;
      merge_operation_writer.exchange_merge() = 0;
      merge_operation_writer.change_last_request() = request_params.change_last_request;
      merge_operation_writer.household() = request_params.household ? 1 : 0;
      merge_operation_writer.request_colo_id() = request_params.request_colo_id;

      merge_operation_writer.merge_base_profile().alloc(merge_base_profile.size());
      ::memcpy(
        merge_operation_writer.merge_base_profile().data(),
        merge_base_profile.data(),
        merge_base_profile.size());
      merge_operation_writer.merge_add_profile().alloc(merge_add_profile.size());
      ::memcpy(
        merge_operation_writer.merge_add_profile().data(),
        merge_add_profile.data(),
        merge_add_profile.size());
      merge_operation_writer.merge_history_profile().alloc(merge_history_profile.size());
      ::memcpy(
        merge_operation_writer.merge_history_profile().data(),
        merge_history_profile.data(),
        merge_history_profile.size());
      merge_operation_writer.merge_freq_cap_profile().alloc(merge_freq_cap_profile.size());
      ::memcpy(
        merge_operation_writer.merge_freq_cap_profile().data(),
        merge_freq_cap_profile.data(),
        merge_freq_cap_profile.size());

      save_(request_params.user_id, UO_MERGE, merge_operation_writer);
    }

    co_return co_await next_processor_->co_merge(
      request_params,
      merge_base_profile,
      merge_add_profile,
      merge_history_profile,
      merge_freq_cap_profile,
      user_app,
      last_colo_id,
      current_placement_colo_id,
      op_priority,
      ho_info);
  }

  AdServer::Commons::StartableAwaitable<bool>
  UserOperationSaver::co_exchange_merge(
    const UserId& user_id,
    const Generics::MemBuf& merge_base_profile,
    const Generics::MemBuf& merge_history_profile,
    UserInfoManagerLogger::HistoryOptimizationInfo* ho_info)
    /*throw(NotReady, ChunkNotFound, UserOperationProcessor::Exception)*/
  {
    {
      UserMergeOperationWriter merge_operation_writer;
      merge_operation_writer.operation_type() = UO_MERGE;
      merge_operation_writer.version() = MERGE_OPERATION_PROFILE_VERSION;
      merge_operation_writer.user_id() = user_id.to_string();
      merge_operation_writer.time() = 0;
      merge_operation_writer.exchange_merge() = 1;
      merge_operation_writer.change_last_request() = 1;

      merge_operation_writer.merge_base_profile().alloc(merge_base_profile.size());
      ::memcpy(
        merge_operation_writer.merge_base_profile().data(),
        merge_base_profile.data(),
        merge_base_profile.size());
      merge_operation_writer.merge_history_profile().alloc(merge_history_profile.size());
      ::memcpy(
        merge_operation_writer.merge_history_profile().data(),
        merge_history_profile.data(),
        merge_history_profile.size());

      save_(user_id, UO_MERGE, merge_operation_writer);
    }

    co_return co_await next_processor_->co_exchange_merge(
      user_id,
      merge_base_profile,
      merge_history_profile,
      ho_info);
  }

  AdServer::Commons::StartableAwaitable<bool>
  UserOperationSaver::co_update_freq_caps(
    const UserId& user_id,
    const Generics::Time& now,
    const Commons::RequestId& request_id,
    const UserFreqCapProfile::FreqCapIdArray& freq_caps,
    const UserFreqCapProfile::FreqCapIdArray& uc_freq_caps,
    const UserFreqCapProfile::FreqCapIdArray& virtual_freq_caps,
    const UserFreqCapProfile::SeqOrderArray& seq_orders,
    const UserFreqCapProfile::CampaignIds& campaign_ids,
    const UserFreqCapProfile::CampaignIds& uc_campaign_ids,
    AdServer::ProfilingCommons::OperationPriority op_priority)
    /*throw(NotReady, ChunkNotFound, UserOperationProcessor::Exception)*/
  {
    {
      UserFreqCapUpdateOperationWriter profile_writer;
      profile_writer.operation_type() = UO_FC_UPDATE;
      profile_writer.version() = FC_UPDATE_OPERATION_PROFILE_VERSION;
      profile_writer.user_id() = user_id.to_string();
      profile_writer.time() = now.tv_sec;
      profile_writer.request_id() = request_id.to_string();
      std::copy(freq_caps.begin(), freq_caps.end(), std::back_inserter(profile_writer.freq_caps()));
      std::copy(uc_freq_caps.begin(),
        uc_freq_caps.end(),
        std::back_inserter(profile_writer.uc_freq_caps()));
      std::copy(virtual_freq_caps.begin(),
        virtual_freq_caps.end(),
        std::back_inserter(profile_writer.virtual_freq_caps()));
      std::copy(campaign_ids.begin(),
        campaign_ids.end(),
        std::back_inserter(profile_writer.campaign_ids()));
      std::copy(uc_campaign_ids.begin(),
        uc_campaign_ids.end(),
        std::back_inserter(profile_writer.uc_campaign_ids()));

      for (UserFreqCapProfile::SeqOrderArray::const_iterator it = seq_orders.begin();
           it != seq_orders.end(); ++it)
      {
        SeqOrderDescriptorWriter seq_order;
        seq_order.ccg_id() = it->ccg_id;
        seq_order.set_id() = it->set_id;
        seq_order.imps() = it->imps;

        profile_writer.seq_orders().push_back(seq_order);
      }

      save_(user_id, UO_FC_UPDATE, profile_writer);
    }

    co_return co_await next_processor_->co_update_freq_caps(
      user_id,
      now,
      request_id,
      freq_caps,
      uc_freq_caps,
      virtual_freq_caps,
      seq_orders,
      campaign_ids,
      uc_campaign_ids,
      op_priority);
  }

  AdServer::Commons::StartableAwaitable<bool>
  UserOperationSaver::co_confirm_freq_caps(
    const UserId& user_id,
    const Generics::Time& now,
    const Commons::RequestId& request_id,
    const std::set<unsigned long>& exclude_pubpixel_accounts)
    /*throw(ChunkNotFound, UserOperationProcessor::Exception)*/
  {
    {
      UserFreqCapConfirmOperationWriter profile_writer;
      profile_writer.operation_type() = UO_FC_CONFIRM;
      profile_writer.version() = FC_CONFIRM_OPERATION_PROFILE_VERSION;
      profile_writer.user_id() = user_id.to_string();
      profile_writer.time() = now.tv_sec;
      profile_writer.request_id() = request_id.to_string();

      std::copy(
        exclude_pubpixel_accounts.begin(),
        exclude_pubpixel_accounts.end(),
        std::back_inserter(profile_writer.publisher_accounts()));

      save_(user_id, UO_FC_CONFIRM, profile_writer);
    }

    co_return co_await next_processor_->co_confirm_freq_caps(
      user_id,
      now,
      request_id,
      exclude_pubpixel_accounts);
  }

  void pack_freq_cap_info(FreqCapInfoWriter& res, const AdServer::Commons::FreqCap& fc)
  {
    res.fc_id() = fc.fc_id;
    res.lifelimit() = fc.lifelimit;
    res.period() = fc.period.tv_sec;
    res.window_limit() = fc.window_limit;
    res.window_time() = fc.window_time.tv_sec;
  }

  AdServer::Commons::StartableAwaitable<bool>
  UserOperationSaver::co_consider_publishers_optin(
    const UserId& user_id,
    const std::set<unsigned long>& publisher_account_ids,
    const Generics::Time& now,
    AdServer::ProfilingCommons::OperationPriority op_priority)
    /*throw(ChunkNotFound, UserOperationProcessor::Exception)*/
  {
    co_return co_await next_processor_->co_consider_publishers_optin(
      user_id,
      publisher_account_ids,
      now,
      op_priority);
  }

  template <typename WriterType>
  void
  UserOperationSaver::save_(
    const AdServer::Commons::UserId& user_id,
    unsigned long op_index,
    const WriterType& writer)
  {
    Generics::MemBuf membuf(writer.size());
    writer.save(membuf.data(), membuf.size());
    write_operation(
      AdServer::Commons::uuid_distribution_hash(user_id),
      op_index,
      std::move(membuf));
  }

  void
  UserOperationSaver::wait_object()
    /*throw(Generics::ActiveObject::Exception, eh::Exception)*/
  {
    MessageSaver::wait_object();
    flush();
  }
}
