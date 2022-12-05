#include <LogCommons/LogCommons.hpp>
#include <UserInfoSvcs/UserInfoCommons/UserOperationProfiles.hpp>

#include "Compatibility/UserOperationProfilesAdapter.hpp"
#include "UserOperationSaver.hpp"

namespace Aspect
{
  const char USER_OPERATION_SAVER[] = "UserOperationSaver";
}

namespace AdServer
{
namespace UserInfoSvcs
{
  // UserOperationSaver::Dumper
  class UserOperationSaver::Dumper: public Generics::ActiveObjectCommonImpl
  {
  public:
    Dumper(
      Generics::ActiveObjectCallback* callback,
      Logging::Logger* logger,
      const char* output_dir,
      const char* file_prefix,
      unsigned long chunks_count,
      QueueHolder* queue_holder,
      FilesHolder* files_holder,
      ProfilingCommons::FileController* file_controller)
      noexcept;

  protected:
    class Job: public Generics::ActiveObjectCommonImpl::SingleJob
    {
    public:
      Job(
        Generics::ActiveObjectCallback* callback,
        Logging::Logger* logger,
        const char* output_dir,
        const char* file_prefix,
        unsigned long chunks_count,
        QueueHolder* queue_holder,
        FilesHolder* files_holder,
        ProfilingCommons::FileController* file_controller)
        noexcept;

      virtual void
      work() noexcept;

      virtual void
      terminate() noexcept;

    protected:
      virtual ~Job() noexcept
      {}

      void
      dump_queues_(UserOperationSaver::SaveQueueList& dump_queues)
        noexcept;

      void
      dump_queue_(unsigned long chunk_i, ConstSmartMemBufList& bufs)
        noexcept;

    protected:
      Logging::Logger_var logger_;
      const std::string output_dir_;
      const std::string output_file_prefix_;
      const unsigned long chunks_count_;
      QueueHolder_var queue_holder_;
      FilesHolder_var files_holder_;
      ProfilingCommons::FileController_var file_controller_;
    };
  };

  // UserOperationSaver::File impl
  UserOperationSaver::File::File(
    const char* res_file,
    const char* tmp_file,
    ProfilingCommons::FileController* file_controller)
    /*throw(UserOperationSaver::Exception)*/
  try
    : ProfilingCommons::FileWriter(
        tmp_file,
        1024*1024,
        false, // not append
        true, // disable caching
        file_controller),
      tmp_file_name_(tmp_file),
      file_name_(res_file)
  {}
  catch(const eh::Exception& ex)
  {
    Stream::Error ostr;
    ostr << "UserOperationSaver::File::File(): " << ex.what();
    throw UserOperationSaver::Exception(ostr);
  }

  UserOperationSaver::File::~File() noexcept
  {
    FileWriter::close();
    ::rename(tmp_file_name_.c_str(), file_name_.c_str());
  }

  // UserOperationSaver::Dumper
  UserOperationSaver::Dumper::Dumper(
    Generics::ActiveObjectCallback* callback,
    Logging::Logger* logger,
    const char* output_dir,
    const char* output_file_prefix,
    unsigned long chunks_number,
    QueueHolder* queue_holder,
    FilesHolder* files_holder,
    ProfilingCommons::FileController* file_controller)
    noexcept
    : Generics::ActiveObjectCommonImpl(
        SingleJob_var(new Job(
          callback,
          logger,
          output_dir,
          output_file_prefix,
          chunks_number,
          queue_holder,
          files_holder,
          file_controller)),
        1)
  {}

  // UserOperationSaver::Dumper::Job
  UserOperationSaver::Dumper::Job::Job(
    Generics::ActiveObjectCallback* callback,
    Logging::Logger* logger,
    const char* output_dir,
    const char* file_prefix,
    unsigned long chunks_count,
    QueueHolder* queue_holder,
    FilesHolder* files_holder,
    ProfilingCommons::FileController* file_controller)
    noexcept
    : SingleJob(callback),
      logger_(ReferenceCounting::add_ref(logger)),
      output_dir_(output_dir),
      output_file_prefix_(file_prefix),
      chunks_count_(chunks_count),
      queue_holder_(ReferenceCounting::add_ref(queue_holder)),
      files_holder_(ReferenceCounting::add_ref(files_holder)),
      file_controller_(ReferenceCounting::add_ref(file_controller))
  {}

