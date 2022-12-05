#include <list>
#include <vector>
#include <set>
#include <iterator>
#include <time.h>

#include <String/StringManip.hpp>
#include <Generics/GnuHashTable.hpp>
#include <Generics/HashTableAdapters.hpp>
#include <Generics/Statistics.hpp>
#include <Generics/TaskRunner.hpp>
#include <Logger/Logger.hpp>
#include <Logger/StreamLogger.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <ReferenceCounting/DefaultImpl.hpp>
#include <CORBACommons/CorbaAdapters.hpp>
#include <CORBACommons/ServantImpl.hpp>
#include <Commons/CorbaAlgs.hpp>

#include "ChannelSessionFactory.hpp"
#include "ChannelLoadSessionFactory.hpp"
#include "ThreadHandlerTemplate.hpp"


namespace AdServer
{
namespace ChannelSvcs
{

const char* TimedQueryStat::ASPECT =
  "TimedQueryStatistics";

NullStat* TimedQueryStat::CreateStatistic(
    Logging::Logger* logger,
    Generics::TaskRunner* task_runner,
    unsigned long dump_margin,
    const std::vector<size_t>& lengths)
    /*throw(eh::Exception)*/
{
  if(logger)
  {
    return new TimedQueryStat(logger, task_runner, dump_margin, lengths);
  }
  else
  {
    return new NullStat();
  }
}

TimedQueryStat::TimedQueryStat(
    Logging::Logger* logger,
    Generics::TaskRunner* task_runner,
    unsigned long dump_margin,
    const std::vector<size_t>& lengths)
    /*throw(InvalidArgument, eh::Exception)*/
  : logger_(ReferenceCounting::add_ref(logger)),
    task_runner_(task_runner),
    counter_(0),
    dump_margin_(dump_margin)
{
  if(!logger_)
  {
    throw InvalidArgument("TimedQueryStat::TimedQueryStat: logger is null");
  }
  Generics::Statistics::DumpRunner_var stat_runner;
  stat_runner = new Generics::Statistics::NullDumpRunner;
  policy_ = new Generics::Statistics::NullDumpPolicy;
  collection_ = new Generics::Statistics::Collection(stat_runner.in());
  for(std::vector<size_t>::const_iterator it = lengths.begin();
      it != lengths.end(); ++it)
  {
    for(unsigned long i = 0; i < *it; i++)
    {
      char name[64];
      collection_->add(
        generate_name(*it, i, name),
        new Generics::Statistics::TimedStatSink(),
        policy_.in());
    }
  }
}

const char* TimedQueryStat::generate_name(
  size_t index,
  size_t num,
  char name[64])
  noexcept
{
  Stream::Buffer<64/*sizeof(name)=8*/> ostr(name);
  ostr << index << ". " << num;
  return name;
}

void TimedQueryStat::prepare(Generics::Timer& timer)
{
  try
  {
    timer.start();
  }
  catch(...)
  {
  }
}

void TimedQueryStat::consider(size_t index, size_t num, Generics::Timer& timer)
  noexcept
{
  try
  {
    timer.stop();
    Generics::Time time = timer.elapsed_time();
    char name[64];
    try
    {
      Generics::Statistics::StatSink_var stat(
        collection_->get(generate_name(index, num, name)));
      stat->consider(Generics::Statistics::TimedSubject(time));
    }
    catch(const Generics::Statistics::Collection::StatItemNotFound& e)
    {
      //std::cout << "not found exception" <<std::endl;
    }
  }
  catch(...)
  {
  }
}

void TimedQueryStat::check() noexcept
{
  static const char* FN="TimedQueryStat::check";
  try
  {
    if(need_dump_())
    {
      Protected::Task_var task = new StatisticTask(this);
      task_runner_->enqueue_task(task);
    }
  }
  catch(...)
  {
    try
    {
      logger_->sstream(Logging::Logger::ERROR,
                       ASPECT)
        << FN << ": caught exception on checking statistic";
    }
    catch(...)
    {
    }
  }
}

void TimedQueryStat::dump() noexcept
{
  try
  {
    const char* header = "Match performance: ";
    std::ostringstream ostr;
    ostr << header;
    collection_->dump(ostr);
    const std::string& str = ostr.str();
    if (str.size() > strlen(header))
    {
      logger_->log(str, Logging::Logger::INFO, ASPECT);
    }
  }
  catch(...)
  {
  }
}

bool TimedQueryStat::need_dump_() noexcept
{
  WriteGuard_ lock(lock_);
  if(counter_++ > dump_margin_)
  {
    counter_ = 0;
    return true;
  }
  return false;
}

}
}

