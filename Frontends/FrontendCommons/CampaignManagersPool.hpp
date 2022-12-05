/**
 * @file CampaignManagersPool.hpp
 */
#ifndef CAMPAIGN_MANAGER_HOOK_HPP
#define CAMPAIGN_MANAGER_HOOK_HPP

#include <Commons/CorbaAlgs.hpp>
#include <Commons/CorbaConfig.hpp>
#include <Commons/UserInfoManip.hpp>

#include <CORBACommons/ObjectPool.hpp>

#include <xsd/Frontends/FeConfig.hpp>

#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>
#include <CampaignSvcs/CampaignManager/CampaignManager.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignSvcsVersionAdapter.hpp>

namespace FrontendCommons
{
  namespace Configuration
  {
    using namespace xsd::AdServer::Configuration;
  }

  /**
   * Allow resolving Campaign manager CORBA object
   */
  template <typename Exception>
  class CampaignManagersPool
  {
    typedef Configuration::CommonFeConfigurationType
      CommonFeConfiguration;
  public:
    CampaignManagersPool(
      Logging::Logger* logger,
      const char* aspect) noexcept
      : logger_(ReferenceCounting::add_ref(logger)),
        ASPECT_(aspect)
    {}

    void
    resolve(
      const CommonFeConfiguration& common_config,
      const CORBACommons::CorbaClientAdapter* corba_client_adapter)
      /*throw(Exception)*/;

    void
    resolve(
      const CORBACommons::CorbaObjectRefList& campaign_managers_refs,
      const CORBACommons::CorbaClientAdapter* corba_client_adapter)
      /*throw(Exception)*/;

    void
    get_campaign_creative(
      const AdServer::CampaignSvcs::CampaignManager::RequestParams&
        request_params,
      CORBA::String_var& hostname,
      AdServer::CampaignSvcs::CampaignManager::RequestCreativeResult_out
        campaign_matching_result)
      /*throw(Exception)*/;

    void
    match_geo_channels(
      const AdServer::CampaignSvcs::CampaignManager::GeoInfoSeq& location,
      const AdServer::CampaignSvcs::CampaignManager::GeoCoordInfoSeq& coord_location,
      AdServer::CampaignSvcs::ChannelIdSeq_out geo_channels_result,
      AdServer::CampaignSvcs::ChannelIdSeq_out coord_channels_result)
      /*throw(Exception)*/;

    void
    instantiate_ad(
      const AdServer::CampaignSvcs::CampaignManager::InstantiateAdInfo&
        instantiate_ad_info,
      AdServer::CampaignSvcs::CampaignManager::InstantiateAdResult_out
        instantiate_ad_result,
      const char* service_id)
      /*throw(Exception)*/;

    AdServer::CampaignSvcs::CampaignManager::DiscoverChannelResultSeq_var
    get_discover_channels(
      const AdServer::CampaignSvcs::ChannelWeightSeq& channels,
      const char* country,
      const char* language,
      bool all) /*throw(Exception)*/;

    AdServer::CampaignSvcs::CampaignManager::CategoryChannelNodeSeq_var
    get_category_channels(const char* language) /*throw(Exception)*/;

    void
    consider_passback(
     const AdServer::CampaignSvcs::CampaignManager::PassbackInfo& info)
      /*throw(Exception)*/;

    void
    consider_passback_track(
      const AdServer::CampaignSvcs::CampaignManager::PassbackTrackInfo& info)
      /*throw(Exception)*/;

    bool
    get_click_url(
      const AdServer::CampaignSvcs::CampaignManager::ClickInfo& click_info,
      AdServer::CampaignSvcs::CampaignManager::ClickResultInfo_var& click_result_info,
      const char* service_id)
      /*throw(Exception)*/;

    void
    verify_opt_operation(
      unsigned long time,
      long colo_id,
      const char* referer,
      const AdServer::CampaignSvcs::CampaignManager::OptOperation& operation,
      unsigned long status,
      unsigned long user_status,
      bool log_as_test,
      const char* browser,
      const char* os,
      const char* ct,
      const char* curct,
      const char* local_aspect = 0,
      const AdServer::Commons::UserId& user_id = AdServer::Commons::UserId())
      /*throw(Exception)*/;

    void
    consider_web_operation(
      const AdServer::CampaignSvcs::CampaignManager::WebOperationInfo&
        web_op_info)
      /*throw(AdServer::CampaignSvcs::CampaignManager::IncorrectArgument,
        Exception)*/;

    void
    action_taken(
      const AdServer::CampaignSvcs::CampaignManager::ActionInfo& action_info)
      /*throw(Exception)*/;

    void
    verify_impression(
      const AdServer::CampaignSvcs::CampaignManager::ImpressionInfo&
        impression_info,
      AdServer::CampaignSvcs::CampaignManager::ImpressionResultInfo_var&
        impression_result_info)
      /*throw(Exception)*/;

    AdServer::CampaignSvcs::ColocationFlagsSeq_var
    get_colocation_flags()
      /*throw(Exception)*/;