  void
  UserOperationSaver::Dumper::Job::work() noexcept
  {
    SaveQueueArray empty_queues;
    SaveQueueList dump_queues;

    {
      empty_queues.resize(chunks_count_);

      unsigned long chunk_id = 0;
      for(SaveQueueArray::iterator it = empty_queues.begin();
        it != empty_queues.end(); ++it, ++chunk_id)
      {
        *it = new SaveQueue();
        (*it)->chunk_id = chunk_id;
      }
    }

    while (!is_terminating())
    {
      {
        Sync::ConditionalGuard guard(queue_holder_->cond);

        if(!queue_holder_->non_empty_queues.empty())
        {
          for(SaveQueueList::const_iterator it =
                queue_holder_->non_empty_queues.begin();
              it != queue_holder_->non_empty_queues.end(); ++it)
          {
            queue_holder_->queues[(*it)->chunk_id].swap(
              empty_queues[(*it)->chunk_id]);
          }

          dump_queues.splice(dump_queues.begin(), queue_holder_->non_empty_queues);
        }

        if (dump_queues.empty() && !is_terminating())
        {
          guard.wait();
        }
      }

      // do dump
      dump_queues_(dump_queues);
      dump_queues.clear();
    }

    // do last dump before stop
    dump_queues_(dump_queues);
  }

  void
  UserOperationSaver::Dumper::Job::terminate()
    noexcept
  {
    Sync::ConditionalGuard guard(queue_holder_->cond);
    queue_holder_->cond.broadcast();
  }

  void
  UserOperationSaver::Dumper::Job::dump_queues_(
    UserOperationSaver::SaveQueueList& dump_queues)
    noexcept
  {
    for(UserOperationSaver::SaveQueueList::iterator it =
          dump_queues.begin();
        it != dump_queues.end(); ++it)
    {
      assert(it->in());

      if(!(*it)->bufs.empty())
      {
        dump_queue_((*it)->chunk_id, (*it)->bufs);
        (*it)->bufs.clear();
      }
    }
  }

