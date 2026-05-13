#include "UserInfoCorbaClient.hpp"

#include <algorithm>
#include <string>
#include <utility>

#include <Commons/CorbaAlgs.hpp>
#include <Commons/Grpc/GrpcServer.hpp>
#include <Commons/Grpc/GrpcSync.hpp>
#include <Commons/UserInfoManip.hpp>
#include <Generics/TaskRunner.hpp>
#include <UserInfoSvcs/UserInfoManagerController/UserInfoManagerController.hpp>
#include <UserInfoSvcs/UserInfoManagerController/UserInfoManagerSessionImpl.h>

namespace AdServer::UserInfoSvcs
{
  namespace
  {
    namespace pb = adserver::user_info_svcs::user_info_manager;

    template<typename CorbaSeq>
    void bytes_to_oct_seq_(const std::string& src, CorbaSeq& dst)
    {
      dst.length(src.size());
      std::copy(src.begin(), src.end(), dst.get_buffer());
    }

    template<typename CorbaSeq>
    std::string oct_seq_to_bytes_(const CorbaSeq& src)
    {
      return std::string(
        reinterpret_cast<const char*>(src.get_buffer()),
        reinterpret_cast<const char*>(src.get_buffer() + src.length()));
    }

    template<typename CorbaSeq, typename Repeated>
    void repeated_to_id_seq_(const Repeated& src, CorbaSeq& dst)
    {
      dst.length(src.size());
      for(int i = 0; i < src.size(); ++i)
      {
        dst[i] = src.Get(i);
      }
    }

    template<typename CorbaSeq, typename Repeated>
    void id_seq_to_repeated_(const CorbaSeq& src, Repeated* dst)
    {
      for(CORBA::ULong i = 0; i < src.length(); ++i)
      {
        dst->Add(src[i]);
      }
    }

    grpc::Status exception_status_(
      grpc::StatusCode code,
      const char* description)
    {
      return AdServer::Grpc::error_status(code, description);
    }

    grpc::Status exception_status_(const CORBA::SystemException& ex)
    {
      Stream::Error ostr;
      ostr << ex;
      return grpc::Status(grpc::StatusCode::UNAVAILABLE, ostr.str().str());
    }

    grpc::Status exception_status_(const eh::Exception& ex)
    {
      return grpc::Status(grpc::StatusCode::INTERNAL, ex.what());
    }

    grpc::Status unsupported_status_(const char* method)
    {
      Stream::Error ostr;
      ostr << "UserInfoCorbaClient::" << method <<
        "(): unsupported by CORBA distributor";
      return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, ostr.str().str());
    }

    void convert_(const pb::GeoData& src, GeoData& dst)
    {
      bytes_to_oct_seq_(src.latitude(), dst.latitude);
      bytes_to_oct_seq_(src.longitude(), dst.longitude);
      bytes_to_oct_seq_(src.accuracy(), dst.accuracy);
    }

    void convert_(const GeoData& src, pb::GeoData& dst)
    {
      dst.set_latitude(oct_seq_to_bytes_(src.latitude));
      dst.set_longitude(oct_seq_to_bytes_(src.longitude));
      dst.set_accuracy(oct_seq_to_bytes_(src.accuracy));
    }

    void convert_(const pb::UserInfo& src, UserInfo& dst)
    {
      bytes_to_oct_seq_(src.user_id(), dst.user_id);
      bytes_to_oct_seq_(src.huser_id(), dst.huser_id);
      dst.time = src.time();
      dst.last_colo_id = src.last_colo_id();
      dst.current_colo_id = src.current_colo_id();
      dst.request_colo_id = src.request_colo_id();
      dst.temporary = src.temporary();
    }

    void convert_(const UserInfo& src, pb::UserInfo& dst)
    {
      dst.set_user_id(oct_seq_to_bytes_(src.user_id));
      dst.set_huser_id(oct_seq_to_bytes_(src.huser_id));
      dst.set_time(src.time);
      dst.set_last_colo_id(src.last_colo_id);
      dst.set_current_colo_id(src.current_colo_id);
      dst.set_request_colo_id(src.request_colo_id);
      dst.set_temporary(src.temporary);
    }

    void convert_(const pb::ProfilesRequestInfo& src, ProfilesRequestInfo& dst)
    {
      dst.base_profile = src.base_profile();
      dst.add_profile = src.add_profile();
      dst.history_profile = src.history_profile();
      dst.freq_cap_profile = src.freq_cap_profile();
      dst.pref_profile = src.pref_profile();
    }

    void convert_(const ProfilesRequestInfo& src, pb::ProfilesRequestInfo& dst)
    {
      dst.set_base_profile(src.base_profile);
      dst.set_add_profile(src.add_profile);
      dst.set_history_profile(src.history_profile);
      dst.set_freq_cap_profile(src.freq_cap_profile);
      dst.set_pref_profile(src.pref_profile);
    }

    void convert_(const pb::UserProfiles& src, UserProfiles& dst)
    {
      bytes_to_oct_seq_(src.base_user_profile(), dst.base_user_profile);
      bytes_to_oct_seq_(src.add_user_profile(), dst.add_user_profile);
      bytes_to_oct_seq_(src.history_user_profile(), dst.history_user_profile);
      bytes_to_oct_seq_(src.freq_cap(), dst.freq_cap);
      bytes_to_oct_seq_(src.pref_profile(), dst.pref_profile);
    }

