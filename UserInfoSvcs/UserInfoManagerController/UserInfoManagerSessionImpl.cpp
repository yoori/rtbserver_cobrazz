#include "UserInfoManagerSessionImpl.h"

#include <Commons/CorbaAlgs.hpp>

namespace AdServer
{
namespace UserInfoSvcs
{

  UserInfoManagerSessionImpl::UserInfoManagerSessionImpl()
    /*throw(eh::Exception)*/
    : max_chunk_number_(0),
      inited_(false)
  {}

  UserInfoManagerSessionImpl::UserInfoManagerSessionImpl(
    const UserInfoManagerSessionImpl& init)
    /*throw(eh::Exception)*/
    : CORBA::ValueBase(),
      CORBA::AbstractBase(),
      AdServer::UserInfoSvcs::UserInfoMatcher(),
      AdServer::UserInfoSvcs::UserInfoManagerSession(),
      CORBA::DefaultValueRefCountBase(),
      OBV_AdServer::UserInfoSvcs::UserInfoManagerSession(init.user_info_managers()),
      chunk_refs_(init.chunk_refs_),
      max_chunk_number_(init.max_chunk_number_),
      inited_(init.inited_)
  {}

  UserInfoManagerSessionImpl::UserInfoManagerSessionImpl(
    const AdServer::UserInfoSvcs::UserInfoManagerDescriptionSeq&
      user_info_managers__)
    /*throw(eh::Exception)*/
    : OBV_AdServer::UserInfoSvcs::UserInfoManagerSession(user_info_managers__),
      max_chunk_number_(0),
      inited_(false)
  {
    init_();
  }

  UserInfoManagerSessionImpl::~UserInfoManagerSessionImpl()
    noexcept
  {}

  void UserInfoManagerSessionImpl::init_() noexcept
  {
    /* fill chunk refs */
    {
      Sync::Policy::PosixThread::WriteGuard lock(init_lock_);

      if (!inited_)
      {
        for(unsigned int i = 0; i < user_info_managers().length(); ++i)
        {
          AdServer::UserInfoSvcs::UserInfoManagerDescription&
            user_info_manager_descr = user_info_managers()[i];

          for(unsigned int j = 0; j < user_info_manager_descr.chunk_ids.length(); ++j)
          {
            unsigned long chunk_id = user_info_manager_descr.chunk_ids[j];

            chunk_refs_.insert(
              std::make_pair(
                chunk_id,
                user_info_manager_descr.user_info_manager));

            if(chunk_id >= max_chunk_number_)
            {
              max_chunk_number_ = chunk_id + 1;
            }
          }
        }

        inited_ = true;
      }
    }
  }

  AdServer::UserInfoSvcs::UserInfoManager*
  UserInfoManagerSessionImpl::get_user_host_(
    unsigned long host_index)
    /*throw(
      AdServer::UserInfoSvcs::UserInfoManager::NotReady,
      AdServer::UserInfoSvcs::UserInfoManager::ImplementationException)*/
  {
    static const char* FUN = "UserInfoManagerSessionImpl::get_user_host_()";

    try
    {
      ChunkRefMap::const_iterator it = chunk_refs_.find(host_index);

      if(it == chunk_refs_.end())
      {
        Stream::Error ostr;
        ostr << FUN << ": Can't find chunk with id = " << host_index << ".";
        CORBACommons::throw_desc<
          AdServer::UserInfoSvcs::UserInfoManager::ImplementationException>(
            ostr.str());
      }

      return it->second;
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();

      CORBACommons::throw_desc<
        AdServer::UserInfoSvcs::UserInfoManager::ImplementationException>(
          ostr.str());
    }
    catch(const AdServer::UserInfoSvcs::UserInfoManager::NotReady&)
    {
      throw;
    }
    catch(const AdServer::UserInfoSvcs::
          UserInfoManager::ImplementationException&)
    {
      throw;
    }
    catch(const CORBA::SystemException& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught CORBA::SystemException: " << ex;

      CORBACommons::throw_desc<
        AdServer::UserInfoSvcs::UserInfoManager::ImplementationException>(
          ostr.str());
    }
    return 0; // never reach
  }