const char* ChannelServerSessionFactoryImpl::ASPECT =
  "ChannelServerSessionFactoryImpl";


namespace AdServer
{
namespace ChannelSvcs
{

  TimedQueryStat::StatisticTask::StatisticTask(
    TimedQueryStat* callback) noexcept
    : callback_(callback)
  {
  }

  void TimedQueryStat::StatisticTask::execute() noexcept
  {
    callback_->dump();
  }

} /* ChannelSvcs */
} /* AdServer */


ChannelServerSessionFactoryImpl::ChannelServerSessionFactoryImpl(
    Logging::Logger* stat_logger,
    unsigned long count_threads,
    Generics::ActiveObjectCallback* callback,
    unsigned long check_period) noexcept
  : stat_logger_(ReferenceCounting::add_ref(stat_logger)),
    callback_(ReferenceCounting::add_ref(callback)),
    task_runner_(new Generics::TaskRunner(callback_, count_threads)),
    check_period_(check_period)
{
  try
  {
    add_child_object(task_runner_);
    activate_object();
  }
  catch(const eh::Exception& e)
  {
    if(callback_)
    {
      Stream::Error ostr;
      ostr << "ChannelServerSessionFactoryImpl::"
        "ChannelServerSessionFactoryImpl: "
        "Catch eh::Exception on activating task_runner. "
        ": " << e.what();
      callback_->critical(ostr.str(), "ADS-IMPL-600");
    }
  }
  catch(...)
  {
    if(callback_)
    {
      callback_->critical(
        String::SubString(
        "ChannelServerSessionFactoryImpl::ChannelServerSessionFactoryImpl: "
        "Catch exception on activating task_runner"), "ADS-IMPL-600");
    }
  }
}

ChannelServerSessionFactoryImpl::~ChannelServerSessionFactoryImpl()
  noexcept
{
    deactivate_object();
    wait_object();
}

CORBA::ValueBase* ChannelServerSessionFactoryImpl::create_for_unmarshal()
{
  try
  {
    return new ::AdServer::ChannelSvcs::ChannelServerSessionImpl(
      callback_,
      stat_logger_.in(), task_runner_.in(), check_period_);
  }
  catch(const eh::Exception& e)
  {
    if(callback_)
    {
      Stream::Error ostr;
      ostr << "ChannelServerSessionFactoryImpl::create_for_unmarshal: "
        "Catch eh::Exception on creating ChannelServerSessionImpl. "
        ": " << e.what();
      callback_->critical(ostr.str());
    }
    throw;
  }
}


namespace AdServer
{
namespace ChannelSvcs
{

  const char* ChannelServerSessionImpl::ASPECT = "ChannelServerSessionImpl";
  const unsigned long ChannelServerSessionImpl::stat_dump_margin = 10;

    /**
     * ChannelServerSessionImpl
     */
  ChannelServerSessionImpl::ChannelServerSessionImpl(
    Generics::ActiveObjectCallback* callback,
    Logging::Logger* stat_logger,
    Generics::TaskRunner* runner,
    unsigned long check_period)
    /*throw(eh::Exception)*/
    : callback_(ReferenceCounting::add_ref(callback)),
      stat_logger_(ReferenceCounting::add_ref(stat_logger)),
      task_runner_(ReferenceCounting::add_ref(runner)),
      check_period_(check_period),
      stat_()
  {
  }

  ChannelServerSessionImpl::ChannelServerSessionImpl(
      ChannelServerSessionImpl& init)
      /*throw(Exception)*/:
      CORBA::ValueBase(),
      CORBA::AbstractBase(),
      AdServer::ChannelSvcs::ChannelServerBase(),
      AdServer::ChannelSvcs::ChannelServerSession(),
      CORBA::DefaultValueRefCountBase(),
      OBV_AdServer::ChannelSvcs::ChannelServerSession(),
      callback_(init.callback_),
      stat_logger_(init.stat_logger_),
      task_runner_(init.task_runner_),
      check_period_(init.check_period_),
      stat_()
  {
    static const char* FN="ChannelServerSessionImpl::ChannelServerSessionImpl";
    try
    {
      channel_servers(init.channel_servers());
    }
    catch(...)
    {
      Stream::Error ostr;
      ostr << FN <<
        ": Catch exception on initialization session";
      throw Exception(ostr);
    }
  }

