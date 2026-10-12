#include <algorithm>
#include <climits>
#include <set>
#include <utility>
#include <ChannelSvcs/ChannelManagerController/ThreadHandlerTemplate.hpp>
#include "ChannelLoadSessionImpl.hpp"

namespace AdServer::ChannelSvcs
{

  struct Filter
  {
    Filter(unsigned int* begin, unsigned int* end, unsigned int count) noexcept;

    bool operator()(unsigned int id) noexcept;

    unsigned int* begin_;
    unsigned int* end_;
    unsigned int count_;
  };

  Filter::Filter(unsigned int* begin, unsigned int* end, unsigned int count) noexcept
    : begin_(begin), end_(end), count_(count)
  {
  }

  bool Filter::operator()(unsigned int id) noexcept
  {
    return (std::find(begin_, end_, id % count_) == end_);
  }

  ChannelLoadSessionImpl::ChannelLoadSessionImpl(
    const GroupLoadDescription& servers, long source, Logging::Logger* logger)
    : servers_(servers),
      source_id_(source),
      callback_(
        new Logging::ActiveObjectCallbackImpl(logger, "ChannelLoadSession", "ChannelUpdate")),
      task_runner_(new Generics::TaskRunner(callback_, 10))
  {
    for (const auto& group : servers_)
    {
      if (group.empty())
      {
        throw Exception("empty ChannelUpdate source group");
      }

      for (const auto& server : group)
      {
        if (!server.load_server)
        {
          throw Exception("missing ChannelUpdate gRPC client");
        }
      }
    }

    task_runner_->activate_object();
  }

  ChannelLoadSessionImpl::~ChannelLoadSessionImpl() noexcept
  {
    task_runner_->deactivate_object();
    task_runner_->wait_object();
  }

  struct SmartCheck:
    public Protected::BaseCaller<ChannelUpdate::CheckDataPtr, ChannelLoadDescription>
  {
    SmartCheck(const char* name, size_t len);

    const ChannelUpdate::CheckQuery* params;

    void execute();
  };

  SmartCheck::SmartCheck(const char* name, size_t len)
    : Protected::BaseCaller<ChannelUpdate::CheckDataPtr, ChannelLoadDescription>(name, len)
  {
  }

  void SmartCheck::execute()
  {
    ChannelUpdate::CheckDataPtr result;
    ref_->load_server->check(*params, result);
    callback_->set_result(id_, result);
  }

  GroupLoadDescription& ChannelLoadSessionImpl::load_servers() noexcept
  {
    return servers_;
  }

  long ChannelLoadSessionImpl::source_id() const noexcept
  {
    return source_id_;
  }

  size_t ChannelLoadSessionImpl::get_item_() noexcept
  {
    RGuard_ lock(init_lock_);
    return index_;
  }

  void ChannelLoadSessionImpl::bad_(size_t index) noexcept
  {
    WGuard_ lock(init_lock_);
    if (index == index_)
    {
      index_++;
      if (index_ == load_servers().size())
      {
        index_ = 0;
      }
    }
  }

  void ChannelLoadSessionImpl::check(
    const ChannelUpdate::CheckQuery& query, ChannelUpdate::CheckDataPtr& data)
  {
    if (load_servers().empty())
    {
      throw NotConfigured("no ChannelUpdate sources");
    }

    size_t index = get_item_();
    try
    {
      size_t len = load_servers()[index].size();
      SmartCheck cquery(__func__, len);
      cquery.params = &query;
      Generics::Task_var task;
      for (size_t i = 0; i < len; i++)
      {
        cquery.id_ = i;
        cquery.ref_ = &load_servers()[index][i];
        task = new Protected::CallTask<SmartCheck>(cquery);
        if (i != len - 1)
        {
          task_runner_->enqueue_task(task);
        }
      }
      task->execute();
      data = std::make_unique<ChannelUpdate::CheckData>();
      data->special_adv = false;
      data->special_track = false;
      size_t answer_len = 0;
      Generics::Time master, max_time, temp, temp2, first_stamp;
      bool first = true;
      SmartCheck::Handler::ResultsVector& results = cquery.callback_->get_result();
      if (cquery.callback_->count_exception() > 0)
      {
        Stream::Error ostr;
        bad_(index);
        ostr << __func__ << ": " << cquery.callback_->count_exception()
          << " ChannelServers are failed. : " << cquery.callback_->get_exception().str();
        throw ImplementationException(ostr.str());
      }
      for (size_t i = 0; i < len; i++)
      {
        ChannelUpdate::CheckDataPtr& value = results[i];
        if (value)
        {
          answer_len += value->versions.size();
          temp = value->master_stamp;
          temp2 = value->max_time;
          if (first)
          {
            master = temp;
            max_time = temp2;
            data->source_id = value->source_id;
            first_stamp = value->first_stamp;
            first = false;
          }
          else
          {
            master = std::min(master, temp);
            max_time = std::max(max_time, temp2);
            first_stamp = std::max(first_stamp, value->first_stamp);
          }

          if (value->special_adv)
          {
            data->special_adv = true;
          }

          if (value->special_track)
          {
            data->special_track = true;
          }
        }
      }
      data->master_stamp = master;
      data->first_stamp = first_stamp;
      data->versions.resize(answer_len);
      data->max_time = max_time;
      if (data->source_id < 0 && source_id() > 0)
      {
        data->source_id = source_id() + index;
      }

      if (answer_len > 0)
      {
        size_t j = 0;
        for (size_t i = 0; i < len; i++)
        {
          ChannelUpdate::CheckDataPtr& value = results[i];
          if (value)
          {
            std::copy(value->versions.begin(), value->versions.end(), data->versions.begin() + j);
            j += value->versions.size();
            value.reset();
          }
        }
      }
    }
    catch (const SmartCheck::Handler::Exception& e)
    {
      Stream::Error ostr;
      bad_(index);
      ostr << __func__ << ": SmartCheck::Handler::Exception: " << e.what();
      throw ImplementationException(ostr.str());
    }
    catch (const eh::Exception& e)
    {
      Stream::Error ostr;
      bad_(index);
      ostr << __func__ << ": eh::Exception: " << e.what();
      throw ImplementationException(ostr.str());
    }
  }

