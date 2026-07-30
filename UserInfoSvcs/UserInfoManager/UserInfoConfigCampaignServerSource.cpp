#include "UserInfoConfigCampaignServerSource.hpp"

#include <list>
#include <map>

#include <Commons/CorbaAlgs.hpp>
#include <Commons/CorbaConfig.hpp>
#include <Commons/FreqCapManip.hpp>
#include <CORBACommons/CorbaAdapters.hpp>
#include <CORBACommons/ObjectPool.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignSvcsVersionAdapter.hpp>
#include <CampaignSvcs/CampaignServer/CampaignServer.hpp>

#include "UserInfoManagerCore.hpp"

namespace
{
  namespace Aspect
  {
    const char USER_INFO_MANAGER[] = "UserInfoManager";
  }

  struct SimpleChannelProperties
  {
    SimpleChannelProperties(
      unsigned long channel_id_val,
      bool discover_val,
      unsigned long behav_param_list_id_val,
      const char* str_behav_param_list_id_val)
      : channel_id(channel_id_val),
        discover(discover_val),
        behav_param_list_id(behav_param_list_id_val),
        str_behav_param_list_id(str_behav_param_list_id_val)
    {
    }

    unsigned long channel_id;
    bool discover;
    unsigned long behav_param_list_id;
    std::string str_behav_param_list_id;
  };

  typedef std::list<
    SimpleChannelProperties,
    Generics::TAlloc::ThreadPool<SimpleChannelProperties, 256>>
    SimpleChannelPropertiesList;

  struct BehavIdTypeKey
  {
    BehavIdTypeKey(unsigned long id_val, char type_val)
      : id(id_val), type(type_val)
    {
    }

    bool
    operator<(const BehavIdTypeKey& right) const
    {
      return ((id < right.id) || (id == right.id && type < right.type));
    }

    unsigned long id;
    char type;
  };

  struct StrBehavIdTypeKey
  {
    StrBehavIdTypeKey(const char* id_val, char type_val)
      : id(id_val), type(type_val)
    {
    }

    bool
    operator<(const StrBehavIdTypeKey& right) const
    {
      return ((id < right.id) || (id == right.id && type < right.type));
    }

    std::string id;
    char type;
  };

  void
  push_channel_interval_(
    AdServer::UserInfoSvcs::ChannelIntervalsPack* cip,
    const AdServer::UserInfoSvcs::ChannelInterval& ci)
    noexcept
  {
    if (ci.time_to < Generics::Time::ONE_DAY)
    {
      cip->short_intervals.insert(ci);
    }
    else if (ci.time_to >= Generics::Time::ONE_DAY &&
      ci.time_from == Generics::Time::ZERO)
    {
      cip->today_long_intervals.insert(ci);
    }
    else if (ci.time_to > ci.time_from &&
      ci.time_from >= Generics::Time::ONE_DAY)
    {
      cip->long_intervals.insert(ci);
    }
  }

  void
  init_ht_candidate_templates_(
    AdServer::UserInfoSvcs::ChannelIntervalsPack* cip)
    noexcept
  {
    cip->ht_candidate_templates.clear();
    cip->ht_candidate_templates.reserve(cip->today_long_intervals.size());

    for (auto it = cip->today_long_intervals.begin();
      it != cip->today_long_intervals.end(); ++it)
    {
      const unsigned long req_visits = it->min_visits - 1;
      cip->ht_candidate_templates.emplace_back(
        it->min_visits,
        req_visits,
        req_visits == 0 ? it->weight : 0);
    }
  }

  void
  init_ht_candidate_templates_(
    AdServer::UserInfoSvcs::ChannelIntervalsPack* page_cip,
    AdServer::UserInfoSvcs::ChannelIntervalsPack* search_cip,
    AdServer::UserInfoSvcs::ChannelIntervalsPack* url_cip,
    AdServer::UserInfoSvcs::ChannelIntervalsPack* url_keyword_cip,
    AdServer::UserInfoSvcs::ChannelIntervalsPack* audience_cip)
    noexcept
  {
    init_ht_candidate_templates_(page_cip);
    init_ht_candidate_templates_(search_cip);
    init_ht_candidate_templates_(url_cip);
    init_ht_candidate_templates_(url_keyword_cip);
    init_ht_candidate_templates_(audience_cip);
  }