  CORBA::ValueBase* UserInfoManagerSessionImpl::_copy_value()
  {
    return new UserInfoManagerSessionImpl(*this);
  }

  /* UserInfoMatcher interface */
  CORBA::Boolean
  UserInfoManagerSessionImpl::merge(
    const AdServer::UserInfoSvcs::UserInfo& user_info,
    const AdServer::UserInfoSvcs::UserInfoMatcher::MatchParams& match_params,
    const AdServer::UserInfoSvcs::UserProfiles& merge_user_profile,
    CORBA::Boolean_out merge_success,
    CORBACommons::TimestampInfo_out last_request)
    /*throw(
      AdServer::UserInfoSvcs::UserInfoManager::NotReady,
      AdServer::UserInfoSvcs::UserInfoManager::ImplementationException,
      AdServer::UserInfoSvcs::UserInfoManager::ChunkNotFound)*/
  {
    static const char* FUN = "UserInfoManagerSessionImpl::merge()";

    unsigned long host_index = -1;
    try
    {
      if (CorbaAlgs::unpack_user_id(user_info.huser_id).is_null())
      {
        host_index = get_user_host_index_(user_info.user_id);
        return get_user_host_(host_index)->merge(
          user_info,
          match_params,
          merge_user_profile,
          merge_success,
          last_request);
      }
      else
      {
        unsigned long uid_index = get_user_host_index_(user_info.user_id);
        unsigned long hid_index = get_user_host_index_(user_info.huser_id);

        if (uid_index == hid_index)
        {
          return get_user_host_(uid_index)->merge(
            user_info,
            match_params,
            merge_user_profile,
            merge_success,
            last_request);
        }
        else
        {
          AdServer::UserInfoSvcs::UserInfo temp_uid_info;
          AdServer::UserInfoSvcs::UserInfo temp_hid_info;

          temp_uid_info.user_id = user_info.user_id;
          temp_uid_info.time = user_info.time;
          temp_uid_info.last_colo_id = user_info.last_colo_id;
          temp_uid_info.current_colo_id = user_info.current_colo_id;
          temp_uid_info.request_colo_id = user_info.request_colo_id;
          temp_uid_info.temporary = user_info.temporary;

          temp_hid_info.huser_id = user_info.huser_id;
          temp_hid_info.time = user_info.time;
          temp_hid_info.last_colo_id = user_info.request_colo_id;
          temp_hid_info.current_colo_id = user_info.current_colo_id;
          temp_hid_info.request_colo_id = user_info.request_colo_id;
          temp_hid_info.temporary = user_info.temporary;

          CORBACommons::TimestampInfo_var hid_last_request;

          CORBA::Boolean res =
            get_user_host_(uid_index)->merge(
              temp_uid_info,
              match_params,
              merge_user_profile,
              merge_success,
              last_request) &&
            get_user_host_(hid_index)->merge(
              temp_hid_info,
              match_params,
              merge_user_profile,
              merge_success,
              hid_last_request);

          return res;
        }
      }
    }
    catch(const AdServer::UserInfoSvcs::UserInfoManager::NotReady&)
    {
      throw;
    }
    catch(const AdServer::UserInfoSvcs::UserInfoManager::ImplementationException&)
    {
      throw;
    }
    catch(const AdServer::UserInfoSvcs::UserInfoManager::ChunkNotFound&)
    {
      throw;
    }
    catch(const CORBA::SystemException& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught CORBA::SystemException: " << ex;

      CORBACommons::throw_desc<
        AdServer::UserInfoSvcs::UserInfoManager::ImplementationException>(
          ostr.str());
    }
    return 0; // never reach
  }

