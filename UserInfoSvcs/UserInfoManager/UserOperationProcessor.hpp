#ifndef USEROPERATIONPROCESSOR_HPP
#define USEROPERATIONPROCESSOR_HPP

#include <list>
#include <string>

#include <eh/Exception.hpp>

#include <ReferenceCounting/ReferenceCounting.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>

#include <Generics/Time.hpp>
#include <Generics/MemBuf.hpp>

#include <Commons/UserInfoManip.hpp>
#include <ProfilingCommons/ProfileMap/ProfileMap.hpp>

#include <UserInfoSvcs/UserInfoCommons/ChannelMatcher.hpp>
#include <UserInfoSvcs/UserInfoCommons/UserFreqCapProfile.hpp>
#include "UserInfoManagerLogger.hpp"

namespace AdServer
{
namespace UserInfoSvcs
{
  typedef AdServer::Commons::UserId UserId;

  struct ColoUserIds
  {
    std::list<std::string> user_id;
  };

  struct ColoUserId
  {
    ColoUserId() noexcept : need_profile(false), colo_id(0)
    {}

    bool need_profile;
    unsigned long colo_id;
    std::string user_id;
  };

  typedef std::list<ColoUserIds> ColoUserIdList;

  /** UserOperationProcessor
   *    interface for user info change
   */
  class UserOperationProcessor: public virtual ReferenceCounting::Interface
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    DECLARE_EXCEPTION(NotReady, Exception);
    DECLARE_EXCEPTION(ChunkNotFound, Exception);

    struct UserAppearance
    {
      Generics::Time last_request;
      Generics::Time create_time;
      Generics::Time session_start;
    };

    struct RequestMatchParams: public ProfileMatchParams
    {
      RequestMatchParams(
        const AdServer::Commons::UserId& uid_val,
        const Generics::Time& cur_time,
        const String::SubString& cohort,
        const String::SubString& cohort2,
        bool use_empty_profile_val,
        long colo_id_val,
        const Generics::Time& repeat_trigger_timeout_val = Generics::Time::ZERO,
        bool filter_contextual_triggers_val = false,
        bool temporary_val = true,
        bool silent_match_val = false,
        bool no_match_val = false,
        bool no_result_val = false,
        bool provide_channel_count_val = false,
        bool provide_persistent_channels_val = false,
        bool change_last_request_val = true,
        bool household_val = false,
        const CoordData* coord_data_val = nullptr) noexcept
        : ProfileMatchParams(
            cohort,
            cohort2,
            repeat_trigger_timeout_val,
            filter_contextual_triggers_val,
            no_match_val,
            no_result_val,
            provide_channel_count_val,
            provide_persistent_channels_val,
            change_last_request_val,
            household_val,
            colo_id_val,
            coord_data_val),
          user_id(uid_val),
          current_time(cur_time),
          use_empty_profile(use_empty_profile_val),
          temporary(temporary_val),
          silent_match(silent_match_val)
      {}

      AdServer::Commons::UserId user_id;
      Generics::Time current_time;
      bool use_empty_profile;

      bool temporary;
      bool silent_match;
    };

  public:
    virtual
    bool remove_user_profile(
      const UserId& user_id)
      /*throw(ChunkNotFound, Exception)*/ = 0;

    
    virtual
    void fraud_user(
      const UserId& user_id,
      const Generics::Time& now)
      /*throw(NotReady, ChunkNotFound, Exception)*/ = 0;

    virtual
    void match(
      const RequestMatchParams& channel_match_info,
      long last_colo_id,
      long current_placement_colo_id,
      ColoUserId& colo_user_id,
      const ChannelMatchPack& matched_channels,
      ChannelMatchMap& result_channels,
      UserAppearance& user_app,
      ProfileProperties& properties,
      AdServer::ProfilingCommons::OperationPriority op_priority,
      UserInfoManagerLogger::HistoryOptimizationInfo* ho_info,
      UniqueChannelsResult* pucr = 0)
      /*throw(NotReady, ChunkNotFound, Exception)*/ = 0;

    virtual void
    merge(
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
      /*throw(NotReady, ChunkNotFound, Exception)*/ = 0;

    virtual void exchange_merge(
      const UserId& user_id,
      const Generics::MemBuf& base_profile_buf,
      const Generics::MemBuf& history_profile_buf,
      UserInfoManagerLogger::HistoryOptimizationInfo* ho_info)
      /*throw(NotReady, ChunkNotFound, Exception)*/ = 0;

    // user freq caps
    virtual
    void update_freq_caps(
      const UserId& user_id,
      const Generics::Time& now,
      const Commons::RequestId& request_id,
      const UserFreqCapProfile::FreqCapIdList& freq_caps,
      const UserFreqCapProfile::FreqCapIdList& uc_freq_caps,
      const UserFreqCapProfile::FreqCapIdList& virtual_freq_caps,
      const UserFreqCapProfile::SeqOrderList& seq_orders,
      const UserFreqCapProfile::CampaignIds& campaign_ids,
      const UserFreqCapProfile::CampaignIds& uc_campaign_ids,
      AdServer::ProfilingCommons::OperationPriority op_priority)
      /*throw(ChunkNotFound, Exception)*/ = 0;

    virtual
    void confirm_freq_caps(
      const UserId& user_id,
      const Generics::Time& now,
      const Commons::RequestId& request_id,
      const std::set<unsigned long>& exclude_pubpixel_accounts)
      /*throw(ChunkNotFound, Exception)*/ = 0;

    virtual void
    remove_audience_channels(
      const UserId& user_id,
      const AudienceChannelSet& audience_channels)
      /*throw(ChunkNotFound, Exception)*/ = 0;

    virtual void
    add_audience_channels(
      const UserId& user_id,
      const AudienceChannelSet& audience_channels)
      /*throw(NotReady, ChunkNotFound, Exception)*/ = 0;

    virtual void
    consider_publishers_optin(
      const UserId& user_id,
      const std::set<unsigned long>& publisher_account_ids,
      const Generics::Time& now,
      AdServer::ProfilingCommons::OperationPriority op_priority)
      /*throw(ChunkNotFound, Exception)*/ = 0;
  };

  typedef ReferenceCounting::SmartPtr<UserOperationProcessor>
    UserOperationProcessor_var;
}
}

#endif /*USEROPERATIONPROCESSOR_HPP*/
