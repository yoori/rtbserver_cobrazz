
#include <unistd.h>
#include <getopt.h>
#include <limits.h>
#include <sstream>

#include <Generics/ActiveObject.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/TaskRunner.hpp>
#include <Generics/Rand.hpp>
#include <Generics/Time.hpp>
#include <Sync/SyncPolicy.hpp>
#include <CORBACommons/CorbaAdapters.hpp>
#include"DummyChannelServer.hpp"

const char CHANNEL_SERVER_OBJ_KEY[] = "ChannelServer";

namespace AdServer
{
  namespace UnitTests
  {
    DummyChannelServer::DummyChannelServer(
      CORBACommons::OrbShutdowner* shutdowner,
      unsigned long time,
      unsigned long count_matching,
      bool verbose)
      noexcept
      : verbose_(verbose),
        count_matching_(count_matching),
        shutdowner_(ReferenceCounting::add_ref(shutdowner))
    {
      if(time)
      {
        scheduler_ = new Generics::Planner(this);
        task_runner_ = new Generics::TaskRunner(this, 1);
        Task_var msg = new FinishTask(this, task_runner_);
        Generics::Time tm = Generics::Time::get_time_of_day() + time;
        scheduler_->schedule(msg, tm);
      }
      while(response_info_.size() < count_matching_)
      {
        response_info_.insert(Generics::safe_rand());
      }
    }
    
    void DummyChannelServer::set_sources(
        const ::AdServer::ChannelSvcs::
          ChannelServerControl::DBSourceInfo& /*db_info*/,
        const ::AdServer::ChannelSvcs::ChunkKeySeq& /*sources*/)
      noexcept
    {
      return;
    }

    void DummyChannelServer::set_proxy_sources(
      const ::AdServer::ChannelSvcs::
        ChannelServerControl::ProxySourceInfo& /*proxy_info*/,
      const ::AdServer::ChannelSvcs::ChunkKeySeq& /*sources*/)
      noexcept
    {
      return;
    }

    ::CORBA::ULong DummyChannelServer::get_queries_counter() noexcept
    {
      return 12;
    }

    ::CORBA::ULong get_count_chunks()
      /*throw(AdServer::ChannelSvcs::ImplementationException)*/
    {
      throw AdServer::ChannelSvcs::ImplementationException(
        "DummyChannelServer doesn't support this");
    }

    void DummyChannelServer::update_triggers(
      const ::AdServer::ChannelSvcs::ChannelIdSeq& /*ids*/,
      ::AdServer::ChannelSvcs::ChannelCurrent::UpdateData_out /*result*/)
      /*throw(AdServer::ChannelSvcs::ImplementationException)*/
    {
      throw AdServer::ChannelSvcs::ImplementationException(
        "DummyChannelServer doesn't support this");
    }

    void DummyChannelServer::match(
        const ::AdServer::ChannelSvcs::ChannelServerBase::MatchQuery& /*query*/,
      ::AdServer::ChannelSvcs::ChannelServer::MatchResult_out result)
      /*throw(AdServer::ChannelSvcs::ImplementationException)*/
    {
      try
      {
        Generics::Timer timer;
        timer.start();
        result = new ::AdServer::ChannelSvcs::ChannelServer::MatchResult;

        result->no_adv = 0;
        result->no_track = 0;
        TriggerSet::const_iterator word = response_info_.begin();
        result->matched_channels.page_channels.length(count_matching_);
        for(size_t i = 0; i < count_matching_; i++, word++)
        {
          ::AdServer::ChannelSvcs::ChannelServerBase::ChannelAtom& out =
            result->matched_channels.page_channels[i];

          out.id = i + 10;
          out.trigger_channel_id = *word;
        }
        timer.stop();
        if(verbose_)
        {
          std::cout << "Match takes " << timer.elapsed_time() << std::endl;
        }
      }
      catch(const eh::Exception& e)
      {
        throw AdServer::ChannelSvcs::ImplementationException(
          e.what());
      }
    }

    void DummyChannelServer::get_ccg_traits(
      const ::AdServer::ChannelSvcs::ChannelIdSeq& /*query*/,
      ::AdServer::ChannelSvcs::ChannelServer::TraitsResult_out /*result*/)
    /*throw(AdServer::ChannelSvcs::ImplementationException,
          AdServer::ChannelSvcs::NotConfigured)*/
    {
      throw AdServer::ChannelSvcs::ImplementationException(
        "Not implemented");
    }

