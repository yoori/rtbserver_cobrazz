#include<ChannelSvcs/ChannelCommons/TriggerParser.hpp>
#include<ChannelSvcs/ChannelServer/UpdateContainer.hpp>
#include"Application.hpp"

namespace AdServer
{
  void Application::load_dictionary_(const char* file_name) /*throw(BadParams)*/
  {
      std::ifstream dict(file_name);
      if(dict.fail())
      {
        Stream::Error err;
        err << "failed to open '" << file_name << "' error: " << errno;
        throw BadParams(err);
      }
      do
      {
        std::string str;
        std::getline(dict, str);
        if(!str.empty())
        {
          dict_.insert(str);
        }
      } while(!(dict.bad() || dict.fail() || dict.eof()));
      dict.close();
  }

  void Application::read_triggers_(std::istream& is) /*throw(eh::Exception)*/
  {
    unsigned long id = 1;
    ChannelSvcs::UpdateContainer upcont(&container_, 0);
    ChannelSvcs::TriggerList triggers;
    do
    {
      ChannelSvcs::Trigger trigger;
      std::getline(is, trigger.trigger);
      if(!trigger.trigger.empty())
      {
        trigger.type = 'P';
        trigger.negative = (trigger.trigger[0] == '-');
        trigger.channel_trigger_id = id;
        triggers.push_back(trigger);
        triggers_[id++].swap(trigger.trigger);
      }
    } while(!(is.bad() || is.fail() || is.eof()));
    ChannelSvcs::TriggerParser::TriggerParser::parse_triggers(
      1,
      "",
      triggers,
      0,
      std::set<unsigned short>(),
      &upcont,
      Commons::DEFAULT_MAX_HARD_WORD_SEQ);
    ChannelSvcs::ChannelIdToMatchInfo_var info =
      new ChannelSvcs::ChannelIdToMatchInfo;
    ChannelSvcs::Channel& ch = (*info)[1].channel;
    ch.id = 1;
    ch.set_status('A');
    ch.mark_type(ChannelSvcs::CT_PAGE);
    container_.merge(upcont, *info, true, 0, Generics::Time::ZERO); 
  }

  void Application::write_result_(std::ostream& os) noexcept
  {
    for(auto it = triggers_.begin(); it != triggers_.end(); ++it)
    {
      os << it->second << std::endl;
    }
  }
  
  int Application::compary_triggers_(
    const Language::Trigger::Trigger& trigger_view,
    const Language::Trigger::Trigger& trigger_view2) noexcept
  {
    if(trigger_view.exact != trigger_view2.exact)
    {
      if(trigger_view.exact)
      {
        return 2;
      }
      if(trigger_view2.exact)
      {
        return 1;
      }
    }
    if(trigger_view.parts.size() < trigger_view2.parts.size())
    {
      return 1;
    }
    else if(trigger_view.parts.size() > trigger_view2.parts.size())
    {
      return 2;
    }
    if(trigger_view.trigger == trigger_view2.trigger)
    {
      return -1;
    }
    for(size_t i = 0; i < trigger_view2.parts.size(); ++i)
    {
      if(trigger_view2.parts[i].part == trigger_view.parts[i].part)
      {
        if(trigger_view.parts[i].quotes != trigger_view2.parts[i].quotes)
        {
          if(trigger_view2.parts[i].quotes)
          {
            return 1;
          }
          else
          {
            return 2;
          }
        }
      }
      else
      {
        return 0;
      }
    }
    return 0;
  }

