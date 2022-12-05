
#include <unistd.h>
#include <limits.h>
#include <limits>
#include <sstream>

#include <Generics/ActiveObject.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/TaskRunner.hpp>
#include <Generics/Rand.hpp>
#include <Generics/Time.hpp>
#include <Generics/AppUtils.hpp>
#include <TestCommons/Memory.hpp>
#include <Sync/SyncPolicy.hpp>
#include <CORBACommons/CorbaAdapters.hpp>
#include <CORBACommons/ProcessControlImpl.hpp>
#include<Language/BLogic/NormalizeTrigger.hpp>
#include <Commons/CorbaAlgs.hpp>
#include <ChannelSvcs/ChannelCommons/Serialization.hpp>
#include <tests/UnitTests/ChannelSvcs/Commons/ChannelServerTestCommons.hpp>
#include"UpdateImitator.hpp"

namespace
{
  const char CHANNEL_UPDATE_OBJECT_KEY[] = "ChannelUpdate";
  const char PROCESS_CONTROL_OBJ_KEY[] = "ProcessControl";
}

namespace AdServer
{
  namespace UnitTests
  {
    UpdateImitator::UpdateImitator(
      Algorithm alg,
      size_t count_channels,
      size_t count_triggers,
      size_t count_keywords,
      char trigger_type,
      size_t trigger_length,
      size_t start_point,
      size_t grow_step,
      bool dump_memory_stat) noexcept
      : alg_(alg),
        count_channels_(count_channels),
        count_triggers_(count_triggers),
        count_keywords_(count_keywords),
        trigger_length_(trigger_length),
        start_point_(start_point),
        grow_step_(grow_step),
        trigger_type_(trigger_type),
        count_channels_i_(start_point),
        dump_memory_stat_(dump_memory_stat)
    {
      if (trigger_type_ == 'D')
      {
        count_triggers_ = 1;
      }
      generate_triggers_();
    }

    struct UidAdaptor
    {
    public:
      bool is_quoted(const std::string&) noexcept
      {
        return false;
      }
      String::SubString get_part(const std::string& part) noexcept
      {
        return part;
      }
    };

    void UpdateImitator::generate_triggers_() noexcept
    {
      WriteGuard_ guard(lock_triggers_);
      if (trigger_type_ == 'D')
      {
        ChannelSvcs::StringSet uids;
        for(size_t i = 0; i < trigger_length_; i++)
        {//generate uid trigger
          Generics::Uuid uid = Generics::Uuid::create_random_based();
          uids.insert(uids.end(), uid.to_string(false));
        }
        std::string& trigger = triggers_[1];
        ChannelSvcs::Serialization::serialize(
          uids,
          'D',
          false,
          false,
          ChannelSvcs::Serialization::StringAllocator(trigger),
          trigger,
          UidAdaptor());
      }
      else
      {
        for(size_t i = 0; i <  count_triggers_; i++)
        {
          std::string trigger;
          Language::Trigger::Trigger trigger_view;
          ChannelServerTestCommons::generate_asc_word(trigger, trigger_length_);
          try
          {
            Language::Trigger::normalize(trigger, trigger_view, 0);
          }
          catch(const eh::Exception& e)
          {
            std::cerr << "On parsing '" << trigger
              << "' Caught eh::Exception : " << e.what() << std::endl;
            continue;
          }
          ChannelSvcs::Serialization::serialize(
            trigger_view.parts,
            trigger_type_,
            false,
            false,
            triggers_[i + 1]);
        }
      }
      count_channels_i_ = start_point_;
    }

    UpdateImitator::~UpdateImitator() noexcept
    {
    }
    
    //
    // IDL:AdServer/ChannelSvcs/ChannelProxy/get_count_chunks:1.0
    //
    ::CORBA::ULong UpdateImitator::get_count_chunks()
      /*throw(AdServer::ChannelSvcs::ImplementationException)*/
    {
      return 1;
    }

