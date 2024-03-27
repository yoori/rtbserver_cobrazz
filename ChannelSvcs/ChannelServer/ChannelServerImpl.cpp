
#include <ReferenceCounting/ReferenceCounting.hpp>

#include <eh/Exception.hpp>

#include <set>
#include <vector>
#include <deque>
#include <sstream>
#include <iostream>
#include <algorithm>

#include <Logger/Logger.hpp>
#include <Logger/FileLogger.hpp>
#include <Generics/ActiveObject.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/TaskRunner.hpp>
#include <Generics/Time.hpp>
#include <HTTP/UrlAddress.hpp>
#include <String/AsciiStringManip.hpp>
#include <String/UTF8Case.hpp>
#include <String/Tokenizer.hpp>
#include <Stream/MemoryStream.hpp>
#include <Language/GenericSegmentor/Polyglot.hpp>
//#include <Language/ChineeseSegmentor/NLPIR.hpp>
#include <Language/SegmentorManager/SegmentorManager.hpp>


#include <Commons/Constants.hpp>
#include <Commons/CorbaAlgs.hpp>
#include <Commons/GrpcAlgs.hpp>
#include <Commons/PathManip.hpp>
#include <Commons/CorbaConfig.hpp>
#include <ChannelSvcs/ChannelCommons/ChannelUtils.hpp>
#include <ChannelSvcs/ChannelCommons/TriggerParser.hpp>
//#include <ChannelSvcs/ChannelServer/ChannelServer_s.hpp>
#include <ChannelSvcs/DictionaryProvider/DictionaryProvider.hpp>
#include <CORBACommons/CorbaAdapters.hpp>
#include <CORBACommons/ServantImpl.hpp>

#include <xsd/AdServerCommons/AdServerCommons.hpp>
#include <xsd/ChannelSvcs/ChannelServerConfig.hpp>

#include "ChannelServerTypes.hpp"
#include "ChannelServerImpl.hpp"
#include "ContainerMatchers.hpp"
#include "ChannelContainer.hpp"


namespace
{
  template<class Response>
  auto create_grpc_response(const std::uint32_t id_request_grpc)
  {
    auto response = std::make_unique<Response>();
    response->set_id_request_grpc(id_request_grpc);
    return response;
  }

  template<class Response>
  auto create_grpc_error_response(
    const AdServer::ChannelSvcs::Proto::Error_Type error_type,
    const String::SubString detail,
    const std::uint32_t id_request_grpc)
  {
    auto response = create_grpc_response<Response>(id_request_grpc);
    auto* error = response->mutable_error();
    error->set_type(error_type);
    error->set_description(detail.data(), detail.length());
    return response;
  }
} // namespace

namespace AdServer
{
namespace ChannelSvcs
{
  typedef const String::AsciiStringManip::Char2Category<'"', ' '> HardWordSeparatorsType;

  const char* ChannelServerCustomImpl::ASPECT = "ChannelServer";
  const char* ChannelServerCustomImpl::MATCHASPECT = "Match";
  const char* ChannelServerCustomImpl::TRAFFICKING_PROBLEM =
    "TraffickingProblem";

  const char*
  ChannelServerStats::param_name[ChannelServerStats::PARAMS_MAX] =
    {
      "KeywordsTriggerCount",
      "IdKeywordsTriggerCount",
      "UrlsTriggerCount",
      "IdUrlsTriggerCount",
      "UidTriggerCount",
      "IdUidTriggerCount",
      "NsKeywordsTriggerCount",
      "NsUrlsTriggerCount",
      "MatchingsCount",
      "ExceptionsCount"
    };

  ChannelServerCustomImpl::ChannelServerCustomImpl(
    Logging::Logger* init_logger,
    const ChannelServerConfig* server_config) /*throw(Exception)*/
    : callback_(new Logging::ActiveObjectCallbackImpl(init_logger, "ChannelServerCustomImpl",
        ASPECT)),
      statistic_logger_(),
      c_adapter_(new CORBACommons::CorbaClientAdapter()),
      colo_(0),
      ready_(false),
      source_id_(-1),
      progress_(new ProgressCounter(STAGES_COUNT, 0)),
      load_keywords_(0),
      state_(UpdateData::US_ZERO),
      scheduler_(new Generics::Planner(callback_)),
      task_runner_(new Generics::TaskRunner(callback_,1)),
      segmentor_(),
      SERVICE_INDEX_(server_config->service_index()),
      update_period_(server_config->update_period()),
      reschedule_period_(0),
      variant_server_(),
      new_variant_server_(),
      count_chunks_(0),
      count_container_chunks_(server_config->count_chunks()),
      merge_limit_(server_config->merge_size() * 1024 * 1024),
      container_(new ChannelContainer(
          count_container_chunks_,
          server_config->MatchOptions().nonstrict())),
      queries_counter_(0)
  {
    AdServer::ChannelSvcs::ChannelServerSessionFactory::init(
      *c_adapter_,
      0,
      &load_session_factory_,
      callback_,
      0,
      0,
      10 // threads
      );

    try
    {
      if(server_config->reschedule_period().present())
      {
        reschedule_period_ = server_config->reschedule_period().get();
      }
      if(server_config->ccg_portion().present())
      {
        ccg_limit_ = server_config->ccg_portion().get();
      }
      else
      {
        ccg_limit_ = merge_limit_ / (1024 * 8);
        //experimental measurement, size of one keyword is about 6K
      }
      for(xsd::AdServer::Configuration::
          MatchOptionsType::AllowPort_sequence::const_iterator it =
          server_config->MatchOptions().AllowPort().begin();
          it != server_config->MatchOptions().AllowPort().end(); ++it)
      {
        ports_.insert(*it);
      }
      if(server_config->MatchOptions().match_logger_file().present())
      {
        Logging::File::Policies::PolicyList pl;
        Logging::File::Config config(
          server_config->MatchOptions().match_logger_file().get().c_str(),
          pl);
        config.log_level = Logging::BaseLogger::TRACE;
        statistic_logger_ = new Logging::File::Logger(std::move(config));
      }

      CORBACommons::CorbaObjectRefList dictionary_server_refs;
      if(server_config->DictionaryRefs().present())
      {
        Config::CorbaConfigReader::read_multi_corba_ref(
          server_config->DictionaryRefs().get(), dictionary_server_refs);

        dict_matcher_.reset(new DictionaryMatcher(
            *c_adapter_, dictionary_server_refs, logger()));
      }

      update_cont_.reset(new UpdateContainer(container_.get(), dict_matcher_.get()));

      if(server_config->Segmentors().present())
      {
        auto matching_segmentor_cfg =
          server_config->Segmentors().get().matching_segmentor();
        
        for(xsd::AdServer::Configuration::SegmentorSettingsType::Segmentor_const_iterator
            it = server_config->Segmentors().get().Segmentor().begin();
            it != server_config->Segmentors().get().Segmentor().end(); ++it)
        {
          try
          {
            if(it->name() == "Polyglot")
            {
              if(!normal_segmentor_)
              {
                normal_segmentor_ =
                  new Language::Segmentor::NormalizePolyglotSegmentor(it->base().c_str());
                if(matching_segmentor_cfg &&
                  *matching_segmentor_cfg == it->name())
                {
                  segmentor_ = normal_segmentor_;
                  logger()->log(String::SubString(
                      "Use Polyglot as default segmentor"),
                    Logging::Logger::TRACE,
                    ASPECT);
                }
              }
              segmentors_[it->country()] = normal_segmentor_;
            }
            /*
            else if(it->name() == "Nlpir")
            {
              if(!china_segmentor_)
              {
                china_segmentor_ =
                  new Language::Segmentor::Chineese::NlpirSegmentor(it->base().c_str());
                if(matching_segmentor_cfg &&
                  *matching_segmentor_cfg == it->name())
                {
                  segmentor_ = china_segmentor_;
                  logger()->log(String::SubString(
                      "Use NLPir as default segmentor"),
                    Logging::Logger::TRACE,
                    ASPECT);
                }
              }
              segmentors_[it->country()] = china_segmentor_;
            }
            */
          }
          catch(const eh::Exception& e)
          {
            Stream::Error ostr;
            ostr << __func__ << ": eh::Exception on initialization Segmentor. "
              "Parameters: base = '" << it->base()
              << "' : " << e.what();
            logger()->log(ostr.str(),
              Logging::Logger::WARNING,
              ASPECT,
              "ADS-IMPL-13");
          }
        }
        if(!segmentor_)
        {
          if(normal_segmentor_)
          {
            logger()->log(String::SubString(
                "Choose Polyglot as default segmentor"),
              Logging::Logger::TRACE,
              ASPECT);
            segmentor_ = normal_segmentor_;
          }
          else
          {
            logger()->log(String::SubString(
                "Choose NLPir as default segmentor"),
              Logging::Logger::TRACE,
              ASPECT);
            segmentor_ = china_segmentor_;
          }
        }
      }

      init_logger_(server_config);

      add_child_object(task_runner_);
      add_child_object(scheduler_);

      schedule_update_task_(1, 0);
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << __func__ << ": eh::Exception on reading config :" << e.what();
      throw Exception(ostr);
    }
  }

