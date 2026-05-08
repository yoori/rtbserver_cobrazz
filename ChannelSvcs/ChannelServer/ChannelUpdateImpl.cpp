#include <eh/Exception.hpp>
#include <ReferenceCounting/DefaultImpl.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <Logger/Logger.hpp>
#include <cstring>
#include <vector>
#include <Commons/CorbaAlgs.hpp>

#include <CORBACommons/CorbaAdapters.hpp>
#include <CORBACommons/ServantImpl.hpp>

#include <ChannelSvcs/ChannelCommons/ChannelServer_s.hpp>
#include "ChannelServerCore.hpp"
#include "ChannelUpdateImpl.hpp"

namespace AdServer
{
namespace ChannelSvcs
{
  namespace
  {
    void
    pack_ccg_keyword(
      const ChannelServerCore::CCGKeyword& source,
      ChannelCurrent::CCGKeyword& target)
    {
      target.ccg_keyword_id = source.ccg_keyword_id;
      target.ccg_id = source.ccg_id;
      target.channel_id = source.channel_id;
      target.max_cpc =
        CorbaAlgs::pack_decimal<CampaignSvcs::RevenueDecimal>(
          source.max_cpc);
      target.ctr = CorbaAlgs::pack_decimal<CampaignSvcs::CTRDecimal>(
        source.ctr);
      target.click_url << source.click_url;
      target.original_keyword << source.original_keyword;
    }
  }

  ChannelUpdateImpl::ChannelUpdateImpl(ChannelServerCorePtr server)
    /*throw(eh::Exception)*/
    : server_(std::move(server))
  {
  }

  ChannelUpdateImpl::~ChannelUpdateImpl() noexcept
  {
  }

  //
  // IDL:AdServer/ChannelSvcs/ChannelCurrent/check:1.0
  //
  void ChannelUpdateImpl::check(
    const ::AdServer::ChannelSvcs::ChannelCurrent::CheckQuery& query,
    ::AdServer::ChannelSvcs::ChannelCurrent::CheckData_out data)
    /*throw(AdServer::ChannelSvcs::ImplementationException,
      AdServer::ChannelSvcs::NotConfigured)*/
  {
    try
    {
      ChannelServerCore::CheckQuery core_query;
      core_query.master_stamp = CorbaAlgs::unpack_time(query.master_stamp);
      core_query.new_ids.assign(
        query.new_ids.get_buffer(),
        query.new_ids.get_buffer() + query.new_ids.length());
      core_query.use_only_list = query.use_only_list;

      ChannelServerCore::CheckData core_data;
      server_->check(core_query, core_data);

      data = new ChannelCurrent::CheckData;
      data->first_stamp = CorbaAlgs::pack_time(core_data.first_stamp);
      data->master_stamp = CorbaAlgs::pack_time(core_data.master_stamp);
      data->versions.length(core_data.versions.size());
      for(CORBA::ULong i = 0; i < core_data.versions.size(); ++i)
      {
        data->versions[i].id = core_data.versions[i].id;
        data->versions[i].size = core_data.versions[i].size;
        data->versions[i].stamp =
          CorbaAlgs::pack_time(core_data.versions[i].stamp);
      }
      data->special_track = core_data.special_track;
      data->special_adv = core_data.special_adv;
      data->source_id = core_data.source_id;
      data->max_time = CorbaAlgs::pack_time(core_data.max_time);
    }
    catch(const ChannelServerCore::NotConfigured& ex)
    {
      throw AdServer::ChannelSvcs::NotConfigured(ex.what());
    }
    catch(const ChannelServerCore::Exception& ex)
    {
      CORBACommons::throw_desc<ImplementationException>(
        String::SubString(ex.what()));
    }
  }