    void UpdateImitator::update_all_ccg(
      const ChannelCurrent::CCGQuery& /*query*/,
      ChannelCurrent::PosCCGResult_out result)
      /*throw(AdServer::ChannelSvcs::ImplementationException,
            AdServer::ChannelSvcs::NotConfigured)*/
    {
      std::string keyword = "keyword_";
      std::string click = "click_url_";
      keyword.append(512, '0');
      click.append(512, '0');
      result = new ChannelCurrent::PosCCGResult;
      result->start_id = 1;
      result->source_id = 1;
      result->keywords.length(count_keywords_);
      for(size_t i = 0; i < result->keywords.length(); i++)
      {
        ChannelCurrent::CCGKeyword& value = result->keywords[i];
        value.ccg_keyword_id = result->start_id + i;
        value.ccg_id = result->start_id + i;
        value.channel_id = result->start_id + i;
        value.max_cpc = CorbaAlgs::pack_decimal<CampaignSvcs::RevenueDecimal>(
          CampaignSvcs::RevenueDecimal(1.12));
        value.ctr = CorbaAlgs::pack_decimal<CampaignSvcs::CTRDecimal>(
          CampaignSvcs::CTRDecimal(0.12));
        value.click_url << click;
        value.original_keyword << keyword;
      }
    }

    //
    // IDL:AdServer/ChannelSvcs/ChannelCurrent/check:1.0
    //
    void UpdateImitator::check(
      const ChannelCurrent::CheckQuery& /*query*/,
      ChannelCurrent::CheckData_out data)
      /*throw(AdServer::ChannelSvcs::ImplementationException,
            AdServer::ChannelSvcs::NotConfigured)*/
    {
      data = new ChannelCurrent::CheckData;
      ReadGuard_ guard(lock_triggers_);
      Generics::Time stamp = Generics::Time::get_time_of_day(); 
      data->first_stamp = CorbaAlgs::pack_time(stamp);
      data->master_stamp = CorbaAlgs::pack_time(stamp);
      data->versions.length(count_channels_);
      for(size_t i = 0; i < count_channels_; i++)
      {
        ChannelCurrent::ChannelVersion& version = data->versions[i];
        version.id = i + 1;
        version.size = 1000;
        version.stamp = CorbaAlgs::pack_time(stamp);
      }
      data->special_track = false;
      data->special_adv = false;
      data->source_id = 1;
      data->max_time = CorbaAlgs::pack_time(
        Generics::Time::get_time_of_day() - stamp);
    }

    void UpdateImitator::update_triggers(
      const ::AdServer::ChannelSvcs::ChannelIdSeq& /*ids*/,
      ChannelCurrent::UpdateData_out result)
      /*throw(AdServer::ChannelSvcs::ImplementationException,
            AdServer::ChannelSvcs::NotConfigured)*/
    {
      ReadGuard_ guard(lock_triggers_);
      Generics::Time stamp = Generics::Time::get_time_of_day(); 
      size_t count_channels = 0;
      switch(alg_)
      {
        case ALG_RANDOM:
          count_channels = Generics::safe_rand(1, count_channels_);
          break;
        case ALG_CONST:
          count_channels = count_channels_;
          break;
        case ALG_GROW:
          if(count_channels_i_ > count_channels_)
          {
            count_channels_i_ = start_point_;
            throw ChannelSvcs::NotConfigured("Grow limit achieved");
          }
          count_channels = count_channels_i_;
          count_channels_i_ += grow_step_;
          break;
        default:
          throw ChannelSvcs::ImplementationException("Wrong algorithm");
          break;
      }
      result = new ChannelCurrent::UpdateData;
      result->source_id = 1;
      result->channels.length(count_channels);
      for(size_t i = 0; i < count_channels; i++)
      {
        ChannelCurrent::ChannelById& channel = result->channels[i];
        channel.channel_id = i + 1;
        channel.words.length(count_triggers_);
        size_t j = 0;
        for(TriggerMap::const_iterator it = triggers_.begin();
            it != triggers_.end() && j < channel.words.length(); ++it, j++)
        {
          channel.words[j].channel_trigger_id = it->first;
          channel.words[j].trigger.length(it->second.size());
          memcpy(
            channel.words[j].trigger.get_buffer(),
            it->second.data(),
            it->second.size());
        }
        channel.words.length(j);
        channel.stamp = CorbaAlgs::pack_time(stamp);
      }
      if(dump_memory_stat_)
      {
        struct mallinfo info = mallinfo();
        std::ostringstream ostr;
        ostr << __func__ << ": " << stamp.gm_ft() << std::endl;
        TestCommons::print_mallinfo(ostr, &info);
        std::cout << ostr.str() << std::endl;
      }
    }

