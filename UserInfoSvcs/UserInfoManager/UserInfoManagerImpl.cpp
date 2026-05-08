#include "UserInfoManagerImpl.hpp"

#include <algorithm>
#include <set>
#include <vector>

#include <Commons/CorbaAlgs.hpp>

namespace AdServer::UserInfoSvcs
{
  namespace
  {
    UserInfoManagerCore::ByteVector
    unpack_bytes(const CORBACommons::OctSeq& source)
    {
      return UserInfoManagerCore::ByteVector(
        source.get_buffer(),
        source.get_buffer() + source.length());
    }

    void
    pack_bytes(
      const UserInfoManagerCore::ByteVector& source,
      CORBACommons::OctSeq& target)
    {
      target.length(source.size());
      std::copy(source.begin(), source.end(), target.get_buffer());
    }

    template<typename Seq>
    std::vector<unsigned long>
    unpack_ids(const Seq& source)
    {
      return std::vector<unsigned long>(
        source.get_buffer(),
        source.get_buffer() + source.length());
    }

    template<typename Seq>
    std::set<unsigned long>
    unpack_id_set(const Seq& source)
    {
      return std::set<unsigned long>(
        source.get_buffer(),
        source.get_buffer() + source.length());
    }

    UserInfoManagerCore::UserInfo
    unpack_user_info(const UserInfo& source)
    {
      UserInfoManagerCore::UserInfo result;
      result.user_id = CorbaAlgs::unpack_user_id(source.user_id);
      result.huser_id = CorbaAlgs::unpack_user_id(source.huser_id);
      result.time = Generics::Time(source.time);
      result.request_colo_id = source.request_colo_id;
      result.current_colo_id = source.current_colo_id;
      result.temporary = source.temporary;
      return result;
    }

    template<typename Seq>
    std::vector<UserInfoManagerCore::ChannelTriggerMatch>
    unpack_channel_matches(const Seq& source)
    {
      std::vector<UserInfoManagerCore::ChannelTriggerMatch> result;
      result.reserve(source.length());
      for(CORBA::ULong i = 0; i < source.length(); ++i)
      {
        result.push_back({source[i].channel_id, source[i].channel_trigger_id});
      }
      return result;
    }

    UserInfoManagerCore::MatchParams
    unpack_match_params(const UserInfoMatcher::MatchParams& source)
    {
      UserInfoManagerCore::MatchParams result;
      result.page_channel_ids = unpack_channel_matches(source.page_channel_ids);
      result.search_channel_ids = unpack_channel_matches(source.search_channel_ids);
      result.url_channel_ids = unpack_channel_matches(source.url_channel_ids);
      result.url_keyword_channel_ids =
        unpack_channel_matches(source.url_keyword_channel_ids);
      result.persistent_channel_ids = unpack_ids(source.persistent_channel_ids);
      result.cohort = source.cohort.in();
      result.cohort2 = source.cohort2.in();
      result.publishers_optin_timeout =
        CorbaAlgs::unpack_time(source.publishers_optin_timeout);
      result.use_empty_profile = source.use_empty_profile;
      result.filter_contextual_triggers = source.filter_contextual_triggers;
      result.silent_match = source.silent_match;
      result.no_match = source.no_match;
      result.no_result = source.no_result;
      result.provide_channel_count = source.provide_channel_count;
      result.provide_persistent_channels = source.provide_persistent_channels;
      result.change_last_request = source.change_last_request;
      result.ret_freq_caps = source.ret_freq_caps;
      result.geo_data_seq.reserve(source.geo_data_seq.length());
      for(CORBA::ULong i = 0; i < source.geo_data_seq.length(); ++i)
      {
        result.geo_data_seq.push_back({
          CorbaAlgs::unpack_decimal<CampaignSvcs::CoordDecimal>(
            source.geo_data_seq[i].latitude),
          CorbaAlgs::unpack_decimal<CampaignSvcs::CoordDecimal>(
            source.geo_data_seq[i].longitude),
          CorbaAlgs::unpack_decimal<CampaignSvcs::AccuracyDecimal>(
            source.geo_data_seq[i].accuracy)});
      }
      return result;
    }

    UserInfoManagerCore::ProfilesRequest
    unpack_profiles_request(const ProfilesRequestInfo& source)
    {
      return {
        source.base_profile,
        source.add_profile,
        source.history_profile,
        source.freq_cap_profile};
    }

    UserInfoManagerCore::UserProfiles
    unpack_user_profiles(const UserProfiles& source)
    {
      return {
        unpack_bytes(source.base_user_profile),
        unpack_bytes(source.add_user_profile),
        unpack_bytes(source.history_user_profile),
        unpack_bytes(source.freq_cap),
        unpack_bytes(source.pref_profile)};
    }

