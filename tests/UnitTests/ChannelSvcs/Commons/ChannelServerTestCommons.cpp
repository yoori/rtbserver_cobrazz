#include <getopt.h>
#include <malloc.h>
#include <string>
#include <Generics/Rand.hpp>
#include <String/UnicodeSymbol.hpp>
#include <String/UTF8IsProperty.hpp>
#include <HTTP/UrlAddress.hpp>
#include <Commons/Constants.hpp>
#include "ChannelServerTestCommons.hpp"

namespace
{
  const String::UnicodeSymbol BASE_SYMBOL = String::UnicodeSymbol::random();
  const std::size_t ALPHABET_SIZE=26;
  String::UnicodeSymbol CHARS[ALPHABET_SIZE];

  struct AlphabetInit
  {
    AlphabetInit()
    {
      String::UnicodeSymbol sym = BASE_SYMBOL;
      for (std::size_t i = 0; i < ALPHABET_SIZE;)
      {
        if(String::is_digit(sym.c_str()) || String::is_letter(sym.c_str()))
        {
          CHARS[i++] = sym;
        }
        try
        {
          ++sym;
        }
        catch(...)
        {
          sym = String::UnicodeSymbol::random();
        }
      }
    }
  };

  AlphabetInit initializer;

  template <std::size_t divider>
  uint32_t
  randomizer()
  {
    static uint32_t rand_cache = 0;

    if (!rand_cache)
    {
      rand_cache = Generics::safe_rand();
    }
    uint32_t result = rand_cache % divider;
    rand_cache /= divider;
    return result;
  }
}


namespace AdServer
{
namespace UnitTests
{
  std::string& ChannelServerTestCommons::generate_uid_word(std::string& word)
    /*throw(eh::Exception)*/
  {
    Generics::Uuid uid = Generics::Uuid::create_random_based();
    word = uid.to_string();
    return word;
  }

  std::string& ChannelServerTestCommons::generate_word(
    std::string& word,
    size_t length)
    /*throw(eh::Exception)*/
  {
    const std::size_t MAX_LEN = 10;
    if(!length)
    {
      length = 1 + randomizer<MAX_LEN - 1>();
    }
    word.clear();
    word.reserve(length * BASE_SYMBOL.length());
    for (size_t i = 0; i < length; ++i)
    {
      const String::UnicodeSymbol& sym = CHARS[randomizer<ALPHABET_SIZE>()];
      word.append(sym.c_str(), sym.length());
    }
    return word;
  }

  /**
   * return true, if wrote whole word
   */

  bool
  ChannelServerTestCommons::generate_asc_word(
    FixedBuf& word,
    size_t length)
    /*throw(eh::Exception)*/
  {
    std::size_t growth = std::min(word.max_size() - word.size(), length);
    bool result = growth == length;
    while (growth--)
    {
      char next_char = 'a' + randomizer<'z' - 'a'>();
      word.append(&next_char, 1);
    }
    return result;
  }

  void
  ChannelServerTestCommons::generate_asc_word(
    std::string& word,
    size_t length)
    /*throw(eh::Exception)*/
  {
    word.reserve(word.size() + length);
    while (length--)
    {
      word.push_back('a' + randomizer<'z' - 'a'>());
    }
  }

  void ChannelServerTestCommons::generate_url(
    std::string& word,
    size_t domain_length,
    size_t path_length,
    size_t path_steps)
    /*throw(eh::Exception)*/
  {
    std::string url_str;
    url_str.reserve(domain_length + (path_length + 1) * path_steps);
    generate_asc_word(url_str, domain_length);
    for(size_t i = 0; i < path_steps; i++)
    {
      url_str.push_back('/');
      generate_asc_word(url_str, path_length);
    }
    HTTP::BrowserAddress url(url_str);
    url.get_view(
      (HTTP::HTTPAddress::VW_HOSTNAME | HTTP::HTTPAddress::VW_PATH |
       HTTP::HTTPAddress::VW_QUERY),
      word);
  }

