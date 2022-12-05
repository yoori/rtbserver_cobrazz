#include <eh/Errno.hpp>
#include <Commons/PathManip.hpp>

#include <Generics/DirSelector.hpp>

#include <LogCommons/FileReceiver.hpp>

#include <UserInfoSvcs/UserInfoCommons/UserOperationProfiles.hpp>

#include "Compatibility/UserOperationProfilesAdapter.hpp"
#include "UserOperationSaver.hpp"
#include "UserOperationLoader.hpp"

namespace
{
  namespace Aspect
  {
    const char USER_OPERATION_LOADER[] = "UserOperationLoader";
  }

  const char DEFAULT_ERROR_DIR[] = "Error";
  const unsigned long FETCH_FILES_LIMIT = 50000;
}

namespace AdServer
{
namespace UserInfoSvcs
{
  /* BaseOperationRecordFetcher implementation */
  BaseOperationRecordFetcher::BaseOperationRecordFetcher(
    Generics::ActiveObjectCallback* callback,
    UserOperationProcessor* user_operation_processor,
    const char* folder,
    const char* unprocessed_folder,
    const char* file_prefix,
    const ChunkIdSet& chunk_ids,
    Generics::ActiveObject* interrupter)
    noexcept
    : user_operation_processor_(ReferenceCounting::add_ref(user_operation_processor)),
      log_errors_(ReferenceCounting::add_ref(callback)),
      DIR_(folder),
      unprocessed_dir_(unprocessed_folder),
      file_prefix_(file_prefix),
      chunk_ids_(chunk_ids),
      interrupter_(ReferenceCounting::add_ref(interrupter))
  {}

  void
  BaseOperationRecordFetcher::process(
    LogProcessing::FileReceiver::FileGuard* file_ptr)
    noexcept
  {
    static const char* FUN = "OperationRecordFetcher::process()";

    // file guard must be destroyed after moving file into errors store
    LogProcessing::FileReceiver::FileGuard_var file(
      ReferenceCounting::add_ref(file_ptr));

    try
    {
      if (!file)
      {
        return;
      }

      Generics::SmartMemBuf_var smart_mem_buf(new Generics::SmartMemBuf());
      Generics::MemBuf& mem_buf = smart_mem_buf.in()->membuf();

      LogProcessing::LogFileNameInfo file_name_info;
      LogProcessing::parse_log_file_name(file->file_name().c_str(), file_name_info);
      unsigned long chunk_id = file_name_info.distrib_index;

      // check, that chunk id controllable
      if(chunk_ids_.find(chunk_id) != chunk_ids_.end())
      {
        std::ifstream file_stream(file->full_path().c_str(), std::ios_base::binary);
        if(file_stream.fail())
        {
          Stream::Error ostr;
          ostr << "Can't open file";
          throw Exception(ostr);
        }

        bool terminated = false;
        unsigned long processed_records = 0;
        unsigned long processed_lines_count = file_name_info.processed_lines_count;

        while(true)
        {
          uint32_t record_size = 0;
          file_stream.read(reinterpret_cast<char*>(&record_size), 4);

          if(file_stream.eof())
          {
            break;
          }

          if(file_stream.fail())
          {
            Stream::Error ostr;
            ostr << "Reading failed";
            throw Exception(ostr);
          }

          mem_buf.alloc(record_size);
          file_stream.read(mem_buf.get<char>(), mem_buf.size());

          if(file_stream.eof() || file_stream.fail())
          {
            Stream::Error ostr;
            ostr << "Unexpected eof or fail";
            throw Exception(ostr);
          }

          if (processed_records >= file_name_info.processed_lines_count)
          {
            read_operation_(smart_mem_buf.in());
            ++processed_lines_count;
          }

          ++processed_records;

          if (interrupter_ && !interrupter_->active())
          {
            terminated = true;
            break;
          }
        }

        file_stream.close();

        if (terminated)
        {
          file_name_info.processed_lines_count = processed_lines_count;
          file_move_back_to_input_dir_(file_name_info, file->full_path().c_str());
        }
        else if(::unlink(file->full_path().c_str()) != 0)
        {
          Stream::Error ostr;
          ostr << FUN << ": Can't delete file '" << file->full_path() << "'";
          log_errors_->report_error(
            Generics::ActiveObjectCallback::ERROR, ostr.str());
        }
      }
      else
      {
        // move file to reprocess dir
        file_move_back_to_input_dir_(file_name_info, file->full_path().c_str());
      }
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;

      // copy the erroneous file to the error folder
      try
      {
        AdServer::LogProcessing::FileStore file_store(
          DIR_, DEFAULT_ERROR_DIR);
        file_store.store(file->full_path());
      }
      catch (const eh::Exception& store_ex)
      {
        ostr << FUN << store_ex.what() << " Can't copy the file '" <<
          file->full_path() << "' to the error folder. Initial error: " <<
          ex.what() << std::endl;
      }

      ostr << ex.what();
      log_errors_->report_error(
        Generics::ActiveObjectCallback::ERROR, ostr.str());
    }
  }

