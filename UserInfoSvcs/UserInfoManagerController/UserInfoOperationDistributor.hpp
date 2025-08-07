#ifndef USERINFOOPERATIONDISTRIBUTOR_HPP
#define USERINFOOPERATIONDISTRIBUTOR_HPP

#include <CORBACommons/CorbaAdapters.hpp>
#include <CORBACommons/ObjectPool.hpp>

#include <Generics/CompositeActiveObject.hpp>
#include <Generics/TaskRunner.hpp>

#include <UserInfoSvcs/UserInfoManager/UserInfoManager.hpp>
#include <UserInfoSvcs/UserInfoManagerController/UserInfoManagerController.hpp>
#include "UserInfoManagerSessionImpl.h"

namespace AdServer
{
namespace UserInfoSvcs
{
  class UserInfoOperationDistributor:
    public CORBACommons::ReferenceCounting::CorbaRefCountImpl<
      OBV_AdServer::UserInfoSvcs::UserInfoManagerSession>,
    public Generics::CompositeActiveObject
  {
  public:
    typedef CORBACommons::CorbaObjectRefList ControllerRef;
    typedef std::list<ControllerRef> ControllerRefList;

  public:
    UserInfoOperationDistributor(
      Logging::Logger* logger,
      const ControllerRefList& controller_refs,
      const CORBACommons::CorbaClientAdapter* corba_client_adapter,
      const Generics::Time& pool_timeout = Generics::Time::ONE_SECOND);

    virtual ~UserInfoOperationDistributor() noexcept;

    /* UserInfoMatcher interface */
    virtual CORBA::Boolean
    merge(
      const AdServer::UserInfoSvcs::UserInfo& user_info,
      const AdServer::UserInfoSvcs::UserInfoMatcher::MatchParams& match_params,
      const AdServer::UserInfoSvcs::UserProfiles& merge_user_profile,
      CORBA::Boolean_out merge_success,
      CORBACommons::TimestampInfo_out last_request)
      /*throw(
        AdServer::UserInfoSvcs::UserInfoMatcher::NotReady,
        AdServer::UserInfoSvcs::UserInfoMatcher::ImplementationException)*/;

    virtual CORBA::Boolean
    fraud_user(
      const CORBACommons::UserIdInfo& user_id,
      const CORBACommons::TimestampInfo& time)
      /*throw(AdServer::UserInfoSvcs::UserInfoManager::NotReady,
        AdServer::UserInfoSvcs::UserInfoManager::ImplementationException)*/;

    virtual CORBA::Boolean
    match(
      const AdServer::UserInfoSvcs::UserInfo& user_info,
      const AdServer::UserInfoSvcs::UserInfoMatcher::MatchParams& match_params,
      AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult_out match_result)
      /*throw(
        AdServer::UserInfoSvcs::UserInfoManager::NotReady,
        AdServer::UserInfoSvcs::UserInfoManager::ImplementationException,
        CORBA::SystemException)*/;

    virtual CORBA::Boolean
    get_user_profile(
      const CORBACommons::UserIdInfo& user_id,
      CORBA::Boolean temporary,
      const AdServer::UserInfoSvcs::ProfilesRequestInfo& profile_request,
      AdServer::UserInfoSvcs::UserProfiles_out user_profile)
      /*throw(AdServer::UserInfoSvcs::UserInfoManager::NotReady,
        AdServer::UserInfoSvcs::UserInfoManager::ImplementationException)*/;

    virtual void get_user_channels(
      const ::CORBACommons::UserIdInfo& user_id,
      const ::AdServer::UserInfoSvcs::ProfilesRequestInfo& profile_request,
      const ::AdServer::UserInfoSvcs::WlChannelIdSeq& wl_channel_ids,
      ::AdServer::UserInfoSvcs::ChannelIdSeq_out channel_ids);

    virtual CORBA::Boolean
    remove_user_profile(
      const CORBACommons::UserIdInfo& user_id_info)
      /*throw(AdServer::UserInfoSvcs::UserInfoManager::NotReady,
        AdServer::UserInfoSvcs::UserInfoManager::ImplementationException)*/;

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
        AdServer::UserInfoSvcs::UserInfoManager::ImplementationException)*/;

    virtual void
    confirm_user_freq_caps(
      const CORBACommons::UserIdInfo& user_id,
      const CORBACommons::TimestampInfo& time,
      const CORBACommons::RequestIdInfo& request_id,
      const CORBACommons::IdSeq& exclude_pubpixel_accounts)
      /*throw(AdServer::UserInfoSvcs::UserInfoManager::NotReady,
        AdServer::UserInfoSvcs::UserInfoManager::ImplementationException)*/;