  ChannelServerSessionImpl::ChannelServerSessionImpl(
      const AdServer::ChannelSvcs::Protected::GroupDescriptionSeq&
        servers) /*throw(Exception)*/:
      callback_()
  {
    static const char* FN="ChannelServerSessionImpl::ChannelServerSessionImpl";
    try
    {
      channel_servers(servers);
    }
    catch(...)
    {
      Stream::Error ostr;
      ostr << FN << "Catch exception on initialization session";
      throw Exception(ostr);
    }
  }

  ChannelServerSessionImpl::~ChannelServerSessionImpl() noexcept
  {
  }

  CORBA::ValueBase* ChannelServerSessionImpl::_copy_value()
  {
    try
    {
      return new AdServer::ChannelSvcs::ChannelServerSessionImpl(*this);
    }
    catch(const Exception& e)
    {
      if(callback_)
      {
        Stream::Error ostr;
        ostr << "ChannelServerSessionImpl::_copy_value: "
          "Catch Exception on creating ChannelServerSessionImp. : "
          << e.what();
        callback_->critical(ostr.str());
      }
    }
    catch(const eh::Exception& e)
    {
      if(callback_)
      {
        Stream::Error ostr;
        ostr << "ChannelServerSessionImpl::_copy_value: "
          "Catch eh::Exception on creating ChannelServerSessionImp. "
          ": " << e.what();
        callback_->critical(ostr.str());
      }
      throw;
    }
    return 0;
  }

  struct Match:
    public Protected::BaseCaller<
      ChannelServer::MatchResult_var,
      ChannelSvcs::ChannelServer>
  {
    Match(const char* name, size_t index, size_t len) /*throw(eh::Exception)*/
      : Protected::BaseCaller<
          ChannelServer::MatchResult_var,
          ChannelSvcs::ChannelServer> (name, len),
        index_(index)
    {};

    void execute()
      /*throw(AdServer::ChannelSvcs::NotConfigured,
        AdServer::ChannelSvcs::ImplementationException,
        CORBA::SystemException)*/;

    const ::AdServer::ChannelSvcs::ChannelServerBase::MatchQuery* in_;
    size_t index_;
    NullStat* stat_;
  };

  void
  Match::execute()
    /*throw(AdServer::ChannelSvcs::NotConfigured,
      AdServer::ChannelSvcs::ImplementationException,
      CORBA::SystemException)*/
  {
    AdServer::ChannelSvcs::ChannelServer::MatchResult_var res;
    Generics::Timer timer;
    stat_->prepare(timer);
    ref_->match(*in_, res);
    stat_->consider(index_, id_, timer);
    callback_->set_result(id_, res);
  }

  void ChannelServerSessionImpl::init_stat_() noexcept
  {
    static const char* FN = "ChannelServerSessionImpl::init_stat_";
    if(!stat_.get())
    {
      WriteGuard_ lock(init_lock_);
      if(stat_.get())
      {
        return;
      }
      try
      {
        std::vector<size_t> lengths(channel_servers().length());
        for(size_t i = 0; i < channel_servers().length(); i++)
        {
          lengths[i] = channel_servers()[i].length();
        }
        stat_.reset(
          AdServer::ChannelSvcs::TimedQueryStat::CreateStatistic(
            stat_logger_, task_runner_, stat_dump_margin, lengths));
      }
      catch(const eh::Exception& e)
      {
        Stream::Error ostr;
        ostr << FN << ": Caught eh::Exception on init statistic. "
          ": " << e.what();
        if(callback_)
        {
          callback_->error(ostr.str());
        }
      }
    }
  }

  ChannelServerSessionImpl::GroupPool::ObjectHandlerType
  ChannelServerSessionImpl::get_item_() /*throw(GroupPool::Exception)*/
  {
    if(!pool_.get())
    {
      WriteGuard_ lock(init_lock_);
      if(!pool_.get())
      {
        GroupPoolConfiguration conf(
          channel_servers(), Generics::Time(check_period_));
        pool_.reset(
          new GroupPool(conf,
          CORBACommons::ChoosePolicyType::PT_LOOP));
      }
    }
    return pool_->get_object();
  }