    AdServer::CampaignSvcs::StringSeq_var
    get_pub_pixels(
      const char* country,
      unsigned long user_status,
      const AdServer::CampaignSvcs::PublisherAccountIdSeq& publisher_account_ids)
      /*throw(Exception)*/;

    void
    process_anonymous_request(
      const AdServer::CampaignSvcs::CampaignManager::AnonymousRequestInfo&
        request_info)
      /*throw(Exception)*/;

    void
    process_match_request(
      const AdServer::CampaignSvcs::CampaignManager::MatchRequestInfo&
        match_request_info)
      /*throw(Exception)*/;

    void
    get_file(
      const char* file_name,
      CORBACommons::OctSeq_var& content,
      const char* service_id)
      /*throw(Exception)*/;

  private:
    typedef CORBACommons::ObjectPoolRefConfiguration
      CampaignManagerPoolConfig;
    typedef CORBACommons::ObjectPool<
      AdServer::CampaignSvcs::CampaignManager, CampaignManagerPoolConfig>
      CampaignManagerPool;
    typedef std::unique_ptr<CampaignManagerPool> CampaignManagerPoolPtr;
    typedef CampaignManagerPool::ObjectHandlerType CampaignManagerHandler;
    typedef Config::CorbaConfigReader::ServiceIndexRefHashMap ServiceIndexRefHashMap;

    void
    exception_handle_(
      CampaignManagerHandler& campaign_manager,
      const char* descr = "",
      const char* impl_ex_code = 0,
      const char* aspect = 0) /*throw(Exception)*/;

    unsigned
    convert_service_index_(const char* service_id) const noexcept;

    CampaignManagerPoolPtr campaign_managers_;
    Logging::Logger_var logger_;
    ServiceIndexRefHashMap service_id_pool_key_map_;
    const std::string ASPECT_;
  };

}

namespace FrontendCommons
{
  template <typename Exception>
  void
  CampaignManagersPool<Exception>::resolve(
    const CommonFeConfiguration& common_config,
    const CORBACommons::CorbaClientAdapter* corba_client_adapter)
    /*throw(Exception)*/
  {
    static const char* FUN = "FrontendCommons::resolve()";
    CORBACommons::CorbaObjectRefList campaign_managers_refs;

    try
    {
      if (common_config.CampaignManagerRef().present())
      {
        Config::CorbaConfigReader::read_indexed_multi_corba_ref(
          *common_config.CampaignManagerRef(),
          campaign_managers_refs,
          service_id_pool_key_map_);
      }
    }
    catch (const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN <<
        ": Can't read Campaign Manager corba object references: " << e.what();
      throw Exception(ostr);
    }

    resolve(campaign_managers_refs, corba_client_adapter);
  }

  template <typename Exception>
  void
  CampaignManagersPool<Exception>::resolve(
    const CORBACommons::CorbaObjectRefList& campaign_managers_refs,
    const CORBACommons::CorbaClientAdapter* corba_client_adapter)
    /*throw(Exception)*/
  {
    static const char* FUN = "FrontendCommons::resolve()";

    try
    {
      CampaignManagerPoolConfig pool_config(corba_client_adapter);
      pool_config.timeout = Generics::Time(10); // 10 sec

      std::copy(
        campaign_managers_refs.begin(),
        campaign_managers_refs.end(),
        std::back_inserter(pool_config.iors_list));

      campaign_managers_.reset(new CampaignManagerPool(pool_config,
        CORBACommons::ChoosePolicyType::PT_PRECISE));
    }
    catch (const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't create Campaign Manager objects pool: " <<
        e.what();
      throw Exception(ostr);
    }
  }

  template <typename Exception>
  inline unsigned
  CampaignManagersPool<Exception>::convert_service_index_(
    const char* service_id) const
    noexcept
  {
    ServiceIndexRefHashMap::const_iterator cit =
      service_id_pool_key_map_.find(service_id);
    return cit != service_id_pool_key_map_.end() ? cit->second :
      CampaignManagerPool::SPECIAL_KEY;
  }