  //
  // IDL:AdServer/ChannelSvcs/ChannelServerControl/update_channels:1.0
  //
  void ChannelUpdateImpl::update_triggers(
    const ::AdServer::ChannelSvcs::ChannelIdSeq& ids,
    ::AdServer::ChannelSvcs::ChannelCurrent::UpdateData_out result)
    /*throw(AdServer::ChannelSvcs::ImplementationException,
      AdServer::ChannelSvcs::NotConfigured)*/
  {
    try
    {
      std::vector<unsigned long> core_ids(
        ids.get_buffer(),
        ids.get_buffer() + ids.length());
      ChannelServerCore::UpdateDataResult core_result;
      server_->update_triggers(core_ids, core_result);

      result = new ChannelCurrent::UpdateData;
      result->source_id = core_result.source_id;
      result->channels.length(core_result.channels.size());
      for(CORBA::ULong i = 0; i < core_result.channels.size(); ++i)
      {
        const auto& core_channel = core_result.channels[i];
        auto& target_channel = result->channels[i];
        target_channel.channel_id = core_channel.channel_id;
        target_channel.words.length(core_channel.words.size());
        for(CORBA::ULong j = 0; j < core_channel.words.size(); ++j)
        {
          const auto& core_word = core_channel.words[j];
          target_channel.words[j].channel_trigger_id =
            core_word.channel_trigger_id;
          target_channel.words[j].trigger.length(core_word.trigger.size());
          memcpy(
            target_channel.words[j].trigger.get_buffer(),
            core_word.trigger.data(),
            core_word.trigger.size());
        }
        target_channel.stamp = CorbaAlgs::pack_time(core_channel.stamp);
      }
    }
    catch(const ChannelServerCore::NotConfigured& ex)
    {
      throw AdServer::ChannelSvcs::NotConfigured(ex.what());
    }
    catch(const ChannelServerCore::Exception& ex)
    {
      CORBACommons::throw_desc<ImplementationException>(
        String::SubString(ex.what()));
    }
  }

  //
  // IDL:AdServer/ChannelSvcs/ChannelServerControl/update_all_ccg:1.0
  //
  void ChannelUpdateImpl::update_all_ccg(
    const AdServer::ChannelSvcs::ChannelCurrent::CCGQuery& query,
    AdServer::ChannelSvcs::ChannelCurrent::PosCCGResult_out result)
    /*throw(AdServer::ChannelSvcs::ImplementationException,
      AdServer::ChannelSvcs::NotConfigured)*/
  {
    try
    {
      ChannelServerCore::CCGQuery core_query;
      core_query.master_stamp = CorbaAlgs::unpack_time(query.master_stamp);
      core_query.start = query.start;
      core_query.limit = query.limit;
      core_query.channel_ids.assign(
        query.channel_ids.get_buffer(),
        query.channel_ids.get_buffer() + query.channel_ids.length());
      core_query.use_only_list = query.use_only_list;

      ChannelServerCore::PosCCGResult core_result;
      server_->update_all_ccg(core_query, core_result);

      result = new ChannelCurrent::PosCCGResult;
      result->start_id = core_result.start_id;
      result->keywords.length(core_result.keywords.size());
      for(CORBA::ULong i = 0; i < core_result.keywords.size(); ++i)
      {
        pack_ccg_keyword(core_result.keywords[i], result->keywords[i]);
      }
      result->deleted.length(core_result.deleted.size());
      for(CORBA::ULong i = 0; i < core_result.deleted.size(); ++i)
      {
        result->deleted[i].id = core_result.deleted[i].id;
        result->deleted[i].stamp =
          CorbaAlgs::pack_time(core_result.deleted[i].stamp);
      }
      result->source_id = core_result.source_id;
    }
    catch(const ChannelServerCore::NotConfigured& ex)
    {
      throw AdServer::ChannelSvcs::NotConfigured(ex.what());
    }
    catch(const ChannelServerCore::Exception& ex)
    {
      CORBACommons::throw_desc<ImplementationException>(
        String::SubString(ex.what()));
    }
  }

  ::CORBA::ULong ChannelUpdateImpl::get_count_chunks()
    /*throw(AdServer::ChannelSvcs::ImplementationException)*/
  {
    return server_->get_count_chunks();
  }
}
}
