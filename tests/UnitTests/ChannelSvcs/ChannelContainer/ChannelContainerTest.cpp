
#include <sstream>
#include <getopt.h>
#include <Generics/Time.hpp>
#include<Language/BLogic/NormalizeTrigger.hpp>
#include <String/Tokenizer.hpp>
#include <HTTP/UrlAddress.hpp>
#include <Commons/CorbaAlgs.hpp>
#include <ChannelSvcs/ChannelServer/ChannelContainer.hpp>
#include <ChannelSvcs/ChannelServer/UpdateContainer.hpp>
#include <ChannelSvcs/ChannelCommons/ChannelUtils.hpp>
#include "ChannelContainerTest.hpp"

namespace AdServer
{
  namespace Testing
  {
    using namespace AdServer::ChannelSvcs;

    template<typename OUTTYPE>
    void read_number(
      const char* in,
      OUTTYPE min_value,
      OUTTYPE max_value,
      OUTTYPE &value) noexcept
    {
      OUTTYPE converted;
      std::stringstream convert;
      convert << in;
      convert >> converted;
      if(converted >= min_value && converted <= max_value)
      {
        value = converted;
      }
    }

    ChannelContainerTest::TestNames ChannelContainerTest::test_names_[] =
    {
      {"constructor", TC_CONSTRUCTOR},
      {"add_trigger", TC_ADD_TRIGGER},
      {"merge", TC_MERGE},
      {"match", TC_MATCH},
      {"update", TC_UPDATE},
      {"uids", TC_UIDS}
    };

    ChannelContainerTest::ChannelContainerTest() noexcept
      : cases_(TC_NONE),
        count_chunks_(32),
        count_triggers_(30),
        count_channels_(100),
        count_uids_(10000),
        verbose_(0)
    {
    }

    int ChannelContainerTest::check_constructor_() noexcept
    {
      const char* FN = "ChannelContainerTest::check_constructor_:";
      try
      {
        log_action_(" started.", 0, FN);
        ContPtr first;
        first.reset(new ChannelContainer(count_chunks_));
        log_action_("Container was created");
        ChannelServerStats stats;
        first->get_stats(stats);
        log_action_("Container stats was gotten");
        log_action_(" finished.", 0, FN);
        return 0;
      }
      catch(const eh::Exception& e)
      {
        std::cerr << FN << "caught eh::Exception " << e.what() << std::endl;
      }
      return 1;
    }

    int ChannelContainerTest::check_add_trigger_() noexcept
    {
      const char* FN = "ChannelContainerTest::check_add_trigger_:";
      try
      {
        log_action_(" started.", 0, FN);
        UpdPtr cont;
        ContPtr base;
        base.reset(new ChannelContainer(count_chunks_));
        cont.reset(new UpdateContainer(base.get(), 0));
        ChannelSvcs::ChannelContainerBase::ChannelMap buffer;
        log_action_("Container was created");
        create_data_for_container_(
          10, 10 + count_triggers_, cont.get(), 0, &buffer);
        {
          std::ostringstream ostr;
          ostr << count_triggers_ << " trigger lists were added to container";
          log_action_(ostr.str());
        }
        ChannelSvcs::ChannelIdToMatchInfo_var info =
          new ChannelSvcs::ChannelIdToMatchInfo;
        base->merge(*cont, *info, true); 
        base->fill(buffer);
        log_action_("Buffer was filled");
        check_data_of_container_(10, 10 + count_triggers_, buffer, FN);
        log_action_(" finished.", 0, FN);
        return 0;
      }
      catch(const ErrorDescriptor& e)
      {
        std::cerr << FN << ":ERROR:" << e.what() << std::endl;
      }
      catch(const eh::Exception& e)
      {
        std::cerr << FN << ":caught eh::Exception " << e.what() << std::endl;
      }
      return 1;
    }

