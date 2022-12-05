#include<iostream>
#include<vector>
#include<eh/Exception.hpp>
#include<Commons/CorbaAlgs.hpp>
#include<Generics/Statistics.hpp>
#include<Generics/Rand.hpp>
#include<ChannelSvcs/ChannelCommons/ChannelUtils.hpp>
#include"MatchingCheck.hpp"

namespace AdServer
{
namespace UnitTests
{
  void MatchingCheckTest::prepare_data_(SaveType& atom_save)
    /*throw(eh::Exception)*/
  {
    ChannelSvcs::UpdateContainer update_container(&container_, 0);
    ChannelSvcs::ChannelIdToMatchInfo_var info = 
      new ChannelSvcs::ChannelIdToMatchInfo;
    std::string base_word, ending;
    unsigned long channel_trigger_id = 10;
    unsigned long atom_id = 10;
    //complex hard words with one same word
    for(size_t j = 0; j < 4; j++)
    {
      ChannelSvcs::MergeAtom atom;
      ChannelServerTestCommons::new_atom(
        atom_id++, ChannelSvcs::CT_PAGE, atom, info);
      ChannelServerTestCommons::generate_word(base_word, hard_length_);
      for(size_t i_hard = 0; i_hard < count_hards_; i_hard++)
      {
        std::string trigger_str;
        trigger_str.reserve(base_word.size() + (2 + hard_length_) * j);
        trigger_str.push_back('"');
        trigger_str.append(base_word);
        for(size_t k = 0; k < j; k++)
        {
          ChannelServerTestCommons::generate_word(ending, hard_length_);
          trigger_str.push_back(' ');
          trigger_str.append(ending);
        }
        trigger_str.push_back('"');
        ChannelSvcs::TriggerParser::TriggerParser::parse_word(
          atom.id, channel_trigger_id++, 'P',
          trigger_str,
          false, 0, atom.soft_words, Commons::DEFAULT_MAX_HARD_WORD_SEQ, 0);
        atom_save[atom.id].push_back(trigger_str);
      }
      update_container.add_trigger(atom);
    }
    //soft words
    {
      ChannelSvcs::MergeAtom atom;
      ChannelServerTestCommons::new_atom(
        atom_id++, ChannelSvcs::CT_PAGE, atom, info);
      for(size_t i_soft = 0; i_soft < count_soft_; i_soft++)
      {
        std::string word, trigger_str;
        trigger_str.reserve(2 + soft_length_ * 20);
        for(size_t k = 0; k < soft_length_; k++)
        {
          if(k != 0)
          {
            trigger_str.push_back(' ');
          }
          ChannelServerTestCommons::generate_word(word);
          trigger_str.append(word);
        }
        ChannelSvcs::TriggerParser::TriggerParser::parse_word(
          atom.id, channel_trigger_id++, 'P', trigger_str,
          false, 0, atom.soft_words, Commons::DEFAULT_MAX_HARD_WORD_SEQ, 0);
        atom_save[atom.id].push_back(trigger_str);
      }
      update_container.add_trigger(atom);
    }
    //soft words with one same word
    {
      ChannelSvcs::MergeAtom atom;
      ChannelServerTestCommons::new_atom(
        atom_id++, ChannelSvcs::CT_PAGE, atom, info);
      for(size_t i_soft = 0; i_soft < count_soft_; i_soft++)
      {
        std::string word, trigger_str;
        trigger_str.reserve(2 + soft_length_ * 21 + base_word.size());
        for(size_t k = 0; k < soft_length_; k++)
        {
          trigger_str.push_back(' ');
          ChannelServerTestCommons::generate_word(word);
          trigger_str.push_back('"');
          trigger_str.append(word);
          trigger_str.push_back(' ');
          trigger_str.append(base_word);
          trigger_str.push_back('"');
        }
        ChannelSvcs::TriggerParser::TriggerParser::parse_word(
          atom.id, channel_trigger_id++, 'P',
          trigger_str, false, 0,
          atom.soft_words, Commons::DEFAULT_MAX_HARD_WORD_SEQ, 0);
        atom_save[atom.id].push_back(trigger_str);
      }
      update_container.add_trigger(atom);
    }
    std::cout << "Simple atoms prepared" << std::endl;
    dump_memory_statistic_(std::cout);
    //special case
    {
      ChannelSvcs::MergeAtom atom;
      std::vector<std::string> words(101);
      for(std::vector<std::string>::iterator it = words.begin();
          it != words.end(); ++it)
      {
        ChannelServerTestCommons::generate_asc_word(*it, 10);
      }
      ChannelServerTestCommons::new_atom(
        atom_id++, ChannelSvcs::CT_PAGE, atom, info);
      for(size_t step = 1; step < words.size(); step++)
      {
        std::string trigger_str;
        for(size_t i = step + 1; i < words.size(); i++)
        {
          trigger_str += words[step];
          trigger_str.push_back(' ');
          trigger_str += words[i];
        }
        ChannelSvcs::TriggerParser::TriggerParser::parse_word(
          atom.id, channel_trigger_id++, 'P', trigger_str,
          false, 0, atom.soft_words, Commons::DEFAULT_MAX_HARD_WORD_SEQ, 0);
        trigger_str.clear();
      }
      for(size_t i = 0; i < 1000; i++)
      {
        std::string trigger_str;
        size_t num = 0;
        for(size_t j = 0; j < 3; j++)
        {
          if(j != 0)
          {
            trigger_str.push_back(' ');
          }
          num = Generics::safe_rand(num, 100);
          trigger_str += words[num];
        }
        atom_save[atom.id].push_back(trigger_str);
      }
      update_container.add_trigger(atom);
    }
    std::cout << "All atoms prepared" << std::endl;
    dump_memory_statistic_(std::cout);
    container_.merge(update_container, *info, true);
    std::cout << "Atoms merged" << std::endl;
  }