    UpdateImitator::Algorithm UpdateImitator::parse_alg_name(
      const std::string& name) noexcept
    {
      if(name == "random")
      {
        return ALG_RANDOM;
      }
      else if(name == "const")
      {
        return ALG_CONST;
      }
      else if(name == "grow")
      {
        return ALG_GROW;
      }
      return ERROR;
    }

    const std::string UpdateImitator::control(
      const char* param_name,
      const char* param_value)
    {
      std::ostringstream res;
      bool setted = true;
      if(strcmp(param_name, "alg") == 0)
      {
        Algorithm new_value = parse_alg_name(param_value);
        if(new_value != ERROR)
        {
          alg_ = new_value;
        }
        else
        {
          res << "Invalid value '" << param_value << "' for '"
            <<  param_name << "'";
          setted = false;
        }
      }
      else if(strcmp(param_name, "channels") == 0)
      {
        read_number(
          param_value,
          1UL,
          std::numeric_limits<size_t>::max(),
          count_channels_);
      }
      else if(strcmp(param_name, "triggers") == 0)
      {
        read_number(
          param_value,
          1UL,
          std::numeric_limits<size_t>::max(),
          count_triggers_);
        generate_triggers_();
      }
      else if(strcmp(param_name, "keywords") == 0)
      {
        read_number(
          param_value,
          1UL,
          std::numeric_limits<size_t>::max(),
          count_keywords_);
      }
      else if(strcmp(param_name, "length") == 0)
      {
        read_number(
          param_value,
          1UL,
          1024UL,
          trigger_length_);
        generate_triggers_();
      }
      else if(strcmp(param_name, "from") == 0)
      {
        read_number(
          param_value,
          1UL,
          count_channels_,
          start_point_);
        count_channels_i_ = start_point_;
      }
      else if(strcmp(param_name, "step") == 0)
      {
        read_number(
          param_value,
          1UL,
          count_channels_,
          grow_step_);
        count_channels_i_ = start_point_;
      }
      else if(strcmp(param_name, "configuration") == 0)
      {
        res << "algorithm = ";
        switch(alg_)
        {
          case ALG_RANDOM:
            res << "random";
            break;
          case ALG_CONST:
            res << "const";
            break;
          case ALG_GROW:
            res << "grow";
            res << ", from = " << start_point_;
            res << ", step = " << grow_step_;
            break;
          default:
            res << "error";
            break;
        }
        res << ", channels = " << count_channels_;
        res << ", triggers = " << count_triggers_;
        res << ", length = " << trigger_length_;
        res << ", keywords = " << count_keywords_;
        res << std::endl;
        setted = false;
      }
      else
      {
        res << '\'' <<  param_name <<
          "' unknown parameter";
        setted = false;
      }
      if(setted)
      {
        res << '\'' <<  param_name <<
          "' was setted to '" << param_value << '\'';
      }
      return res.str();
    }

    char*
    ControlImpl::control(const char* param_name, const char* param_value)
      /*throw(CORBACommons::OutOfMemory, CORBACommons::ImplementationError)*/
    {
      return CORBA::string_dup(
        server_->control(param_name, param_value).c_str());
    }

  }
}

