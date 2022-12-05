/**
 * ChannelServerSessionFactoryImpl
 */

#include <CORBACommons/CorbaAdapters.hpp>
#include <Commons/CorbaAlgs.hpp>
#include <ChannelSvcs/ChannelManagerController/ThreadHandlerTemplate.hpp>
#include "ChannelLoadSessionImpl.hpp"

namespace AdServer
{
namespace ChannelSvcs
{

  struct Filter
  {
    Filter(unsigned int* begin, unsigned int* end, unsigned int count) noexcept
      : begin_(begin), end_(end), count_(count) {};

    bool operator()(unsigned int id) noexcept;

    unsigned int* begin_;
    unsigned int* end_;
    unsigned int count_;
  };

  bool Filter::operator()(unsigned int id) noexcept
  {
    return (std::find(begin_, end_, id % count_) == end_); 
  }

  /**
   * ChannelLoadSessionImpl
   * implementation of ChannelLoadSession valuetype 
   */

  ChannelLoadSessionImpl::ChannelLoadSessionImpl(
    Generics::ActiveObjectCallback* callback,
    Generics::TaskRunner* runner)
    noexcept
    : task_runner_(ReferenceCounting::add_ref(runner)),
      callback_(ReferenceCounting::add_ref(callback)),
      index_(0)
  {
  }

  ChannelLoadSessionImpl::ChannelLoadSessionImpl(
    const ChannelSvcs::GroupLoadDescriptionSeq& servers,
    CORBA::ULong source)
    /*throw(Exception)*/
      : index_(0)
  { // XXX-1
    try
    {
      /*
      ChannelSvcs::GroupLoadDescriptionSeq servers_copy(servers);
      for(CORBA::ULong group_i = 0; group_i < servers_copy.length(); ++group_i)
      {
        for(CORBA::ULong server_i = 0; server_i < servers_copy[group_i].length(); ++server_i)
        {
          auto& server = servers_copy[group_i][server_i];
          server.load_server =
            AdServer::ChannelSvcs::ChannelUpdateBase_v33::_narrow(
              static_cast<CORBA::Object*>(server.load_server.in()));
        }
      }

      load_servers(servers_copy);
      */
      load_servers(servers);
      source_id(source);
    }
    catch(const CORBA::SystemException& e)
    {
      Stream::Error ostr;
      ostr << __func__ << ": CORBA::SystemException. :" << e;
      throw Exception(ostr);
    }
  }

  ChannelLoadSessionImpl::ChannelLoadSessionImpl(
      ChannelLoadSessionImpl& init)
      /*throw(eh::Exception, Exception)*/
      : CORBA::ValueBase(),
        CORBA::AbstractBase(),
        AdServer::ChannelSvcs::ChannelUpdateBase_v33(),
        AdServer::ChannelSvcs::ChannelLoadSession(),
        CORBA::DefaultValueRefCountBase(),
        OBV_AdServer::ChannelSvcs::ChannelLoadSession(),
        task_runner_(init.task_runner_),
        callback_(init.callback_),
        index_(0)
  {
    try
    {
      load_servers(init.load_servers());
      source_id(init.source_id());
    }
    catch(CORBA::SystemException& e)
    {
      Stream::Error ostr;
      ostr << __func__ << ": CORBA::SystemException. :" << e;
      throw Exception(ostr);
    }
  }

  ChannelLoadSessionImpl::~ChannelLoadSessionImpl() noexcept
  {
  }

  CORBA::ValueBase* ChannelLoadSessionImpl::_copy_value()
  {
    try
    {
      return new ChannelLoadSessionImpl(*this);
    }
    catch(const Exception& e)
    {
      if(callback_)
      {
        Stream::Error ostr;
        ostr << __func__ << ": Exception: " << e.what();
        callback_->critical(ostr.str());
      }
    }
    catch(const eh::Exception& e)
    {
      if(callback_)
      {
        Stream::Error ostr;
        ostr << __func__ << ": eh::Exception: " << e.what();
        callback_->critical(ostr.str());
      }
      throw;
    }
    return 0;
  }