    int ChannelContainerTest::check_merge_() noexcept
    {
      const char* FN = "ChannelContainerTest::check_merge_:";
      try
      {
        log_action_(" started.", 0, FN);
        UpdPtr cont;
        ContPtr base;
        base.reset(new ChannelContainer(count_chunks_));
        cont.reset(new UpdateContainer(base.get(), 0));
        ChannelSvcs::ChannelContainerBase::ChannelMap buffer, empty;
        log_action_("1.Container was created");
        create_data_for_container_(
          10, 10 + count_triggers_, cont.get(), "old_", &empty);
        ChannelSvcs::ChannelIdToMatchInfo_var info =
          new ChannelSvcs::ChannelIdToMatchInfo;
        base->merge(*cont, *info, true); 
        log_action_("1.Container was merged");
        buffer = empty;
        base->fill(buffer);
        log_action_("1.Buffer was filled");
        check_data_of_container_(10, 10 + count_triggers_, buffer, FN, "old_");
        unsigned long half_triggers = count_triggers_ / 2;
        create_data_for_container_(
          10, 10 + half_triggers, cont.get(), "new_");
        base->merge(*cont, *info, true); 
        log_action_("2.Container was merged");
        buffer = empty;
        base->fill(buffer);
        log_action_("2.Buffer was filled");
        check_data_of_container_(10, 10 + half_triggers, buffer, FN, "new_");
        check_data_of_container_(
          10 + half_triggers, 10 + count_triggers_, buffer, FN, "old_");
        create_data_for_container_(
          10 + half_triggers, 10 + count_triggers_, cont.get(), "new_");
        base->merge(*cont, *info, true); 
        log_action_("3.Container was merged");
        buffer = empty;
        base->fill(buffer);
        log_action_("3.Buffer was filled");
        check_data_of_container_(10, 10 + count_triggers_, buffer, FN, "new_");
        log_action_(" finished.", 0, FN);
        return 0;
      }
      catch(const ErrorDescriptor& e)
      {
        std::cerr << e.what();
      }
      catch(const eh::Exception& e)
      {
        std::cerr << FN << "caught eh::Exception " << e.what() << std::endl;
      }
      return 1;
    }

    int ChannelContainerTest::check_match_() noexcept
    {
      const char* FN = "ChannelContainerTest::check_match_:";
      try
      {
        IdToTrigger triggers[] =
        {
          {11, "simple_hard", false, false},
          {12, "\"complex hard\"", false, false},
          {13, "simple soft", false, false},
          {14, "\"complex soft\" second", false, false},
          {15, "www.int.ua", true, false},
          {16, "www.int.ua/path", true, false}
        };
        IdToTrigger match_cases[] =
        {
          {11, "simple_hard", false, true},
          {12, "complex hard", false, true},
          {12, "hard complex", false, false},
          {13, "simple soft", false, true},
          {13, "simple soft case", false, true},
          {13, "simple case", false, false},
          {14, "complex soft second", false, true},
          {14, "soft complex second", false, false},
          {15, "www.int.ua", true, true},
          {16, "www.int.ua", true, false},
          {15, "www.int.ua/path", true, true},
          {16, "www.int.ua/path", true, true},
          {15, "www.int.ua/path/p", true, true},
          {16, "www.int.ua/path/p", true, true},
          {15, "www.int.ua/p", true, true},
          {16, "www.int.ua/p", true, false}
        };
        const size_t size_match = sizeof(match_cases)/sizeof(match_cases[0]);
        log_action_(" started.", 0, FN);
        ContPtr base(new ChannelContainer(count_chunks_));
        log_action_("Container was created");
        init_triggers_(
          triggers,
          sizeof(triggers)/sizeof(triggers[0]),
          *base);
        std::set<unsigned short> ports;
        for(size_t j = 0; j < size_match; j++)
        {
          AdServer::ChannelSvcs::MatchWords phrases[CT_MAX];
          MatchUrls url_words;
          TriggerMatchRes res;
          if(match_cases[j].flag)//url
          {
            HTTP::BrowserAddress url(String::SubString(match_cases[j].trigger));
            ChannelContainer::match_parse_refer(
              url.url(),
              ports,
              false, //non strict
              url_words,
              0);
          }
          else
          {
            ChannelSvcs::parse_keywords<ChannelSvcs::MatchBreakSeparators>(
              String::SubString(match_cases[j].trigger), phrases[CT_PAGE],
              PM_SIMPLIFY);
            std::ostringstream debug;
            debug << "Match words = ";
            for(AdServer::ChannelSvcs::MatchWords::const_iterator it =
                phrases[CT_PAGE].begin(); it != phrases[CT_PAGE].end(); ++it)
            {
              if(it != phrases[CT_PAGE].begin())
              {
                debug << ',';
              }
              const String::SubString& value = it->text();
              debug << value;
            }
            debug << ".";
            log_action_(debug.str(), 3);
          }
         // ostr << "Check trigger '" << match_cases[j].trigger << '\'';
         //log_action_(ostr.str(), 2);
          base->match(
            url_words,
            ChannelSvcs::MatchUrls(), 
            phrases,
            ChannelSvcs::MatchWords(), 
            ChannelSvcs::StringVector(),
            Generics::Uuid(),
            MF_ACTIVE, res);
          bool pass = false;
          //size_t index = match_cases[j].flag ? CT_URL : CT_PAGE;

          if(res.find(match_cases[j].id) != res.end())
          {
            pass = true;
          }
          if(pass != match_cases[j].match)
          {
            Stream::Error err;
            err << FN << "BUG: atom id = " << match_cases[j].id <<
              " for trigger = '" << match_cases[j].trigger << "' ";
            if(res.empty())
            {
              err << " no channels matched.";
              /*
              if(verbose_ >=3)
              {
                err << "Match reslut: ";
                size_t index = match_cases[j].flag ? CT_URL : CT_PAGE;
                for(TriggerMatchRes::const_iterator trigger_iter = res.begin();
                    trigger_iter != res.end(); trigger_iter++)
                {
                  err << '\'' << trigger_iter->first << '\'' << " => { ";
                  const TriggerMatchItem& item = trigger_iter->second; 
                  for(TriggerMatchItem::value_type::const_iterator it = item.channel_ids[index].begin();
                      it != item.channel_ids[index].end(); ++it)
                  {
                    err << it->channel_id << '(' << it->trigger_channel_id << ") ";
                  }
                  err << "} ";
                }
              }*/
            }
            else
            {
              for(auto it = res.begin(); it != res.end(); ++it)
              {
                if(it != res.begin())
                {
                  err << ',';
                }
                err << it->first;
              }
              err << '.';
            }
            err << std::endl;
            throw ErrorDescriptor(err);
          }
          else
          {
            log_action_(" PASSED.", 2, FN);
          }
        }
        log_action_(" finished.", 0, FN);
        return 0;
      }
      catch(const ErrorDescriptor& e)
      {
        std::cerr << e.what();
      }
      catch(const eh::Exception& e)
      {
        std::cerr << FN << "caught eh::Exception " << e.what() << std::endl;
      }
      return 1;
    }