    void DummyChannelServer::activate_object()
      /*throw(
        Generics::ActiveObject::AlreadyActive,
        Generics::ActiveObject::Exception,
        eh::Exception)*/
    {
      try
      {
        if(task_runner_) task_runner_->activate_object();
        if(scheduler_) scheduler_->activate_object();
      }
      catch(...)
      {
        if(scheduler_ && scheduler_->active())
        {
          scheduler_->deactivate_object();
        }
        if(task_runner_ && task_runner_->active())
        {
          task_runner_->deactivate_object();
        }
        throw;
      }
      active_ = true;
    }

    void DummyChannelServer::deactivate_object()
      /*throw(Generics::ActiveObject::Exception, eh::Exception)*/
    {
      if(scheduler_ && scheduler_->active())
      {
        scheduler_->deactivate_object();
      }
      if(task_runner_ && task_runner_->active())
      {
        task_runner_->deactivate_object();
      }
      active_ = false;
    }

    void DummyChannelServer::wait_object()
      /*throw(Generics::ActiveObject::Exception, eh::Exception)*/
    {
      if(scheduler_)
      {
        scheduler_->wait_object();
        scheduler_.reset();
      }
      if(task_runner_)
      {
        task_runner_->wait_object();
        task_runner_.reset();
      }
    }

    void DummyChannelServer::report_error(
      Generics::ActiveObjectCallback::Severity /*severity*/,
      const String::SubString& description,
      const char* /*error_code*/) noexcept
    {
      std::cerr << description << std::endl;
    }

    void DummyChannelServer::finish() noexcept
    {
      shutdowner_->shutdown(true);
      shutdowner_.reset();
    }

  }
}

template<class T>
void read_number_(
  const char* in,
  const T& min_value,
  const T& max_value,
  T &value) noexcept
{
  T converted;
  std::stringstream convert;
  convert << in;
  convert >> converted;
  if(converted>=min_value && converted<=max_value)
  {
    value = converted;
  }
}

int main(int argc, char* argv[])
{
  struct option long_options[] = 
  {
    {"port", required_argument, 0, 'p'},
    {"time", required_argument, 0, 't'},
    {"threads", required_argument, 0, 'h'},
    {"verbose", required_argument, 0, 'v'},
    {0, 0, 0, 0}
  };
  int port = 10103;
  unsigned long work_time = 120;
  unsigned long count_threads = 30, count_matching = 0;
  int opt, index=0;
  bool verbose = false;
  do
  {
    opt = getopt_long(argc, argv, "p:t:h:m:v",
      long_options, &index);
    switch(opt)
    {
      case 'p':
        read_number_(optarg, 1025, 65535, port);
        break;
      case 't':
        read_number_(optarg, 0UL, ULONG_MAX, work_time);
        break;
      case 'h':
        read_number_(optarg, 1UL, 65535UL, count_threads);
        break;
      case 'm':
        read_number_(optarg, 0UL, 65535UL, count_matching);
        break;
      case 'v':
        verbose = true;
        break;
    }
  } while(opt!=-1);
  try
  {
    CORBACommons::CorbaConfig config;
    config.thread_pool = count_threads;
    {
      CORBACommons::EndpointConfig endpoint;
      endpoint.host = "*";
      endpoint.ior_names = "*";
      endpoint.port = port;
      endpoint.objects[CHANNEL_SERVER_OBJ_KEY].insert(CHANNEL_SERVER_OBJ_KEY);
      config.endpoints.push_back(endpoint);
    }
    CORBACommons::CorbaServerAdapter_var corba_server_adapter(
      new CORBACommons::CorbaServerAdapter(config));

    CORBACommons::OrbShutdowner_var shutdowner =
      corba_server_adapter->shutdowner();

    AdServer::UnitTests::DummyChannelServer_var server =
      new AdServer::UnitTests::DummyChannelServer(
        shutdowner.in(),
        work_time,
        count_matching,
        verbose);

    corba_server_adapter->add_binding(
      CHANNEL_SERVER_OBJ_KEY, server.in());

    server->activate_object();

    // Running orb loop
    corba_server_adapter->run();

    server->deactivate_object();
    server->wait_object();

    server.reset();

    shutdowner.reset();

    corba_server_adapter.reset();

  }
  catch(...)
  {
    return 1;
  }
  return 0;
}