  void
  apply_freq_caps_(
    AdServer::UserInfoSvcs::FreqCapConfig& freq_cap_config,
    const AdServer::CampaignSvcs_v360::FreqCapConfigInfo& freq_cap_config_info,
    const Generics::Time& confirm_timeout)
    noexcept
  {
    freq_cap_config.confirm_timeout = confirm_timeout;
    const CORBA::ULong freq_caps_length = freq_cap_config_info.freq_caps.length();
    freq_cap_config.freq_caps.reserve(freq_caps_length);

    for (CORBA::ULong i = 0; i < freq_caps_length; ++i)
    {
      const AdServer::CampaignSvcs_v360::FreqCapInfo& fc = freq_cap_config_info.freq_caps[i];
      AdServer::Commons::FreqCap freq_cap;
      freq_cap.fc_id = fc.fc_id;
      freq_cap.lifelimit = fc.lifelimit;
      freq_cap.period = Generics::Time(fc.period);
      freq_cap.window_limit = fc.window_limit;
      freq_cap.window_time = Generics::Time(fc.window_time);
      freq_cap_config.freq_caps.emplace(freq_cap.fc_id, freq_cap);
    }

    freq_cap_config.campaign_ids.reserve(freq_cap_config_info.campaign_ids.length());
    CorbaAlgs::convert_sequence(freq_cap_config_info.campaign_ids, freq_cap_config.campaign_ids);
  }

  AdServer::UserInfoSvcs::UserInfoConfigSourcePtr
  make_config_source(
    Logging::Logger* logger,
    const xsd::AdServer::Configuration::UserInfoManagerConfigType& config)
  {
    CORBACommons::CorbaObjectRefList campaign_server_refs;
    Config::CorbaConfigReader::read_multi_corba_ref(
      config.CampaignServerCorbaRef(),
      campaign_server_refs);

    std::vector<std::string> campaign_server_ref_strings;
    campaign_server_ref_strings.reserve(campaign_server_refs.size());
    for (const auto& campaign_server_ref : campaign_server_refs)
    {
      campaign_server_ref_strings.push_back(campaign_server_ref.object_ref);
    }

    return std::make_shared<AdServer::UserInfoSvcs::UserInfoConfigCampaignServerSource>(
      logger,
      campaign_server_ref_strings,
      config.service_index(),
      Generics::Time(config.FreqCaps().confirm_timeout()));
  }
}

namespace AdServer::UserInfoSvcs
{
  UserInfoManagerCore::UserInfoManagerCore(
    Generics::ActiveObjectCallback* callback,
    Logging::Logger* logger,
    const UserInfoManagerConfig& user_info_manager_config)
    /*throw(Exception)*/
    : UserInfoManagerCore(
        callback,
        logger,
        user_info_manager_config,
        make_config_source(logger, user_info_manager_config))
  {
  }

  struct UserInfoConfigCampaignServerSource::State
  {
    typedef CORBACommons::ObjectPoolRefConfiguration CampaignServerPoolConfig;
    typedef CORBACommons::ObjectPool<
      CampaignSvcs::CampaignServer,
      CampaignServerPoolConfig>
      CampaignServerPool;
    typedef std::unique_ptr<CampaignServerPool> CampaignServerPoolPtr;

    CORBACommons::CorbaClientAdapter_var corba_client_adapter;
    CampaignServerPoolPtr campaign_servers;

    State()
      : corba_client_adapter(new CORBACommons::CorbaClientAdapter())
    {
    }
  };

  UserInfoConfigCampaignServerSource::UserInfoConfigCampaignServerSource(
    Logging::Logger* logger,
    const std::vector<std::string>& campaign_server_refs,
    unsigned long service_index,
    const Generics::Time& confirm_timeout)
    : logger_(ReferenceCounting::add_ref(logger)),
      state_(new State()),
      service_index_(service_index),
      confirm_timeout_(confirm_timeout)
  {
    State::CampaignServerPoolConfig pool_config(state_->corba_client_adapter.in());
    pool_config.timeout = Generics::Time(10); // 10 sec

    for (const auto& campaign_server_ref : campaign_server_refs)
    {
      pool_config.iors_list.emplace_back(campaign_server_ref.c_str());
    }

    state_->campaign_servers.reset(
      new State::CampaignServerPool(
        pool_config,
        CORBACommons::ChoosePolicyType::PT_PERSISTENT));
  }