    void ChannelContainerTest::init_triggers_(
      const IdToTrigger triggers[],
      size_t size_triggers,
      ChannelSvcs::ChannelContainer& base,
      unsigned int offset) const
      /*throw(eh::Exception)*/
    {
      UpdateContainer cont(&base, 0);
      ChannelSvcs::ChannelIdToMatchInfo_var info = 
        new ChannelSvcs::ChannelIdToMatchInfo;
      std::set<unsigned short> ports;
      std::map<unsigned long, MergeAtom> atoms;
      for(size_t i = 0; i < size_triggers; i++)
      {
        {
          std::ostringstream ostr;
          ostr << "Generating atom number " << triggers[i].id;
          log_action_(ostr.str(), 2);
        }
        MergeAtom& atom = atoms[triggers[i].id];
        atom.id = triggers[i].id;

        MatchInfo& m_info = (*info)[triggers[i].id];
        m_info.stamp = Generics::Time::get_time_of_day();
        m_info.channel = Channel(triggers[i].id);
        if(triggers[i].flag)//url
        {
          m_info.channel.mark_type(CT_URL);
        }
        else
        {
          m_info.channel.mark_type(CT_PAGE);
        }
          m_info.channel.mark_type(Channel::CT_ACTIVE);
        if(triggers[i].flag)
        {
          TriggerParser::TriggerParser::parse_url(
            atom.id,
            triggers[i].id * (i + 1) + offset, //channel_trigger_id
            triggers[i].trigger, false, ports, atom.soft_words, 0);
        }
        else
        {
          TriggerParser::TriggerParser::parse_word(
            atom.id,
            triggers[i].id * (i + 1) + offset, //channel_trigger_id
            'P',
            triggers[i].trigger,
            false,
            0, atom.soft_words, Commons::DEFAULT_MAX_HARD_WORD_SEQ, 0);
        }
      }
      for(auto it = atoms.begin(); it != atoms.end(); it++)
      {
        cont.add_trigger(it->second);
      }
      base.merge(cont, *info, true); 
      log_action_("Container was merged");
    }

