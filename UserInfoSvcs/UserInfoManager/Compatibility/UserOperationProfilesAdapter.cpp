#include <Commons/Algs.hpp>

#include <UserInfoSvcs/UserInfoCommons/UserOperationProfiles.hpp>
#include <UserInfoSvcs/UserInfoManager/Compatibility/UserOperationProfiles_v24.hpp>
#include <UserInfoSvcs/UserInfoManager/Compatibility/UserOperationProfiles_v25.hpp>
#include <UserInfoSvcs/UserInfoManager/Compatibility/UserOperationProfiles_v26.hpp>
#include <UserInfoSvcs/UserInfoManager/Compatibility/UserOperationProfiles_v290.hpp>
#include <UserInfoSvcs/UserInfoManager/Compatibility/UserOperationProfiles_v300.hpp>
#include <UserInfoSvcs/UserInfoManager/Compatibility/UserOperationProfiles_v301.hpp>
#include <UserInfoSvcs/UserInfoManager/Compatibility/UserOperationProfiles_v310.hpp>
#include <UserInfoSvcs/UserInfoManager/Compatibility/UserOperationProfiles_v320.hpp>
#include <UserInfoSvcs/UserInfoManager/Compatibility/UserOperationProfiles_v340.hpp>

#include "UserOperationProfilesAdapter.hpp"

namespace AdServer
{
  namespace UserInfoSvcs
  {
    Generics::SmartMemBuf_var
    UserFreqCapUpdateOperationProfilesAdapter::operator()(
      Generics::SmartMemBuf* smart_mem_buf)
      /*throw(Exception)*/
    {
      static const char* FUN =
        "UserFreqCapUpdateOperationProfilesAdapter::adapt_match_operation_profile()";

      const unsigned long version_head_size = UserOperationTypeReader::FIXED_SIZE;
      Generics::MemBuf& membuf = smart_mem_buf->membuf();

      if(membuf.size() < version_head_size)
      {
        Stream::Error ostr;
        ostr << FUN << "Corrupt header: size of profile = " << membuf.size();
        throw Exception(ostr);
      }

      UserOperationTypeReader version_reader(membuf.data(), version_head_size);
      uint32_t current_version = version_reader.version();

      if (version_reader.version() == 1)
      {
        AdServer::UserInfoSvcs_v300::
          UserFreqCapUpdateOperationReader old_reader(
            membuf.data(), membuf.size());

        AdServer::UserInfoSvcs_v310::UserFreqCapUpdateOperationWriter
          ufc_operation_writer;

        ufc_operation_writer.operation_type() = old_reader.operation_type();
        current_version = 310;
        ufc_operation_writer.version() = current_version;

        ufc_operation_writer.user_id() = old_reader.user_id();
        ufc_operation_writer.time() = old_reader.time();
        ufc_operation_writer.request_id() = old_reader.request_id();

        std::copy(
          old_reader.freq_caps().begin(), old_reader.freq_caps().end(),
          std::back_inserter(ufc_operation_writer.freq_caps()));
        std::copy(
          old_reader.uc_freq_caps().begin(), old_reader.uc_freq_caps().end(),
          std::back_inserter(ufc_operation_writer.uc_freq_caps()));
        std::copy(
          old_reader.virtual_freq_caps().begin(),
          old_reader.virtual_freq_caps().end(),
          std::back_inserter(ufc_operation_writer.virtual_freq_caps()));

        membuf.alloc(ufc_operation_writer.size());
        ufc_operation_writer.save(membuf.data(), membuf.size());
      }

      if (current_version == 310)
      {
        const AdServer::UserInfoSvcs_v310::
          UserFreqCapUpdateOperationReader old_reader(
            membuf.data(), membuf.size());

        UserFreqCapUpdateOperationWriter ufc_operation_writer;

        ufc_operation_writer.operation_type() = old_reader.operation_type();
        current_version = FC_UPDATE_OPERATION_PROFILE_VERSION;
        ufc_operation_writer.version() = current_version;

        ufc_operation_writer.user_id() = old_reader.user_id();
        ufc_operation_writer.time() = old_reader.time();
        ufc_operation_writer.request_id() = old_reader.request_id();

        std::copy(
          old_reader.freq_caps().begin(), old_reader.freq_caps().end(),
          std::back_inserter(ufc_operation_writer.freq_caps()));
        std::copy(
          old_reader.uc_freq_caps().begin(), old_reader.uc_freq_caps().end(),
          std::back_inserter(ufc_operation_writer.uc_freq_caps()));
        std::copy(
          old_reader.virtual_freq_caps().begin(),
          old_reader.virtual_freq_caps().end(),
          std::back_inserter(ufc_operation_writer.virtual_freq_caps()));

        for (auto it = old_reader.seq_orders().begin();
             it != old_reader.seq_orders().end(); ++it)
        {
          const AdServer::UserInfoSvcs_v310::SeqOrderDescriptorReader&
            seq_order_reader = *it;
          SeqOrderDescriptorWriter seq_order;
          seq_order.ccg_id() = seq_order_reader.ccg_id();
          seq_order.set_id() = seq_order_reader.set_id();
          seq_order.imps() = seq_order_reader.imps();
          ufc_operation_writer.seq_orders().push_back(seq_order);
        }

        membuf.alloc(ufc_operation_writer.size());
        ufc_operation_writer.save(membuf.data(), membuf.size());
      }

      if (current_version != FC_UPDATE_OPERATION_PROFILE_VERSION)
      {
        Stream::Error ostr;
        ostr << FUN << "Unknown profile version: " << version_reader.version();
        throw Exception(ostr);
      }

      return Generics::SmartMemBuf_var(
        ReferenceCounting::add_ref(smart_mem_buf));
    }

