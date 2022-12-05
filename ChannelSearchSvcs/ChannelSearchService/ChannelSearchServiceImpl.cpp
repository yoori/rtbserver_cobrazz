

#include <set>
#include <sstream>

#include <HTTP/UrlAddress.hpp>

#include <Language/GenericSegmentor/Polyglot.hpp>
#include <Language/SegmentorManager/SegmentorManager.hpp>

#include <Commons/Algs.hpp>
#include <Commons/CorbaConfig.hpp>
#include <Commons/CorbaAlgs.hpp>

#include <ChannelSvcs/ChannelCommons/ChannelUtils.hpp>
#include <ChannelSvcs/ChannelManagerController/ChannelSessionFactory.hpp>

#include <CampaignSvcs/CampaignCommons/CampaignCommons.hpp>
#include <CampaignSvcs/CampaignCommons/ExpressionChannelCorbaAdapter.hpp>

#include <xsd/AdServerCommons/AdServerCommons.hpp>

#include "ChannelSearchServiceImpl.hpp"

namespace
{
  static const char ASPECT[] = "ChannelSearchService";
}

namespace
{

  typedef std::list<unsigned long> TriggerIdList;
  typedef AdServer::CampaignSvcs::ChannelIdSet ChannelIdSet;

  typedef std::map<unsigned long, TriggerIdList> ChannelToChTrId;

  struct ChannelMatch
  {
    ChannelIdSet triggers;
    std::set<unsigned long> ccg_ids;
    std::string language;
  };

  typedef std::map<unsigned long, ChannelMatch> ChannelMatchMap;

  void
  fill_channel_triggers(
    const AdServer::ChannelSvcs::ChannelServerBase::ChannelAtomSeq& channel_seq,
    ChannelToChTrId& channel_to_triggers)
  {
    for(CORBA::ULong i = 0; i < channel_seq.length(); ++i)
    {
      channel_to_triggers[channel_seq[i].id].push_back(
        channel_seq[i].trigger_channel_id);
    }
  }

  void fill_channels(
    const AdServer::ChannelSvcs::ChannelServerBase::ChannelAtomSeq& channel_seq,
    ChannelIdSet& channels)
  {
    for(CORBA::ULong i = 0; i < channel_seq.length(); ++i)
    {
      channels.insert(channel_seq[i].id);
    }
  }

  void fill_trigger_seq(
    AdServer::ChannelSearchSvcs::TrChIdSeq& trigger_seq,
    const ChannelIdSet& triggers) noexcept
  {
    trigger_seq.length(triggers.size());
    std::copy(triggers.begin(), triggers.end(), trigger_seq.get_buffer());
  }
}

namespace AdServer
{
  namespace ChannelSearchSvcs
  {
    ChannelSearchServiceImpl::ChannelSearchServiceImpl(
      Generics::ActiveObjectCallback* callback,
      Logging::Logger* logger,
      const ConfigType& config)
      /*throw(Exception, eh::Exception)*/
      : logger_(ReferenceCounting::add_ref(logger)),
        corba_client_adapter_(new CORBACommons::CorbaClientAdapter()),
        task_runner_(new Generics::TaskRunner(callback, 1)),
        scheduler_(new Generics::Planner(callback)),
        SERVICE_INDEX_(config.service_index()),
        channel_matcher_(new ChannelMatcher())
    {
      add_child_object(task_runner_);
      add_child_object(scheduler_);

      campaign_servers_ = resolve_campaign_servers_(
        corba_client_adapter_, config);

      resolve_channel_session_(config, callback);

      task_runner_->enqueue_task(
        Generics::Task_var(new UpdateExpressionChannelsTask(this, 0)));
    }

    ChannelSearchServiceImpl::~ChannelSearchServiceImpl() noexcept
    {}

