/**
 * @file UserCampaignReachContainer.cpp
 */
#include <Generics/Time.hpp>
#include <PrivacyFilter/Filter.hpp>

#include <RequestInfoSvcs/RequestInfoCommons/UserCampaignReachProfile.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/Algs.hpp>

#include "Compatibility/UserCampaignReachProfileAdapter.hpp"
#include "UserCampaignReachContainer.hpp"

namespace Aspect
{
  const char USER_CAMPAIGN_REACH_CONTAINER[] = "UserCampaignReachContainer";
}

namespace AdServer
{
namespace RequestInfoSvcs
{
  /**
   * UserCampaignReachContainer
   */
  UserCampaignReachContainer::UserCampaignReachContainer(
    Logging::Logger* logger,
    CampaignReachProcessor* campaign_reach_processor,
    const char* user_campaign_reach_file_base_path,
    const char* user_campaign_reach_file_prefix,
    ProfilingCommons::ProfileMapFactory::Cache* cache,
    const Generics::Time& expire_time,
    const Generics::Time& extend_time_period)
    /*throw(Exception)*/
    : logger_(ReferenceCounting::add_ref(logger)),
      expire_time_(expire_time),
      campaign_reach_processor_(
        ReferenceCounting::add_ref(campaign_reach_processor))
  {
    static const char* FUN = "UserCampaignReachContainer::UserCampaignReachContainer()";

    Generics::Time extend_time_period_val(extend_time_period);
    
    if(extend_time_period_val == Generics::Time::ZERO)
    {
      extend_time_period_val = std::max(expire_time / 4, Generics::Time(1));
    }

    try
    {
      user_map_ = ProfilingCommons::ProfileMapFactory::
        open_transaction_expire_map<
          Commons::UserId,
          ProfilingCommons::UserIdAccessor,
          UserCampaignReachProfileAdapter>(
          user_campaign_reach_file_base_path,
          user_campaign_reach_file_prefix,
          extend_time_period_val,
          UserCampaignReachProfileAdapter(),
          cache);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't init UserCampaignReachMap. Caught eh::Exception: " <<
        ex.what();
      throw Exception(ostr);
    }
  }

  UserCampaignReachContainer::~UserCampaignReachContainer() noexcept
  {}