    int ChannelContainerTest::check_update_() noexcept
    {//inspired by ADSC-4874
      const char* FN = "ChannelContainerTest::check_update_:";
      IdToTrigger triggers[] =
      {
        {11, "www.url1.com/error", true, false},
        {12, "www.url1.com/path/", true, false},
        {12, "www.url1.com/path", true, false},
        {13, "www.url1.com/b", true, false},
        {13, "www.url1.com/d", true, false},
        {13, "www.url1.com/f", true, false}
      };
      IdToTrigger triggers2[] =
      {
        {11, "www.url1.com/error", true, false},
        {12, "www.url1.com/path1", true, false},
        {12, "www.url1.com/path2", true, false} 
      };
      try
      {
        log_action_(" started.", 0, FN);
        ContPtr base;
        base.reset(new ChannelContainer(count_chunks_));
        log_action_("Container was created");
        init_triggers_(
          triggers,
          sizeof(triggers)/sizeof(triggers[0]),
          *base);
        ChannelSvcs::ChannelContainerBase::ChannelMap data;
        data[11].reset();
        data[12].reset();
        data[13].reset();
        log_action_("Get trigger lists from container");
        base->fill(data);
        if(verbose_ >= 2)
        {
          for(ChannelSvcs::ChannelContainerBase::ChannelMap::const_iterator i =
              data.begin(); i != data.end(); ++i)
          {
            std::cout << "id = " << i->first << " ";
            //i->second.print(std::cout);
            std::cout << std::endl;
          }
        }
        for(ChannelSvcs::ChannelContainerBase::ChannelMap::const_iterator it =
            data.begin(); it != data.end(); ++it)
        {
          if(it->second->triggers.size() != (it->first - 10))
          {
            std::cerr 
              << FN << "BUG: count url words for id=" << it->first
              << " isn't equal " << it->second->triggers.size() << " != "
              << (it ->first - 10) << "." << std::endl;
            if(verbose_ >= 3)
            {
              std::cerr << "Triggers:" << std::endl;
              for(auto it_tr = it->second->triggers.begin();
                  it_tr != it->second->triggers.end(); it_tr++)
              {
                std::string trigger_text;
                std::cerr << '\'' <<
                  ChannelSvcs::Serialization::get_trigger(
                    it_tr->matcher->get_trigger(), trigger_text) << '\''
                  << std::endl;
              }
            }
            return 1;
          }
        }
        log_action_("Second merge.");
        init_triggers_(
          triggers2,
          sizeof(triggers2)/sizeof(triggers2[0]),
          *base,
          100);
        log_action_("Get trigger lists from container");
        data.clear();
        data[11].reset();
        data[12].reset();
        base->fill(data);
        /*
        if(verbose_ >= 2)
        {
          for(TriggerBuffer::const_iterator i = data.begin();
              i != data.end(); i++)
          {
            i->second.print(std::cout);
            std::cout << std::endl;
          }
        }*/
        for(ChannelSvcs::ChannelContainerBase::ChannelMap::const_iterator it =
            data.begin(); it != data.end(); ++it)
        {
          if(it->second->triggers.size() != (it->first - 10) * 2)
          {
            std::cerr 
              << FN << "BUG: count url words for id=" << it->first
              << " isn't equal " << (it->first  - 10) << "." << std::endl;
            return 1;
          }
        }
        log_action_(" finished.", 0, FN);
        return 0;
      }
      catch(const ErrorDescriptor& e)
      {
        std::cerr << e.what();
      }
      catch(const eh::Exception& e)
      {
        std::cerr << FN << "caught eh::Exception " << e.what() << std::endl;
      }
      return 1;
    }

    ChannelContainerTest::AddUidMap& ChannelContainerTest::prepare_add_map_(
      const UidsData& uid_data,
      AddUidMap& add_uids,
      const IdSet& channels,
      bool add)
      noexcept
    {
      for(auto it_uid = uid_data.begin(); it_uid != uid_data.end(); it_uid++)
      {
        auto channel_id = it_uid->first;
        if (channels.empty() || channels.find(channel_id) != channels.end())
        {
          const Uuids& uids = it_uid->second;
          for(auto it = uids.begin(); it != uids.end(); ++it)
          {
            IdSet& ids = add_uids[*it];
            if (add)
            {
              ids.insert(ids.end(), channel_id);
            }
          }
        }
      }
      return  add_uids;
    }

    void ChannelContainerTest::check_uids_data_(
      const UidsData& data,
      const ChannelSvcs::UidMap& res,
      const IdSet& channels)
      /*throw(ErrorDescriptor)*/
    {
      UidsData result;
      for(auto it = res.begin(); it != res.end(); ++it)
      {
        const UidAtom& atom = *it->second;
        for(auto it_id = atom.begin(); it_id != atom.end(); ++it_id)
        {
          result[*it_id].insert(it->first);
        }
      }
      auto res_it = result.begin();
      auto it = data.begin();
      while(it != data.end() && res_it != result.end())
      {
        if (!channels.empty() &&
            channels.find(it->first) == channels.end())
        {//don't check data for channel not from list
          it++;
          continue;
        }
        if (it->first != res_it->first)
        {
          Stream::Error err;
          if (it->first < res_it->first)
          {
            err << __func__ << ": there is no id " << it->first << " in result";
          }
          else
          {
            err << __func__ << ": unexpected id "
              << res_it->first << " in result";
          }
          throw ErrorDescriptor(err);
        }
        const Uuids& left = it->second;
        const Uuids& right = res_it->second;
        auto left_it = left.begin();
        auto right_it = right.begin();
        for(; left_it != left.end() && right_it != right.end();
            ++left_it, ++right_it)
        {
          if (*left_it != *right_it)
          {
            Stream::Error err;
            if (*left_it < *right_it)
            {
              err << __func__ << ": no expected uid = " << left_it->to_string();
            }
            else
            {
              err << __func__ << ": unexpected uid = " << right_it->to_string();
            }
            throw ErrorDescriptor(err);
          }
        }
        if (left_it != left.end())
        {
          Stream::Error err;
          err << __func__ << ": no expected uid = " << left_it->to_string();
          throw ErrorDescriptor(err);
        }
        if (right_it != right.end())
        {
          Stream::Error err;
          err << __func__ << ": unexpected uid = " << right_it->to_string();
          throw ErrorDescriptor(err);
        }
        ++it;
        ++res_it;
      }
      while(it != data.end())
      {//skip data not from list
        if (!channels.empty() &&
            channels.find(it->first) == channels.end())
        {//don't check data for channel not from list
          it++;
        }
        else
        {
          break;
        }
      }
      if (it != data.end() || res_it != result.end())
      {
        Stream::Error err;
        err << __func__ << ": there isn't id: ";
        bool first = true;
        while(it != data.end())
        {
          if (!first)
          {
            err << ", ";
          }
          else
          {
            first = false;
          }
          err << it->first;
          it++;
        }
        if (!first)
        {
          err << " in result";
        }
        first = true;
        while(res_it != result.end())
        {
          if (!first)
          {
            err << ", ";
          }
          else
          {
            first = false;
          }
          err << res_it->first;
          res_it++;
        }
        if (!first)
        {
          err << " in source data";
        }
        throw ErrorDescriptor(err);
      }
      log_action_("uids chacked", 2, __func__);
    }

