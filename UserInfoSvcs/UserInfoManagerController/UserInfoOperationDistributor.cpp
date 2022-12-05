#include <Commons/UserInfoManip.hpp>
#include <Commons/CorbaAlgs.hpp>
#include "UserInfoOperationDistributor.hpp"

namespace AdServer
{
namespace UserInfoSvcs
{
  UserInfoOperationDistributor::PartitionResolver::PartitionResolver(
    UserInfoOperationDistributor& distributor,
    unsigned int partition_num)
    noexcept
    : distributor_(distributor),
      partition_num_(partition_num)
  {}

  void
  UserInfoOperationDistributor::PartitionResolver::execute()
    noexcept
  {
    distributor_.resolve_partition_(partition_num_);
  }

  // UserInfoOperationDistributor
  UserInfoOperationDistributor::UserInfoOperationDistributor(
    Logging::Logger* logger,
    const ControllerRefList& controller_refs,
    const CORBACommons::CorbaClientAdapter* corba_client_adapter,
    const Generics::Time& pool_timeout)
    : callback_(Generics::ActiveObjectCallback_var(
        new Logging::ActiveObjectCallbackImpl(
          logger,
          "UserInfoOperationDistributor",
          "UserInfo"))),
      try_count_(controller_refs.size()),
      controller_refs_(controller_refs),
      corba_client_adapter_(ReferenceCounting::add_ref(corba_client_adapter)),
      task_runner_(new Generics::TaskRunner(callback_, try_count_)),
      pool_timeout_(pool_timeout)
  {
    try
    {
      add_child_object(task_runner_);
    }
    catch(const Generics::CompositeActiveObject::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "UserInfoOperationDistributor: CompositeActiveObject::Exception caught: " <<
        ex.what();
      throw Exception(ostr);
    }

    init_pool_();
  }

  UserInfoOperationDistributor::~UserInfoOperationDistributor()
    noexcept
  {}

  void
  UserInfoOperationDistributor::init_pool_()
    /*throw(eh::Exception, CORBACommons::CorbaClientAdapter::Exception)*/
  {
    int partition_num = 0;
    for (ControllerRefList::const_iterator group_it = controller_refs_.begin();
         group_it != controller_refs_.end(); ++group_it)
    {
      partitions_pool_.push_back(Partition());
      resolve_partition_(partition_num++);
    }
  }

  void
  UserInfoOperationDistributor::try_to_reresolve_partition_(unsigned int partition_num)
    noexcept
  {
    Generics::Time curtime = Generics::Time::get_time_of_day();

    {
      Guard guard(partition_lock_);
      if(!partitions_pool_[partition_num].resolve_in_progress_ &&
         curtime >= partitions_pool_[partition_num].last_try_to_resolve_ + pool_timeout_)
      {
        partitions_pool_[partition_num].resolve_in_progress_ = true;
      }
      else
      {
        return;
      }
    }

    Generics::Task_var msg(new PartitionResolver(
      *this, partition_num));
    task_runner_->enqueue_task(msg);
  }

