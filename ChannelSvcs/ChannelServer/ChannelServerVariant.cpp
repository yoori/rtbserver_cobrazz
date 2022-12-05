
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <set>
#include <algorithm>

#include <eh/Exception.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <Generics/ArrayAutoPtr.hpp>
#include <Commons/CorbaAlgs.hpp>
#include <Commons/Postgres/ResultSet.hpp>
#include<Commons/Postgres/Lob.hpp>
#include <ChannelSvcs/ChannelCommons/CommonTypes.hpp>
#include <ChannelSvcs/ChannelCommons/TriggerParser.hpp>
#include <ChannelSvcs/ChannelCommons/ChannelUtils.hpp>
#include <ChannelSvcs/ChannelCommons/ChannelUpdateBase.hpp>
#include <CampaignSvcs/CampaignServer/CampaignServer.hpp>
#include <CORBACommons/CorbaAdapters.hpp>

#include "ChannelServerVariant.hpp"
#include "ChannelServerImpl.hpp"

namespace AdServer
{
namespace ChannelSvcs
{
  const char* ChannelServerVariantBase::ASPECT = "ChannelServerVariantBase";

  typedef ::AdServer::ChannelSvcs::ChannelUpdateBase_v33 ChannelCurrent;

  ChannelServerVariantBase::ChannelServerVariantBase(
    const std::vector<unsigned int>& sources,
    const std::set<unsigned short>& ports,
    unsigned long count_chunks,
    unsigned long colo,
    const char* version,
    const ServerPoolConfig& campaign_pool_config,
    unsigned service_index,
    Logging::Logger* logger,
    unsigned long check_sum)
    /*throw(ChannelServerException::Exception, ChannelContainer::Exception)*/
    : ports_(ports),
      count_chunks_(count_chunks),
      source_id_(-1),
      colo_(colo),
      version_(version),
      logger_(ReferenceCounting::add_ref(logger)),
      check_sum_(check_sum),
      service_index_(service_index)
  {
    try
    {
      first_load_stamp_ = Generics::Time::get_time_of_day();
      sources_ = sources;
      std::sort(sources_.begin(), sources_.end());
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << __func__ << ": eh:Exception: " << e.what();
      throw ChannelServerException::Exception(ostr);
    }
    if(campaign_pool_config.iors_list.empty())
    {
      Stream::Error ostr;
      ostr << __func__ << ": empty list of CampaignServer references";
      throw ChannelServerException::Exception(ostr);
    }
    //create campaign pool
    try
    {
      campaign_pool_ =
        CampaignServerPoolPtr(new CampaignServerPool(
          campaign_pool_config, CORBACommons::ChoosePolicyType::PT_PERSISTENT));

    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << __func__ << ": eh:Exception: "
        << e.what();
      throw ChannelServerException::Exception(ostr);
    }
  }

  void ChannelServerVariantBase::change_db_state(bool /*activation*/)
    /*throw(ChannelServerException::Exception)*/
  {
    throw ChannelServerException::Exception("Not supported");
  }

  bool ChannelServerVariantBase::get_db_state()
    /*throw(NotSupported)*/
  {
    throw NotSupported("");
  }

  const std::vector<unsigned int>& ChannelServerVariantBase::get_sources(
    unsigned int& count)
    noexcept
  {
    count = count_chunks_;
    return sources_;
  }

  ChannelServerDB::ChannelServerDB(
    const DBInfo& db_info,
    const std::set<unsigned short>& ports,
    const std::vector<unsigned int>& sources,
    unsigned long count_chunks,
    unsigned long colo,
    const char* version,
    const ServerPoolConfig& campaign_pool_config,
    unsigned service_index,
    const SegmMap& segmentors,
    Logging::Logger* logger,
    unsigned long check_sum)
    /*throw(ChannelServerException::Exception)*/:
      ChannelServerVariantBase(
        sources,
        ports,
        count_chunks,
        colo,
        version,
        campaign_pool_config,
        service_index,
        logger,
        check_sum),
      callback_(
        new Logging::ActiveObjectCallbackImpl(logger)),
      segmentors_(segmentors),
      db_info_(db_info),
      update_size_(1024)
  {
    try
    {
      pg_env_ =
        new Commons::Postgres::Environment(db_info.pg_connection.c_str());
      pg_pool_ = pg_env_->create_connection_pool();

      add_child_object(pg_env_);
      activate_object();
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << __func__ << ": "
        "can't postgress environment: " << e.what();
      throw ChannelServerException::Exception(ostr);
    }
  }

  ChannelServerDB::~ChannelServerDB() noexcept
  {
    try
    {
      change_db_state(false);
    }
    catch(...)
    {
    }
  }

  void ChannelServerVariantBase::add_special_(
    unsigned long id,
    ChannelIdToMatchInfo& match_info)
    /*throw(eh::Exception)*/
  {
    if(is_my_id_(id))
    {
      MatchInfo& info = match_info[id];
      Channel& ch = info.channel;
      ch.id = id;
      ch.mark_type(CT_URL);
      ch.mark_type(CT_PAGE);
      ch.mark_type(CT_SEARCH);
      ch.mark_type(CT_URL_KEYWORDS);
      ch.mark_type(Channel::CT_ACTIVE);
      ch.mark_type(Channel::CT_BLACK_LIST);
      //mark channel as blacklist for escaping matching on additional urls 
    }
  }

  void ChannelServerVariantBase::update_actual_channels(
    UpdateData* update_data)
  {
    const char* FUN = "ChannelServerVariantBase::update_actual_channels";
    try
    {
      if(sources_.empty())
      {
        throw ChannelServerException::NotReady("Empty sources");
      }

      const CORBA::ULong COUNT_PORTIONS = 20;
      AdServer::CampaignSvcs::ChannelServerChannelAnswer_var res;
      CORBA::ULong portion = 0;
      AdServer::CampaignSvcs::CampaignServer::GetSimpleChannelsInfo settings;
      settings.portions_number = COUNT_PORTIONS;
      settings.channel_statuses << String::SubString("AIW");
      ChannelIdToMatchInfo_var info_ptr = new ChannelIdToMatchInfo;
      for (;;)
      {
        CampaignServerPool::ObjectHandlerType campaign_server =
          campaign_pool_->get_object<CampaignServerPool::Exception>(
            logger_,
            Logging::Logger::CRITICAL,
            ASPECT,
            "ADS_ICON-1",
            service_index_,
            service_index_);

        try
        {
          for(; portion < COUNT_PORTIONS; ++portion)
          {
            settings.portion = portion;
            res = campaign_server->chsv_simple_channels(settings);
            process_portion_(res->simple_channels, *info_ptr);
          }
          break;
        }
        catch(
          const AdServer::CampaignSvcs::CampaignServer::ImplementationException& e)
        {
          Stream::Error ostr;
          ostr << FUN << ": CampaignServer::ImplementationException: "
            << e.description;
          campaign_server.release_bad(ostr.str());
          logger_->log(
            ostr.str(),
            Logging::Logger::CRITICAL,
            ASPECT);
        }
        catch(const AdServer::CampaignSvcs::CampaignServer::NotReady& e)
        {
          campaign_server.release_bad(
            String::SubString("CampaignServer is not ready"));
        }
        catch (const CORBA::SystemException& ex)
        {
          Stream::Error ostr;
          ostr << FUN << ": caught CORBA::SystemException at get_channels(): "
            << ex;
          campaign_server.release_bad(ostr.str());
          logger_->log(
            ostr.str(),
            Logging::Logger::CRITICAL,
            ASPECT,
            "ADS-ICON-1");
        }
      }

      update_data->cost_limit =
        CorbaAlgs::unpack_decimal<CampaignSvcs::RevenueDecimal>(
          res->cost_limit);
      add_special_(c_special_track, *info_ptr);
      add_special_(c_special_adv, *info_ptr);
      update_data->info_ptr = info_ptr;
    }
    catch (const ChannelServerException::Exception&)
    {
      throw;
    }
    catch(const eh::Exception& e)
    {
      Stream::Error err;
      err << FUN << ": eh::Exception. : " << e.what();
      throw ChannelServerException::Exception(err);
    }
  }

