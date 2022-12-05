#include <RequestInfoSvcs/RequestInfoCommons/UserChannelInventoryProfile.hpp>
#include <RequestInfoSvcs/ExpressionMatcher/Compatibility/UserChannelInventoryProfile_v281.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/Algs.hpp>
#include <Commons/Algs.hpp>

#include "UserColoReachProfileAdapter.hpp"

namespace AdServer
{
namespace RequestInfoSvcs
{
  namespace
  {
    namespace RequestInfoSvcs_v0
    {
      namespace RequestInfoSvcsTarget
      {
        using namespace ::AdServer::RequestInfoSvcs; // v34
      }

      void
      copy_colo_appears_(
        RequestInfoSvcsTarget::UserChannelInventoryProfileWriter::
          colo_appears_Container& res,
        const AdServer::RequestInfoSvcs_v281::UserChannelInventoryProfileReader::
          colo_appears_Container& src)
      {
        for(AdServer::RequestInfoSvcs_v281::UserChannelInventoryProfileReader::
              colo_appears_Container::const_iterator it = src.begin();
            it != src.end(); ++it)
        {
          RequestInfoSvcsTarget::ColoIdAppearanceWriter colo_ap;
          colo_ap.id() = (*it).id();
          colo_ap.last_appearance_date() = (*it).last_appearance_date();
          colo_ap.prev_appearances_mask() = (*it).prev_appearances_mask();
          res.push_back(colo_ap);
        }
      }

      Generics::ConstSmartMemBuf_var
      convert_to_v34(const Generics::MemBuf& membuf)
      {
        const AdServer::RequestInfoSvcs_v281::UserChannelInventoryProfileReader
          old_reader(membuf.data(), membuf.size());
        RequestInfoSvcsTarget::UserChannelInventoryProfileWriter writer;
        writer.version() = 34;
        writer.create_time() = old_reader.create_time();

        copy_colo_appears_(writer.colo_appears(), old_reader.colo_appears());
        copy_colo_appears_(writer.isp_colo_appears(), old_reader.isp_colo_appears());

        // Fill isp_colo_create_time with colos from colo_appears and old_reader.isp_create_time()
        {
          RequestInfoSvcsTarget::UserChannelInventoryProfileWriter::
            isp_colo_create_time_Container& dst = writer.isp_colo_create_time();

          const AdServer::RequestInfoSvcs_v281::UserChannelInventoryProfileReader::
            colo_appears_Container& src = old_reader.colo_appears();

          for(AdServer::RequestInfoSvcs_v281::UserChannelInventoryProfileReader::
                colo_appears_Container::const_iterator it = src.begin();
              it != src.end(); ++it)
          {
            RequestInfoSvcsTarget::ColoCreateTimeWriter colo_ap;
            colo_ap.id() = (*it).id();
            colo_ap.create_time() = old_reader.isp_create_time();
            dst.push_back(colo_ap);
          }
        }

        return Algs::save_to_membuf(writer);
      }
    }
  }

  UserColoReachProfileAdapter::UserColoReachProfileAdapter()
    noexcept
  {}

  Generics::ConstSmartMemBuf_var
  UserColoReachProfileAdapter::operator()(
    const Generics::ConstSmartMemBuf* mem_buf)
    /*throw(Exception)*/
  {
    static const char* FUN = "UserColoReachProfileAdapter::operator()";

    // check profile version and convert to latest if version is old
    const unsigned long version_head_size =
      UserChannelInventoryProfileVersionReader::FIXED_SIZE;

    if(mem_buf->membuf().size() < version_head_size)
    {
      Stream::Error ostr;
      ostr << FUN << "Corrupt header: size of profile = " <<
        mem_buf->membuf().size();
      throw Exception(ostr);
    }

    const UserChannelInventoryProfileVersionReader version_reader(
      mem_buf->membuf().data(),
      version_head_size);

    Generics::ConstSmartMemBuf_var result_mem_buf =
      ReferenceCounting::add_ref(mem_buf);

    if(version_reader.version() != CURRENT_USER_COLO_REACH_PROFILE_VERSION)
    {
      const unsigned long original_version = version_reader.version();
      unsigned long current_version = original_version;

      try
      {
        if(current_version == 0)
        {
          result_mem_buf = RequestInfoSvcs_v0::convert_to_v34(result_mem_buf->membuf());
          current_version = 34;
        }
      }
      catch(const PlainTypes::CorruptedStruct& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": at version=" << original_version <<
          " adaptation, caught PlainTypes::CorruptedStruct: " << ex.what();
        throw Exception(ostr);
      }

      if(current_version != CURRENT_USER_COLO_REACH_PROFILE_VERSION)
      {
        Stream::Error ostr;
        ostr << FUN << ": incorrect version after adaptation = " <<
          current_version;
        throw Exception(ostr);
      }
    }

    return result_mem_buf;
  }
}
}