  void
  BaseOperationRecordFetcher::file_move_back_to_input_dir_(
    const AdServer::LogProcessing::LogFileNameInfo& info,
    const char* file_path) /*throw(eh::Exception)*/
  {
    static const char* FUN = "OperationRecordFetcher::file_move_back_to_input_dir_()";

    const std::string new_file_name = 
      AdServer::LogProcessing::restore_log_file_name(info, DIR_);
    
    std::string file_name;
    AdServer::PathManip::split_path(new_file_name.c_str(), 0, &file_name);
    std::string reprocess_path = unprocessed_dir_;
    reprocess_path += "/";
    reprocess_path += file_name;
    if(::rename(file_path, reprocess_path.c_str()) != 0)
    {
      Stream::Error ostr;
      ostr << FUN << "can't move file '" << file_path << "' to '" <<
        reprocess_path << "'";
      eh::throw_errno_exception<Exception>(ostr.str());
    }
  }

  InternalOperationRecordFetcher::InternalOperationRecordFetcher(
    Generics::ActiveObjectCallback* callback,
    UserOperationProcessor* user_operation_processor,
    const char* folder,
    const char* unprocessed_folder,
    const char* file_prefix,
    const ChunkIdSet& chunk_ids,
    Generics::ActiveObject* interrupter)
    noexcept
    : BaseOperationRecordFetcher(
        callback,
        user_operation_processor,
        folder,
        unprocessed_folder,
        file_prefix,
        chunk_ids,
        interrupter)
  {}

  ExternalOperationRecordFetcher::ExternalOperationRecordFetcher(
    Generics::ActiveObjectCallback* callback,
    UserOperationProcessor* user_operation_processor,
    const char* folder,
    const char* unprocessed_folder,
    const char* file_prefix,
    const ChunkIdSet& chunk_ids,
    Generics::ActiveObject* interrupter)
    noexcept
    : BaseOperationRecordFetcher(
        callback,
        user_operation_processor,
        folder,
        unprocessed_folder,
        file_prefix,
        chunk_ids,
        interrupter)
  {}
  
  /** ExternalUserOperationLoader */
  ExternalUserOperationLoader::ExternalUserOperationLoader(
    Generics::ActiveObjectCallback* callback,
    UserOperationProcessor* user_operation_processor,
    const char* operation_file_in_dir,
    const char* unprocessed_dir,
    const char* file_prefix,
    const BaseOperationRecordFetcher::ChunkIdSet& chunk_ids,
    const Generics::Time& check_period,
    std::size_t threads_count)
    /*throw(Exception)*/
    : log_errors_callback_(ReferenceCounting::add_ref(callback)),
      unprocessed_dir_(unprocessed_dir)
  {
    Generics::ActiveObject_var interrupter =
      new AdServer::LogProcessing::FileReceiverInterrupter();
    add_child_object(interrupter);

    operation_fetcher_ =
      new ExternalOperationRecordFetcher(
        log_errors_callback_,
        user_operation_processor,
        operation_file_in_dir,
        unprocessed_dir,
        file_prefix,
        chunk_ids,
        interrupter);

    file_thread_processor_ = new AdServer::LogProcessing::FileThreadProcessor(
      operation_fetcher_,
      callback,
      threads_count,
      (std::string(operation_file_in_dir) + "/Intermediate").c_str(),
      FETCH_FILES_LIMIT,
      operation_file_in_dir,
      file_prefix,
      check_period);

    add_child_object(file_thread_processor_);
  }

  ExternalUserOperationLoader::~ExternalUserOperationLoader() noexcept
  {}
  