  template <typename Exception>
  void
  CampaignManagersPool<Exception>::exception_handle_(
    CampaignManagerHandler& campaign_manager,
    const char* descr,
    const char* impl_ex_code,
    const char* aspect
    ) /*throw(Exception)*/
  {
    try
    {
      throw;
    }
      catch (const AdServer::CampaignSvcs::
        CampaignManager::ImplementationException& ex)
      {
        Stream::Error ostr;
        ostr << "CampaignManager::" << descr <<
          " Caught ImplementationException:" << ex.description;

        logger_->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          aspect ? aspect : ASPECT_.c_str(),
          impl_ex_code ? impl_ex_code : "ADS-IMPL-118");
        throw Exception(ostr);
      }
      catch (const AdServer::CampaignSvcs::CampaignManager::NotReady& ex)
      {
        Stream::Error ostr;
        ostr << "CampaignManager::" << descr <<
          " Caught NotReady exception:" << ex.description;

        logger_->log(ostr.str(),
          Logging::Logger::NOTICE,
          aspect ? aspect : ASPECT_.c_str(),
          impl_ex_code ? impl_ex_code : "ADS-IMPL-118");
        campaign_manager.release_bad(ostr.str());
      }
      catch (const CORBA::SystemException& ex)
      {
        Stream::Error ostr;
        ostr << "CampaignManager::" << descr <<
          " Caught CORBA::SystemException "<< ex;

        campaign_manager.release_bad(ostr.str());
        logger_->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          aspect ? aspect : ASPECT_.c_str(),
          "ADS-ICON-4");
      }
  }

  template <typename Exception>
  void
  CampaignManagersPool<Exception>::match_geo_channels(
    const AdServer::CampaignSvcs::CampaignManager::GeoInfoSeq& location,
    const AdServer::CampaignSvcs::CampaignManager::GeoCoordInfoSeq& coord_location,
    AdServer::CampaignSvcs::ChannelIdSeq_out geo_channels_result,
    AdServer::CampaignSvcs::ChannelIdSeq_out coord_channels_result)
    /*throw(Exception)*/
  {
    static const char* descr =
      "CampaignManager::match_geo_channels(): Can't match geo channels, caught ";

    for (;;)
    {
      CampaignManagerHandler campaign_manager =
        campaign_managers_->get_object<Exception>(
          logger_,
          Logging::Logger::EMERGENCY,
          ASPECT_.c_str(),
          "ADS_ICON-4");
      try
      {
        campaign_manager->match_geo_channels(
          location,
          coord_location,
          geo_channels_result,
          coord_channels_result);
        return;
      }
      catch (const AdServer::CampaignSvcs::
        CampaignManager::ImplementationException& ex)
      {
        Stream::Error ostr;
        ostr << descr << "ImplementationException: " <<
          ex.description;

        logger_->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          ASPECT_.c_str(),
          "ADS-IMPL-118");
        throw Exception(ostr);
      }
      catch (const AdServer::CampaignSvcs::CampaignManager::NotReady& ex)
      {
        Stream::Error ostr;
        ostr << descr << "NotReady exception: " << ex.description;

        logger_->log(ostr.str(),
          Logging::Logger::NOTICE,
          ASPECT_.c_str(),
          "ADS-IMPL-118");
        campaign_manager.release_bad(ostr.str());
      }
      catch (const CORBA::SystemException& ex)
      {
        Stream::Error ostr;
        ostr << descr << "CORBA::SystemException: " << ex;

        campaign_manager.release_bad(ostr.str());
        logger_->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          ASPECT_.c_str(),
          "ADS-ICON-4");
      }
    }
  }

  template <typename Exception>
  void
  CampaignManagersPool<Exception>::get_campaign_creative(
    const AdServer::CampaignSvcs::CampaignManager::RequestParams&
      request_params,
    CORBA::String_var& hostname,
    AdServer::CampaignSvcs::CampaignManager::RequestCreativeResult_out
      campaign_matching_result)
    /*throw(Exception)*/
  {
    static const char* descr =
      "CampaignManager::get_campaign_creative(): Can't select creative, caught ";

    for (;;)
    {
      CampaignManagerHandler campaign_manager =
        campaign_managers_->get_object<Exception>(
          logger_,
          Logging::Logger::EMERGENCY,
          ASPECT_.c_str(),
          "ADS_ICON-4");
      try
      {
        campaign_manager->get_campaign_creative(
          request_params,
          hostname,
          campaign_matching_result);
        return;
      }
      catch (const AdServer::CampaignSvcs::
        CampaignManager::ImplementationException& ex)
      {
        Stream::Error ostr;
        ostr << descr << "ImplementationException: " <<
          ex.description;

        logger_->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          ASPECT_.c_str(),
          "ADS-IMPL-118");
        throw Exception(ostr);
      }
      catch (const AdServer::CampaignSvcs::CampaignManager::NotReady& ex)
      {
        Stream::Error ostr;
        ostr << descr << "NotReady exception: " << ex.description;

        logger_->log(ostr.str(),
          Logging::Logger::NOTICE,
          ASPECT_.c_str(),
          "ADS-IMPL-118");
        campaign_manager.release_bad(ostr.str());
      }
      catch (const CORBA::SystemException& ex)
      {
        Stream::Error ostr;
        ostr << descr << "CORBA::SystemException: " << ex;

        campaign_manager.release_bad(ostr.str());
        logger_->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          ASPECT_.c_str(),
          "ADS-ICON-4");
      }
    }
  }

  template <typename Exception>
  void
  CampaignManagersPool<Exception>::process_match_request(
    const AdServer::CampaignSvcs::CampaignManager::MatchRequestInfo&
      match_request_info)
    /*throw(Exception)*/
  {
    static const char* descr =
      "CampaignManager::process_match_request(): ";
    for (;;)
    {
      CampaignManagerHandler campaign_manager =
        campaign_managers_->get_object<Exception>(
          logger_,
          Logging::Logger::EMERGENCY,
          ASPECT_.c_str(),
          "ADS_ICON-4");
      try
      {
        campaign_manager->process_match_request(match_request_info);
        return;
      }
      catch (const AdServer::CampaignSvcs::
        CampaignManager::ImplementationException& ex)
      {
        Stream::Error ostr;
        ostr << descr << "ImplementationException: " <<
          ex.description;

        logger_->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          ASPECT_.c_str(),
          "ADS-IMPL-118");
        throw Exception(ostr);
      }
      catch (const AdServer::CampaignSvcs::CampaignManager::NotReady& ex)
      {
        Stream::Error ostr;
        ostr << descr << "NotReady exception: " << ex.description;

        logger_->log(ostr.str(),
          Logging::Logger::NOTICE,
          ASPECT_.c_str(),
          "ADS-IMPL-118");
        campaign_manager.release_bad(ostr.str());
      }
      catch (const CORBA::SystemException& ex)
      {
        Stream::Error ostr;
        ostr << descr << "CORBA::SystemException: " << ex;

        campaign_manager.release_bad(ostr.str());
        logger_->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          ASPECT_.c_str(),
          "ADS-ICON-4");
      }
    }
  }

  template <typename Exception>
  void
  CampaignManagersPool<Exception>::process_anonymous_request(
    const AdServer::CampaignSvcs::CampaignManager::AnonymousRequestInfo&
      request_info)
    /*throw(Exception)*/
  {
    static const char* descr =
      "CampaignManager::process_anonymous_request(): ";

    for (;;)
    {
      CampaignManagerHandler campaign_manager =
        campaign_managers_->get_object<Exception>(
          logger_,
          Logging::Logger::EMERGENCY,
          ASPECT_.c_str(),
          "ADS_ICON-4");
      try
      {
        campaign_manager->process_anonymous_request(request_info);
        return;
      }
      catch (const AdServer::CampaignSvcs::
        CampaignManager::ImplementationException& ex)
      {
        Stream::Error ostr;
        ostr << descr << "ImplementationException: " <<
          ex.description;

        logger_->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          ASPECT_.c_str(),
          "ADS-IMPL-118");
        throw Exception(ostr);
      }
      catch (const AdServer::CampaignSvcs::CampaignManager::NotReady& ex)
      {
        Stream::Error ostr;
        ostr << descr << "NotReady exception: " << ex.description;

        logger_->log(ostr.str(),
          Logging::Logger::NOTICE,
          ASPECT_.c_str(),
          "ADS-IMPL-118");
        campaign_manager.release_bad(ostr.str());
      }
      catch (const CORBA::SystemException& ex)
      {
        Stream::Error ostr;
        ostr << descr << "CORBA::SystemException: " << ex;

        campaign_manager.release_bad(ostr.str());
        logger_->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          ASPECT_.c_str(),
          "ADS-ICON-4");
      }
    }
  }

  template <typename Exception>
  void
  CampaignManagersPool<Exception>::instantiate_ad(
    const AdServer::CampaignSvcs::CampaignManager::InstantiateAdInfo&
      instantiate_ad_info,
    AdServer::CampaignSvcs::CampaignManager::InstantiateAdResult_out
      instantiate_ad_result,
    const char* service_id)
    /*throw(Exception)*/
  {
    static const char* descr = "CampaignManagersPool::instantiate_ad(): caught ";
    unsigned service_pool_key = convert_service_index_(service_id);
    for (;;)
    {
      CampaignManagerHandler campaign_manager =
        campaign_managers_->get_object<Exception>(
          logger_,
          Logging::Logger::EMERGENCY,
          ASPECT_.c_str(),
          "ADS_ICON-4",
          service_pool_key);
      try
      {
        campaign_manager->instantiate_ad(
          instantiate_ad_info,
          instantiate_ad_result);
        return;
      }
      catch (const AdServer::CampaignSvcs::
        CampaignManager::ImplementationException& ex)
      {
        Stream::Error ostr;
        ostr << descr << "ImplementationException: " <<
          ex.description;

        logger_->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          ASPECT_.c_str(),
          "ADS-IMPL-118");
        throw Exception(ostr);
      }
      catch (const AdServer::CampaignSvcs::CampaignManager::NotReady& ex)
      {
        Stream::Error ostr;
        ostr << descr << "NotReady exception: " << ex.description;

        logger_->log(ostr.str(),
          Logging::Logger::NOTICE,
          ASPECT_.c_str(),
          "ADS-IMPL-118");
        campaign_manager.release_bad(ostr.str());
      }
      catch (const CORBA::SystemException& ex)
      {
        Stream::Error ostr;
        ostr << descr << "CORBA::SystemException: " << ex;

        campaign_manager.release_bad(ostr.str());
        logger_->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          ASPECT_.c_str(),
          "ADS-ICON-4");
      }
      service_pool_key = CampaignManagerPool::SPECIAL_KEY;
    }
  }

  template <typename Exception>
  AdServer::CampaignSvcs::CampaignManager::DiscoverChannelResultSeq_var
  CampaignManagersPool<Exception>::get_discover_channels(
    const AdServer::CampaignSvcs::ChannelWeightSeq& channels,
    const char* country,
    const char* language,
    bool all)
    /*throw(Exception)*/
  {
    static const char* descr =
      "CampaignManager::get_discover_channels(): Can't get discover channels, caught ";
    for (;;)
    {
      CampaignManagerHandler campaign_manager =
        campaign_managers_->get_object<Exception>(
          logger_,
          Logging::Logger::EMERGENCY,
          ASPECT_.c_str(),
          "ADS_ICON-4");
      try
      {
        return campaign_manager->get_discover_channels(
          channels, country, language, all);
      }
      catch (const AdServer::CampaignSvcs::
        CampaignManager::ImplementationException& ex)
      {
        Stream::Error ostr;
        ostr << descr << "ImplementationException: " <<
          ex.description;

        logger_->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          ASPECT_.c_str(),
          "ADS-IMPL-7507");
        throw Exception(ostr);
      }
      catch (const AdServer::CampaignSvcs::CampaignManager::NotReady& ex)
      {
        Stream::Error ostr;
        ostr << descr << "NotReady exception: " << ex.description;

        logger_->log(ostr.str(),
          Logging::Logger::NOTICE,
          ASPECT_.c_str(),
          "ADS-IMPL-7507");
        campaign_manager.release_bad(ostr.str());
      }
      catch (const CORBA::SystemException& ex)
      {
        Stream::Error ostr;
        ostr << descr << "CORBA::SystemException: " << ex;

        campaign_manager.release_bad(ostr.str());
        logger_->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          ASPECT_.c_str(),
          "ADS-ICON-4");
      }
    }
  }

  template <typename Exception>
  AdServer::CampaignSvcs::CampaignManager::CategoryChannelNodeSeq_var
  CampaignManagersPool<Exception>::get_category_channels(
    const char* language)
    /*throw(Exception)*/
  {
    static const char* descr =
      "CampaignManager::get_category_channels(): Can't get category channels, caught ";
    for (;;)
    {
      CampaignManagerHandler campaign_manager =
        campaign_managers_->get_object<Exception>(
          logger_,
          Logging::Logger::EMERGENCY,
          ASPECT_.c_str(),
          "ADS_ICON-4");
      try
      {
        return campaign_manager->get_category_channels(language);
      }
      catch (const AdServer::CampaignSvcs::
        CampaignManager::ImplementationException& ex)
      {
        Stream::Error ostr;
        ostr << descr << "ImplementationException: " <<
          ex.description;

        logger_->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          ASPECT_.c_str(),
          "ADS-IMPL-118");
        throw Exception(ostr);
      }
      catch (const AdServer::CampaignSvcs::CampaignManager::NotReady& ex)
      {
        Stream::Error ostr;
        ostr << descr << "NotReady exception: " << ex.description;

        logger_->log(ostr.str(),
          Logging::Logger::NOTICE,
          ASPECT_.c_str(),
          "ADS-IMPL-118");
        campaign_manager.release_bad(ostr.str());
      }
      catch (const CORBA::SystemException& ex)
      {
        Stream::Error ostr;
        ostr << descr << "CORBA::SystemException: " << ex;

        campaign_manager.release_bad(ostr.str());
        logger_->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          ASPECT_.c_str(),
          "ADS-ICON-4");
      }
    }
  }

  template <typename Exception>
  void
  CampaignManagersPool<Exception>::consider_passback(
    const AdServer::CampaignSvcs::CampaignManager::PassbackInfo& info)
    /*throw(Exception)*/
  {
    static const char* descr =
      "CampaignManager::consider_passback(): caught ";
    for (;;)
    {
      CampaignManagerHandler campaign_manager =
        campaign_managers_->get_object<Exception>(
          logger_,
          Logging::Logger::EMERGENCY,
          ASPECT_.c_str(),
          "ADS_ICON-4");
      try
      {
        campaign_manager->consider_passback(info);
        return;
      }
      catch (const AdServer::CampaignSvcs::
        CampaignManager::ImplementationException& ex)
      {
        Stream::Error ostr;
        ostr << descr << "ImplementationException: " <<
          ex.description;

        logger_->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          ASPECT_.c_str(),
          "ADS-IMPL-193");
        throw Exception(ostr);
      }
      catch (const CORBA::SystemException& ex)
      {
        Stream::Error ostr;
        ostr << descr << "CORBA::SystemException: " << ex;

        campaign_manager.release_bad(ostr.str());
        logger_->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          ASPECT_.c_str(),
          "ADS-ICON-4");
      }
    }
  }

  template <typename Exception>
  void
  CampaignManagersPool<Exception>::consider_passback_track(
    const AdServer::CampaignSvcs::CampaignManager::PassbackTrackInfo& info)
    /*throw(Exception)*/
  {
    static const char* descr =
      "CampaignManager::consider_passback_track(): caught ";
    for (;;)
    {
      CampaignManagerHandler campaign_manager =
        campaign_managers_->get_object<Exception>(
          logger_,
          Logging::Logger::EMERGENCY,
          ASPECT_.c_str(),
          "ADS_ICON-4");
      try
      {
        campaign_manager->consider_passback_track(info);
        return;
      }
      catch (const AdServer::CampaignSvcs::
        CampaignManager::ImplementationException& ex)
      {
        Stream::Error ostr;
        ostr << descr << "ImplementationException: " << ex.description;

        logger_->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          ASPECT_.c_str(),
          "ADS-IMPL-193");
        throw Exception(ostr);
      }
      catch (const AdServer::CampaignSvcs::CampaignManager::NotReady& ex)
      {
        Stream::Error ostr;
        ostr << descr << "NotReady exception: " << ex.description;

        logger_->log(ostr.str(),
          Logging::Logger::NOTICE,
          ASPECT_.c_str(),
          "ADS-IMPL-193");
        campaign_manager.release_bad(ostr.str());
      }
      catch (const CORBA::SystemException& ex)
      {
        Stream::Error ostr;
        ostr << descr << "CORBA::SystemException: " << ex;

        campaign_manager.release_bad(ostr.str());
        logger_->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          ASPECT_.c_str(),
          "ADS-ICON-4");
      }
    }
  }

  template <typename Exception>
  bool
  CampaignManagersPool<Exception>::get_click_url(
    const AdServer::CampaignSvcs::CampaignManager::ClickInfo& click_info,
    AdServer::CampaignSvcs::CampaignManager::ClickResultInfo_var& click_result_info,
    const char* service_id)
    /*throw(Exception)*/
  {
    static const char* descr =
      "CampaignManager::get_click_url(): Can't get click URL, ";
    unsigned service_pool_key = convert_service_index_(service_id);
    for (;;)
    {
      CampaignManagerHandler campaign_manager =
        campaign_managers_->get_object<Exception>(
          logger_,
          Logging::Logger::EMERGENCY,
          ASPECT_.c_str(),
          "ADS_ICON-4",
          service_pool_key);
      try
      {
        return campaign_manager->get_click_url(click_info, click_result_info.out());
      }
      catch (const AdServer::CampaignSvcs::
        CampaignManager::ImplementationException& ex)
      {
        Stream::Error ostr;
        ostr << descr << "caught ImplementationException:" <<
          ex.description;

        logger_->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          ASPECT_.c_str(),
          "ADS-IMPL-118");
        throw Exception(ostr);
      }
      catch (const AdServer::CampaignSvcs::CampaignManager::NotReady& ex)
      {
        Stream::Error ostr;
        ostr << descr << "caught NotReady exception:" << ex.description;

        logger_->log(ostr.str(),
          Logging::Logger::NOTICE,
          ASPECT_.c_str(),
          "ADS-IMPL-118");
        campaign_manager.release_bad(ostr.str());
      }
      catch (const CORBA::SystemException& ex)
      {
        Stream::Error ostr;
        ostr << descr << "caught CORBA::SystemException:" << ex;

        campaign_manager.release_bad(ostr.str());
        logger_->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          ASPECT_.c_str(),
          "ADS-ICON-4");
      }
      service_pool_key = CampaignManagerPool::SPECIAL_KEY;
    }
  }

  template <typename Exception>
  void
  CampaignManagersPool<Exception>::verify_opt_operation(
    unsigned long time,
    long colo_id,
    const char* referer,
    const AdServer::CampaignSvcs::CampaignManager::OptOperation& operation,
    unsigned long status,
    unsigned long user_status,
    bool log_as_test,
    const char* browser,
    const char* os,
    const char* ct,
    const char* curct,
    const char* local_aspect,
    const AdServer::Commons::UserId& user_id)
    /*throw(Exception)*/
  {
    static const char* descr =
      "CampaignManager::verify_opt_operation(): Can't verify opt out, caught ";
    for (;;)
    {
      CampaignManagerHandler campaign_manager =
       campaign_managers_->get_object<Exception>(
          logger_,
          Logging::Logger::EMERGENCY,
          ASPECT_.c_str(),
          "ADS_ICON-4");
      try
      {
        campaign_manager->verify_opt_operation(
          time,
          colo_id,
          referer,
          operation,
          status,
          static_cast<CORBA::ULong>(user_status),
          log_as_test,
          browser,
          os,
          ct,
          curct,
          CorbaAlgs::pack_user_id(user_id));
        return;
      }
      catch (const AdServer::CampaignSvcs::
        CampaignManager::ImplementationException& ex)
      {
        Stream::Error ostr;
        ostr << descr << "ImplementationException: " <<
          ex.description;

        logger_->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          local_aspect,
          "ADS-IMPL-118");
        throw Exception(ostr);
      }
      catch (const AdServer::CampaignSvcs::CampaignManager::NotReady& ex)
      {
        Stream::Error ostr;
        ostr << descr << "NotReady exception: " << ex.description;

        logger_->log(ostr.str(),
          Logging::Logger::NOTICE,
          local_aspect,
          "ADS-IMPL-118");
        campaign_manager.release_bad(ostr.str());
      }
      catch (const CORBA::SystemException& ex)
      {
        Stream::Error ostr;
        ostr << descr << "CORBA::SystemException: " << ex;

        campaign_manager.release_bad(ostr.str());
        logger_->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          local_aspect,
          "ADS-ICON-4");
      }
    }
  }

  template <typename Exception>
  void
  CampaignManagersPool<Exception>::consider_web_operation(
    const AdServer::CampaignSvcs::CampaignManager::WebOperationInfo&
      web_op_info)
    /*throw(AdServer::CampaignSvcs::CampaignManager::IncorrectArgument,
      Exception)*/
  {
    static const char* descr =
      "CampaignManager::consider_web_operation(): "
      "Can't conside web operation, caught ";
    for (;;)
    {
      CampaignManagerHandler campaign_manager =
       campaign_managers_->get_object<Exception>(
          logger_,
          Logging::Logger::EMERGENCY,
          ASPECT_.c_str(),
          "ADS_ICON-4");
      try
      {
        campaign_manager->consider_web_operation(web_op_info);
        return;
      }
      catch(const AdServer::CampaignSvcs::CampaignManager::IncorrectArgument&)
      {
        throw;
      }
      catch (const AdServer::CampaignSvcs::
        CampaignManager::ImplementationException& ex)
      {
        Stream::Error ostr;
        ostr << descr << "ImplementationException: " <<
          ex.description;

        logger_->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          ASPECT_.c_str(),
          "ADS-IMPL-118");
        throw Exception(ostr);
      }
      catch (const AdServer::CampaignSvcs::CampaignManager::NotReady& ex)
      {
        Stream::Error ostr;
        ostr << descr << "NotReady exception: " << ex.description;

        logger_->log(ostr.str(),
          Logging::Logger::NOTICE,
          ASPECT_.c_str(),
          "ADS-IMPL-118");
        campaign_manager.release_bad(ostr.str());
      }
      catch (const CORBA::SystemException& ex)
      {
        Stream::Error ostr;
        ostr << descr << "CORBA::SystemException: " << ex;

        campaign_manager.release_bad(ostr.str());
        logger_->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          ASPECT_.c_str(),
          "ADS-ICON-4");
      }
    }
  }

  template <typename Exception>
  void
  CampaignManagersPool<Exception>::action_taken(
    const AdServer::CampaignSvcs::CampaignManager::ActionInfo& action_info)
    /*throw(Exception)*/
  {
    static const char* descr = "CampaignManager::action_taken(): caught ";
    for (;;)
    {
      CampaignManagerHandler campaign_manager =
       campaign_managers_->get_object<Exception>(
          logger_,
          Logging::Logger::EMERGENCY,
          ASPECT_.c_str(),
          "ADS_ICON-4");
      try
      {
        campaign_manager->action_taken(action_info);
        return;
      }
      catch (const AdServer::CampaignSvcs::
        CampaignManager::ImplementationException& ex)
      {
        Stream::Error ostr;
        ostr << descr << "ImplementationException: " <<
          ex.description;

        logger_->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          ASPECT_.c_str(),
          "ADS-IMPL-118");
        throw Exception(ostr);
      }
      catch (const AdServer::CampaignSvcs::CampaignManager::NotReady& ex)
      {
        Stream::Error ostr;
        ostr << descr << "NotReady exception: " << ex.description;

        logger_->log(ostr.str(),
          Logging::Logger::NOTICE,
          ASPECT_.c_str(),
          "ADS-IMPL-118");
        campaign_manager.release_bad(ostr.str());
      }
      catch (const CORBA::SystemException& ex)
      {
        Stream::Error ostr;
        ostr << descr << "CORBA::SystemException: " << ex;

        campaign_manager.release_bad(ostr.str());
        logger_->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          ASPECT_.c_str(),
          "ADS-ICON-4");
      }
    }
  }

  template <typename Exception>
  void
  CampaignManagersPool<Exception>::verify_impression(
    const AdServer::CampaignSvcs::CampaignManager::ImpressionInfo&
      impression_info,
    AdServer::CampaignSvcs::CampaignManager::ImpressionResultInfo_var&
      impression_result_info)
    /*throw(Exception)*/
  {
    static const char* descr = "CampaignManager::verify_impression(): "
      "Can't get impression verify information, caught ";
    for (;;)
    {
      CampaignManagerHandler campaign_manager =
       campaign_managers_->get_object<Exception>(
          logger_,
          Logging::Logger::EMERGENCY,
          ASPECT_.c_str(),
          "ADS_ICON-4");
      try
      {
        campaign_manager->verify_impression(impression_info, impression_result_info);
        return;
      }
      catch (const AdServer::CampaignSvcs::
        CampaignManager::ImplementationException& ex)
      {
        Stream::Error ostr;
        ostr << descr << "ImplementationException: " <<
          ex.description;

        logger_->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          ASPECT_.c_str(),
          "ADS-IMPL-118");
        throw Exception(ostr);
      }
      catch (const AdServer::CampaignSvcs::CampaignManager::NotReady& ex)
      {
        Stream::Error ostr;
        ostr << descr << "NotReady exception: " << ex.description;

        logger_->log(ostr.str(),
          Logging::Logger::NOTICE,
          ASPECT_.c_str(),
          "ADS-IMPL-118");
        campaign_manager.release_bad(ostr.str());
      }
      catch (const CORBA::SystemException& ex)
      {
        Stream::Error ostr;
        ostr << descr << "CORBA::SystemException: " << ex;

        campaign_manager.release_bad(ostr.str());
        logger_->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          ASPECT_.c_str(),
          "ADS-ICON-4");
      }
    }
  }

  template <typename Exception>
  AdServer::CampaignSvcs::ColocationFlagsSeq_var
  CampaignManagersPool<Exception>::get_colocation_flags()
    /*throw(Exception)*/
  {
    static const char* descr = "CampaignManager::get_colocation_flags(): "
      "Can't update colocation flags, caught ";
    for (;;)
    {
      CampaignManagerHandler campaign_manager =
       campaign_managers_->get_object<Exception>(
          logger_,
          Logging::Logger::EMERGENCY,
          ASPECT_.c_str(),
          "ADS_ICON-4");
      try
      {
        return campaign_manager->get_colocation_flags();
      }
      catch (const AdServer::CampaignSvcs::
        CampaignManager::ImplementationException& ex)
      {
        Stream::Error ostr;
        ostr << descr << "ImplementationException: " <<
          ex.description;

        logger_->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          ASPECT_.c_str(),
          "ADS-IMPL-118");
        throw Exception(ostr);
      }
      catch (const AdServer::CampaignSvcs::CampaignManager::NotReady& ex)
      {
        Stream::Error ostr;
        ostr << descr << "NotReady exception: " << ex.description;

        logger_->log(ostr.str(),
          Logging::Logger::NOTICE,
          ASPECT_.c_str(),
          "ADS-IMPL-118");
        campaign_manager.release_bad(ostr.str());
      }
      catch (const CORBA::SystemException& ex)
      {
        Stream::Error ostr;
        ostr << descr << "CORBA::SystemException: " << ex;

        campaign_manager.release_bad(ostr.str());
        logger_->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          ASPECT_.c_str(),
          "ADS-ICON-4");
      }
    }
  }

  template <typename Exception>
  AdServer::CampaignSvcs::StringSeq_var
  CampaignManagersPool<Exception>::get_pub_pixels(
    const char* country,
    unsigned long user_status,
    const AdServer::CampaignSvcs::PublisherAccountIdSeq& publisher_account_ids)
    /*throw(Exception)*/
  {
    static const char* descr = "CampaignManagersPool::get_pub_pixels(): caught ";

    for (;;)
    {
      CampaignManagerHandler campaign_manager =
        campaign_managers_->get_object<Exception>(
          logger_,
          Logging::Logger::EMERGENCY,
          ASPECT_.c_str(),
          "ADS_ICON-4");
      try
      {
        return campaign_manager->get_pub_pixels(
          country,
          user_status,
          publisher_account_ids);
      }
      catch (const AdServer::CampaignSvcs::
        CampaignManager::ImplementationException& ex)
      {
        Stream::Error ostr;
        ostr << descr << "ImplementationException: " <<
          ex.description;

        logger_->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          ASPECT_.c_str(),
          "ADS-IMPL-118");
        throw Exception(ostr);
      }
      catch (const AdServer::CampaignSvcs::CampaignManager::NotReady& ex)
      {
        Stream::Error ostr;
        ostr << descr << "NotReady exception: " << ex.description;

        logger_->log(ostr.str(),
          Logging::Logger::NOTICE,
          ASPECT_.c_str(),
          "ADS-IMPL-118");
        campaign_manager.release_bad(ostr.str());
      }
      catch (const CORBA::SystemException& ex)
      {
        Stream::Error ostr;
        ostr << descr << "CORBA::SystemException: " << ex;

        campaign_manager.release_bad(ostr.str());
        logger_->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          ASPECT_.c_str(),
          "ADS-ICON-4");
      }
    }
  }

  template <typename Exception>
  void
  CampaignManagersPool<Exception>::get_file(
    const char* file,
    CORBACommons::OctSeq_var& content,
    const char* service_id)
    /*throw(Exception)*/
  {
    static const char* descr =
      "CampaignManager::get_file(): ";
    unsigned service_pool_key = convert_service_index_(service_id);

    for (;;)
    {
      CampaignManagerHandler campaign_manager =
        campaign_managers_->get_object<Exception>(
          logger_,
          Logging::Logger::EMERGENCY,
          ASPECT_.c_str(),
          "ADS_ICON-4",
          service_pool_key);
      try
      {
        campaign_manager->get_file(file, content.out());
        return;
      }
      catch (const AdServer::CampaignSvcs::
        CampaignManager::ImplementationException& ex)
      {
        Stream::Error ostr;
        ostr << descr << "ImplementationException: " <<
          ex.description;

        logger_->log(ostr.str(),
          Logging::Logger::NOTICE,
          ASPECT_.c_str(),
          "ADS-IMPL-118");
        throw Exception(ostr);
      }
      catch (const CORBA::SystemException& ex)
      {
        Stream::Error ostr;
        ostr << descr << "CORBA::SystemException: " << ex;

        campaign_manager.release_bad(ostr.str());
        logger_->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          ASPECT_.c_str(),
          "ADS-ICON-4");
      }
    }
  }


}

#endif // CAMPAIGN_MANAGER_HOOK_HPP