  void ChannelServerVariantBase::process_portion_(
    const AdServer::CampaignSvcs::CSSimpleChannelSeq& channels,
    ChannelIdToMatchInfo& info)
    /*throw(eh::Exception)*/
  {
    for(CORBA::ULong i = 0; i < channels.length(); ++i)
    {
      if ((channels[i].channel_type == 'A' && channels[i].status != 'A') ||
          channels[i].channel_type == 'W' ||
          !is_my_id_(channels[i].channel_id))
      {//skip inactive uid channels, bad channels, not from my source channels
        continue;
      }
      MatchInfo& m_info = info[channels[i].channel_id];
      m_info.country = channels[i].country_code;
      m_info.lang = channels[i].language;
      m_info.channel_size = 0;
      Channel& ch = m_info.channel;
      ch.id = channels[i].channel_id;
      ch.set_status(channels[i].status);
      if (channels[i].channel_type == 'P')
      {
        ch.mark_type(Channel::CT_BLACK_LIST);
      }
      for(CORBA::ULong j = 0; j < channels[i].behave_info.length(); ++j)
      {
        size_t index_type;
        bool ignore = false;
        switch(channels[i].behave_info[j].trigger_type) 
        {
          case 'U':
          case 'u':
            index_type = CT_URL;
            break;
          case 'P':
          case 'p':
            index_type = CT_PAGE;
            break;
          case 'S':
          case 's':
            index_type = CT_SEARCH;
            break;
          case 'R':
          case 'r':
            index_type = CT_URL_KEYWORDS;
            break;
          case 'A':
          case 'a':
            index_type = Channel::CT_UIDS;
            break;
          default:
            ignore = true;
            break;
        }
        if (ignore)
        {
          break;
        }
        ch.mark_type(index_type);
        if (channels[i].behave_info[j].is_context && index_type < CT_MAX)
        {
          ch.set_weight(index_type, channels[i].behave_info[j].weight);
        }
      }
    }
  }

  struct FillChannelIdAdapter
  {
    FillChannelIdAdapter(std::vector<unsigned int>& out) noexcept : out_(out){};
    void operator()(
      const MatchInfoContainerType::value_type& in) 
      noexcept
    {
      out_.push_back(in.second.channel.get_id());
    }
    std::vector<unsigned int>& out_;
  };

  void ChannelServerDB::check_id_(
    unsigned int id,
    const Generics::Time& new_db_stamp,
    const Generics::Time& new_master,
    const ExcludeContainerType& updated,
    UpdateData::CheckContainerType& check_ids,
    UpdateData::CheckContainerType& uid_check_ids,
    ChannelIdToMatchInfo& info)
    /*throw(ChannelServerException::Exception)*/
  {
    ChannelIdToMatchInfo::iterator info_it;
    info_it = info.find(id);
    if(info_it == info.end())
    {
      Stream::Error err;
      err << "unexpected channel id = " << id;
      throw ChannelServerException::Exception(err);
    }
    else if(updated.find(id) == updated.end() &&
            info_it->second.db_stamp == new_db_stamp)
    {
      return;
    }
    info_it->second.db_stamp = new_db_stamp;
    info_it->second.stamp = new_master;
    if(!info_it->second.channel.match_mask(Channel::CH_UIDS))
    {
      check_ids.insert(check_ids.end(), id);
    }
    else if(info_it->second.channel.match_mask(Channel::CH_ACTIVE))
    {
      uid_check_ids.insert(uid_check_ids.end(), id);
    }
  }

  void ChannelServerDB::check_updating(
    UpdateData* data)//update data
    /*throw(ChannelServerException::Exception,
      ChannelServerException::TemporyUnavailable)*/
  {
    if(!data->container_ptr->ready())
    {
      Stream::Error err;
      err << __func__ << ": container not ready to parse";
      throw ChannelServerException::TemporyUnavailable(err);
    }
    Commons::Postgres::Connection_var conn;
    try
    {
      conn = pg_pool_->get_connection();

      size_t update_size = data->info_ptr->size();
      if(!update_size)
      {
        return;
      }

      std::vector<unsigned int> channel_ids;

      Commons::Postgres::Statement_var stmt =
        new Commons::Postgres::Statement(
        "SELECT "
          "channel_id,"
          "triggers_version "
        "FROM adserver.get_trigger_versions($1)");

      channel_ids.reserve(update_size);

      std::for_each(
        data->info_ptr->begin(),
        data->info_ptr->end(),
        FillChannelIdAdapter(channel_ids));

      stmt->set_array(1, channel_ids);

      Commons::Postgres::ResultSet_var rs = conn->execute_statement(stmt);

      enum
      {
        FP_CHANNEL_ID = 1,
        FP_VERSION
      };

      UpdateData::CheckContainerType& check_ids = data->check_data;
      UpdateData::CheckContainerType& uid_check_ids = data->uid_check_data;

      check_ids.clear();
      uid_check_ids.clear();
      if(rs->next() && rs->get_number<long>(FP_CHANNEL_ID) == 0)
      {
        data->new_master = rs->get_timestamp(FP_VERSION);
        unsigned long id;
        bool have_special_adv = false;
        bool have_special_track = false;
        Generics::Time new_db_stamp;
        const ExcludeContainerType& updated =
          data->container_ptr->get_updated();
        while(rs->next())
        {
          id = rs->get_number<long>(FP_CHANNEL_ID);
          if(id == c_special_adv)
          {
            have_special_adv = true;
          }
          if(id == c_special_track)
          {
            have_special_track = true;
          }
          new_db_stamp = rs->get_timestamp(FP_VERSION);
          check_id_(
            id,
            new_db_stamp,
            data->new_master,
            updated,
            check_ids,
            uid_check_ids,
            *data->info_ptr);
        }
        if(!have_special_adv)
        {
          data->info_ptr->erase(c_special_adv);
        }
        if(!have_special_track)
        {
          data->info_ptr->erase(c_special_track);
        }
        /*
        if(logger_->log_level() >= Logging::Logger::TRACE)
        {
          trace_sequence(
            "New channels", update_data->container_ptr->get_new(), ostr);
        }*/
      }
      else
      {
        Stream::Error err;
        err << __func__ << ": can't get sysdate from database.";
        throw ChannelServerException::Exception(err);
      }
    }
    catch(const Commons::Postgres::NotActive& e)
    {//it isn't error
      throw ChannelServerException::TemporyUnavailable(0);
    }
    catch(const Commons::Postgres::Exception& ex)
    {
      pg_pool_->bad_connection(conn);
      Stream::Error ostr;
      ostr << __func__ << ": Postgres::Exception : " << ex.what();
      logger_->log(
          ostr.str(),
          Logging::Logger::CRITICAL,
          ASPECT,
          "ADS-DB-0");
      throw ChannelServerException::Exception(ostr);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << __func__ << ": eh::Exception. : " << ex.what();
      throw ChannelServerException::Exception(ostr);
    }
  }