  /** InternalUserOperationLoader */
  InternalUserOperationLoader::InternalUserOperationLoader(
    Generics::ActiveObjectCallback* callback,
    UserOperationProcessor* user_operation_processor,
    const char* operation_file_in_dir,
    const char* unprocessed_dir,
    const char* file_prefix,
    const BaseOperationRecordFetcher::ChunkIdSet& chunk_ids,
    const Generics::Time& check_period,
    std::size_t threads_count)
    /*throw(Exception)*/
    : log_errors_callback_(ReferenceCounting::add_ref(callback)),
      unprocessed_dir_(unprocessed_dir)
  {
    Generics::ActiveObject_var interrupter =
      new AdServer::LogProcessing::FileReceiverInterrupter();
    add_child_object(interrupter);

    operation_fetcher_ =
      new InternalOperationRecordFetcher(
        log_errors_callback_,
        user_operation_processor,
        operation_file_in_dir,
        unprocessed_dir,
        file_prefix,
        chunk_ids,
        interrupter);

    file_thread_processor_ = new AdServer::LogProcessing::FileThreadProcessor(
      operation_fetcher_,
      callback,
      threads_count,
      (std::string(operation_file_in_dir) + "/Intermediate").c_str(),
      FETCH_FILES_LIMIT,
      operation_file_in_dir,
      file_prefix,
      check_period);

    add_child_object(file_thread_processor_);
  }

  InternalUserOperationLoader::~InternalUserOperationLoader() noexcept
  {}
}
}

namespace AdServer
{
namespace UserInfoSvcs
{
  void
  ExternalOperationRecordFetcher::read_operation_(
    Generics::SmartMemBuf* smart_mem_buf)
    /*throw(eh::Exception)*/
  {
    static const char* FUN = "ExternalOperationRecordFetcher::read_operation_()";

    UserOperationTypeReader profile_type_reader(
      smart_mem_buf->membuf().data(), UserOperationTypeReader::FIXED_SIZE);
    Generics::MemBuf& mem_buf = smart_mem_buf->membuf();

    if (profile_type_reader.operation_type() == UserOperationSaver::ADD_AUDIENCE)
    {
      read_add_audience_operation_(mem_buf);
    }
    else if (profile_type_reader.operation_type() == UserOperationSaver::REMOVE_AUDIENCE)
    {
      read_remove_audience_operation_(mem_buf);
    }
    else
    {
      Stream::Error ostr;
      ostr << FUN << ": unknown operation: " << profile_type_reader.operation_type();
      throw Exception(ostr);
    }
  }

  void
  ExternalOperationRecordFetcher::read_add_audience_operation_(
    const Generics::MemBuf& mem_buf)
    /*throw(eh::Exception)*/
  {
    AudienceChannelsOperationReader reader(mem_buf.data(), mem_buf.size());
    const Generics::Uuid uuid(reader.user_id());
    AudienceChannelSet audience_channels;

    for (auto it = reader.audience_channels().begin();
         it != reader.audience_channels().end(); ++it)
    {
      const AudienceChannelDescriptorReader& channel = *it;
      audience_channels.insert({ channel.channel_id(), Generics::Time(channel.time()) });
    }

    user_operation_processor_->add_audience_channels(
      uuid,
      audience_channels);
  }

  void
  ExternalOperationRecordFetcher::read_remove_audience_operation_(
    const Generics::MemBuf& mem_buf)
    /*throw(eh::Exception)*/
  {
    AudienceChannelsOperationReader reader(mem_buf.data(), mem_buf.size());
    const Generics::Uuid uuid(reader.user_id());
    AudienceChannelSet audience_channels;

    for (auto it = reader.audience_channels().begin();
         it != reader.audience_channels().end(); ++it)
    {
      const AudienceChannelDescriptorReader& channel = *it;
      audience_channels.insert({ channel.channel_id(), Generics::Time(channel.time()) });
    }

    user_operation_processor_->remove_audience_channels(
      uuid,
      audience_channels);
  }
  
