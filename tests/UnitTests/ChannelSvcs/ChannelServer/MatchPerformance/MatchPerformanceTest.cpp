
#include <unistd.h>
#include <getopt.h>
#include <limits.h>
#include <deque>
#include <string>
#include <sstream>
#include <Generics/Rand.hpp>
#include <Generics/Time.hpp>
#include<Generics/Statistics.hpp>
#include <String/UnicodeSymbol.hpp>
#include <Stream/MemoryStream.hpp>
#include <ChannelSvcs/ChannelCommons/CommonTypes.hpp>
#include <ChannelSvcs/ChannelCommons/TriggerParser.hpp>
#include <ChannelSvcs/ChannelServer/ChannelContainer.hpp>
#include<tests/UnitTests/ChannelSvcs/Commons/ChannelServerTestCommons.hpp>

#include"MatchPerformanceTest.hpp"

namespace AdServer
{
  namespace UnitTests
  {

    MatchPerformanceTest::MatchPerformanceTest() noexcept
    {
    }

    void MatchPerformanceTest::prepare_data_(
      ChannelSvcs::UpdateContainer& cont,
      ChannelSvcs::ChannelIdToMatchInfo* info)
      /*throw(eh::Exception)*/
    {
      try
      {
        std::set<unsigned short> empty;
        ChannelSvcs::TriggerList triggers;
        unsigned int id;
        for(size_t i = 0; i < data_size_; i++)
        {
          triggers.clear();
          id = i + 1;
          ChannelSvcs::MatchInfo& match_info = (*info)[id];
          if(verbose_level_)
          {
            std::cout << "id=" << id << ";";
          }
          if(i % 2 == 0)
          {
            for(size_t j = 0; j < urls_length_; j++)
            {
              triggers.push_back(ChannelSvcs::Trigger());
              ChannelSvcs::Trigger& trigger = triggers.back();
              trigger.channel_trigger_id = i * 1000 + j + 1;
              trigger.type = 'U';
              trigger.negative = false;
              generate_url_(trigger.trigger);
            }
            if(verbose_level_ > 1)
            {
              std::cout << "url=";
              for(ChannelSvcs::TriggerList::const_iterator it =
                  triggers.begin(); it != triggers.end(); ++it)
              {
                if(it != triggers.begin())
                {
                  std::cout << ", ";
                }
                std::cout << it->trigger;
              }
              std::cout << "." << std::endl;
            }
            match_info.channel = ChannelSvcs::Channel(id);
            match_info.channel.mark_type(ChannelSvcs::CT_URL);
          }
          else
          {
            for(size_t j = 0; j < count_hards_; j++)
            {
              triggers.push_back(ChannelSvcs::Trigger());
              ChannelSvcs::Trigger& trigger = triggers.back();
              trigger.channel_trigger_id = i * 1000 + j + 1;
              trigger.type = 'P';
              trigger.negative = false;
              ChannelServerTestCommons::generate_word(trigger.trigger);
            }
            for(size_t j = 0; j < count_soft_; j++)
            {
              triggers.push_back(ChannelSvcs::Trigger());
              ChannelSvcs::Trigger& trigger = triggers.back();
              trigger.channel_trigger_id = i * 1000 + j + 1;
              trigger.type = 'P';
              trigger.negative = false;
              for(size_t k = 0; k < soft_length_; k++)
              {
                std::string word;
                if(k != 0)
                {
                  trigger.trigger.push_back(' ');
                }
                ChannelServerTestCommons::generate_word(word);
                trigger.trigger.append(word);
              }
            }
            if(verbose_level_ > 1)
            {
              std::cout << "words=";
              for(ChannelSvcs::TriggerList::const_iterator it =
                  triggers.begin(); it != triggers.end(); ++it)
              {
                if(it != triggers.begin())
                {
                  std::cout << ", ";
                }
                std::cout << it->trigger;
              }
              std::cout << "." << std::endl;
              std::cout << "." << std::endl;
            }
            match_info.channel = ChannelSvcs::Channel(id);
            match_info.channel.mark_type(ChannelSvcs::CT_PAGE);
          }
          ChannelSvcs::TriggerParser::TriggerParser::parse_triggers(
            id,
            "",
            triggers, 0, empty, &cont, Commons::DEFAULT_MAX_HARD_WORD_SEQ);
        }
        if(verbose_level_)
        {
          std::cout  << std::endl << "Generating data: "
            << data_size_ << '.' << std::endl;
        }
      }
      catch(const ChannelSvcs::ChannelContainer::Exception& e)
      {
        std::cerr << "caught << ChannelContainer::Exception: " << e.what();
        throw eh::Exception();
      }
      catch(const ChannelSvcs::TriggerParser::Exception& e)
      {
        std::cerr << "caught << TriggerParser::Exception: " << e.what();
        throw eh::Exception();
      }
    }