  ChannelServerCustomImpl::~ChannelServerCustomImpl() noexcept
  {
  }

  void ChannelServerCustomImpl::init_logger_(
    const ChannelServerConfig* config) noexcept
  {
    try
    {
      update_stat_logger_.reset();
      if(config->UpdateStatLogger().present())
      {
        logger()->log(String::SubString("Create ColoUpdateStat"),
          Logging::Logger::TRACE, ASPECT);
        const xsd::AdServer::Configuration::LogFlushPolicyType& statistic =
          config->UpdateStatLogger().get();
        std::string path;
        unsigned long size = 0;
        if(statistic.size().present())
        {
          size = statistic.size().get();
        }
        if(statistic.path().present())
        {
          path = config->log_root();
          PathManip::create_path(path, statistic.path().get().c_str());
        }
        update_stat_logger_ = new ChannelUpdateStatLogger(
          size,
          statistic.period(),
          path.c_str());
      }
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << "ChannelServerCustomImpl::init_logger_: "
        "caught ChannelProxyImpl::Exception. : "
        << e.what();

      logger()->log(
        ostr.str(),
        Logging::Logger::ERROR,
        ASPECT,
        "ADS-IMPL-34");
    }
  }

  void ChannelServerCustomImpl::init_update_(
    UpdateData* update_data,
    ChannelServerVariantBase* variant_server)
    /*throw(
      ChannelServerException::TemporyUnavailable,
      ChannelServerException::NotReady,
      ChannelServerException::Exception,
      eh::Exception)*/
  {
    bool campaign_server_exception = false;
    try
    {
      variant_server->update_actual_channels(update_data);
    }
    catch(const ChannelServerException::NotReady& e)
    {
      if (ready_)
      {
        Stream::Error ostr;
        ostr << __func__ << ": ChannelServerException::NotReady: " << e.what();
        logger()->log(ostr.str(),
            Logging::Logger::ERROR,
            ASPECT,
            "ADS-IMPL-15");

        campaign_server_exception = true;
      }
      else
      {
        throw;
      }
    }
    catch(const ChannelServerException::Exception& e)
    {
      if (ready_)
      {//use existing data from CampaignServer
        Stream::Error ostr;
        ostr << __func__ << ": ChannelServerException::Exception: " << e.what();
        logger()->log(ostr.str(),
            Logging::Logger::ERROR,
            ASPECT,
            "ADS-IMPL-15");

        campaign_server_exception = true;
      }
      else
      {
        throw;
      }
    }
    if (campaign_server_exception)
    {
      update_data->info_ptr = 
        update_data->container_ptr->get_old_info();
    }
    update_data->merge_size =
      update_data->container_ptr->check_actual(*update_data->info_ptr);

    if(update_data->info_ptr->size() == 0)
    {//case of absent of channels
      ready_ = true;
    }

    logger()->log(
        String::SubString("got actual channels."),
        Logging::Logger::TRACE,
        ASPECT);

    variant_server->check_updating(update_data);

    progress_->reset(STAGES_COUNT, update_data->size());
    update_data->progress = progress_.get();

    if(logger()->log_level() >= Logging::Logger::TRACE)
    {
      std::ostringstream ostr;
      trace_sequence(
        "New channels", update_data->container_ptr->get_new(), ostr);
      trace_sequence("Removed channels", update_data->container_ptr->get_removed(), ostr);
      trace_sequence("Updated channels", update_data->container_ptr->get_updated(), ostr);
      trace_sequence("Marked on load trigger channels", update_data->check_data, ostr);
      trace_sequence("Marked on load uid channels", update_data->uid_check_data, ostr);

      if(ostr.str().size())
      {
        logger()->log(
          ostr.str(),
          Logging::Logger::TRACE,
          ASPECT);
      }
    }
  }

  void ChannelServerCustomImpl::trace_ccg_info_changing_(
    const UpdateData* update_data)
    noexcept
  {
    Stream::Error trace_out;
    logger()->log(
      update_data->trace_ccg_info_changing(trace_out).str(),
      Logging::Logger::TRACE,
      ASPECT);
  }

  void ChannelServerCustomImpl::update_task(
      UpdateData* update_data) noexcept
  {
    unsigned long next_time = reschedule_period_;
    ChannelServerVariantBase_var variant_server = get_source_of_data_();
    bool exception = false;
    try
    {
      logger()->log(String::SubString("update task."),
        Logging::Logger::TRACE, ASPECT);
      if(state_ != UpdateData::US_FINISH)
      {
        state_ = update_data->state;
      }
      switch(update_data->state)
      {
        case UpdateData::US_ZERO:
          if(change_source_of_data_())
          {
            update_data->state = UpdateData::US_START;
            next_time = 0;
            //std::cerr << "START LOADING: " << Generics::Time::get_time_of_day().gm_ft() << std::endl;
          }
          else
          {
            next_time = std::max(reschedule_period_, 5UL);
          }
          break;
        case UpdateData::US_START:
          if(change_source_of_data_())
          {
            variant_server = get_source_of_data_();
          }
          //get actual channels and check updating
          logger()->log(String::SubString("start update."),
            Logging::Logger::TRACE, ASPECT);
          init_update_(update_data, variant_server);
          update_data->state = UpdateData::US_CHECKED;
          break;
        case UpdateData::US_CHECKED:
          logger()->sstream(Logging::Logger::TRACE, ASPECT)
            << "loading ccg keywords, id = " << update_data->start_ccg_id
            << ", limit = " << ccg_limit_;
          /* Loading ccg keywordss */
          variant_server->update_ccg(update_data, ccg_limit_);
          {
            load_keywords_ = update_data->new_ccg_map->active().size();
          }
          if(update_data->ccg_loaded)
          {
            update_data->state = UpdateData::US_CCG_LOADED;
            container_->set_ccg(update_data->new_ccg_map);
            if(logger()->log_level() >= Logging::Logger::TRACE)
            {
              trace_ccg_info_changing_(update_data);
            }
          }
          break;
        case UpdateData::US_CCG_LOADED:
          if(update_data->merge_size < merge_limit_)
          {
            size_t max_load_size = (merge_limit_ - update_data->merge_size);

            variant_server->update(max_load_size, update_data);

          }
          if(update_data->check_data.empty())//all trigger lists loaded
          {
              update_data->need_merge = true;
          }
          if(update_data->empty())//all trigger lists loaded
          {
            update_data->state = UpdateData::US_LOADED;
          }
          break;
        case UpdateData::US_LOADED:
          update_data->need_merge = true;
          break;
        default:
          break;
      }

      logger()->sstream(Logging::Logger::DEBUG, ASPECT)
        << "Merge size = " << update_data->merge_size
        << ", merge limit = " << merge_limit_
        << ", force merge = " << (update_data->need_merge ? "true" : "false")
        << ", keywords = " << (update_data->new_ccg_map ?
          update_data->new_ccg_map->active().size() : 0);

      if(update_data->merge_size >= merge_limit_ * 9 / 10 || update_data->need_merge)
      {
        bool all_loaded = (update_data->state ==
           UpdateData::US_LOADED);

        Generics::Timer timer;
        std::ostringstream debug;

        timer.start();

        container_->merge(
          *update_data->container_ptr,
          *update_data->info_ptr,
          all_loaded,
          progress_.get(),
          update_data->new_master,
          variant_server_->get_first_load_stamp(),
          logger()->log_level() >= Logging::Logger::TRACE ? &debug : 0);

        timer.stop();

        progress_->change_stage(PROGRESS_LOAD);

        logger()->sstream(Logging::Logger::DEBUG, ASPECT)
          << "Merge time: " << timer.elapsed_time();
        update_data->merge_size = 0;
        update_data->need_merge = false;

        if(update_data->state == UpdateData::US_LOADED && all_loaded)
        {
          update_data->state = UpdateData::US_FINISH;
          //std::cerr << "FINISH LOADING: " << Generics::Time::get_time_of_day().gm_ft() << std::endl;
        }
        if(logger()->log_level() >= Logging::Logger::TRACE &&
           !debug.str().empty())
        {
          logger()->log(debug.str(), Logging::Logger::TRACE, ASPECT);
        }
      }

      if(update_data->state == UpdateData::US_FINISH)
      {
        update_data->end_iteration();

        source_id_ = variant_server_->get_source_id();

        ready_ = true;
        log_update_();

        logger()->log(
          String::SubString("update finished."),
          Logging::Logger::TRACE,
          ASPECT);
        next_time = update_period_;
      }
    }
    catch(const ChannelServerException::TemporyUnavailable&)
    {
      logger()->log(
        String::SubString("Server in suspend mode"),
        Logging::Logger::TRACE,
        ASPECT);
      next_time = update_period_;
    }
    catch(const ChannelServerException::NotReady& e)
    {
      Stream::Error ostr;
      ostr << __func__ << ": ChannelServerException::NotReady: " << e.what();
      logger()->log(ostr.str(),
          Logging::Logger::NOTICE,
          ASPECT,
          "ADS-IMPL-15");

      next_time = std::max(reschedule_period_, 5UL);
    }
    catch(const ChannelServerException::Exception& e)
    {
      Stream::Error ostr;
      ostr << __func__ << ": ChannelServerException::Exception: " << e.what();

      logger()->log(ostr.str(),
          Logging::Logger::ERROR,
          ASPECT,
          "ADS-IMPL-15");
      exception  = true;
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << __func__ << ": eh::Exception: " << e.what();

      logger()->log(ostr.str(),
          Logging::Logger::ERROR,
          ASPECT,
          "ADS-IMPL-15");
      exception  = true;
    }
    if(exception)
    {
      next_time = std::min(20UL, update_period_/2);
      Stream::Error ostr;
      ostr <<
        "update wasn't finished due to exception. Next try will be in "
        << next_time << " seconds.";
      logger()->log(
        ostr.str(),
        Logging::Logger::TRACE,
        ASPECT);
    }
    schedule_update_task_(next_time, update_data);
  }