    void convert_(const UserProfiles& src, pb::UserProfiles& dst)
    {
      dst.set_base_user_profile(oct_seq_to_bytes_(src.base_user_profile));
      dst.set_add_user_profile(oct_seq_to_bytes_(src.add_user_profile));
      dst.set_history_user_profile(oct_seq_to_bytes_(src.history_user_profile));
      dst.set_freq_cap(oct_seq_to_bytes_(src.freq_cap));
      dst.set_pref_profile(oct_seq_to_bytes_(src.pref_profile));
    }

    void convert_(
      const pb::ChannelTriggerMatch& src,
      UserInfoMatcher::ChannelTriggerMatch& dst)
    {
      dst.channel_id = src.channel_id();
      dst.channel_trigger_id = src.channel_trigger_id();
    }

    void convert_(
      const UserInfoMatcher::ChannelTriggerMatch& src,
      pb::ChannelTriggerMatch& dst)
    {
      dst.set_channel_id(src.channel_id);
      dst.set_channel_trigger_id(src.channel_trigger_id);
    }

    template<typename PbRepeated, typename CorbaSeq>
    void convert_channel_match_seq_(const PbRepeated& src, CorbaSeq& dst)
    {
      dst.length(src.size());
      for(int i = 0; i < src.size(); ++i)
      {
        convert_(src.Get(i), dst[i]);
      }
    }

    void convert_(const pb::MatchParams& src, UserInfoMatcher::MatchParams& dst)
    {
      repeated_to_id_seq_(src.persistent_channel_ids(), dst.persistent_channel_ids);
      convert_channel_match_seq_(src.search_channel_ids(), dst.search_channel_ids);
      convert_channel_match_seq_(src.page_channel_ids(), dst.page_channel_ids);
      convert_channel_match_seq_(src.url_channel_ids(), dst.url_channel_ids);
      convert_channel_match_seq_(
        src.url_keyword_channel_ids(),
        dst.url_keyword_channel_ids);
      bytes_to_oct_seq_(src.publishers_optin_timeout(), dst.publishers_optin_timeout);
      dst.use_empty_profile = src.use_empty_profile();
      dst.silent_match = src.silent_match();
      dst.no_match = src.no_match();
      dst.no_result = src.no_result();
      dst.ret_freq_caps = src.ret_freq_caps();
      dst.provide_channel_count = src.provide_channel_count();
      dst.provide_persistent_channels = src.provide_persistent_channels();
      dst.change_last_request = src.change_last_request();
      dst.cohort = src.cohort().c_str();
      dst.cohort2 = src.cohort2().c_str();
      dst.filter_contextual_triggers = src.filter_contextual_triggers();
      dst.geo_data_seq.length(src.geo_data_seq_size());
      for(int i = 0; i < src.geo_data_seq_size(); ++i)
      {
        convert_(src.geo_data_seq(i), dst.geo_data_seq[i]);
      }
    }

    template<typename CorbaSeq, typename PbRepeated>
    void convert_channel_match_seq_(const CorbaSeq& src, PbRepeated* dst)
    {
      for(CORBA::ULong i = 0; i < src.length(); ++i)
      {
        convert_(src[i], *dst->Add());
      }
    }

    void convert_(const UserInfoMatcher::MatchParams& src, pb::MatchParams& dst)
    {
      id_seq_to_repeated_(
        src.persistent_channel_ids,
        dst.mutable_persistent_channel_ids());
      convert_channel_match_seq_(
        src.search_channel_ids,
        dst.mutable_search_channel_ids());
      convert_channel_match_seq_(
        src.page_channel_ids,
        dst.mutable_page_channel_ids());
      convert_channel_match_seq_(
        src.url_channel_ids,
        dst.mutable_url_channel_ids());
      convert_channel_match_seq_(
        src.url_keyword_channel_ids,
        dst.mutable_url_keyword_channel_ids());
      dst.set_publishers_optin_timeout(oct_seq_to_bytes_(
        src.publishers_optin_timeout));
      dst.set_use_empty_profile(src.use_empty_profile);
      dst.set_silent_match(src.silent_match);
      dst.set_no_match(src.no_match);
      dst.set_no_result(src.no_result);
      dst.set_ret_freq_caps(src.ret_freq_caps);
      dst.set_provide_channel_count(src.provide_channel_count);
      dst.set_provide_persistent_channels(src.provide_persistent_channels);
      dst.set_change_last_request(src.change_last_request);
      dst.set_cohort(src.cohort.in());
      dst.set_cohort2(src.cohort2.in());
      dst.set_filter_contextual_triggers(src.filter_contextual_triggers);
      for(CORBA::ULong i = 0; i < src.geo_data_seq.length(); ++i)
      {
        convert_(src.geo_data_seq[i], *dst.add_geo_data_seq());
      }
    }

    void convert_(const UserInfoMatcher::ChannelWeight& src, pb::ChannelWeight& dst)
    {
      dst.set_channel_id(src.channel_id);
      dst.set_weight(src.weight);
    }

    void convert_(const UserInfoMatcher::SeqOrderInfo& src, pb::SeqOrderInfo& dst)
    {
      dst.set_ccg_id(src.ccg_id);
      dst.set_set_id(src.set_id);
      dst.set_imps(src.imps);
    }

    void convert_(const pb::SeqOrderInfo& src, UserInfoMatcher::SeqOrderInfo& dst)
    {
      dst.ccg_id = src.ccg_id();
      dst.set_id = src.set_id();
      dst.imps = src.imps();
    }

    void convert_(const Commons::CampaignFreq& src, pb::CampaignFreq& dst)
    {
      dst.set_campaign_id(src.campaign_id);
      dst.set_imps(src.imps);
    }

