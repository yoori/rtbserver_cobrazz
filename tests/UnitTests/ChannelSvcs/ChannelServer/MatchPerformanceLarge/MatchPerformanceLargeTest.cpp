
#include <unistd.h>
#include <limits.h>
#include <deque>
#include <string>
#include <sstream>
#undef _USE_OCCI
#include <Generics/Rand.hpp>
#include <Generics/Time.hpp>
#include <Stream/MemoryStream.hpp>
#include <Commons/CorbaAlgs.hpp>
#include <ChannelSvcs/ChannelCommons/CommonTypes.hpp>
#include <ChannelSvcs/ChannelCommons/TriggerParser.hpp>
#include <ChannelSvcs/ChannelCommons/ChannelUtils.hpp>
#include <ChannelSvcs/ChannelServer/ChannelContainer.hpp>
#include<tests/UnitTests/ChannelSvcs/Commons/ChannelServerTestCommons.hpp>

#include"MatchPerformanceLargeTest.hpp"

namespace AdServer
{
  namespace UnitTests
  {

    MatchPerformanceLargeTest::MatchPerformanceLargeTest() noexcept
    {
      time_ = -1;
    }


    void MatchPerformanceLargeTest::parse_keywords_(
      ChannelSvcs::MatchWords& out)
      noexcept
    {
      ChannelSvcs::parse_keywords<ChannelSvcs::MatchBreakSeparators>(
        match_data_, out, ChannelSvcs::PM_SIMPLIFY);
    }

    void MatchPerformanceLargeTest::fill_matched_words_(
      std::vector<std::string>& matched_words,
      size_t count_hard,
      size_t count_soft) noexcept
    {
      ChannelSvcs::MatchWords match_words[ChannelSvcs::CT_MAX];
      parse_keywords_(match_words[ChannelSvcs::CT_PAGE]);
      ChannelSvcs::MatchWords::const_iterator it, it_stop;
      for(it = match_words[ChannelSvcs::CT_PAGE].begin();
          it != match_words[ChannelSvcs::CT_PAGE].end() &&
          matched_words.size() < count_hard; ++it)
      {
        const String::SubString& current = it->text();
        if(current.find(' ') != std::string::npos)
        {
          matched_words.push_back(current.str());
        }
      }
      it_stop = it;
      if(matched_words.size() && matched_words.size() < count_hard)
      {
        size_t i = 0;
        while(matched_words.size() < count_hard)
        {
          matched_words.push_back(matched_words[i++]);
        }
      }
      for(;it != match_words[ChannelSvcs::CT_PAGE].end() &&
          matched_words.size() < count_soft + count_hard; ++it)
      {
        const String::SubString& current = it->text();
        if(current.find(' ') == std::string::npos)
        {
          matched_words.push_back(current.str());
        }
      }
      for(it = match_words[ChannelSvcs::CT_PAGE].begin();
          it != it_stop &&
          matched_words.size() < count_soft + count_hard; ++it)
      {
        const String::SubString& current = it->text();
        if(current.find(' ') == std::string::npos)
        {
          matched_words.push_back(current.str());
        }
      }
      if(matched_words.size())
      {
        if(matched_words.size() < count_hard + count_soft)
        {
          size_t i = matched_words.size() < count_hard ? 0 : count_hard;
          while(matched_words.size() < count_hard + count_soft)
          {
            matched_words.push_back(matched_words[i++]);
          }
        }
      }
    }