  //
  // IDL:AdServer/ChannelSvcs/ChannelServer/match:1.0
  //
  void
  ChannelServerSessionImpl::match(
    const ::AdServer::ChannelSvcs::ChannelServerBase::MatchQuery& query,
    ::AdServer::ChannelSvcs::ChannelServerBase::MatchResult_out result)
    /*throw(AdServer::ChannelSvcs::ImplementationException,
      AdServer::ChannelSvcs::NotConfigured)*/
  {
    if(!channel_servers().length())
    {
      throw AdServer::ChannelSvcs::NotConfigured();
    }

    try
    {
      GroupPool::ObjectHandlerType handler = get_item_();
      CORBA::ULong index = *handler - 1;
      try
      {
        init_stat_();

        size_t len = channel_servers()[index].length();
        Match mquery(__func__, index, len);
        mquery.in_ = &query;
        mquery.stat_ = stat_.get();
        Generics::Task_var task;
        for(size_t i = 0; i < len; i++)
        {
          mquery.id_ = i;
          mquery.ref_ = channel_servers()[index][i].channel_server;
          task = new Protected::CallTask<Match>(mquery);
          if(i != len - 1)
          {
            task_runner_->enqueue_task(task);
          }
        }
        task->execute();
        result = new AdServer::ChannelSvcs::ChannelServerBase::MatchResult;
        result->no_adv = result->no_track = 0;
        result->match_time.length(len);

        Match::Handler::ResultsVector& results =
          mquery.callback_->get_result();
        for(size_t i = 0; i < len; i++)
        {
          try
          {
            ChannelSvcs::ChannelServer::MatchResult_var& value = results[i];
            if(value)
            {
              result->no_adv |= value->no_adv;
              result->no_track |= value->no_track;
              result->match_time[i] = value->match_time;
            }
            else
            {
              value = new ChannelSvcs::ChannelServer::MatchResult;
              result->match_time[i] =
                CorbaAlgs::pack_time(Generics::Time::ZERO);
            }
          }
          catch(const Match::Handler::Exception& e)
          {
            Stream::Error ostr;
            ostr << __func__ << "Internal error in Match::Handler: " << e.what();
            if(callback_)
            {
              callback_->error(ostr.str());
            }
            CORBACommons::throw_desc<ImplementationException>(ostr.str());
          }
        }

        if(callback_ && mquery.callback_->count_exception() > 0)
        {
          Stream::Error ostr;
          handler.release_bad();
          if(mquery.callback_->count_exception() == len)
          {
            // throw it farther, all servers are failed
            ostr << "All ChannelServers are failed: "
              << mquery.callback_->get_exception().str();
            CORBACommons::throw_desc<ImplementationException>(ostr.str());
          }
          ostr << __func__ << ": "
            "Communication is failed with some channel servers: " <<
            mquery.callback_->get_exception().str();
          callback_->error(ostr.str());
        }
        compose_result_(results, result);
        merge_content_(results, result);
        stat_->check();
      }
      catch(const eh::Exception& e)
      {
        Stream::Error ostr;
        ostr << __func__ << ": eh::Exception: " << e.what();
        CORBACommons::throw_desc<ImplementationException>(ostr.str());
      }
      catch(const AdServer::ChannelSvcs::NotConfigured& e)
      {
        Stream::Error ostr;
        handler.release_bad();
        ostr << __func__ << ": NotConfigured. ";
        CORBACommons::throw_desc<ImplementationException>(ostr.str());
      }
      catch(const AdServer::ChannelSvcs::ImplementationException& e)
      {
        Stream::Error ostr;
        handler.release_bad();
        ostr << __func__ << ": ChannelSvcs::ImplementationException: " <<
          e.description;
        CORBACommons::throw_desc<ImplementationException>(ostr.str());
      }
      catch(const CORBA::SystemException& e)
      {
        Stream::Error ostr;
        handler.release_bad();
        ostr << __func__ << ": CORBA::SystemException: " << e;
        CORBACommons::throw_desc<ImplementationException>(ostr.str());
      }
    }
    catch(const GroupPool::Exception& e)
    {
      Stream::Error ostr;
      ostr << __func__ << ": GroupPool::Exception: " << e.what();
      CORBACommons::throw_desc<ImplementationException>(ostr.str());
    }
  }

