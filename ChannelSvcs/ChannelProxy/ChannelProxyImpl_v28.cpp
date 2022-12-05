
#include <ReferenceCounting/ReferenceCounting.hpp>

#include <eh/Exception.hpp>

#include <set>
#include <vector>
#include <sstream>
#include <iostream>

#include <Logger/Logger.hpp>
#include <Generics/ActiveObject.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/TaskRunner.hpp>
#include <Generics/Time.hpp>

#include <CORBACommons/CorbaAdapters.hpp>
#include <CORBACommons/ServantImpl.hpp>

#include <Commons/Algs.hpp>
#include <Commons/CorbaAlgs.hpp>

#include <ChannelSvcs/ChannelCommons/Serialization.hpp>
#include <ChannelSvcs/ChannelProxy/ChannelProxy_v28_s.hpp>


#include "Serialization_v28.hpp"
#include "ChannelProxy.hpp"
#include "ChannelProxyImpl_v28.hpp"

namespace AdServer
{
namespace ChannelSvcs
{
  namespace
  {
    const char *EX_IMPL = "ChannelUpdate_v33::ImplementationException";
    const char *EX_NOT_CONF  = "ChannelUpdate_v33::NotConfigured";
  }

  /*************************************************************************
   * ChannelProxyImpl_v28
   * **********************************************************************/
  struct ChannelVersion_v33_to_v28
  {
    ::AdServer::ChannelSvcs::ChannelUpdate_v28::ChannelVersion 
    operator()(
      const ::AdServer::ChannelSvcs::ChannelUpdate_v33::ChannelVersion& in)
    {
      ::AdServer::ChannelSvcs::ChannelUpdate_v28::ChannelVersion out;
      out.id = in.id;
      out.size = in.size;
      out.stamp = in.stamp;
      return out;
    }
  };

  struct TriggerVersion_v33_to_v28
  {
    ::AdServer::ChannelSvcs::ChannelUpdate_v28::TriggerVersion 
    operator()(
      const ::AdServer::ChannelSvcs::ChannelUpdate_v33::TriggerVersion& in)
    {
      ::AdServer::ChannelSvcs::ChannelUpdate_v28::TriggerVersion out;
      out.id = in.id;
      out.stamp = in.stamp;
      return out;
    }
  };

  struct CCGKeyword_v33_to_v28
  {
    AdServer::ChannelSvcs::ChannelUpdate_v28::CCGKeyword
    operator()(const AdServer::ChannelSvcs::ChannelUpdate_v33::CCGKeyword& in)
    noexcept
    {
      AdServer::ChannelSvcs::ChannelUpdate_v28::CCGKeyword out;
      out.ccg_keyword_id = in.ccg_keyword_id;
      out.ccg_id = in.ccg_id;
      out.channel_id = in.channel_id;
      out.max_cpc = in.max_cpc;
      out.ctr = in.ctr;
      out.click_url = in.click_url;
      out.original_keyword = in.original_keyword;
      return out;
    }
  };

  struct ChannelById_v33_to_v28
  {
    AdServer::ChannelSvcs::ChannelUpdate_v28::ChannelById
    operator()(const AdServer::ChannelSvcs::ChannelUpdate_v33::ChannelById& in)
    noexcept
    {
      AdServer::ChannelSvcs::ChannelUpdate_v28::ChannelById out;
      out.channel_id = in.channel_id;
      out.words.length(in.words.length());
      auto j = 0UL;
      for(auto i = 0UL; i < in.words.length(); i++)
      {
        if(Serialization::trigger_type(in.words[i].trigger.get_buffer()) != 'R')
        {
          ChannelSvcs_v28::Parts parts;
          out.words[j].channel_trigger_id = in.words[i].channel_trigger_id;
          out.words[j].trigger.length(in.words[i].trigger.length());
          ChannelSvcs_v28::Serialization::serialize(
            ChannelSvcs::Serialization::get_parts(
              in.words[i].trigger.get_buffer(),
              in.words[i].trigger.length(), parts),
            Serialization::trigger_type(in.words[i].trigger.get_buffer()),
            Serialization::exact(in.words[i].trigger.get_buffer()),
            Serialization::negative(in.words[i].trigger.get_buffer()),
            Serialization::NullAllocator(),
            out.words[j].trigger);
          j++;
        }
      }
      out.words.length(j);
      out.stamp = in.stamp;
      return out;
    }
  };