  void ChannelServerCustomImpl::schedule_update_task_(
    unsigned long period, UpdateData* update_data) noexcept
  {
    try
    {
      Task_var msg = new UpdateTask(this, update_data, task_runner_);
      Generics::Time tm = Generics::Time::get_time_of_day();
      tm += period;
      scheduler_->schedule(msg, tm);
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << __func__ << ": eh::Exception: " << e.what();

      logger()->log(ostr.str(),
        Logging::Logger::CRITICAL,
        ASPECT,
        "ADS-IMPL-10");
    }
  }

  void ChannelServerCustomImpl::check(
    const ::AdServer::ChannelSvcs::ChannelCurrent::CheckQuery& query,
    ::AdServer::ChannelSvcs::ChannelCurrent::CheckData_out data)
    /*throw(AdServer::ChannelSvcs::ImplementationException,
      AdServer::ChannelSvcs::NotConfigured)*/
  {
    const char* FN = "ChannelServerCustomImpl::check";
    if(!ready())
    {
      throw AdServer::ChannelSvcs::NotConfigured();
    }
    try
    {
      std::set<unsigned int> force_id;
      Generics::Time old_master = CorbaAlgs::unpack_time(query.master_stamp);
      data = new ChannelCurrent::CheckData;

      CheckInformation::CheckMap ids;
      force_id.insert(
        query.new_ids.get_buffer(),
        query.new_ids.get_buffer() + query.new_ids.length()); 

      CheckInformation info;
      container_->check_update(old_master, force_id, info, query.use_only_list);

      data->special_track = info.special_track;
      data->special_adv = info.special_adv;
      data->first_stamp = CorbaAlgs::pack_time(info.first_master);
      data->master_stamp = CorbaAlgs::pack_time(info.master);
      data->max_time = CorbaAlgs::pack_time(info.longest_update);
      data->versions.length(info.data.size());
      data->source_id = source_id_;
      size_t i = 0;
      for(CheckInformation::CheckMap::const_iterator
          it = info.data.begin(); it != info.data.end(); ++it, i++)
      {
        data->versions[i].id = it->first;
        data->versions[i].size = it->second.size;
        data->versions[i].stamp = CorbaAlgs::pack_time(it->second.stamp);
      }
      if(logger()->log_level() >= Logging::Logger::TRACE)
      {
        std::ostringstream ostr;
        ostr << "Ask stamp '"
          << old_master.get_gm_time() << "',";
        ostr << " answer stamp '"
          << info.master.get_gm_time() << "' ";
        for(CheckInformation::CheckMap::const_iterator it = ids.begin();
            it != ids.end(); ++it)
        {
          ostr << "id = " << it->first << ", "
            << it->second.size << ", '"
            << it->second.stamp.get_gm_time()
            << "'; ";
        }
        logger()->log(ostr.str(), Logging::Logger::TRACE, ASPECT);
      }
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FN << ": caught eh::Exception. : " << e.what();
      logger()->log(ostr.str(),
        Logging::Logger::ERROR,
        ASPECT,
        "ADS-IMPL-41");
      CORBACommons::throw_desc<
        ChannelSvcs::ImplementationException>(
          ostr.str());
    }
  }

  void ChannelServerCustomImpl::log_update_() noexcept
  {
    const char* FN = "ChannelServerCustomImpl::log_update_";
    try
    {
      if(!update_stat_logger_ || version_.empty())
      {
        return;
      }
      logger()->log(String::SubString("Log ColoUpdateStat"),
        Logging::Logger::TRACE, ASPECT);
      update_stat_logger_->process_config_update(colo_, version_);
      update_stat_logger_->flush_if_required();
    }
    catch(const ChannelUpdateStatLogger::Exception& e)
    {
      Stream::Error ostr;
      ostr << FN << ":caught ChannelColoUpdateStatLogger::Exception. "
        ": " << e.what();
      logger()->log(
        ostr.str(),
        Logging::Logger::WARNING,
        ASPECT,
        "ADS-IMPL-32");
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FN << ":caught eh::Exception. : "
        << e.what();
      logger()->log(
        ostr.str(),
        Logging::Logger::WARNING,
        ASPECT,
        "ADS-IMPL-32");
    }
  }

  void ChannelServerCustomImpl::update_triggers(
    const ::AdServer::ChannelSvcs::ChannelIdSeq& ids,
    ::AdServer::ChannelSvcs::ChannelCurrent::UpdateData_out result)
    /*throw(AdServer::ChannelSvcs::ImplementationException,
      AdServer::ChannelSvcs::NotConfigured)*/
  {
    static const char* FN = "ChannelServerCustomImpl::update_triggers";

    try
    {
      if(!ready())
      {
        throw AdServer::ChannelSvcs::NotConfigured();
      }
      Generics::Timer timer;

      timer.start();

      result =
        new AdServer::ChannelSvcs::ChannelCurrent::UpdateData;

      result->source_id = source_id_;

      if(ids.length())
      {
        ChannelContainerBase::ChannelMap buffer;

        for(size_t i = 0; i < ids.length(); i++)
        {
          buffer.insert(
            buffer.end(),
            std::make_pair(
              ids[i],
              static_cast<ChannelContainerBase::ChannelUpdateData*>(0)));
        }

        container_->fill(buffer);
        size_t j = 0;
        size_t allocated_size = 0;
        result->channels.length(buffer.size());
        for(ChannelContainerBase::ChannelMap::const_iterator i =
            buffer.begin(); i != buffer.end(); ++i)
        {
          const ChannelContainerBase::ChannelMap::mapped_type& info =
            i->second;
          if(info)
          {
            AdServer::ChannelSvcs::ChannelCurrent::ChannelById& add =
              result->channels[j++];
            add.channel_id = i->first;
            add.words.length(info->triggers.size());
            size_t k = 0;
            for(ChannelContainerBase::ChannelUpdateData::
                TriggerItemVector::const_iterator tr_it = info->triggers.begin();
                tr_it != info->triggers.end(); ++tr_it, k++)
            {
              const std::string& trigger = tr_it->matcher->get_trigger();
              add.words[k].channel_trigger_id = tr_it->channel_trigger_id;
              add.words[k].trigger.length(trigger.size());
              allocated_size += trigger.size();
              memcpy(
                add.words[k].trigger.get_buffer(),
                trigger.data(),
                trigger.size());
            }
            add.stamp = CorbaAlgs::pack_time(info->stamp);
          }
        }
        result->channels.length(j);
        if(logger()->log_level() >= Logging::Logger::TRACE)
        {
          std::ostringstream ostr;
          ostr << "Asked channels:";
          CorbaAlgs::print_sequence(ostr, ids);
          for(size_t i = 0; i < result->channels.length(); i++)
          {
            const AdServer::ChannelSvcs::ChannelCurrent::ChannelById& add =
              result->channels[i];
            ostr << "{ " << add.channel_id << ", "
              << add.words.length() << ", "
              << allocated_size << ", "
              << CorbaAlgs::unpack_time(
                add.stamp).get_gm_time()
              << "}; ";
          }
          logger()->log(ostr.str(), Logging::Logger::TRACE, ASPECT);
        }
      }
      timer.stop();
      logger()->sstream(Logging::Logger::DEBUG, ASPECT)
        << "Process update time: " << timer.elapsed_time();
    }
    catch(const ChannelContainer::Exception& e)
    {
      Stream::Error ostr;
      ostr << FN << ": caught eh::Exception "
        "on filling query data. : " << e.what();
      logger()->log(ostr.str(),
        Logging::Logger::ERROR,
        ASPECT,
        "ADS-IMPL-42");
      CORBACommons::throw_desc<
        ChannelSvcs::ImplementationException>(
          ostr.str());
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << ": caught eh::Exception "
        "on filling query data. : " << e.what();
      logger()->log(ostr.str(),
        Logging::Logger::ERROR,
        ASPECT,
        "ADS-IMPL-42");
      CORBACommons::throw_desc<
        ChannelSvcs::ImplementationException>(
          ostr.str());
    }
    catch(const CORBA::SystemException& e)
    {
      Stream::Error ostr;
      ostr << FN << ": caught CORBA::SystemException "
        "on filling query data. : " << e;
      logger()->log(
        ostr.str(), Logging::Logger::ERROR, ASPECT, "ADS-IMPL-42");
      CORBACommons::throw_desc<
        ChannelSvcs::ImplementationException>(
          ostr.str());
    }
  }