    void convert_(
      const pb::ChannelWeight& src,
      UserInfoMatcher::ChannelWeight& dst)
    {
      dst.channel_id = src.channel_id();
      dst.weight = src.weight();
    }

    void convert_(
      const pb::CampaignFreq& src,
      Commons::CampaignFreq& dst)
    {
      dst.campaign_id = src.campaign_id();
      dst.imps = src.imps();
    }

    void convert_(const UserInfoMatcher::MatchResult& src, pb::MatchResult& dst)
    {
      dst.set_times_inited(src.times_inited);
      dst.set_last_request_time(oct_seq_to_bytes_(src.last_request_time));
      dst.set_create_time(oct_seq_to_bytes_(src.create_time));
      dst.set_session_start(oct_seq_to_bytes_(src.session_start));
      dst.set_colo_id(src.colo_id);
      for(CORBA::ULong i = 0; i < src.channels.length(); ++i)
      {
        convert_(src.channels[i], *dst.add_channels());
      }
      for(CORBA::ULong i = 0; i < src.hid_channels.length(); ++i)
      {
        convert_(src.hid_channels[i], *dst.add_hid_channels());
      }
      id_seq_to_repeated_(src.full_freq_caps, dst.mutable_full_freq_caps());
      id_seq_to_repeated_(
        src.full_virtual_freq_caps,
        dst.mutable_full_virtual_freq_caps());
      for(CORBA::ULong i = 0; i < src.seq_orders.length(); ++i)
      {
        convert_(src.seq_orders[i], *dst.add_seq_orders());
      }
      for(CORBA::ULong i = 0; i < src.campaign_freqs.length(); ++i)
      {
        convert_(src.campaign_freqs[i], *dst.add_campaign_freqs());
      }
      dst.set_fraud_request(src.fraud_request);
      dst.set_process_time(oct_seq_to_bytes_(src.process_time));
      dst.set_adv_channel_count(src.adv_channel_count);
      dst.set_discover_channel_count(src.discover_channel_count);
      dst.set_cohort(src.cohort.in());
      dst.set_cohort2(src.cohort2.in());
      id_seq_to_repeated_(
        src.exclude_pubpixel_accounts,
        dst.mutable_exclude_pubpixel_accounts());
      for(CORBA::ULong i = 0; i < src.geo_data_seq.length(); ++i)
      {
        convert_(src.geo_data_seq[i], *dst.add_geo_data_seq());
      }
    }

    void convert_(const pb::MatchResult& src, UserInfoMatcher::MatchResult& dst)
    {
      dst.times_inited = src.times_inited();
      bytes_to_oct_seq_(src.last_request_time(), dst.last_request_time);
      bytes_to_oct_seq_(src.create_time(), dst.create_time);
      bytes_to_oct_seq_(src.session_start(), dst.session_start);
      dst.colo_id = src.colo_id();
      dst.channels.length(src.channels_size());
      for(int i = 0; i < src.channels_size(); ++i)
      {
        convert_(src.channels(i), dst.channels[i]);
      }
      dst.hid_channels.length(src.hid_channels_size());
      for(int i = 0; i < src.hid_channels_size(); ++i)
      {
        convert_(src.hid_channels(i), dst.hid_channels[i]);
      }
      repeated_to_id_seq_(src.full_freq_caps(), dst.full_freq_caps);
      repeated_to_id_seq_(
        src.full_virtual_freq_caps(),
        dst.full_virtual_freq_caps);
      dst.seq_orders.length(src.seq_orders_size());
      for(int i = 0; i < src.seq_orders_size(); ++i)
      {
        convert_(src.seq_orders(i), dst.seq_orders[i]);
      }
      dst.campaign_freqs.length(src.campaign_freqs_size());
      for(int i = 0; i < src.campaign_freqs_size(); ++i)
      {
        convert_(src.campaign_freqs(i), dst.campaign_freqs[i]);
      }
      dst.fraud_request = src.fraud_request();
      bytes_to_oct_seq_(src.process_time(), dst.process_time);
      dst.adv_channel_count = src.adv_channel_count();
      dst.discover_channel_count = src.discover_channel_count();
      dst.cohort = src.cohort().c_str();
      dst.cohort2 = src.cohort2().c_str();
      repeated_to_id_seq_(
        src.exclude_pubpixel_accounts(),
        dst.exclude_pubpixel_accounts);
      dst.geo_data_seq.length(src.geo_data_seq_size());
      for(int i = 0; i < src.geo_data_seq_size(); ++i)
      {
        convert_(src.geo_data_seq(i), dst.geo_data_seq[i]);
      }
    }

    UserInfoMatcher::MatchResult*
    convert_history_match_result_(const pb::MatchResult& response)
    {
      UserInfoMatcher::MatchResult_var result =
        new UserInfoMatcher::MatchResult;
      convert_(response, *result);
      return result._retn();
    }

    void throw_user_info_exception_(const grpc::Status& status)
    {
      const std::string message = status.error_message();
      switch(status.error_code())
      {
      case grpc::StatusCode::UNAVAILABLE:
        throw UserInfoMatcher::NotReady(message.c_str());
      case grpc::StatusCode::NOT_FOUND:
        throw UserInfoMatcher::ChunkNotFound(message.c_str());
      default:
        throw UserInfoMatcher::ImplementationException(message.c_str());
      }
    }