  struct GetCCGTraits:
    public Protected::BaseCaller<
      ChannelServer::TraitsResult_var,
      ChannelSvcs::ChannelServer>
  {
    GetCCGTraits(const char* name, size_t len) /*throw(eh::Exception)*/
      : Protected::BaseCaller<
          ChannelServer::TraitsResult_var,
          ChannelSvcs::ChannelServer>(name, len)
    {};

    void execute()
      /*throw(AdServer::ChannelSvcs::ImplementationException,
        AdServer::ChannelSvcs::NotConfigured,
        CORBA::SystemException)*/;

    bool operator() (const ChannelServerBase::CCGKeyword& kw) noexcept;

    const ::AdServer::ChannelSvcs::ChannelIdSeq* ids;
  };

  void
  GetCCGTraits::execute()
    /*throw(AdServer::ChannelSvcs::ImplementationException,
      AdServer::ChannelSvcs::NotConfigured,
      CORBA::SystemException)*/
  {
    ChannelServer::TraitsResult_var res;
    ref_->get_ccg_traits(*ids, res);
    callback_->set_result(id_, res);
  }

  bool GetCCGTraits::operator()(const ChannelServerBase::CCGKeyword& kw) noexcept
  {
    GetCCGTraits::Handler::ResultsVector& results = callback_->results();
    for(size_t i = 0; i < results.size(); i++)
    {
      if(results[i])
      {
        if(std::find(
            results[i]->neg_ccg.get_buffer(),
            results[i]->neg_ccg.get_buffer() + results[i]->neg_ccg.length(),
            kw.ccg_id) !=
          results[i]->neg_ccg.get_buffer() + results[i]->neg_ccg.length())
        {
          return true;
        }
      }
    }
    return false;
  }
  
  ChannelServerBase::CCGKeywordSeq*
  ChannelServerSessionImpl::get_ccg_traits(
    const ::AdServer::ChannelSvcs::ChannelIdSeq& ids)
    /*throw(AdServer::ChannelSvcs::ImplementationException,
      AdServer::ChannelSvcs::NotConfigured)*/
  {
    if(channel_servers().length() == 0)
    {
      throw AdServer::ChannelSvcs::NotConfigured();
    }
    try
    {
      GroupPool::ObjectHandlerType handler = get_item_();
      CORBA::ULong index = *handler - 1;

      try
      {
        size_t len = channel_servers()[index].length();
        GetCCGTraits query(__func__, len);
        query.ids = &ids;
        Generics::Task_var task;
        for(size_t i = 0; i < len; i++)
        {
          query.id_ = i;
          query.ref_ = channel_servers()[index][i].channel_server;
          task = new Protected::CallTask<GetCCGTraits>(query);
          if(i != len - 1)
          {
            task_runner_->enqueue_task(task);
          }
        }
        task->execute();
        ChannelServerBase::CCGKeywordSeq_var result =
          new AdServer::ChannelSvcs::ChannelServerBase::CCGKeywordSeq;
        if(query.callback_->count_exception() > 0)
        {
          Stream::Error ostr;
          handler.release_bad();
          if(query.callback_->count_exception() == len)
          {//throw it farther, all servers are failed
            ostr << "All ChannelServers are failed. : "
              << query.callback_->get_exception().str();
            CORBACommons::throw_desc<ImplementationException>(ostr.str());
          }
          if(callback_)
          {
            ostr << __func__ << "Communication is failed with some channel servers. "
              ": " << query.callback_->get_exception().str();
            callback_->error(ostr.str());
          }
        }
        size_t count = 0;
        GetCCGTraits::Handler::ResultsVector& results =
          query.callback_->get_result();
        for(size_t i = 0; i < len; i++)
        {
          ChannelServer::TraitsResult_var& value = results[i];
          if(value)
          {
            count += value->ccg_keywords.length();
          }
        }
        result->length(count);
        CCGKeyword* last = result->get_buffer();
        for(size_t i = 0; i < results.size(); i++)
        {
          if(results[i])
          {
            last = std::remove_copy_if(
              results[i]->ccg_keywords.get_buffer(),
              results[i]->ccg_keywords.get_buffer() +
              results[i]->ccg_keywords.length(),
              last,
              query);
          }
        }
        result->length(last - result->get_buffer());
        return result._retn();
      }
      catch(const eh::Exception& e)
      {
        Stream::Error ostr;
        ostr << __func__ << " eh::Exception: " << e.what();
        CORBACommons::throw_desc<ImplementationException>(ostr.str());
      }
      catch(const AdServer::ChannelSvcs::NotConfigured& e)
      {
        Stream::Error ostr;
        handler.release_bad();
        ostr << __func__ << " NotConfigured";
        CORBACommons::throw_desc<ImplementationException>(ostr.str());
      }
      catch(const AdServer::ChannelSvcs::ImplementationException& e)
      {
        Stream::Error ostr;
        handler.release_bad();
        ostr << __func__ << " ImplementationException: " << e.description;
        CORBACommons::throw_desc<ImplementationException>(ostr.str());
      }
      catch(const CORBA::SystemException& e)
      {
        Stream::Error ostr;
        handler.release_bad();
        ostr << __func__ << " CORBA::SystemException: " << e;
        CORBACommons::throw_desc<ImplementationException>(ostr.str());
      }
    }
    catch(const GroupPool::Exception& e)
    {
      Stream::Error ostr;
      ostr << __func__ << ": GroupPool::Exception: " << e.what();
      CORBACommons::throw_desc<ImplementationException>(ostr.str());
    }
    return 0; // never reach
  }

