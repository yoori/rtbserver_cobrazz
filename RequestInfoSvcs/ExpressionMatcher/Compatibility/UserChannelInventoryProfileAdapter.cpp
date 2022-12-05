#include <RequestInfoSvcs/RequestInfoCommons/Algs.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/UserChannelInventoryProfile.hpp>
#include <RequestInfoSvcs/ExpressionMatcher/InventoryActionProcessor.hpp>

#include <RequestInfoSvcs/ExpressionMatcher/Compatibility/UserChannelInventoryProfile_v281.hpp>

#include "UserChannelInventoryProfileAdapter.hpp"

namespace AdServer
{
namespace RequestInfoSvcs
{
  namespace
  {
    namespace RequestInfoSvcs_v281
    {
      namespace RequestInfoSvcsTarget
      {
        using namespace ::AdServer::RequestInfoSvcs; // v34
      }

      void copy_colo_appears_(
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
        // convert profile
        const AdServer::RequestInfoSvcs_v281::UserChannelInventoryProfileReader
          old_reader(membuf.data(), membuf.size());
        RequestInfoSvcsTarget::UserChannelInventoryProfileWriter writer;
        writer.version() = 34;
        writer.create_time() = old_reader.create_time();
        writer.last_request_time() = old_reader.last_request_time();
        writer.last_daily_processing_time() = old_reader.last_daily_processing_time();
        writer.sum_revenue() = old_reader.sum_revenue();
        writer.imp_count() = old_reader.imp_count();

        copy_colo_appears_(writer.colo_appears(), old_reader.colo_appears());
        copy_colo_appears_(writer.colo_ad_appears(), old_reader.colo_ad_appears());
        copy_colo_appears_(writer.colo_merge_appears(), old_reader.colo_merge_appears());
        copy_colo_appears_(writer.isp_colo_appears(), old_reader.isp_colo_appears());
        copy_colo_appears_(writer.isp_colo_ad_appears(), old_reader.isp_colo_ad_appears());
        copy_colo_appears_(writer.isp_colo_merge_appears(), old_reader.isp_colo_merge_appears());

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

        for(AdServer::RequestInfoSvcs_v281::UserChannelInventoryProfileReader::
            days_Container::const_iterator day_it =
              old_reader.days().begin();
            day_it != old_reader.days().end(); ++day_it)
        {
          RequestInfoSvcsTarget::ChannelInventoryDayWriter day_writer;
          day_writer.date() = (*day_it).date();
          std::copy((*day_it).total_channel_list().begin(),
            (*day_it).total_channel_list().end(),
            std::back_inserter(day_writer.total_channel_list()));
          std::copy((*day_it).active_channel_list().begin(),
            (*day_it).active_channel_list().end(),
            std::back_inserter(day_writer.active_channel_list()));

          std::copy((*day_it).display_impop_no_imp_channel_list().begin(),
            (*day_it).display_impop_no_imp_channel_list().end(),
            std::back_inserter(day_writer.display_impop_no_imp_channel_list()));
          std::copy((*day_it).display_imp_other_channel_list().begin(),
            (*day_it).display_imp_other_channel_list().end(),
            std::back_inserter(day_writer.display_imp_other_channel_list()));
          std::copy((*day_it).display_imp_channel_list().begin(),
            (*day_it).display_imp_channel_list().end(),
            std::back_inserter(day_writer.display_imp_channel_list()));

          std::copy((*day_it).text_impop_no_imp_channel_list().begin(),
            (*day_it).text_impop_no_imp_channel_list().end(),
            std::back_inserter(day_writer.text_impop_no_imp_channel_list()));
          std::copy((*day_it).text_imp_other_channel_list().begin(),
            (*day_it).text_imp_other_channel_list().end(),
            std::back_inserter(day_writer.text_imp_other_channel_list()));
          std::copy((*day_it).text_imp_channel_list().begin(),
            (*day_it).text_imp_channel_list().end(),
            std::back_inserter(day_writer.text_imp_channel_list()));

          for(AdServer::RequestInfoSvcs_v281::ChannelInventoryDayReader::
                channel_price_ranges_Container::const_iterator it =
                (*day_it).channel_price_ranges().begin();
              it != (*day_it).channel_price_ranges().end(); ++it)
          {
            RequestInfoSvcsTarget::ChInvByCMPCellWriter new_el;
            new_el.ecpm() = (*it).ecpm();
            new_el.country() = (*it).country();
            new_el.tag_size() = (*it).tag_size();
            std::copy((*it).channel_list().begin(),
              (*it).channel_list().end(),
              std::back_inserter(new_el.channel_list()));
            day_writer.channel_price_ranges().push_back(new_el);
          }

          writer.days().push_back(day_writer);
        }

        return Algs::save_to_membuf(writer);
      }
    }
  }

  Generics::ConstSmartMemBuf_var
  UserChannelInventoryProfileAdapter::operator()(
    const Generics::ConstSmartMemBuf* mem_buf)
    /*throw(Exception)*/
  {
    static const char* FUN = "UserInventoryProfileAdapter::operator()";

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

    if(version_reader.version() != CURRENT_CHANNEL_INVENTORY_PROFILE_VERSION)
    {
      const unsigned long original_version = version_reader.version();
      unsigned long current_version = original_version;

      try
      {
        if(current_version == 281) // 2.8
        {
          result_mem_buf = RequestInfoSvcs_v281::convert_to_v34(result_mem_buf->membuf());
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

      if(current_version != CURRENT_CHANNEL_INVENTORY_PROFILE_VERSION)
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