    Generics::SmartMemBuf_var
    UserFreqCapConfirmOperationProfilesAdapter::operator()(
      Generics::SmartMemBuf* smart_mem_buf)
      /*throw(Exception)*/
    {
      static const char* FUN =
        "UserFreqCapConfirmOperationProfilesAdapter::adapt_match_operation_profile()";

      unsigned long version_head_size = UserOperationTypeReader::FIXED_SIZE;
      Generics::MemBuf& membuf = smart_mem_buf->membuf();
      
      if(membuf.size() < version_head_size)
      {
        Stream::Error ostr;
        ostr << FUN << "Corrupt header: size of profile = " << membuf.size();
        throw Exception(ostr);
      }
      
      UserOperationTypeReader version_reader(membuf.data(), version_head_size);

      if(version_reader.version() != FC_CONFIRM_OPERATION_PROFILE_VERSION)
      {
        UserFreqCapConfirmOperationWriter ufc_operation_writer;
        
        if (version_reader.version() == 1)
        {
          AdServer::UserInfoSvcs_v340::
            UserFreqCapConfirmOperationReader old_reader(
              membuf.data(), membuf.size());

          ufc_operation_writer.operation_type() = old_reader.operation_type();
          ufc_operation_writer.version() = FC_CONFIRM_OPERATION_PROFILE_VERSION;

          ufc_operation_writer.user_id() = old_reader.user_id();
          ufc_operation_writer.time() = old_reader.time();
          ufc_operation_writer.request_id() = old_reader.request_id();          
        }
        else
        {
          Stream::Error ostr;
          ostr << FUN << "Unknown profile version: " << version_reader.version();
          throw Exception(ostr);
        }
        
        Generics::MemBuf mb(ufc_operation_writer.size());
        ufc_operation_writer.save(mb.data(), mb.size());
        membuf.assign(mb.data(), mb.size());
      }

      return Generics::SmartMemBuf_var(
        ReferenceCounting::add_ref(smart_mem_buf));
    }

    template<typename InputIterator, typename OutputContainerType>
    void UserMatchOperationProfilesAdapter::fill_channel_trigger_seq(
      InputIterator first,
      InputIterator last,
      OutputContainerType& out)
    {
      ChannelTriggerMatchWriter writer;
      
      while (first != last)
      {
        writer.channel_id() = *first;
        writer.channel_trigger_id() = 0;
        out.push_back(writer);
        
        ++first;
      }
    }
    
