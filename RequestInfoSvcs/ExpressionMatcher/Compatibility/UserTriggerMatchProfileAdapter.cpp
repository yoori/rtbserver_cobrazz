#include <Commons/Algs.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/Algs.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/UserTriggerMatchProfile.hpp>

#include <RequestInfoSvcs/ExpressionMatcher/Compatibility/UserTriggerMatchProfile_v0.hpp>

#include "UserTriggerMatchProfileAdapter.hpp"

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
        using namespace ::AdServer::RequestInfoSvcs;
      }

      void copy_matches_(
        RequestInfoSvcsTarget::UserTriggerMatchWriter::page_matches_Container& res,
        const ::AdServer::RequestInfoSvcs_v0::UserTriggerMatchReader::page_matches_Container& src)
      {
        for(::AdServer::RequestInfoSvcs_v0::UserTriggerMatchReader::
              page_matches_Container::const_iterator src_it = src.begin();
            src_it != src.end(); ++src_it)
        {
          RequestInfoSvcsTarget::ChannelMatchWriter new_writer;
          new_writer.channel_id() = (*src_it).channel_id();
          new_writer.time() = (*src_it).time();
          new_writer.triggers_in_channel() = (*src_it).triggers_in_channel();
          new_writer.matched_triggers() = (*src_it).matched_triggers();
          std::copy((*src_it).positive_matches().begin(),
            (*src_it).positive_matches().end(),
            std::back_inserter(new_writer.positive_matches()));
          std::copy((*src_it).negative_matches().begin(),
            (*src_it).negative_matches().end(),
            std::back_inserter(new_writer.negative_matches()));
          res.push_back(new_writer);
        }
      }

      void copy_impressions_(
        RequestInfoSvcsTarget::UserTriggerMatchWriter::impressions_Container& res,
        const ::AdServer::RequestInfoSvcs_v0::UserTriggerMatchReader::impressions_Container& src)
      {
        for(::AdServer::RequestInfoSvcs_v0::UserTriggerMatchReader::
              impressions_Container::const_iterator src_it = src.begin();
            src_it != src.end(); ++src_it)
        {
          RequestInfoSvcsTarget::ImpressionMarkerWriter new_writer;
          new_writer.time() = (*src_it).time();
          new_writer.request_id() = (*src_it).request_id();
          res.push_back(new_writer);
        }
      }

      Generics::ConstSmartMemBuf_var
      convert_to_v330(const Generics::MemBuf& membuf)
      {
        // convert profile
        const ::AdServer::RequestInfoSvcs_v0::UserTriggerMatchReader
          old_reader(membuf.data(), membuf.size());
        RequestInfoSvcsTarget::UserTriggerMatchWriter writer;
        writer.version() = 330;

        copy_matches_(writer.page_matches(), old_reader.page_matches());
        copy_matches_(writer.search_matches(), old_reader.search_matches());
        copy_matches_(writer.url_matches(), old_reader.url_matches());
        copy_impressions_(writer.impressions(), old_reader.impressions());

        return Algs::save_to_membuf(writer);
      }
    }
  }

  Generics::ConstSmartMemBuf_var
  UserTriggerMatchProfileAdapter::operator()(
    const Generics::ConstSmartMemBuf* mem_buf) /*throw(Exception)*/
  {
    static const char* FUN = "UserTriggerMatchProfileAdapter::operator()";

    /* check profile version and convert to latest if version is old */
    const unsigned long version_head_size =
      UserTriggerMatchVersionReader::FIXED_SIZE;

    if(mem_buf->membuf().size() < version_head_size)
    {
      Stream::Error ostr;
      ostr << FUN << "Corrupt header: size of profile = " << mem_buf->membuf().size();
      throw Exception(ostr);
    }

    const UserTriggerMatchVersionReader version_reader(
      mem_buf->membuf().data(),
      version_head_size);

    Generics::ConstSmartMemBuf_var result_mem_buf =
      ReferenceCounting::add_ref(mem_buf);

    if(version_reader.version() != CURRENT_USER_TRIGGER_MATCH_PROFILE_VERSION)
    {
      const unsigned long original_version = version_reader.version();
      unsigned long current_version = version_reader.version();

      try
      {
        if(current_version != CURRENT_USER_TRIGGER_MATCH_PROFILE_VERSION)
        {
          result_mem_buf = RequestInfoSvcs_v0::convert_to_v330(result_mem_buf->membuf());
          current_version = 330;
        }
      }
      catch(const PlainTypes::CorruptedStruct& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": at version=" << original_version <<
          " adaptation, caught PlainTypes::CorruptedStruct: " << ex.what();
        throw Exception(ostr);
      }

      if(current_version != CURRENT_USER_TRIGGER_MATCH_PROFILE_VERSION)
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