  size_t ChannelServerDB::get_update_size_(unsigned long merge_limit) const
    noexcept
  {
    if(update_history_.empty())
    {
      return update_size_;
    }
    unsigned long channel_size = update_history_.back();
    unsigned long count = update_history_.size();
    std::list<unsigned long>::const_reverse_iterator it =
      update_history_.rbegin();
    size_t max_size =  *it;
    ++it;//skip last size
    for(unsigned long i = 1; i < count; i++, ++it)
    {
      max_size = std::max(*it, max_size);
      if(*it < channel_size)
      {
        channel_size -= (channel_size - *it) * (count - i) / count;
      }
      else
      {
        channel_size += (*it - channel_size) * (count - i) / count;
      }
    }
    if(max_size / 3 > channel_size)
    {
      channel_size = max_size / 3;
    }
    return channel_size ? merge_limit / channel_size + 1 : 1;
  }

  void ChannelServerDB::save_channel_size_(unsigned long channel_size) noexcept
  {
    update_history_.push_back(channel_size);
    if(update_history_.size() > 8)
    {
      update_history_.pop_front();
    }
  }

  void ChannelServerDB::load_uids_(
    Commons::Postgres::Connection* conn,
    unsigned long& merge_limit,
    UpdateData* data)
    /*throw(
      Commons::Postgres::Exception,
      Commons::Postgres::SqlException,
      TriggerParser::Exception)*/
  {
    Generics::Timer timer, all_timer;
    timer.start();

    enum
    {
      FT_CHANNEL_ID = 1,
      FT_LOB
    };

    Commons::Postgres::Statement_var stmt =
      new Commons::Postgres::Statement(
      "SELECT "
        "channel_id,"
        "uids "
      "FROM adserver.get_audience_channels($1)");

    UpdateData::CheckContainerType& check_ids = data->uid_check_data;

    std::set<unsigned int> check_values;
    if(check_ids.size() > 16)//magic number, no more lobs for one query
    {
      std::copy_n(
        check_ids.begin(),
        16,
        std::insert_iterator<std::set<unsigned int> >(
          check_values, check_values.begin()));
    }
    else
    {
      check_values = check_ids;
    }
    stmt->set_array(1, check_values);
    timer.stop();
    logger_->sstream(Logging::Logger::DEBUG, ASPECT)
      << "Prepare query: " << timer.elapsed_time();

    timer.start();
    Commons::Postgres::ResultSet_var rs = conn->execute_statement(stmt, true);
    timer.stop();
    logger_->sstream(Logging::Logger::DEBUG, ASPECT)
      << "Execute query: " << timer.elapsed_time();

    ChannelIdToMatchInfo* a_info_ptr = data->info_ptr;

    while(rs->next())
    {
      long channel_id = rs->get_number<long>(FT_CHANNEL_ID);
      if(rs->is_null(2))
      {
        continue;
      }
      Oid oid = rs->get_number<Oid>(2);
      Commons::Postgres::Lob lob(conn, oid);
      pg_int64 length = lob.length();
      MatchInfo &s_info = (*a_info_ptr)[channel_id];
      s_info.channel_size = length;
      if(!data->merge_size || data->merge_size + length < merge_limit)
      {
        logger_->sstream(Logging::Logger::TRACE, ASPECT)
          << "Load lob for channel = " << channel_id << " length = " << length;
        TriggerList cur_triggers;
        cur_triggers.push_back(Trigger());
        Trigger& trigger = cur_triggers.back();
        trigger.channel_trigger_id = channel_id;
        trigger.negative = false;
        trigger.trigger.resize(length);
        lob.read(const_cast<char*>(trigger.trigger.data()), length);
        trigger.type = 'D';
        data->merge_size += data->container_ptr->select_parsed_triggers(
          channel_id, cur_triggers, false);
        TriggerParser::TriggerParser::parse_triggers(
          channel_id,
          "",
          cur_triggers,
          0,
          ports_,
          data->container_ptr,
          Commons::DEFAULT_MAX_HARD_WORD_SEQ,
          logger_); 
        data->merge_size += length * TriggerParser::TriggerParser::WORSE_MULT;
        check_values.erase(channel_id);
        check_ids.erase(channel_id);
        data->progress->set_progess(1);
      }
      else
      {
        return;
      }
    }
    if(!check_values.empty())
    {//there isn't data in database
      for(auto it = check_values.begin(); it != check_values.end(); ++it)
      { 
        check_ids.erase(*it);
        data->progress->set_progess(1);
      }
    }
  }