    ChannelSearchServiceImpl::CampaignServerPoolPtr
    ChannelSearchServiceImpl::resolve_campaign_servers_(
      const CORBACommons::CorbaClientAdapter* corba_client_adapter,
      const ConfigType& config)
      /*throw(Exception, eh::Exception)*/
    {
      static const char* FUN = "ChannelSearchServiceImpl::resolve_campaign_servers_()";

      try
      {
        CORBACommons::CorbaObjectRefList campaign_server_refs;

        Config::CorbaConfigReader::read_multi_corba_ref(
          config.CampaignServerCorbaRef(),
          campaign_server_refs);

        CampaignServerPoolConfig pool_config(corba_client_adapter);
        pool_config.timeout = Generics::Time(10); // 10 sec

        std::copy(
          campaign_server_refs.begin(),
          campaign_server_refs.end(),
          std::back_inserter(pool_config.iors_list));

        return CampaignServerPoolPtr(
          new CampaignServerPool(
            pool_config, CORBACommons::ChoosePolicyType::PT_PERSISTENT));
      }
      catch(const eh::Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": caught eh::Exception: " << e.what();
        throw Exception(ostr);
      }
    }

    void
    ChannelSearchServiceImpl::resolve_channel_session_(
      const ConfigType& config,
      Generics::ActiveObjectCallback* callback)
      /*throw(Exception)*/
    {
      static const char* FUN =
        "ChannelSearchServiceImpl::resolve_channel_session_()";
      try
      {
        AdServer::ChannelSvcs::ChannelServerSessionFactory::init(
          *corba_client_adapter_,
          &server_session_factory_,
          0,
          callback,
          0);

        add_child_object(server_session_factory_);

        CORBACommons::CorbaObjectRefList channel_manager_controller_refs;
        Config::CorbaConfigReader::read_multi_corba_ref(
          config.ChannelManagerControllerRefs(),
          channel_manager_controller_refs);

        ChannelServerSessionPoolConfig pool_config(
          corba_client_adapter_, server_session_factory_);

        std::copy(channel_manager_controller_refs.begin(),
          channel_manager_controller_refs.end(),
          std::back_inserter(pool_config.iors_list));
        pool_config.timeout = Generics::Time(10);

        channel_manager_controllers_.reset(
          new ChannelServerSessionPool(
            pool_config,
            CORBACommons::ChoosePolicyType::PT_BAD_SWITCH));
      }
      catch(const eh::Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": got eh::Exception: " << e.what();
        throw Exception(ostr);
      }
    }