  void
  UserInfoOperationDistributor::resolve_partition_(unsigned int partition_num)
    noexcept
  {
    Partition::PartitionInfo_var new_partition;

    ControllerRefList::iterator iter = controller_refs_.begin();
    std::advance(iter, partition_num);

    const CORBACommons::CorbaObjectRefList& controllers = *iter;

    for(CORBACommons::CorbaObjectRefList::const_iterator ref_it = controllers.begin();
        ref_it != controllers.end(); ++ref_it)
    {
      try
      {
        CORBA::Object_var obj = corba_client_adapter_->resolve_object(*ref_it);
        if(!CORBA::is_nil(obj.in()))
        {
          AdServer::UserInfoSvcs::UserInfoManagerController_var controller =
            AdServer::UserInfoSvcs::UserInfoManagerController::_narrow(obj.in());

          if(!CORBA::is_nil(controller.in()))
          {
            AdServer::UserInfoSvcs::UserInfoManagerDescriptionSeq_var user_info_managers;
            controller->get_session_description(user_info_managers.out());

            new_partition = new Partition::PartitionInfo();
            for(unsigned int i = 0; i < user_info_managers->length(); ++i)
            {
              AdServer::UserInfoSvcs::UserInfoManagerDescription&
                user_info_manager_descr = user_info_managers[i];

              for(unsigned int j = 0; j < user_info_manager_descr.chunk_ids.length(); ++j)
              {
                unsigned long chunk_id = user_info_manager_descr.chunk_ids[j];

                new_partition->chunks_ref_map_.insert(
                  std::make_pair(
                    chunk_id,
                    Partition::UIMInfo_var(new Partition::UIMInfo(
                      user_info_manager_descr.user_info_manager))));

                if(chunk_id >= new_partition->max_chunk_number_)
                {
                  new_partition->max_chunk_number_ = chunk_id + 1;
                }
              }
            }

            break;
          }
        }
      }
      catch(...)
      {}
    }

    Generics::Time curtime = Generics::Time::get_time_of_day();
    {
      Guard guard(partition_lock_);
      if (new_partition.in())
      {
        partitions_pool_[partition_num].partition_info_.swap(new_partition);
      }
      else
      {
        partitions_pool_[partition_num].last_try_to_resolve_ = curtime;
        partitions_pool_[partition_num].resolve_in_progress_ = false;
      }
    }
  }

  bool
  UserInfoOperationDistributor::is_uim_bad_(Partition::UIMInfo* uim_info)
    const noexcept
  {
    Generics::Time curtime = Generics::Time::get_time_of_day();

    Guard guard(uim_info->lock);
    if (uim_info->is_bad &&
      curtime >= uim_info->marked_as_bad + pool_timeout_)
    {
      uim_info->is_bad = false;
    }

    return uim_info->is_bad;
  }

  void
  UserInfoOperationDistributor::mark_uim_bad_(Partition::UIMInfo* uim_info)
    noexcept
  {
    Generics::Time curtime = Generics::Time::get_time_of_day();
    {
      Guard guard(uim_info->lock);
      uim_info->is_bad = true;
      uim_info->marked_as_bad = curtime;
    }
  }

  bool
  UserInfoOperationDistributor::get_partition_(
    unsigned long partition_num,
    Partition::CPartitionInfo_var& partition)
    noexcept
  {
    {
      Guard guard(partition_lock_);
      partition = partitions_pool_[partition_num].partition_info_;
    }
    if (!(partition.in() && partition->max_chunk_number_))
    {
      return false;
    }
    return true;
  }