    Generics::SmartMemBuf_var
    UserMatchOperationProfilesAdapter::operator()(
      Generics::SmartMemBuf* smart_mem_buf)
      /*throw(Exception)*/
    {
      static const char* FUN =
        "UserMatchOperationProfilesAdapter::adapt_match_operation_profile()";

      unsigned long version_head_size = UserOperationTypeReader::FIXED_SIZE;

      Generics::MemBuf& membuf = smart_mem_buf->membuf();

      if(membuf.size() < version_head_size)
      {
        Stream::Error ostr;
        ostr << FUN << "Corrupt header: size of profile = " << membuf.size();
        throw Exception(ostr);
      }

      UserOperationTypeReader version_reader(
        membuf.data(), version_head_size);

      if(version_reader.version() != MATCH_OPERATION_PROFILE_VERSION)
      {
        UserMatchOperationWriter match_operation_writer;

        if (version_reader.version() == 1)
        {
          AdServer::UserInfoSvcs_v24::UserMatchOperationReader old_reader(
            membuf.data(), membuf.size());

          match_operation_writer.operation_type() = old_reader.operation_type();
          match_operation_writer.version() = MATCH_OPERATION_PROFILE_VERSION;

          match_operation_writer.user_id() = old_reader.user_id();
          match_operation_writer.temporary() = old_reader.temporary();
          match_operation_writer.time() = old_reader.time();
          match_operation_writer.request_colo_id() = old_reader.request_colo_id();
          match_operation_writer.last_colo_id() = old_reader.last_colo_id();
          match_operation_writer.placement_colo_id() = old_reader.placement_colo_id();
          match_operation_writer.change_last_request() =
            ((old_reader.ad_request() & 0x80) == 0);
          match_operation_writer.household() = 0;
          match_operation_writer.repeat_trigger_timeout() = 0;
          match_operation_writer.filter_contextual_triggers() = 1;

          fill_channel_trigger_seq(
            old_reader.page_channels().begin(), old_reader.page_channels().end(),
            match_operation_writer.page_channels());

          fill_channel_trigger_seq(
            old_reader.search_channels().begin(), old_reader.search_channels().end(),
            match_operation_writer.search_channels());

          fill_channel_trigger_seq(
            old_reader.url_channels().begin(), old_reader.url_channels().end(),
            match_operation_writer.url_channels());
        }
        else if (version_reader.version() == 2)
        {
          AdServer::UserInfoSvcs_v25::UserMatchOperationReader old_reader(
            membuf.data(), membuf.size());

          match_operation_writer.operation_type() = old_reader.operation_type();
          match_operation_writer.version() = MATCH_OPERATION_PROFILE_VERSION;

          match_operation_writer.user_id() = old_reader.user_id();
          match_operation_writer.temporary() = old_reader.temporary();
          match_operation_writer.time() = old_reader.time();
          match_operation_writer.request_colo_id() = old_reader.request_colo_id();
          match_operation_writer.last_colo_id() = old_reader.last_colo_id();
          match_operation_writer.placement_colo_id() = old_reader.placement_colo_id();
          match_operation_writer.change_last_request() = old_reader.change_last_request();
          match_operation_writer.household() = 0;
          match_operation_writer.repeat_trigger_timeout() = 0;
          match_operation_writer.filter_contextual_triggers() = 1;

          std::copy(
            old_reader.persistent_channels().begin(),
            old_reader.persistent_channels().end(),
            std::back_inserter(match_operation_writer.persistent_channels()));

          fill_channel_trigger_seq(
            old_reader.page_channels().begin(), old_reader.page_channels().end(),
            match_operation_writer.page_channels());

          fill_channel_trigger_seq(
            old_reader.search_channels().begin(), old_reader.search_channels().end(),
            match_operation_writer.search_channels());

          fill_channel_trigger_seq(
            old_reader.url_channels().begin(), old_reader.url_channels().end(),
            match_operation_writer.url_channels());
        }
        else if (version_reader.version() == 3)
        {
          AdServer::UserInfoSvcs_v26::UserMatchOperationReader old_reader(
            membuf.data(), membuf.size());

          match_operation_writer.operation_type() = old_reader.operation_type();
          match_operation_writer.version() = MATCH_OPERATION_PROFILE_VERSION;

          match_operation_writer.user_id() = old_reader.user_id();
          match_operation_writer.temporary() = old_reader.temporary();
          match_operation_writer.time() = old_reader.time();
          match_operation_writer.request_colo_id() = old_reader.request_colo_id();
          match_operation_writer.last_colo_id() = old_reader.last_colo_id();
          match_operation_writer.placement_colo_id() = old_reader.placement_colo_id();
          match_operation_writer.change_last_request() = old_reader.change_last_request();
          match_operation_writer.household() = 0;
          match_operation_writer.repeat_trigger_timeout() = 0;
          match_operation_writer.filter_contextual_triggers() = 1;
          
          std::copy(
            old_reader.persistent_channels().begin(),
            old_reader.persistent_channels().end(),
            std::back_inserter(match_operation_writer.persistent_channels()));

          fill_channel_trigger_seq(
            old_reader.page_channels().begin(), old_reader.page_channels().end(),
            match_operation_writer.page_channels());

          fill_channel_trigger_seq(
            old_reader.search_channels().begin(), old_reader.search_channels().end(),
            match_operation_writer.search_channels());

          fill_channel_trigger_seq(
            old_reader.url_channels().begin(), old_reader.url_channels().end(),
            match_operation_writer.url_channels());
        }
        else if (version_reader.version() == 4)
        {
          AdServer::UserInfoSvcs_v300::UserMatchOperationReader old_reader(
            membuf.data(), membuf.size());

          match_operation_writer.operation_type() = old_reader.operation_type();
          match_operation_writer.version() = MATCH_OPERATION_PROFILE_VERSION;

          match_operation_writer.user_id() = old_reader.user_id();
          match_operation_writer.temporary() = old_reader.temporary();
          match_operation_writer.time() = old_reader.time();
          match_operation_writer.request_colo_id() = old_reader.request_colo_id();
          match_operation_writer.last_colo_id() = old_reader.last_colo_id();
          match_operation_writer.placement_colo_id() = old_reader.placement_colo_id();
          match_operation_writer.change_last_request() = old_reader.change_last_request();
          match_operation_writer.household() = old_reader.household();
          match_operation_writer.repeat_trigger_timeout() = 0;
          match_operation_writer.filter_contextual_triggers() = 1;

          std::copy(
            old_reader.persistent_channels().begin(),
            old_reader.persistent_channels().end(),
            std::back_inserter(match_operation_writer.persistent_channels()));

          fill_channel_trigger_seq(
            old_reader.page_channels().begin(), old_reader.page_channels().end(),
            match_operation_writer.page_channels());

          fill_channel_trigger_seq(
            old_reader.search_channels().begin(), old_reader.search_channels().end(),
            match_operation_writer.search_channels());

          fill_channel_trigger_seq(
            old_reader.url_channels().begin(), old_reader.url_channels().end(),
            match_operation_writer.url_channels());
        }
        else if (version_reader.version() == 301)
        {
          AdServer::UserInfoSvcs_v301::UserMatchOperationReader old_reader(
            membuf.data(), membuf.size());

          match_operation_writer.operation_type() = old_reader.operation_type();
          match_operation_writer.version() = MATCH_OPERATION_PROFILE_VERSION;

          match_operation_writer.user_id() = old_reader.user_id();
          match_operation_writer.temporary() = old_reader.temporary();
          match_operation_writer.time() = old_reader.time();
          match_operation_writer.request_colo_id() = old_reader.request_colo_id();
          match_operation_writer.last_colo_id() = old_reader.last_colo_id();
          match_operation_writer.placement_colo_id() = old_reader.placement_colo_id();
          match_operation_writer.change_last_request() = old_reader.change_last_request();
          match_operation_writer.household() = old_reader.household();
          match_operation_writer.cohort() = old_reader.cohort();
          match_operation_writer.repeat_trigger_timeout() = 0;
          match_operation_writer.filter_contextual_triggers() = 1;

          std::copy(
            old_reader.persistent_channels().begin(),
            old_reader.persistent_channels().end(),
            std::back_inserter(match_operation_writer.persistent_channels()));

          fill_channel_trigger_seq(
            old_reader.page_channels().begin(), old_reader.page_channels().end(),
            match_operation_writer.page_channels());

          fill_channel_trigger_seq(
            old_reader.search_channels().begin(), old_reader.search_channels().end(),
            match_operation_writer.search_channels());

          fill_channel_trigger_seq(
            old_reader.url_channels().begin(), old_reader.url_channels().end(),
            match_operation_writer.url_channels());
        }
        else if (version_reader.version() == 320)
        {
          AdServer::UserInfoSvcs_v320::UserMatchOperationReader old_reader(
            membuf.data(), membuf.size());

          match_operation_writer.operation_type() = old_reader.operation_type();
          match_operation_writer.version() = MATCH_OPERATION_PROFILE_VERSION;

          match_operation_writer.user_id() = old_reader.user_id();
          match_operation_writer.temporary() = old_reader.temporary();
          match_operation_writer.time() = old_reader.time();
          match_operation_writer.request_colo_id() = old_reader.request_colo_id();
          match_operation_writer.last_colo_id() = old_reader.last_colo_id();
          match_operation_writer.placement_colo_id() = old_reader.placement_colo_id();
          match_operation_writer.change_last_request() = old_reader.change_last_request();
          match_operation_writer.household() = old_reader.household();
          match_operation_writer.cohort() = old_reader.cohort();
          match_operation_writer.repeat_trigger_timeout() =
            old_reader.repeat_trigger_timeout();
          match_operation_writer.filter_contextual_triggers() =
            old_reader.filter_contextual_triggers();
          
          std::copy(
            old_reader.persistent_channels().begin(),
            old_reader.persistent_channels().end(),
            std::back_inserter(match_operation_writer.persistent_channels()));

          std::copy(
            old_reader.page_channels().begin(),
            old_reader.page_channels().end(),
            std::back_inserter(match_operation_writer.page_channels()));

          std::copy(
            old_reader.search_channels().begin(),
            old_reader.search_channels().end(),
            std::back_inserter(match_operation_writer.search_channels()));

          std::copy(
            old_reader.url_channels().begin(),
            old_reader.url_channels().end(),
            std::back_inserter(match_operation_writer.url_channels()));

          std::copy(
            old_reader.url_keyword_channels().begin(),
            old_reader.url_keyword_channels().end(),
            std::back_inserter(match_operation_writer.url_keyword_channels()));
        } 
        else
        {
          Stream::Error ostr;
          ostr << FUN << "Unknown profile version: " << version_reader.version();
          throw Exception(ostr);
        }

        Generics::MemBuf mb(match_operation_writer.size());
        match_operation_writer.save(mb.data(), mb.size());
        membuf.assign(mb.data(), mb.size());
      }

      return Generics::SmartMemBuf_var(
        ReferenceCounting::add_ref(smart_mem_buf));
    }

