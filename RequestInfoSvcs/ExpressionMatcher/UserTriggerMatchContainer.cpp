#include "UserTriggerMatchContainer.hpp"

#include <algorithm>

#include <Generics/Rand.hpp>
#include <Logger/ActiveObjectCallback.hpp>

#include <RequestInfoSvcs/RequestInfoCommons/UserTriggerMatchProfile.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/RequestTriggerMatchProfile.hpp>
#include <RequestInfoSvcs/ExpressionMatcher/Compatibility/UserTriggerMatchProfileAdapter.hpp>
#include <RequestInfoSvcs/ExpressionMatcher/Compatibility/RequestTriggerMatchProfileAdapter.hpp>

namespace Aspect
{
  const char USER_TRIGGER_MATCH_CONTAINER[] = "UserTriggerMatchContainer";
}

namespace AdServer
{
namespace RequestInfoSvcs
{
  const Generics::Time
  UserTriggerMatchContainer::DEFAULT_USER_EXPIRE_TIME = Generics::Time::ONE_DAY * 30;

  const Generics::Time
  UserTriggerMatchContainer::DEFAULT_REQUEST_EXPIRE_TIME = Generics::Time::ONE_DAY * 2;

  namespace
  {
    const Generics::Time IMPRESSION_EXPIRE_TIME = Generics::Time::ONE_DAY;
    const unsigned long MAX_IMPRESSIONS_KEEP = 10;

    // profile manipulations
    struct ChannelIdLess
    {
      bool operator()(const ChannelMatchReader& left, unsigned long right) const
      {
        return left.channel_id() < right;
      }

      bool operator()(unsigned long left, const ChannelMatchReader& right) const
      {
        return left < right.channel_id();
      }
    };

    struct TimeLess
    {
      template<typename LeftType>
      bool operator()(const LeftType& left, const Generics::Time& right) const
      {
        return left.time() < right.tv_sec;
      }

      template<typename RightType>
      bool operator()(const Generics::Time& left, const RightType& right) const
      {
        return left.tv_sec < right.time();
      }
    };

    struct UserTriggerMatchLess
    {
      template<typename LeftType, typename RightType>
      bool operator()(const LeftType& left, const RightType& right) const
      {
        return left.channel_id() < right.channel_id() ||
          (left.channel_id() == right.channel_id() &&
          left.time() <= right.time());
      }
    };

    class UserTriggerMatchWriterAdapter
    {
    public:
      const ChannelMatchWriter& operator()(const ChannelMatchWriter& writer)
      {
        return writer;
      }

      ChannelMatchWriter operator()(const ChannelMatchReader& reader)
      {
        ChannelMatchWriter match_writer;
        match_writer.channel_id() = reader.channel_id();
        match_writer.time() = reader.time();
        match_writer.triggers_in_channel() = reader.triggers_in_channel();
        match_writer.matched_triggers() = reader.matched_triggers();
        match_writer.positive_matches().reserve(reader.positive_matches().size());
        std::copy(
          reader.positive_matches().begin(),
          reader.positive_matches().end(),
          std::back_inserter(match_writer.positive_matches()));
        match_writer.negative_matches().reserve(reader.negative_matches().size());
        std::copy(
          reader.negative_matches().begin(),
          reader.negative_matches().end(),
          std::back_inserter(match_writer.negative_matches()));
        return match_writer;
      }
    };

    void fill_index_sampling(
      std::vector<unsigned long>& sampling,
      unsigned long array_size,
      unsigned long sampling_size)
    {
      sampling.reserve(sampling.size() + sampling_size);
      for(unsigned long i = 0; i < sampling_size; ++i)
      {
        unsigned long new_val = Generics::safe_rand(array_size - sampling.size());
        unsigned long add_ind = 0;
        std::vector<unsigned long>::iterator vit = sampling.begin();
        for(; vit != sampling.end() && new_val + add_ind >= *vit; ++vit)
        {
          ++add_ind;
        }
        sampling.insert(vit, new_val + add_ind);
      }
    }

    typedef UserTriggerMatchContainer::Config::ChannelInfo::TriggerIdArray
      TriggerIdArray;

    /* deactivated_triggers:
     *   triggers that matched, but not found in channel (deactivated),
     *   must be used for compensate triggers
     */
    void fill_noised_triggers(
      UserTriggerMatchContainer::MatchedTriggerIdArray& deactivated_triggers,
      ChannelMatchWriter::positive_matches_Container& noised_triggers,
      const UserTriggerMatchContainer::MatchedTriggerIdArray& triggers,
      const TriggerIdArray& channel_triggers,
      unsigned long noise_size)
    {
      const long use_noise_size = static_cast<long>(noise_size) - 1;

      noised_triggers.reserve(
        noised_triggers.size() + (
          use_noise_size > 0 ? triggers.size() * (use_noise_size + 1) :
          triggers.size()));

      std::vector<unsigned long> trigger_indexes;

      for(std::vector<unsigned long>::const_iterator tr_it = triggers.begin();
          tr_it != triggers.end(); ++tr_it)
      {
        trigger_indexes.clear();

        bool insert_tr_into_noise = false;

        {
          TriggerIdArray::const_iterator tr_idx_it = std::lower_bound(
            channel_triggers.begin(),
            channel_triggers.end(),
            *tr_it);
          if(tr_idx_it != channel_triggers.end() && *tr_idx_it == *tr_it)
          {
            trigger_indexes.push_back(tr_idx_it - channel_triggers.begin());
          }
          else
          {
            deactivated_triggers.push_back(*tr_it);
            insert_tr_into_noise = true;
          }
        }

        if(use_noise_size > 0) // number of triggers in channel > 1
        {
          fill_index_sampling(
            trigger_indexes,
            channel_triggers.size(),
            use_noise_size);
        }

        //
        // fill noised triggers
        for(std::vector<unsigned long>::const_iterator tr_idx_it =
              trigger_indexes.begin();
            tr_idx_it != trigger_indexes.end(); ++tr_idx_it)
        {
          noised_triggers.push_back(channel_triggers[*tr_idx_it]);
        }

        // insert deactivated trigger into ordered pos for hide it in noise (privacy)
        if(insert_tr_into_noise)
        {
          noised_triggers.insert(
            std::lower_bound(noised_triggers.begin(), noised_triggers.end(), *tr_it),
            *tr_it);
        }
      }
    }