  struct SmartCheck:
    public Protected::BaseCaller<
      ChannelCurrent::CheckData_var,
      ChannelLoadDescription>
  {
    SmartCheck(const char* name, size_t len) /*throw(eh::Exception)*/
      : Protected::BaseCaller<
        ChannelCurrent::CheckData_var,
        ChannelLoadDescription> (name, len)
    {};

    const ChannelCurrent::CheckQuery* params;

    void execute()
      /*throw(AdServer::ChannelSvcs::ImplementationException,
        AdServer::ChannelSvcs::NotConfigured,
        CORBA::SystemException)*/;
  };

  void
  SmartCheck::execute()
    /*throw(AdServer::ChannelSvcs::ImplementationException,
      AdServer::ChannelSvcs::NotConfigured,
      CORBA::SystemException)*/
  {
    ChannelCurrent::CheckData_var result;
    ref_->load_server->check(*params, result);
    callback_->set_result(id_, result);
  }

  size_t ChannelLoadSessionImpl::get_item_() noexcept
  {
    RGuard_ lock(init_lock_);
    return index_;
  }

  void ChannelLoadSessionImpl::bad_(size_t index) noexcept
  {
    WGuard_ lock(init_lock_);
    if(index == index_)
    {
      index_++;
      if(index_ == load_servers().length())
      {
        index_ = 0;
      }
    }
  }

  //
  // IDL:AdServer/ChannelSvcs/ChannelCurrent/check:1.0
  //
  void
  ChannelLoadSessionImpl::check(
    const ::AdServer::ChannelSvcs::ChannelCurrent::CheckQuery& query,
    ::AdServer::ChannelSvcs::ChannelCurrent::CheckData_out data) 
    /*throw(AdServer::ChannelSvcs::ImplementationException,
      AdServer::ChannelSvcs::NotConfigured)*/
  {
    if(!load_servers().length())
    {
      throw AdServer::ChannelSvcs::NotConfigured();
    }
    size_t index =  get_item_();
    try
    {
      size_t len = load_servers()[index].length();
      SmartCheck cquery(__func__, len);
      cquery.params = &query;
      Generics::Task_var task;
      for(size_t i = 0; i < len; i++)
      {
        cquery.id_ = i;
        cquery.ref_ = &load_servers()[index][i];
        task = new Protected::CallTask<SmartCheck>(cquery);
        if(i != len - 1)
        {
          task_runner_->enqueue_task(task);
        }
      }
      task->execute();
      data = new ChannelCurrent::CheckData;
      data->special_adv = false;
      data->special_track = false;
      size_t answer_len = 0;
      Generics::Time master, max_time, temp, temp2, first_stamp;
      bool first = true;
      SmartCheck::Handler::ResultsVector& results =
        cquery.callback_->get_result();
      if(cquery.callback_->count_exception() > 0)
      {
        Stream::Error ostr;
        bad_(index);
        ostr << __func__ << ": " << cquery.callback_->count_exception()
          << " ChannelServers are failed. : "
          << cquery.callback_->get_exception().str();
        CORBACommons::throw_desc<
          ChannelSvcs::ImplementationException>(ostr.str());
      }
      for(size_t i = 0; i < len;i++)
      {
        ChannelSvcs::ChannelCurrent::CheckData_var& value = results[i];
        if(value)
        {
          answer_len += value->versions.length();
          temp = CorbaAlgs::unpack_time(value->master_stamp);
          temp2 = CorbaAlgs::unpack_time(value->max_time);
          if(first)
          {
            master = temp;
            max_time = temp2;
            data->source_id = value->source_id;
            first_stamp = CorbaAlgs::unpack_time(value->first_stamp);
            first = false;
          }
          else
          {
            master = std::min(master, temp);
            max_time = std::max(max_time, temp2);
            first_stamp = std::max(
              first_stamp, CorbaAlgs::unpack_time(value->first_stamp));
          }
          if(value->special_adv)
          {
            data->special_adv = true;
          }
          if(value->special_track)
          {
            data->special_track = true;
          }
        }
      }
      data->master_stamp = CorbaAlgs::pack_time(master);
      data->first_stamp = CorbaAlgs::pack_time(first_stamp);
      data->versions.length(answer_len);
      data->max_time = CorbaAlgs::pack_time(max_time);
      if(data->source_id < 0 && source_id() > 0)
      {
        data->source_id = source_id() + index;
      }
      if(answer_len > 0)
      {
        size_t j = 0;
        for(size_t i = 0; i < len; i++)
        {
          ChannelSvcs::ChannelCurrent::CheckData_var& value = results[i];
          if(value)
          {
            std::copy(value->versions.get_buffer(),
                      value->versions.get_buffer() + value->versions.length(),
                      data->versions.get_buffer() + j);
            j += value->versions.length();
            value = 0;
          }
        }
      }
    }
    catch(const SmartCheck::Handler::Exception& e)
    {
      Stream::Error ostr;
      bad_(index);
      ostr << __func__ << ": SmartCheck::Handler::Exception: "
        << e.what();
      CORBACommons::throw_desc<
        ChannelSvcs::ImplementationException>(ostr.str());
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      bad_(index);
      ostr << __func__ << ": eh::Exception: " << e.what();
      CORBACommons::throw_desc<
        ChannelSvcs::ImplementationException>(ostr.str());
    }
    catch(const AdServer::ChannelSvcs::NotConfigured& e)
    {
      Stream::Error ostr;
      bad_(index);
      ostr << __func__ << ": NotConfigured: " << e.description;
      CORBACommons::throw_desc<
        ChannelSvcs::ImplementationException>(ostr.str());
    }
    catch(const ChannelSvcs::ImplementationException& e)
    {
      Stream::Error ostr;
      bad_(index);
      ostr << __func__ << ": ImplementationException: " << e.description;
      CORBACommons::throw_desc<
        ChannelSvcs::ImplementationException>(ostr.str());
    }
    catch(const CORBA::SystemException& e)
    {
      Stream::Error ostr;
      bad_(index);
      ostr << __func__ << ": CORBA::SystemException: " << e;
      CORBACommons::throw_desc<
        ChannelSvcs::ImplementationException>(ostr.str());
    }
  }

