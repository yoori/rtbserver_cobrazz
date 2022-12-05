#ifndef _USER_INFO_SVCS_USER_INFO_MANAGER_SESSION_IMPL_HPP_
#define _USER_INFO_SVCS_USER_INFO_MANAGER_SESSION_IMPL_HPP_

#include <map>

#include <CORBACommons/CorbaAdapters.hpp>
#include <CORBACommons/StatsImpl.hpp>

#include <eh/Exception.hpp>
#include <Sync/SyncPolicy.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

#include <UserInfoSvcs/UserInfoManagerController/UserInfoManagerController.hpp>

namespace AdServer
{
  namespace UserInfoSvcs
  {
    /**
     * UserInfoManagerSessionImpl
     * client side implementation of UserInfoManagerSession valuetype
     */
    class UserInfoManagerSessionImpl:
      public virtual OBV_AdServer::UserInfoSvcs::UserInfoManagerSession
    {
    public:
      UserInfoManagerSessionImpl() /*throw(eh::Exception)*/;

      UserInfoManagerSessionImpl(
        const UserInfoManagerSessionImpl& init)
        /*throw(eh::Exception)*/;

      UserInfoManagerSessionImpl(
        const AdServer::UserInfoSvcs::UserInfoManagerDescriptionSeq&
        user_info_managers__) /*throw(eh::Exception)*/;

      virtual ~UserInfoManagerSessionImpl() throw();

      virtual CORBA::ValueBase* _copy_value();

      /* UserInfoMatcher interface */
      virtual CORBA::Boolean merge(
        const AdServer::UserInfoSvcs::UserInfo& user_info,
        const AdServer::UserInfoSvcs::UserInfoMatcher::MatchParams& match_params,
        const AdServer::UserInfoSvcs::UserProfiles& merge_user_profile,
        CORBA::Boolean_out merge_success,
        CORBACommons::TimestampInfo_out last_request)
        /*throw(
          AdServer::UserInfoSvcs::UserInfoManager::NotReady,
          AdServer::UserInfoSvcs::UserInfoManager::ImplementationException,
          AdServer::UserInfoSvcs::UserInfoManager::ChunkNotFound)*/;

      virtual CORBA::Boolean fraud_user(
        const CORBACommons::UserIdInfo& user_id,
        const CORBACommons::TimestampInfo& time)
        /*throw(AdServer::UserInfoSvcs::UserInfoManager::NotReady,
              AdServer::UserInfoSvcs::UserInfoManager::ImplementationException,
              AdServer::UserInfoSvcs::UserInfoManager::ChunkNotFound)*/;

      virtual CORBA::Boolean match(
        const AdServer::UserInfoSvcs::UserInfo& user_info,
        const AdServer::UserInfoSvcs::UserInfoMatcher::MatchParams& match_params,
        AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult_out match_result)
        /*throw(
          AdServer::UserInfoSvcs::UserInfoManager::NotReady,
          AdServer::UserInfoSvcs::UserInfoManager::ImplementationException,
          AdServer::UserInfoSvcs::UserInfoManager::ChunkNotFound,
          CORBA::SystemException)*/;

      virtual void get_master_stamp(
        CORBACommons::TimestampInfo_out master_stamp)
        /*throw(AdServer::UserInfoSvcs::UserInfoManager::NotReady,
              AdServer::UserInfoSvcs::UserInfoManager::ImplementationException)*/;

      virtual CORBA::Boolean
      get_user_profile(
        const CORBACommons::UserIdInfo& user_id,
        CORBA::Boolean temporary,
        const AdServer::UserInfoSvcs::ProfilesRequestInfo& profile_request,
        AdServer::UserInfoSvcs::UserProfiles_out user_profile)
        /*throw(
          AdServer::UserInfoSvcs::UserInfoManager::NotReady,
          AdServer::UserInfoSvcs::UserInfoManager::ImplementationException,
          AdServer::UserInfoSvcs::UserInfoManager::ChunkNotFound)*/;

      virtual CORBA::Boolean
      remove_user_profile(
        const CORBACommons::UserIdInfo& user_id_info)
        /*throw(
          AdServer::UserInfoSvcs::UserInfoManager::NotReady,
          AdServer::UserInfoSvcs::UserInfoManager::ImplementationException,
          AdServer::UserInfoSvcs::UserInfoManager::ChunkNotFound)*/;

      virtual void
      update_user_freq_caps(
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
          AdServer::UserInfoSvcs::UserInfoManager::ImplementationException,
          AdServer::UserInfoSvcs::UserInfoManager::ChunkNotFound)*/;

      virtual void confirm_user_freq_caps(
        const CORBACommons::UserIdInfo& user_id,
        const CORBACommons::TimestampInfo& time,
        const CORBACommons::RequestIdInfo& request_id,
        const CORBACommons::IdSeq& exclude_pubpixel_accounts)
        /*throw(AdServer::UserInfoSvcs::UserInfoManager::NotReady,
              AdServer::UserInfoSvcs::UserInfoManager::ImplementationException,
              AdServer::UserInfoSvcs::UserInfoManager::ChunkNotFound)*/;

      virtual void consider_publishers_optin(
        const CORBACommons::UserIdInfo& user_id,
        const CORBACommons::IdSeq& exclude_pubpixel_accounts,
        const CORBACommons::TimestampInfo& now)
        /*throw(AdServer::UserInfoSvcs::UserInfoManager::NotReady,
              AdServer::UserInfoSvcs::UserInfoManager::ImplementationException,
              AdServer::UserInfoSvcs::UserInfoManager::ChunkNotFound)*/;
     
      virtual void clear_expired(
        CORBA::Boolean synch,
        const CORBACommons::TimestampInfo& cleanup_time,
        CORBA::Long portion)
        /*throw(AdServer::UserInfoSvcs::UserInfoManager::ImplementationException)*/;

    protected:
      void init_() throw();
      mutable Sync::Policy::PosixThread::Mutex init_lock_;

      unsigned long
      get_user_host_index_(const CORBACommons::UserIdInfo& user_id_info)
        /*throw(
          AdServer::UserInfoSvcs::UserInfoManager::NotReady,
          AdServer::UserInfoSvcs::UserInfoManager::ImplementationException)*/;

      AdServer::UserInfoSvcs::UserInfoManager*
      get_user_host_(unsigned long host_index)
        /*throw(
          AdServer::UserInfoSvcs::UserInfoManager::NotReady,
          AdServer::UserInfoSvcs::UserInfoManager::ImplementationException)*/;

      typedef
        std::map<unsigned long, AdServer::UserInfoSvcs::UserInfoManager_var>
        ChunkRefMap;

      ChunkRefMap chunk_refs_;

      unsigned int max_chunk_number_;
      bool inited_;
    };

  } /* UserInfoSvcs */
} /* AdServer */

#endif /*_USER_INFO_SVCS_USER_INFO_MANAGER_SESSION_IMPL_HPP_*/