  bool ChannelServerDB::load_triggers_(
    Commons::Postgres::Connection* conn,
    unsigned long& merge_limit,
    UpdateData* data)
    /*throw(
      Commons::Postgres::Exception,
      Commons::Postgres::SqlException,
      TriggerParser::Exception,
      eh::Exception)*/
  {
    if(data->check_data.empty())
    {
      return true;
    }
    Generics::Timer timer, all_timer;

    UpdateData::CheckContainerType& check_ids = data->check_data;

    timer.start();

    enum
    {
      FT_CHANNEL_ID = 1,
      FT_CHANNEL_TRIGGER_ID,
      FT_TRIGGER,
      FT_TYPE,
      FT_NEGATIVE
    };
    
    Commons::Postgres::Statement_var stmt =
      new Commons::Postgres::Statement(
      "SELECT "
        "channel_id,"
        "channel_trigger_id,"
        "original_trigger,"
        "trigger_type, "
        "negative "
      "FROM adserver.get_channel_triggers_by_channel_ids($1)");

    bool all_loaded;
    unsigned int stop_id = 0;
    {
      std::vector<unsigned int> check_values;
      size_t count = 0;
      size_t update_size = get_update_size_(merge_limit);
      check_values.reserve(update_size);
      UpdateData::CheckContainerType::const_iterator it = check_ids.begin();
      for(; it != check_ids.end() && count < update_size; ++it, count++)
      {
        check_values.push_back(*it);
        stop_id = std::max(stop_id, *it);
      }
      all_loaded = (it == check_ids.end());
      logger_->sstream(Logging::Logger::DEBUG, ASPECT)
        << "Use update size: " << update_size 
        << " count id: " << count; 
      stmt->set_array(1, check_values);
    }

    timer.stop();

    logger_->sstream(Logging::Logger::DEBUG, ASPECT)
      << "Prepare query: " << timer.elapsed_time();

    timer.start();

    Commons::Postgres::ResultSet_var rs = conn->execute_statement(stmt);

    timer.stop();

    logger_->sstream(Logging::Logger::DEBUG, ASPECT)
      << "Execute query: " << timer.elapsed_time();

    logger_->sstream(Logging::Logger::DEBUG, ASPECT) << "Fetching data... ";

    bool rs_next = rs->next();
    unsigned long cur_channel_id = rs_next ? rs->get_number<long>(FT_CHANNEL_ID) : 0;
    TriggerList cur_triggers;
    unsigned long channel_size = 0;
    unsigned long progress = data->progress->get_progress();
    unsigned long data_channels_size = 0;

    UpdateData::CheckContainerType::iterator check_it = check_ids.begin();
    ChannelIdToMatchInfo* a_info_ptr = data->info_ptr;
    Generics::Time parse_time;
    all_timer.start();
    while(check_it != check_ids.end())
    {
      if(!rs_next || *check_it < cur_channel_id)
      {
        if(logger_->log_level() >= Logging::Logger::TRACE)
        {
          Stream::Error trace;
          trace << "For channnel_id = " << *check_it
            << " loadded from DB channeltrigger ids (";
          for(TriggerList::const_iterator tr_it = cur_triggers.begin();
              tr_it != cur_triggers.end(); ++tr_it)
          {
            trace << tr_it->channel_trigger_id << ",";
          }
          trace << ")";
          logger_->log(trace.str(), Logging::Logger::DEBUG, ASPECT);
        }
        data->merge_size += data->container_ptr->select_parsed_triggers(
          *check_it, cur_triggers, true);
        MatchInfo &s_info = (*a_info_ptr)[*check_it];
        s_info.channel_size = channel_size;
        timer.start();
        SegmMap::const_iterator segm_it = segmentors_.find(s_info.country);
        if(segm_it == segmentors_.end())
        {
          segm_it = segmentors_.find("");
        }
        TriggerParser::TriggerParser::parse_triggers(
          *check_it,
          s_info.lang,
          cur_triggers,
          segm_it == segmentors_.end() ? 0 : segm_it->second,
          ports_,
          data->container_ptr,
          Commons::DEFAULT_MAX_HARD_WORD_SEQ,
          logger_); 
        timer.stop_add(parse_time);
        //Use WORSE_MULT for compatibility to previous versions
        //don't try to count real channel size in contaier
        data_channels_size +=
          channel_size * TriggerParser::TriggerParser::WORSE_MULT;
        data->merge_size +=
          channel_size * TriggerParser::TriggerParser::WORSE_MULT;

        data->progress->set_progess(1);
        ++check_it; // current check_it already processed (found or not)
        if(merge_limit < channel_size)
        {
          merge_limit = 0;
          all_loaded = false;
          data->need_merge = true;
          break;
        }
        else
        {
          merge_limit -=
            channel_size * TriggerParser::TriggerParser::WORSE_MULT;
        }
        cur_triggers.clear();
        channel_size = 0;

        if(!rs_next)
        {
          while(check_it != check_ids.end() && *check_it <= stop_id)
          {//skip all id, which is absent in database
            data->container_ptr->select_parsed_triggers(
              *check_it, cur_triggers, false);
            //marks triggers on remove if they exists in Contailer
            data->progress->set_progess(1);
            ++check_it;
          }
          break;
        }
      }
      else
      {
        if(*check_it == cur_channel_id)
        {
          cur_triggers.push_back(Trigger());
          Trigger& trigger = cur_triggers.back();
          trigger.channel_trigger_id = rs->get_number<long>(FT_CHANNEL_TRIGGER_ID);
          trigger.negative = (rs->get_char(FT_NEGATIVE) == 'Y');
          rs->get_string(FT_TRIGGER).swap(trigger.trigger);
          trigger.type = rs->get_char(FT_TYPE);
          channel_size += trigger.trigger.size();
          if(data->progress->get_progress() - progress != 0 &&
             channel_size * TriggerParser::TriggerParser::WORSE_MULT > merge_limit)
          {
            data->need_merge = true;
            all_loaded = false;
            break;
          }
        }

        rs_next = rs->next();
        cur_channel_id = rs_next ? rs->get_number<long>(FT_CHANNEL_ID) : 0;
      }
    }
    auto load_progress = data->progress->get_progress() - progress;
    if(load_progress)
    {
      save_channel_size_(data_channels_size / load_progress + 1);
    }

    check_ids.erase(check_ids.begin(), check_it);

    all_timer.stop();
    logger_->sstream(Logging::Logger::DEBUG, ASPECT) << "Parsed: " << load_progress
      << " Parse time: " << parse_time
      << " Load time: " << all_timer.elapsed_time();
    logger_->sstream(Logging::Logger::DEBUG, ASPECT) << "Finish query: " <<
      (all_loaded ? "all loaded" : "partly loaded");

    return all_loaded;
  }

  void ChannelServerDB::update(
    unsigned long merge_limit,
    UpdateData* data)//update data
    /*throw(ChannelServerException::Exception,
      ChannelServerException::TemporyUnavailable)*/
  {
    if(!data->container_ptr->ready())
    {
      Stream::Error err;
      err << __func__ << ": container not ready to parse";
      throw ChannelServerException::TemporyUnavailable(err);
    }
    Commons::Postgres::Connection_var conn;
    try
    {
      conn = pg_pool_->get_connection();
      if(load_triggers_(conn, merge_limit, data))
      {
        load_uids_(conn, merge_limit, data);
      }
    }
    catch(const Commons::Postgres::NotActive& e)
    {//it isn't error
      throw ChannelServerException::TemporyUnavailable(0);
    }
    catch(const Commons::Postgres::Exception& ex)
    {
      pg_pool_->bad_connection(conn);
      Stream::Error ostr;
      ostr << __func__ << ": Postgres::Exception : " << ex.what();
      logger_->log(
        ostr.str(),
        Logging::Logger::CRITICAL,
        ASPECT,
        "ADS-DB-0");
      throw ChannelServerException::Exception(ostr);
    }
    catch(const TriggerParser::Exception& e)
    {
      Stream::Error ostr;
      ostr << __func__ << ": TriggerParser::Exception : " << e.what();
      throw ChannelServerException::Exception(e.what());
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << __func__ << ": eh::Exception : " << ex.what();
      throw ChannelServerException::Exception(ostr);
    }
  }

