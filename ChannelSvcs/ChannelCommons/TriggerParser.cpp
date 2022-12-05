
#include<vector>
#include<string>
#include<iterator>
#include<sstream>
//#include<iostream>
#include<eh/Exception.hpp>
#include<String/UTF8Case.hpp>
#include<String/AsciiStringManip.hpp>
#include<Generics/Time.hpp>
#include<Stream/MemoryStream.hpp>
#include<HTTP/UrlAddress.hpp>
#include<Logger/Logger.hpp>
#include<Language/SegmentorCommons/SegmentorInterface.hpp>
#include<Language/BLogic/NormalizeTrigger.hpp>
#include <Commons/Constants.hpp>
#include"CommonTypes.hpp"
#include"TriggerParser.hpp"

namespace AdServer
{
namespace ChannelSvcs
{
  namespace TriggerParser
  {

  const unsigned long TriggerParser::MAX_TRIGGER_LENGTH =
    Commons::MAX_WORD_LENGTH;
  const unsigned long TriggerParser::MAX_URL_LENGTH = 2048;
  const unsigned long TriggerParser::WORSE_MULT = 10;

  ParseTracer::ParseTracer(bool active) noexcept
    : active_(active)
  {
  }

  void ParseTracer::start() noexcept
  {
    if(active_)
    {
      timer_.start();
      cpu_timer_.start();
    }
  }

  void ParseTracer::get_time(Generics::Time& time, Generics::Time& cpu_time)
    noexcept
  {
    if(active_)
    {
      timer_.stop();
      cpu_timer_.stop();
      time =  timer_.elapsed_time();
      cpu_time = cpu_timer_.elapsed_time();
    }
  }

  void ParseTracer::add_time(Generics::Time& time, Generics::Time& cpu_time)
    noexcept
  {
    if(active_)
    {
      timer_.stop();
      cpu_timer_.stop();
      time =  timer_.elapsed_time();
      cpu_time = cpu_timer_.elapsed_time();
    }
  }

  bool get_url_view(
    const String::SubString& url_sub,
    const std::set<unsigned short>& allow_ports,
    bool &exact,
    std::string& out) noexcept
  {
    String::SubString cur_url = url_sub;
    if(cur_url.size() > 1 &&
       (*cur_url.begin() == '"' || *cur_url.begin() == '[') &&
       (*(cur_url.begin() + cur_url.size() - 1) == '"' ||
        *(cur_url.begin() + cur_url.size() - 1) == ']'))
    {
      cur_url.erase_front(1);
      cur_url.erase_back(1);
      exact = true;
    }
    else
    {
      exact = false;
    }
    try
    {
      HTTP::BrowserAddress http_address(cur_url);
      http_address.get_view(
        (HTTP::HTTPAddress::VW_HOSTNAME | HTTP::HTTPAddress::VW_PATH |
          HTTP::HTTPAddress::VW_QUERY ),
        out);
      if(!http_address.is_default_port() &&
         allow_ports.find(http_address.port_number()) == allow_ports.end())
      {
        return false;
      }
    }
    catch(const HTTP::URLAddress::InvalidURL& e)
    {
      return false;
    }
    catch(const eh::Exception& e)
    {
      return false;
    }

    if(out.find('/') == std::string::npos)
    {
      return false;
    }
    String::AsciiStringManip::to_lower(out.begin(), out.end());
    return true;
  }

  size_t TriggerParser::parse_triggers(
    unsigned int id,
    const std::string& lang,
    const TriggerList& triggers,
    const Language::Segmentor::SegmentorInterface* segmentor,
    const std::set<unsigned short>& allow_ports,
    MergeContainer* out,
    size_t seq_length,
    Logging::Logger* logger)
    /*throw(Exception)*/
  {
    MergeAtom atom;
    atom.id = id;
    atom.lang = lang;
    for(TriggerList::const_iterator it = triggers.begin();
        it != triggers.end(); ++it)
    {
      if(it->type == 'U')
      {
        if(it->trigger.size() <= MAX_URL_LENGTH)
        {
          parse_url(
            id,
            it->channel_trigger_id,
            it->trigger,
            it->negative, allow_ports, atom.soft_words, logger);
        }
      }
      else if(it->type == 'D')
      {
        if(!it->trigger.empty())
        {
          parse_uids(id, it->trigger, atom.uids_trigger, logger);
        }
      }
      else
      {
        if(it->trigger.size() <= MAX_TRIGGER_LENGTH)
        {
          parse_word(
            id,
            it->channel_trigger_id,
            it->type,
            it->trigger,
            it->negative,
            segmentor, atom.soft_words, seq_length, logger);
        }
      }
    }
      //atom.print(std::cout);
      //std::cout << std::endl;
    return out->add_trigger(atom);
  }

