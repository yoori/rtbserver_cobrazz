#include <Generics/Time.hpp>
#include <Commons/UserInfoManip.hpp>
#include <Commons/Algs.hpp>

#include <RequestInfoSvcs/RequestInfoCommons/RequestOperationProfile.hpp>
#include <RequestInfoSvcs/RequestInfoManager/Compatibility/RequestOperationImpressionProfile_v0.hpp>

#include "RequestOperationImpressionProfileAdapter.hpp"

namespace AdServer
{
namespace RequestInfoSvcs
{
namespace
{
  namespace RequestInfoSvcs_v1
  {
    void convert_to_v331(Generics::MemBuf& membuf)
    {
      AdServer::RequestInfoSvcs::RequestOperationImpressionWriter
        profile_writer;

      AdServer::RequestInfoSvcs_v0::RequestOperationImpressionReader
        old_profile_reader(membuf.data(), membuf.size());

      profile_writer.version() = 331;
      profile_writer.time() = old_profile_reader.time();
      profile_writer.request_id() = old_profile_reader.request_id();
      profile_writer.verify_impression() = old_profile_reader.verify_impression();
      profile_writer.pub_revenue() = old_profile_reader.pub_revenue();
      profile_writer.pub_sys_revenue() = old_profile_reader.pub_sys_revenue();
      profile_writer.pub_revenue_type() = old_profile_reader.pub_revenue_type();
      profile_writer.user_id() = AdServer::Commons::UserId().to_string();

      Generics::MemBuf mb(profile_writer.size());
      profile_writer.save(mb.data(), mb.size());
      membuf.assign(mb.data(), mb.size());
    }
  }
}
}
}

namespace AdServer
{
namespace RequestInfoSvcs
{
  void
  RequestOperationImpressionProfileAdapter::operator()(
    Generics::MemBuf& membuf) /*throw(Exception)*/
  {
    static const char* FUN = "RequestOperationImpressionProfileAdapter::operator()";

    unsigned long version_head_size =
      RequestOperationVersionReader::FIXED_SIZE;

    if(membuf.size() < version_head_size)
    {
      throw Exception("Corrupt header");
    }

    try
    {
      RequestOperationVersionReader version_reader(
        membuf.data(), membuf.size());

      if(version_reader.version() != AdServer::
         RequestInfoSvcs::CURRENT_REQUESTOPERATIONIMPRESSION_PROFILE_VERSION)
      {
        unsigned long current_version = version_reader.version();

        if(current_version == 0) // previous version
        {
          RequestInfoSvcs_v1::convert_to_v331(membuf);
          current_version = 331;
        }

        if(current_version != AdServer::RequestInfoSvcs::
           CURRENT_REQUESTOPERATIONIMPRESSION_PROFILE_VERSION)
        {
          Stream::Error ostr;
          ostr << FUN << ": incorrect version after adaptation = " <<
            current_version;
          throw Exception(ostr);
        }
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }
}
}