    UserProfiles*
    pack_user_profiles(const UserInfoManagerCore::UserProfiles& source)
    {
      UserProfiles_var result = new UserProfiles;
      pack_bytes(source.base_user_profile, result->base_user_profile);
      pack_bytes(source.add_user_profile, result->add_user_profile);
      pack_bytes(source.history_user_profile, result->history_user_profile);
      pack_bytes(source.freq_cap, result->freq_cap);
      pack_bytes(source.pref_profile, result->pref_profile);
      return result._retn();
    }

    template<typename Seq>
    void
    pack_id_seq(
      const std::vector<unsigned long>& source,
      Seq& target)
    {
      target.length(source.size());
      for(CORBA::ULong i = 0; i < source.size(); ++i)
      {
        target[i] = source[i];
      }
    }

    void
    pack_channel_weights(
      const std::vector<UserInfoManagerCore::ChannelWeight>& source,
      UserInfoMatcher::ChannelWeightSeq& target)
    {
      target.length(source.size());
      for(CORBA::ULong i = 0; i < source.size(); ++i)
      {
        target[i].channel_id = source[i].channel_id;
        target[i].weight = source[i].weight;
      }
    }

    UserInfoMatcher::MatchResult*
    pack_match_result(const UserInfoManagerCore::MatchResult& source)
    {
      UserInfoMatcher::MatchResult_var result =
        new UserInfoMatcher::MatchResult;
      result->adv_channel_count = source.adv_channel_count;
      result->discover_channel_count = source.discover_channel_count;
      result->colo_id = source.colo_id;
      result->times_inited = source.times_inited;
      result->last_request_time = CorbaAlgs::pack_time(source.last_request_time);
      result->create_time = CorbaAlgs::pack_time(source.create_time);
      result->session_start = CorbaAlgs::pack_time(source.session_start);
      result->fraud_request = source.fraud_request;
      result->cohort = source.cohort.c_str();
      result->cohort2 = source.cohort2.c_str();
      result->geo_data_seq.length(source.geo_data_seq.size());
      for(CORBA::ULong i = 0; i < source.geo_data_seq.size(); ++i)
      {
        result->geo_data_seq[i].latitude =
          CorbaAlgs::pack_decimal<CampaignSvcs::CoordDecimal>(
            source.geo_data_seq[i].latitude);
        result->geo_data_seq[i].longitude =
          CorbaAlgs::pack_decimal<CampaignSvcs::CoordDecimal>(
            source.geo_data_seq[i].longitude);
        result->geo_data_seq[i].accuracy =
          CorbaAlgs::pack_decimal<CampaignSvcs::AccuracyDecimal>(
            source.geo_data_seq[i].accuracy);
      }
      pack_id_seq(source.exclude_pubpixel_accounts, result->exclude_pubpixel_accounts);
      pack_id_seq(source.full_freq_caps, result->full_freq_caps);
      pack_id_seq(source.full_virtual_freq_caps, result->full_virtual_freq_caps);
      result->seq_orders.length(source.seq_orders.size());
      for(CORBA::ULong i = 0; i < source.seq_orders.size(); ++i)
      {
        result->seq_orders[i].ccg_id = source.seq_orders[i].ccg_id;
        result->seq_orders[i].set_id = source.seq_orders[i].set_id;
        result->seq_orders[i].imps = source.seq_orders[i].imps;
      }
      result->campaign_freqs.length(source.campaign_freqs.size());
      for(CORBA::ULong i = 0; i < source.campaign_freqs.size(); ++i)
      {
        result->campaign_freqs[i].campaign_id =
          source.campaign_freqs[i].campaign_id;
        result->campaign_freqs[i].imps = source.campaign_freqs[i].imps;
      }
      pack_channel_weights(source.channels, result->channels);
      pack_channel_weights(source.hid_channels, result->hid_channels);
      result->process_time = CorbaAlgs::pack_time(source.process_time);
      return result._retn();
    }

    void
    throw_corba(const UserInfoManagerCore::NotReady& ex)
    {
      CORBACommons::throw_desc<UserInfoManager::NotReady>(
        String::SubString(ex.what()));
    }

    void
    throw_corba(const UserInfoManagerCore::ChunkNotFound& ex)
    {
      CORBACommons::throw_desc<UserInfoManager::ChunkNotFound>(
        String::SubString(ex.what()));
    }

    void
    throw_corba(const UserInfoManagerCore::Exception& ex)
    {
      CORBACommons::throw_desc<UserInfoManager::ImplementationException>(
        String::SubString(ex.what()));
    }
  }