    virtual void
    consider_publishers_optin(
      const CORBACommons::UserIdInfo& user_id,
      const CORBACommons::IdSeq& exclude_pubpixel_accounts,
      const CORBACommons::TimestampInfo& now)
      /*throw(AdServer::UserInfoSvcs::UserInfoManager::NotReady,
        AdServer::UserInfoSvcs::UserInfoManager::ImplementationException)*/;
    
    virtual void
    clear_expired(
      CORBA::Boolean synch,
      const CORBACommons::TimestampInfo& cleanup_time,
      CORBA::Long portion)
      /*throw(AdServer::UserInfoSvcs::UserInfoManager::ImplementationException)*/;

  private:
    class PartitionResolver
      : public Generics::Task,
        public ReferenceCounting::AtomicImpl
    {
    public:
      PartitionResolver(
        UserInfoOperationDistributor& distributor,
        unsigned int partition_num)
        noexcept;

      virtual void execute() noexcept;

    protected:
      virtual
      ~PartitionResolver() noexcept
      {}

      UserInfoOperationDistributor& distributor_;
      unsigned int partition_num_;
    };

    typedef Sync::PosixMutex Mutex;
    typedef Sync::PosixGuard Guard;

    struct Partition
    {
      struct UIMInfo:
        public ReferenceCounting::AtomicImpl
      {
        UIMInfo(AdServer::UserInfoSvcs::UserInfoMatcher* uim_ref)
          : uim(AdServer::UserInfoSvcs::UserInfoMatcher::_duplicate(uim_ref)),
            is_bad(false)
        {}

        const AdServer::UserInfoSvcs::UserInfoMatcher_var uim;

        Mutex lock;
        Generics::Time marked_as_bad;
        bool is_bad;

      protected:
        virtual ~UIMInfo() noexcept
        {}
      };

      typedef ReferenceCounting::SmartPtr<UIMInfo>
        UIMInfo_var;

      struct PartitionInfo:
        public ReferenceCounting::AtomicImpl
      {
        PartitionInfo()
          : max_chunk_number_(0)
        {}

        std::map<unsigned long, UIMInfo_var> chunks_ref_map_;
        unsigned int max_chunk_number_;

      protected:
        virtual ~PartitionInfo() noexcept
        {}
      };

      typedef ReferenceCounting::SmartPtr<PartitionInfo>
        PartitionInfo_var;
      typedef ReferenceCounting::ConstPtr<PartitionInfo>
        CPartitionInfo_var;

      Partition()
        : resolve_in_progress_(false)
      {}

      PartitionInfo_var partition_info_;
      Generics::Time last_try_to_resolve_;
      bool resolve_in_progress_;
    };

    typedef std::vector<Partition> PartitionsPool;

  private:
    unsigned long
    user_partition_(const CORBACommons::UserIdInfo& user_id)
      const noexcept;

    unsigned long user_chunk_(
      const CORBACommons::UserIdInfo& user_id,
      unsigned long max_chunk_number)
      noexcept;

    void init_pool_()
      /*throw(eh::Exception, CORBACommons::CorbaClientAdapter::Exception)*/;

    void try_to_reresolve_partition_(unsigned int partition_num)
      noexcept;

    void resolve_partition_(unsigned int partition_num)
      noexcept;

    bool
    is_uim_bad_(Partition::UIMInfo* uim_info)
      const noexcept;

    static void
    mark_uim_bad_(Partition::UIMInfo* uim_info)
      noexcept;

    bool get_partition_(
      unsigned long partition_num,
      Partition::CPartitionInfo_var& partition)
      noexcept;

    bool the_same_chunk_(
      const CORBACommons::UserIdInfo& uid1,
      const CORBACommons::UserIdInfo& uid2)
      noexcept;

  private:
    Generics::ActiveObjectCallback_var callback_;
    const unsigned long try_count_;
    PartitionsPool partitions_pool_;
    Mutex partition_lock_;
    ControllerRefList controller_refs_;
    CORBACommons::CorbaClientAdapter_var corba_client_adapter_;

    Generics::TaskRunner_var task_runner_;
    Generics::Time pool_timeout_;
  };

  typedef ReferenceCounting::SmartPtr<UserInfoOperationDistributor>
    UserInfoOperationDistributor_var;
}
}

#endif /*USERINFOOPERATIONDISTRIBUTOR_HPP*/
