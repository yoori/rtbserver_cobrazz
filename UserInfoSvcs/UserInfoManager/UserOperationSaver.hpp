#ifndef USERINFOMANAGER_USEROPERATIONSAVER_HPP
#define USERINFOMANAGER_USEROPERATIONSAVER_HPP

#include <string>
#include <Commons/LockMap.hpp>
#include <ProfilingCommons/PlainStorage3/FileController.hpp>
#include <ProfilingCommons/PlainStorage3/FileWriter.hpp>

#include "UserOperationProcessor.hpp"

namespace AdServer
{
namespace UserInfoSvcs
{
  class UserOperationSaver:
    public UserOperationProcessor,
    public Generics::CompositeActiveObject,
    public virtual ReferenceCounting::AtomicImpl
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
      ADD_AUDIENCE,
      REMOVE_AUDIENCE,
    };

    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  public:
    UserOperationSaver(
      Generics::ActiveObjectCallback* callback,
      Logging::Logger* logger,
      const char* output_files_path,
      const char* output_file_prefix,
      unsigned long chunks_number,
      ProfilingCommons::FileController* file_controller,
      UserOperationProcessor* next_processor)
      /*throw(Exception)*/;

    // UserOperationProcessor interface
    virtual bool
    remove_user_profile(
      const UserId& user_id)
      /*throw(ChunkNotFound, UserOperationProcessor::Exception)*/;

    virtual void
    fraud_user(
      const UserId& user_id,
      const Generics::Time& now)
      /*throw(NotReady, ChunkNotFound, UserOperationProcessor::Exception)*/;

    virtual void
    match(
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
      /*throw(NotReady, ChunkNotFound, UserOperationProcessor::Exception)*/;

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
      /*throw(NotReady, ChunkNotFound, UserOperationProcessor::Exception)*/;

    virtual void
    exchange_merge(
      const UserId& user_id,
      const Generics::MemBuf& base_profile_buf,
      const Generics::MemBuf& history_profile_buf,
      UserInfoManagerLogger::HistoryOptimizationInfo* ho_info)
      /*throw(NotReady, ChunkNotFound, UserOperationProcessor::Exception)*/;

    // user freq caps
    virtual void
    update_freq_caps(
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
      /*throw(NotReady, ChunkNotFound, UserOperationProcessor::Exception)*/;

    virtual void
    confirm_freq_caps(
      const UserId& user_id,
      const Generics::Time& now,
      const Commons::RequestId& request_id,
      const std::set<unsigned long>& exclude_pubpixel_accounts)
      /*throw(ChunkNotFound, UserOperationProcessor::Exception)*/;

    virtual void
    remove_audience_channels(
      const UserId& user_id,
      const AudienceChannelSet& audience_channels)
      /*throw(ChunkNotFound, UserOperationProcessor::Exception)*/;

    virtual void
    add_audience_channels(
      const UserId& user_id,
      const AudienceChannelSet& audience_channels)
      /*throw(NotReady, ChunkNotFound, UserOperationProcessor::Exception)*/;

    virtual void
    consider_publishers_optin(
      const UserId& user_id,
      const std::set<unsigned long>& publisher_account_ids,
      const Generics::Time& now,
      AdServer::ProfilingCommons::OperationPriority op_priority)
      /*throw(ChunkNotFound, UserOperationProcessor::Exception)*/;

    // ActiveObject interface
    virtual void
    wait_object()
      /*throw(Generics::ActiveObject::Exception, eh::Exception)*/;

    void
    rotate() noexcept;

  protected:
    typedef Sync::Policy::PosixThreadRW SyncPolicy;

    typedef std::list<Generics::ConstSmartMemBuf_var>
      ConstSmartMemBufList;

    struct SaveQueue: public ReferenceCounting::AtomicImpl
    {
      unsigned long chunk_id;
      ConstSmartMemBufList bufs;

    protected:
      virtual ~SaveQueue() noexcept {}
    };

    typedef ReferenceCounting::SmartPtr<SaveQueue>
      SaveQueue_var;

    typedef std::vector<SaveQueue_var> SaveQueueArray;

    typedef std::list<SaveQueue_var> SaveQueueList;

    struct QueueHolder: public ReferenceCounting::AtomicImpl
    {
      mutable Sync::Condition cond;
      //mutable Sync::PosixMutex lock;
      SaveQueueArray queues;
      SaveQueueList non_empty_queues;

    protected:
      virtual ~QueueHolder() noexcept {}
    };

    typedef ReferenceCounting::SmartPtr<QueueHolder>
      QueueHolder_var;

    struct File:
      public ReferenceCounting::AtomicImpl,
      public ProfilingCommons::FileWriter
    {
    public:
      File(
        const char* file_path,
        const char* file_name,
        ProfilingCommons::FileController* file_controller)
        /*throw(UserOperationSaver::Exception)*/;

    protected:
      virtual ~File() noexcept; // rename result file

    protected:
      const std::string tmp_file_name_;
      const std::string file_name_;
    };

    typedef ReferenceCounting::SmartPtr<File> File_var;
    typedef std::map<unsigned long, File_var> FileMap;
    typedef AdServer::Commons::StrictLockMap<unsigned long> FileLockMap;

    struct FilesHolder: public ReferenceCounting::AtomicImpl
    {
      FileLockMap file_lock;
      SyncPolicy::Mutex files_lock;
      FileMap files;

    protected:
      virtual ~FilesHolder() noexcept {}
    };

    typedef ReferenceCounting::SmartPtr<FilesHolder>
      FilesHolder_var;

    class Dumper;

  protected:
    virtual ~UserOperationSaver() noexcept {}

    template<typename WriterType>
    void
    save_(
      const AdServer::Commons::UserId& user_id,
      const WriterType& writer)
      noexcept;

  private:
    Logging::Logger_var logger_;
    const unsigned long chunks_count_;
    QueueHolder_var queue_holder_;
    FilesHolder_var files_holder_;

    UserOperationProcessor_var next_processor_;
  };

  typedef ReferenceCounting::SmartPtr<UserOperationSaver>
    UserOperationSaver_var;
}
}

#endif /*USERINFOMANAGER_USEROPERATIONSAVER_HPP*/
