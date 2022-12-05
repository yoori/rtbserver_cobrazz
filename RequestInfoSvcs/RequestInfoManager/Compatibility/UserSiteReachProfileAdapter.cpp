#include <Generics/Time.hpp>

#include <Commons/Algs.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/Algs.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/UserSiteReachProfile.hpp>
#include <RequestInfoSvcs/RequestInfoManager/Compatibility/UserSiteReachProfile_v24.hpp>

#include "UserSiteReachProfileAdapter.hpp"

namespace AdServer
{
namespace RequestInfoSvcs
{
  Generics::ConstSmartMemBuf_var
  UserSiteReachProfileAdapter::operator()(
    const Generics::ConstSmartMemBuf* mem_buf_ptr)
    /*throw(Exception)*/
  {
    if(mem_buf_ptr->membuf().size() == 0)
    {
      return Generics::ConstSmartMemBuf_var();
    }

    unsigned long version_head_size = UserSiteReachProfileVersionReader::FIXED_SIZE;
    if(mem_buf_ptr->membuf().size() < version_head_size)
    {
      Stream::Error ostr;
      ostr << "Invalid user site reach profile: size = " <<
        mem_buf_ptr->membuf().size() << " < " <<
        version_head_size;
      throw Exception(ostr);
    }

    UserSiteReachProfileVersionReader version_reader(
      mem_buf_ptr->membuf().data(),
      version_head_size);

    if(version_reader.version() != CURRENT_USER_SITE_REACH_PROFILE_VERSION)
    {
      Generics::SmartMemBuf_var result_mem_buf = Algs::copy_membuf(mem_buf_ptr);
      Generics::MemBuf& membuf = result_mem_buf->membuf();

      UserSiteReachProfileWriter new_writer;
      new_writer.version() = CURRENT_USER_SITE_REACH_PROFILE_VERSION;

      if(version_reader.version() == 0) // <= 2.4
      {
        try
        {
          AdServer::RequestInfoSvcs_v24::UserSiteReachProfileReader old_reader(
            membuf.data(), membuf.size());

          for(AdServer::RequestInfoSvcs_v24::UserSiteReachProfileReader::
                monthly_appear_lists_Container::const_iterator it =
                  old_reader.monthly_appear_lists().begin();
              it != old_reader.monthly_appear_lists().end(); ++it)
          {
            for(AdServer::RequestInfoSvcs_v24::SiteDateReachReader::
                  appear_list_Container::const_iterator ait =
                    (*it).appear_list().begin();
                ait != (*it).appear_list().end(); ++ait)
            {
              update_appearance(
                new_writer.appearance_list(),
                *ait,
                Generics::Time((*it).date()),
                BaseAppearanceLess(),
                BaseInsertAppearanceAdapter<SiteAppearanceWriter>());
            }
          }

          for(AdServer::RequestInfoSvcs_v24::UserSiteReachProfileReader::
                daily_appear_lists_Container::const_iterator it =
                  old_reader.daily_appear_lists().begin();
              it != old_reader.daily_appear_lists().end(); ++it)
          {
            for(AdServer::RequestInfoSvcs_v24::SiteDateReachReader::
                  appear_list_Container::const_iterator ait =
                    (*it).appear_list().begin();
                ait != (*it).appear_list().end(); ++ait)
            {
              update_appearance(
                new_writer.appearance_list(),
                *ait,
                Generics::Time((*it).date()),
                BaseAppearanceLess(),
                BaseInsertAppearanceAdapter<SiteAppearanceWriter>());
            }
          }
        }
        catch(const eh::Exception& ex)
        {
          Stream::Error ostr;
          ostr << "Can't adapt profile with version 2.4(0): " <<
            ex.what();
          throw Exception(ostr);
        }
      }
      else
      {
        Stream::Error ostr;
        ostr << "Unknown user site reach profile version: " << version_reader.version();
        throw Exception(ostr);
      }

      Generics::MemBuf mb(new_writer.size());
      new_writer.save(mb.data(), mb.size());
      membuf.assign(mb.data(), mb.size());

      return Generics::transfer_membuf(result_mem_buf);
    }

    return ReferenceCounting::add_ref(mem_buf_ptr);
  }
} /*RequestInfoSvcs*/
} /*AdServer*/
