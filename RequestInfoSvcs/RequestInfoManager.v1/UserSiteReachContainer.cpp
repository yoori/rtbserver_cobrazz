#include <Generics/Time.hpp>

#include <RequestInfoSvcs/RequestInfoCommons/UserSiteReachProfile.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/Algs.hpp>

#include "Compatibility/UserSiteReachProfileAdapter.hpp"

#include "UserSiteReachContainer.hpp"

namespace Aspect
{
  const char USER_SITE_REACH_CONTAINER[] = "UserSiteReachContainer";
}

namespace AdServer
{
namespace RequestInfoSvcs
{
  /**
   * UserSiteReachContainer
   */
  UserSiteReachContainer::UserSiteReachContainer(
    Logging::Logger* logger,
    SiteReachProcessor* site_reach_processor,
    const char* user_site_reach_file_base_path,
    const char* user_site_reach_file_prefix,
    ProfilingCommons::ProfileMapFactory::Cache* cache,
    const Generics::Time& expire_time,
    const Generics::Time& extend_time_period)
    /*throw(Exception)*/
    : logger_(ReferenceCounting::add_ref(logger)),
      expire_time_(expire_time),
      site_reach_processor_(
        ReferenceCounting::add_ref(site_reach_processor))
  {
    static const char* FUN = "UserSiteReachContainer::UserSiteReachContainer()";

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
          UserSiteReachProfileAdapter>(
          user_site_reach_file_base_path,
          user_site_reach_file_prefix,
          extend_time_period_val,
          UserSiteReachProfileAdapter(),
          cache);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't init UserSiteReachMap. Caught eh::Exception: " <<
        ex.what();
      throw Exception(ostr);
    }
  }

  UserSiteReachContainer::~UserSiteReachContainer() noexcept
  {}

  Generics::ConstSmartMemBuf_var
  UserSiteReachContainer::get_profile(
    const AdServer::Commons::UserId& user_id)
    /*throw(Exception)*/
  {
    static const char* FUN = "UserSiteReachContainer::get_profile()";
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
  
  void UserSiteReachContainer::process_tag_request(
    const TagRequestInfo& tag_request_info)
    /*throw(TagRequestProcessor::Exception)*/
  {
    try
    {
      process_tag_request_(tag_request_info);
    }
    catch(const eh::Exception& ex)
    {
      throw TagRequestProcessor::Exception(ex.what());
    }
  }

  void UserSiteReachContainer::process_tag_request_(
    const TagRequestInfo& tag_request_info)
    /*throw(Exception)*/
  {
    static const char* FUN = "UserSiteReachContainer::process_tag_request_()";

    if(tag_request_info.user_id.is_null() ||
       !tag_request_info.site_id ||
       !tag_request_info.tag_id)
    {
      return;
    }

    SiteReachProcessor::SiteReachInfo reach_info;

    try
    {
      UserSiteReachProfileWriter user_profile_writer;
      bool need_save = false;

      UserSiteReachMap::Transaction_var transaction =
        user_map_->get_transaction(tag_request_info.user_id);

      Generics::ConstSmartMemBuf_var mem_buf = transaction->get_profile();

      if(mem_buf.in())
      {
        user_profile_writer.init(
          mem_buf->membuf().data(),
          mem_buf->membuf().size());
      }
      else
      {
        user_profile_writer.version() = CURRENT_USER_SITE_REACH_PROFILE_VERSION;
        need_save = true;
      }

      Generics::Time date(Algs::round_to_day(tag_request_info.isp_time));

      if(collect_appearance(
        reach_info.appearance_list,
        user_profile_writer.appearance_list().begin(),
        user_profile_writer.appearance_list().end(),
        tag_request_info.site_id,
        date,
        BaseAppearanceLess()))
      {
        update_appearance(
          user_profile_writer.appearance_list(),
          tag_request_info.site_id,
          date,
          BaseAppearanceLess(),
          BaseInsertAppearanceAdapter<SiteAppearanceWriter>());

        need_save = true;
      }

      if(need_save)
      {
        /* save profile */
        try
        {
          unsigned long sz = user_profile_writer.size();
          Generics::SmartMemBuf_var new_mem_buf(new Generics::SmartMemBuf(sz));
          user_profile_writer.save(new_mem_buf->membuf().data(), sz);

          transaction->save_profile(
            Generics::transfer_membuf(new_mem_buf),
            tag_request_info.time);
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
    
    if(!reach_info.appearance_list.empty())
    {
      if(logger_->log_level() >= Logging::Logger::TRACE)
      {
        Stream::Error ostr;
        ostr << FUN << ": Process request: " << std::endl;
        tag_request_info.print(ostr, "  ");
        ostr << std::endl << "Result reach: " << std::endl;
        reach_info.print(ostr, "  ");

        logger_->log(ostr.str(),
          Logging::Logger::TRACE,
          Aspect::USER_SITE_REACH_CONTAINER);
      }

      try
      {
        site_reach_processor_->process_site_reach(reach_info);
      }
      catch(const SiteReachProcessor::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": caught SiteReachProcessor::Exception: " << ex.what();
        throw Exception(ostr);
      }
    }
  }
  
  void UserSiteReachContainer::clear_expired_users()
    /*throw(Exception)*/
  {
    Generics::Time now = Generics::Time::get_time_of_day();
    user_map_->clear_expired(now - expire_time_);
  }
} /* namespace RequestInfoSvcs */
} /* namespace AdServer */