    void ChannelContainerTest::check_uids_prepare_removing_(
      IdSet& channels,
      IdSet& channels2,
      size_t percent)
      noexcept
    {
      auto count_channels = channels.size() * percent / 100;
      while(count_channels && !channels.empty())
      {
        auto random = Generics::safe_rand(channels.size());
        auto mid_it = channels.begin();
        std::advance(mid_it, random);
        channels2.insert(*mid_it);
        channels.erase(mid_it);
        count_channels--;
      }
    }

    void ChannelContainerTest::check_uids_perf_update_(
      ChannelSvcs::UidMap& res,
      AddUidMap& added,
      const IdSet& removed_uid_channels)
      noexcept
    {
      Generics::Timer timer;
      Generics::CPUTimer cpu_timer;
      timer.start();
      cpu_timer.start();
      ChannelSvcs::ChannelChunk::apply_uid_map_update_(
        res, added, removed_uid_channels);
      cpu_timer.stop_add(cpu_time_);
      timer.stop_add(time_);
      log_action_("update applied", 1, __func__);
    }

    int ChannelContainerTest::check_uids_() noexcept
    {
      UidsData data;
      IdSet channels, channels2;
      time_ = Generics::Time::ZERO;
      cpu_time_ = Generics::Time::ZERO;
      try
      {
        //create data
        log_action_("create random data", 1, __func__);
        for(size_t i = 0; i < count_channels_; ++i)
        {
          auto channel_id = i + 10;
          channels.insert(channels.end(), channel_id);
          Uuids& uids = data[channel_id];
          while(uids.size() < count_uids_)
          {
            uids.insert(Generics::Uuid::create_random_based());
          }
          {
            Stream::Error str;
            str << "generated channel " << channel_id;
            log_action_(str.str().str(), 2, __func__);
          }
        }
        {
          Stream::Error str;
          str << "generated " << count_channels_ << " channels";
          log_action_(str.str().str(), 1, __func__);
        }
        UidMap_var res = new UidMap;
        AddUidMap added;
        added.clear();
        prepare_add_map_(data, added, channels);
        log_action_("adding data prepared", 1, __func__);
        check_uids_perf_update_(*res, added, IdSet());
        check_uids_data_(data, *res);
        //remove 30% of channels
        check_uids_prepare_removing_(channels, channels2, 30);
        added.clear();
        prepare_add_map_(data, added, channels2, false);
        log_action_("removing 30 percents of channels", 1, __func__);
        check_uids_perf_update_(*res, added, channels2);
        log_action_("update applied", 1, __func__);
        check_uids_data_(data, *res, channels);
        //adding channels back
        channels.insert(channels2.begin(), channels2.end());
        added.clear();
        prepare_add_map_(data, added, channels2);
        log_action_("adding 30 percents of channels", 1, __func__);
        check_uids_perf_update_(*res, added, IdSet());
        check_uids_data_(data, *res);
        //add 1 uid to every channel
        for(auto it = data.begin(); it != data.end(); ++it)
        {
          Uuids& uids = it->second;
          uids.insert(Generics::Uuid::create_random_based());
        }
        added.clear();
        prepare_add_map_(data, added, channels);
        log_action_("adding 1 uid to every channel", 1, __func__);
        check_uids_perf_update_(*res, added, channels);
        check_uids_data_(data, *res);
        //check same uids
        added.clear();
        channels2 = channels;
        channels2.erase(channels2.begin());
        //remove old uids for all channels except first
        prepare_add_map_(data, added, channels2, false);
        for(auto it = data.begin(); it != data.end(); ++it)
        {
          Uuids& uids = it->second;
          if (it != data.begin())
          {
            uids = data.begin()->second;
          }
        }
        //add uids from first channels to other channels
        prepare_add_map_(data, added, channels2);
        log_action_("make same uids in all channels", 1, __func__);
        check_uids_perf_update_(*res, added, channels2);
        check_uids_data_(data, *res);
        Generics::Uuid uid = Generics::Uuid::create_random_based();
        //add same uid to every channel
        for(auto it = data.begin(); it != data.end(); ++it)
        {
          Uuids& uids = it->second;
          uids.insert(uid);
        }
        added.clear();
        prepare_add_map_(data, added, channels);
        log_action_("adding same uid to every channel", 1, __func__);
        check_uids_perf_update_(*res, added, channels);
        check_uids_data_(data, *res);
        //remove 30% of channels
        channels2.clear();
        check_uids_prepare_removing_(channels, channels2, 30);
        added.clear();
        prepare_add_map_(data, added, channels2, false);
        log_action_("removing 30 percents of channels", 1, __func__);
        check_uids_perf_update_(*res, added, channels2);
        check_uids_data_(data, *res, channels);
        //adding channels back
        channels.insert(channels2.begin(), channels2.end());
        added.clear();
        prepare_add_map_(data, added, channels2);
        log_action_("adding 30 percents of channels", 1, __func__);
        check_uids_perf_update_(*res, added, IdSet());
        check_uids_data_(data, *res);
        {
          Stream::Error times;
          times << "Update time: " << time_;
          times << " CPU time: " << time_;
          log_action_(times.str().str(), 1, __func__);
        }
        return 0;
      }
      catch(const ErrorDescriptor& e)
      {
        std::cerr << __func__ << ": " << e.what() << std::endl;
      }
      catch(const eh::Exception& e)
      {
        std::cerr << __func__ << ": eh::Exception :" << e.what() << std::endl;
      }
      return 1;
    }