  Generics::ConstSmartMemBuf_var
  UserCampaignReachContainer::get_profile(
    const AdServer::Commons::UserId& user_id)
    /*throw(Exception)*/
  {
    static const char* FUN = "UserCampaignReachContainer::get_profile()";
    try
    {
      return user_map_->get_profile(user_id);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't get profile for user_id = " <<
        user_id << ". Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  void
  UserCampaignReachContainer::process_impression(
    const RequestInfo& request_info,
    const ImpressionInfo&,
    const ProcessingState& processing_state)
    /*throw(RequestActionProcessor::Exception)*/
  {
    if(!(request_info.user_id == AdServer::Commons::PROBE_USER_ID ||
      request_info.user_id == OPTOUT_USER_ID ||
      request_info.user_id.is_null() ||
      processing_state.state != RequestInfo::RS_NORMAL))
    {
      try
      {
        process_ad_impression_(request_info);
      }
      catch(const eh::Exception& ex)
      {
        throw RequestActionProcessor::Exception(ex.what());
      }
    }
  }

  void UserCampaignReachContainer::process_ad_impression_(
    const RequestInfo& request_info)
    /*throw(Exception)*/
  {
    static const char* FUN = "UserCampaignReachContainer::process_ad_impression_()";
    
    CampaignReachProcessor::ReachInfo reach_info;
    bool some_appear = false;
    
    try
    {
      Generics::Time date(Algs::round_to_day(request_info.adv_time));

      bool campaign_appeared = true;
      bool ccg_appeared = true;
      bool cc_appeared = true;
      bool advertiser_appeared = false;
      bool advertiser_text_appeared = false;
      bool advertiser_display_appeared = false;

      UserCampaignReachMap::Transaction_var transaction =
        user_map_->get_transaction(request_info.user_id);

      Generics::ConstSmartMemBuf_var mem_buf = transaction->get_profile();

      if(mem_buf.in())
      {
        UserCampaignReachProfileReader user_profile_reader(
          mem_buf->membuf().data(),
          mem_buf->membuf().size());

        campaign_appeared = collect_appearance(
          reach_info.campaigns,
          user_profile_reader.campaign_appears().begin(),
          user_profile_reader.campaign_appears().end(),
          request_info.campaign_id,
          date,
          BaseAppearanceLess());

        ccg_appeared = collect_appearance(
          reach_info.ccgs,
          user_profile_reader.ccg_appears().begin(),
          user_profile_reader.ccg_appears().end(),
          request_info.ccg_id,
          date,
          BaseAppearanceLess());

        cc_appeared = collect_appearance(
          reach_info.creatives,
          user_profile_reader.cc_appears().begin(),
          user_profile_reader.cc_appears().end(),
          request_info.cc_id,
          date,
          BaseAppearanceLess());

        advertiser_appeared = false;

        if(request_info.advertiser_id)
        {
          advertiser_appeared = collect_appearance(
            reach_info.advertisers,
            user_profile_reader.adv_appears().begin(),
            user_profile_reader.adv_appears().end(),
            request_info.advertiser_id,
            date,
            BaseAppearanceLess());

          if(request_info.text_campaign)
          {
            advertiser_text_appeared = collect_appearance(
              reach_info.text_advertisers,
              user_profile_reader.adv_text_appears().begin(),
              user_profile_reader.adv_text_appears().end(),
              request_info.advertiser_id,
              date,
              BaseAppearanceLess());
          }
          else
          {
            advertiser_display_appeared = collect_appearance(
              reach_info.display_advertisers,
              user_profile_reader.adv_display_appears().begin(),
              user_profile_reader.adv_display_appears().end(),
              request_info.advertiser_id,
              date,
              BaseAppearanceLess());
          }
        }
      }
      else
      {
        add_total_appearance(
          reach_info.campaigns,
          request_info.campaign_id,
          date);

        add_total_appearance(
          reach_info.ccgs,
          request_info.ccg_id,
          date);

        add_total_appearance(
          reach_info.creatives,
          request_info.cc_id,
          date);

        if(request_info.advertiser_id)
        {
          advertiser_appeared = true;

          add_total_appearance(
            reach_info.advertisers,
            request_info.advertiser_id,
            date);

          if(request_info.text_campaign)
          {
            advertiser_text_appeared = true;

            add_total_appearance(
              reach_info.text_advertisers,
              request_info.advertiser_id,
              date);
          }
          else
          {
            advertiser_display_appeared = true;

            add_total_appearance(
              reach_info.display_advertisers,
              request_info.advertiser_id,
              date);
          }
        }
      }

      if(campaign_appeared ||
         ccg_appeared ||
         cc_appeared ||
         advertiser_appeared ||
         advertiser_text_appeared ||
         advertiser_display_appeared)
      {
        some_appear = true;

        UserCampaignReachProfileWriter user_profile_writer;

        if(mem_buf.in())
        {
          user_profile_writer.init(
            mem_buf->membuf().data(),
            mem_buf->membuf().size());
        }
        else
        {
          user_profile_writer.version() = CURRENT_CAMPAIGN_REACH_PROFILE_VERSION;
        }

        if(campaign_appeared)
        {
          update_appearance(
            user_profile_writer.campaign_appears(),
            request_info.campaign_id,
            date,
            BaseAppearanceLess(),
            BaseInsertAppearanceAdapter<IdAppearanceWriter>());
        }

        if(ccg_appeared)
        {
          update_appearance(
            user_profile_writer.ccg_appears(),
            request_info.ccg_id,
            date,
            BaseAppearanceLess(),
            BaseInsertAppearanceAdapter<IdAppearanceWriter>());
        }

        if(cc_appeared)
        {
          update_appearance(
            user_profile_writer.cc_appears(),
            request_info.cc_id,
            date,
            BaseAppearanceLess(),
            BaseInsertAppearanceAdapter<IdAppearanceWriter>());
        }

        if(advertiser_appeared)
        {
          update_appearance(
            user_profile_writer.adv_appears(),
            request_info.advertiser_id,
            date,
            BaseAppearanceLess(),
            BaseInsertAppearanceAdapter<IdAppearanceWriter>());
        }

        if(advertiser_text_appeared)
        {
          update_appearance(
            user_profile_writer.adv_text_appears(),
            request_info.advertiser_id,
            date,
            BaseAppearanceLess(),
            BaseInsertAppearanceAdapter<IdAppearanceWriter>());
        }

        if(advertiser_display_appeared)
        {
          update_appearance(
            user_profile_writer.adv_display_appears(),
            request_info.advertiser_id,
            date,
            BaseAppearanceLess(),
            BaseInsertAppearanceAdapter<IdAppearanceWriter>());
        }

        /* save profile */
        try
        {
          unsigned long sz = user_profile_writer.size();
          Generics::SmartMemBuf_var new_mem_buf(new Generics::SmartMemBuf(sz));

          user_profile_writer.save(new_mem_buf->membuf().data(), sz);

          transaction->save_profile(
            Generics::transfer_membuf(new_mem_buf),
            request_info.time);
        }
        catch(const eh::Exception& ex)
        {
          Stream::Error ostr;
          ostr << FUN << ": Can't save profile - caught eh::Exception: " <<
            ex.what();
          throw Exception(ostr);
        }
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't process request - caught eh::Exception: " <<
        ex.what();
      throw Exception(ostr);
    }
    
    if(some_appear && !request_info.test_request)
    {
      if(logger_->log_level() >= Logging::Logger::TRACE)
      {
        Stream::Error ostr;
        ostr << FUN << ": Process request: " << std::endl;
        request_info.print(ostr, "  ");
        ostr << std::endl << "Result reach: " << std::endl;
        reach_info.print(ostr, "  ");

        logger_->log(
          ostr.str(),
          Logging::Logger::TRACE,
          Aspect::USER_CAMPAIGN_REACH_CONTAINER);
      }

      try
      {
        campaign_reach_processor_->process_reach(reach_info);
      }
      catch(const CampaignReachProcessor::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": caught CampaignReachProcessor::Exception: " << ex.what();
        throw Exception(ostr);
      }
    }
  }
  
  void UserCampaignReachContainer::clear_expired_users()
    /*throw(Exception)*/
  {
    Generics::Time now = Generics::Time::get_time_of_day();
    user_map_->clear_expired(now - expire_time_);
  }
} /* namespace RequestInfoSvcs */
} /* namespace AdServer */