  /* update ccg keywords */
  void
  ChannelServerDB::update_ccg(UpdateData* data, unsigned long)
    /*throw(ChannelServerException::Exception,
      ChannelServerException::TemporyUnavailable)*/
  {
    Commons::Postgres::Connection_var conn;
    try
    {
      conn = pg_pool_->get_connection();

      data->new_ccg_map = new CCGMap;
      ChannelIdToMatchInfo& info = *data->info_ptr;
      CCGMap::ActiveMap::const_iterator old_iter;

      std::vector<unsigned long> ccg_keyword_ids(info.size());
      unsigned int mask_act = (Channel::CH_ACTIVE | Channel::CH_WAIT);
      unsigned int mask_type = (Channel::CH_PAGE | Channel::CH_SEARCH);
      size_t j = 0;
      for(ChannelIdToMatchInfo::const_iterator it = info.begin();
          it != info.end(); ++it)
      {
        const Channel& ch = it->second.channel;
        if(ch.match_mask(mask_act) && ch.match_mask(mask_type))
        {
          ccg_keyword_ids[j++] = ch.get_id();
        }
      }
      ccg_keyword_ids.resize(j);

      Commons::Postgres::Statement_var stmt =
        new Commons::Postgres::Statement(
        "SELECT "
          "ccg_keyword_id, "
          "status, "
          "ccg_id, "
          "channel_id, "
          "max_cpc, "
          "ctr, "
          "click_url, "
          "original_keyword "
        "FROM adserver.get_ccg_keywords($1)");

      stmt->set_array(1, ccg_keyword_ids);

      enum
      {
        CCG_KEYWORD_ID = 1,
        CCG_STATUS,
        CCG_ID,
        CHANNEL_ID,
        MAX_CPC,
        CTR,
        CLICK_URL,
        ORIGINAL_KEYWORD
      };

      Commons::Postgres::ResultSet_var rs = conn->execute_statement(stmt);

      CCGKeyword_var ccg_ptr;
      unsigned char status;
      unsigned int ccg_keyword_id = data->start_ccg_id + 1;

      while(rs->next())
      {
        unsigned int channel_id = rs->get_number<unsigned int>(CHANNEL_ID);
        if(is_my_id_(channel_id))
        {
          status = rs->get_char(CCG_STATUS);
          ccg_keyword_id = rs->get_number<unsigned int>(CCG_KEYWORD_ID);
          if(status == 'A')
          {
            ccg_ptr = new CCGKeyword;
            ccg_ptr->ccg_keyword_id = ccg_keyword_id;
            ccg_ptr->ccg_id = rs->get_number<unsigned int>(CCG_ID);
            ccg_ptr->channel_id = channel_id;
            bool incorrect_cpc = false;
            try
            {
              ccg_ptr->max_cpc =
                rs->get_decimal<CampaignSvcs::RevenueDecimal>(MAX_CPC);
              ccg_ptr->ctr = rs->get_decimal<CampaignSvcs::CTRDecimal>(CTR);
            }
            catch(const CampaignSvcs::RevenueDecimal::Overflow&)
            {
              incorrect_cpc = true;
            }
            if(incorrect_cpc || ccg_ptr->max_cpc >= data->cost_limit)
            {
              Stream::Error err;
              err << __func__ << ": ccg keywrod with id = " << ccg_keyword_id
                << " has incorrect max_cpc = "
                << rs->get_string(MAX_CPC)
                << " or ctr = " << rs->get_string(CTR)
                << ", cost_limit = " << data->cost_limit;

              logger_->log(
                err.str(),
                Logging::Logger::WARNING,
                ChannelServerCustomImpl::TRAFFICKING_PROBLEM,
                "ADS-TF-8001");
              data->new_ccg_map->deactivate(
                ccg_keyword_id,
                data->new_master,
                data->old_ccg_map);
              continue;
            }
            else if(ccg_ptr->ctr < CampaignSvcs::CTRDecimal::ZERO)
            {
              Stream::Error err;
              err << __func__ << ": ccg keywrod with id = " << ccg_keyword_id
                << " has negative ctr = " << rs->get_string(CTR)
                << " use ctr = 0.0";

              ccg_ptr->ctr = CampaignSvcs::CTRDecimal::ZERO;

              logger_->log(
                err.str(),
                Logging::Logger::ERROR,
                ASPECT,
                "ADS-IMPL-160");
            }
            ccg_ptr->click_url = rs->get_string(CLICK_URL);
            ccg_ptr->original_keyword = rs->get_string(ORIGINAL_KEYWORD);
            data->new_ccg_map->activate(
              ccg_keyword_id,
              ccg_ptr,
              data->new_master,
              data->old_ccg_map);
            info[ccg_ptr->channel_id].channel.ccg_keywords.push_back(
              data->new_ccg_map->active()[ccg_keyword_id]);
          }
          else
          {
            data->new_ccg_map->deactivate(
              ccg_keyword_id,
              data->new_master,
              data->old_ccg_map);
          }
        }
      }
      if(data->old_ccg_map)
      {
        data->new_ccg_map->deactivate_nonactive(*data->old_ccg_map, data->new_master);
      }
      data->ccg_loaded = true;//all load
      data->start_ccg_id = ccg_keyword_id;
    }
    catch(const Commons::Postgres::NotActive& e)
    {//it isn't error
      throw ChannelServerException::TemporyUnavailable(0);
    }
    catch(const Commons::Postgres::Exception& ex)
    {
      pg_pool_->bad_connection(conn);
      Stream::Error ostr;
      ostr << __func__ << ": Postgres::Exception : " << ex.what();
      logger_->log(
        ostr.str(),
        Logging::Logger::CRITICAL,
        ASPECT,
        "ADS-DB-0");
      throw ChannelServerException::Exception(ostr);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << __func__ << ": eh::Exception: " << ex.what();
      throw ChannelServerException::Exception(ostr);
    }
  }

  void ChannelServerDB::change_db_state(bool activation)
    /*throw(ChannelServerException::Exception)*/
  {
    try
    {
      if(activation)
      {
        logger_->log(
          String::SubString("Begin activation db"),
          Logging::Logger::NOTICE, ASPECT);
        pg_env_->activate_object();
        logger_->log(
          String::SubString("End activation db"),
          Logging::Logger::NOTICE, ASPECT);
      }
      else
      {
        logger_->log(
          String::SubString("Begin deactivation db"),
          Logging::Logger::NOTICE, ASPECT);
        pg_env_->deactivate_object();
        logger_->log(
          String::SubString("End deactivation db"),
          Logging::Logger::NOTICE, ASPECT);
        pg_env_->wait_object();
        logger_->log(
          String::SubString("Wait deactivation db"),
          Logging::Logger::NOTICE, ASPECT);
      }
    }
    catch(const Generics::ActiveObject::AlreadyActive& e)
    {
      throw ChannelServerException::Exception("Already activated.");
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << "eh::Exception. :" << e.what();
      throw ChannelServerException::Exception(ostr);
    }
  }

  bool ChannelServerDB::get_db_state() noexcept
  {
    return pg_env_->active();
  }

/*
 *
 * ChannelServerProxy implementation
 *
 */

  ChannelServerProxy::ChannelServerProxy(
    const ChannelSvcs::GroupLoadDescriptionSeq& servers,
    const ServerPoolConfig& proxy_pool_config,
    const std::set<unsigned short>& ports,
    const std::vector<unsigned int>& sources,
    unsigned long count_chunks,
    unsigned long colo,
    const char* version,
    const ServerPoolConfig& campaign_pool_config,
    unsigned service_index,
    Logging::Logger* logger,
    unsigned long check_sum,
    unsigned long priority)
    /*throw(ChannelServerException::Exception)*/
    : ChannelServerVariantBase(
        sources,
        ports,
        count_chunks,
        colo,
        version,
        campaign_pool_config,
        service_index,
        logger,
        check_sum),
      priority_(priority),
      tries_(servers.length()),
      proxy_pool_(),
      load_session_()
  {
    if(proxy_pool_config.iors_list.empty() && !servers.length())
    {
      Stream::Error ostr;
      ostr << __func__ << ": empty list of ChannelProxy references";
      throw ChannelServerException::Exception(ostr);
    }
    if(servers.length())
    {
      load_session_.reset(
        new ChannelLoadSessionImpl(servers, get_source_id()));
    }
    //channel proxy pool
    if(!proxy_pool_config.iors_list.empty())
    {
      try
      {
        proxy_pool_.reset(
          new ChannelProxyPool(
            proxy_pool_config,
            CORBACommons::ChoosePolicyType::PT_BAD_SWITCH));
      }
      catch(const eh::Exception& e)
      {
        Stream::Error ostr;
        ostr << __func__ << ": eh:Exception: " << e.what();
        throw ChannelServerException::Exception(ostr);
      }
    }
    first_load_stamp_ = Generics::Time::ZERO;
  }