  bool
  UserInfoOperationDistributor::the_same_chunk_(
    const CORBACommons::UserIdInfo& uid1,
    const CORBACommons::UserIdInfo& uid2)
    noexcept
  {
    unsigned long partition_num = user_partition_(uid1);
    if (partition_num != user_partition_(uid2))
    {
      return false;
    }

    Partition::CPartitionInfo_var partition;
    if(!get_partition_(partition_num, partition))
    {
      return false;
    }

    unsigned long chunk_index1 = user_chunk_(uid1, partition->max_chunk_number_);
    unsigned long chunk_index2 = user_chunk_(uid2, partition->max_chunk_number_);

    return (chunk_index1 == chunk_index2);
  }

#define CALL_MATCHER_(USER_ID, PRECALL_STMT, CALL_NAME, POST_CALL_STMT, THROW_COND, ...) \
  { \
    static const char* FUN = "UserInfoOperationDistributor::" #CALL_NAME "()"; \
    unsigned i = 0; \
    unsigned long chunk_index = 0; \
    for(; i < try_count_; ++i) \
    { \
      unsigned long partition_num = (user_partition_(USER_ID) + i) % try_count_; \
      try \
      { \
        Partition::CPartitionInfo_var partition; \
        if(!get_partition_(partition_num, partition)) \
        { \
          try_to_reresolve_partition_(partition_num); \
          continue; \
        } \
        \
        chunk_index = \
          user_chunk_(USER_ID, partition->max_chunk_number_); \
        \
        auto iter = partition->chunks_ref_map_.find(chunk_index); \
        if (iter == partition->chunks_ref_map_.end()) \
        { \
          try_to_reresolve_partition_(partition_num); \
          continue; \
        } \
        if (is_uim_bad_(iter->second)) \
        { \
          continue; \
        } \
        \
        try \
        { \
          PRECALL_STMT iter->second->uim->CALL_NAME( __VA_ARGS__ ); \
          POST_CALL_STMT; \
        } \
        catch(const AdServer::UserInfoSvcs::UserInfoMatcher::NotReady&) \
        { \
          mark_uim_bad_(iter->second); \
        } \
        catch(AdServer::UserInfoSvcs::UserInfoMatcher::ImplementationException& ex) \
        { \
          mark_uim_bad_(iter->second); \
        } \
        catch(const AdServer::UserInfoSvcs::UserInfoMatcher::ChunkNotFound& e) \
        { \
          mark_uim_bad_(iter->second); \
          try_to_reresolve_partition_(partition_num); \
        } \
        catch(const CORBA::SystemException& ex) \
        { \
          mark_uim_bad_(iter->second); \
          try_to_reresolve_partition_(partition_num); \
        } \
      } \
      catch(const eh::Exception& ex) \
      { \
        Stream::Error ostr; \
        ostr << FUN << ": caught eh::Exception: " << ex.what(); \
        CORBACommons::throw_desc< \
          AdServer::UserInfoSvcs::UserInfoMatcher::ImplementationException>( \
            ostr.str()); \
      } \
    } \
    THROW_COND \
    { \
      Stream::Error ostr; \
      ostr << "max tries reached on chunk #" << chunk_index; \
      throw AdServer::UserInfoSvcs::UserInfoMatcher::ImplementationException(ostr.str().str().c_str()); \
    } \
  }

#define CALL_MATCHER(USER_ID, CALL_NAME, ...) \
  CALL_MATCHER_(USER_ID,, CALL_NAME, return,, ##__VA_ARGS__);

#define CALL_MATCHER_WITH_RETURN(USER_ID, CALL_NAME, ...) \
  CALL_MATCHER_(USER_ID, return, CALL_NAME,,, ##__VA_ARGS__);

#define CALL_MATCHER_WITH_RESULT(USER_ID, CALL_NAME, ...) \
  CALL_MATCHER_(USER_ID, res &= , CALL_NAME, break, if (i == try_count_), ##__VA_ARGS__);

  unsigned long
  UserInfoOperationDistributor::user_partition_(
    const CORBACommons::UserIdInfo& user_id)
    const
    noexcept
  {
    return (CorbaAlgs::unpack_user_id(user_id).hash() >> 8) % try_count_;
  }

  unsigned long
  UserInfoOperationDistributor::user_chunk_(
    const CORBACommons::UserIdInfo& user_id,
    unsigned long max_chunk_number)
    noexcept
  {
    return AdServer::Commons::uuid_distribution_hash(
      CorbaAlgs::unpack_user_id(user_id)) % max_chunk_number;
  }

  CORBA::Boolean
  UserInfoOperationDistributor::merge(
    const AdServer::UserInfoSvcs::UserInfo& user_info,
    const AdServer::UserInfoSvcs::UserInfoMatcher::MatchParams& match_params,
    const AdServer::UserInfoSvcs::UserProfiles& merge_user_profile,
    CORBA::Boolean_out merge_success,
    CORBACommons::TimestampInfo_out last_request)
    /*throw(AdServer::UserInfoSvcs::UserInfoMatcher::NotReady,
      AdServer::UserInfoSvcs::UserInfoMatcher::ImplementationException)*/
  {
    CORBA::Boolean res = true;

    AdServer::Commons::UserId huid = CorbaAlgs::unpack_user_id(user_info.huser_id);

    if (huid.is_null() || the_same_chunk_(user_info.user_id, user_info.huser_id))
    {
      CALL_MATCHER_WITH_RETURN(
        user_info.user_id,
        merge,
        user_info,
        match_params,
        merge_user_profile,
        merge_success,
        last_request);
    }
    else 
    {
      AdServer::UserInfoSvcs::UserInfo temp_uid_info;
      AdServer::UserInfoSvcs::UserInfo temp_hid_info;

      temp_uid_info.user_id = user_info.user_id;
      temp_uid_info.time = user_info.time;
      temp_uid_info.last_colo_id = user_info.last_colo_id;
      temp_uid_info.current_colo_id = user_info.current_colo_id;
      temp_uid_info.request_colo_id = user_info.request_colo_id;
      temp_uid_info.temporary = user_info.temporary;

      temp_hid_info.huser_id = user_info.huser_id;
      temp_hid_info.time = user_info.time;
      temp_hid_info.last_colo_id = user_info.request_colo_id;
      temp_hid_info.current_colo_id = user_info.current_colo_id;
      temp_hid_info.request_colo_id = user_info.request_colo_id;
      temp_hid_info.temporary = false;

      AdServer::UserInfoSvcs::UserInfo uinfo = user_info;
      uinfo.huser_id = CorbaAlgs::pack_user_id(AdServer::Commons::UserId());

      CALL_MATCHER_WITH_RESULT(
        user_info.user_id,
        merge,
        temp_uid_info,
        match_params,
        merge_user_profile,
        merge_success,
        last_request);

      CORBACommons::TimestampInfo_var hid_last_request;

      CALL_MATCHER_WITH_RESULT(
        user_info.huser_id,
        merge,
        temp_hid_info,
        match_params,
        merge_user_profile,
        merge_success,
        hid_last_request);
    }

    return res;
  }

  CORBA::Boolean
  UserInfoOperationDistributor::fraud_user(
    const CORBACommons::UserIdInfo& user_id,
    const CORBACommons::TimestampInfo& time)
    /*throw(AdServer::UserInfoSvcs::UserInfoManager::NotReady,
      AdServer::UserInfoSvcs::UserInfoManager::ImplementationException)*/
  {
    CALL_MATCHER_WITH_RETURN(
      user_id,
      fraud_user,
      user_id,
      time);
  }

  CORBA::Boolean
  UserInfoOperationDistributor::match(
    const AdServer::UserInfoSvcs::UserInfo& user_info,
    const AdServer::UserInfoSvcs::UserInfoMatcher::MatchParams& match_params,
    AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult_out match_result)
    /*throw(AdServer::UserInfoSvcs::UserInfoManager::NotReady,
      AdServer::UserInfoSvcs::UserInfoManager::ImplementationException,
      CORBA::SystemException)*/
  {
    CORBA::Boolean res = false;

    AdServer::Commons::UserId huid = CorbaAlgs::unpack_user_id(user_info.huser_id);

    if (huid.is_null() || the_same_chunk_(user_info.user_id, user_info.huser_id))
    {
      CALL_MATCHER_WITH_RETURN(
        user_info.user_id,
        match,
        user_info,
        match_params,
        match_result);
    }
    else
    {
      AdServer::UserInfoSvcs::UserInfo temp_uid_info;
      AdServer::UserInfoSvcs::UserInfo temp_hid_info;

      temp_uid_info.user_id = user_info.user_id;
      temp_uid_info.time = user_info.time;
      temp_uid_info.last_colo_id = user_info.last_colo_id;
      temp_uid_info.current_colo_id = user_info.current_colo_id;
      temp_uid_info.request_colo_id = user_info.request_colo_id;
      temp_uid_info.temporary = user_info.temporary;

      temp_hid_info.huser_id = user_info.huser_id;
      temp_hid_info.time = user_info.time;
      temp_hid_info.last_colo_id = user_info.request_colo_id;
      temp_hid_info.current_colo_id = user_info.current_colo_id;
      temp_hid_info.request_colo_id = user_info.request_colo_id;
      temp_hid_info.temporary = false;

      AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult_var hid_match_result;

      CALL_MATCHER_WITH_RESULT(
        user_info.user_id,
        match,
        temp_uid_info,
        match_params,
        match_result);

      CALL_MATCHER_WITH_RESULT(
        user_info.huser_id,
        match,
        temp_hid_info,
        match_params,
        hid_match_result);

      match_result->hid_channels = hid_match_result->hid_channels;
    }

    return res;
  }

  CORBA::Boolean
  UserInfoOperationDistributor::get_user_profile(
    const CORBACommons::UserIdInfo& user_id,
    CORBA::Boolean temporary,
    const AdServer::UserInfoSvcs::ProfilesRequestInfo& profile_request,
    AdServer::UserInfoSvcs::UserProfiles_out user_profile)
    /*throw(AdServer::UserInfoSvcs::UserInfoManager::NotReady,
      AdServer::UserInfoSvcs::UserInfoManager::ImplementationException)*/
  {
    CALL_MATCHER_WITH_RETURN(
      user_id,
      get_user_profile,
      user_id,
      temporary,
      profile_request,
      user_profile);
  }

  CORBA::Boolean
  UserInfoOperationDistributor::remove_user_profile(
    const CORBACommons::UserIdInfo& user_id)
    /*throw(AdServer::UserInfoSvcs::UserInfoManager::NotReady,
      AdServer::UserInfoSvcs::UserInfoManager::ImplementationException)*/
  {
    CALL_MATCHER_WITH_RETURN(
      user_id,
      remove_user_profile,
      user_id);
  }

  void
  UserInfoOperationDistributor::update_user_freq_caps(
    const CORBACommons::UserIdInfo& user_id,
    const CORBACommons::TimestampInfo& time,
    const CORBACommons::RequestIdInfo& request_id,
    const AdServer::UserInfoSvcs::FreqCapIdSeq& freq_caps,
    const AdServer::UserInfoSvcs::FreqCapIdSeq& uc_freq_caps,
    const AdServer::UserInfoSvcs::FreqCapIdSeq& virtual_freq_caps,
    const AdServer::UserInfoSvcs::UserInfoManager::SeqOrderSeq& seq_orders,
    const AdServer::UserInfoSvcs::CampaignIdSeq& campaign_ids,
    const AdServer::UserInfoSvcs::CampaignIdSeq& uc_campaign_ids)
    /*throw(AdServer::UserInfoSvcs::UserInfoManager::NotReady,
      AdServer::UserInfoSvcs::UserInfoManager::ImplementationException)*/
  {
    CALL_MATCHER(
      user_id,
      update_user_freq_caps,
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

  void
  UserInfoOperationDistributor::confirm_user_freq_caps(
    const CORBACommons::UserIdInfo& user_id,
    const CORBACommons::TimestampInfo& time,
    const CORBACommons::RequestIdInfo& request_id,
    const CORBACommons::IdSeq& exclude_pubpixel_accounts)
    /*throw(AdServer::UserInfoSvcs::UserInfoManager::NotReady,
      AdServer::UserInfoSvcs::UserInfoManager::ImplementationException)*/
  {
    CALL_MATCHER(
      user_id,
      confirm_user_freq_caps,
      user_id,
      time,
      request_id,
      exclude_pubpixel_accounts);
  }
  
  void
  UserInfoOperationDistributor::consider_publishers_optin(
    const CORBACommons::UserIdInfo& user_id,
    const CORBACommons::IdSeq& exclude_pubpixel_accounts,
    const CORBACommons::TimestampInfo& now)
    /*throw(AdServer::UserInfoSvcs::UserInfoManager::NotReady,
      AdServer::UserInfoSvcs::UserInfoManager::ImplementationException)*/
  {
    CALL_MATCHER(
      user_id,
      consider_publishers_optin,
      user_id,
      exclude_pubpixel_accounts,
      now);
  }
  
  void
  UserInfoOperationDistributor::clear_expired(
    CORBA::Boolean /*synch*/,
    const CORBACommons::TimestampInfo& /*cleanup_time*/,
    CORBA::Long /*portion*/)
    /*throw(AdServer::UserInfoSvcs::UserInfoManager::ImplementationException)*/
  {
    throw AdServer::UserInfoSvcs::UserInfoManager::ImplementationException(
      "UserInfoOperationDistributor::clear_expired(): unsupported");
  }
}
}