    void MatchPerformanceLargeTest::prepare_data_(
      ChannelSvcs::UpdateContainer& cont,
      ChannelSvcs::ChannelIdToMatchInfo* info)
      /*throw(eh::Exception)*/
    {
      try
      {
        size_t channel_id = 1;
        ChannelSvcs::TriggerList triggers;
        std::set<unsigned short> empty;
        for(size_t i = 0; i < count_urls_; i++, channel_id++)
        {
          ChannelSvcs::MatchInfo& match_info = (*info)[channel_id];
          match_info.channel = ChannelSvcs::Channel(channel_id);
          match_info.channel.mark_type(ChannelSvcs::CT_URL);
          triggers.clear();
          for(size_t j = 0; j < urls_length_; j++)
          {
            triggers.push_back(ChannelSvcs::Trigger());
            ChannelSvcs::Trigger& trigger = triggers.back();
            trigger.channel_trigger_id = i * 1000 + j + 1;
            trigger.type = 'U';
            trigger.negative = false;
            generate_url_(trigger.trigger);
          }
          ChannelSvcs::TriggerParser::TriggerParser::parse_triggers(
            channel_id,
            "", triggers, 0, empty, &cont, Commons::DEFAULT_MAX_HARD_WORD_SEQ);
        }

        unsigned long soft_filling = (unsigned long)
          (data_size_ * split_percent_ / 100.0f);
        size_t soft_count = soft_filling / (soft_length_ + 1);
        size_t count_soft_in_sentence = sentence_words_ / word_length_;
        size_t count_soft_sentence = soft_count / sentence_words_;
        unsigned long count_soft_match = (unsigned long)
          (count_soft_in_sentence * count_soft_sentence *
           (match_percent_ / 100.0f));
        soft_filling = soft_count * (soft_length_ + 1);

        unsigned long hard_filling =  data_size_ - soft_filling;
        unsigned long count_hard = hard_filling / (hard_length_ + 1);
        unsigned long count_hard_match = (unsigned long)
          (count_hard * (match_percent_ / 100.0f ));

        if(count_soft_match == 0 && count_hard_match == 0) count_hard_match++;

        std::vector<std::string> matched_words;
        std::vector<std::string>::reverse_iterator r_it;
        matched_words.reserve(count_hard_match + count_soft_match);

        if(body_)
        {
          fill_matched_words_(
            matched_words,
            count_hard_match,
            count_soft_match);
        }

        triggers.clear();
        ChannelSvcs::MatchInfo& match_info = (*info)[channel_id];
        match_info.channel = ChannelSvcs::Channel(channel_id);
        match_info.channel.mark_type(ChannelSvcs::CT_PAGE);

        for(size_t j = 0; j < count_hards_; j++)
        {
          triggers.push_back(ChannelSvcs::Trigger());
          ChannelSvcs::Trigger& trigger = triggers.back();
          trigger.channel_trigger_id = j + 10000;
          trigger.type = 'P';
          trigger.negative = false;
          if(body_ && j < count_hard_match)
          {
            trigger.trigger = matched_words[j];
          }
          else
          {
            ChannelServerTestCommons::generate_asc_word(
              trigger.trigger, hard_length_);
            if(j < count_hard_match)
            {
              matched_words.push_back(trigger.trigger);
            }
          }
        }
        ChannelSvcs::TriggerParser::TriggerParser::parse_triggers(
          channel_id,
          "", triggers, 0, empty, &cont, Commons::DEFAULT_MAX_HARD_WORD_SEQ);
        channel_id++;

        {
          ChannelSvcs::MatchInfo& match_info = (*info)[channel_id];
          match_info.channel = ChannelSvcs::Channel(channel_id);
          match_info.channel.mark_type(ChannelSvcs::CT_PAGE);
        }
        for(size_t j = 0; j < count_soft_; j++)
        {
          triggers.push_back(ChannelSvcs::Trigger());
          ChannelSvcs::Trigger& trigger = triggers.back();
          trigger.channel_trigger_id = 20000 + j;
          trigger.type = 'P';
          trigger.negative = false;
          if(body_ && j < count_soft_match &&
             matched_words.size() < count_hard_match + j)
          {
            trigger.trigger = matched_words[j];
          }
          else
          {
            for(size_t k = 0; k < word_length_; k++)
            {
              std::string word;
              if(k != 0)
              {
                trigger.trigger.push_back(' ');
              }
              ChannelServerTestCommons::generate_asc_word(
                word, soft_length_);
              trigger.trigger.append(word);
            }
            if(j < count_soft_match)
            {
              matched_words.push_back(trigger.trigger);
            }
          }
        }
        ChannelSvcs::TriggerParser::TriggerParser::parse_triggers(
          channel_id,
          "", triggers, 0, empty, &cont, Commons::DEFAULT_MAX_HARD_WORD_SEQ);
        channel_id++;

        if(!body_) // generate body
        {
          match_data_.reset_max_size(data_size_ + 2);
          size_t soft_rate = 0;
          if(count_soft_match)
          {
            soft_rate = count_soft_sentence * count_soft_in_sentence
              / count_soft_match;
          }

          size_t soft_match_sum = 0;
          r_it = matched_words.rbegin();
          for(size_t i = 0; i < count_soft_sentence; i++)
          {
            for(size_t j = 0; j < count_soft_in_sentence; j++)
            {
              if(j)
              {
                match_data_.append(" ", 1);
              }
              if (soft_rate
                && (i * count_soft_in_sentence + j) % soft_rate == 0
                && soft_match_sum < count_soft_match
                && r_it != matched_words.rend())
              {
                soft_match_sum++;
                match_data_.append(r_it->c_str(), r_it->size());
                ++r_it;
              }
              else
              {
                for(size_t k = 0; k < word_length_; k++)
                {
                  if(k)
                  {
                    match_data_.append(" ", 1);
                  }
                  ChannelServerTestCommons::generate_asc_word(
                    match_data_,
                    soft_length_);
                }
              }
            }
            for(size_t j = count_soft_in_sentence * word_length_;
                j < sentence_words_; j++)
            {
              ChannelServerTestCommons::generate_asc_word(
                match_data_,
                soft_length_);
            }
            match_data_.append(",", 1);
          }

          size_t hard_rate = 0;
          if(count_hard_match)
          {
            hard_rate = count_hard / count_hard_match;
          }
          for(size_t i = 0; i < count_hard; i++)
          {
            match_data_.append((i % sentence_words_) ? " " : ",", 1);
            if(hard_rate && i % hard_rate == 0 && r_it != matched_words.rend())
            {
              match_data_.append(r_it->c_str(), r_it->size());
              ++r_it;
            }
            else
            {
              ChannelServerTestCommons::generate_asc_word(
                match_data_,
                soft_length_);
            }
          }
        }

        if(verbose_level_)
        {
          std::cout << "Generating " << count_urls_ << " urls, "
            << count_hards_ << " hard words, " << count_soft_
            << " soft words, " << count_soft_match
            << " match soft words, " << count_hard_match
            << " match hard words." << std::endl;
        }
        if(verbose_level_ > 1)
        {
          std::cout << "Match words: " << std::endl;
          for(r_it = matched_words.rbegin();
              r_it != matched_words.rend(); ++r_it)
          {
            std::cout << *r_it << std::endl;
          }
          std::cout << "Match data: " << std::endl
            << match_data_.c_str() << std::endl;
        }
      }
      catch(const ChannelSvcs::ChannelContainer::Exception& e)
      {
        std::cerr << "caught ChannelContainer::Exception: "
          << e.what() << std::endl;
        throw eh::Exception();
      }
      catch(const ChannelSvcs::TriggerParser::Exception& e)
      {
        std::cerr << "caught  TriggerParser::Exception: "
          << e.what() << std::endl;
        throw eh::Exception();
      }
    }