    void fill_compensate_triggers(
      ChannelMatchWriter::negative_matches_Container& compensate_triggers,
      const TriggerIdArray& channel_triggers,
      const TriggerIdArray& deactivated_channel_triggers,
      unsigned long matched_triggers,
      unsigned long compensate_size)
    {
      std::vector<unsigned long> trigger_indexes;

      compensate_triggers.reserve(
        compensate_triggers.size() + matched_triggers * compensate_size);

      for(unsigned long i = 0; i < matched_triggers; ++i)
      {
        trigger_indexes.clear();

        fill_index_sampling(
          trigger_indexes,
          channel_triggers.size() + deactivated_channel_triggers.size(),
          compensate_size);

        for(std::vector<unsigned long>::const_iterator tr_idx_it =
              trigger_indexes.begin();
            tr_idx_it != trigger_indexes.end(); ++tr_idx_it)
        {
          if(*tr_idx_it < channel_triggers.size())
          {
            compensate_triggers.push_back(
              channel_triggers[*tr_idx_it]);
          }
          else
          {
            compensate_triggers.push_back(
              deactivated_channel_triggers[*tr_idx_it - channel_triggers.size()]);
          }
        }
      }
    }

    void add_matches(
      UserTriggerMatchWriter::page_matches_Container& res_matches,
      const UserTriggerMatchContainer::MatchMap& new_matches,
      const Generics::Time& time,
      const UserTriggerMatchContainer::Config::ChannelInfoMap& all_channels,
      TriggerIdArray UserTriggerMatchContainer::Config::ChannelInfo::* channel_triggers_field,
      unsigned long positive_group_size,
      unsigned long negative_group_size)
    {
      UserTriggerMatchWriter::page_matches_Container::iterator res_it =
        res_matches.begin();

      for(UserTriggerMatchContainer::MatchMap::
            const_iterator mit = new_matches.begin();
          mit != new_matches.end(); ++mit)
      {
        while(res_it != res_matches.end() && (
          res_it->channel_id() < mit->first || (
            res_it->channel_id() == mit->first &&
            res_it->time() <= time.tv_sec)))
        {
          ++res_it;
        }

        ChannelMatchWriter& new_channel_match_writer =
          *res_matches.insert(res_it, ChannelMatchWriter());

        new_channel_match_writer.channel_id() = mit->first;
        new_channel_match_writer.time() = time.tv_sec;
        new_channel_match_writer.matched_triggers() = mit->second.size();

        UserTriggerMatchContainer::Config::ChannelInfoMap::const_iterator
          all_ch_it = all_channels.find(mit->first);

        if(all_ch_it != all_channels.end())
        {
          UserTriggerMatchContainer::MatchedTriggerIdArray deactivated_triggers;

          // add noise triggers
          fill_noised_triggers(
            deactivated_triggers,
            new_channel_match_writer.positive_matches(),
            mit->second,
            (*(all_ch_it->second)).*channel_triggers_field,
            std::min(
              positive_group_size,
              ((*(all_ch_it->second)).*channel_triggers_field).size() > 0 ?
                ((*(all_ch_it->second)).*channel_triggers_field).size() - 1 : 0));

          // add negative triggers
          if(((*(all_ch_it->second)).*channel_triggers_field).size() > 1)
          {
            // compensate by all triggers if number of triggers in channel < negative_group_size
            fill_compensate_triggers(
              new_channel_match_writer.negative_matches(),
              (*(all_ch_it->second)).*channel_triggers_field,
              deactivated_triggers,
              mit->second.size(),
              std::min(
                negative_group_size,
                ((*(all_ch_it->second)).*channel_triggers_field).size()));
          }

          new_channel_match_writer.triggers_in_channel() =
            ((*(all_ch_it->second)).*channel_triggers_field).size() +
            deactivated_triggers.size();
        }
        else
        {
          // save triggers without noise and compensate if channel isn't active
          new_channel_match_writer.positive_matches().reserve(mit->second.size());
          std::copy(
            mit->second.begin(),
            mit->second.end(),
            std::back_inserter(new_channel_match_writer.positive_matches()));
          new_channel_match_writer.triggers_in_channel() = 0;
        }
      }
    }

    void add_matches(
      UserTriggerMatchWriter& profile_writer,
      const Generics::Time& request_time,
      const UserTriggerMatchContainer::MatchMap& page_matches,
      const UserTriggerMatchContainer::MatchMap& search_matches,
      const UserTriggerMatchContainer::MatchMap& url_matches,
      const UserTriggerMatchContainer::MatchMap& url_keyword_matches,
      const UserTriggerMatchContainer::Config::ChannelInfoMap& all_channels,
      unsigned long positive_group_size,
      unsigned long negative_group_size)
    {
      add_matches(
        profile_writer.page_matches(),
        page_matches,
        request_time,
        all_channels,
        &UserTriggerMatchContainer::Config::ChannelInfo::page_triggers,
        positive_group_size,
        negative_group_size);

      add_matches(
        profile_writer.search_matches(),
        search_matches,
        request_time,
        all_channels,
        &UserTriggerMatchContainer::Config::ChannelInfo::search_triggers,
        positive_group_size,
        negative_group_size);

      add_matches(
        profile_writer.url_matches(),
        url_matches,
        request_time,
        all_channels,
        &UserTriggerMatchContainer::Config::ChannelInfo::url_triggers,
        positive_group_size,
        negative_group_size);

      add_matches(
        profile_writer.url_keyword_matches(),
        url_keyword_matches,
        request_time,
        all_channels,
        &UserTriggerMatchContainer::Config::ChannelInfo::url_keyword_triggers,
        positive_group_size,
        negative_group_size);
    }

    void merge_matches(
      UserTriggerMatchWriter::page_matches_Container& res_matches,
      const UserTriggerMatchReader::page_matches_Container& new_matches)
    {
      if(!new_matches.empty())
      {
        UserTriggerMatchWriter::page_matches_Container merged_matches;

        std::merge(
          res_matches.begin(),
          res_matches.end(),
          new_matches.begin(),
          new_matches.end(),
          Algs::modify_inserter(
            std::back_inserter(merged_matches),
            UserTriggerMatchWriterAdapter()),
          UserTriggerMatchLess());

        res_matches.swap(merged_matches);
      }
    }