    void ChannelContainerTest::check_data_of_container_(
      unsigned long start,
      unsigned long last,
      const ChannelSvcs::ChannelContainerBase::ChannelMap& buffer,
      const char* func,
      const char* prefix)
      /*throw(ErrorDescriptor)*/
    {
      std::ostringstream out;
      out << "Checking data of container";
      if(prefix)
      {
        out << " with prefix '" << prefix << "'";
      }
      out << ", diapason from " << start << " to " << last << ".";
      log_action_(out.str());
      ChannelSvcs::ChannelContainerBase::ChannelMap::const_iterator it =
        buffer.find(start + 1);
      if(it == buffer.end() || !it->second)
      {
        Stream::Error err;
        err << func << " BUG: atom for id = " << start + 1 <<
          " wasn't found in buffer " <<  std::endl;
        throw ErrorDescriptor(err);
      }
      for(size_t i = start; i < last; i++, it++)
      {
        MergeAtom atom;
        log_action_("Generating test atom.", 2);
        gen_atom_(atom, i, prefix);
        check_atom_(func, *it->second, atom);
      }
    }

    void ChannelContainerTest::create_data_for_container_(
      unsigned long start,
      unsigned long last,
      ChannelSvcs::UpdateContainer* cont,
      const char* prefix,
      ChannelSvcs::ChannelContainerBase::ChannelMap* buffer)
      /*throw(eh::Exception)*/
    {
      std::ostringstream out;
      out << "Generating data for container";
      if(prefix)
      {
        out << " with prefix '" << prefix << "'";
      }
      out << ", diapason from " << start << " to " << last << ".";
      log_action_(out.str());
      //ChannelIdToMatchInfo* info_ptr_ = cont->get_info();
      for(size_t i = start; i < last; i++)
      {
        MergeAtom atom;
        {
          std::ostringstream ostr;
          ostr << "Generating atom number " << i;
          log_action_(ostr.str(), 2);
        }
        gen_atom_(atom, i, prefix);
        //MatchInfo& m_info = (*info_ptr_)[atom.id];
        //m_info.channel.mark_type(atom.id % 3 != 1 ? CT_PAGE : CT_URL);
        //m_info.channel.mark_type(Channel::CT_ACTIVE);
        {
          std::ostringstream ostr;
          ostr << "Atom added to container. Atom = ";
          atom.print(ostr);
          cont->add_trigger(atom);
          ostr << std::endl
            <<"Adding atom with id " << atom.id << " in buffer";
          log_action_(ostr.str(), 3);
        }
        if(buffer)
        {
          (*buffer)[atom.id].reset();
        }
      }
    }

