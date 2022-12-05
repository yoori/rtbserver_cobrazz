#ifndef CHANNEL_UTILS_HPP
#define CHANNEL_UTILS_HPP

#include<string>
#include <Commons/CorbaAlgs.hpp>
#include <Commons/Constants.hpp>
#include <ChannelSvcs/ChannelCommons/CommonTypes.hpp>
#include <ChannelSvcs/ChannelCommons/ChannelCommons.hpp>
#include <ChannelSvcs/ChannelCommons/TriggerParser.hpp>
#include<Language/SegmentorCommons/SegmentorInterface.hpp>

namespace AdServer
{
namespace ChannelSvcs
{
  typedef const String::AsciiStringManip::Char2Category<',', '\n'>
    MatchBreakSeparators;
  typedef const String::AsciiStringManip::Char2Category<' ', '\t'>
    MatchSeparators;

  //stl types
  template<class SEQ, class OUT>
  void trace_sequence(
    const char* description,
    const SEQ& seq,
    OUT& out)
    /*throw(eh::Exception)*/
  {
    if(seq.empty())
    {
      return;
    }
    out << description << ": ";
    for(typename SEQ::const_iterator it = seq.begin(); it != seq.end(); it++)
    {
      if(it != seq.begin())
      {
        out << ',';
      }
      out << *it;
    }
    out << '.' << std::endl; 
  }

  enum ParseMode
  {
    PM_SIMPLIFY,
    PM_NO_SIMPLIFY
  };

  void build_combination(
    const std::vector<size_t>& positions,
    AdServer::ChannelSvcs::MatchWords& match_words,
    size_t max_seq_length)
    noexcept;

  template<typename SEPARATOR>
  void parse_keywords(
    const String::SubString& in,
    AdServer::ChannelSvcs::MatchWords& match_words,
    ParseMode mode,//make simplification or no
    const SEPARATOR* separators = nullptr,//additional separators
    size_t seq_length = Commons::DEFAULT_MAX_HARD_WORD_SEQ,
    StringVector* exact_words = 0,
    const Language::Segmentor::SegmentorInterface* segmentor = 0)
    noexcept
    {
      try
      {
        String::SubString token, parsed_token;
        std::vector<size_t> positions;
        std::string subword;
        bool make_split;
        match_words.data_holder_.reserve(in.length() * 4 + 2);
        String::StringManip::Splitter<SEPARATOR>
          splitter(!separators ? String::SubString() : in);
        //should be enough memory for any result of parsing
        //only one allocation
        if(!separators)
        {
          make_split = true;
          token = in;
        }
        else
        {
          make_split = splitter.get_token(token);
        }
        while(make_split)
        {
          if(mode == PM_SIMPLIFY)
          {
            Language::Trigger::normalize_phrase(token, subword, segmentor);
            token = subword;
          }
          String::StringManip::Splitter<MatchSeparators> splitter2(token);
          positions.push_back(match_words.data_holder_.length());
          while(splitter2.get_token(parsed_token))
          {
            if(parsed_token.length() <= TriggerParser::TriggerParser::MAX_TRIGGER_LENGTH)
            {
              match_words.data_holder_.append(parsed_token.data(), parsed_token.length());
              match_words.data_holder_.push_back(' ');
              positions.push_back(match_words.data_holder_.length());
            }
            else if(exact_words)
            {
              exact_words->clear();
              exact_words = nullptr;
            }
          }
          if(exact_words)
          {
            auto it = positions.begin();
            auto pos = *it;
            for(++it; it != positions.end(); ++it)
            {
              exact_words->emplace_back(match_words.data_holder_, pos, *it - pos - 1);
              pos = *it;
            }
          }
          build_combination(positions, match_words, seq_length);
          positions.clear();
          make_split = splitter.get_token(token);
        }
      }
      catch(const eh::Exception& e)
      {
      }
    }
}
}

#endif