  ChannelServerProxy::~ChannelServerProxy() noexcept
  {}

  void
  ChannelServerProxy::update(
    unsigned long merge_limit,
    UpdateData* data)
    /*throw(ChannelServerException::Exception,
      ChannelServerException::TemporyUnavailable)*/
  {
    if(!data->container_ptr->ready())
    {
      Stream::Error err;
      err << __func__ << ": container not ready to parse";
      throw ChannelServerException::TemporyUnavailable(err);
    }
    Generics::Timer timer, all_timer;
    ::AdServer::ChannelSvcs::ChannelCurrent::UpdateData_var result;
    const UpdateData::CheckContainerType& new_ids =
      data->container_ptr->get_new();

    UpdateData::CheckContainerType& check_ids = data->check_data;
    ChannelIdToMatchInfo* a_info_ptr = data->info_ptr;
    all_timer.start();
    CORBA::ULong i = 0;
    size_t count = 0, calc_channel_size = 0;
    ::AdServer::ChannelSvcs::ChannelIdSeq in;
    if(data->unmerged_data.empty())
    {
      size_t channels_size =
        merge_limit / TriggerParser::TriggerParser::WORSE_MULT;
      UpdateData::CheckContainerType::iterator it = check_ids.begin();
      for(; it != check_ids.end(); ++it, count++)
      {
        const MatchInfo &s_info = (*a_info_ptr)[*it];
        if(calc_channel_size + s_info.channel_size < channels_size || count == 0)
        {
          calc_channel_size += s_info.channel_size;
        }
        else
        {
          break;
        }
      }
      logger_->sstream(Logging::Logger::DEBUG, ASPECT)
        << "Use ids: " << count << " channels size: " << calc_channel_size;
      in.length(count);
      std::copy(check_ids.begin(), it, in.get_buffer());

      typedef decltype(
        &AdServer::ChannelSvcs::ChannelUpdateBase_v33::update_triggers)
        FuncType;
      FuncType func_ptr =
        &AdServer::ChannelSvcs::ChannelUpdateBase_v33::update_triggers;
      do_query_(__func__, func_ptr, in, result);

      if(result->source_id != get_source_id())
      {//use old master if source of data has changed
        data->new_master = data->old_master;
      }
    }
    else
    {
      result = data->unmerged_data.get_unmered_data(i);
    }
    ::AdServer::ChannelSvcs::ChannelCurrent::ChannelByIdSeq& channels =
      result->channels;
    unsigned long progress = data->progress->get_progress();
    Generics::Time parse_time;
    if(channels.length())
    {
      TriggerList triggers;
      for(; i < channels.length(); ++i)
      {
        size_t channel_size = 0;
        ChannelCurrent::ChannelById& value = channels[i];
        if(check_ids.find(value.channel_id) == check_ids.end())
        {
          Stream::Error oerr;
          oerr << __func__ << ": implementation error trigger with id = "
            << value.channel_id << " was gotten from proxy, but shouldn't.";
          throw ChannelServerException::Exception(oerr);
        }
        for(size_t j = 0; j < value.words.length(); j++)
        {
          triggers.push_back(Trigger());
          Trigger& trigger = triggers.back();
          trigger.channel_trigger_id = value.words[j].channel_trigger_id;
          Serialization::get_trigger(
            value.words[j].trigger.get_buffer(),
            value.words[j].trigger.length(),
            trigger.trigger);
          trigger.type =
            Serialization::trigger_type(value.words[j].trigger.get_buffer());
          trigger.negative =
            Serialization::negative(value.words[j].trigger.get_buffer());
          channel_size += trigger.trigger.size();
        }

        MatchInfo &s_info = (*a_info_ptr)[value.channel_id];
        s_info.db_stamp = CorbaAlgs::unpack_time(value.stamp);
        s_info.stamp = s_info.db_stamp;
        //don't use master stamp it can stay same, it is bad for slave hosts
        //preserve stamp from central it grows monotonic

        if(data->merge_size != 0 && 
           channel_size * TriggerParser::TriggerParser::WORSE_MULT > merge_limit)
        {
          data->need_merge = true;
          break;
        }
        else
        {
          data->progress->set_progess(1);
          check_ids.erase(value.channel_id);
          bool uid_channel = (s_info.channel.match_mask(
              Channel::CH_URL | Channel::CH_PAGE |
              Channel::CH_SEARCH | Channel::CH_URL_KEYWORDS)) ? false : true;
          data->merge_size += data->container_ptr->select_parsed_triggers(
            value.channel_id, triggers, !uid_channel);
          timer.start();
          s_info.channel_size = channel_size;
          TriggerParser::TriggerParser::parse_triggers(
            value.channel_id,
            s_info.lang,
            triggers,
            0,
            ports_,
            data->container_ptr,
            Commons::DEFAULT_MAX_HARD_WORD_SEQ, logger_); 
          timer.stop_add(parse_time);
          data->merge_size += channel_size * TriggerParser::TriggerParser::WORSE_MULT;
          if(merge_limit < channel_size)
          {
            merge_limit = 0;
            data->need_merge = true;
            ++i;//alread process this channel
            break;
          }
          else
          {
            merge_limit -= channel_size * TriggerParser::TriggerParser::WORSE_MULT;
          }
        }
        triggers.clear();
      }
    }
    if (i < channels.length())
    {//save unprocessed data
      logger_->sstream(Logging::Logger::NOTICE, ASPECT, "ADS-IMPL-47")
        << "Save unparsed data " << i << "/" << channels.length()
        << ", merge size = " << data->merge_size
        << ", first id = " << channels[i].channel_id;
      data->unmerged_data.set_unmered_data(result, i);
    }
    else if(i != in.length())
    {
      for(count = 0; count < in.length(); count++)
      {
        if(check_ids.find(in[count]) != check_ids.end())
        {
          if(new_ids.find(in[count]) != new_ids.end())
          {//erase new and don't touch old
            a_info_ptr->erase(in[count]);
          }
          check_ids.erase(in[count]);
        }
      }
    }
    all_timer.stop();
    auto load_progress = data->progress->get_progress() - progress;
    logger_->sstream(Logging::Logger::DEBUG, ASPECT) << "Parsed: " << load_progress
      << " Parse time: " << parse_time
      << " Load time: " << all_timer.elapsed_time();
    logger_->sstream(Logging::Logger::DEBUG, ASPECT) << "Finish query: " <<
      (check_ids.empty() ? "all loaded" : "partly loaded");
  }