  ChannelProxyImpl_v28::ChannelProxyImpl_v28(ChannelProxyImpl* impl) noexcept
    : impl_(ReferenceCounting::add_ref(impl))
  {
  }

  ChannelProxyImpl_v28::~ChannelProxyImpl_v28() noexcept
  {
  }

  //
  // IDL:AdServer/ChannelSvcs/ChannelUpdate_v33/check:1.0
  //
  void ChannelProxyImpl_v28::check(
    const ::AdServer::ChannelSvcs::ChannelUpdate_v28::CheckQuery& query,
    ::AdServer::ChannelSvcs::ChannelUpdate_v28::CheckData_out data) 
    /*throw(AdServer::ChannelSvcs::ImplementationException,
      AdServer::ChannelSvcs::NotConfigured)*/
  {
    try
    {
      ::AdServer::ChannelSvcs::ChannelUpdate_v33::CheckQuery query_new;
      ::AdServer::ChannelSvcs::ChannelUpdate_v33::CheckData_var data_new;
      query_new.colo_id = query.colo_id;
      query_new.version = query.version;
      query_new.master_stamp = query.master_stamp;
      query_new.use_only_list = false;
      impl_->check(query_new, data_new);
      data = new ::AdServer::ChannelSvcs::ChannelUpdate_v28::CheckData;
      data->first_stamp = data_new->first_stamp;
      data->master_stamp = data_new->master_stamp;
      data->source_id = data_new->source_id;
      data->max_time = data_new->max_time;
      data->special_adv = data_new->special_adv;
      data->special_track = data_new->special_track;
      data->versions.length(data_new->versions.length());
      std::transform(
        data_new->versions.get_buffer(),
        data_new->versions.get_buffer() + data_new->versions.length(),
        data->versions.get_buffer(),
        ChannelVersion_v33_to_v28());
    }
    catch(const AdServer::ChannelSvcs::ImplementationException& e)
    {
      Stream::Error ostr;
      ostr << __func__ << ": "
        "AdServer::ChannelSvcs::ImplementationException. "
        ": " << e.description;
      CORBACommons::throw_desc<
        ChannelSvcs::ImplementationException>(
          ostr.str());
    }
    catch (const AdServer::ChannelSvcs::NotConfigured& ex)
    {
      Stream::Error ostr;
      ostr << __func__ << ": "
        "AdServer::ChannelSvcs::NotConfigured. "
        ": " << ex.description;
      CORBACommons::throw_desc<
        ChannelSvcs::NotConfigured>(
          ostr.str());
    }
    catch(const CORBA::SystemException& e)
    {
      Stream::Error ostr;
      ostr << __func__ << ": CORBA::SystemException. : " << e;
        CORBACommons::throw_desc<
          ChannelSvcs::ImplementationException>(
            ostr.str());
    }
  }

  //
  // IDL:AdServer/ChannelSvcs/ChannelServerControl/update_triggers:1.0
  //
  void ChannelProxyImpl_v28::update_triggers(
    const ::AdServer::ChannelSvcs::ChannelIdSeq& ids,
    ::AdServer::ChannelSvcs::ChannelUpdate_v28::UpdateData_out result)
    /*throw(AdServer::ChannelSvcs::ImplementationException,
      AdServer::ChannelSvcs::NotConfigured)*/
  {
    try
    {
      ::AdServer::ChannelSvcs::ChannelUpdate_v33::UpdateData_var data_new;
      impl_->update_triggers(ids, data_new);
      result = new ::AdServer::ChannelSvcs::ChannelUpdate_v28::UpdateData;
      result->source_id = data_new->source_id;
      result->channels.length(data_new->channels.length());
      std::transform(
        data_new->channels.get_buffer(),
        data_new->channels.get_buffer() + data_new->channels.length(),
        result->channels.get_buffer(),
        ChannelById_v33_to_v28());
    }
    catch(const AdServer::ChannelSvcs::ImplementationException& e)
    {
      Stream::Error ostr;
      ostr << __func__ << ": "
        "AdServer::ChannelSvcs::ImplementationException. "
        ": " << e.description;
      CORBACommons::throw_desc<
        ChannelSvcs::ImplementationException>(
          ostr.str());
    }
    catch(const AdServer::ChannelSvcs::NotConfigured& e)
    {
      Stream::Error ostr;
      ostr << __func__ << ": " << EX_NOT_CONF << ": " << e.description;

      CORBACommons::throw_desc<
        ChannelSvcs::NotConfigured>(
          ostr.str());
    }
    catch(const CORBA::SystemException& e)
    {
      Stream::Error ostr;
      ostr << __func__ << ": CORBA::SystemException. : " << e;
      CORBACommons::throw_desc<
        ChannelSvcs::ImplementationException>(
          ostr.str());
    }
  }