  void ChannelServerCustomImpl::update_all_ccg(
    const AdServer::ChannelSvcs::ChannelCurrent::CCGQuery& query,
    AdServer::ChannelSvcs::ChannelCurrent::PosCCGResult_out result)
    /*throw(AdServer::ChannelSvcs::ImplementationException,
      AdServer::ChannelSvcs::NotConfigured)*/
  {
    const char* FN = "ChannelServerCustomImpl::update_all_ccg";
    if(!ready())
    {
      throw AdServer::ChannelSvcs::NotConfigured();
    }
    try
    {
      result = new ChannelCurrent::PosCCGResult;

      Generics::Time old_master = CorbaAlgs::unpack_time(query.master_stamp);
      std::map<unsigned long, Generics::Time> deleted;
      std::set<unsigned long> get_id;
      std::vector<CCGKeyword_var> buffer;

      get_id.insert(
        query.channel_ids.get_buffer(),
        query.channel_ids.get_buffer() + query.channel_ids.length());
      container_->get_ccg_update(
        old_master,
        query.start,
        query.limit, get_id, buffer, deleted, query.use_only_list);

      result->keywords.length(buffer.size());
      size_t count = 0;
      for(std::vector<CCGKeyword_var>::const_iterator it = buffer.begin();
          it != buffer.end(); ++it)
      {
        result->keywords[count++] =
          CCGKeywordPacker::convert<ChannelCurrent::CCGKeyword>(*(*it));
      }
      DeletedPacker<
        std::map<unsigned long, Generics::Time>,
        ChannelCurrent::TriggerVersionSeq>
          (deleted, result->deleted);

      result->source_id = source_id_;
      if(buffer.size())
      {
        result->start_id = (*buffer.rbegin())->channel_id + 1;
        if(!get_id.empty() && buffer.size() < query.limit)
        {
          result->start_id = std::max(
            static_cast<unsigned long>(result->start_id), *get_id.rbegin() + 1);
        }
      }
      else
      {
        if(get_id.empty())
        {
          result->start_id = query.start + 1;
        }
        else
        {
          result->start_id = *get_id.rbegin() + 1;
        }
      }
    }
    catch(const ChannelContainer::Exception& e)
    {
      Stream::Error ostr;
      ostr << FN << ": caught eh::Exception "
        "on filling query data. : " << e.what();
      logger()->log(
        ostr.str(), Logging::Logger::ERROR, ASPECT, "ADS-IMPL-42");
      CORBACommons::throw_desc<
        ChannelSvcs::ImplementationException>(
          ostr.str());
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FN << ": caught eh::Exception "
        "on filling query data. : " << e.what();
      logger()->log(
        ostr.str(), Logging::Logger::ERROR, ASPECT, "ADS-IMPL-42");
      CORBACommons::throw_desc<
        ChannelSvcs::ImplementationException>(
          ostr.str());
    }
    catch(const CORBA::SystemException& e)
    {
      Stream::Error ostr;
      ostr << FN << ": caught CORBA::SystemException "
        "on filling query data. : " << e;
      logger()->log(
        ostr.str(), Logging::Logger::ERROR, ASPECT, "ADS-IMPL-42");
      CORBACommons::throw_desc<
        ChannelSvcs::ImplementationException>(
          ostr.str());
    }
  }

  //
  // IDL:AdServer/ChannelSvcs/ChannelProxy/get_count_chunks:1.0
  //
  ::CORBA::ULong ChannelServerCustomImpl::get_count_chunks()
      /*throw(AdServer::ChannelSvcs::ImplementationException)*/
  {
    return count_chunks_;
  }

  //
  // IDL:AdServer/ChannelSvcs/ChannelServer/match:1.0
  //
  void ChannelServerCustomImpl::match(
      const ::AdServer::ChannelSvcs::ChannelServerBase::MatchQuery& query,
      ::AdServer::ChannelSvcs::ChannelServer::MatchResult_out result)
    /*throw(AdServer::ChannelSvcs::ImplementationException,
      AdServer::ChannelSvcs::NotConfigured)*/
  {
    try
    {
      Generics::Timer timer;
      static MatchBreakSeparators separators;
      timer.start();
      result = new ::AdServer::ChannelSvcs::ChannelServer::MatchResult;
      if(state_ == UpdateData::US_ZERO)
      {
        throw AdServer::ChannelSvcs::NotConfigured(
          "Source chunks wasn't setted for server yet");
      }
      std::unique_ptr<std::ostringstream> logstr;
      TriggerMatchRes res;
      const Generics::Uuid uid = CorbaAlgs::unpack_user_id(query.uid);
      if (statistic_logger_)
      {
        logstr.reset(new std::ostringstream);
        *logstr << query.request_id << "::u:" <<  query.urls
          << "::p:" <<  query.pwords
          << "::s:" <<  query.swords
          << "::U:" <<  uid.to_string(false);
      }
      unsigned int match_flags =
        (query.non_strict_word_match ? MF_NONSTRICTKW : MF_NONE) |
        (query.non_strict_url_match ? MF_NONSTRICTURL : MF_NONE) |
        (query.return_negative ? MF_NEGATIVE : MF_NONE) |
        (query.statuses[0] == 'A' ||
         query.statuses[1] == 'A' ? MF_ACTIVE : MF_NONE) |
        (query.statuses[0] == 'I' ||
         query.statuses[1] == 'I' ? MF_INACTIVE : MF_NONE);

      AdServer::ChannelSvcs::MatchWords phrases[CT_MAX];
      AdServer::ChannelSvcs::MatchWords additional_url_keywords;
      MatchUrls url_words, additional_url_words;
      //parsing urls
      ChannelContainer::match_parse_urls(
        String::SubString(query.first_url),
        String::SubString(query.first_url_words),
        ports_,
        query.non_strict_url_match,
        url_words,
        phrases[CT_URL_KEYWORDS],
        logger(),
        segmentor_);
      ChannelContainer::match_parse_urls(
        String::SubString(query.urls),
        String::SubString(query.urls_words),
        ports_,
        query.non_strict_url_match,
        additional_url_words,
        additional_url_keywords,
        logger(),
        segmentor_);

      StringVector exact_phrases;
      parse_keywords(
        String::SubString(query.swords),
        phrases[CT_SEARCH],
        match_flags & MF_NONSTRICTKW ? PM_SIMPLIFY : PM_NO_SIMPLIFY,
        match_flags & MF_NONSTRICTKW ? nullptr : &separators,
        match_flags & MF_NONSTRICTKW ? 1 : Commons::DEFAULT_MAX_HARD_WORD_SEQ,
        &exact_phrases,//exact match
        match_flags & MF_NONSTRICTKW ? segmentor_ : 0);
      parse_keywords(
        String::SubString(query.pwords),
        phrases[CT_PAGE],
        (query.simplify_page || match_flags & MF_NONSTRICTKW) ?
        PM_SIMPLIFY : PM_NO_SIMPLIFY,
        match_flags & MF_NONSTRICTKW ? nullptr : &separators,
        match_flags & MF_NONSTRICTKW ? 1 : Commons::DEFAULT_MAX_HARD_WORD_SEQ,
        0,//exact match
        (query.simplify_page || match_flags & MF_NONSTRICTKW ?
         segmentor_ : 0));

      phrases[CT_URL_KEYWORDS].insert(
        exact_phrases.begin(), exact_phrases.end());

      container_->match(
        url_words,
        additional_url_words,
        phrases,
        additional_url_keywords,
        exact_phrases,
        uid,
        match_flags,
        res);
      fill_result_(res, *result, query.fill_content);
      timer.stop();
      if (statistic_logger_)
      {
        log_parsed_input_(url_words, phrases, exact_phrases, *logstr);
        log_result_(res, timer.elapsed_time(), *logstr);
        statistic_logger_->log(logstr->str(), Logging::Logger::DEBUG, ASPECT);
      }
      result->match_time = CorbaAlgs::pack_time(timer.elapsed_time());
      __gnu_cxx::__atomic_add(&queries_counter_, 1);
    }
    catch(const ChannelContainer::Exception& e)
    {
      Stream::Error ostr;
      ostr << "ChannelServerCustomImpl::match: Caught "
        "ChannelContainer::Exception : " << e.what();
      logger()->log(
          ostr.str(),
          Logging::Logger::ERROR,
          ASPECT,
          "ADS-IMPL-43");
      CORBACommons::throw_desc<
        ChannelSvcs::ImplementationException>(
          ostr.str());
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << "ChannelServerCustomImpl::match: Caught eh::Exception "
        ": " << e.what();
      logger()->log(
        ostr.str(),
        Logging::Logger::ERROR,
        ASPECT,
        "ADS-IMPL-43");
      CORBACommons::throw_desc<
        ChannelSvcs::ImplementationException>(
          ostr.str());
    }
  }