  UserInfoManagerImpl::UserInfoManagerImpl(
  UserInfoManagerCorePtr core) noexcept
  : core_(std::move(core))
  {}

  UserInfoManagerImpl::~UserInfoManagerImpl() noexcept = default;

  void
  UserInfoManagerImpl::get_master_stamp(
  CORBACommons::TimestampInfo_out master_stamp)
  {
  try
  {
    master_stamp = new CORBACommons::TimestampInfo;
    *master_stamp = CorbaAlgs::pack_time(core_->get_master_stamp());
  }
  catch(const UserInfoManagerCore::NotReady& ex) { throw_corba(ex); }
  catch(const UserInfoManagerCore::Exception& ex) { throw_corba(ex); }
  }

  CORBA::Boolean
  UserInfoManagerImpl::uim_ready() noexcept
  {
  return core_->uim_ready();
  }

  char*
  UserInfoManagerImpl::get_progress() noexcept
  {
  return CORBA::string_dup(core_->get_progress().c_str());
  }

  CORBA::Boolean
  UserInfoManagerImpl::merge(
  const AdServer::UserInfoSvcs::UserInfo& user_info,
  const AdServer::UserInfoSvcs::UserInfoMatcher::MatchParams& match_params,
  const AdServer::UserInfoSvcs::UserProfiles& merge_user_profile,
  CORBA::Boolean_out merge_success,
  CORBACommons::TimestampInfo_out last_request)
  {
  try
  {
    bool core_merge_success = false;
    Generics::Time core_last_request;
    const bool result = core_->merge(
      unpack_user_info(user_info),
      unpack_match_params(match_params),
      unpack_user_profiles(merge_user_profile),
      core_merge_success,
      core_last_request);
    merge_success = core_merge_success;
    last_request = new CORBACommons::TimestampInfo;
    *last_request = CorbaAlgs::pack_time(core_last_request);
    return result;
  }
  catch(const UserInfoManagerCore::NotReady& ex) { throw_corba(ex); }
  catch(const UserInfoManagerCore::ChunkNotFound& ex) { throw_corba(ex); }
  catch(const UserInfoManagerCore::Exception& ex) { throw_corba(ex); }
  return false;
  }

  CORBA::Boolean
  UserInfoManagerImpl::fraud_user(
  const CORBACommons::UserIdInfo& user_id,
  const CORBACommons::TimestampInfo& time)
  {
  try
  {
    return core_->fraud_user(
      CorbaAlgs::unpack_user_id(user_id),
      CorbaAlgs::unpack_time(time));
  }
  catch(const UserInfoManagerCore::NotReady& ex) { throw_corba(ex); }
  catch(const UserInfoManagerCore::ChunkNotFound& ex) { throw_corba(ex); }
  catch(const UserInfoManagerCore::Exception& ex) { throw_corba(ex); }
  return false;
  }

  CORBA::Boolean
  UserInfoManagerImpl::match(
  const AdServer::UserInfoSvcs::UserInfo& user_info,
  const AdServer::UserInfoSvcs::UserInfoMatcher::MatchParams& match_params,
  AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult_out match_result)
  {
  try
  {
    UserInfoManagerCore::MatchResult core_result;
    const bool result = core_->match(
      unpack_user_info(user_info),
      unpack_match_params(match_params),
      core_result);
    match_result = pack_match_result(core_result);
    return result;
  }
  catch(const UserInfoManagerCore::NotReady& ex) { throw_corba(ex); }
  catch(const UserInfoManagerCore::ChunkNotFound& ex) { throw_corba(ex); }
  catch(const UserInfoManagerCore::Exception& ex) { throw_corba(ex); }
  return false;
  }

  CORBA::Boolean
  UserInfoManagerImpl::get_user_profile(
  const CORBACommons::UserIdInfo& user_id,
  CORBA::Boolean temporary,
  const AdServer::UserInfoSvcs::ProfilesRequestInfo& profile_request,
  AdServer::UserInfoSvcs::UserProfiles_out user_profile)
  {
  try
  {
    UserInfoManagerCore::UserProfiles core_profile;
    const bool result = core_->get_user_profile(
      CorbaAlgs::unpack_user_id(user_id),
      temporary,
      unpack_profiles_request(profile_request),
      core_profile);
    user_profile = pack_user_profiles(core_profile);
    return result;
  }
  catch(const UserInfoManagerCore::NotReady& ex) { throw_corba(ex); }
  catch(const UserInfoManagerCore::ChunkNotFound& ex) { throw_corba(ex); }
  catch(const UserInfoManagerCore::Exception& ex) { throw_corba(ex); }
  return false;
  }

