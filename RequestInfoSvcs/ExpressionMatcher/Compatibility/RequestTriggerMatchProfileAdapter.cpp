#include <Commons/Algs.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/Algs.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/RequestTriggerMatchProfile.hpp>

#include <RequestInfoSvcs/ExpressionMatcher/Compatibility/RequestTriggerMatchProfile_v330.hpp>

#include "RequestTriggerMatchProfileAdapter.hpp"

namespace AdServer
{
namespace RequestInfoSvcs
{
  namespace
  {
    namespace RequestInfoSvcs_v330
    {
      void
      copy_triggers(
        RequestTriggerMatchWriter::page_match_counters_Container& res,
        const AdServer::RequestInfoSvcs_v330::RequestTriggerMatchReader::
          page_match_counters_Container& src)
      {
        for(auto src_it = src.begin(); src_it != src.end(); ++src_it)
        {
          TriggerMatchCounterWriter new_writer;
          new_writer.channel_trigger_id() = (*src_it).channel_trigger_id();
          new_writer.counter() = (*src_it).counter();
          new_writer.channel_id() = 0;
          res.push_back(new_writer);
        }
      }

      Generics::ConstSmartMemBuf_var
      convert_to_v360(const Generics::MemBuf& membuf)
      {
        const AdServer::RequestInfoSvcs_v330::RequestTriggerMatchReader
          old_reader(membuf.data(), membuf.size());
        RequestTriggerMatchWriter writer;
        writer.version() = 360;

        writer.time() = old_reader.time();
        writer.channels().assign(old_reader.channels().begin(), old_reader.channels().end());
        copy_triggers(writer.page_match_counters(), old_reader.page_match_counters());
        copy_triggers(writer.search_match_counters(), old_reader.search_match_counters());
        copy_triggers(writer.url_match_counters(), old_reader.url_match_counters());
        copy_triggers(writer.url_keyword_match_counters(), old_reader.url_keyword_match_counters());
        writer.click_done() = old_reader.click_done();
        writer.impression_done() = old_reader.impression_done();

        return Algs::save_to_membuf(writer);
      }
    }
  }

  Generics::ConstSmartMemBuf_var
  RequestTriggerMatchProfileAdapter::operator()(
    const Generics::ConstSmartMemBuf* mem_buf) /*throw(Exception)*/
  {
    static const char* FUN = "RequestTriggerMatchProfileAdapter::operator()";

    /* check profile version and convert to latest if version is old */
    const unsigned long version_head_size =
      RequestTriggerMatchVersionReader::FIXED_SIZE;

    if(mem_buf->membuf().size() < version_head_size)
    {
      Stream::Error ostr;
      ostr << FUN << "Corrupt header: size of profile = " <<
        mem_buf->membuf().size();
      throw Exception(ostr);
    }

    const RequestTriggerMatchVersionReader version_reader(
      mem_buf->membuf().data(),
      version_head_size);

    Generics::ConstSmartMemBuf_var result_mem_buf =
      ReferenceCounting::add_ref(mem_buf);

    if(version_reader.version() != CURRENT_REQUEST_TRIGGER_MATCH_PROFILE_VERSION)
    {
      const unsigned long original_version = version_reader.version();
      unsigned long current_version = version_reader.version();

      try
      {
        if (current_version == 330)
        {
          result_mem_buf = RequestInfoSvcs_v330::convert_to_v360(result_mem_buf->membuf());
          current_version = 360;
        }
      }
      catch(const PlainTypes::CorruptedStruct& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": at version=" << original_version <<
          " adaptation, caught PlainTypes::CorruptedStruct: " << ex.what();
        throw Exception(ostr);
      }

      if(current_version != CURRENT_REQUEST_TRIGGER_MATCH_PROFILE_VERSION)
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