  ChannelServerCustomImpl::MatchResponsePtr
  ChannelServerCustomImpl::match(MatchRequestPtr&& request)
  {
    const auto id_request_grpc = request->id_request_grpc();
    auto& query = request->query();
    try
    {
      Generics::Timer timer;
      static MatchBreakSeparators separators;
      timer.start();

      auto response = create_grpc_response<Proto::MatchResponse>(
        id_request_grpc);
      auto* info = response->mutable_info();

      if(state_ == UpdateData::US_ZERO)
      {
        Stream::Error stream;
        stream << FNS
               << ": Source chunks wasn't setted for server yet";
        auto response = create_grpc_error_response<Proto::MatchResponse>(
          Proto::Error_Type::Error_Type_NotConfigured,
          stream.str(),
          id_request_grpc);
        return response;
      }
      std::unique_ptr<std::ostringstream> logstr;
      TriggerMatchRes res;
      const Generics::Uuid uid = GrpcAlgs::unpack_user_id(query.uid());
      if (statistic_logger_)
      {
        logstr.reset(new std::ostringstream);
        *logstr << query.request_id() << "::u:" <<  query.urls()
          << "::p:" <<  query.pwords()
          << "::s:" <<  query.swords()
          << "::U:" <<  uid.to_string(false);
      }
      const auto& query_statuses = query.statuses();
      unsigned int match_flags =
        (query.non_strict_word_match() ? MF_NONSTRICTKW : MF_NONE) |
        (query.non_strict_url_match() ? MF_NONSTRICTURL : MF_NONE) |
        (query.return_negative() ? MF_NEGATIVE : MF_NONE) |
        (query_statuses[0] == 'A' ||
         query_statuses[1] == 'A' ? MF_ACTIVE : MF_NONE) |
        (query_statuses[0] == 'I' ||
         query_statuses[1] == 'I' ? MF_INACTIVE : MF_NONE);

      AdServer::ChannelSvcs::MatchWords phrases[CT_MAX];
      AdServer::ChannelSvcs::MatchWords additional_url_keywords;
      MatchUrls url_words, additional_url_words;
      //parsing urls
      const auto& first_url_query = query.first_url();
      const auto& first_url_words_query = query.first_url_words();
      ChannelContainer::match_parse_urls(
        String::SubString(first_url_query.data(), first_url_query.size()),
        String::SubString(first_url_words_query.data(), first_url_words_query.size()),
        ports_,
        query.non_strict_url_match(),
        url_words,
        phrases[CT_URL_KEYWORDS],
        logger(),
        segmentor_);
      const auto& urls_query = query.urls();
      const auto& urls_words_query = query.urls_words();
      ChannelContainer::match_parse_urls(
        String::SubString(urls_query.data(), urls_query.size()),
        String::SubString(urls_words_query.data(), urls_words_query.size()),
        ports_,
        query.non_strict_url_match(),
        additional_url_words,
        additional_url_keywords,
        logger(),
        segmentor_);

      StringVector exact_phrases;
      const auto& swords_query = query.swords();
      const auto& pwords_query = query.pwords();
      parse_keywords(
        String::SubString(swords_query.data(), swords_query.size()),
        phrases[CT_SEARCH],
        match_flags & MF_NONSTRICTKW ? PM_SIMPLIFY : PM_NO_SIMPLIFY,
        match_flags & MF_NONSTRICTKW ? nullptr : &separators,
        match_flags & MF_NONSTRICTKW ? 1 : Commons::DEFAULT_MAX_HARD_WORD_SEQ,
        &exact_phrases,//exact match
        match_flags & MF_NONSTRICTKW ? segmentor_ : 0);
      parse_keywords(
        String::SubString(pwords_query.data(), pwords_query.size()),
        phrases[CT_PAGE],
        (query.simplify_page() || match_flags & MF_NONSTRICTKW) ?
        PM_SIMPLIFY : PM_NO_SIMPLIFY,
        match_flags & MF_NONSTRICTKW ? nullptr : &separators,
        match_flags & MF_NONSTRICTKW ? 1 : Commons::DEFAULT_MAX_HARD_WORD_SEQ,
        0,//exact match
        (query.simplify_page() || match_flags & MF_NONSTRICTKW ?
         segmentor_ : 0));

      phrases[CT_URL_KEYWORDS].insert(
        exact_phrases.begin(), exact_phrases.end());

      container_->match(
        url_words,
        additional_url_words,
        phrases,
        additional_url_keywords,
        exact_phrases,
        uid,
        match_flags,
        res);
      fill_result_(res, *info, query.fill_content());
      timer.stop();
      if (statistic_logger_)
      {
        log_parsed_input_(url_words, phrases, exact_phrases, *logstr);
        log_result_(res, timer.elapsed_time(), *logstr);
        statistic_logger_->log(logstr->str(), Logging::Logger::DEBUG, ASPECT);
      }

      info->set_match_time(GrpcAlgs::pack_time(timer.elapsed_time()));
      __gnu_cxx::__atomic_add(&queries_counter_, 1);

      return response;
    }
    catch (const eh::Exception& exc)
    {
      Stream::Error stream;
      stream << FNS
             << ": "
             << exc.what();
      auto response = create_grpc_error_response<Proto::MatchResponse>(
        Proto::Error_Type::Error_Type_Implementation,
        stream.str(),
        id_request_grpc);
      return response;
    }
    catch (...)
    {
      Stream::Error stream;
      stream << FNS
             << ": Unknown error";
      auto response = create_grpc_error_response<Proto::MatchResponse>(
        Proto::Error_Type::Error_Type_Implementation,
        stream.str(),
        id_request_grpc);
      return response;
    }
  }

  struct comp_string_ptr
  {
    bool operator() (const char* lhs, const char* rhs) const
      noexcept
    {
      return (strcmp(lhs, rhs) < 0);
    }
  };

  void ChannelServerCustomImpl::get_ccg_traits(
    const ::AdServer::ChannelSvcs::ChannelIdSeq& ids,
    ::AdServer::ChannelSvcs::ChannelServer::TraitsResult_out result)
    /*throw(AdServer::ChannelSvcs::ImplementationException,
      AdServer::ChannelSvcs::NotConfigured)*/
  {
    const char* FN = "ChannelServerCustomImpl::get_ccg_traits";
    if(state_ == UpdateData::US_ZERO)
    {
      throw AdServer::ChannelSvcs::NotConfigured(
        "Source chunks wasn't setted for server yet");
    }
    try
    {
      result = new AdServer::ChannelSvcs::ChannelServer::TraitsResult;
      if(ids.length())
      {
        ChannelMatchInfo_var info = container_->get_active();
        ChannelMatchInfo::const_iterator it;
        size_t len[2] = {0, 0};
        for(size_t i = 0; i < ids.length(); i++)
        {
          it = info->find(ids[i]);
          if(it != info->end())
          {
            len[0] += it->second.ccg_keywords.size();
          }
        }
        result->ccg_keywords.length(len[0]);
        result->neg_ccg.length(len[0]);
        len[0] = 0;
        CCGKeywordPacker packer(
          len,
          result->ccg_keywords.get_buffer(),
          result->neg_ccg.get_buffer());
        for(size_t i = 0; i < ids.length(); i++)
        {
          it = info->find(ids[i]);
          if(it != info->end())
          {
            for(std::vector<CCGKeyword_var>::const_iterator it_id =
                it->second.ccg_keywords.begin();
                it_id != it->second.ccg_keywords.end(); ++it_id)
            {
              packer(**it_id);
            }
          }
        }
        result->ccg_keywords.length(len[0]);
        result->neg_ccg.length(len[1]);
      }
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FN << ": caught eh::Exception. : " << e.what();
      CORBACommons::throw_desc<
        ChannelSvcs::ImplementationException>(
          ostr.str());
    }
  }

  bool ChannelServerCustomImpl::change_source_of_data_() noexcept
  {
    WriteGuard_ lock(lock_set_sources_);
    if(new_variant_server_.in())
    {
      variant_server_ = new_variant_server_;
      new_variant_server_.reset();
      return true;
    }
    return false;
  }

