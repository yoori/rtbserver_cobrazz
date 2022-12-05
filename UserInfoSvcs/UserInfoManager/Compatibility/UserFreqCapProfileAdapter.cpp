#include <Commons/Algs.hpp>

#include <UserInfoSvcs/UserInfoCommons/UserFreqCapProfileDescription.hpp>
#include <UserInfoSvcs/UserInfoCommons/UserFreqCapProfile.hpp>
#include <UserInfoSvcs/UserInfoManager/Compatibility/UserFreqCapProfile_v290.hpp>
#include <UserInfoSvcs/UserInfoManager/Compatibility/UserFreqCapProfile_v330.hpp>
#include <UserInfoSvcs/UserInfoManager/Compatibility/UserFreqCapProfile_v340.hpp>

#include "UserFreqCapProfileAdapter.hpp"

namespace AdServer
{
  namespace UserInfoSvcs_v340
  {
    namespace UserInfoSvcsTarget
    {
      using namespace ::AdServer::UserInfoSvcs;
    }

    void
    convert_to_v351(Generics::MemBuf& membuf)
    {
      UserInfoSvcsTarget::UserFreqCapProfileWriter profile_writer;
      profile_writer.version() =
        UserInfoSvcsTarget::CURRENT_FREQ_CAP_PROFILE_VERSION;

      const AdServer::UserInfoSvcs_v340::UserFreqCapProfileReader profile_reader(
        membuf.data(), membuf.size());

      profile_writer.freq_caps().insert(
        profile_writer.freq_caps().end(),
        profile_reader.freq_caps().begin(),
        profile_reader.freq_caps().end());

      for (auto it = profile_reader.uc_freq_caps().begin();
           it != profile_reader.uc_freq_caps().end(); ++it)
      {
        UserInfoSvcsTarget::UcFreqCapWriter uc_freq_cap;
        uc_freq_cap.request_id() = (*it).request_id();
        uc_freq_cap.time() = (*it).time();
        std::copy(
          (*it).freq_cap_ids().begin(),
          (*it).freq_cap_ids().end(),
          std::back_inserter(uc_freq_cap.freq_cap_ids()));
        profile_writer.uc_freq_caps().push_back(uc_freq_cap);
      }

      profile_writer.virtual_freq_caps().insert(
        profile_writer.virtual_freq_caps().end(),
        profile_reader.virtual_freq_caps().begin(),
        profile_reader.virtual_freq_caps().end());

      profile_writer.seq_orders().insert(
        profile_writer.seq_orders().end(),
        profile_reader.seq_orders().begin(),
        profile_reader.seq_orders().end());

      profile_writer.publisher_accounts().insert(
        profile_writer.publisher_accounts().end(),
        profile_reader.publisher_accounts().begin(),
        profile_reader.publisher_accounts().end());

      Generics::MemBuf mb(profile_writer.size());
      profile_writer.save(mb.data(), mb.size());
      membuf.assign(mb.data(), mb.size());
    }
  }

  namespace UserInfoSvcs_v291
  {
    namespace UserInfoSvcsTarget
    {
      using namespace ::AdServer::UserInfoSvcs_v340;
    }

    void
    convert_to_v340(Generics::MemBuf& membuf)
    {
      UserInfoSvcsTarget::UserFreqCapProfileWriter profile_writer;
      profile_writer.version() = 340;

      const AdServer::UserInfoSvcs_v330::UserFreqCapProfileReader profile_reader(
        membuf.data(), membuf.size());

      for(AdServer::UserInfoSvcs_v330::
            UserFreqCapProfileReader::freq_caps_Container::const_iterator it =
            profile_reader.freq_caps().begin();
          it != profile_reader.freq_caps().end(); ++it)
      {
        profile_writer.freq_caps().push_back(*it);
      }

      for(AdServer::UserInfoSvcs_v330::
            UserFreqCapProfileReader::uc_freq_caps_Container::const_iterator it =
            profile_reader.uc_freq_caps().begin();
          it != profile_reader.uc_freq_caps().end(); ++it)
      {
        profile_writer.uc_freq_caps().push_back(*it);
      }

      for(AdServer::UserInfoSvcs_v330::
            UserFreqCapProfileReader::freq_caps_Container::const_iterator it =
            profile_reader.virtual_freq_caps().begin();
          it != profile_reader.virtual_freq_caps().end(); ++it)
      {
        profile_writer.virtual_freq_caps().push_back(*it);
      }

      for(AdServer::UserInfoSvcs_v330::
            UserFreqCapProfileReader::seq_orders_Container::const_iterator it =
            profile_reader.seq_orders().begin();
          it != profile_reader.seq_orders().end(); ++it)
      {
        profile_writer.seq_orders().push_back(*it);
      }

      Generics::MemBuf mb(profile_writer.size());
      profile_writer.save(mb.data(), mb.size());
      membuf.assign(mb.data(), mb.size());
    }
  }

