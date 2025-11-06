#include <algorithm>
#include <optional>

#include <eh/Exception.hpp>
#include <Logger/Logger.hpp>
#include <Generics/Time.hpp>
#include <Generics/Rand.hpp>
#include <String/StringManip.hpp>
#include <String/UTF8Case.hpp>
#include <Generics/Uuid.hpp>

#include <Commons/CorbaConfig.hpp>
#include <Commons/CorbaAlgs.hpp>
#include <Commons/Algs.hpp>
#include <Commons/GrpcAlgs.hpp>
#include <Commons/UserverConfigUtils.hpp>

#include <LogCommons/AdRequestLogger.hpp>
#include <CampaignSvcs/CampaignServer/CampaignServer.hpp>

#include "CampaignManagerImpl.hpp"
#include "CampaignManagerDeclarations.hpp"
#include "Constants.hpp"

#include "CampaignManagerLogAdapter.hpp"
#include "CampaignManagerLogger.hpp"

#include "DomainParser.hpp"
#include <LogCommons/CsvUtils.hpp>

namespace AdServer
{
  namespace CampaignSvcs
  {
    namespace
    {
      const RevenueDecimal TOP_ALLOWABLE_LOSE_WIN_PERCENTAGE(
        RevenueDecimal::div(
          RevenueDecimal(false, 95, 0),
          RevenueDecimal(false, 100, 0)));
      const RevenueDecimal LOW_ALLOWABLE_LOSE_WIN_PERCENTAGE(
        RevenueDecimal::div(
          RevenueDecimal(false, 75, 0),
          RevenueDecimal(false, 100, 0)));
      const RevenueDecimal FULL_COMMISION = REVENUE_ONE;
      const unsigned long MAX_PUBPIXELS_PER_REQUEST = 10;

      const unsigned long UPDATE_TASKS_COUNT = 3;

      // REQ-3939. Player size in VAST
      const char VAST_APPLICATION[] = "rtb";
      const char VAST_SOURCE[] = "vast";
      const char VAST_OPERATION[] ="request";

      namespace PlatformNames
      {
        const std::string APPLE_IPADS("apple ipads");
        const std::string APPLE_IPHONES("apple iphones");
        const std::string ANDROID_TABLETS("android tablets");
        const std::string ANDROID_SMARTPHONES("android smartphones");
        const std::string WINDOWS_MOBILE("windows mobile");
        const std::string SMARTTV("smarttv");
        const std::string DVR("dvr");
        const std::string NON_MOBILE_DEVICES("non-mobile devices");
      }

      void
      append(
        FreqCapIdSet& freq_caps,
        const CampaignManager::FreqCapIdSeq& cap_seq)
        /*throw(eh::Exception)*/
      {
        std::copy(cap_seq.get_buffer(),
          cap_seq.get_buffer() + cap_seq.length(),
          std::inserter(freq_caps, freq_caps.end()));
      }

      void fill_expanding(
        const AdInstances::Creative::Size& size,
        CORBA::Octet& expanding)
      {
        if (size.expandable)
        {
          if (size.up_expand_space > 0)
          {
            expanding |= CampaignManager::CREATIVE_EXPANDING_UP;
          }
          if (size.right_expand_space > 0)
          {
            expanding |= CampaignManager::CREATIVE_EXPANDING_RIGHT;
          }
          if (size.down_expand_space > 0)
          {
            expanding |= CampaignManager::CREATIVE_EXPANDING_DOWN;
          }
          if (size.left_expand_space > 0)
          {
            expanding |= CampaignManager::CREATIVE_EXPANDING_LEFT;
          }
        }
      }

    void fill_expanding(
      const AdInstances::Creative::Size& size,
      std::uint32_t& expanding)
    {
      expanding = 0;
      if (size.expandable)
      {
        if (size.up_expand_space > 0)
        {
          expanding |= CREATIVE_EXPANDING_UP;
        }
        if (size.right_expand_space > 0)
        {
          expanding |= CREATIVE_EXPANDING_RIGHT;
        }
        if (size.down_expand_space > 0)
        {
          expanding |= CREATIVE_EXPANDING_DOWN;
        }
        if (size.left_expand_space > 0)
        {
          expanding |= CREATIVE_EXPANDING_LEFT;
        }
      }
    }

      void
      fill_passback_url_from_tag(
        std::string& passback_url,      
        const CreativeInstantiateRule& creative_instantiate_rule,
        const Tag* tag)
      {
        if(tag && !tag->passback.empty())
        {
          passback_url = tag->passback;

          if(!creative_instantiate_rule.instantiate_relative_protocol_url(
               passback_url) &&
             !HTTP::HTTP_PREFIX.start(passback_url) &&
             !HTTP::HTTPS_PREFIX.start(passback_url))
          {
            passback_url =
              creative_instantiate_rule.local_passback_prefix + passback_url;
          }
        }
      }

      void
      init_bid_request_params(
        BidCostProvider::RequestParams& bid_request_params,
        const AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
        const Tag* tag)
      {
        bid_request_params.tag_id = tag->tag_id;

        try
        {
          HTTP::BrowserAddress addr(
            String::SubString(request_params.common_info.referer.in()));

          if (!String::case_change<String::Uniform>(
              addr.host(),
              bid_request_params.domain))
          {
            bid_request_params.domain.clear();
          }
        }
        catch(const eh::Exception&)
        {}
      }

      void
      init_bid_request_params(
        BidCostProvider::RequestParams& bid_request_params,
        const Proto::RequestParams& request_params,
        const Tag* tag)
      {
        bid_request_params.tag_id = tag->tag_id;

        try
        {
          HTTP::BrowserAddress addr(
            String::SubString(request_params.common_info().referer()));

          if (!String::case_change<String::Uniform>(
              addr.host(),
              bid_request_params.domain))
          {
            bid_request_params.domain.clear();
          }
        }
        catch(const eh::Exception&)
        {}
      }
    }

    class CampaignOps
    {
    public:
      CampaignOps(const Campaign* campaign_val)
        : campaign_(campaign_val)
      {}

      const Creative* search_ccid(unsigned long ccid) noexcept
      {
        const CreativeList& creatives = campaign_->get_creatives();

        for(CreativeList::const_iterator cr_it = creatives.begin();
            cr_it != creatives.end(); ++cr_it)
        {
          if((*cr_it)->ccid == ccid)
          {
            return *cr_it;
          }
        }

        return 0;
      }

    private:
      const Campaign* campaign_;
    };

    /** CampaignManagerImpl::AdSlotMinCpm */
    CampaignManagerImpl::AdSlotMinCpm::AdSlotMinCpm() noexcept
      : min_pub_ecpm(RevenueDecimal::ZERO),
        min_pub_ecpm_system(RevenueDecimal::ZERO)
    {}

    /** CampaignManagerImpl::AdSlotContext */
    CampaignManagerImpl::AdSlotContext::AdSlotContext() noexcept
      : test_request(false),
        request_blacklisted(false),
        publisher_account_id(0)
    {}

    /**
     * Use this make-function to produce DebugInfo object and init its
     * members with default values
     */
    /*
    AdServer::CampaignSvcs::CampaignManager::DebugInfo*
    create_debug_info(
      const AdServer::CampaignSvcs::CampaignManager::RequestParams&
        request_params)
    {
      AdServer::CampaignSvcs::CampaignManager::DebugInfo* debug_info =
        new AdServer::CampaignSvcs::CampaignManager::DebugInfo();
      debug_info->tag_id = 0;
      debug_info->site_id = 0;
      debug_info->site_rate_id = 0;
      debug_info->colo_id = request_params.colo_id;
      debug_info->min_no_adv_ecpm = 0;
      debug_info->min_text_ecpm = 0;
      debug_info->cpm_threshold = 0;
      debug_info->walled_garden = 0;
      return debug_info;
    }
    */

    CampaignManagerImpl::CampaignManagerImpl(
      TaskProcessor& task_processor,
      const GrpcSchedulerPtr& grpc_scheduler,
      const CampaignManagerConfig& configuration,
      const DomainParser::DomainConfig& domain_config,
      Generics::ActiveObjectCallback* callback,
      Logging::Logger* logger,
      CampaignManagerLogger* campaign_manager_logger,
      const CreativeInstantiate& creative_instantiate,
      const char* campaigns_types)
      /*throw(InvalidArgument, Exception, eh::Exception)*/
      : campaign_manager_config_(configuration),
        callback_(ReferenceCounting::add_ref(callback)),
        logger_(ReferenceCounting::add_ref(logger)),
        domain_parser_(new DomainParser(domain_config)),
        campaign_manager_logger_(
          ReferenceCounting::add_ref(campaign_manager_logger)),
        creative_instantiate_(creative_instantiate),
        task_runner_(new Generics::TaskRunner(callback_, PARALLEL_TASKS_COUNT)),
        update_task_runner_(new Generics::TaskRunner(callback_, UPDATE_TASKS_COUNT)),
        scheduler_(new Generics::Planner(callback_)),
        rid_signer_(campaign_manager_config_.rid_private_key().c_str()),
        template_files_(new Commons::BoundedFileCache(
          campaign_manager_config_.Creative().ContentCache().size(),
          Generics::Time(campaign_manager_config_.Creative().ContentCache().timeout()),
          Commons::TextTemplateCacheConfiguration<Commons::File>(Generics::Time::ONE_SECOND)))
    {
      static const char* FUN = "CampaignManagerImpl::CampaignManagerImpl()";

      if(callback == 0 || logger == 0)
      {
        Stream::Error ostr;
        ostr << FUN << ":" << (callback == 0 ? " callback == 0;" : "") <<
          (logger == 0 ? " logger == 0;" : "");
        throw InvalidArgument(ostr);
      }

      token_to_parameters_.insert(std::make_pair(
        String::SubString("APPLICATION_ID"),
        "appid"));
      token_to_parameters_.insert(std::make_pair(
        String::SubString("ADVERTISING_ID"),
        "adid"));
      token_to_parameters_.insert(std::make_pair(
        String::SubString("TNS_COUNTER_DEVICE_TYPE"),
        "tdt"));
      token_to_parameters_.insert(std::make_pair(
        String::SubString("EXT_TRACK_PARAMS"),
        "ep"));

      try
      {
        add_child_object(task_runner_);
        add_child_object(update_task_runner_);
        add_child_object(scheduler_);

        if (campaign_manager_config_.KafkaAdsSpacesStorage().present())
        {
          kafka_producer_ = new Commons::Kafka::Producer(
            logger, callback,
            campaign_manager_config_.KafkaAdsSpacesStorage().get());
          add_child_object(kafka_producer_);
        }

        if (campaign_manager_config_.KafkaMatchStorage().present())
        {
          kafka_match_producer_ = new Commons::Kafka::Producer(
            logger, callback,
            campaign_manager_config_.KafkaMatchStorage().get());
          add_child_object(kafka_match_producer_);
        }
      }
      catch(const Generics::CompositeActiveObject::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN <<
          ": CompositeActiveObject::Exception caught: " << ex.what();
        throw Exception(ostr);
      }

      if (campaign_manager_config_.SecurityToken().present())
      {
        try
        {
          const xsd::AdServer::Configuration::SecurityTokenType& sec_token =
            *campaign_manager_config_.SecurityToken();
          Commons::SecTokenGenerator::Config sec_token_config;
          sec_token_config.aes_keys.push_back(sec_token.key0());
          sec_token_config.aes_keys.push_back(sec_token.key1());
          sec_token_config.aes_keys.push_back(sec_token.key2());
          sec_token_config.aes_keys.push_back(sec_token.key3());
          sec_token_config.aes_keys.push_back(sec_token.key4());
          sec_token_config.aes_keys.push_back(sec_token.key5());
          sec_token_config.aes_keys.push_back(sec_token.key6());

          sec_token_generator_ = new Commons::SecTokenGenerator(
            sec_token_config);
        }
        catch(const eh::Exception& e)
        {
          Stream::Error ostr;
          ostr << FUN <<
            ": Can't initialize security token generator: " << e.what();
          throw Exception(ostr);
        }
      }

      if (campaign_manager_config_.IpEncryptConfig().present())
      {
        try
        {
          ip_crypter_ = new Commons::IPCrypter(
            campaign_manager_config_.IpEncryptConfig()->key());
        }
        catch(const eh::Exception& e)
        {
          Stream::Error ostr;
          ostr << FUN <<
            ": Can't initialize ip crypter: " << e.what();
          throw Exception(ostr);
        }
      }

      if (campaign_manager_config_.CountryWhitelist().present())
      {
        for (auto it = campaign_manager_config_.CountryWhitelist()->Country().begin();
             it != campaign_manager_config_.CountryWhitelist()->Country().end(); ++it)
        {
          country_whitelist_.push_back(it->country_code());
        }
      }

      try
      {
        campaign_config_source_ = new CampaignConfigSource(
          logger,
          domain_parser_,
          Config::CorbaConfigReader::read_multi_corba_ref(
            campaign_manager_config_.CampaignServerCorbaRef()),
          campaigns_types,
          campaign_manager_config_.Creative().creative_file_dir().c_str(),
          campaign_manager_config_.Creative().template_file_dir().c_str(),
          campaign_manager_config_.service_index(),
          creative_instantiate_.creative_rules,
          campaign_manager_config_.Creative().drop_https_safe().present() &&
            *campaign_manager_config_.Creative().drop_https_safe()
          );

        add_child_object(campaign_config_source_);
      }
      catch(const eh::Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN <<
          ": Can't init CampaignConfigSource: " << e.what();
        throw Exception(ostr);
      }

      if(campaign_manager_config_.Billing().present() && (
           campaign_manager_config_.Billing()->check_bids() ||
           campaign_manager_config_.Billing()->confirm_bids()))
      {
        try
        {
          GrpcBillingStateContainer::Endpoints endpoints;
          const auto& endpoints_config =
            campaign_manager_config_.Billing()->BillingServerEndpointList().Endpoint();
          for (const auto& endpoint_config : endpoints_config)
          {
            endpoints.emplace_back(
              endpoint_config.host(),
              endpoint_config.port());
          }

          const auto config_grpc_client_data = Config::create_pool_client_config(
            campaign_manager_config_.Billing()->BillingGrpcClientPool());

          GrpcBillingStateContainer_var billing_state_container =
            new GrpcBillingStateContainer(
              callback_,
              logger,
              100, // max use count
              campaign_manager_config_.Billing()->optimize_campaign_ctr(),
              task_processor,
              grpc_scheduler,
              endpoints,
              config_grpc_client_data.first,
              config_grpc_client_data.second);

          if(campaign_manager_config_.Billing()->check_bids())
          {
            check_billing_state_container_ = billing_state_container;
          }

          if(campaign_manager_config_.Billing()->confirm_bids())
          {
            confirm_billing_state_container_ = billing_state_container;
          }

          add_child_object(billing_state_container);
        }
        catch(const eh::Exception& e)
        {
          Stream::Error ostr;
          ostr << FUN << ": Can't init BillingStateContainer: " << e.what();
          throw Exception(ostr);
        }
      }

      try
      {
        CampaignManagerTaskMessage_var msg =
          new CheckConfigTaskMessage(this, update_task_runner_);
        update_task_runner_->enqueue_task(msg);

        update_task_runner_->enqueue_task(CampaignManagerTaskMessage_var(
          new UpdateCTRProviderTask(this, update_task_runner_)));

        update_task_runner_->enqueue_task(CampaignManagerTaskMessage_var(
          new UpdateConvRateProviderTask(this, update_task_runner_)));

        update_task_runner_->enqueue_task(CampaignManagerTaskMessage_var(
          new UpdateBidCostProviderTask(this, update_task_runner_)));

        msg = new FlushLogsTaskMessage(this, task_runner_);
        task_runner_->enqueue_task(msg);
      }
      catch (const CampaignManagerLogger::Exception &ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": CampaignManagerLogger::Exception caught: " <<
          ex.what();
        throw Exception(ostr);
      }
      catch (const Generics::Planner::Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN <<
          ": Generics::Planner::Exception caught: " << e.what();
        throw Exception(ostr);
      }
      catch (const Generics::TaskRunner::Overflow& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": Generics::TaskRunner::Overflow caught: " << e.what();
        throw Exception(ostr);
      }
      catch (const eh::Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << e.what();
        throw Exception(ostr);
      }
    }

    bool CampaignManagerImpl::ready() /*throw(eh::Exception)*/
    {
      CampaignIndex_var config_index = configuration_index();
      return config_index.in() != 0;
    }

    bool
    CampaignManagerImpl::match_geo_channels_(
      const AdServer::CampaignSvcs::CampaignManager::GeoInfoSeq& location,
      const AdServer::CampaignSvcs::CampaignManager::GeoCoordInfoSeq& coord_location,
      ChannelIdList& geo_channels,
      ChannelIdSet& coord_channels)
      /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException,
        AdServer::CampaignSvcs::CampaignManager::NotReady)*/
    {
      // static const char* FUN = "CampaignManagerImpl::match_geo_channels_()";
      CampaignConfig_var campaign_config = configuration();

      if (!campaign_config)
      {
        throw AdServer::CampaignSvcs::CampaignManager::NotReady(
          "Campaign configuration isn't loaded");
      }

      if(location.length() || coord_location.length())
      {
        // REVIEW
        // location max size == 1
        if(location.length())
        {
          campaign_config->geo_channels->match(
            geo_channels,
            String::SubString(location[0].country),
            String::SubString(location[0].region),
            String::SubString(location[0].city));
        }

        // pack channels (some can be matched by few points)
        for(CORBA::ULong loc_i = 0;
            loc_i < coord_location.length();
            ++loc_i)
        {
          campaign_config->geo_coord_channels->match(
            coord_channels,
            CorbaAlgs::unpack_decimal<CoordDecimal>(coord_location[loc_i].longitude),
            CorbaAlgs::unpack_decimal<CoordDecimal>(coord_location[loc_i].latitude),
            CorbaAlgs::unpack_decimal<CoordDecimal>(coord_location[loc_i].accuracy));
        }

        return true;
      }
      return false;
    }

    bool
    CampaignManagerImpl::match_geo_channels_(
      const google::protobuf::RepeatedPtrField<Proto::GeoInfo>& location,
      const google::protobuf::RepeatedPtrField<Proto::GeoCoordInfo>& coord_location,
      ChannelIdList& geo_channels,
      ChannelIdSet& coord_channels)
    {
      CampaignConfig_var campaign_config = configuration(false);
      if (!campaign_config)
      {
        throw NotReady("Campaign configuration isn't loaded");
      }

      if(!location.empty() || !coord_location.empty())
      {
        if(!location.empty())
        {
          campaign_config->geo_channels->match(
            geo_channels,
            String::SubString(location[0].country()),
            String::SubString(location[0].region()),
            String::SubString(location[0].city()));
        }

        // pack channels (some can be matched by few points)
        for(const auto& data : coord_location)
        {
          campaign_config->geo_coord_channels->match(
            coord_channels,
            GrpcAlgs::unpack_decimal<CoordDecimal>(data.longitude()),
            GrpcAlgs::unpack_decimal<CoordDecimal>(data.latitude()),
            GrpcAlgs::unpack_decimal<CoordDecimal>(data.accuracy()));
        }

        return true;
      }

      return false;
    }

    void
    CampaignManagerImpl::match_geo_channels(
      const AdServer::CampaignSvcs::CampaignManager::GeoInfoSeq& location,
      const AdServer::CampaignSvcs::CampaignManager::GeoCoordInfoSeq& coord_location,
      AdServer::CampaignSvcs::ChannelIdSeq_out geo_channels_result,
      AdServer::CampaignSvcs::ChannelIdSeq_out coord_channels_result)
      /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException,
        AdServer::CampaignSvcs::CampaignManager::NotReady)*/
    {
      // static const char* FUN = "CampaignManagerImpl::match_geo_channels()";

      ChannelIdList geo_channels;
      ChannelIdSet coord_channels;

      geo_channels_result = new AdServer::CampaignSvcs::ChannelIdSeq();
      coord_channels_result = new AdServer::CampaignSvcs::ChannelIdSeq();

      match_geo_channels_(location, coord_location, geo_channels, coord_channels);

      if(!geo_channels.empty())
      {
        CorbaAlgs::fill_sequence(
          geo_channels.begin(),
          geo_channels.end(),
          *geo_channels_result);
      }

      if(!coord_channels.empty())
      {
        CorbaAlgs::fill_sequence(
          coord_channels.begin(),
          coord_channels.end(),
          *coord_channels_result);
      }
    }

    CampaignManagerImpl::MatchGeoChannelsResponsePtr
    CampaignManagerImpl::match_geo_channels(
      MatchGeoChannelsRequestPtr&& request)
    {
      const auto id_request_grpc = request->id_request_grpc();
      try
      {
        auto response = create_grpc_response<Proto::MatchGeoChannelsResponse>(
          id_request_grpc);
        auto* info_response = response->mutable_info();
        auto* geo_channels_response = info_response->mutable_geo_channels();
        auto* coord_channels_response = info_response->mutable_coord_channels();

        ChannelIdList geo_channels;
        ChannelIdSet coord_channels;

        const auto& location = request->location();
        const auto& coord_location = request->coord_location();

        match_geo_channels_(location, coord_location, geo_channels, coord_channels);

        if(!geo_channels.empty())
        {
          geo_channels_response->Add(
            std::begin(geo_channels),
            std::end(geo_channels));
        }

        if(!coord_channels.empty())
        {
          coord_channels_response->Add(
            std::begin(coord_channels),
            std::end(coord_channels));
        }

        return response;
      }
      catch (const NotReady& exc)
      {
        Stream::Error stream;
        stream << FNS
               << "Caught CampaignManager::NotReady: "
               << exc.what();
        auto response = create_grpc_error_response<Proto::MatchGeoChannelsResponse>(
          Proto::Error_Type::Error_Type_NotReady,
          stream.str(),
          id_request_grpc);
        return response;
      }
      catch (const eh::Exception& exc)
      {
        Stream::Error stream;
        stream << FNS
               << ": "
               << exc.what();
        auto response = create_grpc_error_response<Proto::MatchGeoChannelsResponse>(
          Proto::Error_Type::Error_Type_Implementation,
          stream.str(),
          id_request_grpc);
        return response;
      }
      catch (...)
      {
        Stream::Error stream;
        stream << FNS
               << ": Unknown error";
        auto response = create_grpc_error_response<Proto::MatchGeoChannelsResponse>(
          Proto::Error_Type::Error_Type_Implementation,
          stream.str(),
          id_request_grpc);
        return response;
      }
    }

    void
    CampaignManagerImpl::get_campaign_creative(
      const AdServer::CampaignSvcs::CampaignManager::RequestParams&
        request_params,
      CORBA::String_out hostname,
      AdServer::CampaignSvcs::CampaignManager::RequestCreativeResult_out request_result)
      /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException,
        AdServer::CampaignSvcs::CampaignManager::NotReady)*/
    {
      static const char* FUN = "CampaignManagerImpl::get_campaign_creative()";

      try
      {
        Generics::Timer process_timer;
        process_timer.start();
        hostname << campaign_manager_config_.host();

        request_result =
          new AdServer::CampaignSvcs::CampaignManager::RequestCreativeResult();

        AdServer::CampaignSvcs::CampaignManager::AdRequestDebugInfo*
          request_debug_info = request_params.need_debug_info ?
            &request_result->debug_info : 0;

        if(request_debug_info)
        {
          request_debug_info->colo_id = request_params.common_info.colo_id;
          request_debug_info->user_group_id = 0;
        }

        CampaignConfig_var campaign_config = configuration();

        if (!campaign_config)
        {
          throw AdServer::CampaignSvcs::CampaignManager::NotReady(
            "Campaign configuration isn't loaded");
        }

        AdServer::CampaignSvcs::CampaignManager::RequestParams
          filtered_request_params(request_params);

        const Colocation* colocation = 0;

        // check input colo
        if(request_params.common_info.colo_id == 0 ||
           (request_params.common_info.colo_id !=
             campaign_manager_config_.colocation_id() &&
           campaign_config->colocations.find(
             request_params.common_info.colo_id) ==
             campaign_config->colocations.end()))
        {
          filtered_request_params.common_info.colo_id =
            campaign_manager_config_.colocation_id();
        }

        CampaignConfig::ColocationMap::const_iterator colo_it =
          campaign_config->colocations.find(
            filtered_request_params.common_info.colo_id);

        if (colo_it != campaign_config->colocations.end())
        {
          colocation = colo_it->second;

          filtered_request_params.common_info.test_request |=
            colo_it->second->is_test();
        }

        ChannelIdList geo_channels;
        ChannelIdSet coord_channels;
        match_geo_channels_(
          filtered_request_params.common_info.location,
          filtered_request_params.common_info.coord_location,
          geo_channels,
          coord_channels);

        {
          if(!coord_channels.empty())
          {
            std::copy(coord_channels.begin(),
              coord_channels.end(),
              std::back_inserter(geo_channels));
          }

          if(!geo_channels.empty())
          {
            CorbaAlgs::fill_sequence(
              geo_channels.begin(),
              geo_channels.end(),
              filtered_request_params.context_info.geo_channels,
              true);
          }

          CorbaAlgs::copy_sequence(
            filtered_request_params.context_info.geo_channels,
            filtered_request_params.channels,
            true);
        }

        CorbaAlgs::copy_sequence(
          filtered_request_params.context_info.platform_ids,
          filtered_request_params.channels,
          true);

        ChannelIdHashSet matched_channels(
          filtered_request_params.channels.get_buffer(),
          filtered_request_params.channels.get_buffer() + filtered_request_params.channels.length());

        unsigned long request_tag_id = 0;

        AdServer::CampaignSvcs::RevenueDecimal adsspace_system_cpm =
          AdServer::CampaignSvcs::RevenueDecimal(false, 100000, 0);

        if(filtered_request_params.ad_slots.length())
        {
          //adsspace_system_cpm = AdServer::CampaignSvcs::RevenueDecimal::ZERO;

          // check global blacklist channel
          bool request_blacklisted = size_blacklisted_(
            campaign_config,
            matched_channels,
            0 // global blacklist
            );

          if(request_blacklisted)
          {
            filtered_request_params.common_info.user_status =
              static_cast<CORBA::ULong>(US_BLACKLISTED);

            for(CORBA::ULong slot_i = 0;
                slot_i < filtered_request_params.ad_slots.length(); ++slot_i)
            {
              filtered_request_params.ad_slots[slot_i].passback = true;
            }
          }

          // ad request
          CreativeInstantiateRuleMap::iterator rule_it =
            creative_instantiate_.creative_rules.find(
              filtered_request_params.common_info.creative_instantiate_type.in());

          if(rule_it == creative_instantiate_.creative_rules.end())
          {
            Stream::Error err;
            err << FUN <<
              ": cannot find creative instantiate rule with name = '" <<
              filtered_request_params.common_info.creative_instantiate_type << "'.";
            CORBACommons::throw_desc<
              CampaignSvcs::CampaignManager::ImplementationException>(err.str());
          }

          // fill common params
          filtered_request_params.tag_delivery_factor =
            Generics::safe_rand() % TAG_DELIVERY_MAX;
          filtered_request_params.ccg_delivery_factor =
            Generics::safe_rand() % TAG_DELIVERY_MAX;

          const Generics::Time session_start = CorbaAlgs::unpack_time(
            filtered_request_params.session_start);

          AdSlotContext ad_slot_context;
          ad_slot_context.request_blacklisted = request_blacklisted;

          ad_slot_context.test_request =
          filtered_request_params.common_info.test_request;
          append(
            ad_slot_context.full_freq_caps,
            filtered_request_params.full_freq_caps);

          // process slots
          request_result->ad_slots.length(filtered_request_params.ad_slots.length());

          for(CORBA::ULong ad_slot_i = 0;
              ad_slot_i < filtered_request_params.ad_slots.length();
              ++ad_slot_i)
          {
            // !!! request_result->test_request = filtered_request_params.test_request;
            AdServer::CampaignSvcs::CampaignManager::
              AdSlotResult& ad_slot_result = request_result->ad_slots[ad_slot_i];
            const AdServer::CampaignSvcs::CampaignManager::
              AdSlotInfo& ad_slot = filtered_request_params.ad_slots[ad_slot_i];
            AdServer::CampaignSvcs::CampaignManager::AdSlotDebugInfo*
              ad_slot_debug_info = filtered_request_params.need_debug_info ?
                &ad_slot_result.debug_info : 0;

            ad_slot_result.ad_slot_id = ad_slot.ad_slot_id;
            ad_slot_context.passback_url.clear();

            get_adslot_campaign_creative_(
              campaign_config,
              *request_result,
              ad_slot_result,
              adsspace_system_cpm,
              filtered_request_params,
              session_start,
              colocation,
              ad_slot,
              rule_it->second,
              request_debug_info,
              ad_slot_debug_info,
              ad_slot_context,
              matched_channels,
              request_tag_id);

            if(ad_slot_result.selected_creatives.length() > 0)
            {
              ad_slot_result.tag_size << ad_slot_context.tag_size;
            }
          }
        }
        else
        {
          // profiling request
          CampaignManagerLogger::RequestInfo request_info;
          AdSlotContext ad_slot_context;
          ad_slot_context.test_request =
            filtered_request_params.common_info.test_request;

          CampaignManagerLogAdapter::fill_request_info(
            request_info,
            campaign_config,
            colocation,
            filtered_request_params.common_info,
            filtered_request_params.context_info,
            &filtered_request_params,
            request_debug_info,
            ad_slot_context);

          campaign_manager_logger_->process_request(request_info, request_params.profiling_type);
        }

        // fill request level debug info
        if(request_debug_info)
        {
          CorbaAlgs::copy_sequence(
            filtered_request_params.context_info.geo_channels,
            request_result->debug_info.geo_channels);
        }
        
        // Log ads space info
        produce_ads_space_(
          request_params,
          request_tag_id,
          adsspace_system_cpm);

        process_timer.stop();
        request_result->process_time = CorbaAlgs::pack_time(
          process_timer.elapsed_time());
      }
      catch (const eh::Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << e.what();
        CORBACommons::throw_desc<
          CampaignSvcs::CampaignManager::ImplementationException>(
            ostr.str());
      }
    }

    CampaignManagerImpl::GetCampaignCreativeResponsePtr
    CampaignManagerImpl::get_campaign_creative(
      GetCampaignCreativeRequestPtr&& request)
    {
      static const char* FUN = "CampaignManagerImpl::get_campaign_creative()";

      const auto id_request_grpc = request->id_request_grpc();
      try
      {
        const auto& request_params = request->request_params();

        auto response = create_grpc_response<Proto::GetCampaignCreativeResponse>(
          id_request_grpc);
        auto* info_response = response->mutable_info();
        auto* request_result_response = info_response->mutable_request_result();

        Generics::Timer process_timer;
        process_timer.start();
        info_response->set_hostname(campaign_manager_config_.host());

        Proto::AdRequestDebugInfo* request_debug_info =
          request_params.need_debug_info() ?
            request_result_response->mutable_debug_info() : nullptr;

        if(request_debug_info)
        {
          request_debug_info->set_colo_id(request_params.common_info().colo_id());
          request_debug_info->set_user_group_id(0);
        }

        CampaignConfig_var campaign_config = configuration(false);
        if (!campaign_config)
        {
          throw NotReady("Campaign configuration isn't loaded");
        }

        Proto::RequestParams filtered_request_params(request_params);

        const Colocation* colocation = nullptr;

        // check input colo
        if(request_params.common_info().colo_id() == 0 ||
           (request_params.common_info().colo_id() !=
             campaign_manager_config_.colocation_id() &&
           campaign_config->colocations.find(
             request_params.common_info().colo_id()) ==
             campaign_config->colocations.end()))
        {
          filtered_request_params.mutable_common_info()->set_colo_id(
            campaign_manager_config_.colocation_id());
        }

        const auto colo_it = campaign_config->colocations.find(
          filtered_request_params.common_info().colo_id());

        if (colo_it != campaign_config->colocations.end())
        {
          colocation = colo_it->second;
          const auto test_request =
            filtered_request_params.common_info().test_request() || colo_it->second->is_test();
          filtered_request_params.mutable_common_info()->set_test_request(test_request);
        }

        ChannelIdList geo_channels;
        ChannelIdSet coord_channels;
        match_geo_channels_(
          filtered_request_params.common_info().location(),
          filtered_request_params.common_info().coord_location(),
          geo_channels,
          coord_channels);

        {
          if(!coord_channels.empty())
          {
            geo_channels.insert(
              std::end(geo_channels),
              std::begin(coord_channels),
              std::end(coord_channels));
          }

          if(!geo_channels.empty())
          {
            filtered_request_params.mutable_context_info()->mutable_geo_channels()->Add(
              std::begin(geo_channels),
              std::end(geo_channels));
          }

          if (!filtered_request_params.context_info().geo_channels().empty())
          {
            const auto& geo_channels = filtered_request_params.context_info().geo_channels();
            filtered_request_params.mutable_channels()->Add(
              std::begin(geo_channels),
              std::end(geo_channels));
          }
        }

        if (!filtered_request_params.context_info().platform_ids().empty())
        {
          const auto& platform_ids = filtered_request_params.context_info().platform_ids();
          filtered_request_params.mutable_channels()->Add(
            std::begin(platform_ids),
            std::end(platform_ids));
        }

        ChannelIdHashSet matched_channels(
          std::begin(filtered_request_params.channels()),
          std::end(filtered_request_params.channels()));

        unsigned long request_tag_id = 0;

        AdServer::CampaignSvcs::RevenueDecimal adsspace_system_cpm =
          AdServer::CampaignSvcs::RevenueDecimal(false, 100000, 0);

        if(!filtered_request_params.ad_slots().empty())
        {
          // check global blacklist channel
          bool request_blacklisted = size_blacklisted_(
            campaign_config,
            matched_channels,
            0 // global blacklist
            );

          if(request_blacklisted)
          {
            filtered_request_params.mutable_common_info()->set_user_status(
              static_cast<std::uint32_t>(US_BLACKLISTED));

            for(int slot_i = 0;
                slot_i < filtered_request_params.ad_slots().size(); ++slot_i)
            {
              filtered_request_params.mutable_ad_slots(slot_i)->set_passback(true);
            }
          }

          // ad request
          const auto rule_it = creative_instantiate_.creative_rules.find(
            filtered_request_params.common_info().creative_instantiate_type());

          if(rule_it == creative_instantiate_.creative_rules.end())
          {
            Stream::Error err;
            err << FUN <<
              ": cannot find creative instantiate rule with name = '" <<
              filtered_request_params.common_info().creative_instantiate_type() << "'.";
            throw ImplementationException(err.str());
          }

          // fill common params
          filtered_request_params.set_tag_delivery_factor(
            Generics::safe_rand() % TAG_DELIVERY_MAX);
          filtered_request_params.set_ccg_delivery_factor(
            Generics::safe_rand() % TAG_DELIVERY_MAX);

          const Generics::Time session_start = GrpcAlgs::unpack_time(
            filtered_request_params.session_start());

          AdSlotContext ad_slot_context;
          ad_slot_context.request_blacklisted = request_blacklisted;

          ad_slot_context.test_request =
            filtered_request_params.common_info().test_request();
          filtered_request_params.mutable_full_freq_caps()->Add(
            std::begin(ad_slot_context.full_freq_caps),
            std::end(ad_slot_context.full_freq_caps));

          // process slots
          auto* ad_slots_response = request_result_response->mutable_ad_slots();
          ad_slots_response->Reserve(filtered_request_params.ad_slots().size());
          for(const auto& ad_slot : filtered_request_params.ad_slots())
          {
            auto& ad_slot_result = *ad_slots_response->Add();
            Proto::AdSlotDebugInfo* ad_slot_debug_info =
              filtered_request_params.need_debug_info() ?
                ad_slot_result.mutable_debug_info() : nullptr;

            ad_slot_result.set_ad_slot_id(ad_slot.ad_slot_id());
            ad_slot_context.passback_url.clear();

            get_adslot_campaign_creative_(
              campaign_config,
              *request_result_response,
              ad_slot_result,
              adsspace_system_cpm,
              filtered_request_params,
              session_start,
              colocation,
              ad_slot,
              rule_it->second,
              request_debug_info,
              ad_slot_debug_info,
              ad_slot_context,
              matched_channels,
              request_tag_id);

            if(ad_slot_result.selected_creatives().size() > 0)
            {
              ad_slot_result.set_tag_size(ad_slot_context.tag_size);
            }
          }
        }
        else
        {
          // profiling request
          CampaignManagerLogger::RequestInfo request_info;
          AdSlotContext ad_slot_context;
          ad_slot_context.test_request =
            filtered_request_params.common_info().test_request();

          CampaignManagerLogAdapter::fill_request_info(
            request_info,
            campaign_config,
            colocation,
            filtered_request_params.common_info(),
            filtered_request_params.context_info(),
            &filtered_request_params,
            request_debug_info,
            ad_slot_context);

          campaign_manager_logger_->process_request(
            request_info,
            request_params.profiling_type());
        }

        // fill request level debug info
        if(request_debug_info)
        {
          const auto& geo_channels = filtered_request_params.context_info().geo_channels();
          auto* geo_channels_response =
            request_result_response->mutable_debug_info()->mutable_geo_channels();
          geo_channels_response->Add(
            std::begin(geo_channels),
            std::end(geo_channels));
        }

        // Log ads space info
        produce_ads_space_(
          request_params,
          request_tag_id,
          adsspace_system_cpm);

        process_timer.stop();
        request_result_response->set_process_time(GrpcAlgs::pack_time(
          process_timer.elapsed_time()));

        return response;
      }
      catch (const NotReady& exc)
      {
        Stream::Error stream;
        stream << FNS
               << "Caught CampaignManager::NotReady: "
               << exc.what();
        auto response = create_grpc_error_response<Proto::GetCampaignCreativeResponse>(
          Proto::Error_Type::Error_Type_NotReady,
          stream.str(),
          id_request_grpc);
        return response;
      }
      catch (const eh::Exception& exc)
      {
        Stream::Error stream;
        stream << FNS
               << ": "
               << exc.what();
        auto response = create_grpc_error_response<Proto::GetCampaignCreativeResponse>(
          Proto::Error_Type::Error_Type_Implementation,
          stream.str(),
          id_request_grpc);
        return response;
      }
      catch (...)
      {
        Stream::Error stream;
        stream << FNS
               << ": Unknown error";
        auto response = create_grpc_error_response<Proto::GetCampaignCreativeResponse>(
          Proto::Error_Type::Error_Type_Implementation,
          stream.str(),
          id_request_grpc);
        return response;
      }
    }

    void
    CampaignManagerImpl::process_match_request(
      const AdServer::CampaignSvcs::CampaignManager::MatchRequestInfo&
        match_request_info)
      /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException,
        AdServer::CampaignSvcs::CampaignManager::NotReady)*/
    {
      static const char* FUN = "CampaignManagerImpl::process_match_request()";

      try
      {
        CampaignConfig_var campaign_config = configuration();

        if (!campaign_config)
        {
          throw AdServer::CampaignSvcs::CampaignManager::NotReady(
            "Campaign configuration isn't loaded");
        }

        CampaignManagerLogger::MatchRequestInfo mri;

        ChannelIdList geo_channels;
        ChannelIdSet coord_channels;

        match_geo_channels_(
           match_request_info.match_info.location,
           match_request_info.match_info.coord_location,
           geo_channels,
           coord_channels);

        std::copy(
          coord_channels.begin(),
          coord_channels.end(),
          std::back_inserter(geo_channels));

        CampaignManagerLogAdapter::fill_match_request_info(
          mri,
          campaign_config,
          match_request_info,
          geo_channels);

        campaign_manager_logger_->process_match_request(mri);

        // Log ads space info
        produce_match_(match_request_info);
      }
      catch (const eh::Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << e.what();
        CORBACommons::throw_desc<
          CampaignSvcs::CampaignManager::ImplementationException>(
            ostr.str());
      }
    }

    CampaignManagerImpl::ProcessMatchRequestResponsePtr
    CampaignManagerImpl::process_match_request(
      ProcessMatchRequestRequestPtr&& request)
    {
      const auto id_request_grpc = request->id_request_grpc();
      try
      {
        CampaignConfig_var campaign_config = configuration(false);
        if (!campaign_config)
        {
          throw NotReady("Campaign configuration isn't loaded");
        }

        const auto& match_request_info = request->match_request_info();

        CampaignManagerLogger::MatchRequestInfo mri;

        ChannelIdList geo_channels;
        ChannelIdSet coord_channels;

        match_geo_channels_(
          match_request_info.match_info().location(),
          match_request_info.match_info().coord_location(),
          geo_channels,
          coord_channels);

        std::copy(
          coord_channels.begin(),
          coord_channels.end(),
          std::back_inserter(geo_channels));

        CampaignManagerLogAdapter::fill_match_request_info(
          mri,
          campaign_config,
          match_request_info,
          geo_channels);

        campaign_manager_logger_->process_match_request(mri);

        // Log ads space info
        produce_match_(match_request_info);

        auto response = create_grpc_response<Proto::ProcessMatchRequestResponse>(
          id_request_grpc);
        response->mutable_info();
        return response;
      }
      catch (const NotReady& exc)
      {
        Stream::Error stream;
        stream << FNS
               << "Caught CampaignManager::NotReady: "
               << exc.what();
        auto response = create_grpc_error_response<Proto::ProcessMatchRequestResponse>(
          Proto::Error_Type::Error_Type_NotReady,
          stream.str(),
          id_request_grpc);
        return response;
      }
      catch (const eh::Exception& exc)
      {
        Stream::Error stream;
        stream << FNS
               << ": "
               << exc.what();
        auto response = create_grpc_error_response<Proto::ProcessMatchRequestResponse>(
          Proto::Error_Type::Error_Type_Implementation,
          stream.str(),
          id_request_grpc);
        return response;
      }
      catch (...)
      {
        Stream::Error stream;
        stream << FNS
               << ": Unknown error";
        auto response = create_grpc_error_response<Proto::ProcessMatchRequestResponse>(
          Proto::Error_Type::Error_Type_Implementation,
          stream.str(),
          id_request_grpc);
        return response;
      }
    }

    void
    CampaignManagerImpl::process_anonymous_request(
      const AdServer::CampaignSvcs::CampaignManager::AnonymousRequestInfo&
        anon_request_info)
      /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException,
        AdServer::CampaignSvcs::CampaignManager::NotReady)*/
    {
      static const char* FUN = "CampaignManagerImpl::process_anonymous_request()";

      try
      {
        CampaignConfig_var campaign_config = configuration();

        if (!campaign_config)
        {
          throw AdServer::CampaignSvcs::CampaignManager::NotReady(
            "Campaign configuration isn't loaded");
        }

        CampaignManagerLogger::AnonymousRequestInfo logger_info;
        logger_info.colo_id = anon_request_info.colo_id;

        if(anon_request_info.colo_id == 0 ||
           (anon_request_info.colo_id !=
             campaign_manager_config_.colocation_id() &&
           campaign_config->colocations.find(
               anon_request_info.colo_id) ==
             campaign_config->colocations.end()))
        {
          logger_info.colo_id = campaign_manager_config_.colocation_id();
        }

        CampaignManagerLogAdapter::fill(
          campaign_config,
          anon_request_info,
          logger_info);

        campaign_manager_logger_->process_anon_request(logger_info);
      }
      catch (const eh::Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << e.what();
        CORBACommons::throw_desc<
          CampaignSvcs::CampaignManager::ImplementationException>(
            ostr.str());
      }
    }

    CampaignManagerImpl::ProcessAnonymousRequestResponsePtr
    CampaignManagerImpl::process_anonymous_request(
      ProcessAnonymousRequestRequestPtr&& request)
    {
      const auto id_request_grpc = request->id_request_grpc();
      try
      {
        CampaignConfig_var campaign_config = configuration(false);
        if (!campaign_config)
        {
          throw NotReady("Campaign configuration isn't loaded");
        }

        const auto& anon_request_info = request->anon_request_info();

        CampaignManagerLogger::AnonymousRequestInfo logger_info;
        logger_info.colo_id = anon_request_info.colo_id();

        if(anon_request_info.colo_id() == 0 ||
           (anon_request_info.colo_id() !=
             campaign_manager_config_.colocation_id() &&
           campaign_config->colocations.find(
               anon_request_info.colo_id()) ==
             campaign_config->colocations.end()))
        {
          logger_info.colo_id = campaign_manager_config_.colocation_id();
        }

        CampaignManagerLogAdapter::fill(
          campaign_config,
          anon_request_info,
          logger_info);

        campaign_manager_logger_->process_anon_request(logger_info);

        auto response = create_grpc_response<Proto::ProcessAnonymousRequestResponse>(
          id_request_grpc);
        response->mutable_info();
        return response;
      }
      catch (const NotReady& exc)
      {
        Stream::Error stream;
        stream << FNS
               << "Caught CampaignManager::NotReady: "
               << exc.what();
        auto response = create_grpc_error_response<Proto::ProcessAnonymousRequestResponse>(
          Proto::Error_Type::Error_Type_NotReady,
          stream.str(),
          id_request_grpc);
        return response;
      }
      catch (const eh::Exception& exc)
      {
        Stream::Error stream;
        stream << FNS
               << ": "
               << exc.what();
        auto response = create_grpc_error_response<Proto::ProcessAnonymousRequestResponse>(
          Proto::Error_Type::Error_Type_Implementation,
          stream.str(),
          id_request_grpc);
        return response;
      }
      catch (...)
      {
        Stream::Error stream;
        stream << FNS
               << ": Unknown error";
        auto response = create_grpc_error_response<Proto::ProcessAnonymousRequestResponse>(
          Proto::Error_Type::Error_Type_Implementation,
          stream.str(),
          id_request_grpc);
        return response;
      }
    }

    void
    CampaignManagerImpl::get_file(
      const char* file_name,
      CORBACommons::OctSeq_out file)
      /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException)*/
    {
      static const char* FUN = "CampaignManagerImpl::get_file()";

      try
      {
        std::string full_file_name(campaign_manager_config_.Creative().ContentCache().root());
        full_file_name += file_name;
        file << *template_files_->get(full_file_name);
      }
      catch(const eh::Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << e.what();
        CORBACommons::throw_desc<
          CampaignSvcs::CampaignManager::ImplementationException>(
            ostr.str());
      }
    }

    CampaignManagerImpl::GetFileResponsePtr
    CampaignManagerImpl::get_file(GetFileRequestPtr&& request)
    {
      const auto id_request_grpc = request->id_request_grpc();
      try
      {
        const auto& file_name = request->file_name();

        auto response = create_grpc_response<Proto::GetFileResponse>(
          id_request_grpc);
        auto* info_response = response->mutable_info();

        std::string full_file_name(
          campaign_manager_config_.Creative().ContentCache().root());
        full_file_name += file_name;
        info_response->set_file(
          *template_files_->get(full_file_name));

        return response;
      }
      catch (const eh::Exception& exc)
      {
        Stream::Error stream;
        stream << FNS
               << ": "
               << exc.what();
        auto response = create_grpc_error_response<Proto::GetFileResponse>(
          Proto::Error_Type::Error_Type_Implementation,
          stream.str(),
          id_request_grpc);
        return response;
      }
      catch (...)
      {
        Stream::Error stream;
        stream << FNS
               << ": Unknown error";
        auto response = create_grpc_error_response<Proto::GetFileResponse>(
          Proto::Error_Type::Error_Type_Implementation,
          stream.str(),
          id_request_grpc);
        return response;
      }
    }

    void
    CampaignManagerImpl::instantiate_ad(
      const AdServer::CampaignSvcs::CampaignManager::
        InstantiateAdInfo& instantiate_ad_info,
      AdServer::CampaignSvcs::CampaignManager::
        InstantiateAdResult_out instantiate_ad_result)
      /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException,
        AdServer::CampaignSvcs::CampaignManager::NotReady)*/
    {
      static const char* FUN = "CampaignManagerImpl::instantiate_ad()";

      try
      {
        ConstCampaignConfig_var campaign_config = configuration();
        const Colocation* colocation = 0;
        const Tag* tag = 0;
        const Tag::Size* tag_size = 0;

        if(instantiate_ad_info.common_info.colo_id)
        {
          CampaignConfig::ColocationMap::const_iterator colo_it =
            campaign_config->colocations.find(
              instantiate_ad_info.common_info.colo_id);

          if(colo_it != campaign_config->colocations.end())
          {
            colocation = colo_it->second;
          }
        }

        if(!colocation && (
             instantiate_ad_info.common_info.colo_id !=
               campaign_manager_config_.colocation_id()))
        {
          CampaignConfig::ColocationMap::const_iterator colo_it =
            campaign_config->colocations.find(
              campaign_manager_config_.colocation_id());

          if(colo_it != campaign_config->colocations.end())
          {
            colocation = colo_it->second;
          }
        }

        if(!colocation)
        {
          Stream::Error ostr;
          ostr << FUN << ": can't resolve colocation identifier defined in config";

          logger_->log(ostr.str(),
            Logging::Logger::EMERGENCY,
            Aspect::CAMPAIGN_MANAGER,
            "ADS-IMPL-174");
        }

        const Creative* creative = 0;

        if(instantiate_ad_info.creative_id)
        {
          // determine size by creative
          CreativeMap::const_iterator cr_it = campaign_config->creatives.find(
            instantiate_ad_info.creative_id);
          if(cr_it != campaign_config->creatives.end())
          {
            creative = cr_it->second.in();
          }
        }

        if(instantiate_ad_info.tag_id)
        {
          TagMap::const_iterator tag_it =
            campaign_config->tags.find(instantiate_ad_info.tag_id);

          if(tag_it != campaign_config->tags.end())
          {
            tag = tag_it->second.in();
          }
        }
        else if(creative && instantiate_ad_info.publisher_account_id)
        {
          bool tag_size_found = false;

          for(Creative::SizeMap::const_iterator cs_it = creative->sizes.begin();
              cs_it != creative->sizes.end(); ++cs_it)
          {
            CampaignConfig::IdTagMap::const_iterator acc_tag_it =
              campaign_config->account_tags.find(IdTagKey(
                instantiate_ad_info.publisher_account_id,
                cs_it->second.size->protocol_name.c_str()));

            if(acc_tag_it != campaign_config->account_tags.end())
            {
              for(auto tag_it = acc_tag_it->second.begin();
                tag_it != acc_tag_it->second.end(); ++tag_it)
              {
                tag_size = match_creative_by_size_(*tag_it, creative);
                if(tag_size)
                {
                  tag = tag_it->in();
                  tag_size_found = true;
                  break;
                }
              }

              if(tag_size_found)
              {
                break;
              }
            }
          }
        }

        if(!tag)
        {
          Stream::Error ostr;
          ostr << FUN << ": tag not found";
          CORBACommons::throw_desc<
            CampaignSvcs::CampaignManager::ImplementationException>(
              ostr.str());
        }

        if(!tag_size)
        {
          Tag::SizeMap::const_iterator tag_size_it = tag->sizes.find(
            instantiate_ad_info.tag_size_id);

          if(tag_size_it == tag->sizes.end())
          {
            tag_size_it = tag->sizes.begin();
          }

          if(tag_size_it != tag->sizes.end())
          {
            tag_size = tag_size_it->second;
          }
        }

        if(!tag_size)
        {
          Stream::Error ostr;
          ostr << FUN << ": tag size not found";
          CORBACommons::throw_desc<
            CampaignSvcs::CampaignManager::ImplementationException>(
              ostr.str());
        }

        instantiate_ad_result = new AdServer::CampaignSvcs::
          CampaignManager::InstantiateAdResult();

        AdSlotContext ad_slot_context;
        ad_slot_context.test_request =
          instantiate_ad_info.common_info.test_request;
        if(instantiate_ad_info.pub_imp_revenue_defined)
        {
          ad_slot_context.pub_imp_revenue =
            CorbaAlgs::unpack_decimal<RevenueDecimal>(
              instantiate_ad_info.pub_imp_revenue);
        }

        CreativeInstantiateRuleMap::iterator rule_it =
          creative_instantiate_.creative_rules.find(
            instantiate_ad_info.common_info.creative_instantiate_type.in());

        if(rule_it == creative_instantiate_.creative_rules.end())
        {
          Stream::Error err;
          err << FUN <<
            ": cannot find creative instantiate rule with name = '" <<
            instantiate_ad_info.common_info.creative_instantiate_type.in() << "'.";

          CORBACommons::throw_desc<CampaignSvcs::CampaignManager::ImplementationException>(err.str());
        }

        fill_passback_url_from_tag(
          ad_slot_context.passback_url,
          rule_it->second,
          tag);
        
        AdSelectionResult ad_selection_result;
        ad_selection_result.tag = tag;
        ad_selection_result.tag_size = tag_size;

        if(instantiate_ad_info.creatives.length() > 0)
        {
          for(CORBA::ULong creative_i = 0;
              creative_i < instantiate_ad_info.creatives.length();
              ++creative_i)
          {
            CampaignSelectionData campaign_selection_data;

            CcidMap::const_iterator creative_it =
              campaign_config->campaign_creatives.find(
                instantiate_ad_info.creatives[creative_i].ccid);

            if(creative_it != campaign_config->campaign_creatives.end() &&
               creative_it->second->campaign->advertiser)
            {
              campaign_selection_data.request_id = CorbaAlgs::unpack_request_id(
                instantiate_ad_info.creatives[creative_i].request_id);
              if(campaign_selection_data.request_id.is_null())
              {
                campaign_selection_data.request_id =
                  AdServer::Commons::RequestId::create_random_based();
              }

              campaign_selection_data.campaign =
                creative_it->second->campaign;
              campaign_selection_data.creative =
                creative_it->second;

              if(instantiate_ad_info.creatives[creative_i].ccg_keyword_id)
              {
                CCGKeywordPostClickInfoMap::const_iterator ccg_keyword_it =
                  campaign_config->ccg_keyword_click_info_map.find(
                    instantiate_ad_info.creatives[creative_i].ccg_keyword_id);

                if(ccg_keyword_it != campaign_config->ccg_keyword_click_info_map.end())
                {
                  campaign_selection_data.campaign_keyword =
                    new CampaignKeyword();
                  campaign_selection_data.campaign_keyword->channel_id = 0;
                  campaign_selection_data.campaign_keyword->max_cpc =
                    RevenueDecimal::ZERO;
                  campaign_selection_data.campaign_keyword->ctr =
                    RevenueDecimal::ZERO;
                  campaign_selection_data.campaign_keyword->campaign =
                    creative_it->second->campaign;
                  campaign_selection_data.campaign_keyword->ecpm =
                    RevenueDecimal::ZERO;

                  campaign_selection_data.campaign_keyword->ccg_keyword_id =
                    ccg_keyword_it->first;
                  campaign_selection_data.campaign_keyword->click_url =
                    ccg_keyword_it->second.click_url;
                  campaign_selection_data.campaign_keyword->original_keyword =
                    ccg_keyword_it->second.original_keyword;
                }
                else
                {
                  Stream::Error ostr;
                  ostr << FUN << ": campaign keyword not found";
                  CORBACommons::throw_desc<
                    CampaignSvcs::CampaignManager::ImplementationException>(
                      ostr.str());
                }
              }
            }
            else
            {
              Stream::Error ostr;
              ostr << FUN << ": creative not found";
              CORBACommons::throw_desc<
                CampaignSvcs::CampaignManager::ImplementationException>(
                  ostr.str());
            }

            // instantiate don't use count_impression
            campaign_selection_data.count_impression = false;
            // instantiate don't use track_impr
            campaign_selection_data.track_impr = false;
            campaign_selection_data.selection_done = false;
            campaign_selection_data.ctr = CorbaAlgs::unpack_decimal<RevenueDecimal>(
              instantiate_ad_info.creatives[creative_i].ctr);

            ad_selection_result.selected_campaigns.push_back(campaign_selection_data);
          }
        }
        else if(creative)
        {
          CampaignSelectionData campaign_selection_data;
          campaign_selection_data.request_id = AdServer::Commons::UserId();
          campaign_selection_data.campaign = creative->campaign;
          campaign_selection_data.creative = creative;
          campaign_selection_data.count_impression = false;
          campaign_selection_data.track_impr = false;
          campaign_selection_data.selection_done = false;
          ad_selection_result.selected_campaigns.push_back(campaign_selection_data);
        }
        else
        {
          Stream::Error ostr;
          ostr << FUN << ": creative not found";
          CORBACommons::throw_desc<
            CampaignSvcs::CampaignManager::ImplementationException>(
              ostr.str());
        }

        RequestResultParams request_result_params;
        request_result_params.request_id = CorbaAlgs::unpack_request_id(
          instantiate_ad_info.common_info.request_id);

        CreativeParamsList creative_params_list;
        InstantiateParams inst_params(
          instantiate_ad_info.user_id_hash_mod.defined ?
          std::optional<unsigned long>(
            instantiate_ad_info.user_id_hash_mod.value) :
          std::optional<unsigned long>());
        inst_params.open_price = instantiate_ad_info.open_price;
        inst_params.openx_price = instantiate_ad_info.openx_price;
        inst_params.liverail_price = instantiate_ad_info.liverail_price;
        inst_params.google_price = instantiate_ad_info.google_price;
        inst_params.ext_tag_id =
          String::SubString(instantiate_ad_info.ext_tag_id);
        inst_params.video_width = instantiate_ad_info.video_width;
        inst_params.video_height = instantiate_ad_info.video_height;
        inst_params.publisher_site_id = instantiate_ad_info.publisher_site_id;
        inst_params.publisher_account_id = instantiate_ad_info.publisher_account_id;
        // don't init custom macroses (preclick) by source rule
        inst_params.init_source_macroses = false;

        CorbaAlgs::convert_sequence(
          instantiate_ad_info.pubpixel_accounts,
          inst_params.pubpixel_accounts);

        instantiate_creative_(
          instantiate_ad_info.common_info,
          campaign_config,
          colocation,
          inst_params,
          instantiate_ad_info.format,
          ad_selection_result,
          request_result_params,
          creative_params_list,
          instantiate_ad_result->creative_body,
          ad_slot_context,
          0 // exclude_pubpixel_accounts
          );

        instantiate_ad_result->request_ids.length(0);
        
        instantiate_ad_result->mime_format <<
          request_result_params.mime_format;

        if (instantiate_ad_info.emulate_click)
        {
          instantiate_ad_result->request_ids.length(
            ad_selection_result.selected_campaigns.size());

          CORBA::ULong i = 0;

          for(CampaignSelectionDataList::iterator it =
                ad_selection_result.selected_campaigns.begin();
              it != ad_selection_result.selected_campaigns.end();
              ++it, ++i)
          {
            assert(it->creative && it->campaign);

            instantiate_ad_result->request_ids[i] =
              CorbaAlgs::pack_request_id(it->request_id);
          }
        }

        if(instantiate_ad_info.consider_request)
        {
          // TODO: use empty user id
          assert(instantiate_ad_info.context_info.length() == 1);

          ChannelIdList geo_channels;

          {
            ChannelIdSet coord_channels;
            match_geo_channels_(
              instantiate_ad_info.common_info.location,
              instantiate_ad_info.common_info.coord_location,
              geo_channels,
              coord_channels);

            std::copy(coord_channels.begin(),
              coord_channels.end(),
              std::back_inserter(geo_channels));
          }

          // emulate campaign selection
          CampaignManagerLogger::RequestInfo request_info;
          CampaignManagerLogger::AdRequestSelectionInfo
            ad_request_selection_info;

          CampaignManagerLogAdapter::fill_request_info(
            request_info,
            campaign_config,
            colocation,
            instantiate_ad_info.common_info,
            instantiate_ad_info.context_info[0],
            0, // request_params
            0, // ad_request_debug_info
            ad_slot_context
            );

          if(instantiate_ad_info.context_info[0].enabled_notice)
          {
            request_info.request_user_id = AdServer::Commons::UserId();
            request_info.request_user_status = US_UNDEFINED;
          }
         
          std::copy(geo_channels.begin(),
            geo_channels.end(),
            std::back_inserter(request_info.geo_channels));
          std::copy(geo_channels.begin(),
            geo_channels.end(),
            std::inserter(request_info.history_channels, request_info.history_channels.end()));

          CampaignSvcs::CampaignManager::AdSlotInfo ad_slot_info;
          ad_slot_info.ad_slot_id = 0;
          ad_slot_info.format = instantiate_ad_info.format;
          ad_slot_info.tag_id = instantiate_ad_info.tag_id;
          ad_slot_info.ext_tag_id = instantiate_ad_info.ext_tag_id;
          ad_slot_info.min_ecpm = CorbaAlgs::pack_decimal<RevenueDecimal>(
            RevenueDecimal::ZERO);
          ad_slot_info.passback = false;
          ad_slot_info.up_expand_space = -1;
          ad_slot_info.right_expand_space = -1;
          ad_slot_info.left_expand_space = -1;
          ad_slot_info.down_expand_space = -1;
          ad_slot_info.tag_visibility = -1;
          ad_slot_info.video_min_duration = 0;
          ad_slot_info.video_max_duration = -1;
          ad_slot_info.video_skippable_max_duration = -1;
          ad_slot_info.video_allow_skippable = true;
          ad_slot_info.video_allow_unskippable = true;

          CampaignManagerLogAdapter::fill_ad_request_selection_info(
            ad_request_selection_info,
            campaign_config,
            colocation,
            instantiate_ad_info.common_info,
            instantiate_ad_info.context_info[0],
            0, // request_params
            ad_slot_info,
            tag,
            ad_selection_result,
            ad_slot_context,
            AdSlotMinCpm(),
            tag->sizes, // log all sizes, no blacklisting on inst
            instantiate_ad_info.emulate_click);
         
          campaign_manager_logger_->process_ad_request(
            request_info, ad_request_selection_info);
        }

        if(instantiate_ad_info.consider_request && instantiate_ad_info.emulate_click)
        {
          ConfirmCreativeAmountArray confirm_creatives;

          const Generics::Time time = CorbaAlgs::unpack_time(
            instantiate_ad_info.common_info.time);

          for(auto it = ad_selection_result.selected_campaigns.begin();
            it != ad_selection_result.selected_campaigns.end(); ++it)
          {
            confirm_creatives.push_back(
              ConfirmCreativeAmount(it->creative->ccid, it->ctr));
          }

          // confirm imps
          confirm_amounts_(
            campaign_config,
            time,
            confirm_creatives,
            CR_CPM);

          // confirm clicks
          confirm_amounts_(
            campaign_config,
            time,
            confirm_creatives,
            CR_CPC);
        }
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << ex.what();

        logger_->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          Aspect::CAMPAIGN_MANAGER,
          "ADS-IMPL-174");
        CORBACommons::throw_desc<
          CampaignSvcs::CampaignManager::ImplementationException>(
            ostr.str());
      }
    }

    CampaignManagerImpl::InstantiateAdResponsePtr
    CampaignManagerImpl::instantiate_ad(InstantiateAdRequestPtr&& request)
    {
      static const char* FUN = "CampaignManagerImpl::instantiate_ad()";
      const auto id_request_grpc = request->id_request_grpc();
      try
      {
        const auto& instantiate_ad_info = request->instantiate_ad_info();

        auto response = create_grpc_response<Proto::InstantiateAdResponse>(
          id_request_grpc);
        auto* info_response = response->mutable_info();

        ConstCampaignConfig_var campaign_config = configuration(false);
        if (!campaign_config)
        {
          throw NotReady("Campaign configuration isn't loaded");
        }

        const Colocation* colocation = nullptr;
        const Tag* tag = nullptr;
        const Tag::Size* tag_size = nullptr;

        if(instantiate_ad_info.common_info().colo_id())
        {
          CampaignConfig::ColocationMap::const_iterator colo_it =
            campaign_config->colocations.find(
              instantiate_ad_info.common_info().colo_id());

          if(colo_it != campaign_config->colocations.end())
          {
            colocation = colo_it->second;
          }
        }

        if(!colocation && (
             instantiate_ad_info.common_info().colo_id() !=
               campaign_manager_config_.colocation_id()))
        {
          const auto colo_it =
            campaign_config->colocations.find(
              campaign_manager_config_.colocation_id());

          if(colo_it != campaign_config->colocations.end())
          {
            colocation = colo_it->second;
          }
        }

        if(!colocation)
        {
          Stream::Error ostr;
          ostr << FUN << ": can't resolve colocation identifier defined in config";

          logger_->log(ostr.str(),
            Logging::Logger::EMERGENCY,
            Aspect::CAMPAIGN_MANAGER,
            "ADS-IMPL-174");
        }

        const Creative* creative = 0;

        if(instantiate_ad_info.creative_id())
        {
          // determine size by creative
          CreativeMap::const_iterator cr_it = campaign_config->creatives.find(
            instantiate_ad_info.creative_id());
          if(cr_it != campaign_config->creatives.end())
          {
            creative = cr_it->second.in();
          }
        }

        if(instantiate_ad_info.tag_id())
        {
          TagMap::const_iterator tag_it =
            campaign_config->tags.find(instantiate_ad_info.tag_id());

          if(tag_it != campaign_config->tags.end())
          {
            tag = tag_it->second.in();
          }
        }
        else if(creative && instantiate_ad_info.publisher_account_id())
        {
          bool tag_size_found = false;

          for(Creative::SizeMap::const_iterator cs_it = creative->sizes.begin();
              cs_it != creative->sizes.end(); ++cs_it)
          {
            CampaignConfig::IdTagMap::const_iterator acc_tag_it =
              campaign_config->account_tags.find(IdTagKey(
                instantiate_ad_info.publisher_account_id(),
                cs_it->second.size->protocol_name.c_str()));

            if(acc_tag_it != campaign_config->account_tags.end())
            {
              for(auto tag_it = acc_tag_it->second.begin();
                tag_it != acc_tag_it->second.end(); ++tag_it)
              {
                tag_size = match_creative_by_size_(*tag_it, creative);
                if(tag_size)
                {
                  tag = tag_it->in();
                  tag_size_found = true;
                  break;
                }
              }

              if(tag_size_found)
              {
                break;
              }
            }
          }
        }

        if(!tag)
        {
          Stream::Error ostr;
          ostr << FUN << ": tag not found";
          throw ImplementationException(ostr.str());
        }

        if(!tag_size)
        {
          Tag::SizeMap::const_iterator tag_size_it = tag->sizes.find(
            instantiate_ad_info.tag_size_id());

          if(tag_size_it == tag->sizes.end())
          {
            tag_size_it = tag->sizes.begin();
          }

          if(tag_size_it != tag->sizes.end())
          {
            tag_size = tag_size_it->second;
          }
        }

        if(!tag_size)
        {
          Stream::Error ostr;
          ostr << FUN << ": tag size not found";
          throw ImplementationException(ostr.str());
        }

        AdSlotContext ad_slot_context;
        ad_slot_context.test_request =
          instantiate_ad_info.common_info().test_request();
        if(instantiate_ad_info.pub_imp_revenue_defined())
        {
          ad_slot_context.pub_imp_revenue =
            GrpcAlgs::unpack_decimal<RevenueDecimal>(
              instantiate_ad_info.pub_imp_revenue());
        }

        CreativeInstantiateRuleMap::iterator rule_it =
          creative_instantiate_.creative_rules.find(
            instantiate_ad_info.common_info().creative_instantiate_type());

        if(rule_it == creative_instantiate_.creative_rules.end())
        {
          Stream::Error err;
          err << FUN <<
            ": cannot find creative instantiate rule with name = '" <<
            instantiate_ad_info.common_info().creative_instantiate_type() << "'.";
          throw ImplementationException(err.str());
        }

        fill_passback_url_from_tag(
          ad_slot_context.passback_url,
          rule_it->second,
          tag);

        AdSelectionResult ad_selection_result;
        ad_selection_result.tag = tag;
        ad_selection_result.tag_size = tag_size;

        if(!instantiate_ad_info.creatives().empty())
        {
          for(const auto& instantiate_creative : instantiate_ad_info.creatives())
          {
            CampaignSelectionData campaign_selection_data;

            CcidMap::const_iterator creative_it =
              campaign_config->campaign_creatives.find(instantiate_creative.ccid());

            if(creative_it != campaign_config->campaign_creatives.end() &&
               creative_it->second->campaign->advertiser)
            {
              campaign_selection_data.request_id = GrpcAlgs::unpack_request_id(
                instantiate_creative.request_id());
              if(campaign_selection_data.request_id.is_null())
              {
                campaign_selection_data.request_id =
                  AdServer::Commons::RequestId::create_random_based();
              }

              campaign_selection_data.campaign =
                creative_it->second->campaign;
              campaign_selection_data.creative =
                creative_it->second;

              if(instantiate_creative.ccg_keyword_id())
              {
                CCGKeywordPostClickInfoMap::const_iterator ccg_keyword_it =
                  campaign_config->ccg_keyword_click_info_map.find(
                    instantiate_creative.ccg_keyword_id());

                if(ccg_keyword_it != campaign_config->ccg_keyword_click_info_map.end())
                {
                  campaign_selection_data.campaign_keyword =
                    new CampaignKeyword();
                  campaign_selection_data.campaign_keyword->channel_id = 0;
                  campaign_selection_data.campaign_keyword->max_cpc =
                    RevenueDecimal::ZERO;
                  campaign_selection_data.campaign_keyword->ctr =
                    RevenueDecimal::ZERO;
                  campaign_selection_data.campaign_keyword->campaign =
                    creative_it->second->campaign;
                  campaign_selection_data.campaign_keyword->ecpm =
                    RevenueDecimal::ZERO;

                  campaign_selection_data.campaign_keyword->ccg_keyword_id =
                    ccg_keyword_it->first;
                  campaign_selection_data.campaign_keyword->click_url =
                    ccg_keyword_it->second.click_url;
                  campaign_selection_data.campaign_keyword->original_keyword =
                    ccg_keyword_it->second.original_keyword;
                }
                else
                {
                  Stream::Error ostr;
                  ostr << FUN << ": campaign keyword not found";
                  throw ImplementationException(ostr.str());
                }
              }
            }
            else
            {
              Stream::Error ostr;
              ostr << FUN << ": creative not found";
              throw ImplementationException(ostr.str());
            }

            // instantiate don't use count_impression
            campaign_selection_data.count_impression = false;
            // instantiate don't use track_impr
            campaign_selection_data.track_impr = false;
            campaign_selection_data.selection_done = false;
            campaign_selection_data.ctr = GrpcAlgs::unpack_decimal<RevenueDecimal>(
              instantiate_creative.ctr());

            ad_selection_result.selected_campaigns.push_back(campaign_selection_data);
          }
        }
        else if(creative)
        {
          CampaignSelectionData campaign_selection_data;
          campaign_selection_data.request_id = AdServer::Commons::UserId();
          campaign_selection_data.campaign = creative->campaign;
          campaign_selection_data.creative = creative;
          campaign_selection_data.count_impression = false;
          campaign_selection_data.track_impr = false;
          campaign_selection_data.selection_done = false;
          ad_selection_result.selected_campaigns.push_back(campaign_selection_data);
        }
        else
        {
          Stream::Error ostr;
          ostr << FUN << ": creative not found";
          throw ImplementationException(ostr.str());
        }

        RequestResultParams request_result_params;
        request_result_params.request_id = GrpcAlgs::unpack_request_id(
          instantiate_ad_info.common_info().request_id());

        CreativeParamsList creative_params_list;
        InstantiateParams inst_params(
          instantiate_ad_info.user_id_hash_mod().defined() ?
          std::optional<unsigned long>(
            instantiate_ad_info.user_id_hash_mod().value()) :
          std::optional<unsigned long>());
        inst_params.open_price = instantiate_ad_info.open_price();
        inst_params.openx_price = instantiate_ad_info.openx_price();
        inst_params.liverail_price = instantiate_ad_info.liverail_price();
        inst_params.google_price = instantiate_ad_info.google_price();
        inst_params.ext_tag_id =
          String::SubString(instantiate_ad_info.ext_tag_id());
        inst_params.video_width = instantiate_ad_info.video_width();
        inst_params.video_height = instantiate_ad_info.video_height();
        inst_params.publisher_site_id = instantiate_ad_info.publisher_site_id();
        inst_params.publisher_account_id = instantiate_ad_info.publisher_account_id();
        // don't init custom macroses (preclick) by source rule
        inst_params.init_source_macroses = false;

        inst_params.pubpixel_accounts.insert(
          std::end(inst_params.pubpixel_accounts),
          std::begin(instantiate_ad_info.pubpixel_accounts()),
          std::end(instantiate_ad_info.pubpixel_accounts()));

        instantiate_creative_(
          instantiate_ad_info.common_info(),
          campaign_config,
          colocation,
          inst_params,
          instantiate_ad_info.format().c_str(),
          ad_selection_result,
          request_result_params,
          creative_params_list,
          *info_response->mutable_creative_body(),
          ad_slot_context,
          0 // exclude_pubpixel_accounts
          );

        auto* request_ids_response = info_response->mutable_request_ids();
        info_response->set_mime_format(request_result_params.mime_format);

        if (instantiate_ad_info.emulate_click())
        {
          request_ids_response->Reserve(
            ad_selection_result.selected_campaigns.size());

          for (const auto& selected_campaign : ad_selection_result.selected_campaigns)
          {
            assert(selected_campaign.creative && selected_campaign.campaign);
            request_ids_response->Add(
              GrpcAlgs::pack_request_id(selected_campaign.request_id));
          }
        }

        if(instantiate_ad_info.consider_request())
        {
          // TODO: use empty user id
          assert(instantiate_ad_info.context_info().size() == 1);

          ChannelIdList geo_channels;

          {
            ChannelIdSet coord_channels;
            match_geo_channels_(
              instantiate_ad_info.common_info().location(),
              instantiate_ad_info.common_info().coord_location(),
              geo_channels,
              coord_channels);

            std::copy(coord_channels.begin(),
              coord_channels.end(),
              std::back_inserter(geo_channels));
          }

          // emulate campaign selection
          CampaignManagerLogger::RequestInfo request_info;
          CampaignManagerLogger::AdRequestSelectionInfo
            ad_request_selection_info;

          CampaignManagerLogAdapter::fill_request_info(
            request_info,
            campaign_config,
            colocation,
            instantiate_ad_info.common_info(),
            instantiate_ad_info.context_info()[0],
            nullptr, // request_params
            nullptr, // ad_request_debug_info
            ad_slot_context);

          if(instantiate_ad_info.context_info()[0].enabled_notice())
          {
            request_info.request_user_id = AdServer::Commons::UserId();
            request_info.request_user_status = US_UNDEFINED;
          }

          std::copy(geo_channels.begin(),
            geo_channels.end(),
            std::back_inserter(request_info.geo_channels));
          std::copy(geo_channels.begin(),
            geo_channels.end(),
            std::inserter(request_info.history_channels, request_info.history_channels.end()));

          Proto::AdSlotInfo ad_slot_info;
          ad_slot_info.set_ad_slot_id(0);
          ad_slot_info.set_format(instantiate_ad_info.format());
          ad_slot_info.set_tag_id(instantiate_ad_info.tag_id());
          ad_slot_info.set_ext_tag_id(instantiate_ad_info.ext_tag_id());
          ad_slot_info.set_min_ecpm(GrpcAlgs::pack_decimal<RevenueDecimal>(
            RevenueDecimal::ZERO));
          ad_slot_info.set_passback(false);
          ad_slot_info.set_up_expand_space(-1);
          ad_slot_info.set_right_expand_space(-1);
          ad_slot_info.set_left_expand_space(-1);
          ad_slot_info.set_down_expand_space(-1);
          ad_slot_info.set_tag_visibility(-1);
          ad_slot_info.set_video_min_duration(0);
          ad_slot_info.set_video_max_duration(-1);
          ad_slot_info.set_video_skippable_max_duration(-1);
          ad_slot_info.set_video_allow_skippable(true);
          ad_slot_info.set_video_allow_unskippable(true);

          CampaignManagerLogAdapter::fill_ad_request_selection_info(
            ad_request_selection_info,
            campaign_config,
            colocation,
            instantiate_ad_info.common_info(),
            instantiate_ad_info.context_info()[0],
            nullptr, // request_params
            ad_slot_info,
            tag,
            ad_selection_result,
            ad_slot_context,
            AdSlotMinCpm(),
            tag->sizes, // log all sizes, no blacklisting on inst
            instantiate_ad_info.emulate_click());

          campaign_manager_logger_->process_ad_request(
            request_info, ad_request_selection_info);
        }

        if(instantiate_ad_info.consider_request() && instantiate_ad_info.emulate_click())
        {
          ConfirmCreativeAmountArray confirm_creatives;

          const Generics::Time time = GrpcAlgs::unpack_time(
            instantiate_ad_info.common_info().time());

          for(auto it = ad_selection_result.selected_campaigns.begin();
            it != ad_selection_result.selected_campaigns.end(); ++it)
          {
            confirm_creatives.push_back(
              ConfirmCreativeAmount(it->creative->ccid, it->ctr));
          }

          // confirm imps
          confirm_amounts_(
            campaign_config,
            time,
            confirm_creatives,
            CR_CPM);

          // confirm clicks
          confirm_amounts_(
            campaign_config,
            time,
            confirm_creatives,
            CR_CPC);
        }

        return response;
      }
      catch (const NotReady& exc)
      {
        Stream::Error stream;
        stream << FNS
               << "Caught CampaignManager::NotReady: "
               << exc.what();
        auto response = create_grpc_error_response<Proto::InstantiateAdResponse>(
          Proto::Error_Type::Error_Type_NotReady,
          stream.str(),
          id_request_grpc);
        return response;
      }
      catch (const eh::Exception& exc)
      {
        Stream::Error stream;
        stream << FNS
               << ": "
               << exc.what();
        auto response = create_grpc_error_response<Proto::InstantiateAdResponse>(
          Proto::Error_Type::Error_Type_Implementation,
          stream.str(),
          id_request_grpc);
        return response;
      }
      catch (...)
      {
        Stream::Error stream;
        stream << FNS
               << ": Unknown error";
        auto response = create_grpc_error_response<Proto::InstantiateAdResponse>(
          Proto::Error_Type::Error_Type_Implementation,
          stream.str(),
          id_request_grpc);
        return response;
      }
    }

    const Tag*
    CampaignManagerImpl::resolve_tag(
      std::string* tag_size,
      unsigned long* selected_publisher_account_id,
      const CampaignSvcs::CampaignManager::RequestParams& request_params,
      const CampaignConfig& campaign_config,
      const CampaignSvcs::CampaignManager::AdSlotInfo& ad_slot)
      const noexcept
    {
      if(request_params.common_info.request_type == AR_NORMAL)
      {
        TagMap::const_iterator tag_it =
          campaign_config.tags.find(ad_slot.tag_id);

        return (tag_it != campaign_config.tags.end()) ?
          tag_it->second.in() :
          0;
      }

      if(request_params.publisher_site_id)
      {
        for(CORBA::ULong i = 0; i < ad_slot.sizes.length(); ++i)
        {
          const CampaignConfig::IdTagMap::const_iterator site_tag_it =
            campaign_config.site_tags.find(IdTagKey(
              request_params.publisher_site_id,
              ad_slot.sizes[i].in()));

          if(site_tag_it != campaign_config.site_tags.end())
          {
            if(tag_size)
            {
              *tag_size = ad_slot.sizes[i].in();
            }

            return site_tag_it->second[
              request_params.common_info.random % site_tag_it->second.size()];
          }
        }
      }

      if(request_params.publisher_account_ids.length() > 0)
      {
        std::vector<unsigned long> allowed_publisher_account_ids;
        allowed_publisher_account_ids.reserve(request_params.publisher_account_ids.length());

        if(ad_slot.currency_codes.length() > 0)
        {
          for(CORBA::ULong i = 0; i < request_params.publisher_account_ids.length(); ++i)
          {
            auto account_it = campaign_config.accounts.find(request_params.publisher_account_ids[i]);
            if(account_it != campaign_config.accounts.end())
            {
              for(CORBA::ULong currency_i = 0; currency_i < ad_slot.currency_codes.length(); ++currency_i)
              {
                if(account_it->second->currency->currency_code == ad_slot.currency_codes[currency_i].in())
                {
                  allowed_publisher_account_ids.push_back(request_params.publisher_account_ids[i]);
                }
              }
            }
          }
        }
        else
        {
          CorbaAlgs::convert_sequence(
            request_params.publisher_account_ids,
            allowed_publisher_account_ids);          
        }

        for(auto acc_it = allowed_publisher_account_ids.begin();
          acc_it != allowed_publisher_account_ids.end(); ++acc_it)
        {
          const unsigned long res_publisher_account_id = *acc_it;

          for(CORBA::ULong i = 0; i < ad_slot.sizes.length(); ++i)
          {
            const CampaignConfig::IdTagMap::const_iterator acc_tag_it =
              campaign_config.account_tags.find(IdTagKey(
                res_publisher_account_id,
                ad_slot.sizes[i].in()));

            if(acc_tag_it != campaign_config.account_tags.end())
            {
              if(tag_size)
              {
                *tag_size = ad_slot.sizes[i].in();
              }

              if(selected_publisher_account_id)
              {
                *selected_publisher_account_id = res_publisher_account_id;
              }

              return acc_tag_it->second[
                request_params.common_info.random % acc_tag_it->second.size()];
            }
          }
        }

        if(selected_publisher_account_id && !allowed_publisher_account_ids.empty())
        {
          *selected_publisher_account_id = *allowed_publisher_account_ids.begin();
        }
      }

      return 0;
    }

    const Tag*
    CampaignManagerImpl::resolve_tag(
      std::string* tag_size,
      unsigned long* selected_publisher_account_id,
      const Proto::RequestParams& request_params,
      const CampaignConfig& campaign_config,
      const Proto::AdSlotInfo& ad_slot) const noexcept
    {
      if(request_params.common_info().request_type() == AR_NORMAL)
      {
        TagMap::const_iterator tag_it =
          campaign_config.tags.find(ad_slot.tag_id());

        return (tag_it != campaign_config.tags.end()) ?
          tag_it->second.in() :
          nullptr;
      }

      if(request_params.publisher_site_id())
      {
        for(const auto& size_ad_slot : ad_slot.sizes())
        {
          const CampaignConfig::IdTagMap::const_iterator site_tag_it =
            campaign_config.site_tags.find(IdTagKey(
              request_params.publisher_site_id(),
              size_ad_slot.c_str()));

          if(site_tag_it != campaign_config.site_tags.end())
          {
            if(tag_size)
            {
              *tag_size = size_ad_slot;
            }

            return site_tag_it->second[
              request_params.common_info().random() % site_tag_it->second.size()];
          }
        }
      }

      if(request_params.publisher_account_ids().size() > 0)
      {
        std::vector<unsigned long> allowed_publisher_account_ids;
        allowed_publisher_account_ids.reserve(request_params.publisher_account_ids().size());

        if(ad_slot.currency_codes().size() > 0)
        {
          for(const auto& publisher_account_id : request_params.publisher_account_ids())
          {
            auto account_it = campaign_config.accounts.find(publisher_account_id);
            if(account_it != campaign_config.accounts.end())
            {
              for(const auto& currency_code : ad_slot.currency_codes())
              {
                if(account_it->second->currency->currency_code == currency_code)
                {
                  allowed_publisher_account_ids.push_back(publisher_account_id);
                }
              }
            }
          }
        }
        else
        {
          allowed_publisher_account_ids.insert(
            std::end(allowed_publisher_account_ids),
            std::begin(request_params.publisher_account_ids()),
            std::end(request_params.publisher_account_ids()));
        }

        for(auto& res_publisher_account_id : allowed_publisher_account_ids)
        {
          for(const auto& size_ad_slot : ad_slot.sizes())
          {
            const auto acc_tag_it = campaign_config.account_tags.find(IdTagKey(
              res_publisher_account_id,
              size_ad_slot.c_str()));

            if(acc_tag_it != campaign_config.account_tags.end())
            {
              if(tag_size)
              {
                *tag_size = size_ad_slot;
              }

              if(selected_publisher_account_id)
              {
                *selected_publisher_account_id = res_publisher_account_id;
              }

              return acc_tag_it->second[
                request_params.common_info().random() % acc_tag_it->second.size()];
            }
          }
        }

        if(selected_publisher_account_id && !allowed_publisher_account_ids.empty())
        {
          *selected_publisher_account_id = *allowed_publisher_account_ids.begin();
        }
      }

      return nullptr;
    }

    void
    CampaignManagerImpl::get_adslot_campaign_creative_(
      const CampaignConfig* campaign_config,
      AdServer::CampaignSvcs::CampaignManager::RequestCreativeResult&
        /*ad_request_result*/,
      AdServer::CampaignSvcs::CampaignManager::AdSlotResult& ad_slot_result,
      AdServer::CampaignSvcs::RevenueDecimal& adsspace_system_cpm,
      const AdServer::CampaignSvcs::CampaignManager::RequestParams&
        request_params,
      const Generics::Time& session_start,
      const Colocation* colocation,
      const AdServer::CampaignSvcs::CampaignManager::AdSlotInfo& ad_slot,
      const CreativeInstantiateRule& creative_instantiate_rule,
      AdServer::CampaignSvcs::CampaignManager::AdRequestDebugInfo* ad_request_debug_info,
      AdServer::CampaignSvcs::CampaignManager::AdSlotDebugInfo* ad_slot_debug_info,
      AdSlotContext& ad_slot_context,
      const ChannelIdHashSet& matched_channels,
      unsigned long& request_tag_id)
      noexcept
    {
      //static const char* FUN = "CampaignManagerImpl::get_adslot_campaign_creative_()";

      bool passback = ad_slot.passback;
      bool process_request = true;

      if(ad_slot_debug_info)
      {
        ad_slot_debug_info->cpm_threshold =
          CorbaAlgs::pack_decimal(RevenueDecimal::ZERO);
      }

      const Tag* tag = resolve_tag(
        &ad_slot_context.tag_size,
        &ad_slot_context.publisher_account_id,
        request_params,
        *campaign_config,
        ad_slot);

      if(tag)
      {
        {
          if (!request_tag_id)
          {
            request_tag_id = tag->tag_id;
          }
          ad_slot_result.pub_currency_code <<
            tag->site->account->currency->currency_code;
        }
      }

      if(request_params.common_info.passback_url[0])
      {
        ad_slot_context.passback_url =
          request_params.common_info.passback_url.in();
        creative_instantiate_rule.instantiate_relative_protocol_url(
          ad_slot_context.passback_url);
      }
      else
      {
        fill_passback_url_from_tag(
          ad_slot_context.passback_url,
          creative_instantiate_rule,
          tag);
      }

      if(ad_slot_debug_info)
      {
        ad_slot_debug_info->tag_id = 0;
        ad_slot_debug_info->site_id = 0;
        ad_slot_debug_info->site_rate_id = 0;
        ad_slot_debug_info->cpm_threshold =
          CorbaAlgs::pack_decimal(RevenueDecimal::ZERO);
      }

      if(tag)
      {
        ad_slot_context.test_request |= tag->is_test();

        if(process_request)
        {
          // Check & fill min_ecpm
          AdSlotMinCpm ad_slot_min_cpm;

          if (!fill_ad_slot_min_cpm_(
                ad_slot_min_cpm,
                campaign_config,
                tag,
                String::SubString(ad_slot.min_ecpm_currency_code),
                ad_slot.min_ecpm
                ))
          {
            passback = true;
          }
          else
          {
            adsspace_system_cpm = std::min(
              adsspace_system_cpm, ad_slot_min_cpm.min_pub_ecpm_system);
          }

          try
          {
            get_campaign_creative_(
              request_params,
              ad_slot,
              ad_slot_context,
              ad_slot_min_cpm,
              session_start,
              ad_slot_result,
              tag,
              colocation,
              passback,
              ad_request_debug_info,
              ad_slot_debug_info,
              matched_channels);
          }
          catch(const eh::Exception&)
          {
            ad_slot_result.selected_creatives.length(0);

            if(ad_slot_debug_info)
            {
              ad_slot_debug_info->selected_creatives.length(0);
            }
          }

          ad_slot_result.test_request = ad_slot_context.test_request;
        }

        if(ad_request_debug_info)
        {
          CorbaAlgs::copy_sequence(
            request_params.context_info.geo_channels,
            ad_request_debug_info->geo_channels);
        }
      }
      else
      {
        CampaignManagerLogger::RequestInfo request_info;

        CampaignManagerLogAdapter::fill_request_info(
          request_info,
          campaign_config,
          colocation,
          request_params.common_info,
          request_params.context_info,
          &request_params,
          ad_request_debug_info,
          ad_slot_context);

        campaign_manager_logger_->process_request(request_info);
      }

      passback |=
        ad_slot_result.selected_creatives.length() == 0;
      
      ad_slot_result.passback = passback;

      if (request_params.preview_ccid &&
          ad_slot_result.selected_creatives.length())
      {
        passback = false;
      }

      if(passback && request_params.required_passback)
      {
        if(!ad_slot_context.passback_url.empty() &&
           ::strcmp(request_params.common_info.passback_type, "redir") == 0)
        {
          std::ostringstream passback_imp_url_ostr;
          std::string mime_passback_url;

          String::StringManip::mime_url_encode(
            ad_slot_context.passback_url, mime_passback_url);

          passback_imp_url_ostr << creative_instantiate_rule.passback_pixel_url <<
            "?requestid=" << CorbaAlgs::unpack_request_id(
              request_params.common_info.request_id).to_string() <<
            "&random=" << request_params.common_info.random <<
            "&passback=" << mime_passback_url;

          AdServer::Commons::UserId user_id =
            CorbaAlgs::unpack_user_id(request_params.common_info.user_id);
          if(!user_id.is_null())
          {
            passback_imp_url_ostr << "&h=" <<
              AdServer::LogProcessing::user_id_distribution_hash(user_id);
          }

          if(request_params.common_info.log_as_test)
          {
            passback_imp_url_ostr << "&testrequest=1";
          }

          ad_slot_result.passback_url << passback_imp_url_ostr.str();
        }
        else if(tag)
        {
          instantiate_passback(
            ad_slot_result.mime_format,
            ad_slot_result.creative_body,
            campaign_config,
            colocation,
            tag,
            ad_slot.format,
            request_params,
            ad_slot_context,
            String::SubString(ad_slot.ext_tag_id));
        }
      }

      /* trace campaign if need */
      if(ad_slot_debug_info &&
         configuration_index().in() &&
         ad_slot.debug_ccg)
      {
        CORBA::String_var trace_out;
        trace_campaign_selection(
          ad_slot.debug_ccg,
          request_params,
          ad_slot,
          ad_slot_debug_info->auction_type,
          ad_slot_context.test_request,
          trace_out);

        ad_slot_debug_info->trace_ccg = trace_out;
      }
    }

    void
    CampaignManagerImpl::get_adslot_campaign_creative_(
      const CampaignConfig* campaign_config,
      Proto::RequestCreativeResult& /*ad_request_result*/,
      Proto::AdSlotResult& ad_slot_result,
      AdServer::CampaignSvcs::RevenueDecimal& adsspace_system_cpm,
      const Proto::RequestParams& request_params,
      const Generics::Time& session_start,
      const Colocation* colocation,
      const Proto::AdSlotInfo& ad_slot,
      const CreativeInstantiateRule& creative_instantiate_rule,
      Proto::AdRequestDebugInfo* ad_request_debug_info,
      Proto::AdSlotDebugInfo* ad_slot_debug_info,
      AdSlotContext& ad_slot_context,
      const ChannelIdHashSet& matched_channels,
      unsigned long& request_tag_id) noexcept
    {
      bool passback = ad_slot.passback();
      bool process_request = true;

      if(ad_slot_debug_info)
      {
        ad_slot_debug_info->set_cpm_threshold(
          GrpcAlgs::pack_decimal(RevenueDecimal::ZERO));
      }

      const Tag* tag = resolve_tag(
        &ad_slot_context.tag_size,
        &ad_slot_context.publisher_account_id,
        request_params,
        *campaign_config,
        ad_slot);

      if(tag)
      {
        {
          if (!request_tag_id)
          {
            request_tag_id = tag->tag_id;
          }
          ad_slot_result.set_pub_currency_code(
            tag->site->account->currency->currency_code);
        }
      }

      if(!request_params.common_info().passback_url().empty())
      {
        ad_slot_context.passback_url =
          request_params.common_info().passback_url();
        creative_instantiate_rule.instantiate_relative_protocol_url(
          ad_slot_context.passback_url);
      }
      else
      {
        fill_passback_url_from_tag(
          ad_slot_context.passback_url,
          creative_instantiate_rule,
          tag);
      }

      if(ad_slot_debug_info)
      {
        ad_slot_debug_info->set_tag_id(0);
        ad_slot_debug_info->set_site_id(0);
        ad_slot_debug_info->set_site_rate_id(0);
        ad_slot_debug_info->set_cpm_threshold(
          GrpcAlgs::pack_decimal(RevenueDecimal::ZERO));
      }

      if(tag)
      {
        ad_slot_context.test_request |= tag->is_test();

        if(process_request)
        {
          // Check & fill min_ecpm
          AdSlotMinCpm ad_slot_min_cpm;

          CORBACommons::DecimalInfo min_ecpm;
          min_ecpm.length(ad_slot.min_ecpm().size());
          std::memcpy(
            min_ecpm.get_buffer(),
            ad_slot.min_ecpm().data(),
            ad_slot.min_ecpm().size());

          if (!fill_ad_slot_min_cpm_(
                ad_slot_min_cpm,
                campaign_config,
                tag,
                String::SubString(ad_slot.min_ecpm_currency_code()),
                min_ecpm))
          {
            passback = true;
          }
          else
          {
            adsspace_system_cpm = std::min(
              adsspace_system_cpm, ad_slot_min_cpm.min_pub_ecpm_system);
          }

          try
          {
            get_campaign_creative_(
              request_params,
              ad_slot,
              ad_slot_context,
              ad_slot_min_cpm,
              session_start,
              ad_slot_result,
              tag,
              colocation,
              passback,
              ad_request_debug_info,
              ad_slot_debug_info,
              matched_channels);
          }
          catch(const eh::Exception&)
          {
            ad_slot_result.mutable_selected_creatives()->Clear();

            if(ad_slot_debug_info)
            {
              ad_slot_debug_info->mutable_selected_creatives()->Clear();
            }
          }

          ad_slot_result.set_test_request(ad_slot_context.test_request);
        }

        if(ad_request_debug_info)
        {
          const auto& geo_channels = request_params.context_info().geo_channels();
          ad_request_debug_info->mutable_geo_channels()->Add(
            std::begin(geo_channels),
            std::end(geo_channels));
        }
      }
      else
      {
        CampaignManagerLogger::RequestInfo request_info;

        CampaignManagerLogAdapter::fill_request_info(
          request_info,
          campaign_config,
          colocation,
          request_params.common_info(),
          request_params.context_info(),
          &request_params,
          ad_request_debug_info,
          ad_slot_context);

        campaign_manager_logger_->process_request(request_info);
      }

      passback |=
        ad_slot_result.selected_creatives().size() == 0;

      ad_slot_result.set_passback(passback);

      if (request_params.preview_ccid() &&
          !ad_slot_result.selected_creatives().empty())
      {
        passback = false;
      }

      if(passback && request_params.required_passback())
      {
        if(!ad_slot_context.passback_url.empty() &&
           ::strcmp(request_params.common_info().passback_type().c_str(), "redir") == 0)
        {
          std::ostringstream passback_imp_url_ostr;
          std::string mime_passback_url;

          String::StringManip::mime_url_encode(
            ad_slot_context.passback_url, mime_passback_url);

          passback_imp_url_ostr << creative_instantiate_rule.passback_pixel_url <<
            "?requestid=" << GrpcAlgs::unpack_request_id(
              request_params.common_info().request_id()).to_string() <<
            "&random=" << request_params.common_info().random() <<
            "&passback=" << mime_passback_url;

          AdServer::Commons::UserId user_id =
            GrpcAlgs::unpack_user_id(request_params.common_info().user_id());
          if(!user_id.is_null())
          {
            passback_imp_url_ostr << "&h=" <<
              AdServer::LogProcessing::user_id_distribution_hash(user_id);
          }

          if(request_params.common_info().log_as_test())
          {
            passback_imp_url_ostr << "&testrequest=1";
          }

          ad_slot_result.set_passback_url(passback_imp_url_ostr.str());
        }
        else if(tag)
        {
          instantiate_passback(
            *ad_slot_result.mutable_mime_format(),
            *ad_slot_result.mutable_creative_body(),
            campaign_config,
            colocation,
            tag,
            ad_slot.format().c_str(),
            request_params,
            ad_slot_context,
            String::SubString(ad_slot.ext_tag_id()));
        }
      }

      /* trace campaign if need */
      if(ad_slot_debug_info &&
        configuration_index().in() &&
        ad_slot.debug_ccg())
      {
        std::string trace_out;
        trace_campaign_selection_(
          ad_slot.debug_ccg(),
          request_params,
          ad_slot,
          ad_slot_debug_info->auction_type(),
          ad_slot_context.test_request,
          trace_out);

        ad_slot_debug_info->set_trace_ccg(trace_out);
      }
    }

    void
    CampaignManagerImpl::trace_campaign_selection(
      CORBA::ULong campaign_id,
      const AdServer::CampaignSvcs::CampaignManager::RequestParams&
        request_params,
      const AdServer::CampaignSvcs::CampaignManager::AdSlotInfo& ad_slot,
      CORBA::ULong auction_type,
      CORBA::Boolean test_request,
      CORBA::String_out trace_xml)
      /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException)*/
    {
      static const char* FUN = "CampaignManagerImpl::trace_campaign_selection()";

      try
      {
        CampaignConfig_var used_campaign_config = configuration();
        CampaignIndex_var used_campaign_index = configuration_index();

        if (used_campaign_config.in() == 0 ||
            used_campaign_index.in() == 0)
        {
          Stream::Error ostr;
          ostr << FUN << ": Can't get the configuration.";

          logger_->log(ostr.str(),
            Logging::Logger::EMERGENCY,
            Aspect::CAMPAIGN_MANAGER,
            "ADS-IMPL-172");

          CORBACommons::throw_desc<
            CampaignSvcs::CampaignManager::ImplementationException>(
              ostr.str());
        }

        SyncPolicy::WriteGuard guard(lock_);

        std::ostringstream trace_stream;
        trace_stream << "<request>" << std::endl;

        const Tag* tag = resolve_tag(
          0,
          0, // out publisher account id
          request_params,
          *used_campaign_config,
          ad_slot);

        Generics::Time current_time = CorbaAlgs::unpack_time(
          request_params.common_info.time);

        if(tag)
        {
          Campaign_var campaign;

          CampaignConfig::CampaignMap::const_iterator it =
            used_campaign_config->campaigns.find(campaign_id);

          if(it != used_campaign_config->campaigns.end())
          {
            campaign = it->second;
          }

          if(campaign.in())
          {
            CampaignIndex_var config_index(
              new CampaignIndex(*used_campaign_index, logger_));

            CampaignIndex::Key key(tag);

            key.country_code = request_params.common_info.location.length() ?
              request_params.common_info.location[0].country.in() : "";
            key.format = ad_slot.format;
            key.user_status = static_cast<UserStatus>(request_params.common_info.user_status);
            key.test_request = test_request;
            key.tag_delivery_factor = request_params.tag_delivery_factor;
            key.ccg_delivery_factor = request_params.ccg_delivery_factor;

            // for trace we use uid & hid channels union
            ChannelIdHashSet triggered_channels;
            CorbaAlgs::convert_sequence(request_params.channels, triggered_channels);
            CorbaAlgs::convert_sequence(request_params.hid_channels, triggered_channels);
            CorbaAlgs::convert_sequence(
              request_params.context_info.geo_channels,
              triggered_channels);
	    triggered_channels.emplace(TRUE_CHANNEL_ID);

            FreqCapIdSet full_freq_caps;
            std::copy(
              request_params.full_freq_caps.get_buffer(),
              request_params.full_freq_caps.get_buffer() + request_params.full_freq_caps.length(),
              std::inserter(full_freq_caps, full_freq_caps.begin()));

            CreativeCategoryIdSet exclude_categories;

            convert_external_categories_(
              exclude_categories,
              *used_campaign_config,
              request_params,
              ad_slot.exclude_categories);

            CreativeCategoryIdSet required_categories;

            convert_external_categories_(
              required_categories,
              *used_campaign_config,
              request_params,
              ad_slot.required_categories);

            if(ad_slot.required_categories.length() > 0 && required_categories.empty())
            {
              // some category can't be resolved
              required_categories.insert(0);
            }

            AdSlotMinCpm ad_slot_min_cpm;

            fill_ad_slot_min_cpm_(
              ad_slot_min_cpm,
              used_campaign_config,
              tag,
              String::SubString(ad_slot.min_ecpm_currency_code),
              ad_slot.min_ecpm);

            std::set<unsigned long> allowed_durations;
            CorbaAlgs::convert_sequence(ad_slot.allowed_durations, allowed_durations);

            config_index->trace_indexing(
              key,
              current_time,
              request_params.profiling_available,
              full_freq_caps,
              request_params.common_info.colo_id,
              CorbaAlgs::unpack_time(request_params.client_create_time),
              triggered_channels,
              CorbaAlgs::unpack_user_id(request_params.common_info.user_id),
              campaign,
              ad_slot_min_cpm.min_pub_ecpm_system,
              ad_slot.up_expand_space >= 0 ?
                static_cast<unsigned long>(ad_slot.up_expand_space) : 0,
              ad_slot.right_expand_space >= 0 ?
                static_cast<unsigned long>(ad_slot.right_expand_space) : 0,
              ad_slot.down_expand_space >= 0 ?
                static_cast<unsigned long>(ad_slot.down_expand_space) : 0,
              ad_slot.left_expand_space >= 0 ?
                static_cast<unsigned long>(ad_slot.left_expand_space) : 0,
              ad_slot.tag_visibility,
              ad_slot.video_min_duration,
              ad_slot.video_max_duration >= 0 ?
                std::optional<unsigned long>(ad_slot.video_max_duration) :
                std::optional<unsigned long>(),
              ad_slot.video_skippable_max_duration >= 0 ?
                std::optional<unsigned long>(ad_slot.video_skippable_max_duration) :
                std::optional<unsigned long>(),
              ad_slot.video_allow_skippable,
              ad_slot.video_allow_unskippable,
              allowed_durations,
              exclude_categories,
              required_categories,
              AuctionType(auction_type),
              (request_params.common_info.creative_instantiate_type == AdInstantiateRule::SECURE),
              (request_params.common_info.request_type == AR_GOOGLE),
              trace_stream);
          }
          else
          {
            trace_stream << "  <message>can't find campaign</message>" << std::endl;
          }
        }
        else
        {
          trace_stream << "  <message>can't find tag: "
            "tag_id = " << ad_slot.tag_id <<
            ", sizes = ";
          for(CORBA::ULong i = 0; i < ad_slot.sizes.length(); ++i)
          {
            trace_stream << (i != 0 ? "," : "") << ad_slot.sizes[i];
          }
          trace_stream << ", publisher_account_id = ";
          for(CORBA::ULong i = 0; i < request_params.publisher_account_ids.length(); ++i)
          {
            trace_stream << (i != 0 ? "," : "") << request_params.publisher_account_ids[0];
          }
          trace_stream << "</message>" << std::endl;
        }

        trace_stream << "</request>" << std::endl;

        trace_xml << trace_stream.str();
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << ex.what();

        logger_->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          Aspect::CAMPAIGN_MANAGER,
          "ADS-IMPL-174");
        CORBACommons::throw_desc<
          CampaignSvcs::CampaignManager::ImplementationException>(
            ostr.str());
      }
      catch(const CORBA::SystemException& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": CORBA::SystemException caught: " << ex;

        logger_->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          Aspect::CAMPAIGN_MANAGER,
          "ADS-IMPL-175");
        CORBACommons::throw_desc<
          CampaignSvcs::CampaignManager::ImplementationException>(
            ostr.str());
      }
    }

    CampaignManagerImpl::TraceCampaignSelectionResponsePtr
    CampaignManagerImpl::trace_campaign_selection(TraceCampaignSelectionRequestPtr&& request)
    {
      const auto id_request_grpc = request->id_request_grpc();
      try
      {
        auto response = create_grpc_response<Proto::TraceCampaignSelectionResponse>(
          id_request_grpc);
        auto* info_response = response->mutable_info();
        auto* trace_xml_response = info_response->mutable_trace_xml();

        trace_campaign_selection_(
          request->campaign_id(),
          request->request_params(),
          request->ad_slot(),
          request->auction_type(),
          request->test_request(),
          *trace_xml_response);

        return response;
      }
      catch (const NotReady& exc)
      {
        Stream::Error stream;
        stream << FNS
               << "Caught CampaignManager::NotReady: "
               << exc.what();
        auto response = create_grpc_error_response<Proto::TraceCampaignSelectionResponse>(
          Proto::Error_Type::Error_Type_NotReady,
          stream.str(),
          id_request_grpc);
        return response;
      }
      catch (const eh::Exception& exc)
      {
        Stream::Error stream;
        stream << FNS
               << ": "
               << exc.what();
        auto response = create_grpc_error_response<Proto::TraceCampaignSelectionResponse>(
          Proto::Error_Type::Error_Type_Implementation,
          stream.str(),
          id_request_grpc);
        return response;
      }
      catch (...)
      {
        Stream::Error stream;
        stream << FNS
               << ": Unknown error";
        auto response = create_grpc_error_response<Proto::TraceCampaignSelectionResponse>(
          Proto::Error_Type::Error_Type_Implementation,
          stream.str(),
          id_request_grpc);
        return response;
      }
    }

    void
    CampaignManagerImpl::trace_campaign_selection_(
      std::uint32_t campaign_id,
      const Proto::RequestParams& request_params,
      const Proto::AdSlotInfo& ad_slot,
      std::uint32_t auction_type,
      bool test_request,
      std::string& trace_xml)
    {
      static const char* FUN = "CampaignManagerImpl::trace_campaign_selection_()";

      CampaignConfig_var used_campaign_config = configuration();
      CampaignIndex_var used_campaign_index = configuration_index();

      if (used_campaign_config.in() == nullptr ||
          used_campaign_index.in() == nullptr)
      {
        Stream::Error ostr;
        ostr << FUN << ": Can't get the configuration.";

        logger_->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          Aspect::CAMPAIGN_MANAGER,
          "ADS-IMPL-172");

        throw ImplementationException(ostr.str());
      }

      SyncPolicy::WriteGuard guard(lock_);

      std::ostringstream trace_stream;
      trace_stream << "<request>" << std::endl;

      const Tag* tag = resolve_tag(
        0,
        0, // out publisher account id
        request_params,
        *used_campaign_config,
        ad_slot);

        Generics::Time current_time = GrpcAlgs::unpack_time(
          request_params.common_info().time());

      if(tag)
      {
        Campaign_var campaign;

        auto it = used_campaign_config->campaigns.find(campaign_id);
        if(it != used_campaign_config->campaigns.end())
        {
          campaign = it->second;
        }

        if(campaign.in())
        {
          CampaignIndex_var config_index(
            new CampaignIndex(*used_campaign_index, logger_));

          CampaignIndex::Key key(tag);

          key.country_code = !request_params.common_info().location().empty() ?
            request_params.common_info().location()[0].country().c_str() : "";
          key.format = ad_slot.format();
          key.user_status = static_cast<UserStatus>(request_params.common_info().user_status());
          key.test_request = test_request;
          key.tag_delivery_factor = request_params.tag_delivery_factor();
          key.ccg_delivery_factor = request_params.ccg_delivery_factor();

          // for trace we use uid & hid channels union
          ChannelIdHashSet triggered_channels;
          triggered_channels.insert(
            std::begin(request_params.channels()),
            std::end(request_params.channels()));
          triggered_channels.insert(
            std::begin(request_params.hid_channels()),
            std::end(request_params.hid_channels()));
          triggered_channels.insert(
            std::begin(request_params.context_info().geo_channels()),
            std::end(request_params.context_info().geo_channels()));

          FreqCapIdSet full_freq_caps(
            std::begin(request_params.full_freq_caps()),
            std::end(request_params.full_freq_caps()));

          CreativeCategoryIdSet exclude_categories;

          convert_external_categories_(
            exclude_categories,
            *used_campaign_config,
            request_params,
            ad_slot.exclude_categories());

          CreativeCategoryIdSet required_categories;

          convert_external_categories_(
            required_categories,
            *used_campaign_config,
            request_params,
            ad_slot.required_categories());

          if(ad_slot.required_categories().size() > 0 && required_categories.empty())
          {
            // some category can't be resolved
            required_categories.insert(0);
          }

          AdSlotMinCpm ad_slot_min_cpm;

          CORBACommons::DecimalInfo decimal_min_ecpm;
          if (!ad_slot.min_ecpm().empty())
          {
            decimal_min_ecpm.length(ad_slot.min_ecpm().size());
            std::memcpy(
              decimal_min_ecpm.get_buffer(),
              ad_slot.min_ecpm().data(),
              ad_slot.min_ecpm().size());
          }

          fill_ad_slot_min_cpm_(
            ad_slot_min_cpm,
            used_campaign_config,
            tag,
            String::SubString(ad_slot.min_ecpm_currency_code()),
            decimal_min_ecpm);

          std::set<unsigned long> allowed_durations(
            std::begin(ad_slot.allowed_durations()),
            std::end(ad_slot.allowed_durations()));

          config_index->trace_indexing(
            key,
            current_time,
            request_params.profiling_available(),
            full_freq_caps,
            request_params.common_info().colo_id(),
            GrpcAlgs::unpack_time(request_params.client_create_time()),
            triggered_channels,
            GrpcAlgs::unpack_user_id(request_params.common_info().user_id()),
            campaign,
            ad_slot_min_cpm.min_pub_ecpm_system,
            ad_slot.up_expand_space() >= 0 ?
              static_cast<unsigned long>(ad_slot.up_expand_space()) : 0,
            ad_slot.right_expand_space() >= 0 ?
              static_cast<unsigned long>(ad_slot.right_expand_space()) : 0,
            ad_slot.down_expand_space() >= 0 ?
              static_cast<unsigned long>(ad_slot.down_expand_space()) : 0,
            ad_slot.left_expand_space() >= 0 ?
              static_cast<unsigned long>(ad_slot.left_expand_space()) : 0,
            ad_slot.tag_visibility(),
            ad_slot.video_min_duration(),
            ad_slot.video_max_duration() >= 0 ?
              std::optional<unsigned long>(ad_slot.video_max_duration()) :
              std::optional<unsigned long>(),
            ad_slot.video_skippable_max_duration() >= 0 ?
              std::optional<unsigned long>(ad_slot.video_skippable_max_duration()) :
              std::optional<unsigned long>(),
            ad_slot.video_allow_skippable(),
            ad_slot.video_allow_unskippable(),
            allowed_durations,
            exclude_categories,
            required_categories,
            AuctionType(auction_type),
            (request_params.common_info().creative_instantiate_type() == AdInstantiateRule::SECURE),
            (request_params.common_info().request_type() == AR_GOOGLE),
            trace_stream);
        }
        else
        {
          trace_stream << "  <message>can't find campaign</message>" << std::endl;
        }
      }
      else
      {
        trace_stream << "  <message>can't find tag: "
          "tag_id = " << ad_slot.tag_id() <<
          ", sizes = ";
        for(int i = 0; i < ad_slot.sizes().size(); ++i)
        {
          trace_stream << (i != 0 ? "," : "") << ad_slot.sizes()[i];
        }
        trace_stream << ", publisher_account_id = ";
        for(int i = 0; i < request_params.publisher_account_ids().size(); ++i)
        {
          trace_stream << (i != 0 ? "," : "") << request_params.publisher_account_ids()[0];
        }
        trace_stream << "</message>" << std::endl;
      }

      trace_stream << "</request>" << std::endl;

      trace_xml = trace_stream.str();
    }

    void
    CampaignManagerImpl::trace_campaign_selection_index(
      CORBA::String_out trace_xml)
      /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException)*/
    {
      static const char* FUN = "CampaignManagerImpl::trace_campaign_selection_index()";

      try
      {
        CampaignIndex_var config_index = configuration_index();
        if (config_index.in() == 0)
        {
          Stream::Error ostr;
          ostr << FUN << ": Can't get the configuration.";

          logger_->log(ostr.str(),
            Logging::Logger::EMERGENCY,
            Aspect::CAMPAIGN_MANAGER,
            "ADS-IMPL-176");
          CORBACommons::throw_desc<
            CampaignSvcs::CampaignManager::ImplementationException>(
              ostr.str());
        }

        std::ostringstream trace_stream;
        config_index->trace_tree(trace_stream);
        trace_xml << trace_stream.str();
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught. : " << ex.what();

        logger_->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          Aspect::CAMPAIGN_MANAGER,
          "ADS-IMPL-176");
        CORBACommons::throw_desc<
          CampaignSvcs::CampaignManager::ImplementationException>(
            ostr.str());
      }
    }

    CampaignManagerImpl::TraceCampaignSelectionIndexResponsePtr
    CampaignManagerImpl::trace_campaign_selection_index(
      TraceCampaignSelectionIndexRequestPtr&& request)
    {
      const auto id_request_grpc = request->id_request_grpc();
      try
      {
        CampaignIndex_var config_index = configuration_index();
        if (!config_index)
        {
          Stream::Error stream;
          stream << FNS
                 << ": Can't get the configuration.";

          logger_->log(stream.str(),
                       Logging::Logger::EMERGENCY,
                       Aspect::CAMPAIGN_MANAGER,
                       "ADS-IMPL-176");
          throw ImplementationException(stream.str());
        }

        auto response = create_grpc_response<Proto::TraceCampaignSelectionIndexResponse>(
          id_request_grpc);
        auto* info_response = response->mutable_info();

        std::ostringstream trace_stream;
        config_index->trace_tree(trace_stream);
        info_response->set_trace_xml(trace_stream.str());

        return response;
      }
      catch (const NotReady& exc)
      {
        Stream::Error stream;
        stream << FNS
               << "Caught CampaignManager::NotReady: "
               << exc.what();
        auto response = create_grpc_error_response<Proto::TraceCampaignSelectionIndexResponse>(
          Proto::Error_Type::Error_Type_NotReady,
          stream.str(),
          id_request_grpc);
        return response;
      }
      catch (const eh::Exception& exc)
      {
        Stream::Error stream;
        stream << FNS
               << ": "
               << exc.what();
        auto response = create_grpc_error_response<Proto::TraceCampaignSelectionIndexResponse>(
          Proto::Error_Type::Error_Type_Implementation,
          stream.str(),
          id_request_grpc);
        return response;
      }
      catch (...)
      {
        Stream::Error stream;
        stream << FNS
               << ": Unknown error";
        auto response = create_grpc_error_response<Proto::TraceCampaignSelectionIndexResponse>(
          Proto::Error_Type::Error_Type_Implementation,
          stream.str(),
          id_request_grpc);
        return response;
      }
    }

    void
    CampaignManagerImpl::get_bid_costs_(
      RevenueDecimal& low_predicted_pub_ecpm_system,
      RevenueDecimal& top_predicted_pub_ecpm_system,
      const AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
      const Tag* tag,
      const RevenueDecimal& min_pub_ecpm_system,
      const CampaignSelectionDataList& selected_campaigns)
    {
      RevenueDecimal sum_campaigns_ecpm = RevenueDecimal::ZERO;

      for(CampaignSelectionDataList::const_iterator it = selected_campaigns.begin();
          it != selected_campaigns.end();
          ++it)
      {
        sum_campaigns_ecpm += it->ecpm_bid;
      }

      RevenueDecimal sum_campaigns_imp_cost_system = RevenueDecimal::div(
        sum_campaigns_ecpm,
        ECPM_FACTOR,
        Generics::DDR_FLOOR);

      RevenueDecimal init_predicted_ecpm_system = std::max(
        RevenueDecimal::mul(
          sum_campaigns_imp_cost_system, ECPM_FACTOR, Generics::DMR_CEIL),
        min_pub_ecpm_system);

      low_predicted_pub_ecpm_system = init_predicted_ecpm_system;

      std::optional<BidCostProvider::RequestParams> bid_request_params;

      for(CampaignSelectionDataList::const_iterator it = selected_campaigns.begin();
          it != selected_campaigns.end();
          ++it)
      {
        if(it->campaign->ccg_rate_type == CR_MAXBID)
        {
          ConstBidCostProvider_var bid_cost_provider = bid_cost_provider_.get();
          if(bid_cost_provider)
          {
            if(!bid_request_params.has_value())
            {
              bid_request_params = BidCostProvider::RequestParams();
              init_bid_request_params(*bid_request_params, request_params, tag);
            }

            std::optional<RevenueDecimal> local_low_predicted_pub_ecpm_system =
              bid_cost_provider->get_bid_cost(
                *bid_request_params,
                LOW_ALLOWABLE_LOSE_WIN_PERCENTAGE,
                sum_campaigns_imp_cost_system);
            if(local_low_predicted_pub_ecpm_system.has_value())
            {
              // convert cost per impression to ecpm (0.01 / 1000)
              low_predicted_pub_ecpm_system = std::min(
                std::max(
                  RevenueDecimal::div(
                    RevenueDecimal::mul(
                      *local_low_predicted_pub_ecpm_system, ECPM_FACTOR, Generics::DMR_CEIL),
                    REVENUE_ONE + tag->cost_coef,
                    Generics::DDR_FLOOR
                  ),
                  min_pub_ecpm_system),
                sum_campaigns_ecpm);
            }

            break;
          }
        }
      }

      top_predicted_pub_ecpm_system = init_predicted_ecpm_system;

      if(!selected_campaigns.empty())
      {
        ConstBidCostProvider_var bid_cost_provider = bid_cost_provider_.get();
        if(bid_cost_provider)
        {
          if(!bid_request_params.has_value())
          {
            bid_request_params = BidCostProvider::RequestParams();
            init_bid_request_params(*bid_request_params, request_params, tag);
          }

          std::optional<RevenueDecimal> local_top_predicted_pub_ecpm_system =
            bid_cost_provider->get_bid_cost(
              *bid_request_params,
              TOP_ALLOWABLE_LOSE_WIN_PERCENTAGE,
              sum_campaigns_imp_cost_system);
          if(local_top_predicted_pub_ecpm_system.has_value())
          {
            // convert cost per impression to ecpm (0.01 / 1000)
            top_predicted_pub_ecpm_system = std::min(
              std::max(
                RevenueDecimal::div(
                  RevenueDecimal::mul(
                    *local_top_predicted_pub_ecpm_system, ECPM_FACTOR, Generics::DMR_CEIL),
                  REVENUE_ONE + tag->cost_coef,
                  Generics::DDR_FLOOR
                ),
                min_pub_ecpm_system),
              sum_campaigns_ecpm);
          }
        }
      }
    }

    void
    CampaignManagerImpl::get_bid_costs_(
      RevenueDecimal& low_predicted_pub_ecpm_system,
      RevenueDecimal& top_predicted_pub_ecpm_system,
      const Proto::RequestParams& request_params,
      const Tag* tag,
      const RevenueDecimal& min_pub_ecpm_system,
      const CampaignSelectionDataList& selected_campaigns)
    {
      RevenueDecimal sum_campaigns_ecpm = RevenueDecimal::ZERO;

      for(CampaignSelectionDataList::const_iterator it = selected_campaigns.begin();
          it != selected_campaigns.end();
          ++it)
      {
        sum_campaigns_ecpm += it->ecpm_bid;
      }

      RevenueDecimal sum_campaigns_imp_cost_system = RevenueDecimal::div(
        sum_campaigns_ecpm,
        ECPM_FACTOR,
        Generics::DDR_FLOOR);

      RevenueDecimal init_predicted_ecpm_system = std::max(
        RevenueDecimal::mul(
          sum_campaigns_imp_cost_system, ECPM_FACTOR, Generics::DMR_CEIL),
        min_pub_ecpm_system);

      low_predicted_pub_ecpm_system = init_predicted_ecpm_system;

      std::optional<BidCostProvider::RequestParams> bid_request_params;

      for(CampaignSelectionDataList::const_iterator it = selected_campaigns.begin();
          it != selected_campaigns.end();
          ++it)
      {
        if(it->campaign->ccg_rate_type == CR_MAXBID)
        {
          ConstBidCostProvider_var bid_cost_provider = bid_cost_provider_.get();
          if(bid_cost_provider)
          {
            if(!bid_request_params.has_value())
            {
              bid_request_params = BidCostProvider::RequestParams();
              init_bid_request_params(*bid_request_params, request_params, tag);
            }

            std::optional<RevenueDecimal> local_low_predicted_pub_ecpm_system =
              bid_cost_provider->get_bid_cost(
                *bid_request_params,
                LOW_ALLOWABLE_LOSE_WIN_PERCENTAGE,
                sum_campaigns_imp_cost_system);
            if(local_low_predicted_pub_ecpm_system.has_value())
            {
              // convert cost per impression to ecpm (0.01 / 1000)
              low_predicted_pub_ecpm_system = std::min(
                std::max(
                  RevenueDecimal::div(
                    RevenueDecimal::mul(
                      *local_low_predicted_pub_ecpm_system, ECPM_FACTOR, Generics::DMR_CEIL),
                    REVENUE_ONE + tag->cost_coef,
                    Generics::DDR_FLOOR
                  ),
                  min_pub_ecpm_system),
                sum_campaigns_ecpm);
            }

            break;
          }
        }
      }

      top_predicted_pub_ecpm_system = init_predicted_ecpm_system;

      if(!selected_campaigns.empty())
      {
        ConstBidCostProvider_var bid_cost_provider = bid_cost_provider_.get();
        if(bid_cost_provider)
        {
          if(!bid_request_params.has_value())
          {
            bid_request_params = BidCostProvider::RequestParams();
            init_bid_request_params(*bid_request_params, request_params, tag);
          }

          std::optional<RevenueDecimal> local_top_predicted_pub_ecpm_system =
            bid_cost_provider->get_bid_cost(
              *bid_request_params,
              TOP_ALLOWABLE_LOSE_WIN_PERCENTAGE,
              sum_campaigns_imp_cost_system);
          if(local_top_predicted_pub_ecpm_system.has_value())
          {
            // convert cost per impression to ecpm (0.01 / 1000)
            top_predicted_pub_ecpm_system = std::min(
              std::max(
                RevenueDecimal::div(
                  RevenueDecimal::mul(
                    *local_top_predicted_pub_ecpm_system, ECPM_FACTOR, Generics::DMR_CEIL),
                  REVENUE_ONE + tag->cost_coef,
                  Generics::DDR_FLOOR
                ),
                min_pub_ecpm_system),
              sum_campaigns_ecpm);
          }
        }
      }
    }

    void
    CampaignManagerImpl::get_campaign_creative_(
      const AdServer::CampaignSvcs::CampaignManager::RequestParams&
        request_params,
      const AdServer::CampaignSvcs::CampaignManager::AdSlotInfo& ad_slot,
      AdSlotContext& ad_slot_context,
      const AdSlotMinCpm& ad_slot_min_cpm,
      const Generics::Time& session_start,
      AdServer::CampaignSvcs::CampaignManager::AdSlotResult& ad_slot_result,
      const Tag* tag,
      const Colocation* colocation,
      bool passback,
      AdServer::CampaignSvcs::CampaignManager::AdRequestDebugInfo* ad_request_debug_info,
      AdServer::CampaignSvcs::CampaignManager::AdSlotDebugInfo* ad_slot_debug_info,
      const ChannelIdHashSet& matched_channels)
      /*throw(eh::Exception)*/
    {
      static const char* FUN = "CampaignManagerImpl::get_campaign_creative_()";

      // configuring response by input parameters
      if(ad_request_debug_info)
      {
        ad_request_debug_info->colo_id = request_params.common_info.colo_id;
        ad_request_debug_info->user_group_id =
          CorbaAlgs::unpack_user_id(request_params.common_info.user_id).hash() %
            MAX_TARGET_USERS_GROUPS;
      }

      CampaignIndex_var config_index = configuration_index();

      ConstCampaignConfig_var const_config = config_index.in() ?
        config_index->configuration() :
        ConstCampaignConfig_var(configuration());

      // checking noads_timeout period
      if (tag->site->noads_timeout != 0 &&
          session_start + tag->site->noads_timeout >
            CorbaAlgs::unpack_time(request_params.common_info.time))
      {
        passback = true;
      }

      if(!config_index.in())
      {
        passback = true;
      }

      AdSelectionResult ad_selection_result;
      RequestResultParams request_result_params;
      
      request_result_params.request_id =
        CorbaAlgs::unpack_request_id(request_params.common_info.request_id);
      
      Tag::SizeMap tag_sizes;
      if (!ad_slot_context.request_blacklisted)
      {
        for(Tag::SizeMap::const_iterator tag_size_it = tag->sizes.begin();
            tag_size_it != tag->sizes.end(); ++tag_size_it)
        {
          if(!size_blacklisted_(
               const_config,
               matched_channels,
               tag_size_it->second->size->size_id))
          {
            tag_sizes.insert(*tag_size_it);
          }
        }
      }
      
      if(tag_sizes.empty())
      {
        ad_slot_context.request_blacklisted = true;
        passback = true;
      }

      bool is_preview_ccid = false;

      if (request_params.preview_ccid &&
          preview_ccid_(
            const_config,
            colocation,
            tag,
            request_params,
            ad_slot,
            ad_slot_context,
            request_result_params,
            ad_selection_result,
            ad_slot_result.creative_body))
      {
        is_preview_ccid = true;
        passback = false;
      }

      if(!passback)
      {
        // fill tokens
        if (!is_preview_ccid)
        {
          SeqOrderMap seq_orders;

          for(CORBA::ULong seq_order_i = 0;
              seq_order_i < request_params.seq_orders.length();
              ++seq_order_i)
          {
            SeqOrder& seq_order = seq_orders[request_params.seq_orders[seq_order_i].ccg_id];
            seq_order.set_id = request_params.seq_orders[seq_order_i].set_id;
            seq_order.imps = request_params.seq_orders[seq_order_i].imps;
          }

          get_site_creative_(
            config_index.in(),
            colocation,
            tag,
            tag_sizes,
            request_params,
            ad_slot,
            ad_slot_context,
            ad_slot_min_cpm,
            ad_slot_context.full_freq_caps,
            seq_orders,
            request_result_params,
            ad_selection_result,
            ad_slot_result.creative_body,
            ad_slot_result.creative_url,
            ad_request_debug_info,
            ad_slot_debug_info);
        }

        if(ad_slot_debug_info)
        {
          ad_slot_debug_info->site_id = tag->site->site_id;
          ad_slot_debug_info->tag_id = tag ? tag->tag_id : 0;
          ad_slot_debug_info->tag_size_id =
            ad_selection_result.tag_size ? ad_selection_result.tag_size->size->size_id : 0;
          ad_slot_debug_info->min_no_adv_ecpm =
            ad_selection_result.min_no_adv_ecpm.integer<unsigned long>();
          ad_slot_debug_info->min_text_ecpm =
            ad_selection_result.min_text_ecpm.integer<unsigned long>();
          ad_slot_debug_info->cpm_threshold = CorbaAlgs::pack_decimal(
            ad_selection_result.cpm_threshold);
          ad_slot_debug_info->track_pixel_url <<
            request_result_params.track_pixel_url;
          ad_slot_debug_info->walled_garden = ad_selection_result.walled_garden;
          ad_slot_debug_info->selected_creatives.length(
            ad_selection_result.selected_campaigns.size());
        }

        CorbaAlgs::fill_sequence(
          ad_selection_result.freq_caps.begin(),
          ad_selection_result.freq_caps.end(),
          ad_slot_result.freq_caps);
        CorbaAlgs::fill_sequence(
          ad_selection_result.uc_freq_caps.begin(),
          ad_selection_result.uc_freq_caps.end(),
          ad_slot_result.uc_freq_caps);

        if(!ad_selection_result.selected_campaigns.empty())
        {
          const Creative* creative =
            ad_selection_result.selected_campaigns.begin()->creative;

          ad_slot_result.erid << creative->erid;

          if(creative->initial_contract)
          {
            fill_campaign_contracts_(ad_slot_result.contracts,
              creative->initial_contract);
          }
        }

        // filtering campaign/creative selection by freq caps,
        // which presents in previously processed slots
        ad_slot_context.full_freq_caps.insert(
          ad_selection_result.freq_caps.begin(),
          ad_selection_result.freq_caps.end());
        ad_slot_context.full_freq_caps.insert(
          ad_selection_result.uc_freq_caps.begin(),
          ad_selection_result.uc_freq_caps.end());

        if(!ad_selection_result.selected_campaigns.empty())
        {
          ad_slot_result.notice_url << request_result_params.notice_url;
          if(request_params.fill_track_pixel)
          {
            if(!(request_result_params.track_pixel_url.empty() || ad_slot.fill_track_html))
            {
              ad_slot_result.track_pixel_urls.length(1);
              ad_slot_result.track_pixel_urls[0] << request_result_params.track_pixel_url;
            }

            if(!request_result_params.track_html_body.empty() && ad_slot.fill_track_html)
            {
              ad_slot_result.track_html_body << request_result_params.track_html_body;
            }

            CorbaAlgs::fill_sequence(
              request_result_params.add_track_pixel_urls.begin(),
              request_result_params.add_track_pixel_urls.end(),
              ad_slot_result.track_pixel_urls,
              true);
          }

          ad_slot_result.track_pixel_params << request_result_params.track_pixel_params;
          ad_slot_result.click_params << request_result_params.click_params;
          ad_slot_result.yandex_track_params << request_result_params.yandex_track_params;
          ad_slot_result.iurl << request_result_params.iurl;
          ad_slot_result.overlay_width = request_result_params.overlay_width;
          ad_slot_result.overlay_height = request_result_params.overlay_height;
          ad_slot_result.tokens.length(request_result_params.tokens.size());
          CORBA::ULong token_i = 0;
          for(std::map<std::string, std::string>::const_iterator token_it =
                request_result_params.tokens.begin();
              token_it != request_result_params.tokens.end();
              ++token_it, ++token_i)
          {
            ad_slot_result.tokens[token_i].name << token_it->first;
            ad_slot_result.tokens[token_i].value << token_it->second;
          }

          ad_slot_result.ext_tokens.length(request_result_params.ext_tokens.size());
          CORBA::ULong ext_token_i = 0;
          for(std::map<std::string, std::string>::const_iterator ext_token_it =
                request_result_params.ext_tokens.begin();
              ext_token_it != request_result_params.ext_tokens.end();
              ++ext_token_it, ++ext_token_i)
          {
            ad_slot_result.ext_tokens[token_i].name << ext_token_it->first;
            ad_slot_result.ext_tokens[token_i].value << ext_token_it->second;
          }

          // ADSC-10918 Native ads
          // Data tokens
          ad_slot_result.native_data_tokens.length(
            ad_slot.native_data_tokens.length());
          for (unsigned long token_i = 0;
               token_i < ad_slot.native_data_tokens.length(); ++token_i)
          {
            std::string token_name(ad_slot.native_data_tokens[token_i].name);
            ad_slot_result.native_data_tokens[token_i].name <<
              token_name;
            auto token_it = request_result_params.native_data_tokens.find(
              token_name);
            if (token_it != request_result_params.native_data_tokens.end())
            {
              ad_slot_result.native_data_tokens[token_i].value <<
                token_it->second;
            }
          }

          // Image tokens
          ad_slot_result.native_image_tokens.length(
            ad_slot.native_image_tokens.length());
          for (unsigned long token_i = 0;
               token_i < ad_slot.native_image_tokens.length(); ++token_i)
          {
            std::string token_name(ad_slot.native_image_tokens[token_i].name);
            ad_slot_result.native_image_tokens[token_i].name <<
              token_name;
            auto token_it = request_result_params.native_image_tokens.find(
              token_name);
            if (token_it != request_result_params.native_image_tokens.end())
            {
              ad_slot_result.native_image_tokens[token_i].value <<
                token_it->second.value;
              ad_slot_result.native_image_tokens[token_i].width =
                token_it->second.width;
              ad_slot_result.native_image_tokens[token_i].height =
                token_it->second.height;
            }
          }

          ad_slot_result.request_id = CorbaAlgs::pack_request_id(
            request_result_params.request_id);
          ad_slot_result.mime_format << request_result_params.mime_format;

          ad_slot_result.selected_creatives.length(
            ad_selection_result.selected_campaigns.size());

          CORBA::ULong i = 0;
          StringSet external_visual_categories;
          StringSet external_content_categories;

          // ad_slot_min_cpm.min_pub_ecpm_system
          RevenueDecimal low_predicted_pub_ecpm_system;
          RevenueDecimal top_predicted_pub_ecpm_system;

          {
            get_bid_costs_(
              low_predicted_pub_ecpm_system,
              top_predicted_pub_ecpm_system,
              request_params,
              tag,
              ad_slot_min_cpm.min_pub_ecpm_system,
              ad_selection_result.selected_campaigns);
          }

          // eval slot pub ecpm's
          RevenueDecimal slot_pub_ecpm = RevenueDecimal::ZERO;
          RevenueDecimal slot_low_predicted_pub_ecpm_system = RevenueDecimal::ZERO;
          RevenueDecimal slot_top_predicted_pub_ecpm_system = RevenueDecimal::ZERO;

          if(!ad_selection_result.selected_campaigns.empty())
          {
            RevenueDecimal selected_campaigns_count(false, ad_selection_result.selected_campaigns.size(), 0),
            slot_pub_ecpm = RevenueDecimal::div(
              ad_slot_min_cpm.min_pub_ecpm_system,
              selected_campaigns_count,
              Generics::DDR_FLOOR);
            slot_low_predicted_pub_ecpm_system = RevenueDecimal::div(
              low_predicted_pub_ecpm_system,
              selected_campaigns_count,
              Generics::DDR_FLOOR);
            slot_top_predicted_pub_ecpm_system = RevenueDecimal::div(
              top_predicted_pub_ecpm_system,
              selected_campaigns_count,
              Generics::DDR_FLOOR);
            (void)slot_pub_ecpm;
          }

          for(CampaignSelectionDataList::iterator it =
                ad_selection_result.selected_campaigns.begin();
              it != ad_selection_result.selected_campaigns.end();
              ++it, ++i)
          {
            assert(it->creative && it->campaign);

            it->count_impression = it->creative && !it->track_impr;

            AdServer::CampaignSvcs::CampaignManager::CreativeSelectResult& cs_result =
              ad_slot_result.selected_creatives[i];

            cs_result.request_id = CorbaAlgs::pack_request_id(it->request_id);
            cs_result.cmp_id = it->campaign->campaign_id;
            cs_result.campaign_group_id = it->campaign->campaign_group_id;
            cs_result.advertiser_id = it->campaign->advertiser->account_id;
            cs_result.advertiser_name << it->campaign->advertiser->legal_name;
            cs_result.order_set_id = it->campaign->seq_set_rotate_imps > 0 ?
              it->creative->order_set_id : 0;
            cs_result.revenue = CorbaAlgs::pack_decimal(
              it->track_impr ? RevenueDecimal::ZERO :
                it->campaign->imp_revenue);
            cs_result.ecpm = CorbaAlgs::pack_decimal(it->ecpm);

            RevenueDecimal cs_result_pub_ecpm;

            if(ad_selection_result.auction_type == AT_RANDOM)
            {
              cs_result_pub_ecpm = (i == 0 ? tag->pub_max_random_cpm :
                RevenueDecimal::ZERO);
            }
            else
            {
              //RevenueDecimal orig_ecpm_bid = it->ecpm_bid;

              // set random cost for 10% bids
              if(request_params.common_info.random % 10 == 0 && it->ecpm_bid > slot_pub_ecpm)
              {
                RevenueDecimal c = slot_pub_ecpm + RevenueDecimal::div(
                  RevenueDecimal::mul(
                    it->ecpm_bid - slot_pub_ecpm,
                    RevenueDecimal(false, Generics::safe_rand(100), 0),
                    Generics::DMR_FLOOR),
                  RevenueDecimal(false, 100, 0));
                it->ecpm_bid = c;
              }
              else
              {
                if(it->campaign->ccg_rate_type == CR_MAXBID)
                {
                  it->ecpm_bid = slot_low_predicted_pub_ecpm_system;
                }
                else
                {
                  it->ecpm_bid = slot_top_predicted_pub_ecpm_system;
                }
              }

              //std::cerr << "Bid on " << it->ecpm_bid << " instead " << orig_ecpm_bid << std::endl;

              cs_result_pub_ecpm = tag->site->account->currency->from_system_currency(
                it->ecpm_bid);
            }

            if(tag->cost_coef != RevenueDecimal::ZERO)
            {
              cs_result_pub_ecpm = RevenueDecimal::mul(
                cs_result_pub_ecpm,
                REVENUE_ONE + tag->cost_coef,
                Generics::DMR_FLOOR);
            }

            cs_result.pub_ecpm = CorbaAlgs::pack_decimal(cs_result_pub_ecpm);

            cs_result.ccid = it->creative->ccid;
            cs_result.creative_size <<
              ad_selection_result.tag_size->size->protocol_name;
            cs_result.click_url << it->click_url;
            cs_result.creative_version_id << it->creative->version_id;
            cs_result.creative_id = it->creative->creative_id;
            cs_result.https_safe_flag = it->creative->https_safe_flag;

            std::string destination_url =
              it->creative->destination_url.url();

            if (destination_url.empty() && it->campaign_keyword.in())
            {
              destination_url = it->campaign_keyword->click_url;
            }

            cs_result.destination_url << destination_url;

            AdRequestType request_type = reduce_request_type_(
              request_params.common_info.request_type);

            auto size_it = it->creative->sizes.find(
              ad_selection_result.tag_size->size->size_id);
            
            if (size_it != it->creative->sizes.end())
            {
              fill_expanding(
                size_it->second, cs_result.expanding);
            }

            if(request_type != AR_NORMAL)
            {
              for(Creative::CategorySet::const_iterator cat_it =
                    it->creative->categories.begin();
                  cat_it != it->creative->categories.end(); ++cat_it)
              {
                CampaignConfig::CreativeCategoryMap::const_iterator ccat_it =
                  const_config->creative_categories.find(*cat_it);
                if(ccat_it != const_config->creative_categories.end())
                {
                  CreativeCategory::ExternalCategoryMap::
                    const_iterator req_cat_it =
                      ccat_it->second.external_categories.find(request_type);

                  if(req_cat_it != ccat_it->second.external_categories.end())
                  {
                    if(ccat_it->second.cct_id == CCT_VISUAL)
                    {
                      std::copy(req_cat_it->second.begin(),
                        req_cat_it->second.end(),
                        std::inserter(external_visual_categories,
                          external_visual_categories.begin()));
                    }
                    else if(ccat_it->second.cct_id == CCT_CONTENT)
                    {
                      std::copy(req_cat_it->second.begin(),
                        req_cat_it->second.end(),
                        std::inserter(external_content_categories,
                          external_content_categories.begin()));
                    }
                  }
                }
              }
            }

            if(ad_slot_debug_info)
            {
              AdServer::CampaignSvcs::CampaignManager::CreativeSelectDebugInfo& cs_debug =
                ad_slot_debug_info->selected_creatives[i];
              cs_debug.ecpm_bid = CorbaAlgs::pack_decimal(it->ecpm_bid);
              cs_debug.imp_revenue = CorbaAlgs::pack_decimal(it->campaign->imp_revenue);
              cs_debug.click_revenue =
                it->campaign_keyword.in() ?
                CorbaAlgs::pack_decimal(it->actual_cpc) :
                CorbaAlgs::pack_decimal(it->campaign->click_revenue);

              cs_debug.action_revenue =
                CorbaAlgs::pack_decimal(it->campaign->action_revenue);
            }
          }

          {
            ad_slot_result.external_visual_categories.length(
              external_visual_categories.size());
            CORBA::ULong res_cat_i = 0;
            for(StringSet::const_iterator cat_it = external_visual_categories.begin();
                cat_it != external_visual_categories.end();
                ++cat_it, ++res_cat_i)
            {
              ad_slot_result.external_visual_categories[res_cat_i] << *cat_it;
            }
          }

          {
            ad_slot_result.external_content_categories.length(
              external_content_categories.size());
            CORBA::ULong res_cat_i = 0;
            for(StringSet::const_iterator cat_it = external_content_categories.begin();
                cat_it != external_content_categories.end();
                ++cat_it, ++res_cat_i)
            {
              ad_slot_result.external_content_categories[res_cat_i] << *cat_it;
            }
          }

          {
            CORBA::ULong i = 0;
            ad_slot_result.tokens.length(request_result_params.tokens.size());
            for(auto it = request_result_params.tokens.begin();
                it != request_result_params.tokens.end(); ++it, ++i)
            {
              ad_slot_result.tokens[i].name << it->first;
              ad_slot_result.tokens[i].value << it->second;
            }
          }

          ad_slot_result.track_impr =
            ad_selection_result.selected_campaigns.front().track_impr;
        }
      } // !passback

      try
      {
        CampaignManagerLogger::RequestInfo request_info;
        CampaignManagerLogger::AdRequestSelectionInfo
          ad_request_selection_info;

        CampaignManagerLogAdapter::fill_request_info(
          request_info,
          const_config,
          colocation,
          request_params.common_info,
          request_params.context_info,
          &request_params,
          ad_request_debug_info,
          ad_slot_context);

        CampaignManagerLogAdapter::fill_ad_request_selection_info(
          ad_request_selection_info,
          const_config,
          colocation,
          request_params.common_info,
          request_params.context_info,
          &request_params,
          ad_slot,
          tag,
          ad_selection_result,
          ad_slot_context,
          ad_slot_min_cpm,
          tag_sizes,
          false);

        campaign_manager_logger_->process_ad_request(
          request_info, ad_request_selection_info);
      }
      catch (const CampaignManagerLogger::Exception& e)
      {
        logger_->sstream(Logging::Logger::EMERGENCY,
          Aspect::CAMPAIGN_MANAGER,
          "ADS-IMPL-177") << FUN << ": "
          "eh::Exception caught while logging request: " << e.what();
      }
    }

    void
    CampaignManagerImpl::get_campaign_creative_(
      const Proto::RequestParams& request_params,
      const Proto::AdSlotInfo& ad_slot,
      AdSlotContext& ad_slot_context,
      const AdSlotMinCpm& ad_slot_min_cpm,
      const Generics::Time& session_start,
      Proto::AdSlotResult& ad_slot_result,
      const Tag* tag,
      const Colocation* colocation,
      bool passback,
      Proto::AdRequestDebugInfo* ad_request_debug_info,
      Proto::AdSlotDebugInfo* ad_slot_debug_info,
      const ChannelIdHashSet& matched_channels)
    {
      static const char* FUN = "CampaignManagerImpl::get_campaign_creative_()";

      // configuring response by input parameters
      if(ad_request_debug_info)
      {
        ad_request_debug_info->set_colo_id(request_params.common_info().colo_id());
        ad_request_debug_info->set_user_group_id(
          GrpcAlgs::unpack_user_id(request_params.common_info().user_id()).hash() %
            MAX_TARGET_USERS_GROUPS);
      }

      CampaignIndex_var config_index = configuration_index();

      ConstCampaignConfig_var const_config = config_index.in() ?
        config_index->configuration() :
        ConstCampaignConfig_var(configuration(false));
      if (!const_config)
      {
        throw NotReady("Campaign configuration isn't loaded");
      }

      // checking noads_timeout period
      if (tag->site->noads_timeout != 0 &&
          session_start + tag->site->noads_timeout >
            GrpcAlgs::unpack_time(request_params.common_info().time()))
      {
        passback = true;
      }

      if(!config_index.in())
      {
        passback = true;
      }

      AdSelectionResult ad_selection_result;
      RequestResultParams request_result_params;

      request_result_params.request_id =
        GrpcAlgs::unpack_request_id(request_params.common_info().request_id());

      Tag::SizeMap tag_sizes;
      if (!ad_slot_context.request_blacklisted)
      {
        for(auto tag_size_it = tag->sizes.begin();
            tag_size_it != tag->sizes.end();
            ++tag_size_it)
        {
          if(!size_blacklisted_(
               const_config,
               matched_channels,
               tag_size_it->second->size->size_id))
          {
            tag_sizes.insert(*tag_size_it);
          }
        }
      }

      if(tag_sizes.empty())
      {
        ad_slot_context.request_blacklisted = true;
        passback = true;
      }

      bool is_preview_ccid = false;

      if (request_params.preview_ccid() &&
          preview_ccid_(
            const_config,
            colocation,
            tag,
            request_params,
            ad_slot,
            ad_slot_context,
            request_result_params,
            ad_selection_result,
            *ad_slot_result.mutable_creative_body()))
      {
        is_preview_ccid = true;
        passback = false;
      }

      if(!passback)
      {
        // fill tokens
        if (!is_preview_ccid)
        {
          SeqOrderMap seq_orders;

          for(const auto& seq_order_request_params : request_params.seq_orders())
          {
            SeqOrder& seq_order = seq_orders[seq_order_request_params.ccg_id()];
            seq_order.set_id = seq_order_request_params.set_id();
            seq_order.imps = seq_order_request_params.imps();
          }

          get_site_creative_(
            config_index.in(),
            colocation,
            tag,
            tag_sizes,
            request_params,
            ad_slot,
            ad_slot_context,
            ad_slot_min_cpm,
            ad_slot_context.full_freq_caps,
            seq_orders,
            request_result_params,
            ad_selection_result,
            *ad_slot_result.mutable_creative_body(),
            *ad_slot_result.mutable_creative_url(),
            ad_request_debug_info,
            ad_slot_debug_info);
        }

        if(ad_slot_debug_info)
        {
          ad_slot_debug_info->set_site_id(tag->site->site_id);
          ad_slot_debug_info->set_tag_id(tag ? tag->tag_id : 0);
          ad_slot_debug_info->set_tag_size_id(
            ad_selection_result.tag_size ? ad_selection_result.tag_size->size->size_id : 0);
          ad_slot_debug_info->set_min_no_adv_ecpm(
            ad_selection_result.min_no_adv_ecpm.integer<unsigned long>());
          ad_slot_debug_info->set_min_text_ecpm(
            ad_selection_result.min_text_ecpm.integer<unsigned long>());
          ad_slot_debug_info->set_cpm_threshold(GrpcAlgs::pack_decimal(
            ad_selection_result.cpm_threshold));
          ad_slot_debug_info->set_track_pixel_url(
            request_result_params.track_pixel_url);
          ad_slot_debug_info->set_walled_garden(ad_selection_result.walled_garden);
          ad_slot_debug_info->mutable_selected_creatives()->Reserve(
            ad_selection_result.selected_campaigns.size());
        }

        ad_slot_result.mutable_freq_caps()->Add(
          std::begin(ad_selection_result.freq_caps),
          std::end(ad_selection_result.freq_caps));

        ad_slot_result.mutable_uc_freq_caps()->Add(
          std::begin(ad_selection_result.uc_freq_caps),
          std::end(ad_selection_result.uc_freq_caps));

        if(!ad_selection_result.selected_campaigns.empty())
        {
          const Creative* creative =
            ad_selection_result.selected_campaigns.begin()->creative;
          ad_slot_result.set_erid(creative->erid);

          if(creative->initial_contract)
          {
            fill_campaign_contracts_(
              *ad_slot_result.mutable_contracts(),
              creative->initial_contract);
          }
        }

        // filtering campaign/creative selection by freq caps,
        // which presents in previously processed slots
        ad_slot_context.full_freq_caps.insert(
          ad_selection_result.freq_caps.begin(),
          ad_selection_result.freq_caps.end());
        ad_slot_context.full_freq_caps.insert(
          ad_selection_result.uc_freq_caps.begin(),
          ad_selection_result.uc_freq_caps.end());

        if(!ad_selection_result.selected_campaigns.empty())
        {
          ad_slot_result.set_notice_url(request_result_params.notice_url);
          if(request_params.fill_track_pixel())
          {
            if(!(request_result_params.track_pixel_url.empty() || ad_slot.fill_track_html()))
            {
              auto* data = ad_slot_result.mutable_track_pixel_urls()->Add();
              *data = request_result_params.track_pixel_url;
            }

            if(!request_result_params.track_html_body.empty() && ad_slot.fill_track_html())
            {
              ad_slot_result.set_track_html_body(request_result_params.track_html_body);
            }

            ad_slot_result.mutable_track_pixel_urls()->Add(
              std::begin(request_result_params.add_track_pixel_urls),
              std::end(request_result_params.add_track_pixel_urls));
          }

          ad_slot_result.set_track_pixel_params(request_result_params.track_pixel_params);
          ad_slot_result.set_click_params(request_result_params.click_params);
          ad_slot_result.set_yandex_track_params(request_result_params.yandex_track_params);
          ad_slot_result.set_iurl(request_result_params.iurl);
          ad_slot_result.set_overlay_width(request_result_params.overlay_width);
          ad_slot_result.set_overlay_height(request_result_params.overlay_height);

          auto* ad_slot_result_tokens = ad_slot_result.mutable_tokens();
          ad_slot_result_tokens->Reserve(request_result_params.tokens.size());
          for(const auto& [key, value] : request_result_params.tokens)
          {
            auto* token_info = ad_slot_result_tokens->Add();
            token_info->set_name(key);
            token_info->set_value(value);
          }

          auto* ad_slot_result_ext_tokens = ad_slot_result.mutable_ext_tokens();
          ad_slot_result_ext_tokens->Reserve(request_result_params.ext_tokens.size());
          for(const auto& [key, value] : request_result_params.ext_tokens)
          {
            auto* token_info = ad_slot_result_ext_tokens->Add();
            token_info->set_name(key);
            token_info->set_value(value);
          }

          auto* ad_slot_result_native_data_tokens = ad_slot_result.mutable_native_data_tokens();
          ad_slot_result_native_data_tokens->Reserve(ad_slot.native_data_tokens().size());
          for (const auto& native_data_token : ad_slot.native_data_tokens())
          {
            auto* token_info = ad_slot_result_native_data_tokens->Add();
            const auto& token_name = native_data_token.name();
            token_info->set_name(token_name);
            const auto token_it = request_result_params.native_data_tokens.find(
              token_name);
            if (token_it != request_result_params.native_data_tokens.end())
            {
              token_info->set_value(token_it->second);
            }
          }

          // Image tokens
          auto* ad_slot_result_native_image_tokens = ad_slot_result.mutable_native_image_tokens();
          ad_slot_result_native_image_tokens->Reserve(ad_slot.native_image_tokens().size());
          for (const auto& native_image_token : ad_slot.native_image_tokens())
          {
            auto* token_image_info = ad_slot_result_native_image_tokens->Add();
            const auto& token_name = native_image_token.name();
            token_image_info->set_name(token_name);
            auto token_it = request_result_params.native_image_tokens.find(
              token_name);
            if (token_it != request_result_params.native_image_tokens.end())
            {
              token_image_info->set_value(token_it->second.value);
              token_image_info->set_width(token_it->second.width);
              token_image_info->set_height(token_it->second.height);
            }
          }

          ad_slot_result.set_request_id(GrpcAlgs::pack_request_id(
            request_result_params.request_id));
          ad_slot_result.set_mime_format(request_result_params.mime_format);

          StringSet external_visual_categories;
          StringSet external_content_categories;

          // ad_slot_min_cpm.min_pub_ecpm_system
          RevenueDecimal low_predicted_pub_ecpm_system;
          RevenueDecimal top_predicted_pub_ecpm_system;

          {
            get_bid_costs_(
              low_predicted_pub_ecpm_system,
              top_predicted_pub_ecpm_system,
              request_params,
              tag,
              ad_slot_min_cpm.min_pub_ecpm_system,
              ad_selection_result.selected_campaigns);
          }

          // eval slot pub ecpm's
          RevenueDecimal slot_pub_ecpm = RevenueDecimal::ZERO;
          RevenueDecimal slot_low_predicted_pub_ecpm_system = RevenueDecimal::ZERO;
          RevenueDecimal slot_top_predicted_pub_ecpm_system = RevenueDecimal::ZERO;

          if(!ad_selection_result.selected_campaigns.empty())
          {
            RevenueDecimal selected_campaigns_count(false, ad_selection_result.selected_campaigns.size(), 0),
            slot_pub_ecpm = RevenueDecimal::div(
              ad_slot_min_cpm.min_pub_ecpm_system,
              selected_campaigns_count,
              Generics::DDR_FLOOR);
            slot_low_predicted_pub_ecpm_system = RevenueDecimal::div(
              low_predicted_pub_ecpm_system,
              selected_campaigns_count,
              Generics::DDR_FLOOR);
            slot_top_predicted_pub_ecpm_system = RevenueDecimal::div(
              top_predicted_pub_ecpm_system,
              selected_campaigns_count,
              Generics::DDR_FLOOR);
            (void)slot_pub_ecpm;
          }

          auto& ad_slot_result_selected_creatives = *ad_slot_result.mutable_selected_creatives();
          ad_slot_result_selected_creatives.Reserve(ad_selection_result.selected_campaigns.size());
          int i = 0;
          for(auto it = ad_selection_result.selected_campaigns.begin();
              it != ad_selection_result.selected_campaigns.end();
              ++it, ++i)
          {
            assert(it->creative && it->campaign);

            it->count_impression = it->creative && !it->track_impr;

            auto& cs_result = *ad_slot_result_selected_creatives.Add();

            cs_result.set_request_id(GrpcAlgs::pack_request_id(it->request_id));
            cs_result.set_cmp_id(it->campaign->campaign_id);
            cs_result.set_campaign_group_id(it->campaign->campaign_group_id);
            cs_result.set_advertiser_id(it->campaign->advertiser->account_id);
            cs_result.set_advertiser_name(it->campaign->advertiser->legal_name);
            cs_result.set_order_set_id(it->campaign->seq_set_rotate_imps > 0 ?
              it->creative->order_set_id : 0);
            cs_result.set_revenue(GrpcAlgs::pack_decimal(
              it->track_impr ? RevenueDecimal::ZERO :
                it->campaign->imp_revenue));
            cs_result.set_ecpm(GrpcAlgs::pack_decimal(it->ecpm));

            RevenueDecimal cs_result_pub_ecpm;

            if(ad_selection_result.auction_type == AT_RANDOM)
            {
              cs_result_pub_ecpm = (i == 0 ? tag->pub_max_random_cpm :
                RevenueDecimal::ZERO);
            }
            else
            {
              //RevenueDecimal orig_ecpm_bid = it->ecpm_bid;

              // set random cost for 10% bids
              if(request_params.common_info().random() % 10 == 0 && it->ecpm_bid > slot_pub_ecpm)
              {
                RevenueDecimal c = slot_pub_ecpm + RevenueDecimal::div(
                  RevenueDecimal::mul(
                    it->ecpm_bid - slot_pub_ecpm,
                    RevenueDecimal(false, Generics::safe_rand(100), 0),
                    Generics::DMR_FLOOR),
                  RevenueDecimal(false, 100, 0));
                it->ecpm_bid = c;
              }
              else
              {
                if(it->campaign->ccg_rate_type == CR_MAXBID)
                {
                  it->ecpm_bid = slot_low_predicted_pub_ecpm_system;
                }
                else
                {
                  it->ecpm_bid = slot_top_predicted_pub_ecpm_system;
                }
              }

              cs_result_pub_ecpm = tag->site->account->currency->from_system_currency(
                it->ecpm_bid);
            }

            if(tag->cost_coef != RevenueDecimal::ZERO)
            {
              cs_result_pub_ecpm = RevenueDecimal::mul(
                cs_result_pub_ecpm,
                REVENUE_ONE + tag->cost_coef,
                Generics::DMR_FLOOR);
            }

            cs_result.set_pub_ecpm(GrpcAlgs::pack_decimal(cs_result_pub_ecpm));
            cs_result.set_ccid(it->creative->ccid);
            cs_result.set_creative_size(
              ad_selection_result.tag_size->size->protocol_name);
            cs_result.set_click_url(it->click_url);
            cs_result.set_creative_version_id(it->creative->version_id);
            cs_result.set_creative_id(it->creative->creative_id);
            cs_result.set_https_safe_flag(it->creative->https_safe_flag);

            std::string destination_url =
              it->creative->destination_url.url();

            if (destination_url.empty() && it->campaign_keyword.in())
            {
              destination_url = it->campaign_keyword->click_url;
            }

            cs_result.set_destination_url(destination_url);

            AdRequestType request_type = reduce_request_type_(
              request_params.common_info().request_type());

            auto size_it = it->creative->sizes.find(
              ad_selection_result.tag_size->size->size_id);

            if (size_it != it->creative->sizes.end())
            {
              std::uint32_t expanding = 0;
              fill_expanding(size_it->second, expanding);
              cs_result.set_expanding(expanding);
            }

            if(request_type != AR_NORMAL)
            {
              for(Creative::CategorySet::const_iterator cat_it =
                    it->creative->categories.begin();
                  cat_it != it->creative->categories.end();
                  ++cat_it)
              {
                const auto ccat_it =
                  const_config->creative_categories.find(*cat_it);
                if(ccat_it != const_config->creative_categories.end())
                {
                  const auto req_cat_it =
                    ccat_it->second.external_categories.find(request_type);
                  if(req_cat_it != ccat_it->second.external_categories.end())
                  {
                    if(ccat_it->second.cct_id == CCT_VISUAL)
                    {
                      std::copy(req_cat_it->second.begin(),
                        req_cat_it->second.end(),
                        std::inserter(external_visual_categories,
                          external_visual_categories.begin()));
                    }
                    else if(ccat_it->second.cct_id == CCT_CONTENT)
                    {
                      std::copy(req_cat_it->second.begin(),
                        req_cat_it->second.end(),
                        std::inserter(external_content_categories,
                          external_content_categories.begin()));
                    }
                  }
                }
              }
            }

            if(ad_slot_debug_info)
            {
              auto& cs_debug =
                *ad_slot_debug_info->mutable_selected_creatives()->Add();
              cs_debug.set_ecpm_bid(GrpcAlgs::pack_decimal(it->ecpm_bid));
              cs_debug.set_imp_revenue(GrpcAlgs::pack_decimal(it->campaign->imp_revenue));
              cs_debug.set_click_revenue(
                it->campaign_keyword.in() ?
                  GrpcAlgs::pack_decimal(it->actual_cpc) :
                  GrpcAlgs::pack_decimal(it->campaign->click_revenue));

              cs_debug.set_action_revenue(
                GrpcAlgs::pack_decimal(it->campaign->action_revenue));
            }
          }

          {
            auto* ad_slot_result_categories =
              ad_slot_result.mutable_external_visual_categories();
            ad_slot_result_categories->Reserve(
              external_visual_categories.size());
            for(const auto& categorie : external_visual_categories)
            {
              auto* data = ad_slot_result_categories->Add();
              *data = categorie;
            }
          }

          {
            auto* ad_slot_result_categories =
              ad_slot_result.mutable_external_content_categories();
            ad_slot_result_categories->Reserve(
              external_content_categories.size());
            for(const auto& categorie : external_content_categories)
            {
              auto* data = ad_slot_result_categories->Add();
              *data = categorie;
            }
          }

          {
            auto* ad_slot_result_tokens =
              ad_slot_result.mutable_tokens();
            ad_slot_result_tokens->Reserve(
              request_result_params.tokens.size());
            for(const auto& [key, value] : request_result_params.tokens)
            {
              auto* token_info = ad_slot_result_tokens->Add();
              token_info->set_name(key);
              token_info->set_value(value);
            }
          }

          ad_slot_result.set_track_impr(
            ad_selection_result.selected_campaigns.front().track_impr);
        }
      } // !passback

      try
      {
        CampaignManagerLogger::RequestInfo request_info;
        CampaignManagerLogger::AdRequestSelectionInfo
          ad_request_selection_info;

        CampaignManagerLogAdapter::fill_request_info(
          request_info,
          const_config,
          colocation,
          request_params.common_info(),
          request_params.context_info(),
          &request_params,
          ad_request_debug_info,
          ad_slot_context);

        CampaignManagerLogAdapter::fill_ad_request_selection_info(
          ad_request_selection_info,
          const_config,
          colocation,
          request_params.common_info(),
          request_params.context_info(),
          &request_params,
          ad_slot,
          tag,
          ad_selection_result,
          ad_slot_context,
          ad_slot_min_cpm,
          tag_sizes,
          false);

        campaign_manager_logger_->process_ad_request(
          request_info, ad_request_selection_info);
      }
      catch (const CampaignManagerLogger::Exception& e)
      {
        logger_->sstream(Logging::Logger::EMERGENCY,
          Aspect::CAMPAIGN_MANAGER,
          "ADS-IMPL-177") << FUN << ": "
          "eh::Exception caught while logging request: " << e.what();
      }
    }

    bool
    CampaignManagerImpl::check_request_constraints(
      const AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
      std::string& referer_hostname_str,
      std::string& original_url_str)
      /*throw(eh::Exception)*/
    {
      try
      {
        HTTP::BrowserAddress addr(
          String::SubString(request_params.common_info.referer.in()));

        if (!String::case_change<String::Uniform>(
              addr.host(),
              referer_hostname_str))
        {
          referer_hostname_str.clear();
        }
      }
      catch(const eh::Exception&)
      {}

      if (!String::case_change<String::Uniform>(
        String::SubString(request_params.common_info.original_url.in()),
        original_url_str))
      {
        original_url_str = "";
      }

      return true;
    }

    bool
    CampaignManagerImpl::check_request_constraints(
      const Proto::RequestParams& request_params,
      std::string& referer_hostname_str,
      std::string& original_url_str)
    {
      try
      {
        HTTP::BrowserAddress addr(
          String::SubString(request_params.common_info().referer()));

        if (!String::case_change<String::Uniform>(
              addr.host(),
              referer_hostname_str))
        {
          referer_hostname_str.clear();
        }
      }
      catch(const eh::Exception&)
      {}

      if (!String::case_change<String::Uniform>(
        String::SubString(request_params.common_info().original_url()),
        original_url_str))
      {
        original_url_str = "";
      }

      return true;
    }

    CORBA::Boolean
    CampaignManagerImpl::get_campaign_creative_by_ccid(
      const ::AdServer::CampaignSvcs::CampaignManager::CreativeParams&
        params,
      CORBA::String_out creative_body)
      /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException)*/
    {
      static const char* FUN = "CampaignManagerImpl::get_campaign_creative_by_ccid()";

      try
      {
        creative_body = (const char*)0;

        CORBA::Boolean result = get_campaign_creative_by_ccid_impl(
          params,
          creative_body);

        if(creative_body == 0)
        {
          creative_body << String::SubString("");
        }

        return result;
      }
      catch (const eh::Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught. : " << e.what();

        CORBACommons::throw_desc<
          CampaignSvcs::CampaignManager::ImplementationException>(
            ostr.str());
      }
      return 0; // never reach
    }

    CampaignManagerImpl::GetCampaignCreativeByCcidResponsePtr
    CampaignManagerImpl::get_campaign_creative_by_ccid(
      GetCampaignCreativeByCcidRequestPtr&& request)
    {
      const auto id_request_grpc = request->id_request_grpc();
      try
      {
        // getting configuration
        CampaignConfig_var config = configuration(false);
        if(!config) // can't do anything
        {
          throw NotReady("Campaign configuration isn't loaded");
        }

        const auto& params = request->params();

        // selecting creative
        const Campaign* campaign = nullptr;
        const Creative* creative = nullptr;
        const Tag* tag = nullptr;

        for(const auto& [_, value] : config->campaigns)
        {
          const Campaign* cmp = value;
          if((creative = CampaignOps(cmp).search_ccid(params.ccid())))
          {
            campaign = cmp;
            break;
          }
        }

        TagMap::const_iterator tag_it = config->tags.find(params.tag_id());
        if(tag_it != config->tags.end())
        {
          tag = tag_it->second;
        }

        auto response = create_grpc_response<Proto::GetCampaignCreativeByCcidResponse>(
          id_request_grpc);
        auto* info_response = response->mutable_info();

        if (!creative)
        {
          info_response->set_return_value(false);
          return response;
        }

        const Tag::Size* tag_size = match_creative_by_size_(tag, creative);

        if(!tag_size)
        {
          info_response->set_return_value(false);
          return response;
        }

        std::string creative_body;
        const bool result = instantiate_creative_preview(
          params,
          config,
          campaign,
          creative,
          tag,
          *tag_size,
          creative_body);
        info_response->set_return_value(result);
        if (!result)
        {
          return response;
        }

        info_response->set_creative_body(std::move(creative_body));
        return response;
      }
      catch (const NotReady& exc)
      {
        Stream::Error stream;
        stream << FNS
               << "Caught CampaignManager::NotReady: "
               << exc.what();
        auto response = create_grpc_error_response<Proto::GetCampaignCreativeByCcidResponse>(
          Proto::Error_Type::Error_Type_NotReady,
          stream.str(),
          id_request_grpc);
        return response;
      }
      catch (const eh::Exception& exc)
      {
        Stream::Error stream;
        stream << FNS
               << ": "
               << exc.what();
        auto response = create_grpc_error_response<Proto::GetCampaignCreativeByCcidResponse>(
          Proto::Error_Type::Error_Type_Implementation,
          stream.str(),
          id_request_grpc);
        return response;
      }
      catch (...)
      {
        Stream::Error stream;
        stream << FNS
               << ": Unknown error";
        auto response = create_grpc_error_response<Proto::GetCampaignCreativeByCcidResponse>(
          Proto::Error_Type::Error_Type_Implementation,
          stream.str(),
          id_request_grpc);
        return response;
      }
    }

    CORBA::Boolean
    CampaignManagerImpl::get_campaign_creative_by_ccid_impl(
      const ::AdServer::CampaignSvcs::CampaignManager::CreativeParams&
        params,
      CORBA::String_out creative_body)
      /*throw(eh::Exception)*/
    {
      // getting configuration
      CampaignConfig_var config = configuration();
      if(config.in() == 0) // can't do anything
      {
        return 0;
      }

      // selecting creative
      const Campaign* campaign = 0;
      const Creative* creative = 0;
      const Tag* tag = 0;

      for(CampaignConfig::CampaignMap::iterator it = config->campaigns.begin();
          it != config->campaigns.end(); ++it)
      {
        const Campaign* cmp = it->second;
        if((creative = CampaignOps(cmp).search_ccid(params.ccid)))
        {
          campaign = cmp;
          break;
        }
      }

      TagMap::const_iterator tag_it = config->tags.find(params.tag_id);
      if(tag_it != config->tags.end())
      {
        tag = tag_it->second;
      }

      if (creative == 0) return 0;

      const Tag::Size* tag_size = match_creative_by_size_(tag, creative);

      if(!tag_size)
      {
        return false;
      }

      return instantiate_creative_preview(
        params, config, campaign, creative, tag, *tag_size, creative_body) ? 1 : 0;
    }

    void CampaignManagerImpl::consider_passback(
      const AdServer::CampaignSvcs::CampaignManager::PassbackInfo& in)
      /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException)*/
    {
      static const char* FUN = "CampaignManagerImpl::consider_passback()";

      try
      {
        campaign_manager_logger_->process_passback(
          CampaignManagerLogger::PassbackInfo(
            CorbaAlgs::unpack_request_id(in.request_id),
            CorbaAlgs::unpack_time(in.time),
            in.user_id_hash_mod.defined ?
              CampaignManagerLogger::UserIdHashMod(
                in.user_id_hash_mod.value) :
              CampaignManagerLogger::UserIdHashMod()));
      }
      catch(const eh::Exception& ex)
      {
        std::ostringstream ostr;
        ostr << FUN << ": eh::Exception caught: " << ex.what();

        CORBACommons::throw_desc<AdServer::CampaignSvcs::
          CampaignManager::ImplementationException>(ostr.str());
      }
    }

    CampaignManagerImpl::ConsiderPassbackResponsePtr
    CampaignManagerImpl::consider_passback(
      ConsiderPassbackRequestPtr&& request)
    {
      const auto id_request_grpc = request->id_request_grpc();
      try
      {
        const auto& pass_info = request->pass_info();

        campaign_manager_logger_->process_passback(
          CampaignManagerLogger::PassbackInfo(
            GrpcAlgs::unpack_request_id(pass_info.request_id()),
            GrpcAlgs::unpack_time(pass_info.time()),
            pass_info.user_id_hash_mod().defined() ?
              CampaignManagerLogger::UserIdHashMod(
                pass_info.user_id_hash_mod().value()) :
              CampaignManagerLogger::UserIdHashMod{}));

        auto response = create_grpc_response<Proto::ConsiderPassbackResponse>(
          id_request_grpc);
        response->mutable_info();
        return response;
      }
      catch (const NotReady& exc)
      {
        Stream::Error stream;
        stream << FNS
               << "Caught CampaignManager::NotReady: "
               << exc.what();
        auto response = create_grpc_error_response<Proto::ConsiderPassbackResponse>(
          Proto::Error_Type::Error_Type_NotReady,
          stream.str(),
          id_request_grpc);
        return response;
      }
      catch (const eh::Exception& exc)
      {
        Stream::Error stream;
        stream << FNS
               << ": "
               << exc.what();
        auto response = create_grpc_error_response<Proto::ConsiderPassbackResponse>(
          Proto::Error_Type::Error_Type_Implementation,
          stream.str(),
          id_request_grpc);
        return response;
      }
      catch (...)
      {
        Stream::Error stream;
        stream << FNS
               << ": Unknown error";
        auto response = create_grpc_error_response<Proto::ConsiderPassbackResponse>(
          Proto::Error_Type::Error_Type_Implementation,
          stream.str(),
          id_request_grpc);
        return response;
      }
    }

    void CampaignManagerImpl::consider_passback_track(
      const AdServer::CampaignSvcs::CampaignManager::PassbackTrackInfo& in)
      /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException,
        AdServer::CampaignSvcs::CampaignManager::NotReady)*/
    {
      static const char* FUN = "CampaignManagerImpl::consider_passback_track()";

      CampaignConfig_var config = configuration();
      if (!config)
      {
        throw AdServer::CampaignSvcs::CampaignManager::NotReady(
          "Campaign configuration isn't loaded");
      }

      TagMap::const_iterator tag_it = config->tags.find(in.tag_id);
      CampaignConfig::ColocationMap::const_iterator colo_it =
        config->colocations.find(in.colo_id);

      if(tag_it != config->tags.end() && colo_it != config->colocations.end())
      {
        const Tag::TagPricing* tag_pricing =
          tag_it->second->select_country_tag_pricing(in.country);

        try
        {
          CampaignManagerLogger::PassbackTrackInfo passback_track_info;
          passback_track_info.time = CorbaAlgs::unpack_time(in.time);
          passback_track_info.country = in.country;
          passback_track_info.currency_exchange_id = config->currency_exchange_id;
          passback_track_info.colo_id = in.colo_id;
          passback_track_info.colo_rate_id = colo_it->second->colo_rate_id;
          passback_track_info.tag_id = in.tag_id;
          passback_track_info.site_rate_id = tag_pricing ? tag_pricing->site_rate_id : 0;
          passback_track_info.user_status = static_cast<UserStatus>(in.user_status);
          passback_track_info.log_as_test =
            colo_it->second->is_test() || tag_it->second->is_test();
          campaign_manager_logger_->process_passback_track(passback_track_info);
        }
        catch(const eh::Exception& ex)
        {
          std::ostringstream ostr;
          ostr << FUN << ": eh::Exception caught: " << ex.what();
          CORBACommons::throw_desc<AdServer::CampaignSvcs::
            CampaignManager::ImplementationException>(ostr.str());
        }
      }
    }

    CampaignManagerImpl::ConsiderPassbackTrackResponsePtr
    CampaignManagerImpl::consider_passback_track(
      ConsiderPassbackTrackRequestPtr&& request)
    {
      const auto id_request_grpc = request->id_request_grpc();
      try
      {
        CampaignConfig_var config = configuration(false);
        if (!config)
        {
          throw NotReady("Campaign configuration isn't loaded");
        }

        const auto& pass_info = request->pass_info();
        const auto tag_it = config->tags.find(pass_info.tag_id());
        const auto colo_it = config->colocations.find(pass_info.colo_id());

        if(tag_it != config->tags.end() && colo_it != config->colocations.end())
        {
          const auto* tag_pricing =
            tag_it->second->select_country_tag_pricing(pass_info.country().c_str());

          try
          {
            CampaignManagerLogger::PassbackTrackInfo passback_track_info;
            passback_track_info.time = GrpcAlgs::unpack_time(pass_info.time());
            passback_track_info.country = pass_info.country();
            passback_track_info.currency_exchange_id = config->currency_exchange_id;
            passback_track_info.colo_id = pass_info.colo_id();
            passback_track_info.colo_rate_id = colo_it->second->colo_rate_id;
            passback_track_info.tag_id = pass_info.tag_id();
            passback_track_info.site_rate_id = tag_pricing ? tag_pricing->site_rate_id : 0;
            passback_track_info.user_status = static_cast<UserStatus>(pass_info.user_status());
            passback_track_info.log_as_test =
              colo_it->second->is_test() || tag_it->second->is_test();
            campaign_manager_logger_->process_passback_track(passback_track_info);
          }
          catch(const eh::Exception& ex)
          {
            std::ostringstream ostr;
            ostr << FNS
                 << ": eh::Exception caught: "
                 << ex.what();
            throw Exception(ostr.str());
          }
        }

        auto response = create_grpc_response<Proto::ConsiderPassbackTrackResponse>(
          id_request_grpc);
        response->mutable_info();
        return response;
      }
      catch (const NotReady& exc)
      {
        Stream::Error stream;
        stream << FNS
               << "Caught CampaignManager::NotReady: "
               << exc.what();
        auto response = create_grpc_error_response<Proto::ConsiderPassbackTrackResponse>(
          Proto::Error_Type::Error_Type_NotReady,
          stream.str(),
          id_request_grpc);
        return response;
      }
      catch (const eh::Exception& exc)
      {
        Stream::Error stream;
        stream << FNS
               << ": "
               << exc.what();
        auto response = create_grpc_error_response<Proto::ConsiderPassbackTrackResponse>(
          Proto::Error_Type::Error_Type_Implementation,
          stream.str(),
          id_request_grpc);
        return response;
      }
      catch (...)
      {
        Stream::Error stream;
        stream << FNS
               << ": Unknown error";
        auto response = create_grpc_error_response<Proto::ConsiderPassbackTrackResponse>(
          Proto::Error_Type::Error_Type_Implementation,
          stream.str(),
          id_request_grpc);
        return response;
      }
    }

    CORBA::Boolean
    CampaignManagerImpl::get_click_url(
      const ::AdServer::CampaignSvcs::CampaignManager::ClickInfo& click_info,
      ::AdServer::CampaignSvcs::CampaignManager::ClickResultInfo_out click_result_info)
      /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException,
        AdServer::CampaignSvcs::CampaignManager::NotReady)*/
    {
      static const char* FUN = "CampaignManagerImpl::get_click_url()";

      CORBA::Boolean result = 0;
      const Generics::Time click_time = CorbaAlgs::unpack_time(click_info.time);
      const Generics::Time bid_time = CorbaAlgs::unpack_time(click_info.bid_time);

      try
      {
        click_result_info = new AdServer::CampaignSvcs::
          CampaignManager::ClickResultInfo();

        CampaignConfig_var config = configuration(true);

        std::string redirect;
        const CampaignKeywordBase* ckw = 0;

        String::StringManip::trim(
          String::SubString(click_info.relocate.in()), redirect);

        OptionValue click_url;

        if(redirect.empty())
        {
          CCGKeywordPostClickInfoMap::const_iterator kw_it =
            config->ccg_keyword_click_info_map.find(click_info.ccg_keyword_id);

          if(kw_it != config->ccg_keyword_click_info_map.end())
          {
            ckw = &(kw_it->second);
            click_url.value = ckw->click_url;
          }
        }

        const Colocation* colocation = 0;
        const Tag* tag = 0;
        Tag::ConstSize_var tag_size;
        const Creative* creative = 0;

        {
          CampaignConfig::ColocationMap::const_iterator colo_it =
            config->colocations.find(click_info.colo_id);
          if(colo_it != config->colocations.end())
          {
            colocation = colo_it->second;
          }
        }

        {
          TagMap::const_iterator tag_it = config->tags.find(click_info.tag_id);
          if(tag_it != config->tags.end())
          {
            tag = tag_it->second.in();
            Tag::SizeMap::const_iterator tag_size_it =
              tag->sizes.find(click_info.tag_size_id);
            if(tag_size_it != tag->sizes.end())
            {
              tag_size = tag_size_it->second;
            }
          }
        }

        click_result_info->advertiser_id = 0;

        if (click_info.ccid)
        {
          const CcidMap::const_iterator creative_it =
            config->campaign_creatives.find(click_info.ccid);

          if(creative_it != config->campaign_creatives.end())
          {
            creative = creative_it->second.in();
          }
        }
        else if (click_info.creative_id)
        {
          // Get creative by click_info.creative_id if click_info.ccid not defined.
          const CreativeMap::const_iterator creative_it =
            config->creatives.find(click_info.creative_id);

          if(creative_it != config->creatives.end())
          {
            creative = creative_it->second.in();
          }
        }

        if (creative)
        {
          if(redirect.empty())
          {
            if(click_url.value.empty())
            {
              click_url.value = creative->click_url.value;
            }

            click_url.option_id = creative->click_url.option_id;
          }

          // campaign_group_id is campaign_id in OIX terms
          click_result_info->campaign_id = creative->campaign->campaign_group_id;

          if (creative->campaign->advertiser)
          {
            click_result_info->advertiser_id = creative->campaign->advertiser->account_id;
          }
        }
        else
        {
          click_result_info->campaign_id = 0;
        }

        if(redirect.empty())
        {
          if(!click_url.value.empty())
          {
            try
            {
              TokenValueMap tokens;

              for(CORBA::ULong tok_i = 0; tok_i < click_info.tokens.length(); ++tok_i)
              {
                tokens[click_info.tokens[tok_i].name.in()] =
                  click_info.tokens[tok_i].value.in();
              }

              instantiate_click_url(
                config,
                click_url,
                redirect,
                colocation ? &colocation->colo_id : 0,
                tag,
                tag_size,
                creative,
                ckw,
                tokens);
            }
            catch(const eh::Exception& ex)
            {
              logger_->sstream(Logging::Logger::NOTICE,
                Aspect::TRAFFICKING_PROBLEM) <<
                "Can't instantiate click url '" <<
                click_url.value <<
                "' (ccid = " << click_info.ccid <<
                ", creative_id = " << click_info.creative_id <<
                ", ccg_keyword_id = " << click_info.ccg_keyword_id <<
                ", tag_id = " << (tag ? tag->tag_id : 0) <<
                "): " << ex.what();

              throw;
            }
          }
        }

        if(!redirect.empty())
        {
          try
          {
            HTTP::BrowserAddress normalized_redirect(redirect);
            redirect = normalized_redirect.url();
          }
          catch(const eh::Exception& ex)
          {
            logger_->sstream(Logging::Logger::NOTICE,
              Aspect::TRAFFICKING_PROBLEM) <<
              "Invalid click url(after tokens substitution): '" <<
              redirect <<
              "' (ccid = " << click_info.ccid <<
              ", creative_id = " << click_info.creative_id <<
              ", ccg_keyword_id = " << click_info.ccg_keyword_id <<
              "): " << ex.what();

            throw;
          }

          click_result_info->url << redirect;
          result = 1;
        }

        AdServer::Commons::RequestId request_id =
          CorbaAlgs::unpack_request_id(click_info.request_id);

        if(!request_id.is_null()  && click_info.log_click)
        {
          try
          {
            campaign_manager_logger_->process_click(
              CampaignManagerLogger::ClickInfo(
                click_time,
                CorbaAlgs::unpack_request_id(click_info.request_id),
                click_info.user_id_hash_mod.defined ?
                  CampaignManagerLogger::UserIdHashMod(
                    click_info.user_id_hash_mod.value) :
                  CampaignManagerLogger::UserIdHashMod(),
                click_info.referer));
          }
          catch (const CampaignManagerLogger::Exception& e)
          {
            logger_->sstream(Logging::Logger::EMERGENCY,
              Aspect::CAMPAIGN_MANAGER,
              "ADS-IMPL-177") << FUN <<
              ": eh::Exception caught while logging request "
              "for request_id = '" <<
              CorbaAlgs::unpack_request_id(click_info.request_id) <<
              "': " << e.what();
          }
          catch (const AdServer::Commons::RequestId::InvalidArgument& ex)
          {
            if (logger_->log_level() >= Logging::Logger::TRACE)
            {
              logger_->sstream(Logging::Logger::TRACE,
                Aspect::CAMPAIGN_MANAGER) << FUN <<
                ": RequestId::InvalidArgument caught while logging request "
                "for request_id = '" <<
                 CorbaAlgs::unpack_request_id(click_info.request_id) <<
                "': " << ex.what();
            }
          }
        }

        {
          // confirm click amount if other operations success
          // otherwise frontend will call other CampaignManager
          ConfirmCreativeAmountArray creatives;
          creatives.push_back(
            ConfirmCreativeAmount(
              click_info.ccid,
              CorbaAlgs::unpack_decimal<RevenueDecimal>(click_info.ctr)));
          confirm_amounts_(config, bid_time, creatives, CR_CPC);
        }

        AdServer::Commons::UserId match_user_id = CorbaAlgs::unpack_user_id(
          click_info.match_user_id);
        AdServer::Commons::UserId cookie_user_id = CorbaAlgs::unpack_user_id(
          click_info.cookie_user_id);

        if(kafka_producer_ && (!match_user_id.is_null() || !cookie_user_id.is_null()))
        {
          const CORBACommons::UserIdInfo* produce_user_ids[] = {0, 0};
          unsigned long produce_user_id_i = 0;
          if(!match_user_id.is_null())
          {
            produce_user_ids[produce_user_id_i++] = &click_info.match_user_id;
          }

          if(!cookie_user_id.is_null())
          {
            produce_user_ids[produce_user_id_i++] = &click_info.cookie_user_id;
          }

          for(unsigned long i = 0; i < produce_user_id_i; ++i)
          {
            char campaign_id_str[40];
            size_t campaign_id_str_size = String::StringManip::int_to_str(
              click_result_info ? click_result_info->campaign_id : 0,
              campaign_id_str,
              sizeof(campaign_id_str));
            produce_ads_space_message_impl_(
              CorbaAlgs::unpack_time(click_info.time),
              CorbaAlgs::unpack_user_id(*produce_user_ids[i]),
              0, // tag_id
              std::string("click-").append(campaign_id_str, campaign_id_str_size), // referer
              0, // ad_slots
              0, // publisher_account_ids
              String::SubString(),
              AdServer::CampaignSvcs::CampaignManager::GeoInfoSeq(),
              String::SubString(),
              RevenueDecimal::ZERO,
              String::SubString());
          }
        }
      }
      catch (const eh::Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << e.what();

        CORBACommons::throw_desc<
          CampaignSvcs::CampaignManager::ImplementationException>(
            ostr.str());
      }

      return result;
    }

    CampaignManagerImpl::GetClickUrlResponsePtr
    CampaignManagerImpl::get_click_url(
      GetClickUrlRequestPtr&& request)
    {
      static const char* FUN = "CampaignManagerImpl::get_click_url()";

      const auto id_request_grpc = request->id_request_grpc();
      try
      {
        bool result = false;

        const auto& click_info = request->click_info();
        const Generics::Time click_time = GrpcAlgs::unpack_time(click_info.time());
        const Generics::Time bid_time = GrpcAlgs::unpack_time(click_info.bid_time());

        auto response = create_grpc_response<Proto::GetClickUrlResponse>(
          id_request_grpc);
        auto* info_response = response->mutable_info();
        auto* click_result_info_response = info_response->mutable_click_result_info();

        CampaignConfig_var config = configuration(false);
        if (!config)
        {
          throw NotReady("Campaign configuration isn't loaded");
        }

        std::string redirect;
        const CampaignKeywordBase* ckw = nullptr;

        String::StringManip::trim(
          String::SubString(click_info.relocate()), redirect);

        OptionValue click_url;

        if(redirect.empty())
        {
          const auto kw_it = config->ccg_keyword_click_info_map.find(
            click_info.ccg_keyword_id());

          if(kw_it != config->ccg_keyword_click_info_map.end())
          {
            ckw = &(kw_it->second);
            click_url.value = ckw->click_url;
          }
        }

        const Colocation* colocation = nullptr;
        const Tag* tag = nullptr;
        Tag::ConstSize_var tag_size;
        const Creative* creative = nullptr;

        {
          const auto colo_it = config->colocations.find(
            click_info.colo_id());
          if(colo_it != config->colocations.end())
          {
            colocation = colo_it->second;
          }
        }

        {
          const auto tag_it = config->tags.find(click_info.tag_id());
          if(tag_it != config->tags.end())
          {
            tag = tag_it->second.in();
            const auto tag_size_it = tag->sizes.find(
              click_info.tag_size_id());
            if(tag_size_it != tag->sizes.end())
            {
              tag_size = tag_size_it->second;
            }
          }
        }

        click_result_info_response->set_advertiser_id(0);

        if (click_info.ccid())
        {
          const auto creative_it = config->campaign_creatives.find(
            click_info.ccid());

          if(creative_it != config->campaign_creatives.end())
          {
            creative = creative_it->second.in();
          }
        }
        else if (click_info.creative_id())
        {
          // Get creative by click_info.creative_id if click_info.ccid not defined.
          const auto creative_it = config->creatives.find(
            click_info.creative_id());
          if(creative_it != config->creatives.end())
          {
            creative = creative_it->second.in();
          }
        }

        if (creative)
        {
          if(redirect.empty())
          {
            if(click_url.value.empty())
            {
              click_url.value = creative->click_url.value;
            }

            click_url.option_id = creative->click_url.option_id;
          }

          // campaign_group_id is campaign_id in OIX terms
          click_result_info_response->set_campaign_id(
            creative->campaign->campaign_group_id);

          if (creative->campaign->advertiser)
          {
            click_result_info_response->set_advertiser_id(
              creative->campaign->advertiser->account_id);
          }
        }
        else
        {
          click_result_info_response->set_campaign_id(0);
        }

        if(redirect.empty())
        {
          if(!click_url.value.empty())
          {
            try
            {
              TokenValueMap tokens;
              const auto& click_info_tokens = click_info.tokens();
              for(const auto& click_info_token : click_info_tokens)
              {
                tokens[click_info_token.name()] = click_info_token.value();
              }

              instantiate_click_url(
                config,
                click_url,
                redirect,
                colocation ? &colocation->colo_id : 0,
                tag,
                tag_size,
                creative,
                ckw,
                tokens);
            }
            catch(const eh::Exception& ex)
            {
              logger_->sstream(Logging::Logger::NOTICE,
                Aspect::TRAFFICKING_PROBLEM) <<
                "Can't instantiate click url '" <<
                click_url.value <<
                "' (ccid = " << click_info.ccid() <<
                ", creative_id = " << click_info.creative_id() <<
                ", ccg_keyword_id = " << click_info.ccg_keyword_id() <<
                ", tag_id = " << (tag ? tag->tag_id : 0) <<
                "): " << ex.what();

              throw;
            }
          }
        }

        if(!redirect.empty())
        {
          try
          {
            HTTP::BrowserAddress normalized_redirect(redirect);
            redirect = normalized_redirect.url();
          }
          catch(const eh::Exception& ex)
          {
            logger_->sstream(Logging::Logger::NOTICE,
              Aspect::TRAFFICKING_PROBLEM) <<
              "Invalid click url(after tokens substitution): '" <<
              redirect <<
              "' (ccid = " << click_info.ccid() <<
              ", creative_id = " << click_info.creative_id() <<
              ", ccg_keyword_id = " << click_info.ccg_keyword_id() <<
              "): " << ex.what();

            throw;
          }

          click_result_info_response->set_url(redirect);
          result = true;
        }

        auto request_id = GrpcAlgs::unpack_request_id(click_info.request_id());

        if(!request_id.is_null() && click_info.log_click())
        {
          try
          {
            campaign_manager_logger_->process_click(
              CampaignManagerLogger::ClickInfo(
                click_time,
                GrpcAlgs::unpack_request_id(click_info.request_id()),
                click_info.user_id_hash_mod().defined() ?
                  CampaignManagerLogger::UserIdHashMod(
                    click_info.user_id_hash_mod().value()) :
                  CampaignManagerLogger::UserIdHashMod(),
                click_info.referer().c_str()));
          }
          catch (const CampaignManagerLogger::Exception& e)
          {
            logger_->sstream(Logging::Logger::EMERGENCY,
              Aspect::CAMPAIGN_MANAGER,
              "ADS-IMPL-177") << FUN <<
              ": eh::Exception caught while logging request "
              "for request_id = '" <<
              GrpcAlgs::unpack_request_id(click_info.request_id()) <<
              "': " << e.what();
          }
          catch (const AdServer::Commons::RequestId::InvalidArgument& ex)
          {
            if (logger_->log_level() >= Logging::Logger::TRACE)
            {
              logger_->sstream(Logging::Logger::TRACE,
                Aspect::CAMPAIGN_MANAGER) << FUN <<
                ": RequestId::InvalidArgument caught while logging request "
                "for request_id = '" <<
                 GrpcAlgs::unpack_request_id(click_info.request_id()) <<
                "': " << ex.what();
            }
          }
        }

        {
          // confirm click amount if other operations success
          // otherwise frontend will call other CampaignManager
          ConfirmCreativeAmountArray creatives;
          creatives.push_back(
            ConfirmCreativeAmount(
              click_info.ccid(),
              GrpcAlgs::unpack_decimal<RevenueDecimal>(click_info.ctr())));
          confirm_amounts_(config, bid_time, creatives, CR_CPC);
        }

        AdServer::Commons::UserId match_user_id = GrpcAlgs::unpack_user_id(
          click_info.match_user_id());
        AdServer::Commons::UserId cookie_user_id = GrpcAlgs::unpack_user_id(
          click_info.cookie_user_id());

        if(kafka_producer_ && (!match_user_id.is_null() || !cookie_user_id.is_null()))
        {
          const std::string* produce_user_ids[] = {nullptr, nullptr};
          std::uint32_t produce_user_id_i = 0;
          if(!match_user_id.is_null())
          {
            produce_user_ids[produce_user_id_i++] = &click_info.match_user_id();
          }

          if(!cookie_user_id.is_null())
          {
            produce_user_ids[produce_user_id_i++] = &click_info.cookie_user_id();
          }

          for(std::uint32_t i = 0; i < produce_user_id_i; ++i)
          {
            char campaign_id_str[40];
            size_t campaign_id_str_size = String::StringManip::int_to_str(
              click_result_info_response->campaign_id(),
              campaign_id_str,
              sizeof(campaign_id_str));
            produce_ads_space_message_impl_(
              GrpcAlgs::unpack_time(click_info.time()),
              GrpcAlgs::unpack_user_id(*produce_user_ids[i]),
              0, // tag_id
              std::string("click-").append(campaign_id_str, campaign_id_str_size), // referer
              nullptr, // ad_slots
              nullptr, // publisher_account_ids
              String::SubString{},
              google::protobuf::RepeatedPtrField<Proto::GeoInfo>{},
              String::SubString{},
              RevenueDecimal::ZERO,
              String::SubString{});
          }
        }

        info_response->set_return_value(result);
        return response;
      }
      catch (const NotReady& exc)
      {
        Stream::Error stream;
        stream << FUN
               << "Caught CampaignManager::NotReady: "
               << exc.what();
        auto response = create_grpc_error_response<Proto::GetClickUrlResponse>(
          Proto::Error_Type::Error_Type_NotReady,
          stream.str(),
          id_request_grpc);
        return response;
      }
      catch (const eh::Exception& exc)
      {
        Stream::Error stream;
        stream << FUN
               << ": "
               << exc.what();
        auto response = create_grpc_error_response<Proto::GetClickUrlResponse>(
          Proto::Error_Type::Error_Type_Implementation,
          stream.str(),
          id_request_grpc);
        return response;
      }
      catch (...)
      {
        Stream::Error stream;
        stream << FUN
               << ": Unknown error";
        auto response = create_grpc_error_response<Proto::GetClickUrlResponse>(
          Proto::Error_Type::Error_Type_Implementation,
          stream.str(),
          id_request_grpc);
        return response;
      }
    }

    void
    CampaignManagerImpl::log_incoming_request(
      const AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
      const AdServer::CampaignSvcs::CampaignManager::AdSlotInfo& ad_slot)
      /*throw(eh::Exception)*/
    {
      std::ostringstream ostr;
      ostr << "Incoming request: " << std::endl <<
        "  tag_id: " << ad_slot.tag_id << std::endl <<
        "  referer_hostname: " << request_params.common_info.referer << std::endl <<
        "  channels: " << std::endl;

      CorbaAlgs::print_sequence(ostr, request_params.channels);
      ostr << "  full freq caps: ";
      CorbaAlgs::print_sequence(ostr, request_params.full_freq_caps);

      logger_->log(ostr.str(),
        TraceLevel::MIDDLE,
        Aspect::CAMPAIGN_MANAGER);
    }

    void
    CampaignManagerImpl::log_incoming_request(
      const Proto::RequestParams& request_params,
      const Proto::AdSlotInfo& ad_slot)
    {
      std::ostringstream ostr;
      ostr << "Incoming request: " << std::endl <<
        "  tag_id: " << ad_slot.tag_id() << std::endl <<
        "  referer_hostname: " << request_params.common_info().referer() << std::endl <<
        "  channels: " << std::endl;

      GrpcAlgs::print_repeated(ostr, ",", request_params.channels());
      ostr << "  full freq caps: ";
      GrpcAlgs::print_repeated(ostr, ",", request_params.full_freq_caps());

      logger_->log(ostr.str(),
        TraceLevel::MIDDLE,
        Aspect::CAMPAIGN_MANAGER);
    }

    const Tag::Size*
    CampaignManagerImpl::match_creative_by_size_(
      const Tag* tag,
      const Creative* creative)
      noexcept
    {
      for(Tag::SizeMap::const_iterator tag_size_it =
            tag->sizes.begin();
          tag_size_it != tag->sizes.end(); ++tag_size_it)
      {
        if(!creative->campaign->is_text() ||
           tag_size_it->second->max_text_creatives > 0)
        {
          Creative::SizeMap::const_iterator crs_it =
            creative->sizes.find(tag_size_it->first);
          if(crs_it != creative->sizes.end())
          {
            return tag_size_it->second;
          }
        }
      }

      return 0;
    }

    bool
    CampaignManagerImpl::preview_ccid_(
      const CampaignConfig* config,
      const Colocation* colocation,
      const Tag* tag,
      const AdServer::CampaignSvcs::CampaignManager::RequestParams&
        request_params,
      const AdServer::CampaignSvcs::CampaignManager::AdSlotInfo& ad_slot,
      AdSlotContext& ad_slot_context,
      RequestResultParams& request_result_params,
      AdSelectionResult& select_result,
      CORBA::String_out creative_body)
      /*throw(eh::Exception)*/
    {
      const CcidMap::const_iterator ccid_it =
        config->campaign_creatives.find(request_params.preview_ccid);

      if(ccid_it == config->campaign_creatives.end())
      {
        return false;
      }

      const Tag::Size* tag_size = match_creative_by_size_(
        tag, ccid_it->second);

      if(!tag_size)
      {
        return false;
      }

      const Creative* creative = ccid_it->second;

      if (!creative->campaign->advertiser)
      {
        return false;
      }

      select_result.tag = tag;
      select_result.tag_pricing = tag->select_tag_pricing(
        creative->campaign->country.c_str(),
        creative->campaign->ccg_type,
        creative->campaign->ccg_rate_type);
      select_result.tag_size = tag_size;

      CampaignSelectionData select_params;
      select_params.campaign = creative->campaign;
      select_params.creative = creative;
      select_params.selection_done = false; // no selection

      // use first intersect size
      select_result.selected_campaigns.push_back(select_params);

      CreativeParamsList creative_params_list;
      InstantiateParams inst_params(
        request_params.common_info.user_id,
        request_params.context_info.enabled_notice);
      inst_params.ext_tag_id = String::SubString(ad_slot.ext_tag_id);

      inst_params.video_width = ad_slot.video_width;
      inst_params.video_height = ad_slot.video_height;
      inst_params.publisher_site_id = request_params.publisher_site_id;
      inst_params.publisher_account_id = ad_slot_context.publisher_account_id;

      instantiate_creative_(
        request_params.common_info,
        config,
        colocation,
        inst_params,
        ad_slot.format,
        select_result,
        request_result_params,
        creative_params_list,
        creative_body,
        ad_slot_context,
        &request_params.exclude_pubpixel_accounts
        );

      if (!select_result.selected_campaigns.empty() &&
          !creative_params_list.empty())
      {
        select_result.selected_campaigns.front().click_url =
          creative_params_list.front().click_url;
      }

      return true;
    }

    bool CampaignManagerImpl::preview_ccid_(
      const CampaignConfig* config,
      const Colocation* colocation,
      const Tag* tag,
      const Proto::RequestParams& request_params,
      const Proto::AdSlotInfo& ad_slot,
      AdSlotContext& ad_slot_context,
      RequestResultParams& request_result_params,
      AdSelectionResult& select_result,
      std::string& creative_body)
    {
      const auto ccid_it = config->campaign_creatives.find(
        request_params.preview_ccid());
      if(ccid_it == config->campaign_creatives.end())
      {
        return false;
      }

      const Tag::Size* tag_size = match_creative_by_size_(
        tag, ccid_it->second);
      if(!tag_size)
      {
        return false;
      }

      const Creative* creative = ccid_it->second;
      if (!creative->campaign->advertiser)
      {
        return false;
      }

      select_result.tag = tag;
      select_result.tag_pricing = tag->select_tag_pricing(
        creative->campaign->country.c_str(),
        creative->campaign->ccg_type,
        creative->campaign->ccg_rate_type);
      select_result.tag_size = tag_size;

      CampaignSelectionData select_params;
      select_params.campaign = creative->campaign;
      select_params.creative = creative;
      select_params.selection_done = false; // no selection

      // use first intersect size
      select_result.selected_campaigns.push_back(select_params);

      CreativeParamsList creative_params_list;
      InstantiateParams inst_params(
        request_params.common_info().user_id(),
        request_params.context_info().enabled_notice());
      inst_params.ext_tag_id = String::SubString(ad_slot.ext_tag_id());

      inst_params.video_width = ad_slot.video_width();
      inst_params.video_height = ad_slot.video_height();
      inst_params.publisher_site_id = request_params.publisher_site_id();
      inst_params.publisher_account_id = ad_slot_context.publisher_account_id;

      instantiate_creative_(
        request_params.common_info(),
        config,
        colocation,
        inst_params,
        ad_slot.format().c_str(),
        select_result,
        request_result_params,
        creative_params_list,
        creative_body,
        ad_slot_context,
        &request_params.exclude_pubpixel_accounts()
      );

      if (!select_result.selected_campaigns.empty() &&
          !creative_params_list.empty())
      {
        select_result.selected_campaigns.front().click_url =
          creative_params_list.front().click_url;
      }

      return true;
    }

    void
    CampaignManagerImpl::get_site_creative_(
      CampaignIndex* config_index,
      const Colocation* colocation,
      const Tag* tag,
      const Tag::SizeMap& tag_sizes,
      const AdServer::CampaignSvcs::CampaignManager::RequestParams&
        request_params,
      const AdServer::CampaignSvcs::CampaignManager::AdSlotInfo& ad_slot,
      AdSlotContext& ad_slot_context,
      const AdSlotMinCpm& ad_slot_min_cpm,
      const FreqCapIdSet& full_freq_caps,
      const SeqOrderMap& seq_orders,
      RequestResultParams& request_result_params,
      AdSelectionResult& select_result,
      CORBA::String_out creative_body,
      CORBA::String_out creative_url,
      AdServer::CampaignSvcs::CampaignManager::AdRequestDebugInfo*
        /*ad_request_debug_info*/,
      AdServer::CampaignSvcs::CampaignManager::AdSlotDebugInfo*
        ad_slot_debug_info)
      /*throw(eh::Exception)*/
    {
      ConstCampaignConfig_var config = config_index->configuration();
      std::string referer_hostname_str;
      std::string original_url_str;

      // logging incoming request
      if (logger_ && logger_->log_level() >= TraceLevel::MIDDLE)
      {
        log_incoming_request(request_params, ad_slot);
      }

      if (!check_request_constraints(
            request_params,
            referer_hostname_str,
            original_url_str))
      {
        return;
      }

      if (logger_->log_level() >= TraceLevel::MIDDLE)
      {
        logger_->log(
          String::SubString(
            "CampaignManagerImpl::get_creative(): Weightening campaigns: \n"),
          TraceLevel::MIDDLE,
          Aspect::CAMPAIGN_MANAGER);
      }

      /*
       * Note the same 'tag visibility' checking and 'frequency caps' checking are used
       * in the function CampaignIndex::trace_indexing().
       * Please read comment in CampaignIndex::trace_indexing() if you want to modify this code.
       */
      if (CampaignIndex::check_tag_visibility(ad_slot.tag_visibility, tag) ||
          CampaignIndex::check_site_freq_cap(request_params.profiling_available, full_freq_caps, tag))
      {
        return;
      }

      convert_ccg_keywords_(
        config,
        tag,
        request_result_params.hit_keywords,
        request_params.ccg_keywords,
        request_params.profiling_available,
        full_freq_caps);

      AuctionType auction_type = AT_MAX_ECPM;
      AuctionType second_auction_type = AT_MAX_ECPM;

      // weightening campaigns
      CampaignSelector::WeightedCampaignPtr weighted_campaign;
      CampaignSelector::WeightedCampaignKeywordListPtr weighted_campaign_keywords;

      CampaignSelector campaign_selector(
        config_index, ctr_provider_.get(), conv_rate_provider_.get());
      CampaignSelectParams_var campaign_select_params_ptr(new CampaignSelectParams(
        request_params.profiling_available,
        full_freq_caps,
        seq_orders,
        colocation,
        tag,
        tag_sizes,
        request_params.common_info.request_type == AR_GOOGLE,
        ad_slot.tag_visibility,
        ad_slot.tag_predicted_viewability));

      CampaignSelectParams& campaign_select_params = *campaign_select_params_ptr;

      {
        ChannelIdHashSet triggered_channels(
          request_params.channels.get_buffer(),
          request_params.channels.get_buffer() + request_params.channels.length());
        triggered_channels.emplace(TRUE_CHANNEL_ID);

        campaign_select_params.user_id =
          CorbaAlgs::unpack_user_id(request_params.common_info.user_id);
        campaign_select_params.country_code =
          request_params.common_info.location.length() ?
          request_params.common_info.location[0].country.in() : "";
        campaign_select_params.format = ad_slot.format;
        campaign_select_params.referer_hostname = referer_hostname_str;
        campaign_select_params.original_url = original_url_str;
        campaign_select_params.user_status = static_cast<UserStatus>(
          request_params.common_info.user_status);
        campaign_select_params.test_request = ad_slot_context.test_request;
        campaign_select_params.time = CorbaAlgs::unpack_time(request_params.common_info.time);
        campaign_select_params.tag_delivery_factor = request_params.tag_delivery_factor;
        campaign_select_params.random = request_params.common_info.random;
        campaign_select_params.random2 = Generics::safe_rand(RANDOM_PARAM_MAX);
        campaign_select_params.up_expand_space = ad_slot.up_expand_space >= 0 ?
          static_cast<unsigned long>(ad_slot.up_expand_space) : 0;
        campaign_select_params.right_expand_space = ad_slot.right_expand_space >= 0 ?
          static_cast<unsigned long>(ad_slot.right_expand_space) : 0;
        campaign_select_params.down_expand_space = ad_slot.down_expand_space >= 0 ?
          static_cast<unsigned long>(ad_slot.down_expand_space) : 0;
        campaign_select_params.left_expand_space = ad_slot.left_expand_space >= 0 ?
          static_cast<unsigned long>(ad_slot.left_expand_space) : 0;
        campaign_select_params.video_min_duration = ad_slot.video_min_duration;
        campaign_select_params.video_max_duration =
          ad_slot.video_max_duration >= 0 ?
            std::optional<unsigned long>(ad_slot.video_max_duration) :
            std::optional<unsigned long>();
        campaign_select_params.video_skippable_max_duration =
          ad_slot.video_skippable_max_duration >= 0 ?
            std::optional<unsigned long>(ad_slot.video_skippable_max_duration) :
            std::optional<unsigned long>();
        campaign_select_params.video_allow_skippable = ad_slot.video_allow_skippable;
        campaign_select_params.video_allow_unskippable = ad_slot.video_allow_unskippable;
        CorbaAlgs::convert_sequence(ad_slot.allowed_durations, campaign_select_params.allowed_durations);
        campaign_select_params.user_create_time = CorbaAlgs::unpack_time(
          request_params.client_create_time);
        campaign_select_params.only_display_ad = request_params.only_display_ad;
        campaign_select_params.min_pub_ecpm = ad_slot_min_cpm.min_pub_ecpm;
        campaign_select_params.min_ecpm = ad_slot_min_cpm.min_pub_ecpm_system;

        convert_external_categories_(
          campaign_select_params.exclude_categories,
          *config,
          request_params,
          ad_slot.exclude_categories);

        convert_external_categories_(
          campaign_select_params.required_categories,
          *config,
          request_params,
          ad_slot.required_categories);

        if(ad_slot.required_categories.length() > 0 && campaign_select_params.required_categories.empty())
        {
          // some category can't be resolved
          campaign_select_params.required_categories.insert(0);
        }

        // fill campaign_select_params fields required for CTR calculate
        campaign_select_params.ext_tag_id = ad_slot.ext_tag_id.in();
        campaign_select_params.short_referer_hash = request_params.context_info.short_referer_hash;
        campaign_select_params.channels = triggered_channels;
        //CorbaAlgs::convert_sequence(request_params.channels, campaign_select_params.channels_array);

        campaign_select_params.last_platform_channel_id = 0;
        campaign_select_params.time_hour = campaign_select_params.time.get_gm_time().tm_hour;
        campaign_select_params.time_week_day = ((
          campaign_select_params.time /
          Generics::Time::ONE_DAY.tv_sec).tv_sec + 3) % 7;
        CorbaAlgs::convert_sequence(
          request_params.context_info.geo_channels, campaign_select_params.geo_channels);
        campaign_select_params.secure =
          (request_params.common_info.creative_instantiate_type == AdInstantiateRule::SECURE);
        for(CORBA::ULong i = 0; i < request_params.campaign_freqs.length(); ++i)
        {
          campaign_select_params.campaign_imps.insert(
            std::make_pair(
              request_params.campaign_freqs[i].campaign_id,
              std::min(
                static_cast<unsigned long>(request_params.campaign_freqs[i].imps),
                100ul)));
        }

        StringSet norm_platform_names;

        if(config)
        {
          // TODO: remove equal block from CampaignManagerLogAdapter
          ChannelIdSet platform_channels;
          ChannelIdHashSet platforms(
            request_params.context_info.platform_ids.get_buffer(),
            request_params.context_info.platform_ids.get_buffer() + request_params.context_info.platform_ids.length());

          config->platform_channels->match(
            platform_channels,
            platforms);

          for(auto platform_id_it = platforms.begin();
            platform_id_it != platforms.end();
            ++platform_id_it)
          {
            auto platform_it = config->platforms.find(*platform_id_it);

            if(platform_it != config->platforms.end())
            {
              norm_platform_names.insert(platform_it->second);              
            }
          }

          unsigned long cur_priority = 0;

          for(ChannelIdSet::const_iterator pch_it = platform_channels.begin();
              pch_it != platform_channels.end();
              ++pch_it)
          {
            CampaignConfig::PlatformChannelPriorityMap::const_iterator pr_it =
              config->platform_channel_priorities.find(*pch_it);
            if(pr_it != config->platform_channel_priorities.end())
            {
              if(campaign_select_params.last_platform_channel_id == 0 ||
                 cur_priority < pr_it->second.priority)
              {
                cur_priority = pr_it->second.priority;
                campaign_select_params.last_platform_channel_id = *pch_it;
              }

              norm_platform_names.insert(pr_it->second.norm_name);
            }
          }
        }

        fill_tns_counter_device_type_(
          ad_slot_context.tns_counter_device_type,
          norm_platform_names);

        for(CORBA::ULong token_i = 0; token_i < ad_slot.tokens.length(); ++token_i)
        {
          ad_slot_context.tokens.emplace(ad_slot.tokens[token_i].name,
              ad_slot.tokens[token_i].value);
        }

        // determine auction type, using order: max ecpm, prop probability, random
        {
          RevenueDecimal auction_offset = RevenueDecimal::div(
            RevenueDecimal(false, campaign_select_params.random % RANDOM_PARAM_MAX, 0),
            RevenueDecimal(false, RANDOM_PARAM_MAX, 0));

          if(auction_offset >= tag->auction_max_ecpm_share)
          {
            if(auction_offset <
               tag->auction_max_ecpm_share + tag->auction_prop_probability_share)
            {
              auction_type = AT_PROPORTIONAL_PROBABILITY;
            }
            else
            {
              auction_type = AT_RANDOM;
              if(tag->auction_prop_probability_share + tag->auction_max_ecpm_share ==
                 RevenueDecimal::ZERO)
              {
                second_auction_type = AT_RANDOM;
              }
              else
              {
                // calculate auction type for second auction
                auction_offset -=
                  (tag->auction_max_ecpm_share +
                   tag->auction_prop_probability_share);
                if(RevenueDecimal::div(RevenueDecimal::mul(
                      auction_offset,
                      tag->auction_prop_probability_share +
                      tag->auction_max_ecpm_share, Generics::DMR_ROUND),
                    tag->auction_random_share) >=
                  tag->auction_max_ecpm_share)
                {
                  second_auction_type = AT_PROPORTIONAL_PROBABILITY;
                }
              }
            }
          }
        }

        campaign_selector.select_campaigns(
          auction_type,
          second_auction_type,
          campaign_select_params_ptr,
          triggered_channels,
          request_result_params.hit_keywords,
          true, // collect lost
          weighted_campaign_keywords,
          weighted_campaign,
          select_result);

        assert(
          (weighted_campaign.get() || (
            weighted_campaign_keywords.get() && !weighted_campaign_keywords->empty())) ?
          (select_result.tag_size != 0) : true);

        /*
        if(!CorbaAlgs::unpack_user_id(request_params.household_id).is_null() &&
           weighted_campaign.get() == 0 &&
           weighted_campaign_keywords.get() == 0)
        {
          // try to select household based campaign
          ChannelIdHashSet hid_triggered_channels;
          CampaignKeywordMap hid_hit_keywords;

          CorbaAlgs::convert_sequence(
            request_params.hid_channels, hid_triggered_channels);
          CorbaAlgs::convert_sequence(
            request_params.context_info.platform_ids, hid_triggered_channels);
          CorbaAlgs::convert_sequence(
            request_params.context_info.geo_channels, hid_triggered_channels);

          convert_ccg_keywords_(
            config,
            tag,
            hid_hit_keywords,
            request_params.hid_ccg_keywords,
            request_params.profiling_available,
            full_freq_caps);

          campaign_selector.select_campaigns(
            auction_type,
            second_auction_type,
            campaign_select_params_ptr,
            hid_triggered_channels,
            hid_hit_keywords,
            false, // collect lost
            weighted_campaign_keywords,
            weighted_campaign,
            select_result);

          if(weighted_campaign.get() != 0 ||
            weighted_campaign_keywords.get() != 0)
          {
            select_result.household_based = true;
          }
        }
        */
      }

      if(check_billing_state_container_)
      {
        if(weighted_campaign_keywords.get())
        {
          // TODO: one call for all keywords
          // clear weighted_campaign_keywords if showing not allowed
          bool available = true;

          for(CampaignSelector::WeightedCampaignKeywordList::const_iterator cmp_it =
              weighted_campaign_keywords->begin();
            cmp_it != weighted_campaign_keywords->end(); ++cmp_it)
          {
            const GrpcBillingStateContainer::BidCheckResult check_result =
              check_billing_state_container_->check_available_bid(
                campaign_select_params.time,
                cmp_it->campaign->advertiser->bill_account_id(),
                cmp_it->campaign->advertiser->not_bill_account_id(),
                cmp_it->campaign->campaign_group_id,
                cmp_it->campaign->campaign_id,
                cmp_it->ctr,
                cmp_it->campaign);

            available &= apply_check_available_bid_result_(
              cmp_it->campaign,
              check_result,
              cmp_it->ctr);
          }

          if(!available)
          {
            weighted_campaign_keywords.reset(nullptr);
          }
        }

        if(weighted_campaign.get())
        {
          // clear weighted_campaign if showing not allowed
          const GrpcBillingStateContainer::BidCheckResult check_result =
            check_billing_state_container_->check_available_bid(
              campaign_select_params.time,
              weighted_campaign->campaign->advertiser->bill_account_id(),
              weighted_campaign->campaign->advertiser->not_bill_account_id(),
              weighted_campaign->campaign->campaign_group_id,
              weighted_campaign->campaign->campaign_id,
              weighted_campaign->ctr,
              weighted_campaign->campaign);

          if(!apply_check_available_bid_result_(
               weighted_campaign->campaign,
               check_result,
               weighted_campaign->ctr))
          {
            weighted_campaign.reset(nullptr);
          }
        }
      }

      // instantiate creatives
      bool text_creative_selected = false;
      select_result.text_campaigns = false;

      if(weighted_campaign_keywords.get() &&
         !weighted_campaign_keywords->empty())
      {
        assert(select_result.tag_size);

        CreativeParamsList creative_params_list;

        text_creative_selected |= instantiate_text_creatives(
          config,
          colocation,
          request_params,
          ad_slot,
          *weighted_campaign_keywords.get(),
          select_result,
          request_result_params,
          creative_params_list,
          ad_slot_debug_info,
          creative_body,
          creative_url,
          ad_slot_context);

        if(!text_creative_selected)
        {
          select_result.selected_campaigns.clear();
        }

        select_result.text_campaigns = text_creative_selected;
      }

      bool display_creative_selected = false;

      if(weighted_campaign.get() && !text_creative_selected)
      {
        size_t display_try_number = 0;

        while(!display_creative_selected && display_try_number++ < 2)
        {
          CreativeParams creative_params;
          AdSelectionResult display_ad_selection_result(select_result);

          display_creative_selected |= instantiate_display_creative(
            config,
            colocation,
            request_params,
            ad_slot,
            *weighted_campaign.get(),
            display_ad_selection_result,
            request_result_params,
            creative_params,
            ad_slot_debug_info,
            creative_body,
            creative_url,
            ad_slot_context);

          if(display_creative_selected)
          {
            select_result = display_ad_selection_result;
          }
        }
      }

      if(!select_result.selected_campaigns.empty())
      {
        // updating freq_caps
        if(tag->site->freq_cap_id)
        {
          select_result.freq_caps.insert(tag->site->freq_cap_id);
        }
      }

      ConfirmCreativeAmountArray confirm_creatives;

      for(CampaignSelectionDataList::iterator cs_it =
            select_result.selected_campaigns.begin();
          cs_it != select_result.selected_campaigns.end(); ++cs_it)
      {
        const CampaignSelectionData& select_params = *cs_it;

        assert(cs_it->campaign && cs_it->creative);

        const Campaign* campaign = cs_it->campaign;
        const Creative* creative = cs_it->creative;

        if(!select_params.track_impr)
        {
          confirm_creatives.push_back(ConfirmCreativeAmount(creative->ccid, select_params.ctr));
        }

        // fill imp
        {
          CampaignSelectParams::CampaignImpsMap::const_iterator cmp_it =
            campaign_select_params.campaign_imps.find(campaign->campaign_group_id);
          cs_it->campaign_imps = (
            cmp_it != campaign_select_params.campaign_imps.end() ?
            cmp_it->second : 0);
        }

        // updating freq caps
        FreqCapIdSet& result_fc_set =
          select_params.track_impr ? select_result.uc_freq_caps :
          select_result.freq_caps;

        if(creative->fc_id)
        {
          result_fc_set.insert(creative->fc_id);
        }

        if(campaign->fc_id)
        {
          result_fc_set.insert(campaign->fc_id);
        }

        if(campaign->group_fc_id)
        {
          result_fc_set.insert(campaign->group_fc_id);
        }

        if(cs_it->campaign_keyword.in())
        {
          CampaignConfig::ChannelMap::const_iterator ch_it =
            config->expression_channels.find(cs_it->campaign_keyword->channel_id);
          if(ch_it != config->expression_channels.end() &&
             ch_it->second->params().common_params.in() &&
             ch_it->second->params().common_params->freq_cap_id)
          {
            result_fc_set.insert(ch_it->second->params().common_params->freq_cap_id);
          }
        }
      }

      confirm_amounts_(
        config,
        campaign_select_params.time,
        confirm_creatives,
        CR_CPM);

      if(ad_slot_debug_info)
      {
        ad_slot_debug_info->auction_type = auction_type;
      }
    }

    void
    CampaignManagerImpl::get_site_creative_(
      CampaignIndex* config_index,
      const Colocation* colocation,
      const Tag* tag,
      const Tag::SizeMap& tag_sizes,
      const Proto::RequestParams& request_params,
      const Proto::AdSlotInfo& ad_slot,
      AdSlotContext& ad_slot_context,
      const AdSlotMinCpm& ad_slot_min_cpm,
      const FreqCapIdSet& full_freq_caps,
      const SeqOrderMap& seq_orders,
      RequestResultParams& request_result_params,
      AdSelectionResult& select_result,
      std::string& creative_body,
      std::string& creative_url,
      Proto::AdRequestDebugInfo* /*ad_request_debug_info*/,
      Proto::AdSlotDebugInfo* ad_slot_debug_info)
    {
      ConstCampaignConfig_var config = config_index->configuration();
      std::string referer_hostname_str;
      std::string original_url_str;

      // logging incoming request
      if (logger_ && logger_->log_level() >= TraceLevel::MIDDLE)
      {
        log_incoming_request(request_params, ad_slot);
      }

      if (!check_request_constraints(
        request_params,
        referer_hostname_str,
        original_url_str))
      {
        return;
      }

      if (logger_->log_level() >= TraceLevel::MIDDLE)
      {
        logger_->log(
          String::SubString(
            "CampaignManagerImpl::get_creative(): Weightening campaigns: \n"),
          TraceLevel::MIDDLE,
          Aspect::CAMPAIGN_MANAGER);
      }

      /*
       * Note the same 'tag visibility' checking and 'frequency caps' checking are used
       * in the function CampaignIndex::trace_indexing().
       * Please read comment in CampaignIndex::trace_indexing() if you want to modify this code.
       */
      if (CampaignIndex::check_tag_visibility(ad_slot.tag_visibility(), tag) ||
          CampaignIndex::check_site_freq_cap(request_params.profiling_available(), full_freq_caps, tag))
      {
        return;
      }

      convert_ccg_keywords_(
        config,
        tag,
        request_result_params.hit_keywords,
        request_params.ccg_keywords(),
        request_params.profiling_available(),
        full_freq_caps);

      AuctionType auction_type = AT_MAX_ECPM;
      AuctionType second_auction_type = AT_MAX_ECPM;

      // weightening campaigns
      CampaignSelector::WeightedCampaignPtr weighted_campaign;
      CampaignSelector::WeightedCampaignKeywordListPtr weighted_campaign_keywords;

      CampaignSelector campaign_selector(
        config_index, ctr_provider_.get(), conv_rate_provider_.get());
      CampaignSelectParams_var campaign_select_params_ptr(new CampaignSelectParams(
        request_params.profiling_available(),
        full_freq_caps,
        seq_orders,
        colocation,
        tag,
        tag_sizes,
        request_params.common_info().request_type() == AR_GOOGLE,
        ad_slot.tag_visibility(),
        ad_slot.tag_predicted_viewability()));

      CampaignSelectParams& campaign_select_params = *campaign_select_params_ptr;

      {
        ChannelIdHashSet triggered_channels(
          std::begin(request_params.channels()),
          std::end(request_params.channels()));

        campaign_select_params.user_id =
          GrpcAlgs::unpack_user_id(request_params.common_info().user_id());
        campaign_select_params.country_code =
          !request_params.common_info().location().empty() ?
            request_params.common_info().location()[0].country().c_str() : "";
        campaign_select_params.format = ad_slot.format();
        campaign_select_params.referer_hostname = referer_hostname_str;
        campaign_select_params.original_url = original_url_str;
        campaign_select_params.user_status = static_cast<UserStatus>(
          request_params.common_info().user_status());
        campaign_select_params.test_request = ad_slot_context.test_request;
        campaign_select_params.time = GrpcAlgs::unpack_time(request_params.common_info().time());
        campaign_select_params.tag_delivery_factor = request_params.tag_delivery_factor();
        campaign_select_params.random = request_params.common_info().random();
        campaign_select_params.random2 = Generics::safe_rand(RANDOM_PARAM_MAX);
        campaign_select_params.up_expand_space = ad_slot.up_expand_space() >= 0 ?
          static_cast<unsigned long>(ad_slot.up_expand_space()) : 0;
        campaign_select_params.right_expand_space = ad_slot.right_expand_space() >= 0 ?
          static_cast<unsigned long>(ad_slot.right_expand_space()) : 0;
        campaign_select_params.down_expand_space = ad_slot.down_expand_space() >= 0 ?
          static_cast<unsigned long>(ad_slot.down_expand_space()) : 0;
        campaign_select_params.left_expand_space = ad_slot.left_expand_space() >= 0 ?
          static_cast<unsigned long>(ad_slot.left_expand_space()) : 0;
        campaign_select_params.video_min_duration = ad_slot.video_min_duration();
        campaign_select_params.video_max_duration =
          ad_slot.video_max_duration() >= 0 ?
            std::optional<unsigned long>(ad_slot.video_max_duration()) :
          std::optional<unsigned long>();
        campaign_select_params.video_skippable_max_duration =
          ad_slot.video_skippable_max_duration() >= 0 ?
            std::optional<unsigned long>(ad_slot.video_skippable_max_duration()) :
            std::optional<unsigned long>();
        campaign_select_params.video_allow_skippable = ad_slot.video_allow_skippable();
        campaign_select_params.video_allow_unskippable = ad_slot.video_allow_unskippable();
        campaign_select_params.allowed_durations.insert(
          std::begin(ad_slot.allowed_durations()),
          std::end(ad_slot.allowed_durations()));
        campaign_select_params.user_create_time = GrpcAlgs::unpack_time(
          request_params.client_create_time());
        campaign_select_params.only_display_ad = request_params.only_display_ad();
        campaign_select_params.min_pub_ecpm = ad_slot_min_cpm.min_pub_ecpm;
        campaign_select_params.min_ecpm = ad_slot_min_cpm.min_pub_ecpm_system;

        convert_external_categories_(
          campaign_select_params.exclude_categories,
          *config,
          request_params,
          ad_slot.exclude_categories());

        convert_external_categories_(
          campaign_select_params.required_categories,
          *config,
          request_params,
          ad_slot.required_categories());

        if(ad_slot.required_categories().size() > 0 && campaign_select_params.required_categories.empty())
        {
          // some category can't be resolved
          campaign_select_params.required_categories.insert(0);
        }

        // fill campaign_select_params fields required for CTR calculate
        campaign_select_params.ext_tag_id = ad_slot.ext_tag_id();
        campaign_select_params.short_referer_hash = request_params.context_info().short_referer_hash();
        campaign_select_params.channels = triggered_channels;

        campaign_select_params.last_platform_channel_id = 0;
        campaign_select_params.time_hour = campaign_select_params.time.get_gm_time().tm_hour;
        campaign_select_params.time_week_day = ((
          campaign_select_params.time /
          Generics::Time::ONE_DAY.tv_sec).tv_sec + 3) % 7;

        campaign_select_params.geo_channels.insert(
          std::begin(request_params.context_info().geo_channels()),
          std::end(request_params.context_info().geo_channels()));

        campaign_select_params.secure =
          (request_params.common_info().creative_instantiate_type() == AdInstantiateRule::SECURE);
        for(const auto& campaign_freq : request_params.campaign_freqs())
        {
          campaign_select_params.campaign_imps.insert(
            std::make_pair(
              campaign_freq.campaign_id(),
              std::min(
                static_cast<unsigned long>(campaign_freq.imps()),
                100ul)));
        }

        StringSet norm_platform_names;

        if(config)
        {
          // TODO: remove equal block from CampaignManagerLogAdapter
          ChannelIdSet platform_channels;
          ChannelIdHashSet platforms(
            std::begin(request_params.context_info().platform_ids()),
            std::end(request_params.context_info().platform_ids()));

          config->platform_channels->match(
            platform_channels,
            platforms);

          for(auto platform_id_it = platforms.begin();
              platform_id_it != platforms.end();
              ++platform_id_it)
          {
            auto platform_it = config->platforms.find(*platform_id_it);

            if(platform_it != config->platforms.end())
            {
              norm_platform_names.insert(platform_it->second);
            }
          }

          unsigned long cur_priority = 0;

          for(ChannelIdSet::const_iterator pch_it = platform_channels.begin();
              pch_it != platform_channels.end();
              ++pch_it)
          {
            CampaignConfig::PlatformChannelPriorityMap::const_iterator pr_it =
              config->platform_channel_priorities.find(*pch_it);
            if(pr_it != config->platform_channel_priorities.end())
            {
              if(campaign_select_params.last_platform_channel_id == 0 ||
                 cur_priority < pr_it->second.priority)
              {
                cur_priority = pr_it->second.priority;
                campaign_select_params.last_platform_channel_id = *pch_it;
              }

              norm_platform_names.insert(pr_it->second.norm_name);
            }
          }
        }

        fill_tns_counter_device_type_(
          ad_slot_context.tns_counter_device_type,
          norm_platform_names);

        // determine auction type, using order: max ecpm, prop probability, random
        {
          RevenueDecimal auction_offset = RevenueDecimal::div(
            RevenueDecimal(false, campaign_select_params.random % RANDOM_PARAM_MAX, 0),
            RevenueDecimal(false, RANDOM_PARAM_MAX, 0));

          if(auction_offset >= tag->auction_max_ecpm_share)
          {
            if(auction_offset <
               tag->auction_max_ecpm_share + tag->auction_prop_probability_share)
            {
              auction_type = AT_PROPORTIONAL_PROBABILITY;
            }
            else
            {
              auction_type = AT_RANDOM;
              if(tag->auction_prop_probability_share + tag->auction_max_ecpm_share ==
                 RevenueDecimal::ZERO)
              {
                second_auction_type = AT_RANDOM;
              }
              else
              {
                // calculate auction type for second auction
                auction_offset -=
                  (tag->auction_max_ecpm_share +
                   tag->auction_prop_probability_share);
                if(RevenueDecimal::div(RevenueDecimal::mul(
                        auction_offset,
                        tag->auction_prop_probability_share +
                        tag->auction_max_ecpm_share, Generics::DMR_ROUND),
                      tag->auction_random_share) >=
                   tag->auction_max_ecpm_share)
                {
                  second_auction_type = AT_PROPORTIONAL_PROBABILITY;
                }
              }
            }
          }
        }

        campaign_selector.select_campaigns(
          auction_type,
          second_auction_type,
          campaign_select_params_ptr,
          triggered_channels,
          request_result_params.hit_keywords,
          true, // collect lost
          weighted_campaign_keywords,
          weighted_campaign,
          select_result);

        assert(
          (weighted_campaign.get() || (
            weighted_campaign_keywords.get() && !weighted_campaign_keywords->empty())) ?
          (select_result.tag_size != 0) : true);
      }

      if(check_billing_state_container_)
      {
        if(weighted_campaign_keywords.get())
        {
          // TODO: one call for all keywords
          // clear weighted_campaign_keywords if showing not allowed
          bool available = true;

          for(auto cmp_it = weighted_campaign_keywords->begin();
              cmp_it != weighted_campaign_keywords->end();
              ++cmp_it)
          {
            const GrpcBillingStateContainer::BidCheckResult check_result =
              check_billing_state_container_->check_available_bid(
                campaign_select_params.time,
                cmp_it->campaign->advertiser->bill_account_id(),
                cmp_it->campaign->advertiser->not_bill_account_id(),
                cmp_it->campaign->campaign_group_id,
                cmp_it->campaign->campaign_id,
                cmp_it->ctr,
                cmp_it->campaign);

            available &= apply_check_available_bid_result_(
              cmp_it->campaign,
              check_result,
              cmp_it->ctr);
          }

          if(!available)
          {
            weighted_campaign_keywords.reset(nullptr);
          }
        }

        if(weighted_campaign.get())
        {
          // clear weighted_campaign if showing not allowed
          const GrpcBillingStateContainer::BidCheckResult check_result =
            check_billing_state_container_->check_available_bid(
              campaign_select_params.time,
              weighted_campaign->campaign->advertiser->bill_account_id(),
              weighted_campaign->campaign->advertiser->not_bill_account_id(),
              weighted_campaign->campaign->campaign_group_id,
              weighted_campaign->campaign->campaign_id,
              weighted_campaign->ctr,
              weighted_campaign->campaign);

          if(!apply_check_available_bid_result_(
            weighted_campaign->campaign,
            check_result,
            weighted_campaign->ctr))
          {
            weighted_campaign.reset(nullptr);
          }
        }
      }

      // instantiate creatives
      bool text_creative_selected = false;
      select_result.text_campaigns = false;

      if(weighted_campaign_keywords.get() &&
         !weighted_campaign_keywords->empty())
      {
        assert(select_result.tag_size);

        CreativeParamsList creative_params_list;

        text_creative_selected |= instantiate_text_creatives(
          config,
          colocation,
          request_params,
          ad_slot,
          *weighted_campaign_keywords.get(),
          select_result,
          request_result_params,
          creative_params_list,
          ad_slot_debug_info,
          creative_body,
          creative_url,
          ad_slot_context);

        if(!text_creative_selected)
        {
          select_result.selected_campaigns.clear();
        }

        select_result.text_campaigns = text_creative_selected;
      }

      bool display_creative_selected = false;

      if(weighted_campaign.get() && !text_creative_selected)
      {
        size_t display_try_number = 0;

        while(!display_creative_selected && display_try_number++ < 2)
        {
          CreativeParams creative_params;
          AdSelectionResult display_ad_selection_result(select_result);

          display_creative_selected |= instantiate_display_creative(
            config,
            colocation,
            request_params,
            ad_slot,
            *weighted_campaign.get(),
            display_ad_selection_result,
            request_result_params,
            creative_params,
            ad_slot_debug_info,
            creative_body,
            creative_url,
            ad_slot_context);

          if(display_creative_selected)
          {
            select_result = display_ad_selection_result;
          }
        }
      }

      if(!select_result.selected_campaigns.empty())
      {
        // updating freq_caps
        if(tag->site->freq_cap_id)
        {
          select_result.freq_caps.insert(tag->site->freq_cap_id);
        }
      }

      ConfirmCreativeAmountArray confirm_creatives;

      for(auto cs_it = select_result.selected_campaigns.begin();
          cs_it != select_result.selected_campaigns.end();
          ++cs_it)
      {
        const CampaignSelectionData& select_params = *cs_it;

        assert(cs_it->campaign && cs_it->creative);

        const Campaign* campaign = cs_it->campaign;
        const Creative* creative = cs_it->creative;

        if(!select_params.track_impr)
        {
          confirm_creatives.push_back(ConfirmCreativeAmount(creative->ccid, select_params.ctr));
        }

        // fill imp
        {
          auto cmp_it = campaign_select_params.campaign_imps.find(
            campaign->campaign_group_id);
          cs_it->campaign_imps = (
            cmp_it != campaign_select_params.campaign_imps.end() ?
            cmp_it->second : 0);
        }

        // updating freq caps
        FreqCapIdSet& result_fc_set =
          select_params.track_impr ? select_result.uc_freq_caps :
          select_result.freq_caps;

        if(creative->fc_id)
        {
          result_fc_set.insert(creative->fc_id);
        }

        if(campaign->fc_id)
        {
          result_fc_set.insert(campaign->fc_id);
        }

        if(campaign->group_fc_id)
        {
          result_fc_set.insert(campaign->group_fc_id);
        }

        if(cs_it->campaign_keyword.in())
        {
          const auto ch_it = config->expression_channels.find(
            cs_it->campaign_keyword->channel_id);
          if(ch_it != config->expression_channels.end() &&
             ch_it->second->params().common_params.in() &&
             ch_it->second->params().common_params->freq_cap_id)
          {
            result_fc_set.insert(ch_it->second->params().common_params->freq_cap_id);
          }
        }
      }

      confirm_amounts_(
        config,
        campaign_select_params.time,
        confirm_creatives,
        CR_CPM);

      if(ad_slot_debug_info)
      {
        ad_slot_debug_info->set_auction_type(auction_type);
      }
    }

    void
    CampaignManagerImpl::flush_logs() noexcept
    {
      static const char* FUN = "CampaignManagerImpl::flush_logs()";

      const Generics::Time LOGS_DUMP_ERROR_RESCHEDULE_PERIOD(2);

      logger_->log(
        String::SubString("flush logs"),
        TraceLevel::MIDDLE,
        Aspect::CAMPAIGN_MANAGER);

      Generics::Time next_flush;

      try
      {
        next_flush = campaign_manager_logger_->flush_if_required(
          Generics::Time::get_time_of_day());
      }
      catch (const eh::Exception& ex)
      {
        next_flush = Generics::Time::get_time_of_day() +
          LOGS_DUMP_ERROR_RESCHEDULE_PERIOD;

        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught on flush logs:" << ex.what();

        logger_->log(ostr.str(),
          Logging::Logger::ERROR,
          Aspect::CAMPAIGN_MANAGER,
          "ADS-IMPL-250");
      }

      if(logger_->log_level() >= TraceLevel::MIDDLE)
      {
        Stream::Error ostr;
        ostr << FUN << ": logs flushed, next flush at " << next_flush.get_gm_time();
        logger_->log(
          ostr.str(),
          TraceLevel::MIDDLE,
          Aspect::CAMPAIGN_MANAGER);
      }

      if(next_flush != Generics::Time::ZERO)
      {
        try
        {
          CampaignManagerTaskMessage_var task =
            new FlushLogsTaskMessage(this, task_runner_);

          scheduler_->schedule(task, next_flush);
        }
        catch (const eh::Exception &ex)
        {
          Stream::Error ostr;
          ostr << FUN << ": Can't schedule next flush task. "
            "eh::Exception caught:" << ex.what();

          logger_->log(ostr.str(),
            Logging::Logger::ERROR,
            Aspect::CAMPAIGN_MANAGER,
            "ADS-IMPL-251");
        }
      }
    }

    AdServer::CampaignSvcs::CampaignManager::ChannelSearchResultSeq*
    CampaignManagerImpl::get_channel_links(
      const AdServer::CampaignSvcs::ChannelIdSeq& channels,
      bool match)
      /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException)*/
    {
      static const char* FUN = "CampaignManagerImpl::get_channel_links()";

      try
      {
        ConstCampaignConfig_var config = configuration();

        if(config.in() == 0)
        {
          throw Exception("Can't receive configuration.");
        }

        ChannelIdHashSet used_simple_channels;
        CorbaAlgs::convert_sequence(channels, used_simple_channels);

        ChannelUseCountMap uc_tbl;

        /* expression channel cycle */
        for(CampaignConfig::ChannelMap::const_iterator ch_it =
              config->expression_channels.begin();
            ch_it != config->expression_channels.end(); ++ch_it)
        {
          if(match)
          {
            ch_it->second->triggered(&used_simple_channels, 0, "A", &uc_tbl);
          }
          else
          {
            ch_it->second->use(uc_tbl, used_simple_channels, "AI");
          }
        }

        for(CampaignConfig::CampaignMap::const_iterator cmp_it =
              config->campaigns.begin();
            cmp_it != config->campaigns.end(); ++cmp_it)
        {
          if(cmp_it->second->channel.in() && cmp_it->second->channel->has_params())
          {
            ChannelUseCountMap::iterator uc_it = uc_tbl.find(
              cmp_it->second->channel->params().channel_id);
            if(uc_it != uc_tbl.end())
            {
              uc_it->second.ccg_ids.insert(cmp_it->first);
            }
          }
        }

        AdServer::CampaignSvcs::CampaignManager::ChannelSearchResultSeq_var
          result =
          new AdServer::CampaignSvcs::CampaignManager::ChannelSearchResultSeq();
        result->length(uc_tbl.size());
        CORBA::ULong i = 0;
        for (ChannelUseCountMap::const_iterator uc_it = uc_tbl.begin();
             uc_it != uc_tbl.end(); ++i, ++uc_it)
        {
          result[i].channel_id = uc_it->first.value();
          result[i].use_count = uc_it->second.count;

          CampaignConfig::ChannelMap::const_iterator discover_it =
            config->discover_channels.find(uc_it->first.value());

          if(discover_it != config->discover_channels.end())
          {
            assert(discover_it->second.in());
            const ChannelParams& ch_params = discover_it->second->params();
            assert(ch_params.discover_params.in());
            result[i].discover_query << ch_params.discover_params->query;
            if(ch_params.common_params.in())
            {
              result[i].language << ch_params.common_params->language;
            }
          }

          CorbaAlgs::fill_sequence(
            uc_it->second.ccg_ids.begin(),
            uc_it->second.ccg_ids.end(),
            result[i].ccg_ids);
          CorbaAlgs::fill_sequence(
            uc_it->second.channel_ids.begin(),
            uc_it->second.channel_ids.end(),
            result[i].matched_simple_channels);
        }

        return result._retn();
      }
      catch (const eh::Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << e.what();

        logger_->log(ostr.str(),
          Logging::Logger::ERROR,
          Aspect::CAMPAIGN_MANAGER,
          "ADS-IMPL-179");
        CORBACommons::throw_desc<
          CampaignSvcs::CampaignManager::ImplementationException>(
            ostr.str());
      }
      catch(const CORBA::SystemException& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": CORBA::SystemException caught: " << ex;

        logger_->log(ostr.str(),
          Logging::Logger::ERROR,
          Aspect::CAMPAIGN_MANAGER,
          "ADS-IMPL-180");
        CORBACommons::throw_desc<
          CampaignSvcs::CampaignManager::ImplementationException>(
            ostr.str());
      }
      return 0; // never reach
    }

    CampaignManagerImpl::GetChannelLinksResponsePtr
    CampaignManagerImpl::get_channel_links(GetChannelLinksRequestPtr&& request)
    {
      const auto id_request_grpc = request->id_request_grpc();
      try
      {
        ConstCampaignConfig_var config = configuration(false);
        if(!config)
        {
          throw NotReady("Can't receive configuration.");
        }

        const bool match = request->match();
        const auto& channels = request->channels();

        ChannelIdHashSet used_simple_channels(
          std::begin(channels),
          std::end(channels));

        ChannelUseCountMap uc_tbl;

        /* expression channel cycle */
        for(const auto& [_, info] : config->expression_channels)
        {
          if(match)
          {
            info->triggered(&used_simple_channels, 0, "A", &uc_tbl);
          }
          else
          {
            info->use(uc_tbl, used_simple_channels, "AI");
          }
        }

        for(const auto& [key, info] : config->campaigns)
        {
          if(info->channel.in() && info->channel->has_params())
          {
            auto uc_it = uc_tbl.find(
              info->channel->params().channel_id);
            if(uc_it != uc_tbl.end())
            {
              uc_it->second.ccg_ids.insert(key);
            }
          }
        }

        auto response = create_grpc_response<Proto::GetChannelLinksResponse>(
          id_request_grpc);
        auto* info_response = response->mutable_info();
        auto* channel_search_results_response =
          info_response->mutable_channel_search_results();

        channel_search_results_response->Reserve(uc_tbl.size());
        for (const auto& [key, info] : uc_tbl)
        {
          auto* result = channel_search_results_response->Add();
          result->set_channel_id(key.value());
          result->set_use_count(info.count);

          const auto discover_it = config->discover_channels.find(key.value());
          if(discover_it != config->discover_channels.end())
          {
            assert(discover_it->second.in());
            const ChannelParams& ch_params = discover_it->second->params();
            assert(ch_params.discover_params.in());
            result->set_discover_query(ch_params.discover_params->query);
            if(ch_params.common_params.in())
            {
              result->set_language(ch_params.common_params->language);
            }
          }

          auto* ccg_ids = result->mutable_ccg_ids();
          ccg_ids->Add(
            std::begin(info.ccg_ids),
            std::end(info.ccg_ids));

          auto* channel_ids = result->mutable_matched_simple_channels();
          channel_ids->Add(
            std::begin(info.channel_ids),
            std::end(info.channel_ids));
        }

        return response;
      }
      catch (const NotReady& exc)
      {
        Stream::Error stream;
        stream << FNS
               << "Caught CampaignManager::NotReady: "
               << exc.what();
        auto response = create_grpc_error_response<Proto::GetChannelLinksResponse>(
          Proto::Error_Type::Error_Type_NotReady,
          stream.str(),
          id_request_grpc);
        return response;
      }
      catch (const eh::Exception& exc)
      {
        Stream::Error stream;
        stream << FNS
               << ": "
               << exc.what();
        auto response = create_grpc_error_response<Proto::GetChannelLinksResponse>(
          Proto::Error_Type::Error_Type_Implementation,
          stream.str(),
          id_request_grpc);
        return response;
      }
      catch (...)
      {
        Stream::Error stream;
        stream << FNS
               << ": Unknown error";
        auto response = create_grpc_error_response<Proto::GetChannelLinksResponse>(
          Proto::Error_Type::Error_Type_Implementation,
          stream.str(),
          id_request_grpc);
        return response;
      }
    }

    AdServer::CampaignSvcs::CampaignManager::DiscoverChannelResultSeq*
    CampaignManagerImpl::get_discover_channels(
      const AdServer::CampaignSvcs::ChannelWeightSeq& channels,
      const char* country,
      const char* language,
      bool all)
      /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException,
        AdServer::CampaignSvcs::CampaignManager::NotReady)*/
    {
      static const char* FUN = "CampaignManagerImpl::get_discover_channels()";

      try
      {
        ConstCampaignConfig_var config = configuration();
        if (!config)
        {
          throw AdServer::CampaignSvcs::CampaignManager::NotReady(
            "Campaign configuration isn't loaded");
        }

        AdServer::CampaignSvcs::CampaignManager::DiscoverChannelResultSeq_var
          result_ptr(
            new AdServer::CampaignSvcs::CampaignManager::DiscoverChannelResultSeq());
        AdServer::CampaignSvcs::CampaignManager::DiscoverChannelResultSeq&
          res = *result_ptr;

        res.length(config->discover_channels.size());

        ChannelWeightMap triggered_channels;
        fill_triggered_channels_(triggered_channels, channels);

        CORBA::ULong i = 0;
        for(CampaignConfig::ChannelMap::const_iterator ch_it =
              config->discover_channels.begin();
            ch_it != config->discover_channels.end(); ++ch_it)
        {
          unsigned long weight;

          if ((weight = ch_it->second->triggered(0, &triggered_channels)) || (
                all && ch_it->second->params().status == 'A'))
          {
            const ChannelParams& ch_params = ch_it->second->params();

            if((country[0] == 0 || ch_params.country == country) &&
               ch_params.common_params.in() &&
               (language[0] == 0 ||
                ch_params.common_params->language == language))
            {
              res[i].channel_id = ch_it->first;
              res[i].weight = weight;
              res[i].country_code << ch_params.country;
              res[i].language << ch_params.common_params->language;

              if(ch_params.descriptive_params.in())
              {
                res[i].name << ch_params.descriptive_params->name;
              }

              res[i].query << ch_params.discover_params->query;
              res[i].annotation << ch_params.discover_params->annotation;

              CampaignConfig::SimpleChannelMap::const_iterator sit =
                config->simple_channels.find(ch_it->first);
              if(sit != config->simple_channels.end())
              {
                CorbaAlgs::fill_sequence(
                  sit->second->categories.begin(),
                  sit->second->categories.end(),
                  res[i].categories);
              }

              ++i;
            }
          }
        }

        res.length(i);

        if(logger_->log_level() >= Logging::Logger::TRACE)
        {
          Stream::Error ostr;
          ostr << FUN << ": result for (";
          for(CORBA::ULong i = 0; i < channels.length(); ++i)
          {
            ostr << (i == 0 ? "" : ",") <<
              channels[i].channel_id << ":" << channels[i].weight;
          }
          ostr << "): ";
          for(CORBA::ULong i = 0; i < res.length(); ++i)
          {
            if(i != 0) ostr << ", ";
            ostr << res[i].channel_id << "->" << res[i].weight;
          }

          logger_->log(
            ostr.str(),
            Logging::Logger::TRACE,
            Aspect::CAMPAIGN_MANAGER);
        }

        return result_ptr._retn();
      }
      catch (const eh::Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << e.what();
        logger_->log(
          ostr.str(),
          Logging::Logger::ERROR,
          Aspect::CAMPAIGN_MANAGER,
          "ADS-IMPL-181");
        CORBACommons::throw_desc<
          CampaignSvcs::CampaignManager::ImplementationException>(
            ostr.str());
      }
      catch(const CORBA::SystemException& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": CORBA::SystemException caught: " << ex;
        logger_->log(
          ostr.str(),
          Logging::Logger::ERROR,
          Aspect::CAMPAIGN_MANAGER,
          "ADS-IMPL-182");
        CORBACommons::throw_desc<
          CampaignSvcs::CampaignManager::ImplementationException>(
            ostr.str());
      }
      return 0; // never reach
    }

    CampaignManagerImpl::GetDiscoverChannelsResponsePtr
    CampaignManagerImpl::get_discover_channels(
      GetDiscoverChannelsRequestPtr&& request)
    {
      static const char* FUN = "CampaignManagerImpl::get_discover_channels()";
      const auto id_request_grpc = request->id_request_grpc();
      try
      {
        const auto& channels = request->channels();
        const auto& country = request->country();
        const auto& language = request->language();
        const auto& all = request->all();

        ConstCampaignConfig_var config = configuration(false);
        if (!config)
        {
          throw NotReady("Campaign configuration isn't loaded");
        }

        auto response = create_grpc_response<Proto::GetDiscoverChannelsResponse>(
          id_request_grpc);
        auto* info_response = response->mutable_info();
        auto* discover_channel_results_response =
          info_response->mutable_discover_channel_results();
        discover_channel_results_response->Reserve(
          config->discover_channels.size());

        ChannelWeightMap triggered_channels;
        for (const auto& channel : channels)
        {
          triggered_channels.try_emplace(
            channel.channel_id(),
            channel.weight());
        }

        for(auto ch_it = config->discover_channels.begin();
            ch_it != config->discover_channels.end();
            ++ch_it)
        {
          unsigned long weight;

          if ((weight = ch_it->second->triggered(0, &triggered_channels)) || (
                all && ch_it->second->params().status == 'A'))
          {
            const ChannelParams& ch_params = ch_it->second->params();

            if((country.empty() || ch_params.country == country) &&
               ch_params.common_params.in() &&
               (language.empty() ||
                ch_params.common_params->language == language))
            {
              auto* channel_result = discover_channel_results_response->Add();
              channel_result->set_channel_id(ch_it->first);
              channel_result->set_weight(weight);
              channel_result->set_country_code(ch_params.country);
              channel_result->set_language(ch_params.common_params->language);

              if(ch_params.descriptive_params.in())
              {
                channel_result->set_name(ch_params.descriptive_params->name);
              }

              channel_result->set_query(ch_params.discover_params->query);
              channel_result->set_annotation(ch_params.discover_params->annotation);

              CampaignConfig::SimpleChannelMap::const_iterator sit =
                config->simple_channels.find(ch_it->first);
              if(sit != config->simple_channels.end())
              {
                channel_result->mutable_categories()->Add(
                  std::begin(sit->second->categories),
                  std::end(sit->second->categories));
              }
            }
          }
        }

        if(logger_->log_level() >= Logging::Logger::TRACE)
        {
          Stream::Error ostr;
          ostr << FUN << ": result for (";
          for(int i = 0; i < channels.size(); ++i)
          {
            ostr << (i == 0 ? "" : ",") <<
              channels[i].channel_id() << ":" << channels[i].weight();
          }
          ostr << "): ";
          const int size = discover_channel_results_response->size();
          for(int i = 0; i < size; ++i)
          {
            if(i != 0) ostr << ", ";
            ostr << (*discover_channel_results_response)[i].channel_id()
                 << "->"
                 << (*discover_channel_results_response)[i].weight();
          }

          logger_->log(
            ostr.str(),
            Logging::Logger::TRACE,
            Aspect::CAMPAIGN_MANAGER);
        }

        return response;
      }
      catch (const NotReady& exc)
      {
        Stream::Error stream;
        stream << FUN
               << "Caught CampaignManager::NotReady: "
               << exc.what();
        auto response = create_grpc_error_response<Proto::GetDiscoverChannelsResponse>(
          Proto::Error_Type::Error_Type_NotReady,
          stream.str(),
          id_request_grpc);
        return response;
      }
      catch (const eh::Exception& exc)
      {
        Stream::Error stream;
        stream << FUN
               << ": "
               << exc.what();
        auto response = create_grpc_error_response<Proto::GetDiscoverChannelsResponse>(
          Proto::Error_Type::Error_Type_Implementation,
          stream.str(),
          id_request_grpc);
        return response;
      }
      catch (...)
      {
        Stream::Error stream;
        stream << FUN
               << ": Unknown error";
        auto response = create_grpc_error_response<Proto::GetDiscoverChannelsResponse>(
          Proto::Error_Type::Error_Type_Implementation,
          stream.str(),
          id_request_grpc);
        return response;
      }
    }

    AdServer::CampaignSvcs::CampaignManager::CategoryChannelNodeSeq*
    CampaignManagerImpl::get_category_channels(const char* language)
      /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException,
        AdServer::CampaignSvcs::CampaignManager::NotReady)*/
    {
      static const char* FUN = "CampaignManagerImpl::get_category_channels()";

      try
      {
        CampaignConfig_var config = configuration();
        if (!config)
        {
          throw AdServer::CampaignSvcs::CampaignManager::NotReady(
            "Campaign configuration isn't loaded");
        }

        AdServer::CampaignSvcs::CampaignManager::CategoryChannelNodeSeq_var
          result_ptr(
            new AdServer::CampaignSvcs::CampaignManager::CategoryChannelNodeSeq());

        AdServer::CampaignSvcs::CampaignManager::CategoryChannelNodeSeq&
          res = *result_ptr;

        res.length(config->category_channel_nodes.size());

        CORBA::ULong i = 0;
        for(CategoryChannelNodeMap::const_iterator ch_it =
              config->category_channel_nodes.begin();
            ch_it != config->category_channel_nodes.end(); ++ch_it)
        {
          if(fill_category_channel_node_(res[i], ch_it->second, language))
          {
            ++i;
          }
        }

        res.length(i);

        return result_ptr._retn();
      }
      catch (const eh::Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << e.what();
        logger_->log(
          ostr.str(),
          Logging::Logger::ERROR,
          Aspect::CAMPAIGN_MANAGER,
          "ADS-IMPL-181");

        CORBACommons::throw_desc<
          CampaignSvcs::CampaignManager::ImplementationException>(
            ostr.str());
      }
      catch(const CORBA::SystemException& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": CORBA::SystemException caught: " << ex;
        logger_->log(
          ostr.str(),
          Logging::Logger::ERROR,
          Aspect::CAMPAIGN_MANAGER,
          "ADS-IMPL-182");
        CORBACommons::throw_desc<
          CampaignSvcs::CampaignManager::ImplementationException>(
            ostr.str());
      }
      return 0; // never reach
    }

    CampaignManagerImpl::GetCategoryChannelsResponsePtr
    CampaignManagerImpl::get_category_channels(
      GetCategoryChannelsRequestPtr&& request)
    {
      const auto id_request_grpc = request->id_request_grpc();
      try
      {
        CampaignConfig_var config = configuration(false);
        if (!config)
        {
          throw NotReady("Campaign configuration isn't loaded");
        }

        const auto& language = request->language();

        auto response = create_grpc_response<Proto::GetCategoryChannelsResponse>(
          id_request_grpc);
        auto* info_response = response->mutable_info();
        auto* category_channel_node_info_seq_response =
          info_response->mutable_category_channel_node_info_seq();

        category_channel_node_info_seq_response->Reserve(config->category_channel_nodes.size());
        for(const auto& [_, value] : config->category_channel_nodes)
        {
          auto* category_channel_node_info = category_channel_node_info_seq_response->Add();
          if(!fill_category_channel_node_(*category_channel_node_info, value, language))
          {
            category_channel_node_info_seq_response->RemoveLast();
          }
        }

        return response;
      }
      catch (const NotReady& exc)
      {
        Stream::Error stream;
        stream << FNS
               << "Caught CampaignManager::NotReady: "
               << exc.what();
        auto response = create_grpc_error_response<Proto::GetCategoryChannelsResponse>(
          Proto::Error_Type::Error_Type_NotReady,
          stream.str(),
          id_request_grpc);
        return response;
      }
      catch (const eh::Exception& exc)
      {
        Stream::Error stream;
        stream << FNS
               << ": "
               << exc.what();
        auto response = create_grpc_error_response<Proto::GetCategoryChannelsResponse>(
          Proto::Error_Type::Error_Type_Implementation,
          stream.str(),
          id_request_grpc);
        return response;
      }
      catch (...)
      {
        Stream::Error stream;
        stream << FNS
               << ": Unknown error";
        auto response = create_grpc_error_response<Proto::GetCategoryChannelsResponse>(
          Proto::Error_Type::Error_Type_Implementation,
          stream.str(),
          id_request_grpc);
        return response;
      }
    }

    void
    CampaignManagerImpl::verify_impression(
      const AdServer::CampaignSvcs::CampaignManager::ImpressionInfo& impression_info,
      ::AdServer::CampaignSvcs::CampaignManager::ImpressionResultInfo_out impression_result_info)
      /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException,
        AdServer::CampaignSvcs::CampaignManager::NotReady)*/
    {
      static const char* FUN = "CampaignManagerImpl::verify_impression()";

      impression_result_info = new AdServer::CampaignSvcs::CampaignManager::ImpressionResultInfo();

      const Generics::Time imp_time = CorbaAlgs::unpack_time(impression_info.time);
      const Generics::Time bid_time = CorbaAlgs::unpack_time(impression_info.bid_time);

      CampaignConfig_var config = configuration(true);

      RevenueType pub_imp_revenue_type = static_cast<RevenueType>(
        impression_info.pub_imp_revenue_type);

      RevenueDecimal pub_first_slot_revenue;
      RevenueDecimal pub_non_first_slot_revenue;

      if(pub_imp_revenue_type == RT_ABSOLUTE)
      {
        RevenueDecimal pub_imp_revenue = RevenueDecimal::ZERO;

        if(static_cast<AdRequestType>(impression_info.request_type) !=
           AR_NORMAL)
        {
          pub_imp_revenue =
            CorbaAlgs::unpack_decimal<RevenueDecimal>(
              impression_info.pub_imp_revenue);
        }

        RevenueDecimal pub_slot_imp_revenue_reminder;
        RevenueDecimal pub_slot_imp_revenue = RevenueDecimal::div(
          pub_imp_revenue,
          RevenueDecimal(false, impression_info.creatives.length(), 0ul),
          pub_slot_imp_revenue_reminder);
        pub_first_slot_revenue = pub_slot_imp_revenue +
          pub_slot_imp_revenue_reminder;
        pub_non_first_slot_revenue = pub_slot_imp_revenue;
      }
      else if (pub_imp_revenue_type == RT_SHARE)
      {
        pub_first_slot_revenue = CorbaAlgs::unpack_decimal<RevenueDecimal>(
          impression_info.pub_imp_revenue);
        pub_non_first_slot_revenue = pub_first_slot_revenue;
      }

      impression_result_info->creatives.length(impression_info.creatives.length());

      for(CORBA::ULong i = 0; i < impression_info.creatives.length(); ++i)
      {
        const Creative* creative = 0;

        AdServer::Commons::RequestId request_id =
          CorbaAlgs::unpack_request_id(
            impression_info.creatives[i].request_id);

        if(!request_id.is_null())
        {
          try
          {
            std::optional<RevenueDecimal> pub_imp_revenue;

            if (pub_imp_revenue_type != RT_NONE)
            {
              pub_imp_revenue =
                (i == 0 ? pub_first_slot_revenue : pub_non_first_slot_revenue);
            }

            campaign_manager_logger_->process_impression(
              CampaignManagerLogger::ImpressionInfo(
                imp_time,
                request_id,
                impression_info.user_id_hash_mod.defined ?
                  CampaignManagerLogger::UserIdHashMod(
                    impression_info.user_id_hash_mod.value) :
                CampaignManagerLogger::UserIdHashMod(),
                static_cast<RequestVerificationType>(impression_info.verify_type),
                pub_imp_revenue_type,
                pub_imp_revenue,
                CorbaAlgs::unpack_user_id(impression_info.user_id),
                impression_info.referer,
                impression_info.viewability,
                String::SubString(impression_info.action_name.in())));
          }
          catch (const eh::Exception& e)
          {
            logger_->sstream(Logging::Logger::NOTICE,
              Aspect::CAMPAIGN_MANAGER,
              "ADS-IMPL-183") <<
              FUN << ": eh::Exception caught while logging request " <<
              "with request id = '" <<
              request_id.to_string() <<
              "': " << e.what();
          }
        }

        if(impression_info.creatives[i].ccid)
        {
          const CcidMap::const_iterator creative_it =
            config->campaign_creatives.find(impression_info.creatives[i].ccid);

          if(creative_it != config->campaign_creatives.end())
          {
            creative = creative_it->second.in();
          }
        }

        if(creative)
        {
          impression_result_info->creatives[i].campaign_id = creative->campaign->campaign_group_id;
          impression_result_info->creatives[i].advertiser_id = creative->campaign->advertiser->account_id;
        }
      }

      if(impression_info.viewability <= 0 &&
        static_cast<RequestVerificationType>(
          impression_info.verify_type) == RVT_IMPRESSION)
      {
        // confirm impression amounts, only if viewability is -1 or 0
        // in other cases we sure that this is duplicate impression tracking request
        if (!config)
        {
          throw AdServer::CampaignSvcs::CampaignManager::NotReady(
            "Campaign configuration isn't loaded");
        }

        ConfirmCreativeAmountArray creatives;
        creatives.reserve(impression_info.creatives.length());
        for(CORBA::ULong cr_i = 0; cr_i < impression_info.creatives.length();
          ++cr_i)
        {
          if(impression_info.creatives[cr_i].ccid)
          {
            creatives.push_back(
              ConfirmCreativeAmount(
                impression_info.creatives[cr_i].ccid,
                CorbaAlgs::unpack_decimal<RevenueDecimal>(impression_info.creatives[cr_i].ctr)));
          }
        }

        confirm_amounts_(config, bid_time, creatives, CR_CPM);
      }
    }

    CampaignManagerImpl::VerifyImpressionResponsePtr
    CampaignManagerImpl::verify_impression(
      VerifyImpressionRequestPtr&& request)
    {
      static const char* FUN = "CampaignManagerImpl::verify_impression()";

      const auto id_request_grpc = request->id_request_grpc();
      try
      {
        const auto& impression_info = request->impression_info();

        auto response = create_grpc_response<Proto::VerifyImpressionResponse>(
          id_request_grpc);
        auto* info_response = response->mutable_info();
        auto* creatives_response = info_response->mutable_creatives();

        const Generics::Time imp_time = GrpcAlgs::unpack_time(impression_info.time());
        const Generics::Time bid_time = GrpcAlgs::unpack_time(impression_info.bid_time());

        CampaignConfig_var config = configuration(false);
        if (!config)
        {
          throw NotReady("Campaign configuration isn't loaded");
        }

        RevenueType pub_imp_revenue_type = static_cast<RevenueType>(
          impression_info.pub_imp_revenue_type());

        RevenueDecimal pub_first_slot_revenue;
        RevenueDecimal pub_non_first_slot_revenue;

        if(pub_imp_revenue_type == RT_ABSOLUTE)
        {
          RevenueDecimal pub_imp_revenue = RevenueDecimal::ZERO;

          if(static_cast<AdRequestType>(impression_info.request_type()) !=
             AR_NORMAL)
          {
            pub_imp_revenue =
              GrpcAlgs::unpack_decimal<RevenueDecimal>(
                impression_info.pub_imp_revenue());
          }

          RevenueDecimal pub_slot_imp_revenue_reminder;
          RevenueDecimal pub_slot_imp_revenue = RevenueDecimal::div(
            pub_imp_revenue,
            RevenueDecimal(false, impression_info.creatives().size(), 0ul),
            pub_slot_imp_revenue_reminder);
          pub_first_slot_revenue = pub_slot_imp_revenue +
            pub_slot_imp_revenue_reminder;
          pub_non_first_slot_revenue = pub_slot_imp_revenue;
        }
        else if (pub_imp_revenue_type == RT_SHARE)
        {
          pub_first_slot_revenue = GrpcAlgs::unpack_decimal<RevenueDecimal>(
            impression_info.pub_imp_revenue());
          pub_non_first_slot_revenue = pub_first_slot_revenue;
        }

        const auto& impression_info_creatives = impression_info.creatives();
        creatives_response->Reserve(impression_info_creatives.size());
        for(int i = 0; i < impression_info_creatives.size(); ++i)
        {
          const Creative* creative = nullptr;

          AdServer::Commons::RequestId request_id =
            GrpcAlgs::unpack_request_id(
              impression_info_creatives[i].request_id());

          if(!request_id.is_null())
          {
            try
            {
              std::optional<RevenueDecimal> pub_imp_revenue;

              if (pub_imp_revenue_type != RT_NONE)
              {
                pub_imp_revenue =
                  (i == 0 ? pub_first_slot_revenue : pub_non_first_slot_revenue);
              }

              campaign_manager_logger_->process_impression(
                CampaignManagerLogger::ImpressionInfo(
                  imp_time,
                  request_id,
                  impression_info.user_id_hash_mod().defined() ?
                    CampaignManagerLogger::UserIdHashMod(
                      impression_info.user_id_hash_mod().value()) :
                  CampaignManagerLogger::UserIdHashMod{},
                  static_cast<RequestVerificationType>(impression_info.verify_type()),
                  pub_imp_revenue_type,
                  pub_imp_revenue,
                  GrpcAlgs::unpack_user_id(impression_info.user_id()),
                  impression_info.referer().c_str(),
                  impression_info.viewability(),
                  String::SubString(impression_info.action_name())));
            }
            catch (const eh::Exception& e)
            {
              logger_->sstream(Logging::Logger::NOTICE,
                Aspect::CAMPAIGN_MANAGER,
                "ADS-IMPL-183") <<
                FUN << ": eh::Exception caught while logging request " <<
                "with request id = '" <<
                request_id.to_string() <<
                "': " << e.what();
            }
          }

          if(impression_info_creatives[i].ccid())
          {
            const auto creative_it =
              config->campaign_creatives.find(impression_info_creatives[i].ccid());

            if(creative_it != config->campaign_creatives.end())
            {
              creative = creative_it->second.in();
            }
          }

          if(creative)
          {
            auto* result = creatives_response->Add();
            result->set_campaign_id(creative->campaign->campaign_group_id);
            result->set_advertiser_id(creative->campaign->advertiser->account_id);
          }
        }

        if(impression_info.viewability() <= 0 &&
          static_cast<RequestVerificationType>(
            impression_info.verify_type()) == RVT_IMPRESSION)
        {
          // confirm impression amounts, only if viewability is -1 or 0
          // in other cases we sure that this is duplicate impression tracking request
          if (!config)
          {
            throw NotReady("Campaign configuration isn't loaded");
          }

          ConfirmCreativeAmountArray creatives;
          creatives.reserve(impression_info_creatives.size());
          for(int cr_i = 0; cr_i < impression_info_creatives.size(); ++cr_i)
          {
            if(impression_info_creatives[cr_i].ccid())
            {
              creatives.push_back(
                ConfirmCreativeAmount(
                  impression_info_creatives[cr_i].ccid(),
                  GrpcAlgs::unpack_decimal<RevenueDecimal>(impression_info_creatives[cr_i].ctr())));
            }
          }

          confirm_amounts_(config, bid_time, creatives, CR_CPM);
        }

        return response;
      }
      catch (const NotReady& exc)
      {
        Stream::Error stream;
        stream << FUN
               << "Caught CampaignManager::NotReady: "
               << exc.what();
        auto response = create_grpc_error_response<Proto::VerifyImpressionResponse>(
          Proto::Error_Type::Error_Type_NotReady,
          stream.str(),
          id_request_grpc);
        return response;
      }
      catch (const eh::Exception& exc)
      {
        Stream::Error stream;
        stream << FUN
               << ": "
               << exc.what();
        auto response = create_grpc_error_response<Proto::VerifyImpressionResponse>(
          Proto::Error_Type::Error_Type_Implementation,
          stream.str(),
          id_request_grpc);
        return response;
      }
      catch (...)
      {
        Stream::Error stream;
        stream << FUN
               << ": Unknown error";
        auto response = create_grpc_error_response<Proto::VerifyImpressionResponse>(
          Proto::Error_Type::Error_Type_Implementation,
          stream.str(),
          id_request_grpc);
        return response;
      }
    }

    void
    CampaignManagerImpl::action_taken(
      const AdServer::CampaignSvcs::CampaignManager::ActionInfo& action_info)
      /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException,
        AdServer::CampaignSvcs::CampaignManager::NotReady)*/
    {
      static const char* FUN = "CampaignManagerImpl::action_taken()";

      try
      {
        CampaignConfig_var config = configuration();
        if (!config)
        {
          throw AdServer::CampaignSvcs::CampaignManager::NotReady(
            "Campaign configuration isn't loaded");
        }

        AdServer::Commons::RequestId ar_id =
          AdServer::Commons::RequestId::create_random_based();

        CampaignManagerLogger::AdvActionInfo adv_action_info;
        adv_action_info.time = CorbaAlgs::unpack_time(action_info.time);
        adv_action_info.action_request_id = ar_id;
        adv_action_info.user_status = static_cast<UserStatus>(action_info.user_status);
        adv_action_info.user_id = CorbaAlgs::unpack_user_id(action_info.user_id);
        adv_action_info.log_as_test = action_info.log_as_test;
        if(action_info.location.length() > 0)
        {
          adv_action_info.country = action_info.location[0].country;
        }
        adv_action_info.referer = action_info.referer;
        adv_action_info.colo_id = campaign_manager_config_.colocation_id();
        adv_action_info.order_id = action_info.order_id;
        adv_action_info.ip_hash = action_info.ip_hash;
        if(action_info.action_value_defined)
        {
          adv_action_info.action_value = CorbaAlgs::unpack_decimal<RevenueDecimal>(
            action_info.action_value);
        }
        else
        {
          adv_action_info.action_value = RevenueDecimal::ZERO;
        }

        if(action_info.action_id_defined)
        {
          AdvActionMap::const_iterator adv_act_it =
            config->adv_actions.find(action_info.action_id);
          if(adv_act_it != config->adv_actions.end())
          {
            adv_action_info.action_id = action_info.action_id;
            adv_action_info.ccg_ids.assign(
              adv_act_it->second.ccg_ids.begin(),
              adv_act_it->second.ccg_ids.end());
            if(!action_info.action_value_defined)
            {
              adv_action_info.action_value = adv_act_it->second.cur_value;
            }
          }
        }

        {
          ChannelIdSet platform_channels;
          ChannelIdHashSet platforms(
            action_info.platform_ids.get_buffer(),
            action_info.platform_ids.get_buffer() + action_info.platform_ids.length());

          config->platform_channels->match(
            platform_channels,
            platforms);

          // fill adv_action_info.device_channel_id : device channel with great priority
          unsigned long cur_priority = 0;
          adv_action_info.device_channel_id = 0;

          for(ChannelIdSet::const_iterator pch_it = platform_channels.begin();
              pch_it != platform_channels.end();
              ++pch_it)
          {
            CampaignConfig::PlatformChannelPriorityMap::const_iterator pr_it =
              config->platform_channel_priorities.find(*pch_it);
            if(pr_it != config->platform_channel_priorities.end())
            {
              if(*adv_action_info.device_channel_id == 0 ||
                 cur_priority < pr_it->second.priority)
              {
                cur_priority = pr_it->second.priority;
                adv_action_info.device_channel_id = *pch_it;
              }
            }
          }
        }

        if(action_info.campaign_id_defined)
        {
          adv_action_info.ccg_ids.push_back(action_info.campaign_id);
        }

        produce_action_message_(action_info);

        campaign_manager_logger_->process_action(adv_action_info);
      }
      catch (const eh::Exception& e)
      {
        logger_->sstream(Logging::Logger::EMERGENCY,
          Aspect::CAMPAIGN_MANAGER,
          "ADS-IMPL-184") <<
          FUN << ": eh::Exception caught while logging request " <<
          "with user_id = '" << CorbaAlgs::unpack_user_id(
            action_info.user_id).to_string() <<
          "': " << e.what();
      }
    }

    CampaignManagerImpl::ActionTakenResponsePtr
    CampaignManagerImpl::action_taken(
      ActionTakenRequestPtr&& request)
    {
      const auto id_request_grpc = request->id_request_grpc();
      try
      {
        CampaignConfig_var config = configuration(false);
        if (!config)
        {
          throw NotReady("Campaign configuration isn't loaded");
        }

        const auto& action_info = request->action_info();

        AdServer::Commons::RequestId ar_id =
          AdServer::Commons::RequestId::create_random_based();

        CampaignManagerLogger::AdvActionInfo adv_action_info;
        adv_action_info.time = GrpcAlgs::unpack_time(action_info.time());
        adv_action_info.action_request_id = ar_id;
        adv_action_info.user_status = static_cast<UserStatus>(action_info.user_status());
        adv_action_info.user_id = GrpcAlgs::unpack_user_id(action_info.user_id());
        adv_action_info.log_as_test = action_info.log_as_test();
        if(action_info.location().size() > 0)
        {
          adv_action_info.country = action_info.location()[0].country();
        }
        adv_action_info.referer = action_info.referer();
        adv_action_info.colo_id = campaign_manager_config_.colocation_id();
        adv_action_info.order_id = action_info.order_id();
        adv_action_info.ip_hash = action_info.ip_hash();
        if(action_info.action_value_defined())
        {
          adv_action_info.action_value = GrpcAlgs::unpack_decimal<RevenueDecimal>(
            action_info.action_value());
        }
        else
        {
          adv_action_info.action_value = RevenueDecimal::ZERO;
        }

        if(action_info.action_id_defined())
        {
          AdvActionMap::const_iterator adv_act_it =
            config->adv_actions.find(action_info.action_id());
          if(adv_act_it != config->adv_actions.end())
          {
            adv_action_info.action_id = action_info.action_id();
            adv_action_info.ccg_ids.assign(
              adv_act_it->second.ccg_ids.begin(),
              adv_act_it->second.ccg_ids.end());
            if(!action_info.action_value_defined())
            {
              adv_action_info.action_value = adv_act_it->second.cur_value;
            }
          }
        }

        {
          ChannelIdSet platform_channels;
          ChannelIdHashSet platforms(
            action_info.platform_ids().begin(),
            action_info.platform_ids().end());

          config->platform_channels->match(
            platform_channels,
            platforms);

          // fill adv_action_info.device_channel_id : device channel with great priority
          unsigned long cur_priority = 0;
          adv_action_info.device_channel_id = 0;

          for(ChannelIdSet::const_iterator pch_it = platform_channels.begin();
              pch_it != platform_channels.end();
              ++pch_it)
          {
            CampaignConfig::PlatformChannelPriorityMap::const_iterator pr_it =
              config->platform_channel_priorities.find(*pch_it);
            if(pr_it != config->platform_channel_priorities.end())
            {
              if(*adv_action_info.device_channel_id == 0 ||
                 cur_priority < pr_it->second.priority)
              {
                cur_priority = pr_it->second.priority;
                adv_action_info.device_channel_id = *pch_it;
              }
            }
          }
        }

        if(action_info.campaign_id_defined())
        {
          adv_action_info.ccg_ids.push_back(action_info.campaign_id());
        }

        produce_action_message_(action_info);

        campaign_manager_logger_->process_action(adv_action_info);

        auto response = create_grpc_response<Proto::ActionTakenResponse>(
          id_request_grpc);
        response->mutable_info();

        return response;
      }
      catch (const NotReady& exc)
      {
        Stream::Error stream;
        stream << FNS
               << "Caught CampaignManager::NotReady: "
               << exc.what();
        auto response = create_grpc_error_response<Proto::ActionTakenResponse>(
          Proto::Error_Type::Error_Type_NotReady,
          stream.str(),
          id_request_grpc);
        return response;
      }
      catch (const eh::Exception& exc)
      {
        Stream::Error stream;
        stream << FNS
               << ": "
               << exc.what();
        auto response = create_grpc_error_response<Proto::ActionTakenResponse>(
          Proto::Error_Type::Error_Type_Implementation,
          stream.str(),
          id_request_grpc);
        return response;
      }
      catch (...)
      {
        Stream::Error stream;
        stream << FNS
               << ": Unknown error";
        auto response = create_grpc_error_response<Proto::ActionTakenResponse>(
          Proto::Error_Type::Error_Type_Implementation,
          stream.str(),
          id_request_grpc);
        return response;
      }
    }

    void CampaignManagerImpl::verify_opt_operation(
      CORBA::ULong time,
      CORBA::Long colo_id,
      const char* /*referer*/,
      AdServer::CampaignSvcs::CampaignManager::OptOperation operation,
      CORBA::ULong status,
      CORBA::ULong user_status,
      bool log_as_test,
      const char* browser,
      const char* os,
      const char* ct,
      const char* curct,
      const CORBACommons::UserIdInfo& user_id)
      /*throw(AdServer::CampaignSvcs::CampaignManager::NotReady)*/
    {
      static const char* FUN = "CampaignManagerImpl::verify_opt_operation()";

      try
      {
        CampaignConfig_var config = configuration();
        if (!config)
        {
          throw AdServer::CampaignSvcs::CampaignManager::NotReady(
            "Campaign configuration isn't loaded");
        }

        CampaignConfig::ColocationMap::const_iterator colo_it =
          colo_id <= 0 ? config->colocations.end() :
          config->colocations.find(colo_id);

        if (colo_it == config->colocations.end())
        {
          colo_it = config->colocations.find(
            campaign_manager_config_.colocation_id());

          colo_id = campaign_manager_config_.colocation_id();
        }

        if (colo_it != config->colocations.end())
        {
          if(operation == AdServer::CampaignSvcs::CampaignManager::OO_OUT ||
             operation == AdServer::CampaignSvcs::CampaignManager::OO_IN ||
             operation == AdServer::CampaignSvcs::CampaignManager::OO_FORCED_IN)
          {
            char oper;
            switch(operation)
            {
            case AdServer::CampaignSvcs::CampaignManager::OO_OUT:
              oper = 'O';
              break;
            case AdServer::CampaignSvcs::CampaignManager::OO_IN:
              oper = 'I';
              break;
            case AdServer::CampaignSvcs::CampaignManager::OO_FORCED_IN:
              oper = 'F';
              break;
            default:
              oper = 'E';
              break;
            };

            const Generics::Time time_offset = colo_it->second->account->time_offset;

            campaign_manager_logger_->process_oo_operation(
              colo_id,
              Generics::Time(time),
              time_offset,
              CorbaAlgs::unpack_user_id(user_id),
              log_as_test,
              oper);
          }

          if(operation != AdServer::CampaignSvcs::CampaignManager::OO_FORCED_IN)
          {
            WebOperationHash::const_iterator web_it = config->web_operations.find(
              WebOperationKey(
                "adserver",
                "oo",
                (operation == AdServer::CampaignSvcs::CampaignManager::OO_OUT ?
                 "out" : (
                   operation == AdServer::CampaignSvcs::CampaignManager::OO_IN ?
                   "in" : "status"))));
            if(web_it == config->web_operations.end())
            {
              throw Exception(
                "There isn't identificators for adserver 'oo' operations");
            }

            CampaignManagerLogger::WebOperationInfo web_op;
            web_op.init_by_flags(
              Generics::Time(time),
              ct,
              curct,
              browser,
              os,
              web_it->second->flags);
            web_op.colo_id = colo_id;
            web_op.tag_id = 0;
            web_op.cc_id = 0;
            web_op.web_operation_id = web_it->second->id;
            web_op.result = (status == 0 || status == 2 ? 'F' : 'S');
            web_op.user_status = static_cast<UserStatus>(user_status);

            web_op.test_request = log_as_test;

            campaign_manager_logger_->process_web_operation(web_op);
          }
        }
      }
      catch(const eh::Exception& e)
      {
        logger_->sstream(Logging::Logger::ERROR,
          Aspect::CAMPAIGN_MANAGER,
          "ADS-IMPL-185") <<
          FUN << ": Caught eh::Exception: " << e.what();
      }
    }

    CampaignManagerImpl::VerifyOptOperationResponsePtr
    CampaignManagerImpl::verify_opt_operation(
      VerifyOptOperationRequestPtr&& request)
    {
      const auto id_request_grpc = request->id_request_grpc();
      try
      {
        CampaignConfig_var config = configuration(false);
        if (!config)
        {
          throw NotReady("Campaign configuration isn't loaded");
        }

        auto colo_id = request->colo_id();
        const auto& time = request->time();
        const auto& operation = request->operation();
        const auto& status = request->status();
        const auto& user_status = request->user_status();
        const auto& log_as_test = request->log_as_test();
        const auto& browser = request->browser();
        const auto& os = request->os();
        const auto& ct = request->ct();
        const auto& curct = request->curct();
        const auto& user_id = request->user_id();

        auto colo_it = colo_id <= 0 ?
          config->colocations.end() : config->colocations.find(colo_id);

        if (colo_it == config->colocations.end())
        {
          colo_it = config->colocations.find(
            campaign_manager_config_.colocation_id());

          colo_id = campaign_manager_config_.colocation_id();
        }

        if (colo_it != config->colocations.end())
        {
          if(operation == Proto::OptOperation::OO_OUT ||
             operation == Proto::OptOperation::OO_IN ||
             operation == Proto::OptOperation::OO_FORCED_IN)
          {
            char oper;
            switch(operation)
            {
            case Proto::OptOperation::OO_OUT:
              oper = 'O';
              break;
            case Proto::OptOperation::OO_IN:
              oper = 'I';
              break;
            case Proto::OptOperation::OO_FORCED_IN:
              oper = 'F';
              break;
            default:
              oper = 'E';
              break;
            };

            const Generics::Time time_offset = colo_it->second->account->time_offset;

            campaign_manager_logger_->process_oo_operation(
              colo_id,
              Generics::Time(time),
              time_offset,
              GrpcAlgs::unpack_user_id(user_id),
              log_as_test,
              oper);
          }

          if(operation != Proto::OptOperation::OO_FORCED_IN)
          {
            WebOperationHash::const_iterator web_it = config->web_operations.find(
              WebOperationKey(
                "adserver",
                "oo",
                (operation == Proto::OptOperation::OO_OUT ?
                 "out" : (
                   operation == Proto::OptOperation::OO_IN ?
                   "in" : "status"))));
            if(web_it == config->web_operations.end())
            {
              throw Exception(
                "There isn't identificators for adserver 'oo' operations");
            }

            CampaignManagerLogger::WebOperationInfo web_op;
            web_op.init_by_flags(
              Generics::Time(time),
              ct.c_str(),
              curct.c_str(),
              browser.c_str(),
              os.c_str(),
              web_it->second->flags);
            web_op.colo_id = colo_id;
            web_op.tag_id = 0;
            web_op.cc_id = 0;
            web_op.web_operation_id = web_it->second->id;
            web_op.result = static_cast<std::uint32_t>(status == 0 || status == 2 ? 'F' : 'S');
            web_op.user_status = static_cast<UserStatus>(user_status);
            web_op.test_request = log_as_test;

            campaign_manager_logger_->process_web_operation(web_op);
          }
        }

        auto response = create_grpc_response<Proto::VerifyOptOperationResponse>(
          id_request_grpc);
        response->mutable_info();
        return response;
      }
      catch (const NotReady& exc)
      {
        Stream::Error stream;
        stream << FNS
               << "Caught CampaignManager::NotReady: "
               << exc.what();
        auto response = create_grpc_error_response<Proto::VerifyOptOperationResponse>(
          Proto::Error_Type::Error_Type_NotReady,
          stream.str(),
          id_request_grpc);
        return response;
      }
      catch (const eh::Exception& exc)
      {
        Stream::Error stream;
        stream << FNS
               << ": "
               << exc.what();
        auto response = create_grpc_error_response<Proto::VerifyOptOperationResponse>(
          Proto::Error_Type::Error_Type_Implementation,
          stream.str(),
          id_request_grpc);
        return response;
      }
      catch (...)
      {
        Stream::Error stream;
        stream << FNS
               << ": Unknown error";
        auto response = create_grpc_error_response<Proto::VerifyOptOperationResponse>(
          Proto::Error_Type::Error_Type_Implementation,
          stream.str(),
          id_request_grpc);
        return response;
      }
    }

    void CampaignManagerImpl::verify_vast_operation_(
      const AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
      const AdServer::CampaignSvcs::CampaignManager::AdSlotInfo& ad_slot,
      const Tag* tag,
      const Colocation* colocation)
      noexcept
    {
      static const char* FUN = "CampaignManagerImpl::verify_vast_operation_()";

      try
      {
        if (ad_slot.format == String::SubString(VAST_SOURCE))
        {
          CampaignConfig_var config = configuration();
          if (!config)
          {
            throw Exception("Campaign configuration isn't loaded");
          }
          
          WebOperationHash::const_iterator web_it = config->web_operations.find(
            WebOperationKey(
              VAST_APPLICATION,
              VAST_SOURCE,
              VAST_OPERATION));
          
          if(web_it == config->web_operations.end())
          {
            return;
          }
          // Fill ct
          std::string ct(String::StringManip::IntToStr(ad_slot.video_width).str().str());
          ct.push_back('x');
          String::StringManip::IntToStr(ad_slot.video_height).str().append_to(ct);
          
          CampaignManagerLogger::WebOperationInfo web_op;
          web_op.init_by_flags(
            CorbaAlgs::unpack_time(request_params.common_info.time),
            ct.c_str(),
            "", // curct
            "", // browser
            "", // os
            web_it->second->flags);

          web_op.colo_id = colocation->colo_id;
          web_op.tag_id = tag->tag_id;
          web_op.cc_id = 0;
          
          web_op.web_operation_id = web_it->second->id;
          web_op.app = VAST_APPLICATION;
          web_op.source = VAST_SOURCE;
          web_op.operation = VAST_OPERATION;
          web_op.result = 'S';
          web_op.user_status =
            static_cast<AdServer::CampaignSvcs::UserStatus>(request_params.common_info.user_status);
          web_op.test_request = request_params.common_info.log_as_test;
          campaign_manager_logger_->process_web_operation(web_op);
        }
      }
      catch(const eh::Exception& e)
      {
        logger_->sstream(Logging::Logger::ERROR,
          Aspect::CAMPAIGN_MANAGER,
          "ADS-IMPL-185") <<
          FUN << ": Caught eh::Exception: " << e.what();
      }
    }

    void CampaignManagerImpl::consider_web_operation(
      const AdServer::CampaignSvcs::CampaignManager::WebOperationInfo& web_op_info)
      /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException,
        AdServer::CampaignSvcs::CampaignManager::IncorrectArgument,
        AdServer::CampaignSvcs::CampaignManager::NotReady)*/
    {
      static const char* FUN = "CampaignManagerImpl::consider_web_operation()";

      try
      {
        CampaignConfig_var config = configuration();
        if (!config)
        {
          throw AdServer::CampaignSvcs::CampaignManager::NotReady(
            "Campaign configuration isn't loaded");
        }

        WebOperationHash::const_iterator web_it = config->web_operations.find(
          WebOperationKey(
            web_op_info.app.in(),
            web_op_info.source.in(),
            web_op_info.operation.in()));
        if(web_it == config->web_operations.end())
        {
          throw AdServer::CampaignSvcs::CampaignManager::IncorrectArgument();
        }

        unsigned long colo_id = web_op_info.colo_id;

        CampaignConfig::ColocationMap::const_iterator colo_it =
          colo_id == 0 ? config->colocations.end() :
          config->colocations.find(colo_id);

        if (colo_it == config->colocations.end())
        {
          colo_it = config->colocations.find(
            campaign_manager_config_.colocation_id());

          colo_id = campaign_manager_config_.colocation_id();
        }

        if (colo_it != config->colocations.end())
        {
          CampaignManagerLogger::WebOperationInfo web_op;
          web_op.init_by_flags(
            CorbaAlgs::unpack_time(web_op_info.time),
            web_op_info.ct,
            web_op_info.curct,
            web_op_info.browser,
            web_op_info.os,
            web_it->second->flags);
          web_op.global_request_id = CorbaAlgs::unpack_request_id(
            web_op_info.global_request_id);
          for(CORBA::ULong request_i = 0;
            request_i < web_op_info.request_ids.length();
            ++request_i)
          {
            web_op.request_ids.push_back(CorbaAlgs::unpack_request_id(
              web_op_info.request_ids[request_i]));
          }
          web_op.colo_id = colo_id;

          // filter inactive tags & ccid's
          if(web_op_info.tag_id && (web_it->second->flags &
             CampaignManagerLogger::WebOperationInfo::LOG_TAG_ID) &&
             config->tags.find(web_op_info.tag_id) != config->tags.end())
          {
            web_op.tag_id = web_op_info.tag_id;
          }
          else
          {
            web_op.tag_id = 0;
          }

          if(web_op_info.cc_id &&
             (web_it->second->flags &
               CampaignManagerLogger::WebOperationInfo::LOG_CC_ID) &&
             config->campaign_creatives.find(web_op_info.cc_id) !=
               config->campaign_creatives.end())
          {
            web_op.cc_id = web_op_info.cc_id;
          }
          else
          {
            web_op.cc_id = 0;
          }

          web_op.web_operation_id = web_it->second->id;
          web_op.app = web_op_info.app;
          web_op.source = web_op_info.source;
          web_op.operation = web_op_info.operation;
          web_op.result = web_op_info.result;
          web_op.user_status = static_cast<AdServer::CampaignSvcs::UserStatus>(
            web_op_info.user_status);
          web_op.test_request = web_op_info.test_request;
          web_op.user_bind_src = web_op_info.user_bind_src;
          web_op.referer = web_op_info.referer;
          web_op.ip_address = web_op_info.ip_address;
          web_op.external_user_id = web_op_info.external_user_id;
          web_op.user_agent = web_op_info.user_agent;

          campaign_manager_logger_->process_web_operation(web_op);
        }
      }
      catch(const eh::Exception& e)
      {
        logger_->sstream(Logging::Logger::ERROR,
          Aspect::CAMPAIGN_MANAGER,
          "ADS-IMPL-185") <<
          FUN << ": Caught eh::Exception: " << e.what();
      }
    }

    CampaignManagerImpl::ConsiderWebOperationResponsePtr
    CampaignManagerImpl::consider_web_operation(
      ConsiderWebOperationRequestPtr&& request)
    {
      const auto id_request_grpc = request->id_request_grpc();
      try
      {
        CampaignConfig_var config = configuration(false);
        if (!config)
        {
          throw NotReady("Campaign configuration isn't loaded");
        }

        const auto& web_op_info = request->web_op_info();

        WebOperationHash::const_iterator web_it = config->web_operations.find(
          WebOperationKey(
            web_op_info.app().c_str(),
            web_op_info.source().c_str(),
            web_op_info.operation().c_str()));
        if(web_it == config->web_operations.end())
        {
          throw IncorrectArgument("");
        }

        unsigned long colo_id = web_op_info.colo_id();

        auto colo_it = colo_id == 0 ? config->colocations.end() : config->colocations.find(colo_id);
        if (colo_it == config->colocations.end())
        {
          colo_it = config->colocations.find(
            campaign_manager_config_.colocation_id());

          colo_id = campaign_manager_config_.colocation_id();
        }

        if (colo_it != config->colocations.end())
        {
          CampaignManagerLogger::WebOperationInfo web_op;
          web_op.init_by_flags(
            GrpcAlgs::unpack_time(web_op_info.time()),
            web_op_info.ct().c_str(),
            web_op_info.curct().c_str(),
            web_op_info.browser().c_str(),
            web_op_info.os().c_str(),
            web_it->second->flags);
          web_op.global_request_id = GrpcAlgs::unpack_request_id(
            web_op_info.global_request_id());

          const auto& request_ids = web_op_info.request_ids();
          for(const auto& request_id : request_ids)
          {
            web_op.request_ids.emplace_back(
              GrpcAlgs::unpack_request_id(request_id));
          }
          web_op.colo_id = colo_id;

          // filter inactive tags & ccid's
          if(web_op_info.tag_id() && (web_it->second->flags &
             CampaignManagerLogger::WebOperationInfo::LOG_TAG_ID) &&
             config->tags.find(web_op_info.tag_id()) != config->tags.end())
          {
            web_op.tag_id = web_op_info.tag_id();
          }
          else
          {
            web_op.tag_id = 0;
          }

          if(web_op_info.cc_id() &&
             (web_it->second->flags &
               CampaignManagerLogger::WebOperationInfo::LOG_CC_ID) &&
             config->campaign_creatives.find(web_op_info.cc_id()) !=
               config->campaign_creatives.end())
          {
            web_op.cc_id = web_op_info.cc_id();
          }
          else
          {
            web_op.cc_id = 0;
          }

          web_op.web_operation_id = web_it->second->id;
          web_op.app = web_op_info.app();
          web_op.source = web_op_info.source();
          web_op.operation = web_op_info.operation();
          web_op.result = web_op_info.result();
          web_op.user_status = static_cast<AdServer::CampaignSvcs::UserStatus>(
            web_op_info.user_status());
          web_op.test_request = web_op_info.test_request();
          web_op.user_bind_src = web_op_info.user_bind_src();
          web_op.referer = web_op_info.referer();
          web_op.ip_address = web_op_info.ip_address();
          web_op.external_user_id = web_op_info.external_user_id();
          web_op.user_agent = web_op_info.user_agent();

          campaign_manager_logger_->process_web_operation(web_op);
        }

        auto response = create_grpc_response<Proto::ConsiderWebOperationResponse>(
          id_request_grpc);
        return response;
      }
      catch (const NotReady& exc)
      {
        Stream::Error stream;
        stream << FNS
               << "Caught CampaignManager::NotReady: "
               << exc.what();
        auto response = create_grpc_error_response<Proto::ConsiderWebOperationResponse>(
          Proto::Error_Type::Error_Type_NotReady,
          stream.str(),
          id_request_grpc);
        return response;
      }
      catch (const IncorrectArgument& exc)
      {
        Stream::Error stream;
        stream << FNS
               << "Caught CampaignManager::IncorrectArgument";
        auto response = create_grpc_error_response<Proto::ConsiderWebOperationResponse>(
          Proto::Error_Type::Error_Type_IncorrectArgument,
          stream.str(),
          id_request_grpc);
        return response;
      }
      catch (const eh::Exception& exc)
      {
        Stream::Error stream;
        stream << FNS
               << ": "
               << exc.what();
        auto response = create_grpc_error_response<Proto::ConsiderWebOperationResponse>(
          Proto::Error_Type::Error_Type_Implementation,
          stream.str(),
          id_request_grpc);
        return response;
      }
      catch (...)
      {
        Stream::Error stream;
        stream << FNS
               << ": Unknown error";
        auto response = create_grpc_error_response<Proto::ConsiderWebOperationResponse>(
          Proto::Error_Type::Error_Type_Implementation,
          stream.str(),
          id_request_grpc);
        return response;
      }
    }

    bool
    CampaignManagerImpl::fill_category_channel_node_(
      AdServer::CampaignSvcs::CampaignManager::CategoryChannelNodeInfo& res,
      const CategoryChannelNode* node,
      const char* language)
      noexcept
    {
      res.channel_id = node->channel_id;
      res.flags = node->flags;

      if(language[0] == 0)
      {
        res.name << node->name;
      }
      else
      {
        CategoryChannel::LocalizationMap::const_iterator loc_it =
          node->localizations.find(language);
        if(loc_it != node->localizations.end())
        {
          res.name << loc_it->second;
        }
        else
        {
          return false;
        }
      }

      res.child_category_channels.length(
        node->child_category_channels.size());
      CORBA::ULong i = 0;
      for(CategoryChannelNodeMap::const_iterator child_node_it =
            node->child_category_channels.begin();
          child_node_it != node->child_category_channels.end();
          ++child_node_it)
      {
        if(fill_category_channel_node_(
             res.child_category_channels[i], child_node_it->second, language))
        {
          ++i;
        }
      }
      res.child_category_channels.length(i);

      return true;
    }

    bool
    CampaignManagerImpl::fill_category_channel_node_(
      Proto::CategoryChannelNodeInfo& res,
      const CategoryChannelNode* node,
      const std::string& language) noexcept
    {
      try
      {
        res.set_channel_id(node->channel_id);
        res.set_flags(node->flags);

        if(language.empty())
        {
          res.set_name(node->name);
        }
        else
        {
          auto loc_it = node->localizations.find(language.c_str());
          if(loc_it != node->localizations.end())
          {
            res.set_name(loc_it->second);
          }
          else
          {
            return false;
          }
        }

        auto* child_category_channels = res.mutable_child_category_channels();
        child_category_channels->Reserve(node->child_category_channels.size());
        for(const auto& [_, value] : node->child_category_channels)
        {
          auto* category_channel_node_info= child_category_channels->Add();
          if(!fill_category_channel_node_(*category_channel_node_info, value, language))
          {
            child_category_channels->RemoveLast();
          }
        }

        return true;
      }
      catch (...)
      {
      }

      return false;
    }

    bool
    CampaignManagerImpl::fill_ad_slot_min_cpm_(
      AdSlotMinCpm& ad_slot_min_cpm,
      const CampaignConfig* campaign_config,
      const Tag* tag,
      const String::SubString& min_ecpm_currency_code,
      const CORBACommons::DecimalInfo& min_ecpm)
      noexcept
    {
      static const char* FUN = "CampaignManagerImpl::fill_ad_slot_min_cpm_()";

      assert(tag);
      
      try
      {
        // convert min_ecpm to tag currency
        RevenueDecimal adapted_min_ecpm = tag->skip_min_ecpm ?
          RevenueDecimal::ZERO :
          CorbaAlgs::unpack_decimal<RevenueDecimal>(min_ecpm);

        if(!min_ecpm_currency_code.empty() &&
          min_ecpm_currency_code != tag->site->account->currency->currency_code)
        {
          auto cur_it = campaign_config->currency_codes.find(
            min_ecpm_currency_code);

          if(cur_it == campaign_config->currency_codes.end())
          {
            // request with unknown currency can't be processed
            return false;
          }

          const Currency* min_ecpm_currency = cur_it->second.in();

          assert(min_ecpm_currency);

          adapted_min_ecpm = tag->site->account->currency->convert(
            min_ecpm_currency,
            adapted_min_ecpm);
        }

        const RevenueDecimal min_pub_ecpm = RevenueDecimal::div(
          adapted_min_ecpm,
          REVENUE_ONE + tag->cost_coef,
          Generics::DDR_CEIL);

        ad_slot_min_cpm.min_pub_ecpm_system = tag->site->account->currency->to_system_currency(
          min_pub_ecpm,
          Generics::DDR_CEIL);

        ad_slot_min_cpm.min_pub_ecpm = min_pub_ecpm;

        return true;
      }
      catch (const Generics::DecimalException& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": tag_id = '" << tag->tag_id <<
          "': min_ecpm processing error: " << e.what();
        
        logger_->log(ostr.str(),
          Logging::Logger::ERROR,
          Aspect::TRAFFICKING_PROBLEM,
          "ADS-IMPL-10340");

        return false;
      }
    }

    CampaignSvcs::AdRequestType
    CampaignManagerImpl::reduce_request_type_(
      CORBA::ULong request_type)
      noexcept
    {
      AdRequestType result_request_type =
        static_cast<CampaignSvcs::AdRequestType>(request_type);

      if(result_request_type == AR_OPENRTB_WITH_CLICKURL ||
          result_request_type == AR_LIVERAIL ||
          result_request_type == AR_ADRIVER)
      {
        return AR_OPENRTB;
      }

      return result_request_type;
    }

    void
    CampaignManagerImpl::convert_external_categories_(
      CreativeCategoryIdSet& exclude_categories,
      const CampaignConfig& config,
      const AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
      const AdServer::CampaignSvcs::CampaignManager::ExternalCreativeCategoryIdSeq& category_seq)
      noexcept
    {
      AdRequestType request_type = reduce_request_type_(
        request_params.common_info.request_type);

      CampaignConfig::ExternalCategoryMap::const_iterator req_ext_cat_it =
        config.external_creative_categories.find(request_type);

      if(req_ext_cat_it != config.external_creative_categories.end())
      {
        const CampaignConfig::ExternalCategoryNameMap& ext_categories =
          req_ext_cat_it->second;

        for(CORBA::ULong ext_cat_i = 0;
          ext_cat_i < category_seq.length(); ++ext_cat_i)
        {
          CampaignConfig::ExternalCategoryNameMap::const_iterator ext_cat_it =
            ext_categories.find(category_seq[ext_cat_i].in());
          if(ext_cat_it != ext_categories.end())
          {
            std::copy(ext_cat_it->second.begin(),
              ext_cat_it->second.end(),
              std::inserter(exclude_categories,
                exclude_categories.begin()));
          }
        }
      }
    }

    void
    CampaignManagerImpl::convert_external_categories_(
      CreativeCategoryIdSet& exclude_categories,
      const CampaignConfig& config,
      const Proto::RequestParams& request_params,
      const google::protobuf::RepeatedPtrField<std::string>& category_seq) noexcept
    {
      AdRequestType request_type = reduce_request_type_(
        request_params.common_info().request_type());

      const auto req_ext_cat_it = config.external_creative_categories.find(
        request_type);
      if(req_ext_cat_it != config.external_creative_categories.end())
      {
        const CampaignConfig::ExternalCategoryNameMap& ext_categories =
          req_ext_cat_it->second;

        for(const auto& category : category_seq)
        {
          const auto ext_cat_it = ext_categories.find(category);
          if(ext_cat_it != ext_categories.end())
          {
            std::copy(ext_cat_it->second.begin(),
              ext_cat_it->second.end(),
              std::inserter(exclude_categories,
                exclude_categories.begin()));
          }
        }
      }
    }

    void CampaignManagerImpl::convert_ccg_keywords_(
      const CampaignConfig* campaign_config,
      const Tag* tag,
      CampaignKeywordMap& result_keywords,
      const AdServer::CampaignSvcs::CampaignManager::CCGKeywordSeq& keywords,
      bool profiling_available,
      const FreqCapIdSet& full_freq_caps)
      noexcept
    {
      for(CORBA::ULong i = 0; i < keywords.length(); ++i)
      {
        const AdServer::CampaignSvcs::CampaignManager::CCGKeywordInfo&
          src_keyword = keywords[i];
        CampaignConfig::CampaignMap::const_iterator cmp_it =
          campaign_config->campaigns.find(src_keyword.ccg_id);
        if(cmp_it != campaign_config->campaigns.end())
        {
          bool blocked = true;

          CampaignConfig::ChannelMap::const_iterator ch_it =
            campaign_config->expression_channels.find(src_keyword.channel_id);
          if(ch_it != campaign_config->expression_channels.end())
          {
            blocked = ch_it->second->has_params() &&
              ch_it->second->params().common_params.in() &&
              ch_it->second->params().common_params->freq_cap_id && (
                full_freq_caps.find(ch_it->second->params().common_params->freq_cap_id) !=
                  full_freq_caps.end() ||
                !profiling_available);
          }

          if(!blocked)
          {
            blocked = !CampaignIndex::check_tag_domain_exclusion(
              String::SubString(src_keyword.click_url.in()),
              tag);
          }

          if(!blocked)
          {
            CampaignKeyword_var res_keyword(new CampaignKeyword);
            res_keyword->ccg_keyword_id = src_keyword.ccg_keyword_id;
            res_keyword->channel_id = src_keyword.channel_id;
            res_keyword->max_cpc =
              CorbaAlgs::unpack_decimal<RevenueDecimal>(src_keyword.max_cpc);

            res_keyword->ctr = std::min(
              RevenueDecimal::mul(
                CorbaAlgs::unpack_decimal<RevenueDecimal>(src_keyword.ctr),
                tag->adjustment,
                Generics::DMR_FLOOR),
              REVENUE_ONE);
            res_keyword->original_keyword = src_keyword.original_keyword;
            res_keyword->click_url = src_keyword.click_url;

            res_keyword->max_cpc = cmp_it->second->advertiser->adapt_cost(
              res_keyword->max_cpc,
              cmp_it->second->commision);

            try
            {
              res_keyword->ecpm = RevenueDecimal::mul(
                cmp_it->second->account->currency->to_system_currency(
                  RevenueDecimal::mul(
                    res_keyword->max_cpc,
                    res_keyword->ctr,
                    Generics::DMR_FLOOR)),
                ECPM_FACTOR,
                Generics::DMR_FLOOR);
            }
            catch(const RevenueDecimal::Overflow&)
            {
              res_keyword->ecpm = RevenueDecimal(
                std::numeric_limits<unsigned long>::max());
            }

            res_keyword->campaign = cmp_it->second;

            if(!res_keyword->click_url.empty())
            {
              try
              {
                HTTP::HTTPAddress http_url(res_keyword->click_url);
                domain_parser_->specific_domain(
                  http_url.host(), res_keyword->click_url_domain);
              }
              catch (const HTTP::URLAddress::InvalidURL& e)
              {}
            }

            result_keywords.insert(std::make_pair(src_keyword.ccg_id, res_keyword));
          }
        }
      }
    }

    void CampaignManagerImpl::convert_ccg_keywords_(
      const CampaignConfig* campaign_config,
      const Tag* tag,
      CampaignKeywordMap& result_keywords,
      const google::protobuf::RepeatedPtrField<Proto::CCGKeyword>& keywords,
      bool profiling_available,
      const FreqCapIdSet& full_freq_caps) noexcept
    {
      for(int i = 0; i < keywords.size(); ++i)
      {
        const Proto::CCGKeyword& src_keyword = keywords[i];
        CampaignConfig::CampaignMap::const_iterator cmp_it =
          campaign_config->campaigns.find(src_keyword.ccg_id());
        if(cmp_it != campaign_config->campaigns.end())
        {
          bool blocked = true;

          CampaignConfig::ChannelMap::const_iterator ch_it =
            campaign_config->expression_channels.find(src_keyword.channel_id());
          if(ch_it != campaign_config->expression_channels.end())
          {
            blocked = ch_it->second->has_params() &&
              ch_it->second->params().common_params.in() &&
              ch_it->second->params().common_params->freq_cap_id && (
                full_freq_caps.find(ch_it->second->params().common_params->freq_cap_id) !=
                  full_freq_caps.end() ||
                !profiling_available);
          }

          if(!blocked)
          {
            blocked = !CampaignIndex::check_tag_domain_exclusion(
              String::SubString(src_keyword.click_url()),
              tag);
          }

          if(!blocked)
          {
            CampaignKeyword_var res_keyword(new CampaignKeyword);
            res_keyword->ccg_keyword_id = src_keyword.ccg_keyword_id();
            res_keyword->channel_id = src_keyword.channel_id();
            res_keyword->max_cpc =
              GrpcAlgs::unpack_decimal<RevenueDecimal>(src_keyword.max_cpc());

            res_keyword->ctr = std::min(
              RevenueDecimal::mul(
                GrpcAlgs::unpack_decimal<RevenueDecimal>(src_keyword.ctr()),
                tag->adjustment,
                Generics::DMR_FLOOR),
              REVENUE_ONE);
            res_keyword->original_keyword = src_keyword.original_keyword();
            res_keyword->click_url = src_keyword.click_url();

            res_keyword->max_cpc = cmp_it->second->advertiser->adapt_cost(
              res_keyword->max_cpc,
              cmp_it->second->commision);

            try
            {
              res_keyword->ecpm = RevenueDecimal::mul(
                cmp_it->second->account->currency->to_system_currency(
                  RevenueDecimal::mul(
                    res_keyword->max_cpc,
                    res_keyword->ctr,
                    Generics::DMR_FLOOR)),
                ECPM_FACTOR,
                Generics::DMR_FLOOR);
            }
            catch(const RevenueDecimal::Overflow&)
            {
              res_keyword->ecpm = RevenueDecimal(
                std::numeric_limits<unsigned long>::max());
            }

            res_keyword->campaign = cmp_it->second;

            if(!res_keyword->click_url.empty())
            {
              try
              {
                HTTP::HTTPAddress http_url(res_keyword->click_url);
                domain_parser_->specific_domain(
                  http_url.host(), res_keyword->click_url_domain);
              }
              catch (const HTTP::URLAddress::InvalidURL& e)
              {}
            }

            result_keywords.insert(std::make_pair(src_keyword.ccg_id(), res_keyword));
          }
        }
      }
    }

    ColocationFlagsSeq*
    CampaignManagerImpl::get_colocation_flags()
      /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException,
        AdServer::CampaignSvcs::CampaignManager::NotReady)*/
    {
      static const char* FUN = "CampaignServerBaseImpl::get_colocation_flags()";

      try
      {
        CampaignConfig_var config = configuration();
        if (!config)
        {
          throw AdServer::CampaignSvcs::CampaignManager::NotReady(
            "Campaign configuration isn't loaded");
        }

        ColocationFlagsSeq_var result = new ColocationFlagsSeq();

        result->length(config->colocations.size());
        unsigned long real_size = 0;
        for (CampaignConfig::ColocationMap::const_iterator it =
          config->colocations.begin();
          it != config->colocations.end(); ++it)
        {
          ColocationFlags& colo = result[real_size++];
          colo.colo_id = it->first;
          colo.flags = it->second->ad_serving;
          colo.hid_profile = it->second->hid_profile;
        }
        result->length(real_size);

        return result._retn();
      }
      catch (const eh::Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": Caught eh::Exception: " << e.what();
        CORBACommons::throw_desc<
          CampaignSvcs::CampaignManager::ImplementationException>(
            ostr.str());
      }
      return 0; // never reach
    }

    CampaignManagerImpl::GetColocationFlagsResponsePtr
    CampaignManagerImpl::get_colocation_flags(
      GetColocationFlagsRequestPtr&& request)
    {
      const auto id_request_grpc = request->id_request_grpc();
      try
      {
        CampaignConfig_var config = configuration(false);
        if (!config)
        {
          throw NotReady("Campaign configuration isn't loaded");
        }

        auto response = create_grpc_response<Proto::GetColocationFlagsResponse>(
          id_request_grpc);
        auto* info_response = response->mutable_info();
        auto* colocation_flags_response = info_response->mutable_colocation_flags();

        colocation_flags_response->Reserve(config->colocations.size());
        for (const auto& [key, value] : config->colocations)
        {
          auto* data = colocation_flags_response->Add();
          data->set_colo_id(key);
          data->set_flags(value->ad_serving);
          data->set_hid_profile(value->hid_profile);
        }

        return response;
      }
      catch (const NotReady& exc)
      {
        Stream::Error stream;
        stream << FNS
               << "Caught CampaignManager::NotReady: "
               << exc.what();
        auto response = create_grpc_error_response<Proto::GetColocationFlagsResponse>(
          Proto::Error_Type::Error_Type_NotReady,
          stream.str(),
          id_request_grpc);
        return response;
      }
      catch (const eh::Exception& exc)
      {
        Stream::Error stream;
        stream << FNS
               << ": "
               << exc.what();
        auto response = create_grpc_error_response<Proto::GetColocationFlagsResponse>(
          Proto::Error_Type::Error_Type_Implementation,
          stream.str(),
          id_request_grpc);
        return response;
      }
      catch (...)
      {
        Stream::Error stream;
        stream << FNS
               << ": Unknown error";
        auto response = create_grpc_error_response<Proto::GetColocationFlagsResponse>(
          Proto::Error_Type::Error_Type_Implementation,
          stream.str(),
          id_request_grpc);
        return response;
      }
    }

    AdServer::CampaignSvcs::StringSeq*
    CampaignManagerImpl::get_pub_pixels(
      const char* country,
      CORBA::ULong user_status,
      const AdServer::CampaignSvcs::PublisherAccountIdSeq& publisher_account_ids)
      /*throw(AdServer::CampaignSvcs::CampaignManager::NotReady,
        AdServer::CampaignSvcs::CampaignManager::ImplementationException)*/
    {
      static const char* FUN = "CampaignServerBaseImpl::get_pub_pixels()";

      try
      {
        CampaignConfig_var config = configuration();

        if (!config)
        {
          throw AdServer::CampaignSvcs::CampaignManager::NotReady(
            "Campaign configuration isn't loaded");
        }

        StringSeq_var result = new StringSeq();
        AccountList result_accounts;

        if(publisher_account_ids.length() != 0)
        {
          // select by account id
          const PubPixelAccountMap::const_iterator it =
            find_pub_pixel_accounts_(config, country, static_cast<UserStatus>(user_status));

          if(it != config->pub_pixel_accounts.end())
          {
            Account_var test_account = new AccountDef;
            result->length(result->length() + publisher_account_ids.length());

            for(CORBA::ULong j = 0; j < publisher_account_ids.length(); ++j)
            {
              test_account->account_id = publisher_account_ids[j];

              AccountSet::const_iterator acc_it = it->second.find(test_account);
              if(acc_it != it->second.end())
              {
                result_accounts.push_back(*acc_it);
              }
            }
          }
        }
        else
        {
          get_pub_pixel_account_ids_(
            result_accounts,
            config,
            country,
            static_cast<UserStatus>(user_status),
            AccountIdSet(),
            MAX_PUBPIXELS_PER_REQUEST);
        }

        result->length(result_accounts.size());
        CORBA::ULong acc_i = 0;
        for(AccountList::const_iterator acc_it = result_accounts.begin();
            acc_it != result_accounts.end(); ++acc_it, ++acc_i)
        {
          (*result)[acc_i] << (
            static_cast<UserStatus>(user_status) == US_OPTIN ?
            (*acc_it)->pub_pixel_optin :
            (*acc_it)->pub_pixel_optout);
        }

        return result._retn();
      }
      catch (const eh::Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": Caught eh::Exception: " << e.what();
        CORBACommons::throw_desc<
          CampaignSvcs::CampaignManager::ImplementationException>(
            ostr.str());
      }

      return 0; // never reach
    }

    CampaignManagerImpl::GetPubPixelsResponsePtr
    CampaignManagerImpl::get_pub_pixels(GetPubPixelsRequestPtr&& request)
    {
      const auto id_request_grpc = request->id_request_grpc();
      try
      {
        CampaignConfig_var config = configuration(false);
        if (!config)
        {
          throw NotReady("Campaign configuration isn't loaded");
        }

        const auto& publisher_account_ids = request->publisher_account_ids();
        const auto& country = request->country();
        const auto& user_status = request->user_status();

        auto response = create_grpc_response<Proto::GetPubPixelsResponse>(
          id_request_grpc);
        auto* info_response = response->mutable_info();
        auto* pub_pixels_response = info_response->mutable_pub_pixels();

        AccountList result_accounts;
        if(!publisher_account_ids.empty())
        {
          const auto it = find_pub_pixel_accounts_(
            config,
            country.c_str(),
            static_cast<UserStatus>(user_status));

          if(it != config->pub_pixel_accounts.end())
          {
            Account_var test_account = new AccountDef;
            for(const auto& publisher_account_id : publisher_account_ids)
            {
              test_account->account_id = publisher_account_id;
              AccountSet::const_iterator acc_it = it->second.find(test_account);
              if(acc_it != it->second.end())
              {
                result_accounts.push_back(*acc_it);
              }
            }
          }
        }
        else
        {
          get_pub_pixel_account_ids_(
            result_accounts,
            config,
            country.c_str(),
            static_cast<UserStatus>(user_status),
            AccountIdSet{},
            MAX_PUBPIXELS_PER_REQUEST);
        }

        pub_pixels_response->Reserve(result_accounts.size());
        for(auto& result_account : result_accounts)
        {
          pub_pixels_response->Add(
            std::move(static_cast<UserStatus>(user_status) == US_OPTIN ?
              result_account->pub_pixel_optin :
              result_account->pub_pixel_optout));
        }

        return response;
      }
      catch (const NotReady& exc)
      {
        Stream::Error stream;
        stream << FNS
               << "Caught CampaignManager::NotReady: "
               << exc.what();
        auto response = create_grpc_error_response<Proto::GetPubPixelsResponse>(
          Proto::Error_Type::Error_Type_NotReady,
          stream.str(),
          id_request_grpc);
        return response;
      }
      catch (const eh::Exception& exc)
      {
        Stream::Error stream;
        stream << FNS
               << ": "
               << exc.what();
        auto response = create_grpc_error_response<Proto::GetPubPixelsResponse>(
          Proto::Error_Type::Error_Type_Implementation,
          stream.str(),
          id_request_grpc);
        return response;
      }
      catch (...)
      {
        Stream::Error stream;
        stream << FNS
               << ": Unknown error";
        auto response = create_grpc_error_response<Proto::GetPubPixelsResponse>(
          Proto::Error_Type::Error_Type_Implementation,
          stream.str(),
          id_request_grpc);
        return response;
      }
    }

    void
    CampaignManagerImpl::get_pub_pixel_account_ids_(
      AccountList& result_account_ids,
      const CampaignConfig* campaign_config,
      const char* country,
      UserStatus user_status,
      const AccountIdSet& exclude_publisher_account_ids,
      unsigned long limit)
      noexcept
    {
      const PubPixelAccountMap::const_iterator it =
        find_pub_pixel_accounts_(campaign_config, country, user_status);

      if (it != campaign_config->pub_pixel_accounts.end())
      {
        AccountSet::const_iterator acc_it = it->second.begin();
        AccountIdSet::const_iterator excl_acc_it =
          exclude_publisher_account_ids.begin();

        while(acc_it != it->second.end() &&
              excl_acc_it != exclude_publisher_account_ids.end())
        {
          if((*acc_it)->account_id < *excl_acc_it)
          {
            result_account_ids.push_back(*acc_it);
            ++acc_it;
          }
          else if(*excl_acc_it < (*acc_it)->account_id)
          {
            ++excl_acc_it;
          }
          else
          {
            ++acc_it;
            ++excl_acc_it;
          }
        }

        while(acc_it != it->second.end())
        {
          result_account_ids.push_back(*acc_it);
          ++acc_it;
        }
      }

      if (result_account_ids.size() > limit)
      {
        std::random_shuffle(result_account_ids.begin(), result_account_ids.end());
        result_account_ids.resize(limit);
      }
    }

    void
    CampaignManagerImpl::get_inst_optin_pub_pixel_account_ids_(
      AccountIdList& result_account_ids,
      const CampaignConfig* campaign_config,
      const Tag* tag,
      const AdServer::CampaignSvcs::CampaignManager::
        CommonAdRequestInfo& request_params,
      const AdServer::CampaignSvcs::
        PublisherAccountIdSeq& exclude_pubpixel_accounts_seq)
      noexcept
    {
      AccountIdSet exclude_pubpixel_accounts;
      CorbaAlgs::convert_sequence(
        exclude_pubpixel_accounts_seq,
        exclude_pubpixel_accounts);
      exclude_pubpixel_accounts.insert(tag->site->account->account_id);

      AccountList result_accounts;
      get_pub_pixel_account_ids_(
        result_accounts,
        campaign_config,
        request_params.location.length() ? request_params.location[0].country.in() : "",
        US_OPTIN,
        exclude_pubpixel_accounts,
        MAX_PUBPIXELS_PER_REQUEST);

      for(AccountList::const_iterator acc_it = result_accounts.begin();
          acc_it != result_accounts.end(); ++acc_it)
      {
        result_account_ids.push_back((*acc_it)->account_id);
      }
    }

    void
    CampaignManagerImpl::get_inst_optin_pub_pixel_account_ids_(
      AccountIdList& result_account_ids,
      const CampaignConfig* campaign_config,
      const Tag* tag,
      const Proto::CommonAdRequestInfo& request_params,
      const google::protobuf::RepeatedField<std::uint32_t>& exclude_pubpixel_accounts_seq) noexcept
    {
      AccountIdSet exclude_pubpixel_accounts(
        std::begin(exclude_pubpixel_accounts_seq),
        std::end(exclude_pubpixel_accounts_seq));

      exclude_pubpixel_accounts.insert(tag->site->account->account_id);

      AccountList result_accounts;
      get_pub_pixel_account_ids_(
        result_accounts,
        campaign_config,
        request_params.location().size() ? request_params.location()[0].country().c_str() : "",
        US_OPTIN,
        exclude_pubpixel_accounts,
        MAX_PUBPIXELS_PER_REQUEST);

      for(AccountList::const_iterator acc_it = result_accounts.begin();
          acc_it != result_accounts.end(); ++acc_it)
      {
        result_account_ids.push_back((*acc_it)->account_id);
      }
    }

    PubPixelAccountMap::const_iterator
    CampaignManagerImpl::find_pub_pixel_accounts_(
      const CampaignConfig* campaign_config,
      const char* country,
      UserStatus user_status) const
      noexcept
    {
      /*
       * if country_whitelist_ is set then pub_pixel_accounts is
       * precalculated for countries from whitelist.
       */
      const char* country_key = country_whitelist_.empty() ? country : "";
      return campaign_config->pub_pixel_accounts.find(
        PubPixelAccountKey(country_key, user_status));
    }

    bool
    CampaignManagerImpl::size_blacklisted_(
      const CampaignConfig* campaign_config,
      const ChannelIdHashSet& matched_channels,
      unsigned long size_id)
      noexcept
    {
      CampaignConfig::BlockChannelMap::const_iterator block_channel_list_it =
        campaign_config->block_channels.find(size_id);

      if(block_channel_list_it != campaign_config->block_channels.end())
      {
        for(CampaignConfig::ExpressionChannelHolderList::
              const_iterator block_channel_it =
                block_channel_list_it->second.begin();
            block_channel_it != block_channel_list_it->second.end();
            ++block_channel_it)
        {
          if((*block_channel_it)->triggered(&matched_channels, 0))
          {
            return true;
          }
        }
      }

      return false;
    }

    bool
    CampaignManagerImpl::apply_check_available_bid_result_(
      const Campaign* campaign,
      const GrpcBillingStateContainer::BidCheckResult& check_result,
      const RevenueDecimal& ctr)
      noexcept
    {
      /*
      std::cerr << "apply_check_available_bid_result_(): "
        "deactivate_account = " << check_result.deactivate_account <<
        ", deactivate_advertiser = " << check_result.deactivate_advertiser <<
        ", deactivate_campaign = " << check_result.deactivate_campaign <<
        ", deactivate_ccg = " << check_result.deactivate_ccg <<
        ", available = " << check_result.available <<
        std::endl;
      */

      if(check_result.deactivate_account)
      {
        campaign->account->set_available(false);
      }

      if(check_result.deactivate_advertiser)
      {
        campaign->advertiser->set_available(false);
      }

      /*
      if(check_result.deactivate_account ||
        check_result.deactivate_advertiser ||
        check_result.deactivate_campaign ||
        check_result.deactivate_ccg)
      {
        campaign->set_available(false, check_result.goal_ctr);
      }
      */

      return check_result.available && ctr >= check_result.goal_ctr;
    }

    void
    CampaignManagerImpl::confirm_amounts_(
      const CampaignConfig* config,
      const Generics::Time& now,
      const ConfirmCreativeAmountArray& creatives,
      CCGRateType rate_type)
      noexcept
    {
      if(confirm_billing_state_container_)
      {
        // resolve id's and rate
        for(ConfirmCreativeAmountArray::const_iterator cc_it = creatives.begin();
          cc_it != creatives.end(); ++cc_it)
        {
          const Creative* creative = 0;

          CcidMap::const_iterator cr_it = config->campaign_creatives.find(cc_it->cc_id);
          if(cr_it != config->campaign_creatives.end())
          {
            creative = cr_it->second.in();
          }

          // account can be non linked if deactivated directly
          if(creative && creative->campaign && creative->campaign->account.in())
          {
            const Campaign* campaign = creative->campaign;
            assert(campaign);

            RevenueDecimal amount = RevenueDecimal::ZERO;
            if(rate_type == CR_CPM || rate_type == CR_MAXBID)
            {
              amount = campaign->imp_revenue;
            }
            else if(rate_type == CR_CPC)
            {
              amount = campaign->click_revenue;
            }
            else if(rate_type == CR_CPA)
            {
              amount = campaign->action_revenue;
            }
            else
            {
              assert(0);
            }

            // eval comm amount
            RevenueDecimal comm_amount = RevenueDecimal::ZERO;

            if(campaign->commision != RevenueDecimal::ZERO)
            {
              if(campaign->account->cost_is_gross())
              {
                comm_amount = amount - RevenueDecimal::mul(
                  amount,
                  REVENUE_ONE - campaign->commision,
                  Generics::DMR_FLOOR);

                amount -= comm_amount;
              }
              else
              {
                comm_amount = RevenueDecimal::div(
                  RevenueDecimal::mul(
                    amount,
                    campaign->commision,
                    Generics::DMR_FLOOR),
                  REVENUE_ONE - campaign->commision); // += comm_amount
              }
            }

            const GrpcBillingStateContainer::BidCheckResult check_result =
              confirm_billing_state_container_->confirm_bid(
                now,
                campaign->advertiser ? campaign->advertiser->bill_account_id() : 0,
                campaign->advertiser ? campaign->advertiser->not_bill_account_id() : 0,
                campaign->campaign_group_id,
                campaign->campaign_id,
                campaign->account && campaign->account->invoice_commision() ?
                  amount + comm_amount : amount, // account amount
                campaign->account && campaign->account->cost_is_gross() ?
                  amount + comm_amount : amount,
                cc_it->ctr,
                RevenueDecimal(false, rate_type == CR_CPM || rate_type == CR_MAXBID ? 1 : 0, 0),
                RevenueDecimal(false, rate_type == CR_CPC ? 1 : 0, 0),
                campaign);

            apply_check_available_bid_result_(
              campaign, check_result, RevenueDecimal::ZERO);
          }
        }
      } // if(confirm_billing_state_container_)
    }

    void
    CampaignManagerImpl::fill_tns_counter_device_type_(
      std::string& tns_counter_device_type,
      const StringSet& norm_platform_names)
      noexcept
    {
      if(norm_platform_names.find(PlatformNames::APPLE_IPADS) != norm_platform_names.end() ||
        norm_platform_names.find(PlatformNames::APPLE_IPHONES) != norm_platform_names.end())
      {
        tns_counter_device_type = "2";
      }
      else if(norm_platform_names.find(PlatformNames::ANDROID_TABLETS) != norm_platform_names.end() ||
        norm_platform_names.find(PlatformNames::ANDROID_SMARTPHONES) != norm_platform_names.end())
      {
        tns_counter_device_type = "3";
      }
      else if(norm_platform_names.find(PlatformNames::WINDOWS_MOBILE) != norm_platform_names.end())
      {
        tns_counter_device_type = "4";
      }
      else if(norm_platform_names.find(PlatformNames::SMARTTV) != norm_platform_names.end())
      {
        tns_counter_device_type = "5";
      }
      else if(norm_platform_names.find(PlatformNames::DVR) != norm_platform_names.end())
      {
        tns_counter_device_type = "6";
      }
      else if(norm_platform_names.find(PlatformNames::NON_MOBILE_DEVICES) != norm_platform_names.end())
      {
        tns_counter_device_type = "1";
      }
      else
      {
        tns_counter_device_type = "7";
      }
    }

    void
    CampaignManagerImpl::produce_ads_space_message_impl_(
      const Generics::Time& request_time,
      const AdServer::Commons::UserId& user_id_orig,
      unsigned long request_tag_id,
      const String::SubString& referer,
      const AdServer::CampaignSvcs::CampaignManager::AdSlotSeq* ad_slots,
      const AdServer::CampaignSvcs::ULongSeq* publisher_account_ids,
      const String::SubString& peer_ip,
      const AdServer::CampaignSvcs::CampaignManager::GeoInfoSeq& location,
      const String::SubString& ssp_location,
      const RevenueDecimal& adsspace_system_cpm,
      const String::SubString& external_user_id)
    {
      std::string user_id(user_id_orig.to_string(false));

      std::string csv_encoded_ref;
      String::StringManip::csv_encode(referer.str().c_str(), csv_encoded_ref);

      char usec_str[40];
      size_t usec_str_size = String::StringManip::int_to_str(
        request_time.tv_usec, usec_str + 6, sizeof(usec_str) - 6);
      ::memset(usec_str + usec_str_size, '0', 6 - usec_str_size);

      char tag_id_str[40];
      size_t tag_id_str_size = String::StringManip::int_to_str(
        request_tag_id, tag_id_str, sizeof(tag_id_str));

      std::string record;
      record.reserve(1024);
      record += request_time.gm_ft();
      record += '.';
      record.append(usec_str + usec_str_size, 6);
      record += ',';
      record += user_id;
      record += ',';
      record.append(tag_id_str, tag_id_str_size);
      record += ',';
      record += csv_encoded_ref;
      record += ',';
      record += adsspace_system_cpm.str();
      record += ',';

      if(publisher_account_ids)
      {
        char acc_id_str[40];
        for(CORBA::ULong acc_i = 0;
          acc_i < publisher_account_ids->length();
          ++acc_i)
        {
          if(acc_i != 0)
          {
            record += ";";
          }

          size_t acc_id_str_size = String::StringManip::int_to_str(
            (*publisher_account_ids)[acc_i], acc_id_str, sizeof(acc_id_str));
          record.append(acc_id_str, acc_id_str_size);
        }
      }

      record += ',';

      if(ad_slots)
      {
        bool first_size = true;
        for(CORBA::ULong ad_slot_i = 0; ad_slot_i < ad_slots->length(); ++ad_slot_i)
        {
          for(CORBA::ULong size_i = 0;
              size_i < (*ad_slots)[ad_slot_i].sizes.length(); ++size_i)
          {
            if(first_size)
            {
              first_size = false;
            }
            else
            {
              record += ';';
            }

            record += (*ad_slots)[ad_slot_i].sizes[size_i];
          }
        }
      }

      record += ',';
      record += peer_ip.str();
      record += ',';
      
      for(CORBA::ULong loc_i = 0; loc_i < location.length(); ++loc_i)
      {
        if(loc_i > 0)
        {
          record += ';';
        }
        record += location[loc_i].country;
        record += '/';
        record += location[loc_i].region;
        record += '/';
        record += location[loc_i].city;
      }

      record += ',';
      record += ssp_location.str();
      record += ',';
      record += external_user_id.str();

      kafka_producer_->push_data(user_id, record);
    }

    void
    CampaignManagerImpl::produce_ads_space_message_impl_(
      const Generics::Time& request_time,
      const AdServer::Commons::UserId& user_id_orig,
      unsigned long request_tag_id,
      const String::SubString& referer,
      const google::protobuf::RepeatedPtrField<Proto::AdSlotInfo>* ad_slots,
      const google::protobuf::RepeatedField<std::uint32_t>* publisher_account_ids,
      const String::SubString& peer_ip,
      const google::protobuf::RepeatedPtrField<Proto::GeoInfo>& location,
      const String::SubString& ssp_location,
      const RevenueDecimal& adsspace_system_cpm,
      const String::SubString& external_user_id)
    {
      std::string user_id(user_id_orig.to_string(false));

      std::string csv_encoded_ref;
      String::StringManip::csv_encode(referer.str().c_str(), csv_encoded_ref);

      char usec_str[40];
      size_t usec_str_size = String::StringManip::int_to_str(
        request_time.tv_usec, usec_str + 6, sizeof(usec_str) - 6);
      ::memset(usec_str + usec_str_size, '0', 6 - usec_str_size);

      char tag_id_str[40];
      size_t tag_id_str_size = String::StringManip::int_to_str(
        request_tag_id, tag_id_str, sizeof(tag_id_str));

      std::string record;
      record.reserve(1024);
      record += request_time.gm_ft();
      record += '.';
      record.append(usec_str + usec_str_size, 6);
      record += ',';
      record += user_id;
      record += ',';
      record.append(tag_id_str, tag_id_str_size);
      record += ',';
      record += csv_encoded_ref;
      record += ',';
      record += adsspace_system_cpm.str();
      record += ',';

      if(publisher_account_ids)
      {
        char acc_id_str[40];
        for(int acc_i = 0;
            acc_i < publisher_account_ids->size();
            ++acc_i)
        {
          if(acc_i != 0)
          {
            record += ";";
          }

          size_t acc_id_str_size = String::StringManip::int_to_str(
            (*publisher_account_ids)[acc_i], acc_id_str, sizeof(acc_id_str));
          record.append(acc_id_str, acc_id_str_size);
        }
      }

      record += ',';

      if(ad_slots)
      {
        bool first_size = true;
        for(int ad_slot_i = 0; ad_slot_i < ad_slots->size(); ++ad_slot_i)
        {
          const auto& sizes = (*ad_slots)[ad_slot_i].sizes();
          for(int size_i = 0; size_i < sizes.size(); ++size_i)
          {
            if(first_size)
            {
              first_size = false;
            }
            else
            {
              record += ';';
            }

            record += sizes[size_i];
          }
        }
      }

      record += ',';
      record += peer_ip.str();
      record += ',';

      for(int loc_i = 0; loc_i < location.size(); ++loc_i)
      {
        if(loc_i > 0)
        {
          record += ';';
        }
        record += location[loc_i].country();
        record += '/';
        record += location[loc_i].region();
        record += '/';
        record += location[loc_i].city();
      }

      record += ',';
      record += ssp_location.str();
      record += ',';
      record += external_user_id.str();

      kafka_producer_->push_data(user_id, record);
    }

    void
    CampaignManagerImpl::produce_action_message_(
      const AdServer::CampaignSvcs::CampaignManager::ActionInfo& action_info)
    {
      if(kafka_producer_)
      {
        if(action_info.referer[0])
        {
          produce_ads_space_message_impl_(
            CorbaAlgs::unpack_time(action_info.time),
            CorbaAlgs::unpack_user_id(action_info.user_id),
            0, // tag_id
            String::SubString(action_info.referer),
            0, // ad_slots
            0, // publisher_account_ids
            String::SubString(action_info.peer_ip),
            action_info.location,
            String::SubString(),
            RevenueDecimal::ZERO,
            String::SubString());
        }

        char action_id_str[40];
        size_t action_id_size = String::StringManip::int_to_str(
          action_info.action_id, action_id_str, sizeof(action_id_str));

        std::string act_ref("act-");
        act_ref.append(action_id_str, action_id_size);

        produce_ads_space_message_impl_(
          CorbaAlgs::unpack_time(action_info.time),
          CorbaAlgs::unpack_user_id(action_info.user_id),
          0, // tag_id
          act_ref,
          0, // ad_slots
          0, // publisher_account_ids
          String::SubString(action_info.peer_ip),
          action_info.location,
          String::SubString(),
          RevenueDecimal::ZERO,
          String::SubString());
      }
    }

    void
    CampaignManagerImpl::produce_action_message_(
      const Proto::ActionInfo& action_info)
    {
      if(kafka_producer_)
      {
        if(!action_info.referer().empty())
        {
          produce_ads_space_message_impl_(
            GrpcAlgs::unpack_time(action_info.time()),
            GrpcAlgs::unpack_user_id(action_info.user_id()),
            0, // tag_id
            String::SubString(action_info.referer()),
            nullptr, // ad_slots
            nullptr, // publisher_account_ids
            String::SubString(action_info.peer_ip()),
            action_info.location(),
            String::SubString{},
            RevenueDecimal::ZERO,
            String::SubString{});
        }

        char action_id_str[40];
        size_t action_id_size = String::StringManip::int_to_str(
          action_info.action_id(), action_id_str, sizeof(action_id_str));

        std::string act_ref("act-");
        act_ref.append(action_id_str, action_id_size);

        produce_ads_space_message_impl_(
          GrpcAlgs::unpack_time(action_info.time()),
          GrpcAlgs::unpack_user_id(action_info.user_id()),
          0, // tag_id
          act_ref,
          nullptr, // ad_slots
          nullptr, // publisher_account_ids
          String::SubString(action_info.peer_ip()),
          action_info.location(),
          String::SubString{},
          RevenueDecimal::ZERO,
          String::SubString{});
      }
    }

    void
    CampaignManagerImpl::produce_ads_space_(
      const AdServer::CampaignSvcs::CampaignManager::
        RequestParams& request_params,
      unsigned long request_tag_id,
      const RevenueDecimal& adsspace_system_cpm)
    {
      if(kafka_producer_)
      {
        produce_ads_space_message_impl_(
          CorbaAlgs::unpack_time(request_params.common_info.time),
          CorbaAlgs::unpack_user_id(request_params.common_info.user_id),
          request_tag_id,
          String::SubString(request_params.common_info.referer),
          &request_params.ad_slots,
          &request_params.publisher_account_ids,
          String::SubString(request_params.common_info.peer_ip),
          request_params.common_info.location,
          String::SubString(request_params.ssp_location),
          adsspace_system_cpm,
          String::SubString(request_params.common_info.external_user_id));
      }
    }

    void
    CampaignManagerImpl::produce_ads_space_(
      const Proto::RequestParams& request_params,
      unsigned long request_tag_id,
      const RevenueDecimal& adsspace_system_cpm)
    {
      if(kafka_producer_)
      {
        produce_ads_space_message_impl_(
          GrpcAlgs::unpack_time(request_params.common_info().time()),
          GrpcAlgs::unpack_user_id(request_params.common_info().user_id()),
          request_tag_id,
          String::SubString(request_params.common_info().referer()),
          &request_params.ad_slots(),
          &request_params.publisher_account_ids(),
          String::SubString(request_params.common_info().peer_ip()),
          request_params.common_info().location(),
          String::SubString(request_params.ssp_location()),
          adsspace_system_cpm,
          String::SubString(request_params.common_info().external_user_id()));
      }
    }

    void
    CampaignManagerImpl::produce_match_(
      const AdServer::CampaignSvcs::CampaignManager::MatchRequestInfo&
        request_params)
    {
      if (kafka_match_producer_ &&
        request_params.match_info.full_referer[0])
      {
        Generics::Time request_time(
          CorbaAlgs::unpack_time(request_params.request_time));

        std::string user_id(
          CorbaAlgs::unpack_user_id(
            request_params.user_id).to_string(false));

        std::string csv_encoded_ref;
        String::StringManip::csv_encode(request_params.match_info.full_referer.in(), csv_encoded_ref);

        std::string csv_encoded_source;
        String::StringManip::csv_encode(request_params.source.in(), csv_encoded_source);

        char usec_str[40];
        size_t usec_str_size = String::StringManip::int_to_str(
          request_time.tv_usec, usec_str + 6, sizeof(usec_str) - 6);
        ::memset(usec_str + usec_str_size, '0', 6 - usec_str_size);

        std::string record;
        record.reserve(1024);
        record += request_time.gm_ft();
        record += '.';
        record.append(usec_str + usec_str_size, 6);
        record += ',';
        record += user_id;
        record += ',';
        record += csv_encoded_ref;
        record += ',';
        record += csv_encoded_source;

        kafka_match_producer_->push_data(
          user_id,
          record);
      }
    }

    void
    CampaignManagerImpl::produce_match_(const Proto::MatchRequestInfo& request_params)
    {
      if (kafka_match_producer_ &&
          !request_params.match_info().full_referer().empty())
      {
        Generics::Time request_time(
          GrpcAlgs::unpack_time(request_params.request_time()));

        std::string user_id(
          GrpcAlgs::unpack_user_id(
            request_params.user_id()).to_string(false));

        std::string csv_encoded_ref;
        String::StringManip::csv_encode(request_params.match_info().full_referer().c_str(), csv_encoded_ref);

        std::string csv_encoded_source;
        String::StringManip::csv_encode(request_params.source().c_str(), csv_encoded_source);

        char usec_str[40];
        size_t usec_str_size = String::StringManip::int_to_str(
          request_time.tv_usec, usec_str + 6, sizeof(usec_str) - 6);
        ::memset(usec_str + usec_str_size, '0', 6 - usec_str_size);

        std::string record;
        record.reserve(1024);
        record += request_time.gm_ft();
        record += '.';
        record.append(usec_str + usec_str_size, 6);
        record += ',';
        record += user_id;
        record += ',';
        record += csv_encoded_ref;
        record += ',';
        record += csv_encoded_source;

        kafka_match_producer_->push_data(
          user_id,
          record);
      }
    }

  } // namespace CampaignSvcs
} // namespace AdServer