  void ChannelServerTestCommons::print_result(
    std::ostream& out,
    const ChannelSvcs::TriggerMatchRes& res)
    noexcept
  {
    out << " ids = ";
    for(auto it = res.begin(); it != res.end(); ++it)
    {
      const ChannelSvcs::TriggerMatchItem& item = it->second;
      for(size_t j = 0; j < ChannelSvcs::CT_MAX; j++)
      {
        for(size_t k = 0; k < item.trigger_ids[j].size(); k++)
        {
          out << " " << item.trigger_ids[j][k];
        }
      }
    }
    out << std::endl;
  }

  ChannelSvcs::MergeAtom& ChannelServerTestCommons::new_atom(
    unsigned long id,
    size_t type,
    ChannelSvcs::MergeAtom& atom,
    ChannelSvcs::ChannelIdToMatchInfo* info)
    /*throw(eh::Exception)*/
  {
    atom.id = id;
    if(info)
    {
      ChannelSvcs::MatchInfo& match_info = (*info)[id];
      match_info.channel = ChannelSvcs::Channel(id);
      match_info.channel.mark_type(type);
      match_info.channel.mark_type(ChannelSvcs::Channel::CT_ACTIVE);
    }
    return atom;
  }

  TestTemplate::TestTemplate() noexcept
    : body_(false),
      count_urls_(100),
      count_hards_(1000),
      count_soft_(1000),
      urls_length_(10),
      hard_length_(6),
      sentence_words_(6),
      split_percent_(50),
      verbose_level_(0),
      soft_length_(6),
      word_length_(3),
      count_chunks_(32),
      quires_(1000),
      res_size_(0),
      zero_match_(0),
      data_size_(512),
      match_percent_(10),
      avg_limit_(100000),
      time_(30),
      container_(count_chunks_),
      time_stat_(new Generics::Statistics::TimedStatSink),
      cpu_time_stat_(new Generics::Statistics::TimedStatSink)
  {
  }

  void TestTemplate::dump_memory_statistic_(std::ostream& ost) noexcept
  {
    struct mallinfo info;
    info = mallinfo();
    ost << "Memory information: " << std::endl;
    ost << "Total size: " << info.arena << std::endl;
    ost << "Unused chunks: " << info.ordblks << std::endl;
    ost << "Allocated chunks with mmap: " << info.hblks << std::endl;
    ost << "Allocated bytes with mmap: " << info.hblkhd << std::endl;
    ost << "Total chunks: " << info.uordblks << std::endl;
    ost << "Free chunks: " << info.fordblks << std::endl;
    ost << "Size of top chunk: " << info.keepcost << std::endl;
  }

  void TestTemplate::dump_statistic_(
    std::ostream& ost,
    Generics::Statistics::TimedStatSink* time_stat,
    Generics::Statistics::TimedStatSink* cpu_time_stat)
    noexcept
  {
    ost << "Time: " << std::endl;
    if(time_stat)
    {
      time_stat->dump(ost);
    }
    else
    {
      time_stat_->dump(ost);
    }
    ost << "CPU time: " << std::endl;
    if(cpu_time_stat)
    {
      cpu_time_stat->dump(ost);
    }
    else
    {
      cpu_time_stat_->dump(ost);
    }
    ost << "Size of matched data = " << res_size_
      << ", not matched quires = " << zero_match_ << '.' << std::endl;
    dump_memory_statistic_(ost);
  }