  void MatchingCheckTest::reset_staitstic_() /*throw(eh::Exception)*/
  {
    time_stat_->reset();
    cpu_time_stat_->reset();
    zero_match_ = 0;
    res_size_ = 0;
  }

  void MatchingCheckTest::match_i_(const String::SubString& in) /*throw(eh::Exception)*/
  {
    Generics::Time t1, t2;
    ChannelSvcs::MatchUrls url_words;
    {
      Generics::ScopedTimer timer1(t1);
      Generics::ScopedCPUTimer timer2(t2);

      AdServer::ChannelSvcs::MatchWords phrases[ChannelSvcs::CT_MAX];
      for(size_t match_counter = 0; match_counter < 100; match_counter++)
      {
        ChannelSvcs::parse_keywords<ChannelSvcs::MatchBreakSeparators>(
          in, phrases[ChannelSvcs::CT_PAGE], ChannelSvcs::PM_SIMPLIFY);
        make_match_(url_words, phrases);
      }
    }
    Generics::Statistics::TimedSubject time_1(t1), cpu_time_1(t2);
    time_stat_->consider(time_1);
    cpu_time_stat_->consider(cpu_time_1);
  }

  void MatchingCheckTest::make_check_(const SaveType& atoms)
    /*throw(eh::Exception)*/
  {
    //stat_->consider(Generics::Statistics::TimedSubject( Generics::Time(0)));
    for(SaveType::const_iterator i = atoms.begin(); i != atoms.end(); ++i)
    {
      const std::vector<std::string>& strings = i->second;
      for(std::vector<std::string>::const_iterator it = strings.begin();
          it != strings.end(); ++it)
      {
        match_i_(*it);
      }
      std::cout << "Match(" << i->first << "):" << std::endl;
      dump_statistic_(std::cout);
      reset_staitstic_();
    }
  }

  int MatchingCheckTest::run() noexcept
  {
    try
    {
      SaveType atoms;
      std::cout << "Start test" << std::endl;
      dump_memory_statistic_(std::cout);
      prepare_data_(atoms);
      std::cout << "Data prepared" << std::endl;
      dump_memory_statistic_(std::cout);
      make_check_(atoms);
      dump_memory_statistic_(std::cout);
      std::cout << "Test finished" << std::endl;
    }
    catch(const eh::Exception& e)
    {
      std::cerr << "MatchingCheckTest: caught eh::Exception : " << e.what()
        << std::endl;
      return 1;
    }
    return 0;
  }
}
}

int main(int argc, char* argv[])
{
  AdServer::UnitTests::MatchingCheckTest test;
  test.parse_arguments(argc, argv);
  return test.run();
}