  CORBA::Boolean UserInfoManagerSessionImpl::fraud_user(
    const CORBACommons::UserIdInfo& user_id,
    const CORBACommons::TimestampInfo& time)
    /*throw(AdServer::UserInfoSvcs::UserInfoManager::NotReady,
          AdServer::UserInfoSvcs::UserInfoManager::ImplementationException,
          AdServer::UserInfoSvcs::UserInfoManager::ChunkNotFound)*/
  {
    static const char* FUN = "UserInfoManagerSessionImpl::fraud_user()";

    try
    {
      return get_user_host_(get_user_host_index_(user_id))->fraud_user(user_id, time);
    }
    catch(const AdServer::UserInfoSvcs::UserInfoManager::NotReady& e)
    {
      throw;
    }
    catch(const AdServer::UserInfoSvcs::UserInfoManager::ImplementationException&)
    {
      throw;
    }
    catch(const AdServer::UserInfoSvcs::UserInfoManager::ChunkNotFound&)
    {
      throw;
    }
    catch(const CORBA::SystemException& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught CORBA::SystemException: " << ex;

      CORBACommons::throw_desc<
        AdServer::UserInfoSvcs::UserInfoManager::ImplementationException>(
          ostr.str());
    }
    return 0; // never reach
  }

  CORBA::Boolean
  UserInfoManagerSessionImpl::match(
    const AdServer::UserInfoSvcs::UserInfo& user_info,
    const AdServer::UserInfoSvcs::UserInfoMatcher::MatchParams& match_params,
    AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult_out match_result)
    /*throw(
      AdServer::UserInfoSvcs::UserInfoManager::NotReady,
      AdServer::UserInfoSvcs::UserInfoManager::ImplementationException,
      AdServer::UserInfoSvcs::UserInfoManager::ChunkNotFound,
      CORBA::SystemException)*/
  {
  //  static const char* FUN = "UserInfoManagerSessionImpl::match()";
    try
    {
      if (CorbaAlgs::unpack_user_id(user_info.huser_id).is_null())
      {
        return get_user_host_(get_user_host_index_(user_info.user_id))->match(
          user_info,
          match_params,
          match_result);
      }
      else
      {
        unsigned long uid_index = get_user_host_index_(user_info.user_id);
        unsigned long hid_index = get_user_host_index_(user_info.huser_id);

        if (uid_index == hid_index)
        {
          return get_user_host_(uid_index)->match(
            user_info,
            match_params,
            match_result);
        }
        else
        {
          AdServer::UserInfoSvcs::UserInfo temp_uid_info;
          AdServer::UserInfoSvcs::UserInfo temp_hid_info;

          temp_uid_info.user_id = user_info.user_id;
          temp_uid_info.time = user_info.time;
          temp_uid_info.last_colo_id = user_info.last_colo_id;
          temp_uid_info.current_colo_id = user_info.current_colo_id;
          temp_uid_info.request_colo_id = user_info.request_colo_id;
          temp_uid_info.temporary = user_info.temporary;

          temp_hid_info.huser_id = user_info.huser_id;
          temp_hid_info.time = user_info.time;
          temp_hid_info.last_colo_id = user_info.request_colo_id;
          temp_hid_info.current_colo_id = user_info.current_colo_id;
          temp_hid_info.request_colo_id = user_info.request_colo_id;
          temp_hid_info.temporary = false;

          AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult_var hid_match_result;

          CORBA::Boolean res =
            get_user_host_(uid_index)->match(temp_uid_info, match_params, match_result) &&
            get_user_host_(hid_index)->match(temp_hid_info, match_params, hid_match_result);

          if (res)
          {
            match_result->hid_channels.length(
              hid_match_result->hid_channels.length());

            for (CORBA::ULong i = 0; i < hid_match_result->hid_channels.length(); ++i)
            {
              match_result->hid_channels[i] = hid_match_result->hid_channels[i];
            }
          }

          return res;
        }
      }
    }
    catch(const AdServer::UserInfoSvcs::UserInfoManager::NotReady& e)
    {
      throw;
    }
    catch(const AdServer::UserInfoSvcs::UserInfoManager::ImplementationException&)
    {
      throw;
    }
    catch(const AdServer::UserInfoSvcs::UserInfoManager::ChunkNotFound&)
    {
      throw;
    }
    catch(const CORBA::SystemException& ex)
    {
      throw;
    }
  }