    void ChannelSearchServiceImpl::update_expression_channels_()
      noexcept
    {
      static const char* FUN = "ChannelSearchServiceImpl::update_expression_channels_()";

      typedef CampaignSvcs::ExpressionChannelHolderMap ChannelMap;

      const unsigned long PORTIONS_NUMBER = 20;

      ChannelMatcher::Config_var new_matcher_config;

      bool expression_channels_updated = false;

      try
      {
        for (;;)
        {
          new_matcher_config = new ChannelMatcher::Config;

          CampaignServerPool::ObjectHandlerType campaign_server =
            campaign_servers_->get_object<CampaignServerPool::Exception>(
              logger_,
              Logging::Logger::EMERGENCY,
              ASPECT,
              "ADS_ICON-12",
              SERVICE_INDEX_,
              SERVICE_INDEX_);

          try
          {
            CampaignSvcs::CampaignServer::GetExpressionChannelsInfo request_settings;
            request_settings.timestamp = CorbaAlgs::pack_time(Generics::Time::ZERO);
            request_settings.portions_number = PORTIONS_NUMBER;
            request_settings.channel_types = "KBDE"; // don't load audience channels
            request_settings.channel_statuses = "AIW";
            request_settings.provide_ccg_links = true;
            request_settings.provide_channel_triggers = false;
            request_settings.provide_overlap_channel_ids = false;

            for(unsigned long portion = 0; portion < PORTIONS_NUMBER; ++portion)
            {
              request_settings.portion = portion;

              /* update expressions */
              CampaignSvcs::ExpressionChannelsInfo_var expression_channels_info =
                campaign_server->get_expression_channels(request_settings);

              const CampaignSvcs::ExpressionChannelSeq& expression_channel_seq =
                expression_channels_info->expression_channels;

              for(CORBA::ULong i = 0; i < expression_channel_seq.length(); ++i)
              {
                CampaignSvcs::ExpressionChannelBase_var new_channel =
                  CampaignSvcs::unpack_channel(
                    expression_channel_seq[i],
                    new_matcher_config->expression_channels);

                ChannelMap::iterator ch_it = new_matcher_config->expression_channels.find(
                  new_channel->params().channel_id);

                if(ch_it != new_matcher_config->expression_channels.end())
                {
                  ch_it->second->channel = new_channel;
                }
                else
                {
                  new_matcher_config->expression_channels.insert(
                    std::make_pair(new_channel->params().channel_id,
                      CampaignSvcs::ExpressionChannelHolder_var(
                        new CampaignSvcs::ExpressionChannelHolder(new_channel))));
                }
              }

              const AdServer::CampaignSvcs::ExpressionChannelCCGSeq& ccg_links =
                expression_channels_info->expression_channel_ccgs;

              for(CORBA::ULong ch_i = 0; ch_i < ccg_links.length(); ++ch_i)
              {
                ChannelMatcher::Config::ChannelTraits& channel_traits =
                  new_matcher_config->expression_channel_traits_map[
                    ccg_links[ch_i].channel_id];

                CorbaAlgs::convert_sequence(
                  ccg_links[ch_i].ccgs,
                  channel_traits.ccg_ids);
              }
            }

            expression_channels_updated = true;
            break;
          }
          catch(const AdServer::CampaignSvcs::CampaignServer::NotReady&)
          {
            campaign_server.release_bad(String::SubString("CampaignServer is not ready"));
          }
          catch(const AdServer::CampaignSvcs::CampaignServer::ImplementationException& ex)
          {
            Stream::Error ostr;
            ostr << FUN << ": caught CampaignServer::ImplementationException "
              "on update config: " << ex.description;

            campaign_server.release_bad(ostr.str());
            logger_->log(ostr.str(),
              Logging::Logger::ERROR,
              ASPECT,
              "ADS-IMPL-147");
          }
          catch(const CORBA::SystemException& e)
          {
            Stream::Error ostr;
            ostr << FUN << ": caught CORBA exception on update config: " << e;

            campaign_server.release_bad(ostr.str());
            logger_->log(ostr.str(),
              Logging::Logger::ERROR,
              ASPECT,
              "ADS-ICON-12");
          }
        }
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": caught eh::Exception on update config: " << ex.what();

        logger_->log(ostr.str(),
          Logging::Logger::ERROR,
          ASPECT,
          "ADS-ICON-12");
      }

      if(expression_channels_updated)
      {
        try
        {
          channel_matcher_->config(new_matcher_config);
        }
        catch(const eh::Exception& ex)
        {
          Stream::Error ostr;
          ostr << FUN << ": caught eh::Exception on channels indexing: " << ex.what();

          logger_->log(ostr.str(),
            Logging::Logger::ERROR,
            ASPECT);
        }

        if (logger_->log_level() >= Logging::Logger::TRACE)
        {
          Stream::Error ostr;
          ostr << FUN << ": Expression channels config updated";
          logger_->trace(ostr.str(), ASPECT);
        }
      }

      try
      {
        Generics::Goal_var task(
          new UpdateExpressionChannelsTask(this, task_runner_));

        scheduler_->schedule(task, Generics::Time::get_time_of_day() + 60);
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": caught eh::Exception on task scheduling: " << ex.what();

        logger_->log(ostr.str(),
          Logging::Logger::ERROR,
          ASPECT);
      }
    }

    void
    ChannelSearchServiceImpl::channel_session_match_(
      const AdServer::ChannelSvcs::ChannelServerBase::MatchQuery& query,
      AdServer::ChannelSvcs::ChannelServerBase::MatchResult_var&
        match_result)
      /*throw(
        AdServer::ChannelSearchSvcs::ChannelSearch::ImplementationException)*/
    {
      static const char* FUN = "ChannelSearchServiceImpl::channel_session_match_()";
      try
      {
        ChannelServerHandler channel_server =
          channel_manager_controllers_->get_object<Exception>();
        try
        {
          channel_server->match(query, match_result.out());
        }
        catch (const AdServer::ChannelSvcs::ImplementationException& ex)
        {
          Stream::Error ostr;
          ostr << FUN <<
            ": Caught ChannelServer::ImplementationException: " <<
            ex.description;

          CORBACommons::throw_desc<AdServer::
            ChannelSearchSvcs::ChannelSearch::ImplementationException>(
              ostr.str());
        }
        catch (const AdServer::ChannelSvcs::NotConfigured& ex)
        {
          Stream::Error ostr;
          ostr << FUN << ": Caught ChannelServer::NotConfigured.";

          CORBACommons::throw_desc<AdServer::
            ChannelSearchSvcs::ChannelSearch::ImplementationException>(
              ostr.str());
        }
        catch (const CORBA::SystemException& ex)
        {
          Stream::Error ostr;
          ostr << FUN << ": Caught CORBA::SystemException by "
            "ChannelServerSession::match(): " << ex;

          channel_server.release_bad(ostr.str());
          CORBACommons::throw_desc<AdServer::
            ChannelSearchSvcs::ChannelSearch::ImplementationException>(
              ostr.str());
        }
      }
      catch (const Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ex.what();
        CORBACommons::throw_desc<AdServer::
          ChannelSearchSvcs::ChannelSearch::ImplementationException>(
            ostr.str());
      }
    }

