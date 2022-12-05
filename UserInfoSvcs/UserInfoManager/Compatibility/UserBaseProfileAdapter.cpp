#include <iomanip>

#include <Commons/Algs.hpp>

#include <UserInfoSvcs/UserInfoCommons/ChannelMatcher.hpp>

#include <UserInfoSvcs/UserInfoManager/Compatibility/UserChannelBaseProfile_v1.hpp>
#include <UserInfoSvcs/UserInfoManager/Compatibility/UserChannelBaseProfile_v2.hpp>
#include <UserInfoSvcs/UserInfoManager/Compatibility/UserChannelBaseProfile_v3.hpp>
#include <UserInfoSvcs/UserInfoManager/Compatibility/UserChannelBaseProfile_v4.hpp>
#include <UserInfoSvcs/UserInfoManager/Compatibility/UserChannelBaseProfile_v5.hpp>
#include <UserInfoSvcs/UserInfoManager/Compatibility/UserChannelBaseProfile_v26.hpp>
#include <UserInfoSvcs/UserInfoManager/Compatibility/UserChannelBaseProfile_v290.hpp>
#include <UserInfoSvcs/UserInfoManager/Compatibility/UserChannelBaseProfile_v300.hpp>
#include <UserInfoSvcs/UserInfoManager/Compatibility/UserChannelBaseProfile_v301.hpp>
#include <UserInfoSvcs/UserInfoManager/Compatibility/UserChannelBaseProfile_v320.hpp>

#include "UserBaseProfileAdapter.hpp"

namespace AdServer
{
  namespace UserInfoSvcs
  {
    namespace
    {
      template <typename WriterType, typename ReaderType>
      static void copy_base_section(
        WriterType& ciw,
        const ReaderType& cir) noexcept
      {
        std::copy(
          cir.ht_candidates().begin(),
          cir.ht_candidates().end(),
          Algs::modify_inserter(
            std::back_inserter(ciw.ht_candidates()),
            Algs::MemoryInitAdapter<
              typename WriterType::ht_candidates_Container::value_type>()));

        std::copy(
          cir.history_matches().begin(),
          cir.history_matches().end(),
          Algs::modify_inserter(
            std::back_inserter(ciw.history_matches()),
            Algs::MemoryInitAdapter<
              typename WriterType::history_matches_Container::value_type>()));

        std::copy(
          cir.history_visits().begin(),
          cir.history_visits().end(),
          Algs::modify_inserter(
            std::back_inserter(ciw.history_visits()),
            Algs::MemoryInitAdapter<
              typename WriterType::history_visits_Container::value_type>()));

        std::copy(
          cir.session_matches().begin(),
          cir.session_matches().end(),
          Algs::modify_inserter(
            std::back_inserter(ciw.session_matches()),
            Algs::MemoryInitAdapter<
              typename WriterType::session_matches_Container::value_type>()));
      }

      template <typename WriterType, typename ReaderType>
      static void copy_section(
        WriterType& ciw,
        const ReaderType& cir) noexcept
      {
        copy_base_section(ciw, cir);

        /*
        std::copy(
          cir.hpm().begin(),
          cir.hpm().end(),
          Algs::modify_inserter(
            std::back_inserter(ciw.hpm()),
            Algs::MemoryInitAdapter<
              typename WriterType::hpm_Container::value_type>()));

          std::copy(
          cir.hpm_from_now().begin(),
          cir.hpm_from_now().end(),
          Algs::modify_inserter(
            std::back_inserter(ciw.hpm_from_now()),
            Algs::MemoryInitAdapter<
              typename WriterType::hpm_from_now_Container::value_type>()));
        */
      }
    }
  }
}

