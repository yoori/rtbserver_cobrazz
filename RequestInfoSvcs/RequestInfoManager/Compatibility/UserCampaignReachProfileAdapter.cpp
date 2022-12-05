#include <Generics/Time.hpp>

#include <Commons/Algs.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/Algs.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/UserCampaignReachProfile.hpp>
#include <RequestInfoSvcs/RequestInfoManager/Compatibility/UserCampaignReachProfile_v24.hpp>

#include "UserCampaignReachProfileAdapter.hpp"

namespace AdServer
{
namespace RequestInfoSvcs
{
  namespace
  {
    namespace RequestInfoSvcs_v24
    {
      using namespace AdServer::RequestInfoSvcs_v24;

      void convert_appearance(
        RequestInfoSvcs::UserCampaignReachProfileWriter::
          campaign_appears_Container& result,
        const ItemReachReader& old_reach_reader,
        const Generics::Time& time_for_total_save)
      {
        typedef AdServer::RequestInfoSvcs::BaseAppearanceLess AppearanceLess;
        typedef AdServer::RequestInfoSvcs::BaseInsertAppearanceAdapter<
          AdServer::RequestInfoSvcs::IdAppearanceWriter>
          InsertAppearanceAdapter;

        for(ItemReachReader::total_appear_list_Container::const_iterator ait =
              old_reach_reader.total_appear_list().begin();
            ait != old_reach_reader.total_appear_list().end(); ++ait)
        {
          update_appearance(
            result,
            *ait,
            time_for_total_save,
            AppearanceLess(),
            InsertAppearanceAdapter());
        }

        // save monthly appearances as appearances at first month day
        for(ItemReachReader::monthly_appear_lists_Container::const_iterator it =
              old_reach_reader.monthly_appear_lists().begin();
            it != old_reach_reader.monthly_appear_lists().end(); ++it)
        {
          for(AdServer::RequestInfoSvcs_v24::DateReachReader::
                appear_list_Container::const_iterator ait =
                  (*it).appear_list().begin();
              ait != (*it).appear_list().end(); ++ait)
          {
            update_appearance(
              result,
              *ait,
              Generics::Time((*it).date()),
              AppearanceLess(),
              InsertAppearanceAdapter());
          }
        }

        for(ItemReachReader::daily_appear_lists_Container::const_iterator it =
              old_reach_reader.daily_appear_lists().begin();
            it != old_reach_reader.daily_appear_lists().end(); ++it)
        {
          for(AdServer::RequestInfoSvcs_v24::DateReachReader::
                appear_list_Container::const_iterator ait =
                  (*it).appear_list().begin();
              ait != (*it).appear_list().end(); ++ait)
          {
            update_appearance(
              result,
              *ait,
              Generics::Time((*it).date()),
              AppearanceLess(),
              InsertAppearanceAdapter());
          }
        }
      }

      void
      convert_to_v25(Generics::MemBuf& membuf)
      {
        try
        {
          AdServer::RequestInfoSvcs::UserCampaignReachProfileWriter profile_writer;

          AdServer::RequestInfoSvcs_v24::UserCampaignReachProfileReader
            old_profile_reader(membuf.data(), membuf.size());
          Generics::Time time_for_total_save =
            Generics::Time(old_profile_reader.last_request_day()) -
            Generics::Time::ONE_DAY * 100;

          profile_writer.version() = AdServer::RequestInfoSvcs::CURRENT_CAMPAIGN_REACH_PROFILE_VERSION;
          convert_appearance(
            profile_writer.campaign_appears(),
            old_profile_reader.campaign_appears(),
            time_for_total_save);
          convert_appearance(
            profile_writer.ccg_appears(),
            old_profile_reader.ccg_appears(),
            time_for_total_save);
          convert_appearance(
            profile_writer.cc_appears(),
            old_profile_reader.cc_appears(),
            time_for_total_save);
          convert_appearance(
            profile_writer.adv_appears(),
            old_profile_reader.adv_appears(),
            time_for_total_save);
          convert_appearance(
            profile_writer.adv_display_appears(),
            old_profile_reader.adv_display_appears(),
            time_for_total_save);
          convert_appearance(
            profile_writer.adv_text_appears(),
            old_profile_reader.adv_text_appears(),
            time_for_total_save);

          Generics::MemBuf mb(profile_writer.size());
          profile_writer.save(mb.data(), mb.size());
          membuf.assign(mb.data(), mb.size());
        }
        catch(const eh::Exception& ex)
        {
          Stream::Error ostr;
          ostr << "Can't adapt profile with version = 2.4: " <<
            ex.what();
          throw UserCampaignReachProfileAdapter::Exception(ostr);
        }
      }
    } /*RequestInfoSvcs_v24*/
  }
}
}

namespace AdServer
{
namespace RequestInfoSvcs
{
  Generics::ConstSmartMemBuf_var
  UserCampaignReachProfileAdapter::operator()(
    const Generics::ConstSmartMemBuf* mem_buf) /*throw(Exception)*/
  {
    static const char* FUN = "UserCampaignReachProfileAdapter::operator()";

    /* check profile version and convert to latest if version is old */
    unsigned long version_head_size =
      UserCampaignReachProfileVersionReader::FIXED_SIZE;

    if(mem_buf->membuf().size() < version_head_size)
    {
      Stream::Error ostr;
      ostr << FUN << ": corrupted header, size of profile = " <<
        mem_buf->membuf().size();
      throw Exception(ostr);
    }

    UserCampaignReachProfileVersionReader version_reader(
      mem_buf->membuf().data(),
      mem_buf->membuf().size());

    if(version_reader.version() != CURRENT_CAMPAIGN_REACH_PROFILE_VERSION)
    {
      Generics::SmartMemBuf_var result_mem_buf = Algs::copy_membuf(mem_buf);
      Generics::MemBuf& membuf = result_mem_buf->membuf();

      unsigned long current_version = version_reader.version();

      try
      {
        if(current_version == 24 || current_version == 2)
          // v24 -> v25 (v24 generate profiles with version = 2)
        {
          RequestInfoSvcs_v24::convert_to_v25(membuf);
          current_version = CURRENT_CAMPAIGN_REACH_PROFILE_VERSION;
        }
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": Caught eh::Exception: " << ex.what();
        throw Exception(ostr);
      }

      if(current_version != CURRENT_CAMPAIGN_REACH_PROFILE_VERSION)
      {
        Stream::Error ostr;
        ostr << FUN << ": incorrect version after adaptation = " <<
          current_version;
        throw Exception(ostr);
      }

      return Generics::transfer_membuf(result_mem_buf);
    }

    return ReferenceCounting::add_ref(mem_buf);
  }
}
}