    AdServer::ChannelSearchSvcs::ChannelSearchResultSeq*
    ChannelSearchServiceImpl::search(const char* phrase)
      /*throw(AdServer::ChannelSearchSvcs::ChannelSearch::ImplementationException)*/
    {
      static const char* FUN = "ChannelSearchServiceImpl::search()";

      try
      {
        ChannelSearchResultSeq_var result =
          new AdServer::ChannelSearchSvcs::ChannelSearchResultSeq;

        ChannelIdSet simple_channels;

        AdServer::ChannelSvcs::ChannelServerBase::MatchQuery query;
        AdServer::ChannelSvcs::ChannelServerBase::MatchResult_var
          match_result;

        String::SubString sph(phrase);
        query.request_id << sph;
        try
        {
          query.first_url << HTTP::BrowserAddress(sph).url();
        }
        catch(const eh::Exception&)
        {
        }
        query.pwords << sph;
        query.swords << sph;
        query.uid = CorbaAlgs::pack_user_id(Generics::Uuid());

        query.non_strict_word_match = true;
        query.non_strict_url_match = true;
        query.return_negative = true;
        query.simplify_page = true;
        query.fill_content = false;
        query.statuses[0] = 'A';
        query.statuses[1] = 'I';

        try
        {
          channel_session_match_(query, match_result);
        }
        catch (const AdServer::ChannelSearchSvcs::
          ChannelSearch::ImplementationException& ex)
        {
          Stream::Error ostr;
          ostr << FUN << ": Caught ChannelSearch::ImplementationException" <<
            ex.description;

          CORBACommons::throw_desc<AdServer::
            ChannelSearchSvcs::ChannelSearch::ImplementationException>(
              ostr.str());
        }

        fill_channels(
          match_result->matched_channels.page_channels, simple_channels);
        fill_channels(
          match_result->matched_channels.search_channels, simple_channels);
        fill_channels(
          match_result->matched_channels.url_channels, simple_channels);
        fill_channels(
          match_result->matched_channels.url_keyword_channels, simple_channels);

        ChannelIdSet expression_channels;
        try
        {
          channel_matcher_->search(expression_channels, simple_channels);
        }
        catch (const ChannelMatcher::Exception& ex)
        {
          Stream::Error ostr;
          ostr << FUN << ": Caught Exception: " << ex.what();
          CORBACommons::throw_desc<
            ChannelSearchSvcs::ChannelSearch::ImplementationException>(
              ostr.str());
        }

        result->length(expression_channels.size());

        CORBA::ULong i = 0;
        for (ChannelIdSet::const_iterator ch_it = expression_channels.begin();
             ch_it != expression_channels.end(); ++ch_it, ++i)
        {
          AdServer::ChannelSearchSvcs::ChannelSearchResult& result_ref = result[i];
          result_ref.channel_id = *ch_it;
          result_ref.reuse = 1;
        }

        if (logger_->log_level() >= Logging::Logger::TRACE)
        {
          std::ostringstream trace_ss;

          trace_ss << std::endl <<
            "Matching for phrase = '" << phrase << "'" << std::endl;

          trace_ss << "Trigger matched channels(" << simple_channels.size() <<
            "):" << std::endl << "  ";

          Algs::print(trace_ss, simple_channels.begin(), simple_channels.end());

          trace_ss << std::endl;

          const ChannelSearchResultSeq& result_ref = result.in();

          trace_ss << "Result channels(" << result_ref.length() << "): " <<
            std::endl;

          for (size_t i = 0; i < result_ref.length(); i++)
          {
            const AdServer::ChannelSearchSvcs::ChannelSearchResult&
              cur_result = result_ref[i];

            trace_ss << "  channel_id = " << cur_result.channel_id <<
              ", reuse = " << cur_result.reuse << std::endl;
          }

          logger_->trace(trace_ss.str(), ASPECT);
        }

        return result._retn();
      }
      catch (const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": Caught eh::Exception: " << ex.what();

        CORBACommons::throw_desc<
          ChannelSearchSvcs::ChannelSearch::ImplementationException>(
            ostr.str());
      }
      catch (const CORBA::SystemException& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": Caught CORBA::SystemException: " << ex;
        CORBACommons::throw_desc<
          ChannelSearchSvcs::ChannelSearch::ImplementationException>(
            ostr.str());
      }
      return 0; // never reach
    }