    void merge_matches(
      UserTriggerMatchWriter& profile_writer,
      const UserTriggerMatchReader& merge_profile_reader)
    {
      merge_matches(
        profile_writer.page_matches(),
        merge_profile_reader.page_matches());

      merge_matches(
        profile_writer.search_matches(),
        merge_profile_reader.search_matches());

      merge_matches(
        profile_writer.url_matches(),
        merge_profile_reader.url_matches());

      merge_matches(
        profile_writer.url_keyword_matches(),
        merge_profile_reader.url_keyword_matches());
    }

    void min_time_matches(
      Generics::Time& time,
      const UserTriggerMatchReader::page_matches_Container& matches)
    {
      for(UserTriggerMatchReader::page_matches_Container::const_iterator it =
            matches.begin();
          it != matches.end(); ++it)
      {
        time = (time == Generics::Time::ZERO ?
          Generics::Time((*it).time()) :
          std::min(time, Generics::Time((*it).time())));
      }
    }

    Generics::Time min_time_matches(
      const UserTriggerMatchReader& merge_profile_reader)
    {
      Generics::Time res;
      min_time_matches(res, merge_profile_reader.page_matches());
      min_time_matches(res, merge_profile_reader.search_matches());
      min_time_matches(res, merge_profile_reader.url_matches());
      min_time_matches(res, merge_profile_reader.url_keyword_matches());
      return res;
    }

    bool clear_excess_matches(
      UserTriggerMatchWriter::page_matches_Container& res_matches,
      const Generics::Time& time,
      const UserTriggerMatchContainer::Config::ChannelInfoMap& all_channels,
      unsigned long UserTriggerMatchContainer::Config::ChannelInfo::* min_visits_field,
      unsigned long UserTriggerMatchContainer::Config::ChannelInfo::* time_to_field,
      unsigned long max_trigger_visits)
    {
      bool changed = false;

      unsigned long time_int = time.tv_sec;

      // clear excess matches by time_to
      UserTriggerMatchWriter::page_matches_Container::iterator mit = res_matches.begin();

      while(mit != res_matches.end())
      {
        UserTriggerMatchWriter::page_matches_Container::iterator first_mit = mit;
        unsigned long match_count = 0;

        do
        {
          ++mit;
          ++match_count;
        }
        while(mit != res_matches.end() && mit->channel_id() == first_mit->channel_id());

        UserTriggerMatchContainer::Config::ChannelInfoMap::const_iterator
          all_ch_it = all_channels.find(first_mit->channel_id());

        unsigned long time_to;
        unsigned long min_visits;

        if(all_ch_it != all_channels.end())
        {
          min_visits = (*(all_ch_it->second)).*min_visits_field;
          if(max_trigger_visits > 0)
          {
            min_visits = std::min(max_trigger_visits, min_visits);
          }
          time_to = (*(all_ch_it->second)).*time_to_field;
        }
        else
        {
          // TODO: use default_channel_info_
          min_visits = 1;
          time_to = (Generics::Time::ONE_DAY * 60).tv_sec;
        }

        long erase_count = min_visits < match_count ?
          match_count - min_visits : 0;

        UserTriggerMatchWriter::page_matches_Container::iterator erase_mit = first_mit;
        while(erase_mit != mit && (
          erase_count > 0 ||
          erase_mit->time() + time_to < time_int))
        {
          --erase_count;
          ++erase_mit;
        }
        changed |= (first_mit != erase_mit);
        res_matches.erase(first_mit, erase_mit);
      }

      return changed;
    }

    bool clear_excess_matches(
      UserTriggerMatchWriter& profile_writer,
      const Generics::Time& time,
      const UserTriggerMatchContainer::Config::ChannelInfoMap& all_channels,
      unsigned long max_trigger_visits)
    {
      bool changed = clear_excess_matches(
        profile_writer.page_matches(),
        time,
        all_channels,
        &UserTriggerMatchContainer::Config::ChannelInfo::page_min_visits,
        &UserTriggerMatchContainer::Config::ChannelInfo::page_time_to,
        max_trigger_visits);

      changed |= clear_excess_matches(
        profile_writer.search_matches(),
        time,
        all_channels,
        &UserTriggerMatchContainer::Config::ChannelInfo::search_min_visits,
        &UserTriggerMatchContainer::Config::ChannelInfo::search_time_to,
        max_trigger_visits);

      changed |= clear_excess_matches(
        profile_writer.url_matches(),
        time,
        all_channels,
        &UserTriggerMatchContainer::Config::ChannelInfo::url_min_visits,
        &UserTriggerMatchContainer::Config::ChannelInfo::url_time_to,
        max_trigger_visits);

      changed |= clear_excess_matches(
        profile_writer.url_keyword_matches(),
        time,
        all_channels,
        &UserTriggerMatchContainer::Config::ChannelInfo::url_keyword_min_visits,
        &UserTriggerMatchContainer::Config::ChannelInfo::url_keyword_time_to,
        max_trigger_visits);

      return changed;
    }

    UserTriggerMatchReader::page_matches_Container::const_iterator
    move_for_channel_matches(
      unsigned long& count,
      UserTriggerMatchReader::page_matches_Container::const_iterator& pit,
      UserTriggerMatchReader::page_matches_Container::const_iterator pit_end,
      unsigned long channel_id,
      unsigned long min_visits)
    {
      count = 0;

      auto cur_pit = pit;

      while(cur_pit != pit_end && (*cur_pit).channel_id() == channel_id)
      {
        ++cur_pit;
        ++count;
      }

      if(count > min_visits)
      {
        // ignore excess matches at begin
        std::advance(pit, count - min_visits);
        count = min_visits;
      }

      return cur_pit;
    }