    int MatchPerformanceLargeTest::execute_() /*throw(eh::Exception)*/
    {
      try
      {
        ChannelSvcs::MatchUrls url_words;

        Generics::Time start_time, end_time;
        start_time = Generics::Time::get_time_of_day();
        if(time_ > 0)
        {
          end_time = start_time + time_;
        }
        size_t match_counter;
        for(match_counter = 0;
            (time_ > 0 && Generics::Time::get_time_of_day() < end_time) ||
            (time_ < 0 && match_counter < quires_); match_counter++)
        {
          ChannelSvcs::MatchWords match_words[ChannelSvcs::CT_MAX];
          Generics::Timer timer;
          Generics::CPUTimer cpu_timer;
          timer.start();
          cpu_timer.start();
          parse_keywords_(match_words[ChannelSvcs::CT_PAGE]);

          make_match_(url_words, match_words);
          cpu_timer.stop();
          timer.stop();
          Generics::Statistics::TimedSubject
            time_1(timer.elapsed_time()), cpu_time_1(cpu_timer.elapsed_time());
          time_stat_->consider(time_1);
          cpu_time_stat_->consider(cpu_time_1);
          if(verbose_level_ > 2)
          {
            std::cout << "Matched words: " << std::endl;
            size_t count = 0;
            for(ChannelSvcs::MatchWords::const_iterator it =
                match_words[ChannelSvcs::CT_PAGE].begin();
                it != match_words[ChannelSvcs::CT_PAGE].end(); ++it)
            {
              count++;
              const String::SubString& current = it->text();
              std::cout << count << " : " << current << std::endl;
            }
          }
        }
        end_time = Generics::Time::get_time_of_day();
        dump_statistic_(std::cout);

        return 0;
      }
      catch(const ChannelSvcs::ChannelContainer::Exception& e)
      {
        std::cerr << "caught eh::Exception: "
          << e.what() << std::endl;
        return 1;
      }
    }