  UserInfoConfigCampaignServerSource::~UserInfoConfigCampaignServerSource()
    noexcept = default;

  UserInfoConfigSource::Config
  UserInfoConfigCampaignServerSource::get_config()
  {
    static const char* FUN = "UserInfoConfigCampaignServerSource::get_config()";
    const unsigned long PORTIONS_NUMBER = 20;

    for (;;)
    {
      State::CampaignServerPool::ObjectHandlerType campaign_server =
        state_->campaign_servers->get_object<
          State::CampaignServerPool::Exception>(
          logger_,
          Logging::Logger::EMERGENCY,
          Aspect::USER_INFO_MANAGER,
          "ADS_ICON-10",
          service_index_,
          service_index_);

      try
      {
        Config config;
        config.channels_config = new ChannelDictionary();
        config.freq_cap_config = new FreqCapConfig();

        using BehavIdChannelIntervalMap = std::map<BehavIdTypeKey, ChannelIntervalsPack_var>;
        BehavIdChannelIntervalMap behav_channel_interval_map;

        using StrBehavIdChannelIntervalMap = std::map<StrBehavIdTypeKey, ChannelIntervalsPack_var>;
        StrBehavIdChannelIntervalMap str_behav_channel_interval_map;

        SimpleChannelPropertiesList channel_properties_list;

        CampaignSvcs::CampaignServer::GetSimpleChannelsInfo settings;
        settings.portions_number = PORTIONS_NUMBER;
        settings.channel_statuses = "AW";

        unsigned long res_length = 0;

        for (unsigned long portion = 0; portion < PORTIONS_NUMBER;
          ++portion)
        {
          settings.portion = portion;
          CampaignSvcs::BriefSimpleChannelAnswer_var channels_to_load =
            campaign_server->brief_simple_channels(settings);

          const CORBA::ULong simple_channels_length = channels_to_load->simple_channels.length();
          res_length += simple_channels_length;
          config.channels_config->channel_features.reserve(
            config.channels_config->channel_features.size() +
            simple_channels_length);

          for (CORBA::ULong i = 0; i < simple_channels_length; ++i)
          {
            const CampaignSvcs::BriefSimpleChannelKey& channel =
              channels_to_load->simple_channels[i];

            channel_properties_list.push_back(SimpleChannelProperties(
              channel.channel_id,
              channel.discover,
              channel.behav_param_list_id,
              channel.str_behav_param_list_id));

            CorbaAlgs::convert_sequence(
              channel.categories,
              config.channels_config->channel_categories[channel.channel_id]);

            config.channels_config->channel_features.insert(std::make_pair(
              channel.channel_id,
              ChannelFeatures(channel.discover, channel.threshold)));
          }

          for (CORBA::ULong i = 0; i < channels_to_load->behav_params.length(); ++i)
          {
            const CampaignSvcs::BriefBehavParamInfo& bpi =
              channels_to_load->behav_params[i];

            if (bpi.bp_seq.length() != 0)
            {
              ChannelIntervalsPack_var page_cip = new ChannelIntervalsPack();
              ChannelIntervalsPack_var search_cip = new ChannelIntervalsPack();
              ChannelIntervalsPack_var url_cip = new ChannelIntervalsPack();
              ChannelIntervalsPack_var url_keyword_cip = new ChannelIntervalsPack();
              ChannelIntervalsPack_var audience_cip = new ChannelIntervalsPack();

              for (CORBA::ULong j = 0; j < bpi.bp_seq.length(); ++j)
              {
                const CampaignSvcs::BehavParameter& bp = bpi.bp_seq[j];

                ChannelIntervalsPack_var target_cip;
                bool unknown_trigger_type = false;

                switch (bp.trigger_type)
                {
                case PAGE_CHANNEL:
                  target_cip = page_cip;
                  break;
                case SEARCH_CHANNEL:
                  target_cip = search_cip;
                  break;
                case URL_CHANNEL:
                  target_cip = url_cip;
                  break;
                case URL_KEYWORD_CHANNEL:
                  target_cip = url_keyword_cip;
                  break;
                case AUDIENCE_CHANNEL:
                  target_cip = audience_cip;
                  break;
                default:
                  unknown_trigger_type = true;
                  break;
                }

                if (!unknown_trigger_type)
                {
                  if (bp.trigger_type == AUDIENCE_CHANNEL)
                  {
                    if (bp.time_from != 0 || bp.time_to == 0 || bp.min_visits != 1)
                    {
                      logger_->stream(
                        Logging::Logger::WARNING,
                        Aspect::USER_INFO_MANAGER,
                        "ADS-IMPL-75")
                        << FUN
                        << ": Audience channel with BEHAV_PARAMS_LIST_ID = "
                        << bpi.id << " is incorrect.";
                    }
                    else
                    {
                      ChannelInterval ci(
                        Generics::Time(bp.time_from),
                        Generics::Time(bp.time_to),
                        bp.min_visits,
                        bp.weight);
                      push_channel_interval_(target_cip, ci);
                    }
                  }
                  else if (bp.time_from == 0 && bp.time_to == 0)
                  {
                    if (bp.min_visits == 1)
                    {
                      target_cip->contextual = true;
                      target_cip->zero_channel = true;
                      target_cip->weight += bp.weight;
                    }
                    else
                    {
                      logger_->stream(
                        Logging::Logger::WARNING,
                        Aspect::USER_INFO_MANAGER,
                        "ADS-IMPL-75")
                        << FUN << ": Channel with BEHAV_PARAMS_LIST_ID = "
                        << bpi.id << " is incorrect.";
                    }
                  }
                  else if (bp.time_from < bp.time_to)
                  {
                    ChannelInterval ci(
                      Generics::Time(bp.time_from),
                      Generics::Time(bp.time_to),
                      bp.min_visits,
                      bp.weight);

                    if (bp.time_from == 0 && bp.min_visits == 1)
                    {
                      target_cip->contextual = true;
                    }

                    push_channel_interval_(target_cip, ci);
                  }
                  else
                  {
                    logger_->stream(
                      Logging::Logger::WARNING,
                      Aspect::USER_INFO_MANAGER,
                      "ADS-IMPL-74")
                      << FUN << ": Channel with BEHAV_PARAMS_LIST_ID = "
                      << bpi.id << " is incorrect.";
                  }
                }
              }

              init_ht_candidate_templates_(
                page_cip,
                search_cip,
                url_cip,
                url_keyword_cip,
                audience_cip);

              behav_channel_interval_map[BehavIdTypeKey(
                bpi.id, PAGE_CHANNEL)] = page_cip;
              behav_channel_interval_map[BehavIdTypeKey(
                bpi.id, URL_CHANNEL)] = url_cip;
              behav_channel_interval_map[BehavIdTypeKey(
                bpi.id, URL_KEYWORD_CHANNEL)] = url_keyword_cip;
              behav_channel_interval_map[BehavIdTypeKey(
                bpi.id, SEARCH_CHANNEL)] = search_cip;
              behav_channel_interval_map[BehavIdTypeKey(
                bpi.id, AUDIENCE_CHANNEL)] = audience_cip;
            }
          }

          for (CORBA::ULong i = 0;
            i < channels_to_load->key_behav_params.length();
            ++i)
          {
            const CampaignSvcs::BriefKeyBehavParamInfo& kbpi =
              channels_to_load->key_behav_params[i];

            if (kbpi.bp_seq.length() != 0)
            {
              ChannelIntervalsPack_var page_cip = new ChannelIntervalsPack();
              ChannelIntervalsPack_var search_cip = new ChannelIntervalsPack();
              ChannelIntervalsPack_var url_cip = new ChannelIntervalsPack();
              ChannelIntervalsPack_var url_keyword_cip = new ChannelIntervalsPack();
              ChannelIntervalsPack_var audience_cip = new ChannelIntervalsPack();

              for (CORBA::ULong j = 0; j < kbpi.bp_seq.length(); ++j)
              {
                const CampaignSvcs::BehavParameter& bp = kbpi.bp_seq[j];

                ChannelIntervalsPack_var target_cip;
                bool unknown_trigger_type = false;

                switch (bp.trigger_type)
                {
                case PAGE_CHANNEL:
                  target_cip = page_cip;
                  break;
                case SEARCH_CHANNEL:
                  target_cip = search_cip;
                  break;
                case URL_CHANNEL:
                  target_cip = url_cip;
                  break;
                case URL_KEYWORD_CHANNEL:
                  target_cip = url_keyword_cip;
                  break;
                case AUDIENCE_CHANNEL:
                  target_cip = audience_cip;
                  break;
                default:
                  unknown_trigger_type = true;
                  break;
                }

                if (!unknown_trigger_type)
                {
                  if (bp.trigger_type == AUDIENCE_CHANNEL)
                  {
                    if (bp.time_from != 0 || bp.time_to == 0 ||
                      bp.min_visits != 1)
                    {
                      logger_->stream(
                        Logging::Logger::WARNING,
                        Aspect::USER_INFO_MANAGER,
                        "ADS-IMPL-75") <<
                        FUN << ": Audience channel with "
                        "BEHAV_PARAMS_LIST_ID = " << kbpi.id << " is incorrect.";
                    }
                    else
                    {
                      ChannelInterval ci(
                        Generics::Time(bp.time_from),
                        Generics::Time(bp.time_to),
                        bp.min_visits,
                        bp.weight);
                      push_channel_interval_(target_cip, ci);
                    }
                  }
                  else if (bp.time_from == 0 && bp.time_to == 0)
                  {
                    if (bp.min_visits == 1)
                    {
                      target_cip->contextual = true;
                      target_cip->zero_channel = true;
                      target_cip->weight += bp.weight;
                    }
                    else
                    {
                      logger_->stream(
                        Logging::Logger::WARNING,
                        Aspect::USER_INFO_MANAGER,
                        "ADS-IMPL-75") <<
                        FUN << ": Channel with BEHAV_PARAMS_LIST_ID = " <<
                        kbpi.id << " is incorrect.";
                    }
                  }
                  else if (bp.time_from < bp.time_to)
                  {
                    ChannelInterval ci(
                      Generics::Time(bp.time_from),
                      Generics::Time(bp.time_to),
                      bp.min_visits,
                      bp.weight);

                    if (bp.time_from == 0 && bp.min_visits == 1)
                    {
                      target_cip->contextual = true;
                    }

                    push_channel_interval_(target_cip, ci);
                  }
                  else
                  {
                    logger_->stream(
                      Logging::Logger::WARNING,
                      Aspect::USER_INFO_MANAGER,
                      "ADS-IMPL-74") <<
                      FUN << ": Channel with BEHAV_PARAMS_LIST_ID = " <<
                      kbpi.id << " is incorrect.";
                  }
                }
              }

              init_ht_candidate_templates_(
                page_cip,
                search_cip,
                url_cip,
                url_keyword_cip,
                audience_cip);

              str_behav_channel_interval_map[StrBehavIdTypeKey(
                kbpi.id, PAGE_CHANNEL)] = page_cip;
              str_behav_channel_interval_map[StrBehavIdTypeKey(
                kbpi.id, URL_CHANNEL)] = url_cip;
              str_behav_channel_interval_map[StrBehavIdTypeKey(
                kbpi.id, URL_KEYWORD_CHANNEL)] = url_keyword_cip;
              str_behav_channel_interval_map[StrBehavIdTypeKey(
                kbpi.id, SEARCH_CHANNEL)] = search_cip;
              str_behav_channel_interval_map[StrBehavIdTypeKey(
                kbpi.id, AUDIENCE_CHANNEL)] = audience_cip;
            }
          }
        }

        for (SimpleChannelPropertiesList::const_iterator it = channel_properties_list.begin();
          it != channel_properties_list.end(); ++it)
        {
          const SimpleChannelProperties& key = *it;

          if (key.behav_param_list_id != 0)
          {
            BehavIdChannelIntervalMap::const_iterator it =
              behav_channel_interval_map.find(
                BehavIdTypeKey(key.behav_param_list_id, PAGE_CHANNEL));

            if (it != behav_channel_interval_map.end())
            {
              config.channels_config->page_channels[key.channel_id] = it->second;
            }

            it = behav_channel_interval_map.find(
              BehavIdTypeKey(key.behav_param_list_id, SEARCH_CHANNEL));

            if (it != behav_channel_interval_map.end())
            {
              config.channels_config->search_channels[key.channel_id] = it->second;
            }

            it = behav_channel_interval_map.find(
              BehavIdTypeKey(key.behav_param_list_id, URL_CHANNEL));

            if (it != behav_channel_interval_map.end())
            {
              config.channels_config->url_channels[key.channel_id] = it->second;
            }

            it = behav_channel_interval_map.find(BehavIdTypeKey(
              key.behav_param_list_id, URL_KEYWORD_CHANNEL));

            if (it != behav_channel_interval_map.end())
            {
              config.channels_config->url_keyword_channels[key.channel_id] = it->second;
            }

            it = behav_channel_interval_map.find(
              BehavIdTypeKey(key.behav_param_list_id, AUDIENCE_CHANNEL));

            if (it != behav_channel_interval_map.end())
            {
              config.channels_config->audience_channels[key.channel_id] = it->second;
            }
          }
          else if (!key.str_behav_param_list_id.empty())
          {
            StrBehavIdChannelIntervalMap::const_iterator it =
              str_behav_channel_interval_map.find(StrBehavIdTypeKey(
                key.str_behav_param_list_id.c_str(), PAGE_CHANNEL));

            if (it != str_behav_channel_interval_map.end())
            {
              config.channels_config->page_channels[key.channel_id] = it->second;
            }

            it = str_behav_channel_interval_map.find(StrBehavIdTypeKey(
              key.str_behav_param_list_id.c_str(), SEARCH_CHANNEL));

            if (it != str_behav_channel_interval_map.end())
            {
              config.channels_config->search_channels[key.channel_id] = it->second;
            }

            it = str_behav_channel_interval_map.find(StrBehavIdTypeKey(
              key.str_behav_param_list_id.c_str(), URL_CHANNEL));

            if (it != str_behav_channel_interval_map.end())
            {
              config.channels_config->url_channels[key.channel_id] = it->second;
            }

            it = str_behav_channel_interval_map.find(StrBehavIdTypeKey(
              key.str_behav_param_list_id.c_str(), URL_KEYWORD_CHANNEL));

            if (it != str_behav_channel_interval_map.end())
            {
              config.channels_config->url_keyword_channels[key.channel_id] = it->second;
            }

            it = str_behav_channel_interval_map.find(StrBehavIdTypeKey(
              key.str_behav_param_list_id.c_str(), AUDIENCE_CHANNEL));

            if (it != str_behav_channel_interval_map.end())
            {
              config.channels_config->audience_channels[key.channel_id] = it->second;
            }
          }
        }

        if (res_length == 0)
        {
          logger_->log(
            String::SubString("Campaign server returns empty channel set."),
            Logging::Logger::WARNING,
            Aspect::USER_INFO_MANAGER,
            "ADS-IMPL-55");
        }

        CampaignSvcs::FreqCapConfigInfo_var freq_cap_config_info = campaign_server->freq_caps();

        apply_freq_caps_(*config.freq_cap_config, *freq_cap_config_info, confirm_timeout_);

        return config;
      }
      catch (const CampaignSvcs::CampaignServer::NotReady& ex)
      {
        campaign_server.release_bad(
          String::SubString("Campaign Server is not ready"));
        logger_->sstream(
          Logging::Logger::NOTICE,
          Aspect::USER_INFO_MANAGER,
          "ADS-ICON-10") << FUN << ": Can't update channels configuration. "
          "Campaign Server is not ready. Caught CampaignServer::NotReady: " <<
          ex.description;
      }
      catch (const CampaignSvcs::CampaignServer::
        ImplementationException& exc)
      {
        Stream::Error ostr;
        ostr << FUN << ": Can't update channels configuration. "
          "Caught CampaignServer::ImplementationException: " << exc.description;
        campaign_server.release_bad(ostr.str());
        logger_->log(
          ostr.str(),
          Logging::Logger::EMERGENCY,
          Aspect::USER_INFO_MANAGER,
          "ADS-IMPL-56");
      }
      catch (const CORBA::SystemException& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": Can't update channels configuration. "
          "Caught CORBA::SystemException: " << ex;
        campaign_server.release_bad(ostr.str());
        logger_->log(
          ostr.str(),
          Logging::Logger::EMERGENCY,
          Aspect::USER_INFO_MANAGER,
          "ADS-ICON-10");
      }
    }
  }
}
