#include <list>
#include <vector>
#include <iterator> 
#include <time.h>

#include <iostream>

#include <String/StringManip.hpp>
#include <Logger/FileLogger.hpp>
#include <Logger/StreamLogger.hpp>

#include <UserInfoSvcs/UserInfoManager/UserInfoContainer.hpp>
#include <Commons/ErrorHandler.hpp>
#include <Commons/ConfigUtils.hpp>

#include "Application.hpp"
#include "UserInfoContainerTest.hpp"

namespace
{
  char USAGE[] =
    "Usage: \nUserInfoContainerTest <test schema file>.";
}

Application_::Application_() noexcept
{
};

Application_::~Application_() noexcept
{
};

void
Application_::main(int& argc, char** argv)
  noexcept
{
  if (!::setlocale(LC_CTYPE, "en_US.utf8"))
  {
    std::cout
      << "UserInfoChunksUtilApp_::main(): cannot set locale."
      << std::endl;
  }

  if(argc != 2)
  {
    std::cout << USAGE << std::endl;
    return;
  }
  else
  {
    Config::ErrorHandler error_handler;
    std::string file_name(argv[1]);

    using namespace xsd::AdServer::Configuration;

    try
    {
      std::unique_ptr<TestConfigurationType>
        test_config = TestConfiguration(file_name.c_str(), error_handler);

      if(error_handler.has_errors())
      {
        std::string error_string;
        throw Exception(error_handler.text(error_string));
      }

      logger_ = Config::LoggerConfigReader::create(
        test_config->UserInfoContainerTestConfig().Logger());

      BehavParamsConfig_var channels = new BehavParamsConfig();
      {
        typedef UserInfoContainerTestConfigType::ChannelRules_sequence
          ChannelRulesContainer;
        const ChannelRulesContainer& channels_config =
          test_config->UserInfoContainerTestConfig().ChannelRules();
        
        for (ChannelRulesContainer::const_iterator it = channels_config.begin();
             it != channels_config.end();
             ++it)
        {
          unsigned long ch_id = it->channel_id();
          unsigned long old_len = channels->behav_params.size();
          
          if (ch_id >= old_len)
          {
            channels->behav_params.resize(ch_id + 1);
            
            for (unsigned long i = old_len; i < ch_id + 1; ++i)
            {
              channels->behav_params[i].status = BehavParam::BS_INACTIVE;
            }
          }

          channels->behav_params[ch_id].status = BehavParam::BS_ACTIVE;
          channels->behav_params[ch_id].time_from = it->time_from();
          channels->behav_params[ch_id].time_to = it->time_to();
          channels->behav_params[ch_id].minimum_visits = it->minimum_visits();

          if (it->time_from() == it->time_to() &&
              it->time_from() == 0)
          {
            channels->behav_params[ch_id].type = BehavParam::BT_CONTEXT;
          }
          else if (it->time_from() > it->time_to())
          {
            channels->behav_params[ch_id].status = BehavParam::BS_INACTIVE;
          }
          else if (it->time_to() <= SESSION_TIMEOUT)
          {
            channels->behav_params[ch_id].type = BehavParam::BT_SESSION;
          }
          else if (it->time_from() >= SEC_IN_DAY)
          {
            channels->behav_params[ch_id].type = BehavParam::BT_HISTORY;
          }
          else
          {
            channels->behav_params[ch_id].type = BehavParam::BT_HISTORY_TODAY;
          }
        }
      }

      std::string chunks_path;
      std::vector<std::string> chunk_files;
      {
        const ChunksConfigType& chunks_config =
          test_config->UserInfoContainerTestConfig().ChunksConfig();
        
        chunks_path = chunks_config.chunks_path();

        if (chunks_path[chunks_path.length() - 1] != '/')
        {
          chunks_path += "/";
        }
        
        chunk_files.push_back(chunks_path + chunks_config.base_chunk_file());
        chunk_files.push_back(chunks_path + chunks_config.additional_chunk_file());
        chunk_files.push_back(chunks_path + chunks_config.history_chunk_file());
        chunk_files.push_back(chunks_path + chunks_config.activity_chunk_file());
        chunk_files.push_back(chunks_path + chunks_config.pref_file());
        chunk_files.push_back(chunks_path + chunks_config.wd_imps_file());
      
        for (int i = 0; i < 8; ++i)
        {
          if (!std::ifstream(chunk_files[i].c_str()))
          {
            std::ofstream ofs(chunk_files[i].c_str());
          }
        }
      }

      UserInfoContainer user_info_container(1,
                                            1,
                                            100,
                                            3600*24,
                                            20,
                                            0,
                                            logger_);
      
      user_info_container.channels_config(channels);
      
      user_info_container.set_chunk_info(
        0,
        chunks_path.c_str(),
        chunk_files[0].c_str(),
        chunk_files[1].c_str(),
        chunk_files[2].c_str(),
        chunk_files[3].c_str(),
        chunk_files[4].c_str(),
        chunk_files[5].c_str(),
        24*60*60*180);

      unsigned long match_users =
        test_config->UserInfoContainerTestConfig().MatchConfig().size();
      
      std::vector<std::string> user_id;
      std::vector<std::string> match_channels;
      std::vector<unsigned long> matches_number;
      
      
      for (unsigned long i = 0; i < match_users; ++i)
      {
        user_id.push_back(
          test_config->UserInfoContainerTestConfig().MatchConfig()[i].user_id());
        match_channels.push_back(
          test_config->UserInfoContainerTestConfig().MatchConfig()[i].channels());
          matches_number.push_back(
            test_config->UserInfoContainerTestConfig().MatchConfig()[i].matches_number());
      }
      
      std::vector<ChannelIdList> matched_channels, result_channels;
      ColoUserId colo_user_id;
      long cust_id = 1, last_colo_id = 1;
      unsigned long colo_req_timeout = 120;

      matched_channels.resize(match_users);
      result_channels.resize(match_users);
      
      for (unsigned long i = 0; i < match_users; ++i)
      {
        String::StringManip::Splitter<
          const String::AsciiStringManip::Char2Category<',', ' '> >
          input_channels_tokenizer(match_channels[i]);
      
        String::SubString channel;
        while(input_channels_tokenizer.get_token(channel))
        {
          ChannelId id;
          String::StringManip::str_to_int(channel, id);
          matched_channels[i].push_back(id);
        }
      }

      for (unsigned long i = 0; i < match_users; ++i)
      {
        UserInfoContainer::RequestMatchParams request_params(
          user_id[i].c_str(),
          std::string(),
          Generics::Time::get_time_of_day(),
          false,
          0);

        UserInfoContainer::UserAppearance user_app;
        
        for (unsigned long j = 0; j < matches_number[i]; ++j)
        {
          PartlyMatchList partly_match_list;
          
          user_info_container.match(
            request_params,
            last_colo_id,
            cust_id,
            colo_req_timeout,
            colo_user_id,
            matched_channels[i],
            result_channels[i],
            user_app,
            partly_match_list);
        }
      }
    }
    catch(const AdServer::UserInfoSvcs::UserInfoContainer::NotReady& e)
    {
      std::cout << "Caught UserInfoContainer::NotReady. ";
    }
    catch(const xml_schema::parsing& e)
    {
      Stream::Error ostr;

      ostr << "Can't parse config file '"
           << argv[1] << "'."
           << ": ";
      
      if(error_handler.has_errors())
      {
        std::string error_string;
        ostr << error_handler.text(error_string);
      }
      
      throw Exception(ostr);
    }
    catch(const eh::Exception& ex)
    {
      std::cout << "Caught eh::Exception. : "
                << ex.what() << std::endl;
    }
  }
}

int
main(int argc, char** argv)
{
  Application_* app = 0;

  try
  {
    app = &Application::instance();
  }
  catch (...)
  {
    std::cerr << "main(): Critical: Got exception while "
      "creating application object.\n";
    return -1;
  }

  if (app == 0)
  {
    std::cerr << "main(): Critical: got NULL application object.\n";
    return -1;
  }
  
  app->main(argc, argv);
}  