  void ChannelServerSessionImpl::compose_result_(
    ResultsVector& results,
    ::AdServer::ChannelSvcs::ChannelServerBase::MatchResult* out)
    /*throw(eh::Exception)*/
  {
    size_t page_channels_length = 0;
    size_t search_channels_length = 0;
    size_t url_channels_length = 0;
    size_t url_keywords_channels_length = 0;
    size_t uid_channels_length = 0;
    for(size_t i = 0; i < results.size(); i++)
    {
      const ChannelSvcs::ChannelServerBase::TriggerMatchResult& cur_value =
        results[i]->matched_channels;
      page_channels_length += cur_value.page_channels.length();
      search_channels_length += cur_value.search_channels.length();
      url_channels_length += cur_value.url_channels.length();
      url_keywords_channels_length += cur_value.url_keyword_channels.length();
      uid_channels_length += cur_value.uid_channels.length();
    }
    out->matched_channels.page_channels.length(page_channels_length);
    out->matched_channels.search_channels.length(search_channels_length);
    out->matched_channels.url_channels.length(url_channels_length);
    out->matched_channels.url_keyword_channels.length(url_keywords_channels_length);
    out->matched_channels.uid_channels.length(uid_channels_length);
    page_channels_length = 0;
    search_channels_length = 0;
    url_channels_length = 0;
    url_keywords_channels_length = 0;
    uid_channels_length = 0;
    for(size_t i = 0; i < results.size(); i++)
    {
      const ChannelSvcs::ChannelServerBase::TriggerMatchResult& cur_value =
        results[i]->matched_channels;
      std::copy(
        cur_value.page_channels.get_buffer(),
        cur_value.page_channels.get_buffer() +
        cur_value.page_channels.length(),
        out->matched_channels.page_channels.get_buffer() +
        page_channels_length);
      std::copy(
        cur_value.search_channels.get_buffer(),
        cur_value.search_channels.get_buffer() +
        cur_value.search_channels.length(),
        out->matched_channels.search_channels.get_buffer() +
        search_channels_length);
      std::copy(
        cur_value.url_channels.get_buffer(),
        cur_value.url_channels.get_buffer() +
        cur_value.url_channels.length(),
        out->matched_channels.url_channels.get_buffer() +
        url_channels_length);
      std::copy(
        cur_value.url_keyword_channels.get_buffer(),
        cur_value.url_keyword_channels.get_buffer() +
        cur_value.url_keyword_channels.length(),
        out->matched_channels.url_keyword_channels.get_buffer() +
        url_keywords_channels_length);
      std::copy(
        cur_value.uid_channels.get_buffer(),
        cur_value.uid_channels.get_buffer() +
        cur_value.uid_channels.length(),
        out->matched_channels.uid_channels.get_buffer() +
        uid_channels_length);
      page_channels_length += cur_value.page_channels.length();
      search_channels_length += cur_value.search_channels.length();
      url_channels_length += cur_value.url_channels.length();
      url_keywords_channels_length += cur_value.url_keyword_channels.length();
      uid_channels_length += cur_value.uid_channels.length();
    }
  }

  template<class INDEX, typename ELEM >
  ChannelServerSessionImpl::ResultsAdapter<INDEX, ELEM>::ResultsAdapter(
    ResultsVector& results)
    /*throw(eh::Exception)*/
    : results_(results),
      sort_off_(results.size(), 0),
      all_length_(0)
  {
    create_index_();
  }
  
