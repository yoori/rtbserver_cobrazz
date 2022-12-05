#include <Generics/Rand.hpp>
#include<Language/BLogic/NormalizeTrigger.hpp>
#include<ChannelSvcs/ChannelCommons/CommonTypes.hpp>
#include<ChannelSvcs/ChannelCommons/Serialization.hpp>
#include<tests/UnitTests/ChannelSvcs/Commons/ChannelServerTestCommons.hpp>
#include"TriggerSerialization.hpp"

namespace AdServer
{
namespace UnitTests
{
  void TriggerSerializationTest::generate_soft_trigger_word(
    size_t iteration,
    size_t max_parts,
    size_t max_word_len,
    SoftWord& word,
    std::string& trigger)
    noexcept
  {
    word.channel_trigger_id = 0;
    if(trigger_type_)
    {
      word.type = trigger_type_;
    }
    else
    {
      size_t type_num = Generics::safe_rand(5); 
      switch(type_num)
      {
        case 0:
          word.type = 'U';
          break;
        case 1:
          word.type = 'P';
          break;
        case 2:
          word.type = 'S';
          break;
        case 3:
          word.type = 'R';
          break;
        case 4:
          word.type = 'D';
          break;
      }
    }
    word.exact = Generics::safe_rand(2); 
    if(word.type == 'U')
    {
      if(trigger_.empty())
      {
        size_t domain_length = 3 + Generics::safe_rand(10); 
        size_t path_length = 1 + Generics::safe_rand(10); 
        size_t path_segments = Generics::safe_rand(3); 
        ChannelServerTestCommons::generate_url(
          word.trigger, domain_length, path_length, path_segments);
        if(word.exact)
        {
          word.trigger.insert(word.trigger.begin(), '[');
          word.trigger.push_back(']');
        }
      }
      else
      {
        word.trigger = trigger_;
        word.exact = (trigger_[0] == '[');
      }
      word.parts.resize(1);
      ChannelSvcs::Parts::iterator part_it = word.parts.begin();
      if(word.exact)
      {
        part_it->part =
          String::SubString(word.trigger.c_str() + 1, word.trigger.size() - 2);
      }
      else
      {
        part_it->part = word.trigger;
      }
      part_it->quotes = word.exact;
    }
    else if(word.type == 'D')
    {
      word.channel_trigger_id += iteration;
      if(trigger_.empty())
      {
        size_t count_parts = Generics::safe_rand(max_parts) + 1; 
        size_t offset = 0;
        std::vector<size_t> sizes;
        size_t i;
        for(size_t i = 0; i < count_parts; i++)
        {
          std::string trigger_str;
          ChannelServerTestCommons::generate_uid_word(trigger_str);
          sizes.push_back(trigger_str.length());
          if(i != 0)
          {
            word.trigger.push_back(' ');
          }
          word.trigger += trigger_str;
        }
        word.parts.resize(count_parts);
        i = 0;
        for(ChannelSvcs::Parts::iterator part_it = word.parts.begin();
            part_it != word.parts.end(); part_it++, i++)
        {
          part_it->part =
            String::SubString(word.trigger.c_str() + offset, sizes[i]);
          part_it->quotes = false;
          offset += sizes[i] + 1;
        }
      }
      else
      {
        word.trigger = trigger_;
      }
      word.exact = false;
    }
    else
    {
      size_t count_parts = Generics::safe_rand(max_parts + 1); 
      if(count_parts)
      {
        if(trigger_.empty())
        {
          trigger.reserve((max_parts + 1) * count_parts + 2);
          if(word.exact)
          {
            trigger.push_back('[');
          }
          for(size_t i = 0; i < count_parts; i++)
          {
            std::string trigger_str;
            ChannelServerTestCommons::generate_word(trigger_str, max_word_len);
            bool quoted = Generics::safe_rand(2); 
            if(i != 0)
            {
              trigger.push_back(' ');
            }
            if(quoted)
            {
              trigger.push_back('"');
            }
            trigger += trigger_str;
            if(quoted)
            {
              trigger.push_back('"');
            }
          }
          if(word.exact)
          {
            trigger.push_back(']');
          }
        }
        else
        {
          trigger = trigger_;
          word.exact = (trigger_[0] == '[');
        }
        Language::Trigger::Trigger trigger_view;
        try
        {
          Language::Trigger::normalize(trigger, trigger_view);
        }
        catch(const eh::Exception& e)
        {
          std::cerr << "eh::Exception: " << e.what() 
            << " on parsing " << trigger << std::endl;
        }
        word.trigger.swap(trigger_view.trigger);
        word.parts.swap(trigger_view.parts);
        word.exact = trigger_view.exact;
      }
    }
    std::cout << iteration << ':' << word.type << ":trigger = "
      << word.trigger << std::endl;
    word.channel_trigger_id += Generics::safe_rand() % 5; 
  }

