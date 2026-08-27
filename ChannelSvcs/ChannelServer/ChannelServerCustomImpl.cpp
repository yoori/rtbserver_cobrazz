#include "ChannelServerCustomImpl.hpp"

#include <Commons/CorbaAlgs.hpp>

namespace AdServer::ChannelSvcs
{
  ChannelServerCustomImpl::ChannelServerCustomImpl(ChannelServerCorePtr core) noexcept
    : core_(std::move(core))
  {}

  ChannelServerCustomImpl::~ChannelServerCustomImpl() noexcept = default;

  void ChannelServerCustomImpl::match(
    const ::AdServer::ChannelSvcs::ChannelServerBase::MatchQuery& query,
    ::AdServer::ChannelSvcs::ChannelServer::MatchResult_out result)
  {
    try
    {
      ChannelServerCore::MatchQuery core_query;
      core_query.request_id = query.request_id.in();
      core_query.first_url = query.first_url.in();
      core_query.first_url_words = query.first_url_words.in();
      core_query.urls = query.urls.in();
      core_query.urls_words = query.urls_words.in();
      core_query.pwords = query.pwords.in();
      core_query.swords = query.swords.in();
      core_query.uid = CorbaAlgs::unpack_user_id(query.uid);
      core_query.statuses[0] = query.statuses[0];
      core_query.statuses[1] = query.statuses[1];
      core_query.non_strict_word_match = query.non_strict_word_match;
      core_query.non_strict_url_match = query.non_strict_url_match;
      core_query.return_negative = query.return_negative;
      core_query.simplify_page = query.simplify_page;
      core_query.fill_content = query.fill_content;

      ChannelServerCore::MatchResult core_result;
      core_->match(core_query, core_result);

      result = new ::AdServer::ChannelSvcs::ChannelServer::MatchResult;
      result->no_adv = core_result.no_adv;
      result->no_track = core_result.no_track;
      result->match_time = CorbaAlgs::pack_time(core_result.match_time);

      auto pack_channels =
        [](
          const std::vector<ChannelServerCore::ChannelAtom>& source,
          ChannelServerBase::ChannelAtomSeq& target)
        {
          target.length(source.size());
          for (CORBA::ULong i = 0; i < source.size(); ++i)
          {
            target[i].id = source[i].id;
            target[i].trigger_channel_id = source[i].trigger_channel_id;
          }
        };

      pack_channels(
        core_result.matched_channels.page_channels,
        result->matched_channels.page_channels);
      pack_channels(
        core_result.matched_channels.search_channels,
        result->matched_channels.search_channels);
      pack_channels(
        core_result.matched_channels.url_channels,
        result->matched_channels.url_channels);
      pack_channels(
        core_result.matched_channels.url_keyword_channels,
        result->matched_channels.url_keyword_channels);

      result->matched_channels.uid_channels.length(
        core_result.matched_channels.uid_channels.size());
      for (CORBA::ULong i = 0; i < core_result.matched_channels.uid_channels.size(); ++i)
      {
        result->matched_channels.uid_channels[i] = core_result.matched_channels.uid_channels[i];
      }

      result->content_channels.length(core_result.content_channels.size());
      for (CORBA::ULong i = 0; i < core_result.content_channels.size(); ++i)
      {
        result->content_channels[i].id = core_result.content_channels[i].id;
        result->content_channels[i].weight = core_result.content_channels[i].weight;
      }
    }
    catch(const ChannelServerCore::NotConfigured& ex)
    {
      throw AdServer::ChannelSvcs::NotConfigured(ex.what());
    }
    catch(const ChannelServerCore::Exception& ex)
    {
      const String::SubString description(ex.what());
      CORBACommons::throw_desc<
        ChannelSvcs::ImplementationException>(description);
    }
  }

  void ChannelServerCustomImpl::get_ccg_traits(
    const ::AdServer::ChannelSvcs::ChannelIdSeq& ids,
    ::AdServer::ChannelSvcs::ChannelServer::TraitsResult_out result)
  {
    try
    {
      std::vector<unsigned long> core_ids;
      core_ids.reserve(ids.length());
      for (CORBA::ULong i = 0; i < ids.length(); ++i)
      {
        core_ids.push_back(ids[i]);
      }

      ChannelServerCore::TraitsResult core_result;
      core_->get_ccg_traits(core_ids, core_result);

      result = new AdServer::ChannelSvcs::ChannelServer::TraitsResult;
      result->ccg_keywords.length(core_result.ccg_keywords.size());
      for (CORBA::ULong i = 0; i < core_result.ccg_keywords.size(); ++i)
      {
        const auto& source = core_result.ccg_keywords[i];
        result->ccg_keywords[i].ccg_keyword_id = source.ccg_keyword_id;
        result->ccg_keywords[i].ccg_id = source.ccg_id;
        result->ccg_keywords[i].channel_id = source.channel_id;
        result->ccg_keywords[i].max_cpc =
          CorbaAlgs::pack_decimal<CampaignSvcs::RevenueDecimal>(source.max_cpc);
        result->ccg_keywords[i].ctr = CorbaAlgs::pack_decimal<CampaignSvcs::CTRDecimal>(source.ctr);
        result->ccg_keywords[i].click_url << source.click_url;
        result->ccg_keywords[i].original_keyword << source.original_keyword;
      }

      result->neg_ccg.length(core_result.neg_ccg.size());
      for (CORBA::ULong i = 0; i < core_result.neg_ccg.size(); ++i)
      {
        result->neg_ccg[i] = core_result.neg_ccg[i];
      }
    }
    catch(const ChannelServerCore::NotConfigured& ex)
    {
      throw AdServer::ChannelSvcs::NotConfigured(ex.what());
    }
    catch(const ChannelServerCore::Exception& ex)
    {
      const String::SubString description(ex.what());
      CORBACommons::throw_desc<
        ChannelSvcs::ImplementationException>(description);
    }
  }
}