  void TriggerParser::parse_word(
    IdType channel_id,
    unsigned int channel_trigger_id,
    char type,
    const std::string& trigger,
    bool negative,
    const Language::Segmentor::SegmentorInterface* segmentor,
    SoftTriggerList& soft_words,
    size_t seq_length,
    Logging::Logger* logger)
    /*throw(Exception)*/
  {
    try
    {
      Language::Trigger::Trigger trigger_view;

      try
      {
        Language::Trigger::normalize(
          trigger, trigger_view, segmentor);
      }
      catch(const eh::Exception& e)
      {
        if(logger)
        {
          logger->sstream(
            Logging::Logger::WARNING, "TraffickingProblem", "ADS-TF-5")
            << "On parsing '" << trigger << "' (" << channel_id << ','
            << channel_trigger_id << ") Caught eh::Exception : " << e.what();
        }
        return;
      }
      size_t parts_size = trigger_view.parts.size();
      if(parts_size <= seq_length)
      {
        if(trigger_view.exact && type != 'S')
        {
          if(logger)
          {
            logger->sstream(
              Logging::Logger::WARNING, "TraffickingProblem", "ADS-TF-5")
            << "On parsing '" << trigger << "' (" << channel_id << ','
            << channel_trigger_id << ") is exact. Exact triggers are "
            "allowed only for search keywords";
          }
        }
        else if(trigger_view.parts.empty())
        {
          if(logger)
          {
            logger->sstream(
              Logging::Logger::WARNING, "TraffickingProblem", "ADS-TF-5")
              << "Trigger '" << trigger << "' (" << channel_id << ','
              << channel_trigger_id << ") doesn't have parts after "
              "normalization.";
          }
        }
        else
        {
          soft_words.emplace_back(SoftTriggerWord());
          SoftTriggerWord& word = soft_words.back();
          word.channel_trigger_id = channel_trigger_id;
          Serialization::serialize(
            trigger_view.parts,
            type,
            trigger_view.exact,
            negative,
            word.trigger);
        }
      }
      else
      {
        if(logger)
        {
          logger->sstream(
            Logging::Logger::WARNING, "TraffickingProblem", "ADS-TF-5")
            << "Trigger '" << trigger << "' (" << channel_id << ','
            << channel_trigger_id << ") ignored because has " << parts_size
            << " words, maximum is " << seq_length;
        }
      }
    }
    catch(const eh::Exception& e)
    {
      if(logger)
      {
        logger->sstream(
          Logging::Logger::WARNING, "TraffickingProblem", "ADS-TF-5")
          << "On merging trigger '" << trigger << "' (" << channel_id << ','
          << channel_trigger_id << "), caught exception : " << e.what();
      }
    }
  }

  struct UidAdaptor
  {
  public:
    bool is_quoted(const String::SubString&) noexcept
    {
      return false;
    }
    const String::SubString& get_part(const String::SubString& part) noexcept
    {
      return part;
    }
  };

  void TriggerParser::parse_uids(
    IdType channel_id,
    const std::string& trigger,
    std::string& uids_trigger,
    Logging::Logger* logger)
    /*throw(Exception)*/
  {
    try
    {
      String::StringManip::SplitNL splitter(trigger);
      String::SubString token;
      SubStringSet uids;
      while(splitter.get_token(token))
      {
        try
        {//validate uid
          Generics::Uuid uuid(token, false);
          uids.insert(uids.end(), token);
        }
        catch(const eh::Exception& e)
        {
          if(logger)
          {
            logger->sstream(
              Logging::Logger::WARNING, "TraffickingProblem", "ADS-TF-6")
              << "On parsing uid '" << token << "' for channel " << channel_id
              << " exception : " << e.what();
          }
        }
      }
      Serialization::serialize(
        uids,
        'D',
        false,
        false,
        Serialization::StringAllocator(uids_trigger),
        uids_trigger,
        UidAdaptor());
    }
    catch(const eh::Exception& e)
    {
      Stream::Error err;
      err << __func__ << ": error on parsing channel " << channel_id
        << " :"  << e.what();
      throw Exception(err);
    }
  }

  void TriggerParser::parse_url(
    IdType channel_id,
    unsigned int channel_trigger_id,
    const std::string& trigger,
    bool negative,
    const std::set<unsigned short>& allow_ports,
    SoftTriggerList& url_words,
    Logging::Logger* logger)
    /*throw(Exception)*/
  {
    try
    {
      std::string trigger_view;
      Parts parts(2);
      bool exact;
      if(get_url_view(
          trigger, allow_ports, exact, trigger_view) &&
          !trigger_view.empty())
      {
        url_words.push_back(SoftTriggerWord());
        url_words.back().channel_trigger_id = channel_trigger_id;
        size_t query_pos = trigger_view.find('?');
        size_t count = trigger_view.find_last_of('/', query_pos);
        parts[0].part = String::SubString(trigger_view.data(), count);
        parts[0].quotes = false;
        parts[1].part = String::SubString(trigger_view.data() + count);
        parts[1].quotes = false;
        Serialization::serialize(
          parts, 'U', exact, negative, url_words.back().trigger);
      }
    }
    catch(const eh::Exception& e)
    {
      if(logger)
      {
        logger->sstream(
          Logging::Logger::WARNING, "TraffickingProblem", "ADS-TF-5")
          << "On parsing trigger '" << trigger << "' (" << channel_id << ','
          << channel_trigger_id << "), caught exception : " << e.what();
      }
    }
  }


  }
}// namespace ChannelSvcs
}