  void
  UserInfoManagerSessionImpl::get_master_stamp(
    CORBACommons::TimestampInfo_out master_stamp)
    /*throw(
      AdServer::UserInfoSvcs::UserInfoManager::NotReady,
      AdServer::UserInfoSvcs::UserInfoManager::ImplementationException)*/
  {
    try
    {
      if(!inited_)
      {
        init_();
      }
      Generics::Time timestamp;
      for(ChunkRefMap::const_iterator it = chunk_refs_.begin();
          it != chunk_refs_.end(); it++)
      {
        it->second->get_master_stamp(master_stamp);
        if(it == chunk_refs_.begin())
        {
          timestamp = CorbaAlgs::unpack_time(*master_stamp);
        }
        else
        {
          std::min(timestamp, CorbaAlgs::unpack_time(*master_stamp)); 
        }
      }
      master_stamp = new CORBACommons::TimestampInfo(
        CorbaAlgs::pack_time(timestamp));
    }
    catch(const AdServer::UserInfoSvcs::UserInfoManager::NotReady& e)
    {
      throw;
    }
    catch(const AdServer::UserInfoSvcs::UserInfoManager::ImplementationException& e)
    {
      Stream::Error ostr;
      ostr << __func__ << ": ImplementationException: " << e.description;
      CORBACommons::throw_desc<
        AdServer::UserInfoSvcs::UserInfoManager::ImplementationException>(
          ostr.str());
    }
    catch(const CORBA::SystemException& ex)
    {
      Stream::Error ostr;
      ostr << __func__ << ": CORBA::SystemException: " << ex;
      CORBACommons::throw_desc<
        AdServer::UserInfoSvcs::UserInfoManager::ImplementationException>(
          ostr.str());
    }
  }


  CORBA::Boolean
  UserInfoManagerSessionImpl::get_user_profile(
    const CORBACommons::UserIdInfo& user_id,
    CORBA::Boolean temporary,
    const AdServer::UserInfoSvcs::ProfilesRequestInfo& profile_request,
    AdServer::UserInfoSvcs::UserProfiles_out user_profile)
    /*throw(
      AdServer::UserInfoSvcs::UserInfoManager::NotReady,
      AdServer::UserInfoSvcs::UserInfoManager::ImplementationException,
      AdServer::UserInfoSvcs::UserInfoManager::ChunkNotFound)*/
  {
    static const char* FUN = "UserInfoManagerSessionImpl::get_user_profile()";

    try
    {
      return get_user_host_(get_user_host_index_(user_id))->
        get_user_profile(user_id, temporary, profile_request, user_profile);
    }
    catch(const AdServer::UserInfoSvcs::UserInfoManager::NotReady& e)
    {
      throw;
    }
    catch(const AdServer::UserInfoSvcs::UserInfoManager::ImplementationException& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught ImplementationException: " << e.description;
      CORBACommons::throw_desc<
        AdServer::UserInfoSvcs::UserInfoManager::ImplementationException>(
          ostr.str());
    }
    catch(const AdServer::UserInfoSvcs::UserInfoManager::ChunkNotFound& e)
    {
      throw;
    }
    catch(const CORBA::SystemException& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught CORBA::SystemException: " << ex;
      CORBACommons::throw_desc<
        AdServer::UserInfoSvcs::UserInfoManager::ImplementationException>(
          ostr.str());
    }
    return 0; // never reach
  }