  ChannelServerCustomImpl::GetCcgTraitsResponsePtr
  ChannelServerCustomImpl::get_ccg_traits(GetCcgTraitsRequestPtr&& request)
  {
    const auto id_request_grpc = request->id_request_grpc();
    try
    {
      auto response = create_grpc_response<Proto::GetCcgTraitsResponse>(
        id_request_grpc);
      auto* response_info = response->mutable_info();

      if(state_ == UpdateData::US_ZERO)
      {
        Stream::Error stream;
        stream << FNS
               << ": Source chunks wasn't setted for server yet";
        auto response = create_grpc_error_response<Proto::GetCcgTraitsResponse>(
          Proto::Error_Type::Error_Type_NotConfigured,
          stream.str(),
          id_request_grpc);
        return response;
      }

      const auto& ids = request->ids();
      if (!ids.empty())
      {
        ChannelMatchInfo_var info = container_->get_active();
        ChannelMatchInfo::const_iterator it;
        size_t len[2] = {0, 0};
        const std::size_t ids_size = ids.size();
        for (std::size_t i = 0; i < ids_size; ++i)
        {
          it = info->find(ids[i]);
          if(it != info->end())
          {
            len[0] += it->second.ccg_keywords.size();
          }
        }

        auto* ccg_keywords = response_info->mutable_ccg_keywords();
        ccg_keywords->Reserve(len[0]);
        auto* neg_ccg = response_info->mutable_neg_ccg();
        neg_ccg->Reserve(len[0]);

        len[0] = 0;
        CCGKeywordProtoPacker packer(ccg_keywords, neg_ccg);
        for(std::size_t i = 0; i < ids_size; ++i)
        {
          it = info->find(ids[i]);
          if(it != info->end())
          {
            for(std::vector<CCGKeyword_var>::const_iterator it_id = it->second.ccg_keywords.begin();
                it_id != it->second.ccg_keywords.end();
                ++it_id)
            {
              packer(**it_id);
            }
          }
        }
      }

      return response;
    }
    catch (const eh::Exception& exc)
    {
      Stream::Error stream;
      stream << FNS
             << ": "
             << exc.what();
      auto response = create_grpc_error_response<Proto::GetCcgTraitsResponse>(
        Proto::Error_Type::Error_Type_Implementation,
        stream.str(),
        id_request_grpc);
      return response;
    }
    catch (...)
    {
      Stream::Error stream;
      stream << FNS
             << ": Unknown error";
      auto response = create_grpc_error_response<Proto::GetCcgTraitsResponse>(
        Proto::Error_Type::Error_Type_Implementation,
        stream.str(),
        id_request_grpc);
      return response;
    }
  }

  //
  // IDL:AdServer/ChannelSvcs/ChannelServerControl/set_sources:1.0
  //
  void ChannelServerCustomImpl::set_sources(
      const ::AdServer::ChannelSvcs::ChannelServerControl::DBSourceInfo& db_info,
      const ::AdServer::ChannelSvcs::ChunkKeySeq& sources)
      /*throw(AdServer::ChannelSvcs::ImplementationException)*/
  {
    if(!sources.length())
    {
      Stream::Error ostr;
      ostr << __func__ << ": setting empty source";
      CORBACommons::throw_desc<ImplementationException>(ostr.str());
    }
    try
    {
      WriteGuard_ lock(lock_set_sources_);
      DBInfo db_in;
      std::vector<unsigned int> vec_sources;

      db_in.pg_connection = db_info.pg_connection;
      colo_ = db_info.colo;
      version_ = db_info.version;

      vec_sources.reserve(sources.length());

      Stream::Error ostr;
      ostr << __func__ << ": sources:";
      unsigned long i = 0;
      do
      {
        vec_sources.push_back(sources[i]);
        ostr << sources[i] << ",";
      } while(++i < sources.length());

      count_chunks_ = db_info.count_chunks;
      ChannelServerVariantBase::ServerPoolConfig
        pool_config(c_adapter_.in());
      pool_config.timeout = Generics::Time(
        std::min(update_period_, 10UL)); // 10 sec
      unpack_ref_list_(db_info.campaign_refs, pool_config, "campaign server");

      new_variant_server_ = new ChannelServerDB(
        db_in,
        ports_,
        vec_sources,
        count_chunks_,
        db_info.colo,
        db_info.version.in(),
        pool_config,
        SERVICE_INDEX_,
        segmentors_,
        logger(),
        db_info.check_sum);

      configuration_date_ = Generics::Time::get_time_of_day();
      if(state_ == UpdateData::US_ZERO)
      {
        state_ = UpdateData::US_START;
      }

      logger()->log(ostr.str(),
        Logging::Logger::TRACE,
        ASPECT);
    }
    catch(const ChannelServerException::Exception& e)
    {
      Stream::Error ostr;
      ostr << __func__ << ": ChannelServerException::Exception: " << e.what();
      logger()->log(
        ostr.str(), Logging::Logger::CRITICAL, ASPECT, "ADS-IMPL-40");
      CORBACommons::throw_desc<ImplementationException>(ostr.str());
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << __func__ << ": eh::Exception: " << e.what();
      logger()->log(
        ostr.str(), Logging::Logger::CRITICAL, ASPECT, "ADS-IMPL-40");
      CORBACommons::throw_desc<ImplementationException>(ostr.str());
    }
  }

  //
  // IDL:AdServer/ChannelSvcs/ChannelServerControl/set_proxy_sources:1.0
  //
  void ChannelServerCustomImpl::set_proxy_sources(
    const AdServer::ChannelSvcs::ChannelServerControl::
      ProxySourceInfo& proxy_info,
    const ::AdServer::ChannelSvcs::ChunkKeySeq& sources)
  {
    if(!sources.length())
    {
      Stream::Error ostr;
      ostr << __func__ << ": setting empty source";
      CORBACommons::throw_desc<ImplementationException>(ostr.str());
    }
    try
    {
      WriteGuard_ lock(lock_set_sources_);
      std::vector<unsigned int> vec_sources;
      vec_sources.reserve(sources.length());

      Stream::Error ostr;
      ostr << __func__ << ": sources:";
      unsigned long i = 0;
      do
      {
        vec_sources.push_back(sources[i]);
        ostr << sources[i] << ",";
      } while(++i < sources.length());

      ChannelServerVariantBase::ServerPoolConfig
        pool_config(c_adapter_.in());
      pool_config.timeout = Generics::Time(
        std::min(update_period_, 10UL)); // 10 sec
      unpack_ref_list_(
        proxy_info.campaign_refs, pool_config, "campaign server");

      ChannelServerVariantBase::ServerPoolConfig
        proxy_pool_config(c_adapter_.in());
      proxy_pool_config.timeout = Generics::Time(
        std::min(update_period_, 10UL)); // 10 sec
      unpack_ref_list_(
        proxy_info.proxy_refs, proxy_pool_config, "channel proxy");

      colo_ = proxy_info.colo;
      version_ = proxy_info.version;
      if(logger()->log_level() >= Logging::Logger::TRACE &&
         proxy_info.local_descriptor.length())
      {
        Stream::Error mes;
        mes << "Local references: ";
        describe_description(mes, proxy_info.local_descriptor);
        logger()->log(mes.str(), Logging::Logger::TRACE, ASPECT);
      }

      new_variant_server_ = new ChannelServerProxy(
        proxy_info.local_descriptor,
        proxy_pool_config,
        ports_,
        vec_sources,
        proxy_info.count_chunks,
        proxy_info.colo,
        proxy_info.version.in(),
        pool_config,
        SERVICE_INDEX_,
        logger(),
        proxy_info.check_sum);

      configuration_date_ = Generics::Time::get_time_of_day();
      if(state_ == UpdateData::US_ZERO)
      {
        state_ = UpdateData::US_START;
      }

      logger()->log(ostr.str(),
          Logging::Logger::TRACE,
        ASPECT);
    }
    catch(const CORBA::SystemException& e)
    {
      Stream::Error ostr;
      ostr << __func__ << ": CORBA::SystemException: " << e;
      logger()->log(
        ostr.str(), Logging::Logger::CRITICAL, ASPECT, "ADS-IMPL-40");
      CORBACommons::throw_desc<ImplementationException>(ostr.str());
    }
    catch(const ChannelServerException::Exception& e)
    {
      Stream::Error ostr;
      ostr << __func__ << ": ChannelServerException::Exception: " << e.what();
      logger()->log(
        ostr.str(), Logging::Logger::CRITICAL, ASPECT, "ADS-IMPL-40");
      CORBACommons::throw_desc<ImplementationException>(ostr.str());
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << __func__ << ": eh::Exception: " << e.what();
      logger()->log(
        ostr.str(), Logging::Logger::CRITICAL, ASPECT, "ADS-IMPL-40");
      CORBACommons::throw_desc<ImplementationException>(ostr.str());
    }
  }