    void MatchPerformanceLargeTest::generate_url_(
      std::string& word) /*throw(eh::Exception)*/
    {
      std::string host, path, part;
      size_t subdomains = Generics::safe_rand(1, 3);
      for(size_t i = 0; i < subdomains; i++)
      {
        if(i)
        {
          host.push_back('.');
        }
        ChannelServerTestCommons::generate_asc_word(host, 4);
      }
      size_t count = Generics::safe_rand(1, 6);
      if(count)
      {
        for(size_t i = 0; i < count; i++)
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

    void MatchPerformanceLargeTest::prepare_match_data_(
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

    int MatchPerformanceLargeTest::run(int argc, char* argv[]) noexcept
    {
      const char* FUN = "MatchPerformanceLargeTest::run";
      try
      {
        parse_arguments(argc, argv);

        if(word_length_ <= sentence_words_)
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
          int ret_value = execute_();
          std::cout << "4. Test finished" << std::endl;
          return ret_value;
        }
        else
        {
          std::cerr << "Length of soft word " << word_length_
            << " more than words in sentence " << sentence_words_
            << "." << std::endl;
        }
      }
      catch(const eh::Exception& e)
      {
        std::cerr << FUN << ": caught eh::Exception: "
          << e.what() << std::endl;
      }
      return 1;
    }

    int MatchPerformanceLargeTest::help() noexcept
    {
      std::cout << "MatchPerformanceLargeTest [options]" << std::endl
        << "options:" << std::endl
        << "-v[--verbose] - show additional info in process of test"
        << std::endl
        << "-u[--count-urls] - count url channels in container" << std::endl
        << "-h[--count-hard] - count hard words in container" << std::endl
        << "-s[--count-soft] - count soft words in container" << std::endl
        << "-q[--quires] - count quires in test" << std::endl
        << "-w[--word-length] - count words in soft word" << std::endl
        << "-U[--urls-length] - length of url in container" << std::endl
        << "-S[--soft-length] - length of word in soft word" << std::endl
        << "-H[--hard-length] - length of hard word" << std::endl
        << "-e[--sentence-words] - length of sentence in words" << std::endl
        << "-p[--percent-soft] - percent of soft words" << std::endl
        << "-c[--chunks] - count chunks in container" << std::endl
        << "-m[--match-percent] - percent of matching words" << std::endl
        << "-M[--match-size] - size of match text" << std::endl
        << "-b[--body] - body of query" << std::endl
        << "-t[--time] - if this option set, option --quires is ignored and "
        " instead uses time in seconds " << std::endl;
      return 1;
    }

  }
}

int main(int argc, char* argv[])
{
  AdServer::UnitTests::MatchPerformanceLargeTest test;
  if(argc > 1 && strcmp(argv[1], "help") == 0)
  {
    return test.help();
  }
  return test.run(argc, argv);
}