  void
  InternalOperationRecordFetcher::read_operation_(
    Generics::SmartMemBuf* smart_mem_buf)
    /*throw(eh::Exception)*/
  {
    static const char* FUN = "InternalOperationRecordFetcher::read_operation_()";

    UserOperationTypeReader profile_type_reader(
      smart_mem_buf->membuf().data(), UserOperationTypeReader::FIXED_SIZE);
    Generics::MemBuf& mem_buf = smart_mem_buf->membuf();

    if(profile_type_reader.operation_type() == UserOperationSaver::UO_FRAUD)
    {
      read_fraud_operation_(mem_buf);
    }
    else if(profile_type_reader.operation_type() == UserOperationSaver::UO_MATCH)
    {
      read_match_operation_(smart_mem_buf);
    }
    else if(profile_type_reader.operation_type() == UserOperationSaver::UO_MERGE)
    {
      read_merge_operation_(smart_mem_buf);
    }
    else if(profile_type_reader.operation_type() == UserOperationSaver::UO_FC_UPDATE)
    {
      read_fc_update_operation_(smart_mem_buf);
    }
    else if(profile_type_reader.operation_type() == UserOperationSaver::UO_FC_CONFIRM)
    {
      read_fc_confirm_operation_(smart_mem_buf);
    }
    else if (profile_type_reader.operation_type() == UserOperationSaver::ADD_AUDIENCE)
    {
      read_add_audience_operation_(mem_buf);
    }
    else if (profile_type_reader.operation_type() == UserOperationSaver::REMOVE_AUDIENCE)
    {
      read_remove_audience_operation_(mem_buf);
    }
    else
    {
      Stream::Error ostr;
      ostr << FUN << ": unknown operation: " << profile_type_reader.operation_type();
      throw Exception(ostr);
    }
  }


  void
  InternalOperationRecordFetcher::read_fraud_operation_(
    const Generics::MemBuf& mem_buf)
    /*throw(eh::Exception)*/
  {
    UserFraudOperationReader reader(mem_buf.data(), mem_buf.size());
    user_operation_processor_->fraud_user(
      UserId(reader.user_id()),
      Generics::Time(reader.fraud_time()));
  }

  void
  InternalOperationRecordFetcher::read_match_operation_(
    Generics::SmartMemBuf* smart_mem_buf)
    /*throw(eh::Exception)*/
  {
    UserMatchOperationProfilesAdapter match_profile_adapter;
    Generics::SmartMemBuf_var smb = match_profile_adapter(smart_mem_buf);

    UserMatchOperationReader reader(
      smb.in()->membuf().data(),
      smb.in()->membuf().size());

    CoordData coord_data;
    CoordData* coord_data_ptr = 0;
    if (reader.coord_data().size() != 0)
    {
      UserMatchOperationReader::coord_data_Container::const_iterator it =
        reader.coord_data().begin();

      coord_data.latitude.unpack((*it).latitude().get());
      coord_data.longitude.unpack((*it).longitude().get());
      coord_data.accuracy.unpack((*it).accuracy().get());

      coord_data_ptr = &coord_data;
    }
    
    UserOperationProcessor::RequestMatchParams channel_match_params(
      AdServer::Commons::UserId(reader.user_id()),
      Generics::Time(reader.time()),
      String::SubString(reader.cohort()), // cohort
      String::SubString(), // cohort2
      false, // use empty profile
      reader.request_colo_id(), // colo id
      Generics::Time(reader.repeat_trigger_timeout()),
      reader.filter_contextual_triggers(),
      reader.temporary(), // temporary
      false,
      false,
      false,
      false,
      false,
      reader.change_last_request(),
      reader.household() == 1,
      coord_data_ptr); 

    channel_match_params.no_result = true;

    ChannelMatchPack matched_channels;

    matched_channels.page_channels.reserve(reader.page_channels().size());

    for (UserMatchOperationReader::page_channels_Container::const_iterator it =
           reader.page_channels().begin(); it != reader.page_channels().end(); ++it)
    {
      matched_channels.page_channels.push_back(
        ChannelMatch((*it).channel_id(), (*it).channel_trigger_id()));
    }

    matched_channels.search_channels.reserve(reader.search_channels().size());

    for (UserMatchOperationReader::search_channels_Container::const_iterator it =
           reader.search_channels().begin(); it != reader.search_channels().end(); ++it)
    {
      matched_channels.search_channels.push_back(
        ChannelMatch((*it).channel_id(), (*it).channel_trigger_id()));
    }

    matched_channels.url_channels.reserve(reader.url_channels().size());

    for (UserMatchOperationReader::url_channels_Container::const_iterator it =
           reader.url_channels().begin(); it != reader.url_channels().end(); ++it)
    {
      matched_channels.url_channels.push_back(
        ChannelMatch((*it).channel_id(), (*it).channel_trigger_id()));
    }

    matched_channels.url_keyword_channels.reserve(reader.url_keyword_channels().size());

    for (UserMatchOperationReader::url_keyword_channels_Container::const_iterator it =
           reader.url_keyword_channels().begin(); it != reader.url_keyword_channels().end(); ++it)
    {
      matched_channels.url_keyword_channels.push_back(
        ChannelMatch((*it).channel_id(), (*it).channel_trigger_id()));
    }

    matched_channels.persistent_channels.reserve(reader.persistent_channels().size());

    std::copy(
      reader.persistent_channels().begin(),
      reader.persistent_channels().end(),
      std::inserter(matched_channels.persistent_channels,
        matched_channels.persistent_channels.begin()));

    ColoUserId colo_user_id;
    ChannelMatchMap result_channels;
    UserOperationProcessor::UserAppearance user_app;
    ProfileProperties properties;

    user_operation_processor_->match(
      channel_match_params,
      reader.last_colo_id(),
      reader.placement_colo_id(),
      colo_user_id,
      matched_channels,
      result_channels,
      user_app,
      properties,
      AdServer::ProfilingCommons::OP_BACKGROUND,
      0);
  }