  void
  UserOperationSaver::Dumper::Job::dump_queue_(
    unsigned long chunk_i,
    ConstSmartMemBufList& bufs)
    noexcept
  {
    static const char* FUN = "UserOperationSaver::Dumper::Job::dump_queue_()";

    try
    {
      File_var target_file;
      bool file_created = false;
      FileLockMap::WriteGuard file_lock = files_holder_->file_lock.write_lock(chunk_i);

      {
        SyncPolicy::WriteGuard lock(files_holder_->files_lock);
        FileMap::iterator it = files_holder_->files.find(chunk_i);
        if(it != files_holder_->files.end())
        {
          target_file = it->second;
        }
      }

      if(!target_file.in())
      {
        LogProcessing::LogFileNameInfo file_name_info(output_file_prefix_);
        file_name_info.distrib_count = chunks_count_;
        file_name_info.distrib_index = chunk_i;
        LogProcessing::StringPair files =
          LogProcessing::make_log_file_name_pair(file_name_info, output_dir_);
        target_file = new File(
          files.first.c_str(),
          files.second.c_str(),
          file_controller_);
        file_created = true;
      }

      if(file_created)
      {
        SyncPolicy::WriteGuard lock(files_holder_->files_lock);
        files_holder_->files.insert(std::make_pair(chunk_i, target_file));
      }

      while(!bufs.empty())
      {
        const Generics::ConstSmartMemBuf_var& save_buf = bufs.front();
        uint32_t buf_size = save_buf->membuf().size();
        target_file->write(
          reinterpret_cast<const char*>(&buf_size),
          sizeof(buf_size));
        target_file->write(
          save_buf->membuf().get<char>(),
          buf_size);
        bufs.pop_front();
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught eh::Exception: " << ex.what();
      logger_->log(ostr.str(),
        Logging::Logger::ERROR,
        Aspect::USER_OPERATION_SAVER);
    }
  }


  // UserOperationSaver
  UserOperationSaver::UserOperationSaver(
    Generics::ActiveObjectCallback* callback,
    Logging::Logger* logger,
    const char* output_dir,
    const char* output_file_prefix,
    unsigned long chunks_number,
    ProfilingCommons::FileController* file_controller,
    UserOperationProcessor* next_processor)
    /*throw(Exception)*/
    : logger_(ReferenceCounting::add_ref(logger)),
      chunks_count_(chunks_number),
      queue_holder_(new QueueHolder()),
      files_holder_(new FilesHolder()),
      next_processor_(ReferenceCounting::add_ref(next_processor))
  {
    queue_holder_->queues.resize(chunks_number);

    unsigned long chunk_i = 0;
    for(SaveQueueArray::iterator it = queue_holder_->queues.begin();
        it != queue_holder_->queues.end(); ++it, ++chunk_i)
    {
      SaveQueue_var new_queue = new SaveQueue();
      new_queue->chunk_id = chunk_i;
      *it = new_queue;
    }
    
    Generics::ActiveObject_var dump_thread = new Dumper(
      callback,
      logger,
      output_dir,
      output_file_prefix,
      chunks_number,
      queue_holder_,
      files_holder_,
      file_controller);

    add_child_object(dump_thread);
  }

  bool
  UserOperationSaver::remove_user_profile(
    const UserId& user_id)
    /*throw(ChunkNotFound, UserOperationProcessor::Exception)*/
  {
    return next_processor_->remove_user_profile(user_id);
  }

  void
  UserOperationSaver::fraud_user(
    const UserId& user_id,
    const Generics::Time& now)
    /*throw(NotReady, ChunkNotFound, UserOperationProcessor::Exception)*/
  {
    {
      UserFraudOperationWriter fraud_operation_writer;
      fraud_operation_writer.operation_type() = UO_FRAUD;
      fraud_operation_writer.version() = FRAUD_OPERATION_PROFILE_VERSION;
      fraud_operation_writer.user_id() = user_id.to_string();
      fraud_operation_writer.fraud_time() = now.tv_sec;
      save_(user_id, fraud_operation_writer);
    }

    next_processor_->fraud_user(user_id, now);
  }

  void
  UserOperationSaver::match(
    const RequestMatchParams& channel_match_info,
    long last_colo_id,
    long current_placement_colo_id,
    ColoUserId& colo_user_id,
    const ChannelMatchPack& matched_channels,
    ChannelMatchMap& result_channels,
    UserAppearance& user_app,
    //PartlyMatchResult& partly_match_result,
    ProfileProperties& properties,
    AdServer::ProfilingCommons::OperationPriority op_priority,
    UserInfoManagerLogger::HistoryOptimizationInfo* ho_info,
    UniqueChannelsResult* pucr)
    /*throw(NotReady, ChunkNotFound, UserOperationProcessor::Exception)*/
  {
    if(!channel_match_info.silent_match)
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

      for (ChannelMatchVector::const_iterator it =  matched_channels.page_channels.begin();
           it != matched_channels.page_channels.end(); ++it)
      {
        cm_writer.channel_id() = it->channel_id;
        cm_writer.channel_trigger_id() = it->channel_trigger_id;
        match_operation_writer.page_channels().push_back(cm_writer);
      }
      
      for (ChannelMatchVector::const_iterator it =  matched_channels.search_channels.begin();
           it != matched_channels.search_channels.end(); ++it)
      {
        cm_writer.channel_id() = it->channel_id;
        cm_writer.channel_trigger_id() = it->channel_trigger_id;
        match_operation_writer.search_channels().push_back(cm_writer);
      }
      
      for (ChannelMatchVector::const_iterator it =  matched_channels.url_channels.begin();
           it != matched_channels.url_channels.end(); ++it)
      {
        cm_writer.channel_id() = it->channel_id;
        cm_writer.channel_trigger_id() = it->channel_trigger_id;
        match_operation_writer.url_channels().push_back(cm_writer);
      }

      for (ChannelMatchVector::const_iterator it =  matched_channels.url_keyword_channels.begin();
           it != matched_channels.url_keyword_channels.end(); ++it)
      {
        cm_writer.channel_id() = it->channel_id;
        cm_writer.channel_trigger_id() = it->channel_trigger_id;
        match_operation_writer.url_keyword_channels().push_back(cm_writer);
      }

      std::copy(
        matched_channels.persistent_channels.begin(),
        matched_channels.persistent_channels.end(),
        std::back_inserter(match_operation_writer.persistent_channels()));

      if (channel_match_info.coord_data.defined)
      {
        CoordDataWriter cdw;

        cdw.latitude().alloc(
          AdServer::CampaignSvcs::CoordDecimal::PACK_SIZE);
        channel_match_info.coord_data.latitude.pack(cdw.latitude().data());
       
        cdw.longitude().alloc(
          AdServer::CampaignSvcs::CoordDecimal::PACK_SIZE);
        channel_match_info.coord_data.longitude.pack(cdw.longitude().data());

        cdw.accuracy().alloc(
          AdServer::CampaignSvcs::AccuracyDecimal::PACK_SIZE);
        channel_match_info.coord_data.accuracy.pack(cdw.accuracy().data());

        match_operation_writer.coord_data().push_back(cdw);
      }

      save_(channel_match_info.user_id, match_operation_writer);
    }

    next_processor_->match(
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

  void
  UserOperationSaver::merge(
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

      save_(request_params.user_id, merge_operation_writer);
    }

    next_processor_->merge(
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

  void
  UserOperationSaver::exchange_merge(
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

      save_(user_id, merge_operation_writer);
    }

    next_processor_->exchange_merge(
      user_id,
      merge_base_profile,
      merge_history_profile,
      ho_info);
  }

  void
  UserOperationSaver::update_freq_caps(
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
    /*throw(NotReady, ChunkNotFound, UserOperationProcessor::Exception)*/
  {
    {
      UserFreqCapUpdateOperationWriter profile_writer;
      profile_writer.operation_type() = UO_FC_UPDATE;
      profile_writer.version() = FC_UPDATE_OPERATION_PROFILE_VERSION;
      profile_writer.user_id() = user_id.to_string();
      profile_writer.time() = now.tv_sec;
      profile_writer.request_id() = request_id.to_string();
      std::copy(freq_caps.begin(),
        freq_caps.end(),
        std::back_inserter(profile_writer.freq_caps()));
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

      for (UserFreqCapProfile::SeqOrderList::const_iterator it = seq_orders.begin();
           it != seq_orders.end(); ++it)
      {
        SeqOrderDescriptorWriter seq_order;
        seq_order.ccg_id() = it->ccg_id;
        seq_order.set_id() = it->set_id;
        seq_order.imps() = it->imps;

        profile_writer.seq_orders().push_back(seq_order);
      }

      save_(user_id, profile_writer);
    }

    next_processor_->update_freq_caps(
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

  void
  UserOperationSaver::confirm_freq_caps(
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

      save_(user_id, profile_writer);
    }

    next_processor_->confirm_freq_caps(
      user_id,
      now,
      request_id,
      exclude_pubpixel_accounts);
  }

  void pack_freq_cap_info(
    FreqCapInfoWriter& res,
    const AdServer::Commons::FreqCap& fc)
  {
    res.fc_id() = fc.fc_id;
    res.lifelimit() = fc.lifelimit;
    res.period() = fc.period.tv_sec;
    res.window_limit() = fc.window_limit;
    res.window_time() = fc.window_time.tv_sec;
  }

  void
  UserOperationSaver::remove_audience_channels(
    const UserId& user_id,
    const AudienceChannelSet& audience_channels)
    /*throw(ChunkNotFound, UserOperationProcessor::Exception)*/
  {
    AudienceChannelsOperationWriter profile_writer;
    profile_writer.operation_type() = REMOVE_AUDIENCE;
    profile_writer.version() = REMOVE_AUDIENCE_CHANNELS_OPERATION_PROFILE_VERSION;
    profile_writer.user_id() = user_id.to_string();

    for (auto it = audience_channels.begin(); it != audience_channels.end(); ++it)
    {
      AudienceChannelDescriptorWriter channel_writer;
      channel_writer.channel_id() = it->channel_id;
      channel_writer.time() = it->time.tv_sec;
      profile_writer.audience_channels().push_back(channel_writer);
    }

    save_(user_id, profile_writer);

    next_processor_->remove_audience_channels(user_id, audience_channels);
  }

  void
  UserOperationSaver::add_audience_channels(
    const UserId& user_id,
    const AudienceChannelSet& audience_channels)
    /*throw(NotReady, ChunkNotFound, UserOperationProcessor::Exception)*/
  {
    AudienceChannelsOperationWriter profile_writer;
    profile_writer.operation_type() = ADD_AUDIENCE;
    profile_writer.version() = ADD_AUDIENCE_CHANNELS_OPERATION_PROFILE_VERSION;
    profile_writer.user_id() = user_id.to_string();

    for (auto it = audience_channels.begin(); it != audience_channels.end(); ++it)
    {
      AudienceChannelDescriptorWriter channel_writer;
      channel_writer.channel_id() = it->channel_id;
      channel_writer.time() = it->time.tv_sec;
      profile_writer.audience_channels().push_back(channel_writer);
    }

    save_(user_id, profile_writer);

    next_processor_->add_audience_channels(user_id, audience_channels);
  }

  void
  UserOperationSaver::consider_publishers_optin(
    const UserId& user_id,
    const std::set<unsigned long>& publisher_account_ids,
    const Generics::Time& now,
    AdServer::ProfilingCommons::OperationPriority op_priority)
    /*throw(ChunkNotFound, UserOperationProcessor::Exception)*/
  {
    next_processor_->consider_publishers_optin(
      user_id,
      publisher_account_ids,
      now,
      op_priority);
  }

  template<typename WriterType>
  void
  UserOperationSaver::save_(
    const AdServer::Commons::UserId& user_id,
    const WriterType& writer)
    noexcept
  {
    Generics::SmartMemBuf_var new_membuf(new Generics::SmartMemBuf(writer.size()));
    writer.save(new_membuf->membuf().data(), new_membuf->membuf().size());

    const unsigned long chunk_i = AdServer::Commons::uuid_distribution_hash(
      user_id) % chunks_count_;

    ConstSmartMemBufList new_mem_buf_list;
    new_mem_buf_list.push_back(Generics::transfer_membuf(new_membuf));

    bool signal_queue;

    {
      Sync::ConditionalGuard guard(queue_holder_->cond);

      const SaveQueue_var& queue = queue_holder_->queues[chunk_i];

      if((signal_queue = queue->bufs.empty()))
      {
        queue_holder_->non_empty_queues.push_back(queue);
      }

      queue->bufs.splice(queue->bufs.end(), new_mem_buf_list);
    }

    if(signal_queue)
    {
      queue_holder_->cond.signal();
    }
  }

  void
  UserOperationSaver::wait_object()
    /*throw(Generics::ActiveObject::Exception, eh::Exception)*/
  {
    Generics::CompositeActiveObject::wait_object();
    // dump files on waiting (not on destruct)
    rotate();
  }

  void
  UserOperationSaver::rotate() noexcept
  {
    FileMap files;

    {
      SyncPolicy::WriteGuard lock(files_holder_->files_lock);
      files.swap(files_holder_->files);
    }

    // dump files by destructors
  }
}
}