  template<class INDEX, typename ELEM >
  size_t ChannelServerSessionImpl::ResultsAdapter<INDEX, ELEM>::size() const
    noexcept
  {
    return all_length_;
  }

  template<class INDEX, typename ELEM >
  void ChannelServerSessionImpl::ResultsAdapter<INDEX, ELEM>::create_index_()
    /*throw(eh::Exception)*/
  {
    ListIndex::iterator li;
    for(size_t i = 0; i < results_.size(); i++)
    {
      size_t length = index_.size(results_[i]);
      if(length)
      {
        all_length_ += length;
        li = sort_index_.begin();
        while(li != sort_index_.end() &&
              index_.id(*results_[i]) > index_.id(*results_[*li]))
        {
          ++li;
        }
        sort_index_.insert(li, i);
      }
    }
  }

  template<class INDEX, typename ELEM>
  const ELEM&
  ChannelServerSessionImpl::ResultsAdapter<INDEX, ELEM>::elem() const
    /*throw(BadParameter)*/
  {
    if(empty())
    {
      throw BadParameter("list is empty");
    }
    ListIndex::const_iterator li = sort_index_.begin();
    return index_.elem(*results_[*li], sort_off_[*li]);
  }

  template<class INDEX, typename ELEM>
  void ChannelServerSessionImpl::ResultsAdapter<INDEX, ELEM>::next()
    noexcept
  {
    ListIndex::iterator sli, tli, li = sort_index_.begin();
    sort_off_[*li]++;
    if(sort_off_[*li] >= index_.size(results_[*li]))
    {
      sort_index_.erase(li);
      return;
    }
    sli = li;
    ++sli;
    if(sli == sort_index_.end())
    {
      return;
    }
    unsigned long id = index_.id(results_[*li], sort_off_[*li]);
    for(tli = sli; tli != sort_index_.end(); ++tli)
    {
      if(id <= index_.id(results_[*tli], sort_off_[*tli]))
      {
        break;
      }
    }
    if(tli != sli)
    {
      sort_index_.splice(tli, sort_index_, li);
    }
  }

  void ChannelServerSessionImpl::merge_content_(
    ResultsVector& results,
    ::AdServer::ChannelSvcs::ChannelServerBase::MatchResult* out)
    /*throw(eh::Exception)*/
  {
    if(results.empty())
    {
      return;
    }
    size_t out_index = 0;

    ResultsAdapter<
      ContentIndex,
      ChannelSvcs::ChannelServerBase::ContentChannelAtom> cont_adapter(results);
    out->content_channels.length(cont_adapter.size());
    if(!out->content_channels.length())
    {
      return;
    }
    //
    out->content_channels[0].id = cont_adapter.elem().id - 1;
    while(!cont_adapter.empty())
    {
      CORBA::ULong id =  cont_adapter.elem().id;
      if(id == out->content_channels[out_index].id)
      {
        out->content_channels[out_index].weight += cont_adapter.elem().weight;
      }
      else
      {
        out->content_channels[out_index] = cont_adapter.elem();
        out_index++;
      }
      cont_adapter.next(); 
    }
    out->content_channels.length(out_index);
  }

  void ChannelServerSessionImpl::report_error(
      Generics::ActiveObjectCallback::Severity severity,
      const char* description, const char* error_code) noexcept
  {
    if(callback_)
    {
      Stream::Error ostr;
      ostr << "ChannelServerSessionImpl::report_error: : "
           << description;
      
      callback_->report_error(severity, ostr.str(), error_code);
    }
  }

  ChannelServerSessionImpl::GroupPoolConfiguration::GroupPoolConfiguration(
    const Protected::GroupDescriptionSeq& groups,
    const Generics::Time& period)
    noexcept
  {
    timeout = period;
    all_bad_no_wait = true;
    for(CORBA::ULong i = 0; i < groups.length(); i++)
    {
      iors_list.push_back(RefAndNumber(i + 1));
    }
  }

}// namespace ChannelSvcs
}// namespace AdServer

void ChannelServerSessionFactoryImpl::report_error(
    Generics::ActiveObjectCallback::Severity severity,
    const char* description,
    const char* error_code) noexcept
{
  if(callback_)
  {
    Stream::Error ostr;
    ostr << "ChannelServerSessionFactoryImpl::report_error: : "
         << description;
    
    callback_->report_error(severity, ostr.str(), error_code);
  }
}