    int MatchPerformanceTest::execute_() /*throw(eh::Exception)*/
    {
      try
      {
        ChannelSvcs::MatchUrls url_words;
        ChannelSvcs::MatchWords match_words[ChannelSvcs::CT_MAX];
        unsigned long period100;

        if(quires_>=10)
        {
          period100 = quires_/10;
        }
        else
        {
          period100 = 1;
        }
        Generics::Statistics::TimedStatSink_var stat(
          new Generics::Statistics::TimedStatSink);
        Generics::Statistics::TimedStatSink_var cpu_stat(
          new Generics::Statistics::TimedStatSink);
        unsigned long i = 0, count = 100, first_id = 1;
        ChannelSvcs::ChannelContainerBase::ChannelMap buffer;
        //ChannelSvcs::ChannelContainerBase::ChannelMap::const_iterator buf_it;

        for(; time_stat_->total_time() < Generics::Time(time_) && i < quires_; i++)
        {
          url_words.clear();
          match_words[ChannelSvcs::CT_PAGE].clear();
          if(count == 100)
          {
            prepare_match_data_(first_id, 100, buffer);
            first_id += 100;
            count = 0;
            //buf_it = buffer.begin();
          }
          /*
          if(i == next_match)
          {
            const ChannelSvcs::MergeAtom& atom = buf_it->second;
            buf_it++;
            if(atom.url_words.size())
            {
              ChannelSvcs::ChannelContainer::match_parse_refer(
                atom.url_words[0],
                std::set<unsigned short>(),
                false,
                url_words,
                0);
            }
            for(ChannelSvcs::StringList::const_iterator s_it =
                atom.hard_words.begin(); s_it != atom.hard_words.end(); s_it++)
            {
              match_words[ChannelSvcs::CT_PAGE].insert(*s_it);
            }
            for(ChannelSvcs::SoftList::const_iterator s_it =
                atom.soft_words.begin(); s_it != atom.soft_words.end(); s_it++)
            {
              std::string soft_word;
              for(size_t k = 0; k < s_it->size(); k++)
              {
                if(k != 0)
                {
                  soft_word.push_back(' ');
                }
                soft_word += (*s_it)[k];
              }
              match_words[ChannelSvcs::CT_PAGE].insert(soft_word);
            }
            value_match += step_match;
            next_match = (unsigned long)value_match;
          }
          else
          {
            ChannelSvcs::UrlWord m_word;
            generate_url_(m_word);
            if(verbose_level_ > 1)
            {
              std::cout << "match query: " << m_word << ",";
            }
            ChannelSvcs::ChannelContainer::match_parse_refer(
              m_word,
              std::set<unsigned short>(),
              false,
              url_words,
              0);
            size_t count = Generics::safe_rand(1, 7);
            std::string word;
            for(size_t j = 0; j < count; j++)
            {
              word.clear();
              ChannelServerTestCommons::generate_word(word);
              if(verbose_level_ > 1)
              {
                std::cout << word;
                (j==count ? std::cout << std::endl : std::cout << ',');
              }
              match_words[ChannelSvcs::CT_PAGE].insert(word);
            }
            if(verbose_level_ > 1)
            {
              std::cout << std::endl;
            }
          }*/
          Generics::Timer timer;
          Generics::CPUTimer cpu_timer;
          timer.start();
          cpu_timer.start();
          for(size_t match_counter = 0; match_counter < 100; match_counter++)
          {
            make_match_(url_words, match_words);
          }
          cpu_timer.stop();
          timer.stop();
          Generics::Statistics::TimedSubject
            time_1(timer.elapsed_time()), cpu_time_1(cpu_timer.elapsed_time());
          time_stat_->consider(time_1);
          cpu_time_stat_->consider(cpu_time_1);
          if(verbose_level_)
          {
            stat->consider(time_1);
            cpu_stat->consider(cpu_time_1);
            if(i % period100 == 0)
            {
              dump_statistic_(std::cout, stat.in(), cpu_stat.in());
              stat = new Generics::Statistics::TimedStatSink;
              cpu_stat = new Generics::Statistics::TimedStatSink;
            }
          }
        }
        dump_statistic_(std::cout);
        return 0;
      }
      catch(const ChannelSvcs::ChannelContainer::Exception& e)
      {
        std::cerr << "caught << eh::Exception: " << e.what();
        return 1;
      }
    }

    void MatchPerformanceTest::generate_url_(
      std::string& word) /*throw(eh::Exception)*/
    {
      std::string host, path, part;
      size_t subdomains = Generics::safe_rand(1, 3);
      for(size_t i=0;i<subdomains;i++)
      {
        if(i)
        {
          host.push_back('.');
        }
        ChannelServerTestCommons::generate_asc_word(host,4);
      }
      size_t count = Generics::safe_rand(1, 6);
      if(count)
      {
        for(size_t i=0;i<count;i++)
        {
          path.push_back('/');
          part.clear();
          ChannelServerTestCommons::generate_asc_word(part);
          path += part;
        }
      }
      else
      {
        path.push_back('/');
      }
      word = host + path;
    }

    void MatchPerformanceTest::prepare_match_data_(
      unsigned long first_id,
      unsigned long count,
      ChannelSvcs::ChannelContainerBase::ChannelMap& buffer)
      /*throw(eh::Exception)*/
    {
      buffer.clear();
      for(size_t i = 0; i< count; i++)
      {
        buffer[first_id + i].reset();
      }
      container_.fill(buffer);
    }

    int MatchPerformanceTest::run(int argc, char* argv[]) noexcept
    {
      try
      {
        parse_arguments(argc, argv);
        {
          ChannelSvcs::UpdateContainer update_container(&container_, 0);
          ChannelSvcs::ChannelIdToMatchInfo_var info  =
            new ChannelSvcs::ChannelIdToMatchInfo;
          std::cout << "1. Start test" << std::endl;
          dump_memory_statistic_(std::cout);
          prepare_data_(update_container, info);
          std::cout << "2. Data prepared" << std::endl;
          dump_memory_statistic_(std::cout);
          container_.merge(update_container, *info, true);
          std::cout << "3. Container merged" << std::endl;
          dump_memory_statistic_(std::cout);
        }
        int ret_value = execute_();
        std::cout << "4. Test finished" << std::endl;
        return ret_value;
      }
      catch(const eh::Exception& e)
      {
        std::cerr << "caught << eh::Exception: " << e.what();
        return 1;
      }
    }

  }
}

int main(int argc, char* argv[])
{
  AdServer::UnitTests::MatchPerformanceTest test;
  return test.run(argc, argv);
}