    void collect_matches(
      TriggerActionProcessor::MatchCountMap& res_matches,
      UserTriggerMatchReader::page_matches_Container::const_iterator pit_begin,
      UserTriggerMatchReader::page_matches_Container::const_iterator pit_end,
      unsigned long min_visits,
      const RevenueDecimal& weight)
    {
      if(min_visits > 0)
      {
        for(UserTriggerMatchReader::page_matches_Container::
              const_iterator pit = pit_begin;
            pit != pit_end; ++pit)
        {
          const unsigned positive_group_size = (*pit).positive_matches().size() /
            (*pit).matched_triggers();
          const unsigned negative_group_size = (*pit).negative_matches().size() /
            (*pit).matched_triggers();

          RevenueDecimal inc;

          if((*pit).triggers_in_channel() > 1)
          {
            inc = RevenueDecimal::div(
              RevenueDecimal((*pit).triggers_in_channel() - 1),
              RevenueDecimal::mul(
                RevenueDecimal((*pit).matched_triggers() * min_visits * (
                  (*pit).triggers_in_channel() - positive_group_size)),
                weight,
                Generics::DMR_FLOOR));
          }
          else
          {
            inc = RevenueDecimal::div(
              RevenueDecimal(false, 1, 0),
              RevenueDecimal::mul(weight, RevenueDecimal(min_visits),
              Generics::DMR_FLOOR));
          }

          TriggerActionProcessor::MatchCounterKey key;
          key.channel_id = (*pit).channel_id();

          for(ChannelMatchReader::positive_matches_Container::const_iterator mit =
                (*pit).positive_matches().begin();
              mit != (*pit).positive_matches().end(); ++mit)
          {
            key.channel_trigger_id = *mit;
            res_matches[key] += inc;
          }

          if(negative_group_size > 0)
          {
            RevenueDecimal dec(RevenueDecimal::div(
              RevenueDecimal((*pit).triggers_in_channel() * (positive_group_size - 1)),
              RevenueDecimal::mul(
                RevenueDecimal(
                  (*pit).matched_triggers() *
                  min_visits *
                  negative_group_size * (
                    (*pit).triggers_in_channel() - positive_group_size)),
                weight,
                Generics::DMR_FLOOR)));

            for(ChannelMatchReader::positive_matches_Container::const_iterator mit =
                  (*pit).negative_matches().begin();
                mit != (*pit).negative_matches().end(); ++mit)
            {
              key.channel_trigger_id = *mit;
              res_matches[key] -= dec;
            }
          }
        }
      }
    }

    void collect_matches(
      TriggerActionProcessor::MatchCountMap& res_page_matches,
      TriggerActionProcessor::MatchCountMap& res_search_matches,
      TriggerActionProcessor::MatchCountMap& res_url_matches,
      TriggerActionProcessor::MatchCountMap& res_url_keyword_matches,
      const UserTriggerMatchReader::page_matches_Container& page_matches,
      const UserTriggerMatchReader::search_matches_Container& search_matches,
      const UserTriggerMatchReader::url_matches_Container& url_matches,
      const UserTriggerMatchReader::url_keyword_matches_Container& url_keyword_matches,
      const ChannelIdSet& channels,
      const UserTriggerMatchContainer::Config::ChannelInfoMap& all_channels,
      const UserTriggerMatchContainer::Config::ChannelInfo* default_channel_info)
    {
      UserTriggerMatchReader::page_matches_Container::
        const_iterator pit = page_matches.begin();
      UserTriggerMatchReader::search_matches_Container::
        const_iterator sit = search_matches.begin();
      UserTriggerMatchReader::url_matches_Container::
        const_iterator uit = url_matches.begin();
      UserTriggerMatchReader::url_keyword_matches_Container::
        const_iterator ukit = url_keyword_matches.begin();

      for(ChannelIdSet::const_iterator ch_it = channels.begin();
          ch_it != channels.end(); ++ch_it)
      {
        // TODO advance pit, sit, uit for matches actual by time_to
        pit = std::lower_bound(pit, page_matches.end(), *ch_it, ChannelIdLess());
        sit = std::lower_bound(sit, search_matches.end(), *ch_it, ChannelIdLess());
        uit = std::lower_bound(uit, url_matches.end(), *ch_it, ChannelIdLess());
        ukit = std::lower_bound(ukit, url_keyword_matches.end(), *ch_it, ChannelIdLess());

        unsigned long page_count = 0;
        unsigned long search_count = 0;
        unsigned long url_count = 0;
        unsigned long url_keyword_count = 0;

        UserTriggerMatchContainer::Config::ChannelInfoMap::
          const_iterator all_ch_it = all_channels.find(*ch_it);

        const UserTriggerMatchContainer::Config::ChannelInfo& channel_info =
          all_ch_it != all_channels.end() ?
          *all_ch_it->second :
          *default_channel_info;

        UserTriggerMatchReader::page_matches_Container::const_iterator
          pit_end = move_for_channel_matches(
            page_count,
            pit,
            page_matches.end(),
            *ch_it,
            channel_info.page_min_visits);
        UserTriggerMatchReader::search_matches_Container::const_iterator
          sit_end = move_for_channel_matches(
            search_count,
            sit,
            search_matches.end(),
            *ch_it,
            channel_info.search_min_visits);
        UserTriggerMatchReader::url_matches_Container::const_iterator
          uit_end = move_for_channel_matches(
            url_count,
            uit,
            url_matches.end(),
            *ch_it,
            channel_info.url_min_visits);
        UserTriggerMatchReader::url_matches_Container::const_iterator
          ukit_end = move_for_channel_matches(
            url_keyword_count,
            ukit,
            url_keyword_matches.end(),
            *ch_it,
            channel_info.url_keyword_min_visits);

        const RevenueDecimal sum_weight =
          (channel_info.page_min_visits > 0 ?
             RevenueDecimal::div(
               RevenueDecimal(page_count),
               RevenueDecimal(channel_info.page_min_visits)) :
             RevenueDecimal::ZERO) +
          (channel_info.search_min_visits > 0 ?
             RevenueDecimal::div(
               RevenueDecimal(search_count),
               RevenueDecimal(channel_info.search_min_visits)) :
             RevenueDecimal::ZERO) +
          (channel_info.url_min_visits > 0 ?
             RevenueDecimal::div(
               RevenueDecimal(url_count),
               RevenueDecimal(channel_info.url_min_visits)) :
             RevenueDecimal::ZERO) +
          (channel_info.url_keyword_min_visits > 0 ?
             RevenueDecimal::div(
               RevenueDecimal(url_keyword_count),
               RevenueDecimal(channel_info.url_keyword_min_visits)) :
             RevenueDecimal::ZERO);

        collect_matches(
          res_page_matches,
          pit,
          pit_end,
          channel_info.page_min_visits,
          sum_weight);
        collect_matches(
          res_search_matches,
          sit,
          sit_end,
          channel_info.search_min_visits,
          sum_weight);
        collect_matches(
          res_url_matches,
          uit,
          uit_end,
          channel_info.url_min_visits,
          sum_weight);
        collect_matches(
          res_url_keyword_matches,
          ukit,
          ukit_end,
          channel_info.url_keyword_min_visits,
          sum_weight);
      }
    }