  // IDL:AdServer/ChannelSvcs/ChannelProxy/get_count_chunks:1.0
  //
  ::CORBA::ULong ChannelLoadSessionImpl::get_count_chunks()
    /*throw(AdServer::ChannelSvcs::ImplementationException)*/
  {
    if(load_servers().length() == 1 && load_servers()[0].length() == 1)
    {
      try
      {
        return load_servers()[0][0].load_server->get_count_chunks();
      }
      catch(const AdServer::ChannelSvcs::ImplementationException& e)
      {
        Stream::Error ostr;
        ostr << __func__ << ": ImplementationException: " << e.description;
        CORBACommons::throw_desc<
          ChannelSvcs::ImplementationException>(ostr.str());
      }
      catch(const CORBA::SystemException& e)
      {
        Stream::Error ostr;
        ostr << __func__ << ": CORBA::SystemException: " << e;
        CORBACommons::throw_desc<
          ChannelSvcs::ImplementationException>(ostr.str());
      }
    }
    throw AdServer::ChannelSvcs::ImplementationException
      ("get_count_chunks not supported for multi server configuration");
  }

  struct UpdateTriggers:
    public Protected::BaseCaller<
      ChannelCurrent::UpdateData_var,
      ChannelLoadDescription>
  {
    UpdateTriggers(const char* name, size_t len) /*throw(eh::Exception)*/
      : Protected::BaseCaller<
          ChannelCurrent::UpdateData_var,
          ChannelLoadDescription> (name, len)
    {}

    void execute()
      /*throw(AdServer::ChannelSvcs::ImplementationException,
        AdServer::ChannelSvcs::NotConfigured,
        CORBA::SystemException)*/;

    const ::AdServer::ChannelSvcs::ChannelIdSeq* ids_;
  };