  int TriggerSerializationTest::print_compary_parts_(
    size_t iteration,
    const ChannelSvcs::Parts& parts1,
    const ChannelSvcs::SubStringVector& parts2,
    const char *result,
    size_t len)
    noexcept
  {
    int ret_value = 0;
    bool print_parts = false;
    if(parts1.size() != parts2.size())
    {
      print_parts = true;
    }
    if(!print_parts)
    {
      for(size_t i = 0; i < parts1.size(); i++)
      {
        if(parts1[i].part != parts2[i])
        {
          print_parts = true;
          break;
        }
      }
      for(size_t i = 0; i < parts1.size(); i++)
      {
        bool quotes = ChannelSvcs::Serialization::quoted(result, i);
        if(parts1[i].quotes != quotes)
        {
          std::cerr << "Quoted for " << i << " are different, "
           << parts1[i].quotes << " and " << quotes << std::endl;
          ret_value++;
          break;
        }
      }
    }
    if(print_parts)
    {
      std::cerr << iteration << ": parts are different, left side:" << std::endl;
      for(size_t i = 0; i < parts1.size(); i++)
      {
        std::cerr << i << "'" << parts1[i].part << "'" << std::endl;
      }
      std::cerr << "right side:" << std::endl;
      for(size_t i = 0; i < parts2.size(); i++)
      {
        std::cerr << i << "'" << parts2[i] << "'" << std::endl;
      }
      std::cerr << "Data: "  << std::setw(2) << std::setfill('0') << std::hex;
      for(size_t i = 0; i < len; i++)
      {
        std::cerr << static_cast<unsigned int>(
          reinterpret_cast<const unsigned char*>(result)[i]) << ' ';
      }
      std::cerr << std::endl;
      ret_value++;
    }
    return ret_value;
  }

  int TriggerSerializationTest::print_compary_result_(
    const SoftWord& word1,
    const std::string& trigger_in,
    char type,
    const std::string& trigger,
    bool exact,
    bool negative) noexcept
  {
    int res = 0;
    if(trigger_in != trigger)
    {
      std::cerr << word1.channel_trigger_id
        << " triggers are different: '";
      std::cerr << word1.trigger;
      std::cerr << "' and '";
      std::cerr << trigger;
      std::cerr << "'" << std::endl;
      res++;
    }
    if(word1.type != type)
    {
      std::cerr << word1.channel_trigger_id
        << " types are different: '"  << word1.type << "' and '"
        << type << "'" << std::endl;
      res++;
    }
    if(word1.exact != exact)
    {
      std::cerr << word1.channel_trigger_id
        << " exact flags are different: '"  << word1.exact << "' and '"
        << exact << "'" << std::endl;
      res++;
    }
    if((word1.channel_trigger_id ? false : true) != negative)
    {
      std::cerr << word1.channel_trigger_id
        << " have negative flag equial "  << negative  << std::endl;
      res++;
    }
    return res;
  }

  int TriggerSerializationTest::regular_test_case_() noexcept
  {
    int res = 0;
    size_t iteration = trigger_.empty() ? 1000 : 1;
    for(size_t i = 0; i < iteration && !res; i++)
    {
      SoftWord word1;
      std::string result, trigger;
      ChannelSvcs::SubStringVector parts;
      generate_soft_trigger_word(i + 1, 10, 10, word1, trigger);
      ChannelSvcs::Serialization::serialize(
        word1.parts,
        word1.type,
        word1.exact,(word1.channel_trigger_id ? false: true), result); 
      ChannelSvcs::Serialization::get_parts(
        result.data(), result.size(), parts);
      res += print_compary_parts_(i + 1, word1.parts, parts, result.data(), result.size());
      res += print_compary_result_(
        word1,
        trigger,
        ChannelSvcs::Serialization::trigger_type(result.data()),
        ChannelSvcs::Serialization::get_trigger(result, trigger),
        ChannelSvcs::Serialization::exact(result.data()),
        ChannelSvcs::Serialization::negative(result.data()));
    }
    return res;
  }
  
  int TriggerSerializationTest::run(int argc, char* argv[]) noexcept
  {
    if(argc == 3)
    {
      trigger_ = argv[1];
      trigger_type_ = argv[2][0];
      if(trigger_type_ !=  'U' && trigger_type_ != 'P' &&
         trigger_type_ != 'S' && trigger_type_ != 'R' &&
         trigger_type_ != 'D')
      {
        std::cerr << "Bad trigger type" << trigger_type_ << std::endl;
        return 1;
      }
    }
    return regular_test_case_();
  }
}
}

int main(int argc, char* argv[])
{
  AdServer::UnitTests::TriggerSerializationTest test;
  return test.run(argc, argv);
}