  CORBA::Boolean
  UserInfoManagerImpl::remove_user_profile(
  const CORBACommons::UserIdInfo& user_info)
  {
  try
  {
    return core_->remove_user_profile(CorbaAlgs::unpack_user_id(user_info));
  }
  catch(const UserInfoManagerCore::NotReady& ex) { throw_corba(ex); }
  catch(const UserInfoManagerCore::ChunkNotFound& ex) { throw_corba(ex); }
  catch(const UserInfoManagerCore::Exception& ex) { throw_corba(ex); }
  return false;
  }

  void
  UserInfoManagerImpl::update_user_freq_caps(
  const CORBACommons::UserIdInfo& user_id,
  const CORBACommons::TimestampInfo& time,
  const CORBACommons::RequestIdInfo& request_id,
  const FreqCapIdSeq& freq_caps,
  const FreqCapIdSeq& uc_freq_caps,
  const FreqCapIdSeq& virtual_freq_caps,
  const AdServer::UserInfoSvcs::UserInfoManager::SeqOrderSeq& seq_orders,
  const CampaignIdSeq& campaign_ids_seq,
  const CampaignIdSeq& uc_campaign_ids_seq)
  {
  try
  {
    std::vector<UserInfoManagerCore::SeqOrder> core_seq_orders;
    core_seq_orders.reserve(seq_orders.length());
    for(CORBA::ULong i = 0; i < seq_orders.length(); ++i)
    {
      core_seq_orders.push_back({
        seq_orders[i].ccg_id,
        seq_orders[i].set_id,
        seq_orders[i].imps});
    }
    core_->update_user_freq_caps(
      CorbaAlgs::unpack_user_id(user_id),
      CorbaAlgs::unpack_time(time),
      CorbaAlgs::unpack_request_id(request_id),
      unpack_ids(freq_caps),
      unpack_ids(uc_freq_caps),
      unpack_ids(virtual_freq_caps),
      core_seq_orders,
      unpack_ids(campaign_ids_seq),
      unpack_ids(uc_campaign_ids_seq));
  }
  catch(const UserInfoManagerCore::NotReady& ex) { throw_corba(ex); }
  catch(const UserInfoManagerCore::ChunkNotFound& ex) { throw_corba(ex); }
  catch(const UserInfoManagerCore::Exception& ex) { throw_corba(ex); }
  }

  void
  UserInfoManagerImpl::confirm_user_freq_caps(
  const CORBACommons::UserIdInfo& user_id,
  const CORBACommons::TimestampInfo& time,
  const CORBACommons::RequestIdInfo& request_id,
  const CORBACommons::IdSeq& exclude_pubpixel_accounts)
  {
  try
  {
    core_->confirm_user_freq_caps(
      CorbaAlgs::unpack_user_id(user_id),
      CorbaAlgs::unpack_time(time),
      CorbaAlgs::unpack_request_id(request_id),
      unpack_id_set(exclude_pubpixel_accounts));
  }
  catch(const UserInfoManagerCore::NotReady& ex) { throw_corba(ex); }
  catch(const UserInfoManagerCore::ChunkNotFound& ex) { throw_corba(ex); }
  catch(const UserInfoManagerCore::Exception& ex) { throw_corba(ex); }
  }

  void
  UserInfoManagerImpl::consider_publishers_optin(
  const CORBACommons::UserIdInfo& user_id_info,
  const CORBACommons::IdSeq& exclude_pubpixel_accounts,
  const CORBACommons::TimestampInfo& now)
  {
  try
  {
    core_->consider_publishers_optin(
      CorbaAlgs::unpack_user_id(user_id_info),
      unpack_id_set(exclude_pubpixel_accounts),
      CorbaAlgs::unpack_time(now));
  }
  catch(const UserInfoManagerCore::NotReady& ex) { throw_corba(ex); }
  catch(const UserInfoManagerCore::ChunkNotFound& ex) { throw_corba(ex); }
  catch(const UserInfoManagerCore::Exception& ex) { throw_corba(ex); }
  }

  void
  UserInfoManagerImpl::clear_expired(
  CORBA::Boolean synch,
  const CORBACommons::TimestampInfo& cleanup_time,
  CORBA::Long portion)
  {
    try
    {
      core_->clear_expired(
        synch,
        CorbaAlgs::unpack_time(cleanup_time),
        portion);
    }
    catch(const UserInfoManagerCore::Exception& ex) { throw_corba(ex); }
  }
} /* AdServer::UserInfoSvcs */