  void
  UpdateTriggers::execute()
    /*throw(AdServer::ChannelSvcs::ImplementationException,
      AdServer::ChannelSvcs::NotConfigured,
      CORBA::SystemException)*/
  {
    AdServer::ChannelSvcs::ChannelIdSeq ids;
    const ::AdServer::ChannelSvcs::ChannelIdSeq* ids_ptr;
    if(ref_->count_chunks)
    {
      ids.length(ids_->length());
      CORBA::ULong* end = std::remove_copy_if(
        ids_->get_buffer(),
        ids_->get_buffer() + ids_->length(),
        ids.get_buffer(),
        Filter(
          ref_->chunks.get_buffer(),
          ref_->chunks.get_buffer() + ref_->chunks.length(),
          ref_->count_chunks));
      ids.length(end - ids.get_buffer());
      ids_ptr = &ids;
    }
    else
    {
      ids_ptr = ids_;
    }
    AdServer::ChannelSvcs::ChannelCurrent::UpdateData_var res;
    ref_->load_server->update_triggers(*ids_ptr, res);
    callback_->set_result(id_, res);
  }

  //
  // IDL:AdServer/ChannelSvcs/ChannelServerControl/update_triggers:1.0
  //
  void
  ChannelLoadSessionImpl::update_triggers(
    const ::AdServer::ChannelSvcs::ChannelIdSeq& ids,
    ::AdServer::ChannelSvcs::ChannelCurrent::UpdateData_out result)
    /*throw(AdServer::ChannelSvcs::ImplementationException,
      AdServer::ChannelSvcs::NotConfigured)*/
  {
    if(!load_servers().length())
    {
      throw AdServer::ChannelSvcs::NotConfigured();
    }
    size_t index = get_item_();
    try
    {
      size_t len = load_servers()[index].length();
      UpdateTriggers query(__func__, len);
      query.ids_ = &ids;
      Generics::Task_var task;
      for(size_t i = 0; i < len; i++)
      {
        query.id_ = i;
        query.ref_ = &load_servers()[index][i];
        task = new Protected::CallTask<UpdateTriggers>(query);
        if(i != len - 1)
        {
          task_runner_->enqueue_task(task);
        }
      }
      task->execute();
      result = new AdServer::ChannelSvcs::ChannelCurrent::UpdateData;
      UpdateTriggers::Handler::ResultsVector& results =
        query.callback_->get_result();
      if(query.callback_->count_exception() > 0)
      {
        Stream::Error ostr;
        bad_(index);
        ostr << __func__ << ": " << query.callback_->count_exception()
          << " ChannelServers are failed. : "
          << query.callback_->get_exception().str();
        CORBACommons::throw_desc<
          ChannelSvcs::ImplementationException>(ostr.str());
      }
      try
      {
        result->source_id = -1;
        size_t old_length = 0;
        for(size_t i = 0; i < len; i++)
        {
          ChannelSvcs::ChannelCurrent::UpdateData_var& value =
            results[i];
          if(value)
          {
            result->source_id = value->source_id;
            old_length += value->channels.length();
          }
        }
        result->channels.length(old_length);
        old_length = 0;
        for(size_t i = 0; i < len; i++)
        {
          ChannelSvcs::ChannelCurrent::UpdateData_var& value =
            results[i];
          if(value && value->channels.length())
          {
            std::copy(
                value->channels.get_buffer(), 
                value->channels.get_buffer() + value->channels.length(),
                result->channels.get_buffer() + old_length);
            old_length += value->channels.length();
          }
        }
        if(result->source_id < 0 && source_id() > 0)
        {
          result->source_id = source_id() + index;
        }
      }
      catch(const eh::Exception& e)
      {
        bad_(index);
        throw AdServer::ChannelSvcs::ImplementationException(e.what());
      }
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      bad_(index);
      ostr << __func__ << ": eh::Exception: " << e.what();
      CORBACommons::throw_desc<
        ChannelSvcs::ImplementationException>(ostr.str());
    }
    catch(const AdServer::ChannelSvcs::NotConfigured& e)
    {
      Stream::Error ostr;
      bad_(index);
      ostr << __func__ << ": NotConfigured: " << e.description;
      CORBACommons::throw_desc<
        ChannelSvcs::ImplementationException>(ostr.str());
    }
    catch(const AdServer::ChannelSvcs::ImplementationException& e)
    {
      Stream::Error ostr;
      bad_(index);
      ostr << __func__ << ": ImplementationException: " << e.description;
      CORBACommons::throw_desc<
        ChannelSvcs::ImplementationException>(ostr.str());
    }
    catch(const CORBA::SystemException& e)
    {
      Stream::Error ostr;
      bad_(index);
      ostr << __func__ << ": CORBA::SystemException: " << e;
      CORBACommons::throw_desc<
        ChannelSvcs::ImplementationException>(ostr.str());
    }
  }