  /* update ccg keywords */
  void ChannelServerProxy::update_ccg(UpdateData* data, unsigned long limit)
    /*throw(ChannelServerException::Exception)*/
  {
    ChannelIdToMatchInfo& info = *data->info_ptr;
    if(data->start_ccg_id == 0)
    {
      data->new_ccg_map = new CCGMap;
    }

    ChannelCurrent::PosCCGResult_var result;
    ChannelCurrent::CCGQuery query;
    query.master_stamp = CorbaAlgs::pack_time(data->old_master);
    query.start = data->start_ccg_id;
    query.limit = limit;
    query.channel_ids.length(info.size());
    query.use_only_list = data->old_ccg_map ? false : true;
    std::set<unsigned long> old_active;
    if(data->old_ccg_map)
    {
      for(CCGMap::ActiveMap::const_iterator it =
          data->old_ccg_map->active().begin();
          it != data->old_ccg_map->active().end(); ++it) 
      {
        old_active.insert(old_active.end(), it->second->channel_id);
      }
    }
    size_t j = 0;
    unsigned int mask_type = (Channel::CH_PAGE | Channel::CH_SEARCH);
    for(ChannelIdToMatchInfo::const_iterator it =
        info.lower_bound(data->start_ccg_id);
        it != info.end() && j < query.channel_ids.length(); ++it)
    {
      const Channel& ch = it->second.channel;
      if(ch.match_mask(mask_type) &&
        (old_active.find(ch.get_id()) == old_active.end()))
      {
        query.channel_ids[j++] = ch.get_id();
      }
    }
    query.channel_ids.length(j);

    typedef decltype(
      &AdServer::ChannelSvcs::ChannelUpdateBase_v33::update_all_ccg)
      FuncType;
    FuncType func_ptr =
      &AdServer::ChannelSvcs::ChannelUpdateBase_v33::update_all_ccg;
    do_query_(__func__, func_ptr, query, result);

    if(result->source_id != get_source_id())
    {//use old master if source of data has changed
      data->new_master = data->old_master;
    }

    data->ccg_loaded = false;
    if(result->keywords.length())
    {
      CCGKeyword_var ccg_ptr;
      for(size_t i = 0; i < result->keywords.length(); i++)
      {
        ChannelCurrent::CCGKeyword& keyword = result->keywords[i];
        if(is_my_id_(keyword.channel_id) &&
           info.find(keyword.channel_id) != info.end())
        {
          ccg_ptr = new CCGKeyword;
          ccg_ptr->ccg_keyword_id = keyword.ccg_keyword_id;
          ccg_ptr->ccg_id = keyword.ccg_id;
          ccg_ptr->channel_id = keyword.channel_id;
          ccg_ptr->max_cpc =
            CorbaAlgs::unpack_decimal<CampaignSvcs::RevenueDecimal>(
              keyword.max_cpc);
          ccg_ptr->ctr =
            CorbaAlgs::unpack_decimal<CampaignSvcs::CTRDecimal>(keyword.ctr);
          ccg_ptr->click_url = keyword.click_url;
          ccg_ptr->original_keyword = keyword.original_keyword;
          data->new_ccg_map->activate(
            keyword.ccg_keyword_id,
            ccg_ptr,
            data->new_master,
            data->old_ccg_map);
          info[ccg_ptr->channel_id].channel.ccg_keywords.push_back(
            data->new_ccg_map->active()[keyword.ccg_keyword_id]);
        }
      }
    }
    else
    {
      for(size_t i = 0; i < result->deleted.length(); i++)
      {
        data->new_ccg_map->deactivate(
          result->deleted[i].id,
          CorbaAlgs::unpack_time(result->deleted[i].stamp),
          data->old_ccg_map);
      }
      data->ccg_loaded = true;
      if(data->old_ccg_map)
      {
        for(CCGMap::ActiveMap::const_iterator old_iter =
            data->old_ccg_map->active().begin();
            old_iter != data->old_ccg_map->active().end(); ++old_iter)
        {
          if(info.find(old_iter->second->channel_id) != info.end())
          {
            if(data->new_ccg_map->active().find(old_iter->first) ==
               data->new_ccg_map->active().end() &&
               data->new_ccg_map->inactive().find(old_iter->first) ==
               data->new_ccg_map->inactive().end())
            {
              info[old_iter->second->channel_id].channel.ccg_keywords.push_back(
                old_iter->second);
              data->new_ccg_map->activate(
                old_iter->first,
                old_iter->second,
                old_iter->second->timestamp,
                data->old_ccg_map);
            }
          }
          else
          {
            data->new_ccg_map->deactivate(
              old_iter->first,
              old_iter->second->timestamp,
              data->old_ccg_map);
          }
        }
      }
    }
    data->start_ccg_id = result->start_id;
  }

  bool ChannelServerProxy::check_source_(
    UpdateData* data,
    int new_source_id,
    const Generics::Time& longest_update,
    const Generics::Time& first_load_stamp,
    const Generics::Time& new_master)
    noexcept
  {
    int old_source_id = get_source_id();
    if(old_source_id == -1)
    {//first load
      set_sources_id_(new_source_id);
      if(first_load_stamp != Generics::Time::ZERO)
      {
        set_first_load_stamp_(first_load_stamp);
      }
    }
    else if(new_source_id != old_source_id)
    {//The source of data has changed, make a query with a lower stamp
      data->old_master -= longest_update;
      set_sources_id_(new_source_id);
      if(first_load_stamp != Generics::Time::ZERO)
      {
        set_first_load_stamp_(first_load_stamp);
      }
      return false;
    }
    else if(get_first_load_stamp() != first_load_stamp)//restart of central server
    {
      if(first_load_stamp == Generics::Time::ZERO)
      {//user old master while central server doesn't load
        data->new_master = data->old_master;
        return true;
      }
      set_first_load_stamp_(first_load_stamp);
      data->old_master = first_load_stamp - longest_update;
      //reload all channels
      return false;
    }
    data->new_master = new_master;
    return true;
  }