  CORBA::Boolean
  UserInfoManagerSessionImpl::remove_user_profile(
    const CORBACommons::UserIdInfo& user_id_info)
    /*throw(
      AdServer::UserInfoSvcs::UserInfoManager::NotReady,
      AdServer::UserInfoSvcs::UserInfoManager::ImplementationException,
      AdServer::UserInfoSvcs::UserInfoManager::ChunkNotFound)*/
  {
    static const char* FUN = "UserInfoManagerSessionImpl::remove_user_profile()";

    try
    {
      return get_user_host_(get_user_host_index_(user_id_info))->
        remove_user_profile(user_id_info);
    }
    catch(const AdServer::UserInfoSvcs::UserInfoManager::NotReady& e)
    {
      throw;
    }
    catch(const AdServer::UserInfoSvcs::UserInfoManager::ImplementationException& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught ImplementationException: " << e.description;

      CORBACommons::throw_desc<
        AdServer::UserInfoSvcs::UserInfoManager::ImplementationException>(
          ostr.str());
    }
    catch(const AdServer::UserInfoSvcs::UserInfoManager::ChunkNotFound&)
    {
      throw;
    }
    catch(const CORBA::SystemException& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught CORBA::SystemException: " << ex;

      CORBACommons::throw_desc<
        AdServer::UserInfoSvcs::UserInfoManager::ImplementationException>(
          ostr.str());
    }
    return 0; // never reach
  }

  void
  UserInfoManagerSessionImpl::update_user_freq_caps(
    const CORBACommons::UserIdInfo& user_id,
    const CORBACommons::TimestampInfo& time,
    const CORBACommons::RequestIdInfo& request_id,
    const AdServer::UserInfoSvcs::FreqCapIdSeq& freq_caps,
    const AdServer::UserInfoSvcs::FreqCapIdSeq& uc_freq_caps,
    const AdServer::UserInfoSvcs::FreqCapIdSeq& virtual_freq_caps,
    const AdServer::UserInfoSvcs::UserInfoManager::SeqOrderSeq& seq_orders,
    const AdServer::UserInfoSvcs::CampaignIdSeq& campaign_ids,
    const AdServer::UserInfoSvcs::CampaignIdSeq& uc_campaign_ids)
    /*throw(AdServer::UserInfoSvcs::UserInfoManager::NotReady,
      AdServer::UserInfoSvcs::UserInfoManager::ImplementationException,
      AdServer::UserInfoSvcs::UserInfoManager::ChunkNotFound)*/
  {
    static const char* FUN = "UserInfoManagerSessionImpl::post_match()";

    try
    {
      get_user_host_(get_user_host_index_(user_id))->update_user_freq_caps(
        user_id,
        time,
        request_id,
        freq_caps,
        uc_freq_caps,
        virtual_freq_caps,
        seq_orders,
        campaign_ids,
        uc_campaign_ids);
    }
    catch(const AdServer::UserInfoSvcs::UserInfoManager::NotReady& e)
    {
      throw;
    }
    catch(const AdServer::UserInfoSvcs::
          UserInfoManager::ImplementationException& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught ImplementationException: " << e.description;

      CORBACommons::throw_desc<
        AdServer::UserInfoSvcs::UserInfoManager::ImplementationException>(
          ostr.str());
    }
    catch(const AdServer::UserInfoSvcs::UserInfoManager::ChunkNotFound&)
    {
      throw;
    }
    catch(const CORBA::SystemException& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught CORBA::SystemException: " << ex;

      CORBACommons::throw_desc<
        AdServer::UserInfoSvcs::UserInfoManager::ImplementationException>(
          ostr.str());
    }
  }