  struct UpdatePosCCG:
    public Protected::BaseCaller<
      ChannelCurrent::PosCCGResult_var,
      ChannelLoadDescription>
  {
    UpdatePosCCG(const char* name, size_t len) /*throw(eh::Exception)*/
      : Protected::BaseCaller<
          ChannelCurrent::PosCCGResult_var,
          ChannelLoadDescription> (name, len)
    {};

    void execute()
      /*throw(AdServer::ChannelSvcs::ImplementationException,
        AdServer::ChannelSvcs::NotConfigured,
        CORBA::SystemException)*/;

    const AdServer::ChannelSvcs::ChannelCurrent::CCGQuery* in;
  };

  void
  UpdatePosCCG::execute()
    /*throw(AdServer::ChannelSvcs::ImplementationException,
      AdServer::ChannelSvcs::NotConfigured,
      CORBA::SystemException)*/
  {
    AdServer::ChannelSvcs::ChannelCurrent::PosCCGResult_var result;

    {
      /*
      AdServer::ChannelSvcs::ChannelCurrent::CCGQuery debug_in;
      debug_in.master_stamp = CorbaAlgs::pack_time(Generics::Time::get_time_of_day());
      debug_in.start = 1;
      debug_in.limit = 10000;
      std::vector<unsigned long> channels_id;

      for(int i = 0; i < 434686; ++i)
      {
        channels_id.emplace_back(i);
      }

      debug_in.channel_ids.length(channels_id.size());                                                                             
      std::copy(
        channels_id.begin(),
        channels_id.end(),
        debug_in.channel_ids.get_buffer());
      */

      ref_->load_server->update_all_ccg(*in, result);
      //ref_->load_server->update_all_ccg(debug_in, result);
    }

    callback_->set_result(id_, result);
  }