  void ChannelServerCustomImpl::unpack_ref_list_(
    const ::AdServer::ChannelSvcs::
      ChannelServerControl::CorbaObjectRefDefSeq& corba_refs,
    ChannelServerVariantBase::ServerPoolConfig& pool_config,
    const char* name)
    /*throw(eh::Exception, CORBACommons::CorbaObjectRef::Exception)*/
  {
    if(corba_refs.length())
    {
      CORBACommons::CorbaObjectRef ref;
      Stream::Error mes;
      mes << "References to " << name << ": ";
      for(size_t i = 0; i < corba_refs.length(); i++)
      {
        ref.load(corba_refs[i]);
        if(i != 0)
        {
          mes << "; ";
        }
        mes << ref.object_ref;
        pool_config.iors_list.push_back(ref);
      }
      logger()->log(mes.str(), Logging::Logger::TRACE, ASPECT);
    }
  }

  //
  // IDL:AdServer/ChannelSvcs/ChannelServerControl/get_queries_counter:1.0
  //
  ::CORBA::ULong ChannelServerCustomImpl::check_configuration()
  noexcept
  {
    ReadGuard_ lock(lock_set_sources_);
    if(new_variant_server_) 
    {
      return new_variant_server_->get_check_sum();
    }
    else if(variant_server_)
    {
      return variant_server_->get_check_sum();
    }
    else
    {
      return 0;
    }
  }

  char* ChannelServerCustomImpl::comment() /*throw(CORBACommons::OutOfMemory)*/
  {
    const char* FUN = "ChannelServerCustomImpl::comment";
    try
    {
      Stream::Error message;
      if(state_ ==  UpdateData::US_ZERO)
      {
        message << "configuring,";
      }
      if(state_ <=  UpdateData::US_START)
      {
        message << "listing channels,";
      }
      if(state_ != UpdateData::US_FINISH)
      {
        if(state_ <= UpdateData::US_CHECKED)
        {
          message << "keywords " << load_keywords_ << ',';
        }
        if (state_ >= UpdateData::US_CHECKED)
        {
          message << (progress_->get_stage() == PROGRESS_LOAD ?
                      "loading" : "merging") << " channels "
            << progress_->get_sum_progress() << '/' << progress_->get_total();
        }
        else
        {
          message << "unknown";
        }
      }
      else
      {
          message << "all loaded";
      }
      if (logger()->log_level() >= Logging::Logger::DEBUG)
      {
        logger()->sstream(Logging::Logger::DEBUG, ASPECT)
          << "Current progress: " << message.str();
      }
      if(ready_)
      {
        return CORBA::string_dup("ready");
      }
      CORBA::String_var str;
      str << message.str();
      return str._retn();
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught  eh::Exception. :"
        << e.what();
      logger()->log(
        ostr.str(),
        Logging::Logger::ERROR,
        ASPECT,
        "ADS-IMPL-44");
      throw CORBACommons::OutOfMemory();
    }
  }

  void ChannelServerCustomImpl::get_stats(ChannelServerStats& stats) noexcept
  {
    container_->get_stats(stats);
    if(state_ !=  UpdateData::US_ZERO)
    {
      ChannelServerVariantBase_var variant_server = get_source_of_data_();
      std::ostringstream chunks_str;
      unsigned int count = 0;
      const std::vector<unsigned int>& sources =
        variant_server->get_sources(count);
      chunks_str << version_ << "; ";
      for(std::vector<unsigned int>::const_iterator it = sources.begin();
          it != sources.end(); ++it)
      {
        if(it != sources.begin())
        {
          chunks_str << ',';
        }
        chunks_str << *it;
      }
      chunks_str << "/" << count;
      chunks_str << "; sum " << variant_server->get_check_sum();
      stats.configuration = chunks_str.str();
      stats.configuration_date = configuration_date_;
    }
  }

  void ChannelServerCustomImpl::change_db_state(bool new_state)
    /*throw(eh::Exception)*/
  {
    if(state_ == UpdateData::US_ZERO)
    {
      throw Exception("Not ready");
    }
    ChannelServerVariantBase_var variant_server = get_source_of_data_();
    variant_server->change_db_state(new_state);
  }

  bool ChannelServerCustomImpl::get_db_state()
    /*throw(Commons::DbStateChanger::NotSupported)*/
  {
    if(state_ != UpdateData::US_ZERO)
    {
      ChannelServerVariantBase_var variant_server = get_source_of_data_();
      try
      {
        return variant_server->get_db_state();
      }
      catch(const ChannelServerVariantBase::NotSupported& e)
      {
        throw Commons::DbStateChanger::NotSupported("");
      }
    }
    return false;
  }

  size_t ChannelServerCustomImpl::add_channels_(
    unsigned int channel_id,
    const TriggerMatchItem::value_type& in,
    AdServer::ChannelSvcs::ChannelServerBase::ChannelAtom* out)
    noexcept
  {
    for(size_t i = 0; i < in.size(); ++i, ++out)
    {
      out->id = channel_id;
      out->trigger_channel_id = in[i];
    } 
    return in.size();
  }

  void ChannelServerCustomImpl::add_channels_(
    unsigned int channel_id,
    const TriggerMatchItem::value_type& in,
    google::protobuf::RepeatedPtrField<Proto::ChannelAtom>* out) noexcept
  {
    for (size_t i = 0; i < in.size(); ++i)
    {
      auto* channel_atom = out->Add();
      channel_atom->set_id(channel_id);
      channel_atom->set_trigger_channel_id(in[i]);
    }
  }

  void ChannelServerCustomImpl::log_parsed_input_(
    const MatchUrls& url_words,
    const MatchWords match_words[CT_MAX],
    const StringVector& exact_words,
    std::ostream& ostr)
    /*throw(eh::Exception)*/
  {
    ostr << "::pu:";
    for(MatchUrls::const_iterator it = url_words.begin();
        it != url_words.end(); ++it)
    {
      if(it != url_words.begin())
      {
        ostr << ',';
      }
      ostr << it->prefix << it->postfix;
    }
    ostr << "::pp:";
    for(MatchWords::const_iterator it = match_words[CT_PAGE].begin();
       it != match_words[CT_PAGE].end(); ++it)
    {
      if(it != match_words[CT_PAGE].begin())
      {
        ostr << ',';
      }
      ostr << it->text();
    } 
    ostr << "::ps:";
    for(MatchWords::const_iterator it = match_words[CT_SEARCH].begin();
       it != match_words[CT_SEARCH].end(); ++it)
    {
      if(it != match_words[CT_SEARCH].begin())
      {
        ostr << ',';
      }
      ostr << it->text();
    } 
    ostr << "::pr:";
    for(MatchWords::const_iterator it = match_words[CT_URL_KEYWORDS].begin();
       it != match_words[CT_URL_KEYWORDS].end(); ++it)
    {
      if(it != match_words[CT_URL_KEYWORDS].begin())
      {
        ostr << ',';
      }
      ostr << it->text();
    } 
    trace_sequence("::pe:", exact_words, ostr);
  }

  void ChannelServerCustomImpl::fill_result_(
    const TriggerMatchRes& result,
    AdServer::ChannelSvcs::ChannelServer::MatchResult& res,
    bool fill_content)
    noexcept
  {
    TriggerMatchRes::const_iterator it = result.begin();
    if(it != result.end() && it->first == c_special_adv)
    {
      res.no_adv = true;
      ++it;
    }
    else
    {
      res.no_adv = false;
    }
    if(it != result.end() && it->first == c_special_track)
    {
      res.no_track = true;
      ++it;
    }
    else
    {
      res.no_track = false;
    }
    if(it != result.end())
    {
      CORBA::ULong index[CT_MAX + 1] = {0, 0, 0, 0, 0};
      CORBA::ULong count_content_channels = 0;
      ::AdServer::ChannelSvcs::ChannelServerBase::TriggerMatchResult& out =
        res.matched_channels;
      out.page_channels.length(result.count_channels[CT_PAGE]);
      out.search_channels.length(result.count_channels[CT_SEARCH]);
      out.url_channels.length(result.count_channels[CT_URL]);
      out.url_keyword_channels.length(result.count_channels[CT_URL_KEYWORDS]);
      out.uid_channels.length(result.count_channels[CT_MAX]);
      if(fill_content)
      {
        res.content_channels.length(
          result.count_channels[CT_PAGE] + result.count_channels[CT_SEARCH] +
          result.count_channels[CT_URL] + result.count_channels[CT_URL_KEYWORDS]); 
      }
      do
      {
        const TriggerMatchItem& item = it->second;
        if(item.flags & TriggerMatchItem::TMI_UID)
        {
          out.uid_channels[index[CT_MAX]++] = it->first;
        }
        else if(!(item.flags & TriggerMatchItem::TMI_NEGATIVE))
        {
          index[CT_PAGE] += add_channels_(
            it->first,
            item.trigger_ids[CT_PAGE],
            out.page_channels.get_buffer() + index[CT_PAGE]);
          index[CT_SEARCH] += add_channels_(
            it->first,
            item.trigger_ids[CT_SEARCH],
            out.search_channels.get_buffer() + index[CT_SEARCH]);
          index[CT_URL] += add_channels_(
            it->first,
            item.trigger_ids[CT_URL],
            out.url_channels.get_buffer() + index[CT_URL]);
          index[CT_URL_KEYWORDS] += add_channels_(
            it->first,
            item.trigger_ids[CT_URL_KEYWORDS],
            out.url_keyword_channels.get_buffer() + index[CT_URL_KEYWORDS]);
          if(item.weight && fill_content)
          {
            res.content_channels[count_content_channels].id = it->first;
            res.content_channels[count_content_channels++].weight = item.weight;
          }
        }
      } while(++it != result.end());
      out.page_channels.length(index[CT_PAGE]);
      out.search_channels.length(index[CT_SEARCH]);
      out.url_channels.length(index[CT_URL]);
      out.url_keyword_channels.length(index[CT_URL_KEYWORDS]);
      out.uid_channels.length(index[CT_MAX]);
      res.content_channels.length(count_content_channels);
    }
  }