    AdServer::ChannelSearchSvcs::MatchInfo*
    ChannelSearchServiceImpl::match(
        const char *url,
        const char* match_phrase,
        CORBA::Long channels_count)
      /*throw(AdServer::ChannelSearchSvcs::ChannelSearch::ImplementationException)*/
    {
      static const char* FUN = "ChannelSearchServiceImpl::match()";

      try
      {
        MatchInfo_var result(new MatchInfo());
        ChannelIdSet simple_channels;
        ChannelToChTrId ch_to_tr_id;
        ChannelMatchMap channel_matches;

        AdServer::ChannelSvcs::ChannelServerBase::MatchQuery query;
        AdServer::ChannelSvcs::ChannelServerBase::MatchResult_var
          match_result;

        String::SubString smp(match_phrase);
        query.request_id << smp;
        try
        {
          query.first_url <<
            HTTP::BrowserAddress(String::SubString(url)).url();
        }
        catch(const eh::Exception&)
        {
        }
        query.pwords << smp;
        query.swords << smp;

        query.non_strict_word_match = false;
        query.non_strict_url_match = false;
        query.return_negative = false;
        query.simplify_page = false;
        query.fill_content = false;

        query.statuses[0] = 'A';
        query.statuses[1] = '\0';

        try
        {
          channel_session_match_(query, match_result);
        }
        catch (const AdServer::ChannelSearchSvcs::
          ChannelSearch::ImplementationException& ex)
        {
          Stream::Error ostr;
          ostr << FUN << ": Caught ChannelSearch::ImplementationException" <<
            ex;

          CORBACommons::throw_desc<AdServer::
            ChannelSearchSvcs::ChannelSearch::ImplementationException>(
              ostr.str());
        }

        fill_channels(
          match_result->matched_channels.page_channels, simple_channels);
        fill_channels(
          match_result->matched_channels.search_channels, simple_channels);
        fill_channels(
          match_result->matched_channels.url_channels, simple_channels);
        fill_channels(
          match_result->matched_channels.url_keyword_channels, simple_channels);

        // fill simple channel to triggers
        fill_channel_triggers(
          match_result->matched_channels.page_channels, ch_to_tr_id);
        fill_channel_triggers(
          match_result->matched_channels.search_channels, ch_to_tr_id);
        fill_channel_triggers(
          match_result->matched_channels.url_channels, ch_to_tr_id);
        fill_channel_triggers(
          match_result->matched_channels.url_keyword_channels, ch_to_tr_id);

        ChannelMatcher::ChannelMatchResultMap result_expression_channels;

        try
        {
          channel_matcher_->match(result_expression_channels, simple_channels);
        }
        catch (const ChannelMatcher::Exception& ex)
        {
          Stream::Error ostr;
          ostr << FUN << ": Caught Exception: " << ex.what();
          CORBACommons::throw_desc<
            ChannelSearchSvcs::ChannelSearch::ImplementationException>(
              ostr.str());
        }

        for (ChannelMatcher::ChannelMatchResultMap::iterator ch_it =
               result_expression_channels.begin();
             ch_it != result_expression_channels.end(); ++ch_it)
        {
          ChannelMatcher::ChannelMatchResult& ch_match_result = ch_it->second;

          ChannelMatch& channel_match = channel_matches[ch_it->first];

          for(ChannelIdSet::const_iterator ms_ch_it =
                ch_it->second.matched_simple_channels.begin();
              ms_ch_it != ch_it->second.matched_simple_channels.end(); ++ms_ch_it)
          {
            ChannelToChTrId::const_iterator ch_to_tr_it =
              ch_to_tr_id.find(*ms_ch_it);

            if(ch_to_tr_it != ch_to_tr_id.end())
            {
              // fill matched triggers for simple channels
              std::copy(
                ch_to_tr_it->second.begin(),
                ch_to_tr_it->second.end(),
                std::inserter(channel_match.triggers, channel_match.triggers.begin()));
            }
          }

          channel_match.ccg_ids.swap(ch_match_result.ccg_ids);
        }


        {
          // fill result
          result->channels.length(channel_matches.size());

          CORBA::Long ch_match_i = 0;

          for(ChannelMatchMap::const_iterator ch_match_it =
                channel_matches.begin();
              ch_match_it != channel_matches.end() && ch_match_i < channels_count;
              ++ch_match_it)
          {
            result->channels[ch_match_i].channel_id = ch_match_it->first;

            fill_trigger_seq(
              result->channels[ch_match_i].triggers,
              ch_match_it->second.triggers);

            CorbaAlgs::fill_sequence(
              ch_match_it->second.ccg_ids.begin(),
              ch_match_it->second.ccg_ids.end(),
              result->channels[ch_match_i].ccgs);

            ++ch_match_i;
          }

          result->channels.length(ch_match_i);
        }

        if (logger_->log_level() >= Logging::Logger::TRACE)
        {
          std::ostringstream trace_ss;

          trace_ss << std::endl <<
            "Matching for phrase = '" << match_phrase << "'" << std::endl <<
            "Trigger matched channels(" << simple_channels.size() <<
            "):" << std::endl << "  ";

          Algs::print(trace_ss, simple_channels.begin(), simple_channels.end());

          trace_ss << std::endl;

          logger_->trace(trace_ss.str(), ASPECT);
        }

        return result._retn();
      }
      catch (const CORBA::SystemException& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": Caught CORBA::SystemException: " << ex;
        CORBACommons::throw_desc<
          ChannelSearchSvcs::ChannelSearch::ImplementationException>(
            ostr.str());
      }
      return 0; // never reach
    }