  void
  InternalOperationRecordFetcher::read_merge_operation_(
    Generics::SmartMemBuf* smart_mem_buf)
    /*throw(eh::Exception)*/
  {
    UserMergeOperationProfilesAdapter merge_profile_adapter;
    Generics::SmartMemBuf_var smb = merge_profile_adapter(smart_mem_buf);
    UserMergeOperationReader reader(
      smb.in()->membuf().data(),
      smb.in()->membuf().size());
    
    Generics::MemBuf merge_base_profile;
    merge_base_profile.assign(
      reader.merge_base_profile().get(),
      reader.merge_base_profile().size());
    Generics::MemBuf merge_history_profile;
    merge_history_profile.assign(
      reader.merge_history_profile().get(),
      reader.merge_history_profile().size());
    Generics::MemBuf merge_freq_cap_profile;
    merge_freq_cap_profile.assign(
      reader.merge_freq_cap_profile().get(),
      reader.merge_freq_cap_profile().size());

    if(!reader.exchange_merge())
    {
      UserOperationProcessor::RequestMatchParams channel_match_params(
        AdServer::Commons::UserId(reader.user_id()),
        Generics::Time(reader.time()),
        String::SubString(), // cohort
        String::SubString(), // cohort2
        false, // use empty profile
        reader.request_colo_id(), // colo id
        Generics::Time::ZERO,
        false,
        false, // temporary
        false,
        false,
        false,
        false,
        false,
        reader.change_last_request(),
        reader.household() == 1); 

      Generics::MemBuf merge_add_profile;
      merge_add_profile.assign(
        reader.merge_add_profile().get(),
        reader.merge_add_profile().size());

      UserOperationProcessor::UserAppearance user_app;

      user_operation_processor_->merge(
        channel_match_params,
        merge_base_profile,
        merge_add_profile,
        merge_history_profile,
        merge_freq_cap_profile,
        user_app,
        0, // last_colo_id
        0, // current_placement_colo_id
        AdServer::ProfilingCommons::OP_BACKGROUND
        );
    }
    else
    {
      user_operation_processor_->exchange_merge(
        AdServer::Commons::UserId(reader.user_id()),
        merge_base_profile,
        merge_history_profile,
        0);
    }
  }