namespace AdServer
{
  namespace UserInfoSvcs
  {
    Generics::ConstSmartMemBuf_var
    BaseProfileAdapter::operator()(
      const Generics::ConstSmartMemBuf* mem_buf,
      bool ignore_future_versions)
      /*throw(Exception)*/
    {
      static const char* FUN = "BaseProfileAdapter::operator()";

      unsigned long version_head_size = ChannelsProfileVersionReader::FIXED_SIZE;
      if(mem_buf->membuf().size() < version_head_size)
      {
        throw Exception("Corrupt header");
      }

      ChannelsProfileVersionReader version_reader(
        mem_buf->membuf().data(),
        version_head_size);

      unsigned long current_version = version_reader.version();

      if(current_version > CURRENT_BASE_PROFILE_VERSION &&
         ignore_future_versions)
      {
        return Generics::ConstSmartMemBuf_var();
      }

      if(current_version != CURRENT_BASE_PROFILE_VERSION)
      {
        Generics::SmartMemBuf_var result_mem_buf = Algs::copy_membuf(mem_buf);
        Generics::MemBuf& membuf = result_mem_buf->membuf();

        UserInfoSvcs_v26::ChannelsProfileWriter profile_writer;
        profile_writer.version() = 6;

        if(current_version == 1)
        {
          AdServer::UserInfoSvcs_v1::ChannelsProfileReader profile_reader(
            membuf.data(), membuf.size());
          profile_writer.last_request_time() = profile_reader.last_request();
          profile_writer.create_time() = profile_reader.last_request();
          profile_writer.history_time() = profile_reader.last_request();
          profile_writer.ignore_fraud_time() = 0;
          profile_writer.session_start() = 0;

          copy_base_section(
            profile_writer.search_channels(),
            profile_reader.search_channels());

          copy_base_section(
            profile_writer.page_channels(),
            profile_reader.page_channels());

          copy_base_section(
            profile_writer.url_channels(),
            profile_reader.url_channels());

          current_version = 6;

          Generics::MemBuf mb(profile_writer.size());
          profile_writer.save(mb.data(), mb.size());
          membuf.assign(mb.data(), mb.size());
        }
        else if (version_reader.version() == 2)
        {
          AdServer::UserInfoSvcs_v2::ChannelsProfileReader profile_reader(
            membuf.data(), membuf.size());
          profile_writer.last_request_time() = profile_reader.last_request_time();
          profile_writer.create_time() = profile_reader.create_time();
          profile_writer.history_time() = profile_reader.history_time();
          profile_writer.ignore_fraud_time() = profile_reader.ignore_fraud_time();
          profile_writer.session_start() = 0;

          copy_base_section(
            profile_writer.search_channels(),
            profile_reader.search_channels());

          copy_base_section(
            profile_writer.page_channels(),
            profile_reader.page_channels());

          copy_base_section(
            profile_writer.url_channels(),
            profile_reader.url_channels());

          current_version = 6;

          Generics::MemBuf mb(profile_writer.size());
          profile_writer.save(mb.data(), mb.size());
          membuf.assign(mb.data(), mb.size());
        }
        else if (version_reader.version() == 3)
        {
          AdServer::UserInfoSvcs_v3::ChannelsProfileReader profile_reader(
            membuf.data(), membuf.size());
          profile_writer.last_request_time() = profile_reader.last_request_time();
          profile_writer.create_time() = profile_reader.create_time();
          profile_writer.history_time() = profile_reader.history_time();
          profile_writer.ignore_fraud_time() = profile_reader.ignore_fraud_time();
          profile_writer.session_start() = profile_reader.session_start();

          copy_base_section(
            profile_writer.search_channels(),
            profile_reader.search_channels());

          copy_base_section(
            profile_writer.page_channels(),
            profile_reader.page_channels());

          copy_base_section(
            profile_writer.url_channels(),
            profile_reader.url_channels());

          current_version = 6;

          Generics::MemBuf mb(profile_writer.size());
          profile_writer.save(mb.data(), mb.size());
          membuf.assign(mb.data(), mb.size());
        }
        else if (version_reader.version() == 4)
        {
          AdServer::UserInfoSvcs_v4::ChannelsProfileReader profile_reader(
            membuf.data(), membuf.size());
          profile_writer.last_request_time() = profile_reader.last_request_time();
          profile_writer.create_time() = profile_reader.create_time();
          profile_writer.history_time() = profile_reader.history_time();
          profile_writer.ignore_fraud_time() = profile_reader.ignore_fraud_time();
          profile_writer.session_start() = profile_reader.session_start();

          copy_section(
            profile_writer.search_channels(),
            profile_reader.search_channels());

          copy_section(
            profile_writer.page_channels(),
            profile_reader.page_channels());

          copy_section(
            profile_writer.url_channels(),
            profile_reader.url_channels());

          current_version = 6;

          Generics::MemBuf mb(profile_writer.size());
          profile_writer.save(mb.data(), mb.size());
          membuf.assign(mb.data(), mb.size());
        }
        else if (version_reader.version() == 5)
        {
          AdServer::UserInfoSvcs_v5::ChannelsProfileReader profile_reader(
            membuf.data(), membuf.size());
          profile_writer.last_request_time() = profile_reader.last_request_time();
          profile_writer.create_time() = profile_reader.create_time();
          profile_writer.history_time() = profile_reader.history_time();
          profile_writer.ignore_fraud_time() = profile_reader.ignore_fraud_time();
          profile_writer.session_start() = profile_reader.session_start();

          copy_section(
            profile_writer.search_channels(),
            profile_reader.search_channels());

          copy_section(
            profile_writer.page_channels(),
            profile_reader.page_channels());

          copy_section(
            profile_writer.url_channels(),
            profile_reader.url_channels());

          current_version = 6;

          Generics::MemBuf mb(profile_writer.size());
          profile_writer.save(mb.data(), mb.size());
          membuf.assign(mb.data(), mb.size());
        }

        if (current_version == 6)
        {
          current_version = 7;
          
          UserInfoSvcs_v290::ChannelsProfileWriter writer;
          writer.version() = CURRENT_BASE_PROFILE_VERSION;

          AdServer::UserInfoSvcs_v26::ChannelsProfileReader profile_reader(
            membuf.data(), membuf.size());
          writer.last_request_time() = profile_reader.last_request_time();
          writer.create_time() = profile_reader.create_time();
          writer.history_time() = profile_reader.history_time();
          writer.ignore_fraud_time() = profile_reader.ignore_fraud_time();
          writer.session_start() = profile_reader.session_start();
          writer.household() = 0;

          copy_section(
            writer.search_channels(),
            profile_reader.search_channels());

          copy_section(
            writer.page_channels(),
            profile_reader.page_channels());

          copy_section(
            writer.url_channels(),
            profile_reader.url_channels());

          std::copy(
            profile_reader.persistent_matches().channel_ids().begin(),
            profile_reader.persistent_matches().channel_ids().end(),
            std::back_inserter(writer.persistent_matches().channel_ids()));

          Generics::MemBuf mb(writer.size());
          writer.save(mb.data(), mb.size());
          membuf.assign(mb.data(), mb.size());
        }

        if (current_version == 7)
        {
          current_version = 300;
          
          UserInfoSvcs_v300::ChannelsProfileWriter writer;

          AdServer::UserInfoSvcs_v290::ChannelsProfileReader profile_reader(
            membuf.data(), membuf.size());
          writer.last_request_time() = profile_reader.last_request_time();
          writer.create_time() = profile_reader.create_time();
          writer.history_time() = profile_reader.history_time();
          writer.ignore_fraud_time() = profile_reader.ignore_fraud_time();
          writer.session_start() = profile_reader.session_start();
          writer.household() = profile_reader.household();
          writer.first_colo_id() = UNKNOWN_COLO_ID;
          writer.last_colo_id() = UNKNOWN_COLO_ID;

          copy_section(
            writer.search_channels(),
            profile_reader.search_channels());

          copy_section(
            writer.page_channels(),
            profile_reader.page_channels());

          copy_section(
            writer.url_channels(),
            profile_reader.url_channels());

          std::copy(
            profile_reader.persistent_matches().channel_ids().begin(),
            profile_reader.persistent_matches().channel_ids().end(),
            std::back_inserter(writer.persistent_matches().channel_ids()));

          Generics::MemBuf mb(writer.size());
          writer.save(mb.data(), mb.size());
          membuf.assign(mb.data(), mb.size());
        }

        if (current_version == 300)
        {
          UserInfoSvcs_v301::ChannelsProfileWriter writer;
          
          current_version = 301;
          
          AdServer::UserInfoSvcs_v300::ChannelsProfileReader profile_reader(
            membuf.data(), membuf.size());
          writer.last_request_time() = profile_reader.last_request_time();
          writer.create_time() = profile_reader.create_time();
          writer.history_time() = profile_reader.history_time();
          writer.ignore_fraud_time() = profile_reader.ignore_fraud_time();
          writer.session_start() = profile_reader.session_start();
          writer.household() = profile_reader.household();
          writer.first_colo_id() = profile_reader.first_colo_id();
          writer.last_colo_id() = profile_reader.last_colo_id();
          
          copy_section(
            writer.search_channels(),
            profile_reader.search_channels());

          copy_section(
            writer.page_channels(),
            profile_reader.page_channels());

          copy_section(
            writer.url_channels(),
            profile_reader.url_channels());

          std::copy(
            profile_reader.persistent_matches().channel_ids().begin(),
            profile_reader.persistent_matches().channel_ids().end(),
            std::back_inserter(writer.persistent_matches().channel_ids()));

          Generics::MemBuf mb(writer.size());
          writer.save(mb.data(), mb.size());
          membuf.assign(mb.data(), mb.size());
        }

        if (current_version == 301)
        {
          UserInfoSvcs_v320::ChannelsProfileWriter writer;
          
          current_version = 320;
          
          AdServer::UserInfoSvcs_v301::ChannelsProfileReader profile_reader(
            membuf.data(), membuf.size());
          writer.last_request_time() = profile_reader.last_request_time();
          writer.create_time() = profile_reader.create_time();
          writer.history_time() = profile_reader.history_time();
          writer.ignore_fraud_time() = profile_reader.ignore_fraud_time();
          writer.session_start() = profile_reader.session_start();
          writer.household() = profile_reader.household();
          writer.first_colo_id() = profile_reader.first_colo_id();
          writer.last_colo_id() = profile_reader.last_colo_id();
          writer.cohort() = profile_reader.cohort();
          
          copy_section(
            writer.search_channels(),
            profile_reader.search_channels());

          copy_section(
            writer.page_channels(),
            profile_reader.page_channels());

          copy_section(
            writer.url_channels(),
            profile_reader.url_channels());

          std::copy(
            profile_reader.persistent_matches().channel_ids().begin(),
            profile_reader.persistent_matches().channel_ids().end(),
            std::back_inserter(writer.persistent_matches().channel_ids()));

          Generics::MemBuf mb(writer.size());
          writer.save(mb.data(), mb.size());
          membuf.assign(mb.data(), mb.size());
        }

        if (current_version == 320)
        {
          ChannelsProfileWriter writer;
          
          current_version = CURRENT_BASE_PROFILE_VERSION;
          writer.version() = CURRENT_BASE_PROFILE_VERSION;

          AdServer::UserInfoSvcs_v320::ChannelsProfileReader profile_reader(
            membuf.data(), membuf.size());
          writer.last_request_time() = profile_reader.last_request_time();
          writer.create_time() = profile_reader.create_time();
          writer.history_time() = profile_reader.history_time();
          writer.ignore_fraud_time() = profile_reader.ignore_fraud_time();
          writer.session_start() = profile_reader.session_start();
          writer.household() = profile_reader.household();
          writer.first_colo_id() = profile_reader.first_colo_id();
          writer.last_colo_id() = profile_reader.last_colo_id();
          writer.cohort() = profile_reader.cohort();
          
          copy_section(
            writer.search_channels(),
            profile_reader.search_channels());

          copy_section(
            writer.page_channels(),
            profile_reader.page_channels());

          copy_section(
            writer.url_channels(),
            profile_reader.url_channels());

          copy_section(
            writer.url_keyword_channels(),
            profile_reader.url_keyword_channels());

          std::copy(
            profile_reader.persistent_matches().channel_ids().begin(),
            profile_reader.persistent_matches().channel_ids().end(),
            std::back_inserter(writer.persistent_matches().channel_ids()));

          std::copy(
            profile_reader.last_page_triggers().begin(),
            profile_reader.last_page_triggers().end(),
            std::back_inserter(writer.last_page_triggers()));

          std::copy(
            profile_reader.last_search_triggers().begin(),
            profile_reader.last_search_triggers().end(),
            std::back_inserter(writer.last_search_triggers()));

          std::copy(
            profile_reader.last_url_triggers().begin(),
            profile_reader.last_url_triggers().end(),
            std::back_inserter(writer.last_url_triggers()));

          std::copy(
            profile_reader.last_url_keyword_triggers().begin(),
            profile_reader.last_url_keyword_triggers().end(),
            std::back_inserter(writer.last_url_keyword_triggers()));

          Generics::MemBuf mb(writer.size());
          writer.save(mb.data(), mb.size());
          membuf.assign(mb.data(), mb.size());
        }
        
        if(current_version != CURRENT_BASE_PROFILE_VERSION)
        {
          Stream::Error ostr;
          ostr << FUN << ": incorrect version after adaptation = " << current_version <<
            "\n Profile size = " << membuf.size() <<
            ", the contents of the profile (36 bytes) : ";
          
          unsigned long len = std::min((size_t)36, membuf.size()) >> 2;
          uint32_t* ptr = static_cast<uint32_t*>(membuf.data());
          for (unsigned long i = 0; i < len; ++i)
          {
            if (i != 0)
            {
              ostr << ", ";
            }
            
            ostr << "0x" << std::hex << std::setfill('0') << std::setw(8)
                 << *(ptr + i);
          }
          
          throw Exception(ostr);
        }

        return Generics::transfer_membuf(result_mem_buf);
      }

      return ReferenceCounting::add_ref(mem_buf);
    }
  }
}