    void ChannelContainerTest::gen_atom_(
      MergeAtom& atom,
      size_t num,
      const char* prefix)
      /*throw(eh::Exception)*/
    {
      std::string pre;
      if(prefix)
      {
        pre = prefix;
      }
      atom.id = num + 1;
      switch(num % 3)
      {
        case 0://text
        case 2:
          log_action_("Generating hard words.", 3);
          gen_hard_words_((pre + "hard").c_str(), atom.soft_words, num, false);
          log_action_("Generating soft words.", 3);
          gen_soft_words_(
            (pre + "soft").c_str(), atom.soft_words, num, 3, false);
          log_action_("Generating negative hard words.", 3);
          gen_hard_words_(
            (pre + "neghard").c_str(), atom.soft_words, num, true);
          log_action_("Generating negative soft words.", 3);
          gen_soft_words_(
            (pre + "negsoft").c_str(), atom.soft_words, num, 3, true);
          break;
        case 1://url
          log_action_("Generating url words.", 3);
          gen_url_((pre + std::string("url")).c_str(), atom.soft_words, num, false);
          log_action_("Generating negative url words.", 3);
          gen_url_((pre + "negurl").c_str(), atom.soft_words, num, true);
          break;
      }
    }

    void ChannelContainerTest::check_atom_(
      const char* test_place,
      const ChannelSvcs::ChannelContainerBase::ChannelUpdateData& value,
      const ChannelSvcs::MergeAtom& atom)
      /*throw(ErrorDescriptor)*/
    {
      Stream::Error err;
      log_action_("Checking words.", 3);
      if(!compary_words_(atom.soft_words, value.triggers))
      {
        err << test_place << "BUG: triggers for id = " << atom.id <<
          " isn't equal. Atoms: " << std::endl;
        atom.print(err);
        err << std::endl;
        //value.second.print(err);
        err <<  std::endl;
        throw ErrorDescriptor(err);
      }
    }

    int ChannelContainerTest::run(int argc, char* argv[]) noexcept
    {
      const char* FN = "ChannelContainerTest::run:";
      try
      {
        int ret_value = 0;
        if(argc > 1 && strcasecmp(argv[1], "help") == 0)
        {
          usage_();
          return ret_value;
        }
        parse_argc_(argc, argv);
        if(cases_ == TC_NONE)
        {
          cases_ = TC_ALL_CASES; // by default check all cases
        }
        if(verbose_)
        {
          log_parameters_();
        }
        if(cases_ & TC_CONSTRUCTOR)
        {
          ret_value += check_constructor_();
        }
        if(cases_ & TC_ADD_TRIGGER)
        {
          ret_value += check_add_trigger_();
        }
        if(cases_ & TC_MERGE)
        {
          ret_value += check_merge_();
        }
        if(cases_ & TC_MATCH)
        {
          ret_value += check_match_();
        }
        if(cases_ & TC_UPDATE)
        {
          ret_value += check_update_();
        }
        if(cases_ & TC_UIDS)
        {
          ret_value += check_uids_();
        }
        return ret_value;
      }
      catch(const eh::Exception& e)
      {
        std::cerr << FN << "caught eh::Exception " << e.what() << std::endl;
      }
      catch(...)
      {
        std::cerr << FN << "caught unknown exception" << std::endl;
      }
      return 1;
    }

    template<class ResultType>
    ResultType& ChannelContainerTest::gen_hard_words_(
      const char* prefix,
      ResultType& result,
      size_t number,
      bool negative, 
      size_t count)
      /*throw(eh::Exception)*/
    {
      for(size_t i = 0; i < count;  i++)
      {
        size_t id = number * 100 + i * 2 + (negative ? 0 : 1);
        std::ostringstream ostr;
        ostr << (negative ? "minus" : " ") << prefix
          << "and" << id; 
        TriggerParser::TriggerParser::parse_word(
          id,//TODO: should be channel id
          id,
          'P',
          ostr.str(),
          negative,
          0,
          result,
          Commons::DEFAULT_MAX_HARD_WORD_SEQ,
          0);
      }
      return result;
    }

    void ChannelContainerTest::gen_soft_words_(
      const char* prefix,
      ChannelSvcs::SoftTriggerList& result,
      size_t number,
      size_t count_words,
      bool negative, 
      size_t count)
      /*throw(eh::Exception)*/
    {
      for(size_t i = 0; i < count;  i++)
      {
        size_t id = 10000 + number * 100 + i * 2 + (negative ? 0 : 1);
        std::ostringstream ostr;
        for(size_t j = 0; j < count_words; j++)
        {
          ostr << prefix << id << "and" << j << ' ';
        }
        TriggerParser::TriggerParser::parse_word(
          id,//TODO: should be channel id
          id,
          'P',
          ostr.str(),
          negative,
          0,
          result,
          Commons::DEFAULT_MAX_HARD_WORD_SEQ,
          0);
      }
    }