  void Application::filter_() /*throw(Exception)*/
  {
    unsigned int flags = ChannelSvcs::MF_ACTIVE;
    for(auto it = triggers_.begin(); it != triggers_.end();)
    {
      ChannelSvcs::MatchUrls urls;
      ChannelSvcs::MatchWords match_words[ChannelSvcs::CT_MAX];
      ChannelSvcs::StringVector exact;
      Generics::Uuid uid;
      ChannelSvcs::TriggerMatchRes res;
      Language::Trigger::Trigger trigger_view;
      try
      {
        Language::Trigger::normalize(it->second, trigger_view, 0);
        if(trigger_view.parts.empty())
        {
          std::cerr << "The trigger '" << it->second
            << "' is empty after normalization'" << std::endl ;
          triggers_.erase(it++);
          continue;
        }
        auto stop_it = dict_.end();
        bool numeric_warn = true;
        for(auto it_part = trigger_view.parts.begin();
            it_part != trigger_view.parts.end(); ++it_part)
        {
          auto fnd_stop_it = dict_.find(it_part->part.str());
          if(fnd_stop_it != dict_.end())
          {
            stop_it = fnd_stop_it;
          }
          if(numeric_warn)
          {
            if(String::AsciiStringManip::NUMBER.find_nonowned(
              it_part->part.data(),
              it_part->part.data() + it_part->part.length()) ==
              it_part->part.data() + it_part->part.length())
            {
              std::cerr << "The word '" << it_part->part
                << "' in the trigger '" << it->second
                << "' contains only numbers" << std::endl;
              numeric_warn = false;
            }
          }
          match_words[ChannelSvcs::CT_PAGE].insert(it_part->part);
        }
        if(stop_it != dict_.end())
        {
          std::cerr << "The trigger '" << it->second
            << "' contains stop word '" << *stop_it << "'" << std::endl ;
        }
        {
          container_.match(
            urls,
            urls,
            match_words,
            match_words[ChannelSvcs::CT_URL],
            exact,
            uid,
            flags,
            res);
          if(res.size() != 1 || res.begin()->first != 1)
          {
            throw Exception("Unexpected size of channel id in result");
          }
          auto& data = res.begin()->second; 
          if(data.trigger_ids[ChannelSvcs::CT_PAGE].size() != 1)
          {//over covering
            auto best = it->first;
            auto cur = it->first;
            for(auto res_it = data.trigger_ids[ChannelSvcs::CT_PAGE].begin();
                res_it != data.trigger_ids[ChannelSvcs::CT_PAGE].end(); ++res_it)
            {
              if(best != *res_it && triggers_.find(*res_it) != triggers_.end())
              {
                Language::Trigger::Trigger trigger_view2;
                Language::Trigger::normalize(
                  triggers_[*res_it],
                  trigger_view2,
                  0);
                int cmp = compary_triggers_(trigger_view, trigger_view2);
                if(cmp == 1)
                {
                  std::cerr << "The trigger '" << triggers_[best] 
                    << "' is more common, remove trigger '"
                    << triggers_[*res_it] << "'" << std::endl;
                  if(*res_it != cur)
                  {
                    triggers_.erase(*res_it);
                  }
                }
                else if(cmp == 2)
                {
                  std::cerr << "The trigger '" << triggers_[*res_it] 
                    << "' is more common, remove trigger '"
                    << triggers_[best] << "'" << std::endl;
                  trigger_view.trigger.swap(trigger_view2.trigger);
                  trigger_view.parts.swap(trigger_view2.parts);
                  std::swap(trigger_view.exact, trigger_view2.exact);
                  if(best != cur)
                  {
                    triggers_.erase(best);
                  }
                  best = *res_it;
                }
                else if(cmp == -1)
                {
                  std::cerr << "The trigger '" << triggers_[*res_it] 
                    << "' is dublicate '" << triggers_[best] 
                    << "', remove it" << std::endl;
                  if(*res_it != cur)
                  {
                    triggers_.erase(*res_it);
                  }
                }
              }
            }
            if(best != cur)
            {
              triggers_.erase(it++);
            }
            else
            {
              ++it;
            }
          }
          else
          {
            if(data.trigger_ids[ChannelSvcs::CT_PAGE].front() != it->first)
            {
              std::cerr << "Expect matching '" << it->second << "', got '"
                << triggers_[data.trigger_ids[ChannelSvcs::CT_PAGE].front()]
                << "'" << std::endl; 
            }
            ++it;
          }
        }
      }
      catch(const eh::Exception& e)
      {
        std::cerr << "Bad trigger '" << it->second << "' :" << e.what() << std::endl;
        triggers_.erase(it++);
      }
    }
  }

  int Application::run(int argc, char* argv[]) /*throw(BadParams)*/
  {
    if(argc > 1)
    {
      if(!strcmp(argv[1], "help"))
      {
        usage_(argv[0]);
        return 0;
      }
      load_dictionary_(argv[1]);
    }
    if(argc > 2)
    {
      std::ifstream in(argv[2]);
      if(in.fail())
      {
        Stream::Error err;
        err << "failed to open file '" << argv[2] <<  "' error: " << errno;
        throw BadParams(err);
      }
      read_triggers_(in);
      in.close();
    }
    else
    {
      read_triggers_(std::cin);
    }
    try
    {
      filter_();
    }
    catch(const Exception& e)
    {
      std::cerr << e.what() << std::endl;
      return 1;
    }
    if(argc > 3)
    {
      std::ofstream out(argv[3]);
      if(out.fail())
      {
          Stream::Error err;
          err << "failed to open file '" << argv[3] <<  "' error: " << errno;
          throw BadParams(err);
      }
      write_result_(out);
      out.close();
    }
    else
    {
      write_result_(std::cout);
    }
    return 0;
  }

  void Application::usage_(const char* program_name) noexcept
  {
    std::cout <<  program_name << " [dictionary] [input] [ouput]" << std::endl;
  }
}

int main(int argc, char* argv[])
{
  AdServer::Application app;
  try
  {
    return app.run(argc, argv);
  }
  catch(const AdServer::Application::BadParams& e)
  {
    std::cerr << e.what();
    return 1;
  }
}