  void UserInfoManagerSessionImpl::confirm_user_freq_caps(
    const CORBACommons::UserIdInfo& user_id,
    const CORBACommons::TimestampInfo& time,
    const CORBACommons::RequestIdInfo& request_id,
    const CORBACommons::IdSeq& exclude_pubpixel_accounts)
    /*throw(AdServer::UserInfoSvcs::UserInfoManager::NotReady,
      AdServer::UserInfoSvcs::UserInfoManager::ImplementationException,
      AdServer::UserInfoSvcs::UserInfoManager::ChunkNotFound)*/
  {
    static const char* FUN = "UserInfoManagerSessionImpl::consider_user_freq_caps()";

    try
    {
      get_user_host_(get_user_host_index_(user_id))->confirm_user_freq_caps(
        user_id,
        time,
        request_id,
        exclude_pubpixel_accounts);
    }
    catch(const AdServer::UserInfoSvcs::UserInfoManager::NotReady& e)
    {
      throw;
    }
    catch(const AdServer::UserInfoSvcs::
          UserInfoManager::ImplementationException& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught ImplementationException: " << e.description;

      CORBACommons::throw_desc<
        AdServer::UserInfoSvcs::UserInfoManager::ImplementationException>(
          ostr.str());
    }
    catch(const AdServer::UserInfoSvcs::UserInfoManager::ChunkNotFound&)
    {
      throw;
    }
    catch(const CORBA::SystemException& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught CORBA::SystemException: " << ex;

      CORBACommons::throw_desc<
        AdServer::UserInfoSvcs::UserInfoManager::ImplementationException>(
          ostr.str());
    }
  }

  void UserInfoManagerSessionImpl::consider_publishers_optin(
    const CORBACommons::UserIdInfo& user_id,
    const CORBACommons::IdSeq& exclude_pubpixel_accounts,
    const CORBACommons::TimestampInfo& now)
    /*throw(AdServer::UserInfoSvcs::UserInfoManager::NotReady,
          AdServer::UserInfoSvcs::UserInfoManager::ImplementationException,
          AdServer::UserInfoSvcs::UserInfoManager::ChunkNotFound)*/
  {
    static const char* FUN = "UserInfoManagerSessionImpl::consider_publishers_optin()";

    try
    {
      get_user_host_(get_user_host_index_(user_id))->
        consider_publishers_optin(
          user_id,
          exclude_pubpixel_accounts,
          now);
    }
    catch(const AdServer::UserInfoSvcs::UserInfoManager::NotReady& e)
    {
      throw;
    }
    catch(const AdServer::UserInfoSvcs::
          UserInfoManager::ImplementationException& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught ImplementationException: "
        << e.description;

      CORBACommons::throw_desc<
        AdServer::UserInfoSvcs::UserInfoManager::ImplementationException>(
          ostr.str());
    }
    catch(const AdServer::UserInfoSvcs::UserInfoManager::ChunkNotFound&)
    {
      throw;
    }
    catch(const CORBA::SystemException& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught CORBA::SystemException: " << ex;

      CORBACommons::throw_desc<
        AdServer::UserInfoSvcs::UserInfoManager::ImplementationException>(
          ostr.str());
    }
  }
  
  void UserInfoManagerSessionImpl::clear_expired(
    CORBA::Boolean synch,
    const CORBACommons::TimestampInfo& cleanup_time,
    CORBA::Long portion)
    /*throw(AdServer::UserInfoSvcs::UserInfoManager::ImplementationException)*/
  {
    static const char* FUN = "UserInfoManagerSessionImpl::clear_expired()";

    try
    {
      for(unsigned int i = 0; i < user_info_managers().length(); ++i)
      {
        user_info_managers()[i].user_info_manager->clear_expired(
          synch, cleanup_time, portion);
      }
    }
    catch(const AdServer::UserInfoSvcs::UserInfoManager::ImplementationException& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught ImplementationException: " << e.description;

      CORBACommons::throw_desc<
        AdServer::UserInfoSvcs::UserInfoManager::ImplementationException>(
          ostr.str());
    }
    catch(const CORBA::SystemException& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught CORBA::SystemException: " << ex;

      CORBACommons::throw_desc<
        AdServer::UserInfoSvcs::UserInfoManager::ImplementationException>(
          ostr.str());
    }
  }

}
}