  void ChannelServerProxy::check_updating(
    UpdateData* data)//update data
    /*throw(ChannelServerException::Exception,
      ChannelServerException::TemporyUnavailable)*/
  {
    if(!data->container_ptr->ready())
    {
      Stream::Error err;
      err << __func__ << ": container not ready to parse";
      throw ChannelServerException::TemporyUnavailable(err);
    }
    Generics::Timer timer, all_timer;
    ChannelCurrent::CheckData_var res;
    const UpdateData::CheckContainerType& new_ids = data->container_ptr->get_new();
    const UpdateData::CheckContainerType& up_ids = data->container_ptr->get_updated();
    ChannelCurrent::CheckQuery query;
    all_timer.start();
    query.colo_id = colo_;
    query.version << version_;
    query.new_ids.length(new_ids.size() + up_ids.size());
    std::copy(new_ids.begin(), new_ids.end(), query.new_ids.get_buffer());
    std::copy(up_ids.begin(), up_ids.end(), query.new_ids.get_buffer() + new_ids.size());
    query.use_only_list = (data->old_master == Generics::Time::ZERO ? true : false);
    do
    {
      query.master_stamp = CorbaAlgs::pack_time(data->old_master);
      typedef decltype(&AdServer::ChannelSvcs::ChannelUpdateBase_v33::check) FuncType;
      FuncType func_ptr = &AdServer::ChannelSvcs::ChannelUpdateBase_v33::check;
      do_query_(__func__, func_ptr, query, res);
    }
    while(!check_source_(
      data,
      res->source_id,
      CorbaAlgs::unpack_time(res->max_time),
      CorbaAlgs::unpack_time(res->first_stamp),
      CorbaAlgs::unpack_time(res->master_stamp)));

    std::ostringstream ostr;//don't use Stream::Error, it isn't enough
    UpdateData::CheckContainerType& check_ids = data->check_data;
    ChannelIdToMatchInfo::iterator fnd;
    unsigned long id;
    Generics::Time time_value;
    ChannelIdToMatchInfo* a_info_ptr = data->info_ptr;

    check_ids = new_ids;
    if(!res->special_adv)
    {
      a_info_ptr->erase(c_special_adv);
      check_ids.erase(c_special_adv);
    }
    if(!res->special_track)
    {
      a_info_ptr->erase(c_special_track);
      check_ids.erase(c_special_track);
    }

    for(size_t i = 0; i < res->versions.length(); i++)
    {
      id = res->versions[i].id;
      time_value = CorbaAlgs::unpack_time(res->versions[i].stamp);
      fnd = a_info_ptr->find(id);
      if(fnd == a_info_ptr->end())
      {
        ostr << "id = " << id << " was gotten, "
          "but it is not actual" << std::endl;
        continue;
      }
      MatchInfo& m_info = fnd->second;
      if(time_value != m_info.db_stamp ||
         up_ids.find(id) != up_ids.end())
      {
        m_info.channel_size = res->versions[i].size;
        check_ids.insert(check_ids.end(), id);
      }
    }
    all_timer.stop();
    if(logger_->log_level() >= Logging::Logger::TRACE)
    {
      if(ostr.str().size())
      {
        logger_->log(
          ostr.str(),
          Logging::Logger::TRACE,
          ASPECT);
      }
      std::ostringstream ostr;//don't use Stream::Error, it isn't enough
      ostr << "Need to load: ";
      for(UpdateData::CheckContainerType::const_iterator it =
          check_ids.begin(); it != check_ids.end(); ++it)
      {
        if(it != check_ids.begin())
        {
          ostr << ", ";
        }
        ostr << *it;
      }
      ostr << ". Load time: " << all_timer.elapsed_time();
      logger_->log(
        ostr.str(),
        Logging::Logger::TRACE,
        ASPECT);
    }
  }

  UpdateData::UpdateData(UpdateContainer* container)
    noexcept
    : state(US_ZERO),
      info_ptr(),
      start_ccg_id(0),
      ccg_loaded(false),
      need_merge(false),
      progress(0),
      merge_size(0),
      container_ptr(container)
  {
    old_ccg_map = container_ptr->get_helper()->get_ccg();
  }

  void UpdateData::end_iteration() noexcept
  {
    Generics::Time unused, first_unused;
    state = US_START;
    info_ptr.reset();
    start_ccg_id = 0;
    merge_size = 0;
    ccg_loaded = false;
    need_merge = false;
    container_ptr->get_helper()->get_master(first_unused, old_master, unused);
    old_ccg_map = container_ptr->get_helper()->get_ccg();
    check_data.clear();
  }

  Stream::Error& UpdateData::trace_ccg_info_changing(
    Stream::Error& trace_out) const
    noexcept
  {
    {
      CCGMap::ActiveMap::const_iterator new_iter =
        new_ccg_map->active().begin();
      CCGMap::ActiveMap::const_iterator old_iter =
        old_ccg_map->active().begin();
      std::list<unsigned long> added, removed, changed;
      while(old_iter != old_ccg_map->active().end() ||
            new_iter != new_ccg_map->active().end())
      {
        if(new_iter == new_ccg_map->active().end())
        {
          for(;old_iter != old_ccg_map->active().end(); ++old_iter)
          {
            removed.push_back(old_iter->first);
          }
        }
        else if (old_iter == old_ccg_map->active().end())
        {
          for(; new_iter != new_ccg_map->active().end(); ++new_iter)
          {
            added.push_back(new_iter->first);
          }
        }
        else if(old_iter->first < new_iter->first)
        {
          removed.push_back(old_iter->first);
          ++old_iter;
        }
        else if(old_iter->first > new_iter->first)
        {
          added.push_back(new_iter->first);
          ++new_iter;
        }
        else
        {
          if(old_iter->second->timestamp != new_iter->second->timestamp)
          {
            changed.push_back(old_iter->first);
          }
          ++old_iter;
          ++new_iter;
        }
      }
      trace_sequence("Changed ccg in active", changed, trace_out);
      trace_sequence("Removed ccg from active", removed, trace_out);
      trace_sequence("Added ccg to active", added, trace_out);
    }
    {
      CCGMap::InactiveMap::const_iterator new_iter =
        new_ccg_map->inactive().begin();
      CCGMap::InactiveMap::const_iterator old_iter =
        old_ccg_map->inactive().begin();
      std::list<unsigned long> added, removed, changed;
      while(old_iter != old_ccg_map->inactive().end() ||
            new_iter != new_ccg_map->inactive().end())
      {
        if(new_iter == new_ccg_map->inactive().end())
        {
          for(;old_iter != old_ccg_map->inactive().end(); ++old_iter)
          {
            removed.push_back(old_iter->first);
          }
        }
        else if (old_iter == old_ccg_map->inactive().end())
        {
          for(; new_iter != new_ccg_map->inactive().end(); ++new_iter)
          {
            added.push_back(new_iter->first);
          }
        }
        else if(old_iter->first < new_iter->first)
        {
          removed.push_back(old_iter->first);
          ++old_iter;
        }
        else if(old_iter->first > new_iter->first)
        {
          added.push_back(new_iter->first);
          ++new_iter;
        }
        else
        {
          if(old_iter->second.timestamp != new_iter->second.timestamp)
          {
            changed.push_back(old_iter->first);
          }
          ++old_iter;
          ++new_iter;
        }
      }
      trace_sequence("Changed in inactive", changed, trace_out);
      trace_sequence("Removed from inactive", removed, trace_out);
      trace_sequence("Added to inactive", added, trace_out);
    }
    trace_out << "All ccg keywords are loaded.";
    return trace_out;
  }

  void UpdateData::UnmergedData::set_unmered_data(
    ::AdServer::ChannelSvcs::ChannelCurrent::UpdateData_var result,
    CORBA::ULong ind)
    noexcept
  {
    index_ = ind;
    data_ = result;
    for(CORBA::ULong i = 0; i < index_; ++i)
    {//try to free memory
      AdServer::ChannelSvcs::ChannelCurrent::TriggerInfoSeq words;
      (*data_).channels[i].words.swap(words);
    }
  }

  ::AdServer::ChannelSvcs::ChannelCurrent::UpdateData*
  UpdateData::UnmergedData::get_unmered_data(CORBA::ULong& index)
    noexcept
  {
    index = index_;
    return data_._retn();
  }

}
}

