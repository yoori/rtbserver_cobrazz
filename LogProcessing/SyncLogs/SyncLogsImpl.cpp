
#include <string>
#include <sstream>

#include <eh/Exception.hpp>
#include <Generics/Time.hpp>
#include <Generics/Rand.hpp>
#include <String/BasicAnalyzer.hpp>
#include <String/Analyzer.hpp>
#include <Stream/MemoryStream.hpp>

#include <Commons/Algs.hpp>

#include "RouteProcessor.hpp"
#include "FeedRouteProcessor.hpp"
#include "FetchRouteProcessor.hpp"

#include "SyncLogsImpl.hpp"

namespace AdServer
{
namespace LogProcessing
{
  namespace Aspect
  {
    const char SYNC_LOGS[] = "SyncLogs";
  }

  /**
   * AdServer::LogProcessing::SyncLogsImpl class
   */
  SyncLogsImpl::SyncLogsImpl(
    Generics::ActiveObjectCallback *callback,
    Logging::Logger* logger,
    const Configuration &config)
    /*throw(Exception, eh::Exception)*/
    : callback_(ReferenceCounting::add_ref(callback)),
      logger_(ReferenceCounting::add_ref(logger)),
      error_logger_(1024, logger),
      configuration_(new Configuration(config)),
      host_checker_(configuration_->hostname()),
      task_runner_(),
      scheduler_(new Generics::Planner(callback_))
  {
    static const char* FUN = "SyncLogsImpl::SyncLogsImpl()";

    if (callback == 0)
    {
      throw Exception(
        "AdServer::LogProcessing::SyncLogsImpl::SyncLogsImpl(): "
        "callback == 0");
    }

    typedef xsd::AdServer::Configuration::ClusterConfigType ClusterConfig;
    const ClusterConfig& cluster_config = configuration_->ClusterConfig();

    if(cluster_config.root_logs_dir().present())
    {
      std::string root_dir = *(cluster_config.root_logs_dir());

      if(root_dir.empty() || root_dir[0] != '/')
      {
        Stream::Error ostr;
        ostr << FUN << ": "
          "Logs root directory should start from '/'.";
        throw Exception(ostr);
      }
    }

    {
      size_t num_routes = 0;

      typedef ClusterConfig::FeedRouteGroup_sequence FeedRoutes;
      const FeedRoutes& feed_routes = cluster_config.FeedRouteGroup();

      for (FeedRoutes::const_iterator feed_it = feed_routes.begin();
        feed_it != feed_routes.end(); ++feed_it)
      {
        num_routes += feed_it->Route().size();
      }

      typedef ClusterConfig::FetchRouteGroup_sequence FetchRoutes;
      const FetchRoutes& fetch_routes = cluster_config.FetchRouteGroup();

      for (FetchRoutes::const_iterator fetch_it = fetch_routes.begin();
        fetch_it != fetch_routes.end(); ++fetch_it)
      {
        num_routes += fetch_it->Route().size();
      }

      task_runner_ = new Generics::TaskRunner(callback_, num_routes);
    }

    {
      /* create feed processors */
      typedef ClusterConfig::FeedRouteGroup_sequence FeedRouteGroups;
      typedef xsd::AdServer::Configuration::FeedRouteGroupType::Route_sequence Routes;

      const FeedRouteGroups& feed_route_groups = cluster_config.FeedRouteGroup();

      for(FeedRouteGroups::const_iterator feed_route_group_it =
            feed_route_groups.begin();
          feed_route_group_it != feed_route_groups.end(); ++feed_route_group_it)
      {
        const Routes& routes = feed_route_group_it->Route();
        unsigned long pool_threads = feed_route_group_it->pool_threads();
        Generics::TaskRunner_var pool_task_runner;
        if(pool_threads)
        {
          pool_task_runner = new Generics::TaskRunner(callback_, pool_threads, 1);
        }

        unsigned long check_period = feed_route_group_it->check_logs_period().present() ?
          feed_route_group_it->check_logs_period().get() :
          configuration_->check_logs_period();

        for(Routes::const_iterator it = routes.begin(); it != routes.end(); ++it)
        {
          typedef xsd::AdServer::Configuration::SrcDestType HostsRoute;
          typedef xsd::AdServer::Configuration::RouteType::files_sequence Files;

          SchedType feed_type = get_feed_type_(it->type().c_str());

          std::string post_command;

          if (it->post_command().present())
          {
            post_command = *it->post_command();
          }

          const HostsRoute& hosts_route = it->hosts();

          StringList source_hosts;
          StringList dest_hosts;

          // don't need procces dst_hosts for ST_DEFINITEHASH
          parse_hosts_route_(
            hosts_route,
            source_hosts,
            feed_type != ST_DEFINITEHASH ? &dest_hosts : 0);

          StringList::const_iterator host_name_it =
            check_hosts_(source_hosts);
          if(host_name_it != source_hosts.end())
          {
            const Files& files = it->files();

            for(Files::const_iterator file_it = files.begin();
                file_it != files.end(); ++file_it)
            {
              std::string dst_dir;
              adapt_path_(file_it->destination().c_str(), dst_dir);
              std::string src_pattern;
              adapt_path_(file_it->source().c_str(), src_pattern);

              if(logger_ && logger_->log_level() >= TraceLevel::MIDDLE)
              {
                std::ostringstream oss;
                oss << "To create FeedRouteProcessor: "
                  "host_name = '" << *host_name_it <<
                  ", source pattern = '" << src_pattern <<
                  ", destination dir = '" << dst_dir << ", ";

                Algs::print(oss, dest_hosts.begin(), dest_hosts.end());

                logger_->log(
                  oss.str(), TraceLevel::MIDDLE, Aspect::SYNC_LOGS);
              }

              FeedRouteProcessor::CommandType local_copy_command_type =
                feed_route_group_it->local_copy_command_type1() == "rsync" ?
                FeedRouteProcessor::CT_RSYNC :
                FeedRouteProcessor::CT_GENERIC;

              FeedRouteProcessor::CommandType remote_copy_command_type =
                feed_route_group_it->remote_copy_command_type1() == "rsync" ?
                FeedRouteProcessor::CT_RSYNC :
                FeedRouteProcessor::CT_GENERIC;

              // init destination router
              RouteBasicHelper_var destination_host_router;

              switch (feed_type)
              {
              case ST_ROUND_ROBIN:
                destination_host_router = new RouteRoundRobinHelper(
                  feed_type,
                  dest_hosts,
                  configuration_->host_check_period());
                break;
              case ST_BY_NUMBER:
                destination_host_router = new RouteByNumberHelper(
                  feed_type, dest_hosts);
                break;
              case ST_HASH:
                if(!file_it->pattern().present())
                {
                  throw Exception("Not present pattern for Hash route");
                }
                destination_host_router = new RouteHashHelper(
                  feed_type,
                  dest_hosts,
                  file_it->pattern()->c_str());
                break;
              case ST_DEFINITEHASH:
                if(!file_it->pattern().present())
                {
                  throw Exception("Not present pattern for DefiniteHash route");
                }
                destination_host_router = new RouteDefiniteHashHelper(
                  feed_type,
                  hosts_route.destination().c_str(),
                  cluster_config.definite_hash_schema().c_str(),
                  file_it->pattern()->c_str(),
                  Generics::Time(10));
                break;
              case ST_FROM_FILE_NAME:
                if(!it->pattern().present())
                {
                  throw Exception("Not present pattern for HostName route");
                }
                destination_host_router = new RouteHostFromFileNameHelper(
                  feed_type,
                  it->pattern()->c_str());
                break;
              }

              const BasicFeedRouteProcessor::FetchType fetch_type =
                BasicFeedRouteProcessor::get_fetch_type(
                  it->fetch_type().c_str());

              // init processor
              BasicFeedRouteProcessor_var route_processor;

              if(pool_task_runner)
              {
                route_processor = new ThreadPoolFeedRouteProcessor(
                  &error_logger_,
                  host_checker_,
                  src_pattern.c_str(),
                  destination_host_router,
                  dst_dir.c_str(),
                  feed_route_group_it->tries_per_file(),
                  feed_route_group_it->local_copy_command().c_str(),
                  local_copy_command_type,
                  feed_route_group_it->remote_copy_command().c_str(),
                  remote_copy_command_type,
                  feed_route_group_it->parse_source().present() ?
                    feed_route_group_it->parse_source().get() : true,
                  feed_route_group_it->unlink_source().present() ?
                    feed_route_group_it->unlink_source().get() : true,
                  feed_route_group_it->interruptible().present() ?
                    feed_route_group_it->interruptible().get() : false,
                  feed_type,
                  pool_task_runner,
                  post_command.c_str(),
                  fetch_type);
              }
              else
              {
                route_processor = new FeedRouteProcessor(
                  &error_logger_,
                  host_checker_,
                  src_pattern.c_str(),
                  destination_host_router,
                  dst_dir.c_str(),
                  feed_route_group_it->tries_per_file(),
                  feed_route_group_it->local_copy_command().c_str(),
                  local_copy_command_type,
                  feed_route_group_it->remote_copy_command().c_str(),
                  remote_copy_command_type,
                  feed_route_group_it->parse_source().present() ?
                    feed_route_group_it->parse_source().get() : true,
                  feed_route_group_it->unlink_source().present() ?
                    feed_route_group_it->unlink_source().get() : true,
                  feed_route_group_it->interruptible().present() ?
                    feed_route_group_it->interruptible().get() : false,
                  feed_type,
                  post_command.c_str(),
                  fetch_type);
              }

              route_processors_.push_back(route_processor);

              add_child_object(route_processor);

              TaskMessage_var task(new ProcessRouteTask(route_processors_.back(),
                check_period, 0, this));
              task_runner_->enqueue_task(task);
            }
          }
        }

        if(pool_task_runner)
        {
          add_child_object(pool_task_runner);
        }
      }
    }

    {
      /* create fetch processors */
      typedef ClusterConfig::FetchRouteGroup_sequence FetchRoutes;
      typedef xsd::AdServer::Configuration::FetchRouteGroupType::Route_sequence Routes;

      const FetchRoutes& fetch_routes = cluster_config.FetchRouteGroup();

      for(FetchRoutes::const_iterator fetch_it = fetch_routes.begin();
          fetch_it != fetch_routes.end(); ++fetch_it)
      {
        const Routes& routes = fetch_it->Route();

        unsigned long check_period = fetch_it->check_logs_period().present() ?
          fetch_it->check_logs_period().get() :
          configuration_->check_logs_period();

        for(Routes::const_iterator it = routes.begin(); it != routes.end(); ++it)
        {
          typedef xsd::AdServer::Configuration::SrcDestType HostsRoute;
          typedef xsd::AdServer::Configuration::RouteType::files_sequence Files;

          const HostsRoute& hosts_route = it->hosts();

          // Our side hosts (source for feed mode and dest for fetch)
          StringList src_hosts;
          StringList dst_hosts;

          parse_hosts_route_(hosts_route, src_hosts, &dst_hosts);

          StringList::const_iterator host_name_it;
          if((host_name_it = check_hosts_(dst_hosts)) != dst_hosts.end())
          {
            const Files& files = it->files();

            for(Files::const_iterator file_it = files.begin();
                file_it != files.end(); ++file_it)
            {
              std::string src, dst;

              FetchRouteProcessor_var route_processor(
                new FetchRouteProcessor(
                  &error_logger_,
                  host_checker_,
                  host_name_it->c_str(),
                  adapt_path_(file_it->source().c_str(), src),
                  src_hosts,
                  adapt_path_(file_it->destination().c_str(), dst),
                  fetch_it->local_copy_command().c_str(),
                  FetchRouteProcessor::CT_GENERIC,
                  fetch_it->remote_copy_command().c_str(),
                  FetchRouteProcessor::CT_RSYNC,
                  fetch_it->ssh_command().c_str(),
                  fetch_it->remote_ls_command().c_str(),
                  fetch_it->remote_rm_command().c_str(),
                  check_period));

              route_processors_.push_back(route_processor);

              add_child_object(route_processor);

              TaskMessage_var task(new ProcessRouteTask(route_processors_.back(),
                check_period, 0, this));
              task_runner_->enqueue_task(task);
            }
          }
        }
      }
    }

    // add error log flusher
    {
      RouteProcessor* ptr =
        new Utils::LogsRouteProcessorFlusher(&error_logger_);
      route_processors_.push_back(ptr);
      TaskMessage_var task(new ProcessRouteTask(route_processors_.back(),
        configuration_->check_logs_period(), 0, this));
      task_runner_->enqueue_task(task);
    }

    try
    {
      add_child_object(task_runner_);
      add_child_object(scheduler_);
    }
    catch(const Generics::CompositeActiveObject::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": CompositeActiveObject::Exception caught: " <<
        ex.what();
      throw Exception(ostr);
    }
  }