    void
    fill_match_counters(
      RequestTriggerMatchWriter::page_match_counters_Container& res,
      const TriggerActionProcessor::MatchCountMap& match_counters)
    {
      for(TriggerActionProcessor::MatchCountMap::const_iterator mit =
            match_counters.begin();
          mit != match_counters.end(); ++mit)
      {
        TriggerMatchCounterWriter match_counter_writer;
        match_counter_writer.channel_trigger_id() = mit->first.channel_trigger_id;
        match_counter_writer.channel_id() = mit->first.channel_id;
        match_counter_writer.counter() = mit->second.str();
        res.push_back(match_counter_writer);
      }
    }

    void
    convert_match_counters(
      TriggerActionProcessor::MatchCountMap& res_match_counters,
      const RequestTriggerMatchWriter::page_match_counters_Container& match_counters)
    {
      for(RequestTriggerMatchWriter::page_match_counters_Container::
            const_iterator mit = match_counters.begin();
          mit != match_counters.end(); ++mit)
      {
        res_match_counters[TriggerActionProcessor::MatchCounterKey(
          mit->channel_trigger_id(), mit->channel_id())] +=
            RevenueDecimal(mit->counter());
      }
    }

    void
    convert_match_counters(
      TriggerActionProcessor::TriggersMatchInfo& matches_info,
      const RequestTriggerMatchWriter& request_profile_writer)
    {
      convert_match_counters(
        matches_info.page_matches,
        request_profile_writer.page_match_counters());
      convert_match_counters(
        matches_info.search_matches,
        request_profile_writer.search_match_counters());
      convert_match_counters(
        matches_info.url_matches,
        request_profile_writer.url_match_counters());
      convert_match_counters(
        matches_info.url_keyword_matches,
        request_profile_writer.url_keyword_match_counters());
    }

    void
    clear_expired_impressions(
      UserTriggerMatchWriter::impressions_Container& impressions,
      const Generics::Time& last_request_time)
    {
      UserTriggerMatchWriter::impressions_Container::reverse_iterator erase_end_it =
        impressions.rbegin();
      unsigned long i = 0;
      while(erase_end_it != impressions.rend() &&
        IMPRESSION_EXPIRE_TIME + erase_end_it->time() > last_request_time &&
        i < MAX_IMPRESSIONS_KEEP)
      {
        ++erase_end_it;
        ++i;
      }
      impressions.erase(impressions.begin(), erase_end_it.base());
    }

    void
    filter_matches(
      UserTriggerMatchContainer::MatchMap& res_matches,
      const UserTriggerMatchContainer::Config::ChannelInfoMap& all_channels,
      const UserTriggerMatchContainer::MatchMap& matches)
      noexcept
    {
      for(UserTriggerMatchContainer::MatchMap::const_iterator mit =
            matches.begin();
          mit != matches.end(); ++mit)
      {
        if(all_channels.find(mit->first) != all_channels.end())
        {
          res_matches.insert(*mit);
        }
      }
    }
  }

  UserTriggerMatchContainer::UserTriggerMatchContainer(
    Logging::Logger* logger,
    TriggerActionProcessor* processor,
    UserTriggerMatchProfileProvider* user_profile_provider,
    unsigned long common_chunks_number,
    const AdServer::ProfilingCommons::ProfileMapFactory::ChunkPathMap& chunk_folders,
    const char* user_file_prefix,
    const char* request_file_base_path,
    const char* request_file_prefix,
    unsigned long positive_triggers_group_size,
    unsigned long negative_triggers_group_size,
    unsigned long max_trigger_visits,
    ProfilingCommons::ProfileMapFactory::Cache* /*cache*/,
    const AdServer::ProfilingCommons::LevelMapTraits& user_level_map_traits,
    const AdServer::ProfilingCommons::LevelMapTraits& request_level_map_traits)
    /*throw(Exception)*/
    : logger_(ReferenceCounting::add_ref(logger)),
      processor_(ReferenceCounting::add_ref(processor)),
      merge_profile_provider_(ReferenceCounting::add_ref(user_profile_provider)),
      positive_triggers_group_size_(positive_triggers_group_size),
      negative_triggers_group_size_(negative_triggers_group_size),
      max_trigger_visits_(max_trigger_visits)
  {
    static const char* FUN =
      "UserTriggerMatchContainer::UserTriggerMatchContainer()";

    typedef AdServer::ProfilingCommons::OptionalProfileAdapter<UserTriggerMatchProfileAdapter>
      AdaptUserTriggerMatchProfile;

    default_channel_info_ = new Config::ChannelInfo();
    default_channel_info_->page_min_visits = 1;
    default_channel_info_->search_min_visits = 1;
    default_channel_info_->url_min_visits = 1;
    default_channel_info_->url_keyword_min_visits = 1;

    try
    {
      user_map_ = AdServer::ProfilingCommons::ProfileMapFactory::
        open_chunked_map<
          AdServer::Commons::UserId,
          AdServer::ProfilingCommons::UserIdAccessor,
          unsigned long (*)(const Generics::Uuid& uuid),
          AdaptUserTriggerMatchProfile>(
            common_chunks_number,
            chunk_folders,
            user_file_prefix,
            user_level_map_traits,
            *this,
            Generics::ActiveObjectCallback_var(
              new Logging::ActiveObjectCallbackImpl(
                logger_,
                "UserTriggerMatchContainer",
                "ExpressionMatcher",
                "ADS-IMPL-4024")),
            AdServer::Commons::uuid_distribution_hash,
            nullptr // file controller
            );
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't init UserProfileMap: " << ex.what();
      throw Exception(ostr);
    }

    if(request_file_prefix != 0)
    {
      try
      {
        Generics::ActiveObject_var active_object;

        request_map_ = AdServer::ProfilingCommons::ProfileMapFactory::
          open_adapt_transaction_level_map<
            AdServer::Commons::RequestId,
            AdServer::ProfilingCommons::RequestIdAccessor,
            RequestTriggerMatchProfileAdapter>(
              active_object,
              Generics::ActiveObjectCallback_var(
                new Logging::ActiveObjectCallbackImpl(
                  logger_,
                  "UserTriggerMatchContainer",
                  "ExpressionMatcher",
                  "ADS-IMPL-4024")),
              request_file_base_path,
              request_file_prefix,
              request_level_map_traits,
              RequestTriggerMatchProfileAdapter());

        add_child_object(active_object);
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": Can't init RequestProfileMap: " << ex.what();
        throw Exception(ostr);
      }
    }
  }