  void
  InternalOperationRecordFetcher::read_fc_update_operation_(
    Generics::SmartMemBuf* smart_mem_buf)
    /*throw(eh::Exception)*/
  {
    UserFreqCapUpdateOperationProfilesAdapter ufc_update_profile_adapter;
    Generics::SmartMemBuf_var smb = ufc_update_profile_adapter(smart_mem_buf);

    const UserFreqCapUpdateOperationReader reader(
      smb.in()->membuf().data(),
      smb.in()->membuf().size());
    UserFreqCapProfile::FreqCapIdList freq_caps;
    UserFreqCapProfile::FreqCapIdList uc_freq_caps;
    UserFreqCapProfile::FreqCapIdList virtual_freq_caps;
    UserFreqCapProfile::SeqOrderList seq_orders;
    UserFreqCapProfile::CampaignIds campaign_ids;
    UserFreqCapProfile::CampaignIds uc_campaign_ids;

    std::copy(
      reader.freq_caps().begin(),
      reader.freq_caps().end(),
      std::back_inserter(freq_caps));
    std::copy(
      reader.uc_freq_caps().begin(),
      reader.uc_freq_caps().end(),
      std::back_inserter(uc_freq_caps));
    std::copy(
      reader.virtual_freq_caps().begin(),
      reader.virtual_freq_caps().end(),
      std::back_inserter(virtual_freq_caps));
    std::copy(
      reader.campaign_ids().begin(),
      reader.campaign_ids().end(),
      std::back_inserter(campaign_ids));
    std::copy(
      reader.uc_campaign_ids().begin(),
      reader.uc_campaign_ids().end(),
      std::back_inserter(uc_campaign_ids));

    for (UserFreqCapUpdateOperationReader::seq_orders_Container::
           const_iterator it = reader.seq_orders().begin();
         it != reader.seq_orders().end(); ++it)
    {
      seq_orders.push_back(
        UserFreqCapProfile::SeqOrder(
          (*it).ccg_id(),
          (*it).set_id(),
          (*it).imps()));
    }

    user_operation_processor_->update_freq_caps(
      AdServer::Commons::UserId(reader.user_id()),
      Generics::Time(reader.time()),
      AdServer::Commons::RequestId(reader.request_id()),
      freq_caps,
      uc_freq_caps,
      virtual_freq_caps,
      seq_orders,
      campaign_ids,
      uc_campaign_ids,
      AdServer::ProfilingCommons::OP_BACKGROUND);
  }

  void
  InternalOperationRecordFetcher::read_fc_confirm_operation_(
    Generics::SmartMemBuf* smart_mem_buf)
    /*throw(eh::Exception)*/
  {
    UserFreqCapConfirmOperationProfilesAdapter ufc_confirm_profile_adapter;
    Generics::SmartMemBuf_var smb = ufc_confirm_profile_adapter(smart_mem_buf);
    
    std::set<unsigned long> publishers;
    
    UserFreqCapConfirmOperationReader reader(
      smb.in()->membuf().data(),
      smb.in()->membuf().size());

    std::copy(
      reader.publisher_accounts().begin(),
      reader.publisher_accounts().end(),
      std::inserter(publishers, publishers.begin()));
    
    user_operation_processor_->confirm_freq_caps(
      AdServer::Commons::UserId(reader.user_id()),
      Generics::Time(reader.time()),
      AdServer::Commons::RequestId(reader.request_id()),
      publishers);
  }

  void unpack_freq_cap_info(
    AdServer::Commons::FreqCap& res,
    const FreqCapInfoReader& fc)
  {
    res.fc_id = fc.fc_id();
    res.lifelimit = fc.lifelimit();
    res.period = Generics::Time(fc.period());
    res.window_limit = fc.window_limit();
    res.window_time = Generics::Time(fc.window_time());
  }

  void
  InternalOperationRecordFetcher::read_add_audience_operation_(
    const Generics::MemBuf& mem_buf)
    /*throw(eh::Exception)*/
  {
    AudienceChannelsOperationReader reader(mem_buf.data(), mem_buf.size());
    const Generics::Uuid uuid(reader.user_id());
    AudienceChannelSet audience_channels;

    for (auto it = reader.audience_channels().begin();
         it != reader.audience_channels().end(); ++it)
    {
      const AudienceChannelDescriptorReader& channel = *it;
      audience_channels.insert({ channel.channel_id(), Generics::Time(channel.time()) });
    }

    user_operation_processor_->add_audience_channels(
      uuid,
      audience_channels);
  }

  void
  InternalOperationRecordFetcher::read_remove_audience_operation_(
    const Generics::MemBuf& mem_buf)
    /*throw(eh::Exception)*/
  {
    AudienceChannelsOperationReader reader(mem_buf.data(), mem_buf.size());
    const Generics::Uuid uuid(reader.user_id());
    AudienceChannelSet audience_channels;

    for (auto it = reader.audience_channels().begin();
         it != reader.audience_channels().end(); ++it)
    {
      const AudienceChannelDescriptorReader& channel = *it;
      audience_channels.insert({ channel.channel_id(), Generics::Time(channel.time()) });
    }

    user_operation_processor_->remove_audience_channels(
      uuid,
      audience_channels);
  }
}
}