    template<class CompType, class CompType2>
    bool ChannelContainerTest::compary_words_(
      const CompType& orig,
      const CompType2& res)
      noexcept
    {
      for(typename CompType::const_iterator it = orig.begin();
          it != orig.end(); ++it)
      {
        typename CompType2::const_iterator it2 =
          std::lower_bound(res.begin(), res.end(), it->channel_trigger_id);
        bool found = (it2 != res.end() && *it2 == it->channel_trigger_id);
        if(!found)
        {
          return false;
        }
      }
      return true;
    }

    void ChannelContainerTest::gen_url_(
      const char* prefix,
      ChannelSvcs::SoftTriggerList& result,
      size_t number,
      bool negative,
      size_t count)
      /*throw(eh::Exception)*/
    {
      std::set<unsigned short> ports;
      for(size_t i = 0; i < count; i++)
      {
        size_t id = 20000 + number * 100 + i * 2 + (negative ? 0 : 1);
        std::string trigger;
        std::ostringstream ostr;
        ostr << "www." << prefix << ".ru/" << id;
        TriggerParser::TriggerParser::parse_url(
          id,//should be channel_id
          id,
          ostr.str(), false, ports, result, 0);
      }
    }

    void ChannelContainerTest::usage_() noexcept
    {
      std::cout << "ChannelContainerTest -c[--chunks] "
        "count_chunks -v[--verbose] -t[--tests] test1,test2 "
        "-r[--triggers] count_triggers" << std::endl;
      std::cout << "Tests: ";
      for(size_t i = 0; i < sizeof(test_names_)/sizeof(test_names_[0]); i++)
      {
        if(i)
        {
          std::cout << ',';
        }
        std::cout << test_names_[i].name;
      }
      std::cout << '.' << std::endl;
    }

    void ChannelContainerTest::log_parameters_() noexcept
    {
      size_t count = 0;
      std::cout << "ChannelContainerTest parameters: {test cases => ";
      for(size_t i = 0; i < sizeof(test_names_)/sizeof(test_names_[0]); i++)
      {
        if(cases_ & test_names_[i].value)
        {
          if(count++)
          {
            std::cout << ',';
          }
          std::cout << test_names_[i].name;
        }
      }
      
      std::cout << "}, {count chunks => " << count_chunks_ << "},"
        " {count triggers => " << count_triggers_ << "}," <<
        " {count uids => " << count_uids_ << "}," <<
        " {count channels => " << count_channels_ << "}," <<
        " {verbose => " << verbose_ << "}." << std::endl;
    }

    void
    ChannelContainerTest::log_action_(
      const std::string& action,
      unsigned int level,
      const char* function) const
      /*throw(eh::Exception)*/
    {
      if(verbose_ >= level)
      {
        std::cout << Generics::Time::get_time_of_day().get_gm_time() << " ";
        if(function)
        {
          std::cout << function << ": ";
        }
        std::cout  << action << std::endl;
      }
    }

    void ChannelContainerTest::parse_tests_(const char* tests) noexcept
    {
      String::SubString s(tests);
      String::StringManip::SplitComma tok(s);
      String::SubString sub;
      while(tok.get_token(sub))
      {
        for(size_t i = 0; i < sizeof(test_names_)/sizeof(test_names_[0]); i++)
        {
          if(strncasecmp(
              test_names_[i].name,
              sub.begin(),
              sub.end() - sub.begin()) == 0)
          {
            cases_ |= test_names_[i].value;
          }
        }
      }
    }

    void ChannelContainerTest::parse_argc_(int argc, char* argv[]) noexcept
    {
      struct option long_options[] = 
      {
        {"verbose", no_argument, 0, 'v'},
        {"chunks", required_argument, 0, 'c'},
        {"uids", required_argument, 0, 'u'},
        {"channels", required_argument, 0, 'h'},
        {"tests", required_argument, 0, 't'},
        {"triggers", required_argument, 0, 'r'},
        {0, 0, 0, 0}
      };
      int opt, index = 0;
      do
      {
        opt = getopt_long(
          argc,
          argv,
          "vt:c:r:u:h:",
          long_options,
          &index);
        switch(opt)
        {
          case 'v':
            verbose_++;
            break;
          case 'c':
            read_number(optarg, 0UL, ULONG_MAX, count_chunks_);
            break;
          case 'u':
            read_number(optarg, 0UL, ULONG_MAX, count_uids_);
            break;
          case 'h':
            read_number(optarg, 0UL, ULONG_MAX, count_channels_);
            break;
          case 'r':
            read_number(optarg, 0UL, ULONG_MAX, count_triggers_);
            break;
          case 't':
            parse_tests_(optarg);
            break;
        }
      } while(opt != -1);
    }
  }
}

int main(int argc, char* argv[])
{
  AdServer::Testing::ChannelContainerTest test;
  return test.run(argc, argv);
}