  UserTriggerMatchContainer::~UserTriggerMatchContainer() noexcept
  {}

  UserTriggerMatchContainer::Config_var
  UserTriggerMatchContainer::config() const noexcept
  {
    return config_.get();
  }

  void
  UserTriggerMatchContainer::config(Config* config) noexcept
  {
    config_ = ReferenceCounting::add_ref(config);
  }

  Generics::ConstSmartMemBuf_var
  UserTriggerMatchContainer::get_user_profile(
    const AdServer::Commons::UserId& user_id)
    /*throw(Exception)*/
  {
    static const char* FUN = "UserTriggerMatchContainer::get_user_profile()";

    try
    {
      return user_map_->get_profile(user_id);
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't get profile. Caught eh::Exception: " << e.what();
      throw Exception(ostr);
    }
  }

  Generics::ConstSmartMemBuf_var
  UserTriggerMatchContainer::get_request_profile(
    const AdServer::Commons::RequestId& request_id)
    /*throw(Exception)*/
  {
    static const char* FUN = "UserTriggerMatchContainer::get_request_profile()";

    if(!request_map_.in())
    {
      Stream::Error ostr;
      ostr << FUN << ": No request profiles mode used";
      throw Exception(ostr);
    }

    try
    {
      return request_map_->get_profile(request_id);
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't get profile. Caught eh::Exception: " << e.what();
      throw Exception(ostr);
    }
  }

  void
  UserTriggerMatchContainer::clear_expired() /*throw(Exception)*/
  {
    Generics::Time now = Generics::Time::get_time_of_day();
    user_map_->clear_expired(now - user_expire_time_);
    if(request_map_.in())
    {
      request_map_->clear_expired(now - request_expire_time_);
    }
  }

