#include <Commons/CorbaAlgs.hpp>

#include <utility>

#include "UserInfoManagerControlImpl.hpp"

namespace AdServer::UserInfoSvcs
{

  /**
   * UserInfoManagerControlImpl
   */

  UserInfoManagerControlImpl::UserInfoManagerControlImpl(
    UserInfoManagerCorePtr user_info_manager_impl)
    /*throw(Exception)*/
    : user_info_manager_impl_(std::move(user_info_manager_impl))
  {
  }

  UserInfoManagerControlImpl::~UserInfoManagerControlImpl() noexcept
  {
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

      UserInfoManagerCore::ChunkIdList chunk_ids;
      user_info_manager_impl_->get_controllable_chunks(
        chunk_ids, common_chunks_number);

      user_info_source->common_chunks_number = common_chunks_number;

      CorbaAlgs::fill_sequence(chunk_ids.begin(),
        chunk_ids.end(), user_info_source->chunk_ids);
    }
    catch(const UserInfoManagerCore::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": "
        "Can't get chunk ids. Caught UserInfoManagerCore::Exception: " <<
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
  }

} /* UserInfoSvcs */
