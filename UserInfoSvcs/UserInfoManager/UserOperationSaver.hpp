#pragma once

#include <ProfilingCommons/MessageSaver.hpp>

#include "UserOperationProcessor.hpp"

namespace AdServer::UserInfoSvcs
{
  class UserOperationSaver:
    public UserOperationProcessor,
    public virtual Generics::RefCountableActiveObject,
    public virtual ReferenceCounting::AtomicImpl,
    protected ProfilingCommons::MessageSaver
  {
  public:
    enum
    {
      UO_REMOVE = 1, // ?
      UO_FRAUD,
      UO_MATCH,
      UO_MERGE,
      UO_FC_UPDATE,
      UO_FC_CONFIRM,
    };

    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  public:
    UserOperationSaver(
      Logging::Logger* logger,
      const char* output_files_path,
      const char* output_file_prefix,
      unsigned long chunks_number,
      const Generics::Time& flush_period,
      ProfilingCommons::FileController* file_controller,
      UserOperationProcessor* next_processor)
      /*throw(Exception)*/;

    // UserOperationProcessor interface
    virtual AdServer::Commons::StartableAwaitable<bool>
    co_remove_user_profile(const UserId& user_id)
      override
      /*throw(ChunkNotFound, UserOperationProcessor::Exception)*/;

    virtual AdServer::Commons::StartableAwaitable<bool>
    co_fraud_user(const UserId& user_id, const Generics::Time& now)
      override
      /*throw(NotReady, ChunkNotFound, UserOperationProcessor::Exception)*/;

    virtual AdServer::Commons::StartableAwaitable<bool>
    co_match(
      const RequestMatchParams& channel_match_info,
      long last_colo_id,
      long current_placement_colo_id,
      ColoUserId& colo_user_id,
      const ChannelIdPack& matched_channels,
      ChannelMatchMap& result_channels,
      UserAppearance& user_app,
      ProfileProperties& properties,
      AdServer::ProfilingCommons::OperationPriority op_priority,
      UserInfoManagerLogger::HistoryOptimizationInfo* ho_info,
      UniqueChannelsResult* pucr = 0)
      override
      /*throw(NotReady, ChunkNotFound, UserOperationProcessor::Exception)*/;

    virtual AdServer::Commons::StartableAwaitable<bool>
    co_merge(
      const RequestMatchParams& request_params,
      const Generics::MemBuf& merge_base_profile,
      Generics::MemBuf& merge_add_profile,
      const Generics::MemBuf& merge_history_profile,
      const Generics::MemBuf& merge_freq_cap_profile,
      UserAppearance& user_app,
      long last_colo_id,
      long current_placement_colo_id,
      AdServer::ProfilingCommons::OperationPriority op_priority,
      UserInfoManagerLogger::HistoryOptimizationInfo* ho_info = 0)
      override
      /*throw(NotReady, ChunkNotFound, UserOperationProcessor::Exception)*/;

    virtual AdServer::Commons::StartableAwaitable<bool>
    co_exchange_merge(
      const UserId& user_id,
      const Generics::MemBuf& base_profile_buf,
      const Generics::MemBuf& history_profile_buf,
      UserInfoManagerLogger::HistoryOptimizationInfo* ho_info)
      override
      /*throw(NotReady, ChunkNotFound, UserOperationProcessor::Exception)*/;

    // user freq caps
    virtual AdServer::Commons::StartableAwaitable<bool>
    co_update_freq_caps(
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
      override
      /*throw(NotReady, ChunkNotFound, UserOperationProcessor::Exception)*/;

    virtual AdServer::Commons::StartableAwaitable<bool>
    co_confirm_freq_caps(
      const UserId& user_id,
      const Generics::Time& now,
      const Commons::RequestId& request_id,
      const std::set<unsigned long>& exclude_pubpixel_accounts)
      override
      /*throw(ChunkNotFound, UserOperationProcessor::Exception)*/;

    virtual AdServer::Commons::StartableAwaitable<bool>
    co_consider_publishers_optin(
      const UserId& user_id,
      const std::set<unsigned long>& publisher_account_ids,
      const Generics::Time& now,
      AdServer::ProfilingCommons::OperationPriority op_priority)
      override
      /*throw(ChunkNotFound, UserOperationProcessor::Exception)*/;

    // ActiveObject interface
    void
    wait_object()
      override
      /*throw(Generics::ActiveObject::Exception, eh::Exception)*/;

  protected:
    virtual ~UserOperationSaver() noexcept {}

    template <typename WriterType>
    void save_(
      const AdServer::Commons::UserId& user_id,
      unsigned long op_index,
      const WriterType& writer);

  private:
    UserOperationProcessor_var next_processor_;
  };

  using UserOperationSaver_var = ReferenceCounting::SmartPtr<UserOperationSaver>;
}