    template<typename PbRepeated, typename CorbaSeq>
    void convert_seq_order_seq_(const PbRepeated& src, CorbaSeq& dst)
    {
      dst.length(src.size());
      for(int i = 0; i < src.size(); ++i)
      {
        convert_(src.Get(i), dst[i]);
      }
    }
  }


  class UserInfoCorbaClient::Distributor:
    public CORBACommons::ReferenceCounting::CorbaRefCountImpl<
      OBV_AdServer::UserInfoSvcs::UserInfoManagerSession>,
    public Generics::CompositeActiveObject,
    public virtual Generics::RefCountableActiveObject
  {
  public:
    typedef CORBACommons::CorbaObjectRefList ControllerRef;
    typedef std::list<ControllerRef> ControllerRefList;

  public:
    Distributor(
      Logging::Logger* logger,
      const ControllerRefList& controller_refs,
      const CORBACommons::CorbaClientAdapter* corba_client_adapter,
      const Generics::Time& pool_timeout = Generics::Time::ONE_SECOND);

    virtual ~Distributor() noexcept;

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
        Distributor& distributor,
        unsigned int partition_num)
        noexcept;

      virtual void execute() noexcept;

    protected:
      virtual
      ~PartitionResolver() noexcept
      {}

      Distributor& distributor_;
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



  UserInfoCorbaClient::Distributor::PartitionResolver::PartitionResolver(
    UserInfoCorbaClient::Distributor& distributor,
    unsigned int partition_num)
    noexcept
    : distributor_(distributor),
      partition_num_(partition_num)
  {}

  void
  UserInfoCorbaClient::Distributor::PartitionResolver::execute()
    noexcept
  {
    distributor_.resolve_partition_(partition_num_);
  }

  // UserInfoCorbaClient::Distributor
  UserInfoCorbaClient::Distributor::Distributor(
    Logging::Logger* logger,
    const ControllerRefList& controller_refs,
    const CORBACommons::CorbaClientAdapter* corba_client_adapter,
    const Generics::Time& pool_timeout)
    : callback_(Generics::ActiveObjectCallback_var(
        new Logging::ActiveObjectCallbackImpl(
          logger,
          "UserInfoCorbaClient::Distributor",
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
      ostr << "UserInfoCorbaClient::Distributor: CompositeActiveObject::Exception caught: " <<
        ex.what();
      throw Exception(ostr);
    }

    init_pool_();
  }

  UserInfoCorbaClient::Distributor::~Distributor()
    noexcept
  {}

  void
  UserInfoCorbaClient::Distributor::init_pool_()
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
  UserInfoCorbaClient::Distributor::try_to_reresolve_partition_(unsigned int partition_num)
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
  UserInfoCorbaClient::Distributor::resolve_partition_(unsigned int partition_num)
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
  UserInfoCorbaClient::Distributor::is_uim_bad_(Partition::UIMInfo* uim_info)
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
  UserInfoCorbaClient::Distributor::mark_uim_bad_(Partition::UIMInfo* uim_info)
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
  UserInfoCorbaClient::Distributor::get_partition_(
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
  UserInfoCorbaClient::Distributor::the_same_chunk_(
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
    static const char* FUN = "UserInfoCorbaClient::Distributor::" #CALL_NAME "()"; \
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
  UserInfoCorbaClient::Distributor::user_partition_(
    const CORBACommons::UserIdInfo& user_id)
    const
    noexcept
  {
    return (CorbaAlgs::unpack_user_id(user_id).hash() >> 8) % try_count_;
  }

  unsigned long
  UserInfoCorbaClient::Distributor::user_chunk_(
    const CORBACommons::UserIdInfo& user_id,
    unsigned long max_chunk_number)
    noexcept
  {
    return AdServer::Commons::uuid_distribution_hash(
      CorbaAlgs::unpack_user_id(user_id)) % max_chunk_number;
  }

  CORBA::Boolean
  UserInfoCorbaClient::Distributor::merge(
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
  UserInfoCorbaClient::Distributor::fraud_user(
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
  UserInfoCorbaClient::Distributor::match(
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
  UserInfoCorbaClient::Distributor::get_user_profile(
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
  UserInfoCorbaClient::Distributor::remove_user_profile(
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
  UserInfoCorbaClient::Distributor::update_user_freq_caps(
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
  UserInfoCorbaClient::Distributor::confirm_user_freq_caps(
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
  UserInfoCorbaClient::Distributor::consider_publishers_optin(
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
  UserInfoCorbaClient::Distributor::clear_expired(
    CORBA::Boolean /*synch*/,
    const CORBACommons::TimestampInfo& /*cleanup_time*/,
    CORBA::Long /*portion*/)
    /*throw(AdServer::UserInfoSvcs::UserInfoManager::ImplementationException)*/
  {
    throw AdServer::UserInfoSvcs::UserInfoManager::ImplementationException(
      "UserInfoCorbaClient::Distributor::clear_expired(): unsupported");
  }

  UserInfoCorbaClient::UserInfoCorbaClient(
    Logging::Logger* logger,
    const ControllerRefList& controller_refs,
    const CORBACommons::CorbaClientAdapter* corba_client_adapter,
    const Generics::Time& pool_timeout)
  {
    Distributor::ControllerRefList corba_controller_refs;
    for(const auto& controller_ref_group : controller_refs)
    {
      Distributor::ControllerRef corba_controller_ref_group;
      for(const auto& controller_ref : controller_ref_group)
      {
        corba_controller_ref_group.emplace_back(controller_ref.c_str());
      }
      corba_controller_refs.emplace_back(std::move(corba_controller_ref_group));
    }

    distributor_ = new Distributor(
      logger,
      corba_controller_refs,
      corba_client_adapter,
      pool_timeout);
    add_child_object(distributor_);
  }

  UserInfoCorbaClient::~UserInfoCorbaClient() noexcept = default;

  namespace GrpcAlgs
  {
    bool get_user_profile(
      UserInfoManagerGrpcAsyncClient& client,
      const CORBACommons::UserIdInfo& user_id,
      CORBA::Boolean temporary,
      const ProfilesRequestInfo& profile_request,
      UserProfiles_out user_profile)
    {
      pb::GetUserProfileRequest request;
      request.set_user_id(oct_seq_to_bytes_(user_id));
      request.set_temporary(temporary);
      convert_(profile_request, *request.mutable_profile_request());
      const auto response = AdServer::Grpc::sync_call<
        pb::GetUserProfileResponse>(
          [&](auto callback)
          {
            client.get_user_profile(request, std::move(callback));
          },
          throw_user_info_exception_);
      UserProfiles_var result = new UserProfiles;
      convert_(response.user_profile(), *result);
      user_profile = result._retn();
      return response.found();
    }

    bool history_match(
      UserInfoManagerGrpcAsyncClient& client,
      const pb::MatchRequest& request,
      pb::MatchResponse& response)
    {
      response = AdServer::Grpc::sync_call<pb::MatchResponse>(
        [&](auto callback)
        {
          client.match(request, std::move(callback));
        },
        throw_user_info_exception_);
      return response.matched();
    }

    UserInfoMatcher::MatchResult* history_match(
      UserInfoManagerGrpcAsyncClient& client,
      const pb::MatchRequest& request)
    {
      pb::MatchResponse response;
      history_match(client, request, response);
      return convert_history_match_result_(response.match_result());
    }

    UserInfoMatcher::MatchResult* make_history_match_result(
      const pb::MatchResponse& response)
    {
      return convert_history_match_result_(response.match_result());
    }

    void make_update_user_freq_caps_request(
      pb::UpdateUserFreqCapsRequest& request,
      const CORBACommons::UserIdInfo& user_id,
      const CORBACommons::TimestampInfo& time,
      const CORBACommons::RequestIdInfo& request_id,
      const FreqCapIdSeq& freq_caps,
      const FreqCapIdSeq& uc_freq_caps,
      const FreqCapIdSeq& virtual_freq_caps,
      const UserInfoManager::SeqOrderSeq& seq_orders,
      const CampaignIdSeq& campaign_ids,
      const CampaignIdSeq& uc_campaign_ids)
    {
      request.set_user_id(oct_seq_to_bytes_(user_id));
      request.set_time(oct_seq_to_bytes_(time));
      request.set_request_id(oct_seq_to_bytes_(request_id));
      id_seq_to_repeated_(freq_caps, request.mutable_freq_caps());
      id_seq_to_repeated_(uc_freq_caps, request.mutable_uc_freq_caps());
      id_seq_to_repeated_(
        virtual_freq_caps,
        request.mutable_virtual_freq_caps());
      for(CORBA::ULong i = 0; i < seq_orders.length(); ++i)
      {
        convert_(seq_orders[i], *request.add_seq_orders());
      }
      id_seq_to_repeated_(campaign_ids, request.mutable_campaign_ids());
      id_seq_to_repeated_(uc_campaign_ids, request.mutable_uc_campaign_ids());
    }

    bool merge(
      UserInfoManagerGrpcAsyncClient& client,
      const UserInfo& user_info,
      const UserInfoMatcher::MatchParams& match_params,
      const UserProfiles& merge_user_profile,
      CORBA::Boolean_out merge_success,
      CORBACommons::TimestampInfo_out last_request)
    {
      pb::MergeRequest request;
      convert_(user_info, *request.mutable_user_info());
      convert_(match_params, *request.mutable_match_params());
      convert_(merge_user_profile, *request.mutable_merge_user_profile());
      const auto response = AdServer::Grpc::sync_call<pb::MergeResponse>(
        [&](auto callback)
        {
          client.merge(request, std::move(callback));
        },
        throw_user_info_exception_);
      merge_success = response.merge_success();
      CORBACommons::TimestampInfo_var last_request_value =
        new CORBACommons::TimestampInfo;
      bytes_to_oct_seq_(response.last_request(), last_request_value.inout());
      last_request = last_request_value._retn();
      return response.result();
    }

    bool remove_user_profile(
      UserInfoManagerGrpcAsyncClient& client,
      const CORBACommons::UserIdInfo& user_id_info)
    {
      pb::RemoveUserProfileRequest request;
      request.set_user_id(oct_seq_to_bytes_(user_id_info));
      const auto response = AdServer::Grpc::sync_call<
        pb::RemoveUserProfileResponse>(
          [&](auto callback)
          {
            client.remove_user_profile(request, std::move(callback));
          },
          throw_user_info_exception_);
      return response.removed();
    }

    void update_user_freq_caps(
      UserInfoManagerGrpcAsyncClient& client,
      const CORBACommons::UserIdInfo& user_id,
      const CORBACommons::TimestampInfo& time,
      const CORBACommons::RequestIdInfo& request_id,
      const FreqCapIdSeq& freq_caps,
      const FreqCapIdSeq& uc_freq_caps,
      const FreqCapIdSeq& virtual_freq_caps,
      const UserInfoManager::SeqOrderSeq& seq_orders,
      const CampaignIdSeq& campaign_ids,
      const CampaignIdSeq& uc_campaign_ids)
    {
      pb::UpdateUserFreqCapsRequest request;
      make_update_user_freq_caps_request(
        request,
        user_id,
        time,
        request_id,
        freq_caps,
        uc_freq_caps,
        virtual_freq_caps,
        seq_orders,
        campaign_ids,
        uc_campaign_ids);
      AdServer::Grpc::sync_call<pb::UpdateUserFreqCapsResponse>(
        [&](auto callback)
        {
          client.update_user_freq_caps(request, std::move(callback));
        },
        throw_user_info_exception_);
    }

    void make_confirm_user_freq_caps_request(
      pb::ConfirmUserFreqCapsRequest& request,
      const CORBACommons::UserIdInfo& user_id,
      const CORBACommons::TimestampInfo& time,
      const CORBACommons::RequestIdInfo& request_id,
      const CORBACommons::IdSeq& exclude_pubpixel_accounts)
    {
      request.set_user_id(oct_seq_to_bytes_(user_id));
      request.set_time(oct_seq_to_bytes_(time));
      request.set_request_id(oct_seq_to_bytes_(request_id));
      id_seq_to_repeated_(
        exclude_pubpixel_accounts,
        request.mutable_exclude_pubpixel_accounts());
    }

    void confirm_user_freq_caps(
      UserInfoManagerGrpcAsyncClient& client,
      const CORBACommons::UserIdInfo& user_id,
      const CORBACommons::TimestampInfo& time,
      const CORBACommons::RequestIdInfo& request_id,
      const CORBACommons::IdSeq& exclude_pubpixel_accounts)
    {
      pb::ConfirmUserFreqCapsRequest request;
      make_confirm_user_freq_caps_request(
        request,
        user_id,
        time,
        request_id,
        exclude_pubpixel_accounts);
      AdServer::Grpc::sync_call<pb::ConfirmUserFreqCapsResponse>(
        [&](auto callback)
        {
          client.confirm_user_freq_caps(request, std::move(callback));
        },
        throw_user_info_exception_);
    }

    bool fraud_user(
      UserInfoManagerGrpcAsyncClient& client,
      const CORBACommons::UserIdInfo& user_id,
      const CORBACommons::TimestampInfo& time)
    {
      pb::FraudUserRequest request;
      request.set_user_id(oct_seq_to_bytes_(user_id));
      request.set_time(oct_seq_to_bytes_(time));
      const auto response =
        AdServer::Grpc::sync_call<pb::FraudUserResponse>(
          [&](auto callback)
          {
            client.fraud_user(request, std::move(callback));
          },
          throw_user_info_exception_);
      return response.fraud();
    }
  }

  void UserInfoCorbaClient::get_source(
    const pb::GetSourceRequest&,
    GetSourceCallback callback)
  {
    pb::GetSourceResponse response;
    callback(unsupported_status_("get_source"), response);
  }

  void UserInfoCorbaClient::get_master_stamp(
    const pb::GetMasterStampRequest&,
    GetMasterStampCallback callback)
  {
    pb::GetMasterStampResponse response;
    callback(unsupported_status_("get_master_stamp"), response);
  }

  void UserInfoCorbaClient::get_user_profile(
    const pb::GetUserProfileRequest& request,
    GetUserProfileCallback callback)
  {
    pb::GetUserProfileResponse response;
    try
    {
      CORBACommons::UserIdInfo user_id;
      ProfilesRequestInfo profile_request;
      bytes_to_oct_seq_(request.user_id(), user_id);
      convert_(request.profile_request(), profile_request);
      UserProfiles_var user_profile;
      response.set_found(distributor_->get_user_profile(
        user_id,
        request.temporary(),
        profile_request,
        user_profile.out()));
      convert_(user_profile.in(), *response.mutable_user_profile());
      callback(grpc::Status::OK, response);
    }
    catch(const UserInfoManager::NotReady& ex)
    {
      callback(exception_status_(grpc::StatusCode::UNAVAILABLE, ex.description.in()), response);
    }
    catch(const UserInfoManager::ChunkNotFound& ex)
    {
      callback(exception_status_(grpc::StatusCode::NOT_FOUND, ex.description.in()), response);
    }
    catch(const UserInfoManager::ImplementationException& ex)
    {
      callback(exception_status_(grpc::StatusCode::INTERNAL, ex.description.in()), response);
    }
    catch(const CORBA::SystemException& ex)
    {
      callback(exception_status_(ex), response);
    }
    catch(const eh::Exception& ex)
    {
      callback(exception_status_(ex), response);
    }
  }

  void UserInfoCorbaClient::match(
    const pb::MatchRequest& request,
    MatchCallback callback)
  {
    pb::MatchResponse response;
    try
    {
      UserInfo user_info;
      UserInfoMatcher::MatchParams match_params;
      convert_(request.user_info(), user_info);
      convert_(request.match_params(), match_params);
      UserInfoMatcher::MatchResult_var match_result;
      response.set_matched(distributor_->match(
        user_info,
        match_params,
        match_result.out()));
      convert_(match_result.in(), *response.mutable_match_result());
      callback(grpc::Status::OK, response);
    }
    catch(const UserInfoManager::NotReady& ex)
    {
      callback(exception_status_(grpc::StatusCode::UNAVAILABLE, ex.description.in()), response);
    }
    catch(const UserInfoManager::ChunkNotFound& ex)
    {
      callback(exception_status_(grpc::StatusCode::NOT_FOUND, ex.description.in()), response);
    }
    catch(const UserInfoManager::ImplementationException& ex)
    {
      callback(exception_status_(grpc::StatusCode::INTERNAL, ex.description.in()), response);
    }
    catch(const CORBA::SystemException& ex)
    {
      callback(exception_status_(ex), response);
    }
    catch(const eh::Exception& ex)
    {
      callback(exception_status_(ex), response);
    }
  }

  void UserInfoCorbaClient::update_user_freq_caps(
    const pb::UpdateUserFreqCapsRequest& request,
    UpdateUserFreqCapsCallback callback)
  {
    pb::UpdateUserFreqCapsResponse response;
    try
    {
      CORBACommons::UserIdInfo user_id;
      CORBACommons::TimestampInfo time;
      CORBACommons::RequestIdInfo request_id;
      FreqCapIdSeq freq_caps;
      FreqCapIdSeq uc_freq_caps;
      FreqCapIdSeq virtual_freq_caps;
      UserInfoManager::SeqOrderSeq seq_orders;
      CampaignIdSeq campaign_ids;
      CampaignIdSeq uc_campaign_ids;
      bytes_to_oct_seq_(request.user_id(), user_id);
      bytes_to_oct_seq_(request.time(), time);
      bytes_to_oct_seq_(request.request_id(), request_id);
      repeated_to_id_seq_(request.freq_caps(), freq_caps);
      repeated_to_id_seq_(request.uc_freq_caps(), uc_freq_caps);
      repeated_to_id_seq_(request.virtual_freq_caps(), virtual_freq_caps);
      convert_seq_order_seq_(request.seq_orders(), seq_orders);
      repeated_to_id_seq_(request.campaign_ids(), campaign_ids);
      repeated_to_id_seq_(request.uc_campaign_ids(), uc_campaign_ids);
      distributor_->update_user_freq_caps(
        user_id,
        time,
        request_id,
        freq_caps,
        uc_freq_caps,
        virtual_freq_caps,
        seq_orders,
        campaign_ids,
        uc_campaign_ids);
      callback(grpc::Status::OK, response);
    }
    catch(const UserInfoManager::NotReady& ex)
    {
      callback(exception_status_(grpc::StatusCode::UNAVAILABLE, ex.description.in()), response);
    }
    catch(const UserInfoManager::ChunkNotFound& ex)
    {
      callback(exception_status_(grpc::StatusCode::NOT_FOUND, ex.description.in()), response);
    }
    catch(const UserInfoManager::ImplementationException& ex)
    {
      callback(exception_status_(grpc::StatusCode::INTERNAL, ex.description.in()), response);
    }
    catch(const CORBA::SystemException& ex)
    {
      callback(exception_status_(ex), response);
    }
    catch(const eh::Exception& ex)
    {
      callback(exception_status_(ex), response);
    }
  }

  void UserInfoCorbaClient::confirm_user_freq_caps(
    const pb::ConfirmUserFreqCapsRequest& request,
    ConfirmUserFreqCapsCallback callback)
  {
    pb::ConfirmUserFreqCapsResponse response;
    try
    {
      CORBACommons::UserIdInfo user_id;
      CORBACommons::TimestampInfo time;
      CORBACommons::RequestIdInfo request_id;
      CORBACommons::IdSeq exclude_pubpixel_accounts;
      bytes_to_oct_seq_(request.user_id(), user_id);
      bytes_to_oct_seq_(request.time(), time);
      bytes_to_oct_seq_(request.request_id(), request_id);
      repeated_to_id_seq_(
        request.exclude_pubpixel_accounts(),
        exclude_pubpixel_accounts);
      distributor_->confirm_user_freq_caps(
        user_id,
        time,
        request_id,
        exclude_pubpixel_accounts);
      callback(grpc::Status::OK, response);
    }
    catch(const UserInfoManager::NotReady& ex)
    {
      callback(exception_status_(grpc::StatusCode::UNAVAILABLE, ex.description.in()), response);
    }
    catch(const UserInfoManager::ChunkNotFound& ex)
    {
      callback(exception_status_(grpc::StatusCode::NOT_FOUND, ex.description.in()), response);
    }
    catch(const UserInfoManager::ImplementationException& ex)
    {
      callback(exception_status_(grpc::StatusCode::INTERNAL, ex.description.in()), response);
    }
    catch(const CORBA::SystemException& ex)
    {
      callback(exception_status_(ex), response);
    }
    catch(const eh::Exception& ex)
    {
      callback(exception_status_(ex), response);
    }
  }

  void UserInfoCorbaClient::fraud_user(
    const pb::FraudUserRequest& request,
    FraudUserCallback callback)
  {
    pb::FraudUserResponse response;
    try
    {
      CORBACommons::UserIdInfo user_id;
      CORBACommons::TimestampInfo time;
      bytes_to_oct_seq_(request.user_id(), user_id);
      bytes_to_oct_seq_(request.time(), time);
      response.set_fraud(distributor_->fraud_user(user_id, time));
      callback(grpc::Status::OK, response);
    }
    catch(const UserInfoManager::NotReady& ex)
    {
      callback(exception_status_(grpc::StatusCode::UNAVAILABLE, ex.description.in()), response);
    }
    catch(const UserInfoManager::ChunkNotFound& ex)
    {
      callback(exception_status_(grpc::StatusCode::NOT_FOUND, ex.description.in()), response);
    }
    catch(const UserInfoManager::ImplementationException& ex)
    {
      callback(exception_status_(grpc::StatusCode::INTERNAL, ex.description.in()), response);
    }
    catch(const CORBA::SystemException& ex)
    {
      callback(exception_status_(ex), response);
    }
    catch(const eh::Exception& ex)
    {
      callback(exception_status_(ex), response);
    }
  }

  void UserInfoCorbaClient::remove_user_profile(
    const pb::RemoveUserProfileRequest& request,
    RemoveUserProfileCallback callback)
  {
    pb::RemoveUserProfileResponse response;
    try
    {
      CORBACommons::UserIdInfo user_id;
      bytes_to_oct_seq_(request.user_id(), user_id);
      response.set_removed(distributor_->remove_user_profile(user_id));
      callback(grpc::Status::OK, response);
    }
    catch(const UserInfoManager::NotReady& ex)
    {
      callback(exception_status_(grpc::StatusCode::UNAVAILABLE, ex.description.in()), response);
    }
    catch(const UserInfoManager::ChunkNotFound& ex)
    {
      callback(exception_status_(grpc::StatusCode::NOT_FOUND, ex.description.in()), response);
    }
    catch(const UserInfoManager::ImplementationException& ex)
    {
      callback(exception_status_(grpc::StatusCode::INTERNAL, ex.description.in()), response);
    }
    catch(const CORBA::SystemException& ex)
    {
      callback(exception_status_(ex), response);
    }
    catch(const eh::Exception& ex)
    {
      callback(exception_status_(ex), response);
    }
  }

  void UserInfoCorbaClient::merge(
    const pb::MergeRequest& request,
    MergeCallback callback)
  {
    pb::MergeResponse response;
    try
    {
      UserInfo user_info;
      UserInfoMatcher::MatchParams match_params;
      UserProfiles merge_user_profile;
      CORBA::Boolean merge_success = false;
      CORBACommons::TimestampInfo_var last_request;
      convert_(request.user_info(), user_info);
      convert_(request.match_params(), match_params);
      convert_(request.merge_user_profile(), merge_user_profile);
      response.set_result(distributor_->merge(
        user_info,
        match_params,
        merge_user_profile,
        merge_success,
        last_request.out()));
      response.set_merge_success(merge_success);
      response.set_last_request(oct_seq_to_bytes_(last_request.in()));
      callback(grpc::Status::OK, response);
    }
    catch(const UserInfoManager::NotReady& ex)
    {
      callback(exception_status_(grpc::StatusCode::UNAVAILABLE, ex.description.in()), response);
    }
    catch(const UserInfoManager::ChunkNotFound& ex)
    {
      callback(exception_status_(grpc::StatusCode::NOT_FOUND, ex.description.in()), response);
    }
    catch(const UserInfoManager::ImplementationException& ex)
    {
      callback(exception_status_(grpc::StatusCode::INTERNAL, ex.description.in()), response);
    }
    catch(const CORBA::SystemException& ex)
    {
      callback(exception_status_(ex), response);
    }
    catch(const eh::Exception& ex)
    {
      callback(exception_status_(ex), response);
    }
  }

  void UserInfoCorbaClient::consider_publishers_optin(
    const pb::ConsiderPublishersOptinRequest& request,
    ConsiderPublishersOptinCallback callback)
  {
    pb::ConsiderPublishersOptinResponse response;
    try
    {
      CORBACommons::UserIdInfo user_id;
      CORBACommons::TimestampInfo now;
      CORBACommons::IdSeq exclude_pubpixel_accounts;
      bytes_to_oct_seq_(request.user_id(), user_id);
      bytes_to_oct_seq_(request.now(), now);
      repeated_to_id_seq_(
        request.exclude_pubpixel_accounts(),
        exclude_pubpixel_accounts);
      distributor_->consider_publishers_optin(
        user_id,
        exclude_pubpixel_accounts,
        now);
      callback(grpc::Status::OK, response);
    }
    catch(const UserInfoManager::NotReady& ex)
    {
      callback(exception_status_(grpc::StatusCode::UNAVAILABLE, ex.description.in()), response);
    }
    catch(const UserInfoManager::ChunkNotFound& ex)
    {
      callback(exception_status_(grpc::StatusCode::NOT_FOUND, ex.description.in()), response);
    }
    catch(const UserInfoManager::ImplementationException& ex)
    {
      callback(exception_status_(grpc::StatusCode::INTERNAL, ex.description.in()), response);
    }
    catch(const CORBA::SystemException& ex)
    {
      callback(exception_status_(ex), response);
    }
    catch(const eh::Exception& ex)
    {
      callback(exception_status_(ex), response);
    }
  }

  void UserInfoCorbaClient::uim_ready(
    const pb::UimReadyRequest&,
    UimReadyCallback callback)
  {
    pb::UimReadyResponse response;
    response.set_ready(true);
    callback(grpc::Status::OK, response);
  }

  void UserInfoCorbaClient::get_progress(
    const pb::GetProgressRequest&,
    GetProgressCallback callback)
  {
    pb::GetProgressResponse response;
    callback(unsupported_status_("get_progress"), response);
  }

  void UserInfoCorbaClient::clear_expired(
    const pb::ClearExpiredRequest& request,
    ClearExpiredCallback callback)
  {
    pb::ClearExpiredResponse response;
    try
    {
      CORBACommons::TimestampInfo cleanup_time;
      bytes_to_oct_seq_(request.cleanup_time(), cleanup_time);
      distributor_->clear_expired(
        request.sync(),
        cleanup_time,
        request.portion());
      callback(grpc::Status::OK, response);
    }
    catch(const UserInfoManager::ImplementationException& ex)
    {
      callback(exception_status_(grpc::StatusCode::INTERNAL, ex.description.in()), response);
    }
    catch(const CORBA::SystemException& ex)
    {
      callback(exception_status_(ex), response);
    }
    catch(const eh::Exception& ex)
    {
      callback(exception_status_(ex), response);
    }
  }
}