  std::uint32_t ChannelLoadSessionImpl::get_count_chunks()
  {
    if (load_servers().size() == 1 && load_servers()[0].size() == 1)
    {
      try
      {
        return load_servers()[0][0].load_server->get_count_chunks();
      }
      catch (const ImplementationException& e)
      {
        Stream::Error ostr;
        ostr << __func__ << ": ImplementationException: " << e.what();
        throw ImplementationException(ostr.str());
      }
    }

    throw ImplementationException("get_count_chunks not supported for multi server configuration");
  }

  struct UpdateTriggers:
    public Protected::BaseCaller<ChannelUpdate::UpdateDataPtr, ChannelLoadDescription>
  {
    UpdateTriggers(const char* name, size_t len);

    void execute();

    const ChannelIdList* ids_;
  };

  UpdateTriggers::UpdateTriggers(const char* name, size_t len)
    : Protected::BaseCaller<ChannelUpdate::UpdateDataPtr, ChannelLoadDescription>(name, len)
  {
  }

  void UpdateTriggers::execute()
  {
    ChannelIdList ids;
    const ChannelIdList* ids_ptr;
    if (ref_->count_chunks)
    {
      std::remove_copy_if(ids_->begin(), ids_->end(), std::back_inserter(ids),
        Filter(ref_->chunks.data(), ref_->chunks.data() + ref_->chunks.size(), ref_->count_chunks));
      ids_ptr = &ids;
    }
    else
    {
      ids_ptr = ids_;
    }
    ChannelUpdate::UpdateDataPtr res;
    ref_->load_server->update_triggers(*ids_ptr, res);
    callback_->set_result(id_, res);
  }

  void ChannelLoadSessionImpl::update_triggers(
    const ChannelIdList& ids, ChannelUpdate::UpdateDataPtr& result)
  {
    if (load_servers().empty())
    {
      throw NotConfigured("no ChannelUpdate sources");
    }

    size_t index = get_item_();
    try
    {
      size_t len = load_servers()[index].size();
      UpdateTriggers query(__func__, len);
      query.ids_ = &ids;
      Generics::Task_var task;
      for (size_t i = 0; i < len; i++)
      {
        query.id_ = i;
        query.ref_ = &load_servers()[index][i];
        task = new Protected::CallTask<UpdateTriggers>(query);
        if (i != len - 1)
        {
          task_runner_->enqueue_task(task);
        }
      }
      task->execute();
      result = std::make_unique<ChannelUpdate::UpdateData>();
      UpdateTriggers::Handler::ResultsVector& results = query.callback_->get_result();
      if (query.callback_->count_exception() > 0)
      {
        Stream::Error ostr;
        bad_(index);
        ostr << __func__ << ": " << query.callback_->count_exception()
          << " ChannelServers are failed. : " << query.callback_->get_exception().str();
        throw ImplementationException(ostr.str());
      }
      try
      {
        result->source_id = -1;
        size_t old_length = 0;
        for (size_t i = 0; i < len; i++)
        {
          ChannelUpdate::UpdateDataPtr& value = results[i];
          if (value)
          {
            result->source_id = value->source_id;
            old_length += value->channels.size();
          }
        }
        result->channels.resize(old_length);
        old_length = 0;
        for (size_t i = 0; i < len; i++)
        {
          ChannelUpdate::UpdateDataPtr& value = results[i];
          if (value && value->channels.size())
          {
            std::move(value->channels.begin(), value->channels.end(),
              result->channels.begin() + old_length);
            old_length += value->channels.size();
          }
        }

        if (result->source_id < 0 && source_id() > 0)
        {
          result->source_id = source_id() + index;
        }
      }
      catch (const eh::Exception& e)
      {
        bad_(index);
        throw ImplementationException(e.what());
      }
    }
    catch (const eh::Exception& e)
    {
      Stream::Error ostr;
      bad_(index);
      ostr << __func__ << ": eh::Exception: " << e.what();
      throw ImplementationException(ostr.str());
    }
  }