  void ChannelServerCustomImpl::fill_result_(
    const TriggerMatchRes& result,
    AdServer::ChannelSvcs::Proto::MatchResponseInfo& res,
    bool fill_content)
  {
    TriggerMatchRes::const_iterator it = result.begin();
    if(it != result.end() && it->first == c_special_adv)
    {
      res.set_no_adv(true);
      ++it;
    }
    else
    {
      res.set_no_adv(false);
    }
    if(it != result.end() && it->first == c_special_track)
    {
      res.set_no_track(true);
      ++it;
    }
    else
    {
      res.set_no_track(false);
    }
    if(it != result.end())
    {
      auto& out = *res.mutable_matched_channels();

      const std::size_t count_ct_page = result.count_channels[CT_PAGE];
      auto* page_channels = out.mutable_page_channels();
      page_channels->Reserve(count_ct_page);

      const std::size_t count_ct_search = result.count_channels[CT_SEARCH];
      auto* search_channels = out.mutable_search_channels();
      search_channels->Reserve(count_ct_search);

      const std::size_t count_ct_url = result.count_channels[CT_URL];
      auto* url_channels = out.mutable_url_channels();
      url_channels->Reserve(count_ct_url);

      const std::size_t count_url_keywords = result.count_channels[CT_URL_KEYWORDS];
      auto* url_keyword_channels = out.mutable_url_keyword_channels();
      url_keyword_channels->Reserve(count_url_keywords);

      auto* uid_channels = out.mutable_uid_channels();
      uid_channels->Reserve(result.count_channels[CT_MAX]);

      auto* content_channels = res.mutable_content_channels();
      if(fill_content)
      {
        content_channels->Reserve(
          count_ct_page + count_ct_search +
          count_ct_url + count_url_keywords);
      }

      do
      {
        const TriggerMatchItem& item = it->second;
        if(item.flags & TriggerMatchItem::TMI_UID)
        {
          uid_channels->Add(it->first);
        }
        else if(!(item.flags & TriggerMatchItem::TMI_NEGATIVE))
        {
          add_channels_(
            it->first,
            item.trigger_ids[CT_PAGE],
            page_channels);
          add_channels_(
            it->first,
            item.trigger_ids[CT_SEARCH],
            search_channels);
          add_channels_(
            it->first,
            item.trigger_ids[CT_URL],
            url_channels);
          add_channels_(
            it->first,
            item.trigger_ids[CT_URL_KEYWORDS],
            url_keyword_channels);
          if(item.weight && fill_content)
          {
            auto* content_channel_atom = content_channels->Add();
            content_channel_atom->set_id(it->first);
            content_channel_atom->set_weight(item.weight);
          }
        }
      } while(++it != result.end());
    }
  }

  void ChannelServerCustomImpl::log_result_(
    const TriggerMatchRes& res,
    const Generics::Time& time,
    std::ostream& ostr)
    /*throw(eh::Exception)*/
  {
    const char types[CT_MAX] = {'U', 'P', 'S', 'R'};
    ostr << "::t:" << time;
    for(auto it = res.begin(); it != res.end(); ++it)
    {
      const TriggerMatchItem& item = it->second;
      for(size_t type = CT_URL; type < CT_MAX; type++)
      {
        if (!item.trigger_ids[type].empty())
        {
          ostr << "::" << types[type] << it->first << ":";
          trace_sequence(":", item.trigger_ids[type], ostr);
        }
      }
    }
    ostr << '.';
  }

  void ChannelServerCustomImpl::UpdateTask::execute() noexcept
  {
    server_impl_->update_task(update_data_);
  }

  void ChannelServerCustomImpl::deactivate_object()
    /*throw(Exception, eh::Exception)*/
  {
    container_->terminate();
    Generics::CompositeActiveObject::deactivate_object();
  }

  template<class MAPIN, class CORBASEQ>
  ChannelServerCustomImpl::DeletedPacker<MAPIN, CORBASEQ>::DeletedPacker(
    const MAPIN& deleted,
    CORBASEQ& out)
    /*throw(eh::Exception)*/
    : out_(out)
  {
    std::size_t len = std::distance(deleted.begin(), deleted.end());
    out_.length(len);
    std::transform(deleted.begin(), deleted.end(), out_.get_buffer(), *this);
  }

  template<class MAPIN, class CORBASEQ>
  typename ChannelServerCustomImpl::DeletedPacker<MAPIN, CORBASEQ>::result_type
  ChannelServerCustomImpl::DeletedPacker<MAPIN, CORBASEQ>::operator()(
    const typename ChannelServerCustomImpl::
    DeletedPacker<MAPIN, CORBASEQ>::argument_type& in)
    /*throw(eh::Exception)*/
  {
    typename
    ChannelServerCustomImpl::DeletedPacker<MAPIN, CORBASEQ>::result_type value;
    value.id = in.first;
    value.stamp = CorbaAlgs::pack_time(in.second);
    return value;
  }

  ChannelServerCustomImpl::CCGKeywordPacker::CCGKeywordPacker(
    size_t count[2],
    ChannelServerBase::CCGKeyword* ccg_out,
    CORBA::ULong* neg_out)
    /*throw(eh::Exception)*/
    : count_(count),
      out_(ccg_out),
      neg_out_(neg_out)
  {
  }

  void ChannelServerCustomImpl::CCGKeywordPacker::operator()(
    const ChannelServerCustomImpl::CCGKeywordPacker::argument_type& id) noexcept
  {
    if(neg_out_ &&
       !id.original_keyword.empty() && id.original_keyword[0] == '-')
    {
      neg_out_[count_[1]++] = id.ccg_id;
    }
    else
    {
      out_[count_[0]++] = convert<ChannelServerBase::CCGKeyword>(id);
    }
  }

  template<class RESULT>
  RESULT
  ChannelServerCustomImpl::CCGKeywordPacker::convert(const CCGKeyword& in)
    noexcept
  {
    RESULT value;
    value.ccg_keyword_id = in.ccg_keyword_id;
    value.ccg_id = in.ccg_id;
    value.channel_id = in.channel_id;
    value.max_cpc =
      CorbaAlgs::pack_decimal<CampaignSvcs::RevenueDecimal>(in.max_cpc);
    value.ctr = CorbaAlgs::pack_decimal<CampaignSvcs::CTRDecimal>(in.ctr);
    value.click_url << in.click_url;
    value.original_keyword << in.original_keyword;
    return value;
  }

  ChannelServerCustomImpl::CCGKeywordProtoPacker::CCGKeywordProtoPacker(
    google::protobuf::RepeatedPtrField<Proto::CCGKeyword>* ccg_out,
    google::protobuf::RepeatedField<uint32_t>* neg_out)
    : out_(ccg_out),
      neg_out_(neg_out)
  {
  }

  void ChannelServerCustomImpl::CCGKeywordProtoPacker::operator()(
    const CCGKeyword& id)
  {
    if(neg_out_ &&
      !id.original_keyword.empty() && id.original_keyword[0] == '-')
    {
      neg_out_->Add(id.ccg_id);
    }
    else
    {
      auto* ccgkeyword = out_->Add();
      ccgkeyword->set_ccg_keyword_id(id.ccg_keyword_id);
      ccgkeyword->set_ccg_id(id.ccg_id);
      ccgkeyword->set_channel_id(id.channel_id);
      ccgkeyword->set_max_cpc(
        GrpcAlgs::pack_decimal<CampaignSvcs::RevenueDecimal>(id.max_cpc));
      ccgkeyword->set_ctr(GrpcAlgs::pack_decimal<CampaignSvcs::CTRDecimal>(id.ctr));
      ccgkeyword->set_click_url(id.click_url);
      ccgkeyword->set_original_keyword(id.original_keyword);
    }
  }


} /* ChannelSvcs */
} /* AdServer */
