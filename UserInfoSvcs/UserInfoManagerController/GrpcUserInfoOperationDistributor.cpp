// THIS
#include <Commons/GrpcAlgs.hpp>
#include <Commons/UserInfoManip.hpp>
#include <UserInfoSvcs/UserInfoManagerController/GrpcUserInfoOperationDistributor.hpp>
#include <UserInfoSvcs/UserInfoManagerController/UserInfoManagerController.hpp>

// UNIX_COMMONS
#include <UServerUtils/Grpc/Core/Common/Utils.hpp>

namespace AdServer::UserInfoSvcs
{

const char* ASPECT_GRPC_USER_INFO_DISTRIBUTOR =
  "GRPC_USER_INFO_OPERATION_DISTRIBUTOR";

GrpcUserInfoOperationDistributor::GrpcUserInfoOperationDistributor(
  Logger* logger,
  ManagerCoro* manager_coro,
  const ControllerRefList& controller_refs,
  const CorbaClientAdapter* corba_client_adapter,
  const ConfigPoolClient& config_pool_client,
  const std::size_t grpc_client_timeout,
  const Generics::Time& pool_timeout)
  : logger_(ReferenceCounting::add_ref(logger)),
    manager_coro_(ReferenceCounting::add_ref(manager_coro)),
    callback_(Generics::ActiveObjectCallback_var(
      new Logging::ActiveObjectCallbackImpl(
        logger,
        "GrpcUserInfoOperationDistributor",
        "UserInfo"))),
    try_count_(controller_refs.size()),
    config_pool_client_(config_pool_client),
    grpc_client_timeout_(grpc_client_timeout),
    pool_timeout_(pool_timeout),
    controller_refs_(controller_refs),
    scheduler_(UServerUtils::Grpc::Core::Common::Utils::create_scheduler(
      config_pool_client_.number_threads,
      logger_.in())),
    factory_client_container_(new FactoryClientContainer(
      logger_.in(),
      scheduler_,
      config_pool_client_,
      manager_coro_->get_main_task_processor())),
    corba_client_adapter_(ReferenceCounting::add_ref(corba_client_adapter)),
    task_runner_(new Generics::TaskRunner(callback_, try_count_))
{
  try
  {
    add_child_object(task_runner_);
  }
  catch(const Generics::CompositeActiveObject::Exception& exc)
  {
    Stream::Error stream;
    stream << FNS
           << exc.what();
    throw Exception(stream);
  }

  const PartitionNumber size = controller_refs_.size();
  for (PartitionNumber partition_number = 0;
       partition_number < size;
       ++partition_number)
  {
    partition_holders_.emplace_back(
      std::make_shared<PartitionHolder>());
    resolve_partition(partition_number);
  }
}

void GrpcUserInfoOperationDistributor::resolve_partition(
  const PartitionNumber partition_number) noexcept
{
  using UserInfoManagerController =
    AdServer::UserInfoSvcs::UserInfoManagerController;
  using UserInfoManagerController_var =
    AdServer::UserInfoSvcs::UserInfoManagerController_var;
  using UserInfoManagerDescriptionSeq_var =
    AdServer::UserInfoSvcs::UserInfoManagerDescriptionSeq_var;

  PartitionPtr partition;

  auto it_controller_ref = controller_refs_.begin();
  std::advance(it_controller_ref, partition_number);
  auto& corba_object_ref_list = *it_controller_ref;

  for (const auto& ref : corba_object_ref_list)
  {
    try
    {
      CORBA::Object_var obj = corba_client_adapter_->resolve_object(ref);
      if (CORBA::is_nil(obj.in()))
      {
        continue;
      }

      UserInfoManagerController_var controller =
        UserInfoManagerController::_narrow(obj.in());

      if (CORBA::is_nil(controller.in()))
      {
        continue;
      }

      UserInfoManagerDescriptionSeq_var user_info_managers;
        controller->get_session_description(user_info_managers.out());

      if (user_info_managers->length() == 0)
      {
        Stream::Error stream;
        stream << FNS
               << ": number of user_info_managers is null";
        logger_->error(
          stream.str(),
          ASPECT_GRPC_USER_INFO_DISTRIBUTOR);
        continue;
      }

      ChunkId max_chunk_number = 0;
      Partition::ChunkToClientContainer chunk_to_client_container;

      const auto length_user_info_managers = user_info_managers->length();
      for (std::size_t i = 0; i < length_user_info_managers; ++i)
      {
        const auto& user_info_manager_description = user_info_managers[i];

        const auto& chunk_ids = user_info_manager_description.chunk_ids;
        const std::size_t grpc_port = user_info_manager_description.grpc_port;
        const std::string host(user_info_manager_description.host.in());
        if (host.empty())
        {
          Stream::Error stream;
          stream << FNS
                 << ": host is empty";
          logger_->error(
            stream.str(),
            ASPECT_GRPC_USER_INFO_DISTRIBUTOR);
          throw Exception(stream.str());
        }

        const std::size_t length_chunk_ids = chunk_ids.length();
        for (std::size_t i = 0; i < length_chunk_ids; ++i)
        {
          const auto chunk_id = chunk_ids[i];
          auto client_container = factory_client_container_->create(
            host,
            grpc_port);
          auto result = chunk_to_client_container.try_emplace(
            chunk_id,
            std::move(client_container));
          if (!result.second)
          {
            Stream::Error stream;
            stream << FNS
                   << ": chunk_id="
                   << chunk_id
                   << " already exist";
            logger_->error(
              stream.str(),
              ASPECT_GRPC_USER_INFO_DISTRIBUTOR);
            continue;
          }

          if (chunk_id >= max_chunk_number)
          {
            max_chunk_number = chunk_id + 1;
          }
        }
      }

      partition = PartitionPtr(
        new Partition(
          std::move(chunk_to_client_container),
          max_chunk_number));
      break;
    }
    catch (...)
    {
    }
  }

  try
  {
    const Generics::Time now = Generics::Time::get_time_of_day();

    std::unique_lock lock(partition_holders_[partition_number]->mutex);
    if (partition)
    {
      partition_holders_[partition_number]->partition = std::move(partition);
    }
    else
    {
      partition_holders_[partition_number]->last_try_to_resolve = now;
      partition_holders_[partition_number]->resolve_in_progress = false;
    }
  }
  catch (...)
  {
  }
}

void GrpcUserInfoOperationDistributor::try_to_reresolve_partition(
  const PartitionNumber partition_num) noexcept
{
  try
  {
    const Generics::Time now = Generics::Time::get_time_of_day();

    std::unique_lock lock(partition_holders_[partition_num]->mutex);
    if(!partition_holders_[partition_num]->resolve_in_progress &&
       now >= partition_holders_[partition_num]->last_try_to_resolve + pool_timeout_)
    {
      partition_holders_[partition_num]->resolve_in_progress = true;
    }
    else
    {
      return;
    }
    lock.unlock();

    task_runner_->enqueue_task(
      Generics::Task_var(
        new PartitionResolver(
          *this,
          partition_num)));
  }
  catch (...)
  {
  }
}

GrpcUserInfoOperationDistributor::PartitionNumber
GrpcUserInfoOperationDistributor::get_partition_number(
  const String::SubString& id) const noexcept
{
  return (AdServer::Commons::external_id_distribution_hash(id) >> 8) % try_count_;
}

GrpcUserInfoOperationDistributor::PartitionPtr
GrpcUserInfoOperationDistributor::get_partition(
  const PartitionNumber partition_number) noexcept
{
  try
  {
    PartitionPtr partition;

    {
      std::shared_lock lock(partition_holders_[partition_number]->mutex);
      partition = partition_holders_[partition_number]->partition;
    }

    return partition;
  }
  catch (...)
  {
  }

  return {};
}

bool GrpcUserInfoOperationDistributor::the_same_chunk(
  const String::SubString uid1,
  const String::SubString uid2)
{
  const auto partition_number1 = get_partition_number(uid1);
  const auto partition_number2 = get_partition_number(uid2);
  if (partition_number1 != partition_number2)
  {
    return false;
  }

  const auto partition = get_partition(partition_number1);
  if (!partition)
  {
    return false;
  }

  const auto chunk_id1 = partition->chunk_id(uid1);
  const auto chunk_id2 = partition->chunk_id(uid2);

  return chunk_id1 == chunk_id2;
}

GrpcUserInfoOperationDistributor::GetUserProfileRequestPtr
GrpcUserInfoOperationDistributor::create_get_user_profile_request(
  const String::SubString& user_id,
  const bool temporary,
  const Types::ProfilesRequestInfo& profiles_request_info)
{
  auto request = std::make_unique<Proto::GetUserProfileRequest>();
  request->set_user_id(user_id.data(), user_id.length());
  request->set_temporary(temporary);

  Proto::ProfilesRequestInfo profiles_request_info_proto;
  profiles_request_info_proto.set_add_profile(profiles_request_info.add_profile);
  profiles_request_info_proto.set_base_profile(profiles_request_info.base_profile);
  profiles_request_info_proto.set_freq_cap_profile(profiles_request_info.freq_cap_profile);
  profiles_request_info_proto.set_history_profile(profiles_request_info.history_profile);
  profiles_request_info_proto.set_pref_profile(profiles_request_info.pref_profile);

  return request;
}

GrpcUserInfoOperationDistributor::GetUserProfileResponsePtr
GrpcUserInfoOperationDistributor::get_user_profile(
  const String::SubString& user_id,
  const bool temporary,
  const Types::ProfilesRequestInfo& profiles_request_info) noexcept
{
  return do_request<
    Proto::UserInfoManagerService_get_user_profile_ClientPool,
    Proto::GetUserProfileRequest,
    Proto::GetUserProfileResponse>(
      user_id,
      user_id,
      temporary,
      profiles_request_info);
}

GrpcUserInfoOperationDistributor::UpdateUserFreqCapsRequestPtr
GrpcUserInfoOperationDistributor::create_update_user_freq_caps_request(
  const String::SubString& user_id,
  const Generics::Time& time,
  const String::SubString& request_id,
  const Types::FreqCaps& freq_caps,
  const Types::UcFreqCaps& uc_freq_caps,
  const Types::VirtualFreqCaps& virtual_freq_caps,
  const Types::SeqOrders& seq_orders,
  const Types::CampaignIds& campaign_ids,
  const Types::UcCampaignIds& uc_campaign_ids)
{
  auto request = std::make_unique<Proto::UpdateUserFreqCapsRequest>();
  request->set_user_id(user_id.data(), user_id.length());
  request->set_time(GrpcAlgs::pack_time(time));
  request->set_request_id(request_id.data(), request_id.length());

  auto* freq_caps_proto = request->mutable_freq_caps();
  freq_caps_proto->Add(std::begin(freq_caps), std::end(freq_caps));

  auto* uc_freq_caps_proto = request->mutable_uc_freq_caps();
  uc_freq_caps_proto->Add(std::begin(uc_freq_caps), std::end(uc_freq_caps));

  auto* virtual_freq_caps_proto = request->mutable_virtual_freq_caps();
  virtual_freq_caps_proto->Add(std::begin(virtual_freq_caps), std::end(virtual_freq_caps));

  auto* seq_orders_proto = request->mutable_seq_orders();
  for (const auto& seq_order_info : seq_orders)
  {
    Proto::SeqOrderInfo seq_order_info_proto;
    seq_order_info_proto.set_ccg_id(seq_order_info.ccg_id);
    seq_order_info_proto.set_set_id(seq_order_info.set_id);
    seq_order_info_proto.set_imps(seq_order_info.imps);

    seq_orders_proto->Add(std::move(seq_order_info_proto));
  }

  auto* campaign_ids_proto = request->mutable_campaign_ids();
  campaign_ids_proto->Add(std::begin(campaign_ids), std::end(campaign_ids));

  auto* uc_campaign_ids_proto = request->mutable_uc_campaign_ids();
  uc_campaign_ids_proto->Add(std::begin(uc_campaign_ids), std::end(uc_campaign_ids));

  return request;
}

GrpcUserInfoOperationDistributor::UpdateUserFreqCapsResponsePtr
GrpcUserInfoOperationDistributor::update_user_freq_caps(
  const String::SubString& user_id,
  const Generics::Time& time,
  const String::SubString& request_id,
  const Types::FreqCaps& freq_caps,
  const Types::UcFreqCaps& uc_freq_caps,
  const Types::VirtualFreqCaps& virtual_freq_caps,
  const Types::SeqOrders& seq_orders,
  const Types::CampaignIds& campaign_ids,
  const Types::UcCampaignIds& uc_campaign_ids) noexcept
{
  return do_request<
    Proto::UserInfoManagerService_update_user_freq_caps_ClientPool,
    Proto::UpdateUserFreqCapsRequest,
    Proto::UpdateUserFreqCapsResponse>(
    user_id,
    user_id,
    time,
    request_id,
    freq_caps,
    uc_freq_caps,
    virtual_freq_caps,
    seq_orders,
    campaign_ids,
    uc_campaign_ids);
}

GrpcUserInfoOperationDistributor::ConfirmUserFreqCapsRequestPtr
GrpcUserInfoOperationDistributor::create_confirm_user_freq_caps_request(
  const String::SubString& user_id,
  const Generics::Time& time,
  const String::SubString& request_id,
  const Types::ExcludePubpixelAccounts& exclude_pubpixel_accounts)
{
  auto request = std::make_unique<Proto::ConfirmUserFreqCapsRequest>();
  request->set_user_id(user_id.data(), user_id.length());
  request->set_time(GrpcAlgs::pack_time(time));
  request->set_request_id(request_id.data(), request_id.length());

  auto* exclude_pubpixel_accounts_proto =
    request->mutable_exclude_pubpixel_accounts();
  exclude_pubpixel_accounts_proto->Add(
    std::begin(exclude_pubpixel_accounts),
    std::end(exclude_pubpixel_accounts));

  return request;
}

GrpcUserInfoOperationDistributor::ConfirmUserFreqCapsResponsePtr
GrpcUserInfoOperationDistributor::confirm_user_freq_caps(
  const String::SubString& user_id,
  const Generics::Time& time,
  const String::SubString& request_id,
  const Types::ExcludePubpixelAccounts& exclude_pubpixel_accounts) noexcept
{
  return do_request<
    Proto::UserInfoManagerService_confirm_user_freq_caps_ClientPool,
    Proto::ConfirmUserFreqCapsRequest,
    Proto::ConfirmUserFreqCapsResponse>(
      user_id,
      user_id,
      time,
      request_id,
      exclude_pubpixel_accounts);
}

GrpcUserInfoOperationDistributor::FraudUserRequestPtr
GrpcUserInfoOperationDistributor::create_fraud_user_request(
  const String::SubString& user_id,
  const Generics::Time& time)
{
  auto request = std::make_unique<Proto::FraudUserRequest>();
  request->set_user_id(user_id.data(), user_id.length());
  request->set_time(GrpcAlgs::pack_time(time));
  return request;
}

GrpcUserInfoOperationDistributor::FraudUserResponsePtr
GrpcUserInfoOperationDistributor::fraud_user(
  const String::SubString& user_id,
  const Generics::Time& time) noexcept
{
  return do_request<
    Proto::UserInfoManagerService_fraud_user_ClientPool,
    Proto::FraudUserRequest,
    Proto::FraudUserResponse>(
    user_id,
    user_id,
    time);
}

GrpcUserInfoOperationDistributor::RemoveUserProfileRequestPtr
GrpcUserInfoOperationDistributor::create_remove_user_profile_request(
  const String::SubString& user_id)
{
  auto request = std::make_unique<Proto::RemoveUserProfileRequest>();
  request->set_user_id(user_id.data(), user_id.length());
  return request;
}

GrpcUserInfoOperationDistributor::RemoveUserProfileResponsePtr
GrpcUserInfoOperationDistributor::remove_user_profile(
  const String::SubString& user_id) noexcept
{
  return do_request<
    Proto::UserInfoManagerService_remove_user_profile_ClientPool,
    Proto::RemoveUserProfileRequest,
    Proto::RemoveUserProfileResponse>(
    user_id,
    user_id);
}

GrpcUserInfoOperationDistributor::ConsiderPublishersOptinRequestPtr
GrpcUserInfoOperationDistributor::create_consider_publishers_optin_request(
  const String::SubString& user_id,
  const Types::ExcludePubpixelAccounts& exclude_pubpixel_accounts,
  const Generics::Time& now)
{
  auto request = std::make_unique<Proto::ConsiderPublishersOptinRequest>();
  request->set_user_id(user_id.data(), user_id.length());
  request->set_now(GrpcAlgs::pack_time(now));

  auto* exclude_pubpixel_accounts_proto =
    request->mutable_exclude_pubpixel_accounts();
  exclude_pubpixel_accounts_proto->Add(
    std::begin(exclude_pubpixel_accounts),
    std::end(exclude_pubpixel_accounts));

  return request;
}

GrpcUserInfoOperationDistributor::ConsiderPublishersOptinResponsePtr
GrpcUserInfoOperationDistributor::consider_publishers_optin(
  const String::SubString& user_id,
  const Types::ExcludePubpixelAccounts& exclude_pubpixel_accounts,
  const Generics::Time& now) noexcept
{
  return do_request<
    Proto::UserInfoManagerService_consider_publishers_optin_ClientPool,
    Proto::ConsiderPublishersOptinRequest,
    Proto::ConsiderPublishersOptinResponse>(
      user_id,
      user_id,
      exclude_pubpixel_accounts,
      now);
}

GrpcUserInfoOperationDistributor::MatchRequestPtr
GrpcUserInfoOperationDistributor::create_match_request(
  const Types::UserInfo& user_info,
  const Types::MatchParams& match_params)
{
  auto request = std::make_unique<Proto::MatchRequest>();

  auto* user_info_proto = request->mutable_user_info();
  user_info_proto->set_user_id(user_info.user_id);
  user_info_proto->set_huser_id(user_info.huser_id);
  user_info_proto->set_time(user_info.time);
  user_info_proto->set_last_colo_id(user_info.last_colo_id);
  user_info_proto->set_current_colo_id(user_info.current_colo_id);
  user_info_proto->set_request_colo_id(user_info.request_colo_id);
  user_info_proto->set_temporary(user_info.temporary);

  auto* match_params_proto = request->mutable_match_params();

  auto* persistent_channel_id_proto = match_params_proto->mutable_persistent_channel_ids();
  persistent_channel_id_proto->Add(
    std::begin(match_params.persistent_channel_ids),
    std::end(match_params.persistent_channel_ids));

  auto* search_channel_ids_proto = match_params_proto->mutable_search_channel_ids();
  const auto& search_channel_ids = match_params.search_channel_ids;
  for (const auto& search_channel_id : search_channel_ids)
  {
    Proto::ChannelTriggerMatch channel_trigger_match;
    channel_trigger_match.set_channel_id(search_channel_id.channel_id);
    channel_trigger_match.set_channel_trigger_id(search_channel_id.channel_trigger_id);

    search_channel_ids_proto->Add(std::move(channel_trigger_match));
  }

  auto* page_channel_ids_proto = match_params_proto->mutable_page_channel_ids();
  const auto& page_channel_ids = match_params.page_channel_ids;
  for (const auto& page_channel_id : page_channel_ids)
  {
    Proto::ChannelTriggerMatch channel_trigger_match;
    channel_trigger_match.set_channel_id(page_channel_id.channel_id);
    channel_trigger_match.set_channel_trigger_id(page_channel_id.channel_trigger_id);

    page_channel_ids_proto->Add(std::move(channel_trigger_match));
  }

  auto* url_channel_ids_proto = match_params_proto->mutable_url_channel_ids();
  const auto& url_channel_ids = match_params.url_channel_ids;
  for (const auto& url_channel_id : url_channel_ids)
  {
    Proto::ChannelTriggerMatch channel_trigger_match;
    channel_trigger_match.set_channel_id(url_channel_id.channel_id);
    channel_trigger_match.set_channel_trigger_id(url_channel_id.channel_trigger_id);

    url_channel_ids_proto->Add(std::move(channel_trigger_match));
  }

  auto* url_keyword_channel_ids_proto = match_params_proto->mutable_url_keyword_channel_ids();
  const auto& url_keyword_channel_ids = match_params.url_keyword_channel_ids;
  for (const auto& url_keyword_channel_id : url_keyword_channel_ids)
  {
    Proto::ChannelTriggerMatch channel_trigger_match;
    channel_trigger_match.set_channel_id(url_keyword_channel_id.channel_id);
    channel_trigger_match.set_channel_trigger_id(url_keyword_channel_id.channel_trigger_id);

    url_keyword_channel_ids_proto->Add(std::move(channel_trigger_match));
  }

  const auto& publishers_optin_timeout = match_params.publishers_optin_timeout;
  match_params_proto->set_publishers_optin_timeout(GrpcAlgs::pack_time(publishers_optin_timeout));

  match_params_proto->set_use_empty_profile(match_params.use_empty_profile);
  match_params_proto->set_silent_match(match_params.silent_match);
  match_params_proto->set_no_match(match_params.no_match);
  match_params_proto->set_no_result(match_params.no_result);
  match_params_proto->set_ret_freq_caps(match_params.ret_freq_caps);
  match_params_proto->set_provide_channel_count(match_params.provide_channel_count);
  match_params_proto->set_provide_persistent_channels(match_params.provide_persistent_channels);
  match_params_proto->set_change_last_request(match_params.change_last_request);
  match_params_proto->set_cohort(match_params.cohort);
  match_params_proto->set_cohort2(match_params.cohort2);
  match_params_proto->set_filter_contextual_triggers(match_params.filter_contextual_triggers);

  auto* geo_data_seq_proto = match_params_proto->mutable_geo_data_seq();
  const auto& geo_data_seq = match_params.geo_data_seq;
  for (const auto& geo_data : geo_data_seq)
  {
    Proto::GeoData geo_data_proto;
    geo_data_proto.set_latitude(GrpcAlgs::pack_decimal(geo_data.latitude));
    geo_data_proto.set_longitude(GrpcAlgs::pack_decimal(geo_data.longitude));
    geo_data_proto.set_accuracy(GrpcAlgs::pack_decimal(geo_data.accuracy));

    geo_data_seq_proto->Add(std::move(geo_data_proto));
  }

  return request;
}

GrpcUserInfoOperationDistributor::MatchResponsePtr
GrpcUserInfoOperationDistributor::match(
  const Types::UserInfo& user_info,
  const Types::MatchParams& match_params) noexcept
{
  try
  {
    const auto huid = GrpcAlgs::unpack_user_id(user_info.huser_id);
    if (huid.is_null() || the_same_chunk(user_info.user_id, user_info.huser_id))
    {
      return do_request<
        Proto::UserInfoManagerService_match_ClientPool,
        Proto::MatchRequest,
        Proto::MatchResponse>(
          user_info.user_id,
          user_info,
          match_params);
    }
    else
    {
      Types::UserInfo temp_uid_info;
      temp_uid_info.user_id = user_info.user_id;
      temp_uid_info.time = user_info.time;
      temp_uid_info.last_colo_id = user_info.last_colo_id;
      temp_uid_info.current_colo_id = user_info.current_colo_id;
      temp_uid_info.request_colo_id = user_info.request_colo_id;
      temp_uid_info.temporary = user_info.temporary;

      auto result = do_request<
        Proto::UserInfoManagerService_match_ClientPool,
        Proto::MatchRequest,
        Proto::MatchResponse>(
          temp_uid_info.user_id,
          temp_uid_info,
          match_params);
      if (!result || result->has_error())
      {
        return result;
      }

      Types::UserInfo temp_hid_info;
      temp_hid_info.huser_id = user_info.huser_id;
      temp_hid_info.time = user_info.time;
      temp_hid_info.last_colo_id = user_info.last_colo_id;
      temp_hid_info.current_colo_id = user_info.current_colo_id;
      temp_hid_info.request_colo_id = user_info.request_colo_id;
      temp_hid_info.temporary = false;

      auto hid_result = do_request<
        Proto::UserInfoManagerService_match_ClientPool,
        Proto::MatchRequest,
        Proto::MatchResponse>(
          temp_hid_info.huser_id,
          temp_hid_info,
          match_params);
      if (!hid_result || hid_result->has_error())
      {
        return hid_result;
      }

      const auto& info_hid = hid_result->info();
      const auto& hid_channels = info_hid.match_result().hid_channels();

      auto* info_proto = result->mutable_info();
      auto* match_result_proto = info_proto->mutable_match_result();
      auto* hid_channels_proto = match_result_proto->mutable_hid_channels();
      hid_channels_proto->Add(std::begin(hid_channels), std::end(hid_channels));

      return result;
    }
  }
  catch (const eh::Exception& exc)
  {
    try
    {
      Stream::Error stream;
      stream << FNS
             << ": "
             << exc.what();
      logger_->error(stream.str(), ASPECT_GRPC_USER_INFO_DISTRIBUTOR);
    }
    catch (...)
    {
    }
  }
  catch (...)
  {
    try
    {
      Stream::Error stream;
      stream << FNS
             << ": Unknown error";
      logger_->error(stream.str(), ASPECT_GRPC_USER_INFO_DISTRIBUTOR);
    }
    catch (...)
    {
    }
  }

  return {};
}

GrpcUserInfoOperationDistributor::MergeRequestPtr
GrpcUserInfoOperationDistributor::create_merge_request(
  const Types::UserInfo& user_info,
  const Types::MatchParams& match_params,
  const Types::UserProfiles& user_profiles)
{
  auto request = std::make_unique<Proto::MergeRequest>();

  auto* user_info_proto = request->mutable_user_info();
  user_info_proto->set_user_id(user_info.user_id);
  user_info_proto->set_huser_id(user_info.huser_id);
  user_info_proto->set_time(user_info.time);
  user_info_proto->set_last_colo_id(user_info.last_colo_id);
  user_info_proto->set_current_colo_id(user_info.current_colo_id);
  user_info_proto->set_request_colo_id(user_info.request_colo_id);
  user_info_proto->set_temporary(user_info.temporary);

  auto* match_params_proto = request->mutable_match_params();

  auto* persistent_channel_id_proto = match_params_proto->mutable_persistent_channel_ids();
  persistent_channel_id_proto->Add(
    std::begin(match_params.persistent_channel_ids),
    std::end(match_params.persistent_channel_ids));

  auto* search_channel_ids_proto = match_params_proto->mutable_search_channel_ids();
  const auto& search_channel_ids = match_params.search_channel_ids;
  for (const auto& search_channel_id : search_channel_ids)
  {
    Proto::ChannelTriggerMatch channel_trigger_match;
    channel_trigger_match.set_channel_id(search_channel_id.channel_id);
    channel_trigger_match.set_channel_trigger_id(search_channel_id.channel_trigger_id);

    search_channel_ids_proto->Add(std::move(channel_trigger_match));
  }

  auto* page_channel_ids_proto = match_params_proto->mutable_page_channel_ids();
  const auto& page_channel_ids = match_params.page_channel_ids;
  for (const auto& page_channel_id : page_channel_ids)
  {
    Proto::ChannelTriggerMatch channel_trigger_match;
    channel_trigger_match.set_channel_id(page_channel_id.channel_id);
    channel_trigger_match.set_channel_trigger_id(page_channel_id.channel_trigger_id);

    page_channel_ids_proto->Add(std::move(channel_trigger_match));
  }

  auto* url_channel_ids_proto = match_params_proto->mutable_url_channel_ids();
  const auto& url_channel_ids = match_params.url_channel_ids;
  for (const auto& url_channel_id : url_channel_ids)
  {
    Proto::ChannelTriggerMatch channel_trigger_match;
    channel_trigger_match.set_channel_id(url_channel_id.channel_id);
    channel_trigger_match.set_channel_trigger_id(url_channel_id.channel_trigger_id);

    url_channel_ids_proto->Add(std::move(channel_trigger_match));
  }

  auto* url_keyword_channel_ids_proto = match_params_proto->mutable_url_keyword_channel_ids();
  const auto& url_keyword_channel_ids = match_params.url_keyword_channel_ids;
  for (const auto& url_keyword_channel_id : url_keyword_channel_ids)
  {
    Proto::ChannelTriggerMatch channel_trigger_match;
    channel_trigger_match.set_channel_id(url_keyword_channel_id.channel_id);
    channel_trigger_match.set_channel_trigger_id(url_keyword_channel_id.channel_trigger_id);

    url_keyword_channel_ids_proto->Add(std::move(channel_trigger_match));
  }

  const auto& publishers_optin_timeout = match_params.publishers_optin_timeout;
  match_params_proto->set_publishers_optin_timeout(GrpcAlgs::pack_time(publishers_optin_timeout));

  match_params_proto->set_use_empty_profile(match_params.use_empty_profile);
  match_params_proto->set_silent_match(match_params.silent_match);
  match_params_proto->set_no_match(match_params.no_match);
  match_params_proto->set_no_result(match_params.no_result);
  match_params_proto->set_ret_freq_caps(match_params.ret_freq_caps);
  match_params_proto->set_provide_channel_count(match_params.provide_channel_count);
  match_params_proto->set_provide_persistent_channels(match_params.provide_persistent_channels);
  match_params_proto->set_change_last_request(match_params.change_last_request);
  match_params_proto->set_cohort(match_params.cohort);
  match_params_proto->set_cohort2(match_params.cohort2);
  match_params_proto->set_filter_contextual_triggers(match_params.filter_contextual_triggers);

  auto* geo_data_seq_proto = match_params_proto->mutable_geo_data_seq();
  const auto& geo_data_seq = match_params.geo_data_seq;
  for (const auto& geo_data : geo_data_seq)
  {
    Proto::GeoData geo_data_proto;
    geo_data_proto.set_latitude(GrpcAlgs::pack_decimal(geo_data.latitude));
    geo_data_proto.set_longitude(GrpcAlgs::pack_decimal(geo_data.longitude));
    geo_data_proto.set_accuracy(GrpcAlgs::pack_decimal(geo_data.accuracy));

    geo_data_seq_proto->Add(std::move(geo_data_proto));
  }

  auto* user_profiles_proto = request->mutable_merge_user_profiles();
  user_profiles_proto->set_base_user_profile(user_profiles.base_user_profile);
  user_profiles_proto->set_add_user_profile(user_profiles.add_user_profile);
  user_profiles_proto->set_history_user_profile(user_profiles.history_user_profile);
  user_profiles_proto->set_freq_cap(user_profiles.freq_cap);
  user_profiles_proto->set_pref_profile(user_profiles.pref_profile);

  return request;
}

GrpcUserInfoOperationDistributor::MergeResponsePtr
GrpcUserInfoOperationDistributor::merge(
  const Types::UserInfo& user_info,
  const Types::MatchParams& match_params,
  const Types::UserProfiles& user_profiles) noexcept
{
  try
  {
    const auto huid = GrpcAlgs::unpack_user_id(user_info.huser_id);
    if (huid.is_null() || the_same_chunk(user_info.user_id, user_info.huser_id))
    {
      return do_request<
        Proto::UserInfoManagerService_merge_ClientPool,
        Proto::MergeRequest,
        Proto::MergeResponse>(
          user_info.user_id,
          user_info,
          match_params,
          user_profiles);
    }
    else
    {
      Types::UserInfo temp_uid_info;
      temp_uid_info.user_id = user_info.user_id;
      temp_uid_info.time = user_info.time;
      temp_uid_info.last_colo_id = user_info.last_colo_id;
      temp_uid_info.current_colo_id = user_info.current_colo_id;
      temp_uid_info.request_colo_id = user_info.request_colo_id;
      temp_uid_info.temporary = user_info.temporary;

      auto result = do_request<
        Proto::UserInfoManagerService_merge_ClientPool,
        Proto::MergeRequest,
        Proto::MergeResponse>(
          temp_uid_info.user_id,
          temp_uid_info,
          match_params,
          user_profiles);
      if (!result || result->has_error() || !result->info().merge_success())
      {
        return result;
      }

      Types::UserInfo temp_hid_info;
      temp_hid_info.huser_id = user_info.huser_id;
      temp_hid_info.time = user_info.time;
      temp_hid_info.last_colo_id = user_info.last_colo_id;
      temp_hid_info.current_colo_id = user_info.current_colo_id;
      temp_hid_info.request_colo_id = user_info.request_colo_id;
      temp_hid_info.temporary = false;

      auto hid_result = do_request<
        Proto::UserInfoManagerService_merge_ClientPool,
        Proto::MergeRequest,
        Proto::MergeResponse>(
        temp_hid_info.huser_id,
        temp_hid_info,
        match_params,
        user_profiles);
      if (!hid_result || hid_result->has_error())
      {
        return hid_result;
      }

      const auto& info_hid = hid_result->info();

      auto* info_proto = result->mutable_info();
      info_proto->set_merge_success(info_hid.merge_success());

      return result;
    }
  }
  catch (const eh::Exception& exc)
  {
    try
    {
      Stream::Error stream;
      stream << FNS
             << ": "
             << exc.what();
      logger_->error(stream.str(), ASPECT_GRPC_USER_INFO_DISTRIBUTOR);
    }
    catch (...)
    {
    }
  }
  catch (...)
  {
    try
    {
      Stream::Error stream;
      stream << FNS
             << ": Unknown error";
      logger_->error(stream.str(), ASPECT_GRPC_USER_INFO_DISTRIBUTOR);
    }
    catch (...)
    {
    }
  }

  return {};
}

} // namespace AdServer::UserInfoSvcs