  SyncLogsImpl::~SyncLogsImpl() noexcept
  {}

  void
  SyncLogsImpl::schedule_route_processing_task_(
    RouteProcessor *processor,
    unsigned long timeout)
    noexcept
  {
    static const char* FUN = "SyncLogsImpl::schedule_route_processing_task_()";

    try
    {
      TaskMessage_var task(
        new ProcessRouteTask(processor, timeout, task_runner_, this));
      Generics::Time tm = Generics::Time::get_time_of_day() + timeout;
      scheduler_->schedule(task, tm);
    }
    catch (const eh::Exception &ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": eh::Exception caught: " << ex.what();
      callback_->critical(ostr.str());
    }
  }

  void SyncLogsImpl::parse_hosts_route_(
    const HostsRoute& hosts_route,
    StringList& src_hosts,
    StringList* dst_hosts)
  {
    static const char* FUN = "SyncLogsImpl::parse_hosts_route_()";

    try
    {
      std::stringstream is;
      is << hosts_route.source();
      String::SequenceAnalyzer::interprete_base_sequence(
        is,
        src_hosts);
    }
    catch (const String::SequenceAnalyzer::BasicAnalyzerException &ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught Generics::BasicAnalyzerException: " <<
        ex.what();
      throw Exception(ostr);
    }

    try
    {
      if(dst_hosts)
      {
        std::stringstream is;
        is << hosts_route.destination();
        String::SequenceAnalyzer::interprete_base_sequence(
          is,
          *dst_hosts);
      }
    }
    catch (const String::SequenceAnalyzer::BasicAnalyzerException &ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught Generics::BasicAnalyzerException: " <<
        ex.what();
      throw Exception(ostr);
    }
  }