int main(int argc, char* argv[])
{
  Generics::AppUtils::Option<unsigned long> count_threads(10);
  Generics::AppUtils::Option<unsigned short> port(10103);
  Generics::AppUtils::Option<size_t> count_channels(100);
  Generics::AppUtils::Option<size_t> count_triggers(100);
  Generics::AppUtils::Option<size_t> count_keywords(1024);
  Generics::AppUtils::Option<size_t> trigger_length(15);
  Generics::AppUtils::Option<size_t> step(1);
  Generics::AppUtils::Option<size_t> start(1);
  Generics::AppUtils::StringOption alg("const");
  Generics::AppUtils::StringOption trigger_type("P");
  Generics::AppUtils::CheckOption dump_memory_stats;
  Generics::AppUtils::Args args(0);
  args.add(
    Generics::AppUtils::equal_name("port") ||
    Generics::AppUtils::short_name("p"),
    port);
  args.add(
    Generics::AppUtils::equal_name("channels") ||
    Generics::AppUtils::short_name("c"),
    count_channels);
  args.add(
    Generics::AppUtils::equal_name("triggers") ||
    Generics::AppUtils::short_name("t"),
    count_triggers);
  args.add(
    Generics::AppUtils::equal_name("trigger_type") ||
    Generics::AppUtils::short_name("T"),
    trigger_type);
  args.add(
    Generics::AppUtils::equal_name("keywords") ||
    Generics::AppUtils::short_name("k"),
    count_keywords);
  args.add(
    Generics::AppUtils::equal_name("length") ||
    Generics::AppUtils::short_name("l"),
    trigger_length);
  args.add(
    Generics::AppUtils::equal_name("alg") ||
    Generics::AppUtils::short_name("a"),
    alg);
  args.add(
    Generics::AppUtils::equal_name("step") ||
    Generics::AppUtils::short_name("s"),
    step);
  args.add(
    Generics::AppUtils::equal_name("from") ||
    Generics::AppUtils::short_name("f"),
    start);
  args.add(
    Generics::AppUtils::equal_name("threads") ||
    Generics::AppUtils::short_name("h"),
    count_threads);
  args.add(
    Generics::AppUtils::equal_name("memorystat") ||
    Generics::AppUtils::short_name("m"),
    dump_memory_stats);
  try
  {
    args.parse(argc - 1, argv + 1);
    AdServer::UnitTests::UpdateImitator::Algorithm alg_value
      = AdServer::UnitTests::UpdateImitator::parse_alg_name(*alg);
    if(alg_value == AdServer::UnitTests::UpdateImitator::ERROR)
    {
      std::cerr << "Wrong algorithm name = " << *alg << std::endl;
      return 1;
    }
    if ((*trigger_type).size() != 1 ||
        !((*trigger_type)[0] == 'P' ||
        (*trigger_type)[0] == 'S' ||
        (*trigger_type)[0] == 'R' ||
        (*trigger_type)[0] == 'D'))
    {
      std::cerr << "Wrong trigger type = " << *trigger_type << std::endl;
      return 1;
    }

    CORBACommons::CorbaConfig config;
    config.thread_pool = *count_threads;
    {
      CORBACommons::EndpointConfig endpoint;
      endpoint.host = "*";
      endpoint.ior_names = "*";
      endpoint.port = *port;
      endpoint.objects[CHANNEL_UPDATE_OBJECT_KEY].insert(CHANNEL_UPDATE_OBJECT_KEY);
      endpoint.objects[PROCESS_CONTROL_OBJ_KEY].insert(PROCESS_CONTROL_OBJ_KEY);
      config.endpoints.push_back(endpoint);
    }
    CORBACommons::CorbaServerAdapter_var corba_server_adapter(
      new CORBACommons::CorbaServerAdapter(config));

    AdServer::UnitTests::UpdateImitator_var server =
      new AdServer::UnitTests::UpdateImitator(
        alg_value,
        *count_channels,
        *count_triggers,
        *count_keywords,
        (*trigger_type)[0],
        *trigger_length,
        *start,
        *step,
        dump_memory_stats.enabled());

    AdServer::UnitTests::ControlImpl_var proccessor = 
      new AdServer::UnitTests::ControlImpl(
        corba_server_adapter->shutdowner(),
        server.in());

    corba_server_adapter->add_binding(
      CHANNEL_UPDATE_OBJECT_KEY, server.in());

    corba_server_adapter->add_binding(PROCESS_CONTROL_OBJ_KEY, proccessor.in());

    // Running orb loop
    corba_server_adapter->run();

    server.reset();

    proccessor.reset();

    corba_server_adapter.reset();

  }
  catch(const eh::Exception& e)
  {
    std::cerr << "Exception: " << e.what() << std::endl;
  }
  catch(...)
  {
    return 1;
  }
  return 0;
}