    Generics::SmartMemBuf_var
    UserMergeOperationProfilesAdapter::operator()(
      Generics::SmartMemBuf* smart_mem_buf)
      /*throw(Exception)*/
    {
      static const char* FUN =
        "UserMergeOperationProfilesAdapter::adapt_merge_operation_profile()";

      unsigned long version_head_size = UserOperationTypeReader::FIXED_SIZE;

      Generics::MemBuf& membuf = smart_mem_buf->membuf();

      if(membuf.size() < version_head_size)
      {
        Stream::Error ostr;
        ostr << FUN << "Corrupt header: size of profile = " << membuf.size();
        throw Exception(ostr);
      }

      UserOperationTypeReader version_reader(
        membuf.data(), version_head_size);

      if(version_reader.version() != MERGE_OPERATION_PROFILE_VERSION)
      {
        UserMergeOperationWriter merge_operation_writer;

        if (version_reader.version() == 1)
        {
          AdServer::UserInfoSvcs_v24::UserMergeOperationReader old_reader(
            membuf.data(), membuf.size());

          merge_operation_writer.operation_type() = old_reader.operation_type();
          merge_operation_writer.version() = MERGE_OPERATION_PROFILE_VERSION;

          merge_operation_writer.user_id() = old_reader.user_id();
          merge_operation_writer.time() = old_reader.time();
          merge_operation_writer.exchange_merge() =
            old_reader.exchange_merge() & 0x7FFFFFFF;
          merge_operation_writer.change_last_request() =
            ((old_reader.exchange_merge() & 0x80000000) == 0);

          merge_operation_writer.household() = 0;
          merge_operation_writer.request_colo_id() = 0;
          
          merge_operation_writer.merge_base_profile().assign(
            old_reader.merge_base_profile().get(),
            old_reader.merge_base_profile().size());

          merge_operation_writer.merge_add_profile().assign(
            old_reader.merge_add_profile().get(),
            old_reader.merge_add_profile().size());

          merge_operation_writer.merge_history_profile().assign(
            old_reader.merge_history_profile().get(),
            old_reader.merge_history_profile().size());       
        }
        else if (version_reader.version() == 2)
        {
          AdServer::UserInfoSvcs_v26::UserMergeOperationReader old_reader(
            membuf.data(), membuf.size());

          merge_operation_writer.operation_type() = old_reader.operation_type();
          merge_operation_writer.version() = MERGE_OPERATION_PROFILE_VERSION;

          merge_operation_writer.user_id() = old_reader.user_id();
          merge_operation_writer.time() = old_reader.time();
          merge_operation_writer.exchange_merge() =
            old_reader.exchange_merge() & 0x7FFFFFFF;
          merge_operation_writer.change_last_request() =
            ((old_reader.exchange_merge() & 0x80000000) == 0);
          merge_operation_writer.household() = 0;
          merge_operation_writer.request_colo_id() = 0;
          
          merge_operation_writer.merge_base_profile().assign(
            old_reader.merge_base_profile().get(),
            old_reader.merge_base_profile().size());

          merge_operation_writer.merge_add_profile().assign(
            old_reader.merge_add_profile().get(),
            old_reader.merge_add_profile().size());

          merge_operation_writer.merge_history_profile().assign(
            old_reader.merge_history_profile().get(),
            old_reader.merge_history_profile().size());
        }
        else if (version_reader.version() == 3)
        {
          AdServer::UserInfoSvcs_v290::UserMergeOperationReader old_reader(
            membuf.data(), membuf.size());

          merge_operation_writer.operation_type() = old_reader.operation_type();
          merge_operation_writer.version() = MERGE_OPERATION_PROFILE_VERSION;

          merge_operation_writer.user_id() = old_reader.user_id();
          merge_operation_writer.time() = old_reader.time();
          merge_operation_writer.exchange_merge() = old_reader.exchange_merge();
          merge_operation_writer.change_last_request() = old_reader.change_last_request();
          merge_operation_writer.household() = old_reader.household();
          merge_operation_writer.request_colo_id() = 0;
          
          merge_operation_writer.merge_base_profile().assign(
            old_reader.merge_base_profile().get(),
            old_reader.merge_base_profile().size());

          merge_operation_writer.merge_add_profile().assign(
            old_reader.merge_add_profile().get(),
            old_reader.merge_add_profile().size());

          merge_operation_writer.merge_history_profile().assign(
            old_reader.merge_history_profile().get(),
            old_reader.merge_history_profile().size());
        }
        else if (version_reader.version() == 300)
        {
          AdServer::UserInfoSvcs_v340::UserMergeOperationReader old_reader(
            membuf.data(), membuf.size());

          merge_operation_writer.operation_type() = old_reader.operation_type();
          merge_operation_writer.version() = MERGE_OPERATION_PROFILE_VERSION;

          merge_operation_writer.user_id() = old_reader.user_id();
          merge_operation_writer.time() = old_reader.time();
          merge_operation_writer.exchange_merge() = old_reader.exchange_merge();
          merge_operation_writer.change_last_request() = old_reader.change_last_request();
          merge_operation_writer.household() = old_reader.household();
          merge_operation_writer.request_colo_id() = old_reader.request_colo_id();
          
          merge_operation_writer.merge_base_profile().assign(
            old_reader.merge_base_profile().get(),
            old_reader.merge_base_profile().size());

          merge_operation_writer.merge_add_profile().assign(
            old_reader.merge_add_profile().get(),
            old_reader.merge_add_profile().size());

          merge_operation_writer.merge_history_profile().assign(
            old_reader.merge_history_profile().get(),
            old_reader.merge_history_profile().size());
        }
        else
        {
          Stream::Error ostr;
          ostr << FUN << "Unknown profile version: " << version_reader.version();
          throw Exception(ostr);
        }

        Generics::MemBuf mb(merge_operation_writer.size());
        merge_operation_writer.save(mb.data(), mb.size());
        membuf.assign(mb.data(), mb.size());
      }

      return Generics::SmartMemBuf_var(
        ReferenceCounting::add_ref(smart_mem_buf));
    }
  }
}