  void
  ChannelLoadSessionImpl::update_all_ccg(
    const AdServer::ChannelSvcs::ChannelCurrent::CCGQuery& query_in,
    AdServer::ChannelSvcs::ChannelCurrent::PosCCGResult_out result)
    /*throw(AdServer::ChannelSvcs::ImplementationException,
      AdServer::ChannelSvcs::NotConfigured)*/
  {
    if(!load_servers().length())
    {
      throw AdServer::ChannelSvcs::NotConfigured();
    }
    size_t index = get_item_();
    try
    {
      size_t len = load_servers()[index].length();
      UpdatePosCCG query(__func__, len);
      query.in = &query_in;
      Generics::Task_var task;
      for(size_t i = 0; i < len; i++)
      {
        query.id_ = i;
        query.ref_ = &load_servers()[index][i];
        task = new Protected::CallTask<UpdatePosCCG>(query);
        if(i != len - 1)
        {
          task_runner_->enqueue_task(task);
        }
      }
      task->execute();
      result = new AdServer::ChannelSvcs::ChannelCurrent::PosCCGResult;
      UpdatePosCCG::Handler::ResultsVector& results =
        query.callback_->get_result();

      if(query.callback_->count_exception() > 0)
      {
        Stream::Error ostr;
        bad_(index);
        ostr << __func__ << ": " << query.callback_->count_exception()
          << " ChannelServers are failed. : "
          << query.callback_->get_exception().str();
        CORBACommons::throw_desc<
          ChannelSvcs::ImplementationException>(ostr.str());
      }
      size_t length_of_result[2] = {0, 0};
      unsigned long start_min = ULONG_MAX, start_max = query_in.start;
      bool use_min = false;
      result->source_id = -1;
      for(size_t i = 0; i < len; i++)
      {
        AdServer::ChannelSvcs::ChannelCurrent::PosCCGResult_var& value = 
          results[i];
        result->source_id = value->source_id;
        length_of_result[0] += value->keywords.length();
        length_of_result[1] += value->deleted.length();
        start_min = std::min(
          static_cast<unsigned long>(value->start_id), start_min);
        start_max = std::max(
          static_cast<unsigned long>(value->start_id), start_max);
        if(value->keywords.length() >= query_in.limit)
        {
          use_min = true;
        }
      }
      if(length_of_result[0] > query_in.limit)
      {
        std::multiset<unsigned int> channels;
        for(size_t i = 0; i < len; i++)
        {
          for(size_t j = 0; j < results[i]->keywords.length(); j++)
          {
            AdServer::ChannelSvcs::ChannelCurrent::CCGKeyword& kw =
              results[i]->keywords[j];
            channels.insert(kw.channel_id);
          }
        }
        auto ch_it = channels.begin();
        std::advance(ch_it, query_in.limit);
        result->start_id = *ch_it + 1;
      }
      else if(use_min)
      {
        result->start_id = start_min;
      }
      else
      {
        result->start_id = start_max;
      }

      if(length_of_result[0])
      {
        result->keywords.length(length_of_result[0]);
      }
      else
      {
        result->deleted.length(length_of_result[1]);
      }
      length_of_result[0] = length_of_result[1] = 0;
      for(size_t i = 0; i < results.size(); i++)
      {
        for(size_t j = 0; j < results[i]->keywords.length(); j++)
        {
          AdServer::ChannelSvcs::ChannelCurrent::CCGKeyword& kw =
            results[i]->keywords[j];
          if(kw.channel_id < result->start_id)
          {
            result->keywords[length_of_result[0]++] = kw;
          }
          else
          {
            break;
          }
        }
        if(result->deleted.length())
        {
          std::copy(
            results[i]->deleted.get_buffer(),
            results[i]->deleted.get_buffer() + results[i]->deleted.length(),
            result->deleted.get_buffer() + length_of_result[1]);
          length_of_result[1] += results[i]->deleted.length();
        }
      }
      result->keywords.length(length_of_result[0]);
      if(result->source_id < 0 && source_id() > 0)
      {
        result->source_id = source_id() + index;
      }
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      bad_(index);
      ostr << __func__ << ": eh::Exception: " << e.what();
      CORBACommons::throw_desc<
        ChannelSvcs::ImplementationException>(ostr.str());
    }
    catch(const AdServer::ChannelSvcs::ImplementationException& e)
    {
      Stream::Error ostr;
      bad_(index);
      ostr << __func__ << ": ImplementationException: " << e.description;
      CORBACommons::throw_desc<
        ChannelSvcs::ImplementationException>(ostr.str());
    }
    catch(const AdServer::ChannelSvcs::NotConfigured& e)
    {
      Stream::Error ostr;
      bad_(index);
      ostr << __func__ << ": NotConfigured: " << e.description;
      CORBACommons::throw_desc<
        ChannelSvcs::ImplementationException>(ostr.str());
    }
    catch(const CORBA::SystemException& e)
    {
      Stream::Error ostr;
      bad_(index);
      ostr << __func__ << ": CORBA::SystemException: " << e;
      CORBACommons::throw_desc<
        ChannelSvcs::ImplementationException>(ostr.str());
    }
  }

}// namespace ChannelSvcs
} // namespace AdServer