  const char*
  SyncLogsImpl::adapt_path_(const char* dir, std::string& res) const
    /*throw(Exception)*/
  {
    if(dir[0] == '\0')
    {
      throw Exception("Non correct empty destination directory.");
    }

    if(dir[0] == '/')
    {
      res = dir;
      return dir;
    }

    if(configuration_->ClusterConfig().root_logs_dir().present())
    {
      res = *(configuration_->ClusterConfig().root_logs_dir());
      if(*res.rbegin() != '/')
      {
        res += "/";
      }
    }

    res += dir;
    return res.c_str();
  }

  SchedType SyncLogsImpl::get_feed_type_(
    const char* feed_type_name)
    /*throw(Exception)*/
  {
    std::string ft_name(feed_type_name);
    if(ft_name == "RoundRobin")
    {
      return ST_ROUND_ROBIN;
    }
    else if(ft_name == "ByNumber")
    {
      return ST_BY_NUMBER;
    }
    else if(ft_name == "Hash")
    {
      return ST_HASH;
    }
    else if(ft_name == "DefiniteHash")
    {
      return ST_DEFINITEHASH;
    }
    else if(ft_name == "HostName")
    {
      return ST_FROM_FILE_NAME;
    }

    Stream::Error ostr;
    ostr << "SyncLogsImpl::get_feed_type_(): unknown feed type '" <<
      feed_type_name << "'";
    throw Exception(ostr);
  }

  StringList::const_iterator
  SyncLogsImpl::check_hosts_(const StringList& hosts_list)
    /*throw(Exception)*/
  {
    try
    {
      for (StringList::const_iterator it = hosts_list.begin();
        it != hosts_list.end(); ++it)
      {
        if (host_checker_.check_host_name(*it))
        {
          return it;
        }
      }

      return hosts_list.end();
    }
    catch (const eh::Exception& e)
    {
      Stream::Error error;
      error << "SyncLogsImpl::check_hosts_(): "
        "eh::Exception caught: " << e.what();

      throw Exception(error);
    }
  }
} // namespace LogProcessing
} // namespace AdServer