  void
  ChannelProxyImpl_v28::update_all_ccg(
    const AdServer::ChannelSvcs::ChannelUpdate_v28::CCGQuery& in,
    AdServer::ChannelSvcs::ChannelUpdate_v28::PosCCGResult_out data)
    /*throw(AdServer::ChannelSvcs::ImplementationException,
      AdServer::ChannelSvcs::NotConfigured)*/
  {
    try
    {
      AdServer::ChannelSvcs::ChannelUpdate_v33::PosCCGResult_var data_new;
      AdServer::ChannelSvcs::ChannelUpdate_v33::CCGQuery in_new;
      in_new.master_stamp = in.master_stamp;
      in_new.start = in.start;
      in_new.limit = in.limit;
      CorbaAlgs::copy_sequence(in.channel_ids, in_new.channel_ids);
      in_new.use_only_list = false;
      impl_->update_all_ccg(in_new, data_new);
      data = new AdServer::ChannelSvcs::ChannelUpdate_v28::PosCCGResult;
      data->start_id = data_new->start_id;
      data->source_id = data_new->source_id;
      if(data_new->keywords.length())
      {
        data->keywords.length(data_new->keywords.length());
        std::transform(
          data_new->keywords.get_buffer(),
          data_new->keywords.get_buffer() + data_new->keywords.length(),
          data->keywords.get_buffer(),
          CCGKeyword_v33_to_v28());
      }
      if(data_new->deleted.length())
      {
        data->deleted.length(data_new->deleted.length());
        std::transform(
          data_new->deleted.get_buffer(),
          data_new->deleted.get_buffer() + data_new->deleted.length(),
          data->deleted.get_buffer(),
          TriggerVersion_v33_to_v28());
      }
    }
    catch(const AdServer::ChannelSvcs::ImplementationException& e)
    {
      Stream::Error ostr;
      ostr << __func__ << ": " << EX_IMPL << ": " << e.description;
      CORBACommons::throw_desc<
        ChannelSvcs::ImplementationException>(
          ostr.str());
    }
    catch(const AdServer::ChannelSvcs::NotConfigured& e)
    {
      Stream::Error ostr;
      ostr << __func__ << ": " << EX_NOT_CONF << ": " << e.description;

      CORBACommons::throw_desc<
        ChannelSvcs::NotConfigured>(
          ostr.str());
    }
    catch(const CORBA::SystemException& e)
    {
      Stream::Error ostr;
      ostr << __func__ << ": CORBA::SystemException. : " << e;
      CORBACommons::throw_desc<
        ChannelSvcs::ImplementationException>(ostr.str());
    }
  }

  //
  // IDL:AdServer/ChannelSvcs/ChannelProxy/get_count_chunks:1.0
  //
  ::CORBA::ULong ChannelProxyImpl_v28::get_count_chunks()
  {
    try
    {
      return impl_->get_count_chunks();
    }
    catch(const AdServer::ChannelSvcs::ImplementationException& e)
    {
      Stream::Error ostr;
      ostr << __func__ << ": " << EX_IMPL << ": " << e.description;
      CORBACommons::throw_desc<
        ChannelSvcs::ImplementationException>(
          ostr.str());
    }
    catch(const CORBA::SystemException& e)
    {
      Stream::Error ostr;
      ostr << __func__ << ": CORBA::SystemException. : " << e;
      CORBACommons::throw_desc<
        ChannelSvcs::ImplementationException>(ostr.str());
    }
    return 0; // never reach
  }

} /* ChannelSvcs */
} /* AdServer */