  namespace UserInfoSvcs_v290
  {
    namespace UserInfoSvcsTarget
    {
      using namespace ::AdServer::UserInfoSvcs_v330;
    }

    void
    convert_to_v291(Generics::MemBuf& membuf)
    {
      UserInfoSvcsTarget::UserFreqCapProfileWriter profile_writer;
      profile_writer.version() = 291;

      AdServer::UserInfoSvcs_v290::ProfileReader profile_reader(
        membuf.data(), membuf.size());

      for(AdServer::UserInfoSvcs_v290::
            ProfileReader::freq_caps_Container::const_iterator it =
            profile_reader.freq_caps().begin();
          it != profile_reader.freq_caps().end(); ++it)
      {
        UserInfoSvcsTarget::FreqCapWriter fc_writer;
        fc_writer.fc_id() = (*it).fc_id();
        fc_writer.total_impressions() = (*it).total_impressions();
        std::copy((*it).last_impressions().begin(),
          (*it).last_impressions().end(),
          std::back_inserter(fc_writer.last_impressions()));
        profile_writer.freq_caps().push_back(fc_writer);
      }

      for(AdServer::UserInfoSvcs_v290::
            ProfileReader::uc_freq_caps_Container::const_iterator it =
            profile_reader.uc_freq_caps().begin();
          it != profile_reader.uc_freq_caps().end(); ++it)
      {
        UserInfoSvcsTarget::UcFreqCapWriter ucfc_writer;
        ucfc_writer.request_id() = (*it).request_id();
        ucfc_writer.time() = (*it).time();
        std::copy((*it).freq_cap_ids().begin(),
          (*it).freq_cap_ids().end(),
          std::back_inserter(ucfc_writer.freq_cap_ids()));
        profile_writer.uc_freq_caps().push_back(ucfc_writer);
      }

      for(AdServer::UserInfoSvcs_v290::
            ProfileReader::freq_caps_Container::const_iterator it =
            profile_reader.virtual_freq_caps().begin();
          it != profile_reader.virtual_freq_caps().end(); ++it)
      {
        UserInfoSvcsTarget::FreqCapWriter fc_writer;
        fc_writer.fc_id() = (*it).fc_id();
        fc_writer.total_impressions() = (*it).total_impressions();
        std::copy((*it).last_impressions().begin(),
          (*it).last_impressions().end(),
          std::back_inserter(fc_writer.last_impressions()));
        profile_writer.virtual_freq_caps().push_back(fc_writer);
      }

      Generics::MemBuf mb(profile_writer.size());
      profile_writer.save(mb.data(), mb.size());
      membuf.assign(mb.data(), mb.size());
    }
  }

  namespace UserInfoSvcs
  {
    Generics::ConstSmartMemBuf_var
    UserFreqCapProfileAdapter::operator()(
      const Generics::ConstSmartMemBuf* mem_buf)
      /*throw(eh::Exception)*/
    {
      static const char* FUN = "UserFreqCapProfileAdapter::operator()";

      unsigned long version_head_size = UserFreqCapProfileVersionReader::FIXED_SIZE;
      if(mem_buf->membuf().size() < version_head_size)
      {
        Stream::Error ostr;
        ostr << FUN << ": Corrupt header, profile size = " << mem_buf->membuf().size();
        throw Exception(ostr);
      }

      UserFreqCapProfileVersionReader version_reader(
        mem_buf->membuf().data(),
        version_head_size);

      unsigned long current_version = version_reader.version();

      if(current_version != CURRENT_FREQ_CAP_PROFILE_VERSION)
      {
        Generics::SmartMemBuf_var result_mem_buf = Algs::copy_membuf(mem_buf);
        Generics::MemBuf& membuf = result_mem_buf->membuf();

        if (current_version == 291)
        {
          UserInfoSvcs_v291::convert_to_v340(membuf);
          current_version = 340;
        }

        if (current_version == 340)
        {
          UserInfoSvcs_v340::convert_to_v351(membuf);
          current_version = CURRENT_FREQ_CAP_PROFILE_VERSION;
        }

        if(current_version != CURRENT_FREQ_CAP_PROFILE_VERSION)
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