  struct UpdatePosCCG:
    public Protected::BaseCaller<ChannelUpdate::PosCCGResultPtr, ChannelLoadDescription>
  {
    UpdatePosCCG(const char* name, size_t len);

    void execute();

    const ChannelUpdate::CCGQuery* in;
  };

  UpdatePosCCG::UpdatePosCCG(const char* name, size_t len)
    : Protected::BaseCaller<ChannelUpdate::PosCCGResultPtr, ChannelLoadDescription>(name, len)
  {
  }

  void UpdatePosCCG::execute()
  {
    ChannelUpdate::PosCCGResultPtr result;
    ref_->load_server->update_all_ccg(*in, result);
    callback_->set_result(id_, result);
  }

  void ChannelLoadSessionImpl::update_all_ccg(
    const ChannelUpdate::CCGQuery& query_in, ChannelUpdate::PosCCGResultPtr& result)
  {
    if (load_servers().empty())
    {
      throw NotConfigured("no ChannelUpdate sources");
    }

    size_t index = get_item_();
    try
    {
      size_t len = load_servers()[index].size();
      UpdatePosCCG query(__func__, len);
      query.in = &query_in;
      Generics::Task_var task;
      for (size_t i = 0; i < len; i++)
      {
        query.id_ = i;
        query.ref_ = &load_servers()[index][i];
        task = new Protected::CallTask<UpdatePosCCG>(query);
        if (i != len - 1)
        {
          task_runner_->enqueue_task(task);
        }
      }
      task->execute();
      result = std::make_unique<ChannelUpdate::PosCCGResult>();
      UpdatePosCCG::Handler::ResultsVector& results = query.callback_->get_result();

      if (query.callback_->count_exception() > 0)
      {
        Stream::Error ostr;
        bad_(index);
        ostr << __func__ << ": " << query.callback_->count_exception()
          << " ChannelServers are failed. : " << query.callback_->get_exception().str();
        throw ImplementationException(ostr.str());
      }
      size_t length_of_result[2] = {0, 0};
      unsigned long start_min = ULONG_MAX, start_max = query_in.start;
      bool use_min = false;
      result->source_id = -1;
      for (size_t i = 0; i < len; i++)
      {
        ChannelUpdate::PosCCGResultPtr& value = results[i];
        result->source_id = value->source_id;
        length_of_result[0] += value->keywords.size();
        length_of_result[1] += value->deleted.size();
        start_min = std::min(static_cast<unsigned long>(value->start_id), start_min);
        start_max = std::max(static_cast<unsigned long>(value->start_id), start_max);
        if (value->keywords.size() >= query_in.limit)
        {
          use_min = true;
        }
      }

      if (length_of_result[0] > query_in.limit)
      {
        std::multiset<unsigned int> channels;
        for (size_t i = 0; i < len; i++)
        {
          for (size_t j = 0; j < results[i]->keywords.size(); j++)
          {
            ChannelUpdate::CCGKeyword& kw = results[i]->keywords[j];
            channels.insert(kw.channel_id);
          }
        }
        auto ch_it = channels.begin();
        std::advance(ch_it, query_in.limit);
        result->start_id = *ch_it + 1;
      }
      else if (use_min)
      {
        result->start_id = start_min;
      }
      else
      {
        result->start_id = start_max;
      }

      result->keywords.resize(length_of_result[0]);
      result->deleted.resize(length_of_result[1]);
      length_of_result[0] = length_of_result[1] = 0;
      for (size_t i = 0; i < results.size(); i++)
      {
        for (size_t j = 0; j < results[i]->keywords.size(); j++)
        {
          ChannelUpdate::CCGKeyword& kw = results[i]->keywords[j];
          if (kw.channel_id < result->start_id)
          {
            result->keywords[length_of_result[0]++] = kw;
          }
          else
          {
            break;
          }
        }

        if (result->deleted.size())
        {
          std::copy(results[i]->deleted.begin(), results[i]->deleted.end(),
            result->deleted.begin() + length_of_result[1]);
          length_of_result[1] += results[i]->deleted.size();
        }
      }
      result->keywords.resize(length_of_result[0]);
      if (result->source_id < 0 && source_id() > 0)
      {
        result->source_id = source_id() + index;
      }
    }
    catch (const eh::Exception& e)
    {
      Stream::Error ostr;
      bad_(index);
      ostr << __func__ << ": eh::Exception: " << e.what();
      throw ImplementationException(ostr.str());
    }
  }

} // namespace AdServer::ChannelSvcs