  void
  UserTriggerMatchContainer::process_request(
    const RequestInfo& request_info)
    /*throw(NotReady, Exception)*/
  {
    /* save new matches, clear excess matches (by min visits) */
    static const char* FUN = "UserTriggerMatchContainer::process_request()";

    if(request_info.page_matches.empty() &&
       request_info.search_matches.empty() &&
       request_info.url_matches.empty() &&
       request_info.url_keyword_matches.empty() &&
       request_info.merge_user_id.is_null())
    {
      return;
    }

    try
    {
      TriggersMatchInfoList delegate_imps;
      TriggersMatchInfoList delegate_clicks;

      process_request_trans_(
        delegate_imps,
        delegate_clicks,
        request_info);

      for(TriggersMatchInfoList::const_iterator imp_it =
            delegate_imps.begin();
          imp_it != delegate_imps.end(); ++imp_it)
      {
        processor_->process_triggers_impression(*imp_it);
      }

      for(TriggersMatchInfoList::const_iterator click_it =
            delegate_clicks.begin();
          click_it != delegate_clicks.end(); ++click_it)
      {
        processor_->process_triggers_click(*click_it);
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": user_id = '" << request_info.user_id.to_string() <<
        "'. Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  void
  UserTriggerMatchContainer::process_impression(
    const ImpressionInfo& imp_info)
    /*throw(NotReady, Exception)*/
  {
    static const char* FUN = "UserTriggerMatchContainer::process_impression()";

    try
    {
      bool delegate_impression;
      bool delegate_click;
      TriggerActionProcessor::TriggersMatchInfo triggers_match_info;

      process_impression_trans_(
        delegate_impression,
        delegate_click,
        triggers_match_info,
        imp_info);

      if(delegate_impression)
      {
        processor_->process_triggers_impression(triggers_match_info);
      }

      if(delegate_click)
      {
        processor_->process_triggers_click(triggers_match_info);
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": user_id = '" << imp_info.user_id.to_string() <<
        "'. Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  void
  UserTriggerMatchContainer::process_click(
    const Commons::RequestId& request_id,
    const Generics::Time& time)
    /*throw(Exception)*/
  {
    static const char* FUN = "UserTriggerMatchContainer::process_click()";

    try
    {
      bool delegate_click;
      TriggerActionProcessor::TriggersMatchInfo triggers_match_info;
      process_click_trans_(
        delegate_click,
        triggers_match_info,
        request_id,
        time);

      if(delegate_click)
      {
        processor_->process_triggers_click(triggers_match_info);
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": request_id = '" << request_id.to_string() <<
        "'. Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  UserTriggerMatchContainer::Config_var
  UserTriggerMatchContainer::current_config_() const /*throw(NotReady)*/
  {
    UserTriggerMatchContainer::Config_var res = config();

    if(!res.in())
    {
      throw NotReady("channels config isn't defined");
    }

    return res;
  }

  template<typename TransactionType, typename ProfileWriterType>
  void
  UserTriggerMatchContainer::save_profile_(
    TransactionType* transaction,
    const ProfileWriterType& profile_writer,
    const Generics::Time& time)
    /*throw(eh::Exception)*/
  {
    const unsigned long profile_size = profile_writer.size();

    Generics::SmartMemBuf_var new_mem_buf(
      new Generics::SmartMemBuf(profile_size));

    profile_writer.save(new_mem_buf->membuf().data(), profile_size);
    transaction->save_profile(Generics::transfer_membuf(new_mem_buf), time);
  }

  void
  UserTriggerMatchContainer::process_request_trans_(
    TriggersMatchInfoList& delegate_imps,
    TriggersMatchInfoList& delegate_clicks,
    const RequestInfo& request_info)
    /*throw(Exception)*/
  {
    /* save new matches, clear excess matches (by min visits) */
    static const char* FUN = "UserTriggerMatchContainer::process_request_trans_()";

    try
    {
      Generics::ConstSmartMemBuf_var temp_mem_buf;

      if(!request_info.merge_user_id.is_null())
      {
        // merge temporary profile
        try
        {
          temp_mem_buf =
            merge_profile_provider_->get_user_profile(request_info.merge_user_id);
        }
        catch(const eh::Exception& ex)
        {
          Stream::Error ostr;
          ostr << FUN << ": can't get merge profile: " << ex.what();
          logger_->log(ostr.str(),
            Logging::Logger::WARNING,
            Aspect::USER_TRIGGER_MATCH_CONTAINER,
            "ADS-IMPL-4022");
        }
      }

      UserProfileMap::Transaction_var transaction =
        user_map_->get_transaction(request_info.user_id);

      Generics::ConstSmartMemBuf_var mem_buf = transaction->get_profile();

      UserTriggerMatchWriter user_profile_writer;

      if(mem_buf.in())
      {
        user_profile_writer.init(
          mem_buf->membuf().data(),
          mem_buf->membuf().size());
      }
      else
      {
        user_profile_writer.version() = CURRENT_USER_TRIGGER_MATCH_PROFILE_VERSION;
      }

      Generics::Time merged_match_min_time;

      if(temp_mem_buf.in())
      {
        UserTriggerMatchReader merge_user_profile_reader(
          temp_mem_buf->membuf().data(),
          temp_mem_buf->membuf().size());

        merge_matches(
          user_profile_writer,
          merge_user_profile_reader);

        merged_match_min_time = min_time_matches(merge_user_profile_reader);
      }

      {
        Config_var current_config = current_config_();

        add_matches(
          user_profile_writer,
          request_info.time,
          request_info.page_matches,
          request_info.search_matches,
          request_info.url_matches,
          request_info.url_keyword_matches,
          current_config->channels,
          positive_triggers_group_size_,
          negative_triggers_group_size_);

        clear_excess_matches(
          user_profile_writer,
          request_info.time,
          current_config->channels,
          max_trigger_visits_);
      }

      clear_expired_impressions(
        user_profile_writer.impressions(),
        request_info.time);

      unsigned long user_profile_size = user_profile_writer.size();

      Generics::SmartMemBuf_var new_user_mem_buf(
        new Generics::SmartMemBuf(user_profile_size));

      user_profile_writer.save(
        new_user_mem_buf->membuf().data(), user_profile_size);

      if(request_map_.in())
      {
        Generics::Time min_time_for_recheck =
          merged_match_min_time == Generics::Time::ZERO ?
          request_info.time :
          std::min(request_info.time, merged_match_min_time);

        UserTriggerMatchWriter::impressions_Container::const_iterator req_it =
          std::lower_bound(
            user_profile_writer.impressions().begin(),
            user_profile_writer.impressions().end(),
            min_time_for_recheck,
            TimeLess());

        if(req_it != user_profile_writer.impressions().end())
        {
          UserTriggerMatchReader user_profile_reader(
            new_user_mem_buf->membuf().data(),
            new_user_mem_buf->membuf().size());

          for(UserTriggerMatchWriter::impressions_Container::
                const_iterator imp_it = req_it;
              imp_it != user_profile_writer.impressions().end(); ++imp_it)
          {
            RequestProfileMap::Transaction_var change_request_trans =
              request_map_->get_transaction(Commons::RequestId((*imp_it).request_id()));
            Generics::ConstSmartMemBuf_var change_request_mem_buf =
              change_request_trans->get_profile();

            if(change_request_mem_buf.in())
            {
              RequestTriggerMatchWriter change_request_profile_writer(
                change_request_mem_buf->membuf().data(),
                change_request_mem_buf->membuf().size());

              Config_var current_config = current_config_();

              if(change_request_profile_writer.impression_done())
              {
                // do impression rollback
                delegate_imps.push_back(TriggerActionProcessor::TriggersMatchInfo());
                TriggerActionProcessor::TriggersMatchInfo& revert_match_info =
                  delegate_imps.back();
                revert_match_info.time =
                  Generics::Time(change_request_profile_writer.time());
                convert_match_counters(
                  revert_match_info,
                  change_request_profile_writer);
                revert_match_info.page_matches.negate();
                revert_match_info.search_matches.negate();
                revert_match_info.url_matches.negate();
                revert_match_info.url_keyword_matches.negate();

                if(change_request_profile_writer.click_done())
                {
                  // do click rollback
                  delegate_clicks.push_back(revert_match_info);
                }
              }

              change_request_profile_writer.page_match_counters().clear();
              change_request_profile_writer.search_match_counters().clear();
              change_request_profile_writer.url_match_counters().clear();
              change_request_profile_writer.url_keyword_match_counters().clear();

              ChannelIdSet channels;
              std::copy(
                change_request_profile_writer.channels().begin(),
                change_request_profile_writer.channels().end(),
                std::inserter(channels, channels.begin()));

              // save new trigger matches into request profile
              TriggerActionProcessor::TriggersMatchInfo replace_match_info;
              replace_match_info.time =
                Generics::Time(change_request_profile_writer.time());

              collect_matches(
                replace_match_info.page_matches,
                replace_match_info.search_matches,
                replace_match_info.url_matches,
                replace_match_info.url_keyword_matches,
                user_profile_reader.page_matches(),
                user_profile_reader.search_matches(),
                user_profile_reader.url_matches(),
                user_profile_reader.url_keyword_matches(),
                channels,
                current_config->channels,
                default_channel_info_);

              delegate_imps.push_back(replace_match_info);
              if(change_request_profile_writer.click_done())
              {
                delegate_clicks.push_back(replace_match_info);
              }

              fill_match_counters(
                change_request_profile_writer.page_match_counters(),
                replace_match_info.page_matches);
              fill_match_counters(
                change_request_profile_writer.search_match_counters(),
                replace_match_info.search_matches);
              fill_match_counters(
                change_request_profile_writer.url_match_counters(),
                replace_match_info.url_matches);
              fill_match_counters(
                change_request_profile_writer.url_keyword_match_counters(),
                replace_match_info.url_keyword_matches);

              save_profile_(
                change_request_trans.in(),
                change_request_profile_writer,
                request_info.time);
            }
          }
        }
      }

      transaction->save_profile(
        Generics::transfer_membuf(new_user_mem_buf),
        request_info.time);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": user_id = '" << request_info.user_id.to_string() <<
        "'. Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  void
  UserTriggerMatchContainer::process_impression_trans_(
    bool& delegate_impression,
    bool& delegate_click,
    TriggerActionProcessor::TriggersMatchInfo& imp_matches_info,
    const ImpressionInfo& imp_info)
    /*throw(Exception)*/
  {
    static const char* FUN = "UserTriggerMatchContainer::process_impression_trans_()";

    if(!request_map_.in())
    {
      Stream::Error ostr;
      ostr << FUN << ": No request profiles mode used";
      throw Exception(ostr);
    }

    try
    {
      delegate_impression = false;
      delegate_click = false;

      UserProfileMap::Transaction_var user_trans = user_map_->get_transaction(
        imp_info.user_id);
      RequestProfileMap::Transaction_var request_trans = request_map_->get_transaction(
        imp_info.request_id);
      Generics::ConstSmartMemBuf_var request_mem_buf = request_trans->get_profile();

      if(request_mem_buf.in())
      {
        RequestTriggerMatchReader request_profile_reader(
          request_mem_buf->membuf().data(),
          request_mem_buf->membuf().size());
        if(request_profile_reader.impression_done())
        {
          // double impression
          return;
        }
      }

      Generics::ConstSmartMemBuf_var user_mem_buf = user_trans->get_profile();
      UserTriggerMatchWriter user_profile_writer;

      // collect matches
      if(user_mem_buf.in())
      {
        user_profile_writer.init(
          user_mem_buf->membuf().data(),
          user_mem_buf->membuf().size());

        Config_var current_config = current_config_();

        const bool changed = clear_excess_matches(
          user_profile_writer,
          imp_info.time,
          current_config->channels,
          max_trigger_visits_);

        if(changed)
        {
          user_mem_buf = Algs::save_to_membuf(user_profile_writer);
        }

        UserTriggerMatchReader profile_reader(
          user_mem_buf->membuf().data(),
          user_mem_buf->membuf().size());

        imp_matches_info.time = imp_info.time;

        collect_matches(
          imp_matches_info.page_matches,
          imp_matches_info.search_matches,
          imp_matches_info.url_matches,
          imp_matches_info.url_keyword_matches,
          profile_reader.page_matches(),
          profile_reader.search_matches(),
          profile_reader.url_matches(),
          profile_reader.url_keyword_matches(),
          imp_info.channels,
          current_config->channels,
          default_channel_info_);

        delegate_impression = true;
      }
      else
      {
        user_profile_writer.version() = CURRENT_USER_TRIGGER_MATCH_PROFILE_VERSION;
      }

      // save request profile
      RequestTriggerMatchWriter request_profile_writer;
      if(request_mem_buf.in())
      {
        request_profile_writer.init(
          request_mem_buf->membuf().data(),
          request_mem_buf->membuf().size());

        if(!request_profile_writer.impression_done() &&
           request_profile_writer.click_done())
        {
          delegate_click = true;
        }
      }
      else
      {
        request_profile_writer.version() = CURRENT_REQUEST_TRIGGER_MATCH_PROFILE_VERSION;
        request_profile_writer.click_done() = 0;
      }

      request_profile_writer.impression_done() = 1;

      request_profile_writer.time() = imp_info.time.tv_sec;
      request_profile_writer.channels().reserve(imp_info.channels.size());
      std::copy(
        imp_info.channels.begin(),
        imp_info.channels.end(),
        std::back_inserter(request_profile_writer.channels()));
      fill_match_counters(
        request_profile_writer.page_match_counters(),
        imp_matches_info.page_matches);
      fill_match_counters(
        request_profile_writer.search_match_counters(),
        imp_matches_info.search_matches);
      fill_match_counters(
        request_profile_writer.url_match_counters(),
        imp_matches_info.url_matches);
      fill_match_counters(
        request_profile_writer.url_keyword_match_counters(),
        imp_matches_info.url_keyword_matches);

      // save request profile
      save_profile_(request_trans.in(), request_profile_writer, imp_info.time);

      // save request marker for user
      clear_expired_impressions(
        user_profile_writer.impressions(),
        imp_info.time);

      ImpressionMarkerWriter new_impression;
      new_impression.time() = imp_info.time.tv_sec;
      new_impression.request_id() = imp_info.request_id.to_string();
      UserTriggerMatchWriter::impressions_Container::iterator ins_it =
        std::lower_bound(
          user_profile_writer.impressions().begin(),
          user_profile_writer.impressions().end(),
          imp_info.time,
          TimeLess());

      user_profile_writer.impressions().insert(ins_it, new_impression);

      save_profile_(user_trans.in(), user_profile_writer, imp_info.time);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": user_id = '" << imp_info.user_id.to_string() <<
        "'. Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  void
  UserTriggerMatchContainer::process_click_trans_(
    bool& delegate_click,
    TriggerActionProcessor::TriggersMatchInfo& click_matches_info,
    const Commons::RequestId& request_id,
    const Generics::Time& time)
    /*throw(Exception)*/
  {
    static const char* FUN = "UserTriggerMatchContainer::process_click_trans_()";

    if(!request_map_.in())
    {
      Stream::Error ostr;
      ostr << FUN << ": No request profiles mode used";
      throw Exception(ostr);
    }

    try
    {
      RequestProfileMap::Transaction_var request_trans =
        request_map_->get_transaction(request_id);
      Generics::ConstSmartMemBuf_var request_mem_buf =
        request_trans->get_profile();

      if(request_mem_buf.in())
      {
        RequestTriggerMatchReader request_profile_reader(
          request_mem_buf->membuf().data(),
          request_mem_buf->membuf().size());
        if(request_profile_reader.click_done())
        {
          // double click
          delegate_click = false;
          return;
        }
      }

      delegate_click = true;

      // save request profile
      RequestTriggerMatchWriter request_profile_writer;

      if(request_mem_buf.in())
      {
        request_profile_writer.init(
          request_mem_buf->membuf().data(),
          request_mem_buf->membuf().size());

        click_matches_info.time =
          Generics::Time(request_profile_writer.time());
        convert_match_counters(
          click_matches_info.page_matches,
          request_profile_writer.page_match_counters());
        convert_match_counters(
          click_matches_info.search_matches,
          request_profile_writer.search_match_counters());
        convert_match_counters(
          click_matches_info.url_matches,
          request_profile_writer.url_match_counters());
        convert_match_counters(
          click_matches_info.url_keyword_matches,
          request_profile_writer.url_keyword_match_counters());
      }
      else
      {
        request_profile_writer.version() = CURRENT_REQUEST_TRIGGER_MATCH_PROFILE_VERSION;
        request_profile_writer.impression_done() = 0;
        delegate_click = false;
      }

      request_profile_writer.click_done() = 1;

      save_profile_(request_trans.in(), request_profile_writer, time);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": request_id = '" << request_id.to_string() <<
        "'. Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }
}
}
