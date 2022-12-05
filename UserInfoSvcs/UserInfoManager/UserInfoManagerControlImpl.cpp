#include <Commons/CorbaAlgs.hpp>

#include "UserInfoManagerControlImpl.hpp"

namespace AdServer{
namespace UserInfoSvcs{

  /**
   * UserInfoManagerControlImpl
   */
  
  UserInfoManagerControlImpl::UserInfoManagerControlImpl(
    UserInfoManagerImpl* user_info_manager_impl)
    /*throw(Exception)*/
    : user_info_manager_impl_(ReferenceCounting::add_ref(user_info_manager_impl)),
      admitted_(false)
  {
  }

  UserInfoManagerControlImpl::~UserInfoManagerControlImpl() noexcept
  {
  }

  AdServer::UserInfoSvcs::UserInfoManagerStatus
  UserInfoManagerControlImpl::status() noexcept
  {
    ReadGuard_ lock(lock_);
    
    if(admitted_)
    {
      return AdServer::UserInfoSvcs::S_ADMITTED;
    }

    return AdServer::UserInfoSvcs::S_READY;
  }
  
  void 
  UserInfoManagerControlImpl::get_source_info(
    AdServer::UserInfoSvcs::ChunksConfig_out user_info_source)
    /*throw(AdServer::UserInfoSvcs::UserInfoManagerControl::ImplementationException)*/
  {
    static const char* FUN = "UserInfoManagerControlImpl::get_source_info()";

    try
    {
      user_info_source = new AdServer::UserInfoSvcs::ChunksConfig();

      unsigned long common_chunks_number;
      
      UserInfoManagerImpl::ChunkIdList chunk_ids;
      user_info_manager_impl_->get_controllable_chunks(
        chunk_ids, common_chunks_number);

      user_info_source->common_chunks_number = common_chunks_number;
      
      CorbaAlgs::fill_sequence(chunk_ids.begin(),
        chunk_ids.end(), user_info_source->chunk_ids);
    }
    catch(const UserInfoManagerImpl::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": "
        "Can't get chunk ids. Caught UserInfoManagerImpl::Exception: " <<
        ex.what();
      CORBACommons::throw_desc<
        UserInfoSvcs::UserInfoManagerControl::ImplementationException>(
          ostr.str());
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": "
        "Can't get chunk ids. Caught eh::Exception: " << ex.what();
      CORBACommons::throw_desc<
        UserInfoSvcs::UserInfoManagerControl::ImplementationException>(
          ostr.str());
    }
  }

  void UserInfoManagerControlImpl::admit() noexcept
  {
    WriteGuard_ lock(lock_);
    admitted_ = true;
  }
  
} /*UserInfoSvcs*/
} /*AdServer*/