  void TestTemplate::make_match_(
    const ChannelSvcs::MatchUrls& urls,
    const ChannelSvcs::MatchWords words[ChannelSvcs::CT_MAX])
    noexcept
  {
    ChannelSvcs::TriggerMatchRes res;
    container_.match(
      urls,
     ChannelSvcs::MatchUrls(),
      words,
      ChannelSvcs::MatchWords(),
      ChannelSvcs::StringVector(),
      Generics::Uuid(),
      ChannelSvcs::MF_ACTIVE, res);
    size_t res_size = 0;
    for(auto it = res.begin(); it != res.end(); ++it)
    {
      const ChannelSvcs::TriggerMatchItem& item = it->second;
      for(size_t j = 0; j < ChannelSvcs::CT_MAX; j++)
      {
        res_size += item.trigger_ids[j].size();
      }
    }
    if(res_size)
    {
      res_size_ += res_size;
      if(verbose_level_ > 1)
      {
        ChannelServerTestCommons::print_result(std::cout, res);
      }
    }
    else
    {
      zero_match_++;
    }
  }

  void TestTemplate::parse_arguments(int argc, char* argv[])
    noexcept
  {
    struct option long_options[] =
    {
      {"verbose", no_argument, 0, 'v'},
      {"count-urls", required_argument, 0, 'u'},
      {"count-hard", required_argument, 0, 'h'},
      {"count-soft", required_argument, 0, 's'},
      {"quires", required_argument, 0, 'q'},
      {"word-length", required_argument, 0, 'w'},
      {"urls-length", required_argument, 0, 'U'},
      {"soft-length", required_argument, 0, 'S'},
      {"hard-length", required_argument, 0, 'H'},
      {"sentence-words", required_argument, 0, 'e'},
      {"percent-soft", required_argument, 0, 'p'},
      {"chunks", required_argument, 0, 'c'},
      {"match-percent", required_argument, 0, 'm'},
      {"match-size", required_argument, 0, 'M'},
      {"time", required_argument, 0, 't'},
      {"body", required_argument, 0, 'b'},
      {0, 0, 0, 0}
    };
    std::string body;
    {
      int opt, index=0;
      do
      {
        opt = getopt_long(
          argc,
          argv,
          "vu:h:s:q:w:U:S:H:e:p:c:m:M:t:b:",
          long_options,
          &index);
        switch(opt)
        {
          case 'v':
            verbose_level_++;
            break;
          case 'c':
            read_number(optarg, 0UL, ULONG_MAX, count_chunks_);
            break;
          case 'u':
            read_number(optarg, 0UL, ULONG_MAX, count_urls_);
            break;
          case 'U':
            read_number(optarg, 0UL, 1024UL, urls_length_);
            break;
          case 'h':
            read_number(optarg, 0UL, ULONG_MAX, count_hards_);
            break;
          case 's':
            read_number(optarg, 0UL, ULONG_MAX, count_soft_);
            break;
          case 'q':
            read_number(optarg, 1UL, ULONG_MAX, quires_);
            break;
          case 'w':
            read_number(optarg, 0UL, ULONG_MAX, word_length_);
            break;
          case 'S':
            read_number(
              optarg,
              1UL,
              Commons::DEFAULT_MAX_HARD_WORD_SEQ,
              soft_length_);
            break;
          case 'H':
            read_number(optarg, 0UL, ULONG_MAX, hard_length_);
            break;
          case 'e':
            read_number(optarg, 0UL, ULONG_MAX, sentence_words_);
            break;
          case 'p':
            read_number(optarg, 0UL, 100UL, split_percent_);
            break;
          case 'm':
            read_number(optarg, 0UL, 100UL, match_percent_);
            break;
          case 'M':
            if(!body_)
            {
              read_number(optarg, 512UL, ULONG_MAX, data_size_);
            }
            break;
          case 'b':
            body = optarg;
            match_data_.reset_max_size(body.size() + 1);
            match_data_.append(body.c_str(), body.size() + 1);
            data_size_ = body.size();
            body_ = true;
            break;
          case 't':
            read_number(optarg, (time_t)1, (time_t)3600, time_);
            break;
        }
      } while(opt != -1);
      ChannelSvcs::ChannelIdToMatchInfo_var info =
        new ChannelSvcs::ChannelIdToMatchInfo;
    }
  }
}
}