    AdServer::ChannelSearchSvcs::ChannelSearchResultSeq*
    ChannelSearchServiceImpl::wsearch(const CORBA::WChar* phrase)
      /*throw(AdServer::ChannelSearchSvcs::ChannelSearch::ImplementationException)*/
    {
      static const char* FUN = "ChannelSearchServiceImpl::wsearch()";

      /* convert phrase to utf-8 */
      std::string utf8_phrase;

      try
      {
        String::StringManip::wchar_to_utf8((const wchar_t*)phrase, utf8_phrase);
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN <<
          ": Can't convert wchar string to utf8. Caught eh::Exception: " <<
          ex.what();

        CORBACommons::throw_desc<
          ChannelSearchSvcs::ChannelSearch::ImplementationException>(
            ostr.str());
      }

      return search(utf8_phrase.c_str());
    }

    AdServer::ChannelSearchSvcs::WMatchInfo*
    ChannelSearchServiceImpl::wmatch(
      const CORBA::WChar *url,
      const CORBA::WChar *phrase,
      CORBA::Long channels_count)
      /*throw(AdServer::ChannelSearchSvcs::ChannelSearch::ImplementationException)*/
    {
      static const char *FUN = "ChannelSearchServiceImpl::wmatch()";

      std::string utf8_url;
      std::string utf8_phrase;

      try
      {
        using String::StringManip::wchar_to_utf8;
        wchar_to_utf8(static_cast<const wchar_t*>(phrase), utf8_phrase);
        wchar_to_utf8(static_cast<const wchar_t*>(url), utf8_url);
      }
      catch (const eh::Exception &ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": Can't convert wchar string to utf8. "
           << "Caught eh::Exception: " << ex.what();
        CORBACommons::throw_desc<
          ChannelSearchSvcs::ChannelSearch::ImplementationException>(
            ostr.str());
      }

      try
      {
        using String::StringManip::utf8_to_wchar;

        WMatchInfo_var result(new WMatchInfo());

        MatchInfo_var match_result =
          match(utf8_url.c_str(), utf8_phrase.c_str(), channels_count);

        /* copy channels */
        result->channels = match_result->channels;

        return result._retn();
      }
      catch (const CORBA::SystemException &ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": Caught CORBA::SystemException: " << ex;
        CORBACommons::throw_desc<
          ChannelSearchSvcs::ChannelSearch::ImplementationException>(
            ostr.str());
      }
      return 0; // never reach
    }

  } // namespace ChannelSearchSvcs
} // namespace AdServer
