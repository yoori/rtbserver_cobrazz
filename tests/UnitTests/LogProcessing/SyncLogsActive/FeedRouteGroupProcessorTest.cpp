#include <dirent.h>
#include <fstream>
#include <sys/stat.h>
#include <set>
#include <string>

#include "../../TestHelpers.hpp"

#include <Generics/CompositeActiveObject.hpp>
#include <Logger/StreamLogger.hpp>

#include <LogProcessing/SyncLogs/FeedRouteGroupProcessor.hpp>

using namespace AdServer::LogProcessing;

namespace
{
  std::string host("-");
  const LocalInterfaceChecker host_checker(host);
  Logging::Logger_var logger(new Logging::Null::Logger());
  Utils::ErrorPool error_logger(1024, logger);

  const std::string root_path = "./.FeedRouteProcessorTest";
  const std::string request_dst_path = root_path + "/Out/Request";
  const std::string request_src_path = root_path + "/Request";
  const std::string request_src_commit_path = root_path + "/Request.commit";
  const std::string click_dst_path = root_path + "/Out/Click";
  const std::string click_src_path = root_path + "/Click";
  const std::string click_src_commit_path = root_path + "/Click.commit";

  const StringList dst_hosts{"-"};

  RouteBasicHelper_var destination_host_router =
    new RouteRoundRobinHelper(
      ST_ROUND_ROBIN,
      dst_hosts,
      10);

  void
  read_dir(
    const std::string& path,
    std::set<std::string>& entries)
    /*throw(std::exception)*/
  {
    DIR *dir = opendir(path.c_str());

    if (!dir)
    {
      throw std::runtime_error("can't open dir " + path);
    }

    dirent* ent;

    while ((ent = readdir(dir)))
    {
      const std::string full_path = path + '/' + ent->d_name;
      struct stat st;

      if (ent->d_name[0] != '.' &&
          stat(full_path.c_str(), &st) == 0)
      {
        if (S_ISREG(st.st_mode))
        {
          entries.insert(ent->d_name);
        }
      }
    }

    closedir (dir);
  }

  class ThrowCallback
    : public Generics::ActiveObjectCallback, public ReferenceCounting::AtomicImpl
  {
  public:
    virtual void
    report_error(
      Severity,
      const String::SubString& description,
      const char* = 0) noexcept
    {
      std::cerr << description.str() << std::endl;
    }

  protected:
    virtual
    ~ThrowCallback() noexcept
    {}
  };

  class CompositeActiveObjectImpl:
    public Generics::CompositeActiveObject,
    public virtual ReferenceCounting::AtomicImpl
  {};

  Generics::CompositeActiveObject_var composite_active_object;

  Generics::ActiveObjectCallback_var callback = new ThrowCallback();
  Generics::TaskRunner_var task_runner;
  Generics::Planner_var scheduler;

  SyncLog::FileReceiverFacade_var file_receiver_facade;
  FeedRouteGroupProcessor_var route_group_processor;
}

void
setup() noexcept
{
  ::system(("rm -r " + root_path + " 2>/dev/null").c_str());
  ::system(("mkdir -p " + request_dst_path + " 2>/dev/null").c_str());
  ::system(("mkdir -p " + request_src_path + " 2>/dev/null").c_str());
  ::system(("mkdir -p " + request_src_path + "/Intermediate 2>/dev/null").c_str());
  ::system(("mkdir -p " + request_src_commit_path + " 2>/dev/null").c_str());
  ::system(("mkdir -p " + click_dst_path + " 2>/dev/null").c_str());
  ::system(("mkdir -p " + click_src_path + " 2>/dev/null").c_str());
  ::system(("mkdir -p " + click_src_path + "/Intermediate 2>/dev/null").c_str());
  ::system(("mkdir -p " + click_src_commit_path + " 2>/dev/null").c_str());

  composite_active_object = new CompositeActiveObjectImpl();
  task_runner = new Generics::TaskRunner(callback, 1);
  scheduler = new Generics::Planner(callback);
  composite_active_object->add_child_object(task_runner);
  composite_active_object->add_child_object(scheduler);
}

void
teardown() noexcept
{
  composite_active_object->deactivate_object();
  composite_active_object->wait_object();

  if (route_group_processor)
  {
    route_group_processor->deactivate_object();
    route_group_processor->wait_object();
  }

  route_group_processor.reset();
  file_receiver_facade.reset();
  scheduler.reset();
  task_runner.reset();
  composite_active_object.reset();

  //::system(("rm -r " + root_path + " 2>/dev/null").c_str());
}

FeedRouteGroupProcessor_var
make_route_group_processor(
  bool commit_mode_flag,
  unsigned long tries_per_file = 1,
  RouteBasicHelper* dest_router = destination_host_router)
  noexcept
{
  std::vector<Route> routes;

  routes.emplace_back(
    &error_logger,
    host_checker,
    (request_src_path + "/Request").c_str(),
    dest_router,
    request_dst_path.c_str(),
    tries_per_file,
    "cp ##SRC_PATH## ##DST_PATH##",
    Route::CT_GENERIC,
    "cp ##SRC_PATH## ##DST_PATH##",
    Route::CT_GENERIC,
    true,
    true,
    false,
    ST_ROUND_ROBIN,
    commit_mode_flag,
    nullptr);

  routes.emplace_back(
    &error_logger,
    host_checker,
    (click_src_path + "/Click").c_str(),
    dest_router,
    click_dst_path.c_str(),
    tries_per_file,
    "cp ##SRC_PATH## ##DST_PATH##",
    Route::CT_GENERIC,
    "cp ##SRC_PATH## ##DST_PATH##",
    Route::CT_GENERIC,
    true,
    true,
    false,
    ST_ROUND_ROBIN,
    commit_mode_flag,
    nullptr);

  SyncLog::FileReceiverFacade::FileReceiversInitializer init_list;

  for (SyncLog::LogType log_type = 0; log_type < routes.size(); ++ log_type)
  {
    Route& route = routes[log_type];
    init_list.emplace_back(log_type, route.file_receiver);
    composite_active_object->add_child_object(route.mover);

    if (route.commit_mover)
    {
      composite_active_object->add_child_object(route.commit_mover);
    }
  }

  file_receiver_facade = new SyncLog::FileReceiverFacade(init_list);
  composite_active_object->add_child_object(file_receiver_facade);

  return FeedRouteGroupProcessor_var(
    new FeedRouteGroupProcessor(
      routes,
      file_receiver_facade,
      &error_logger,
      task_runner,
      scheduler,
      callback,
      1, // threads_number
      5)); // timeout
}

TEST_EX(no_commit_mode, setup, teardown)
{
  {
    std::ostringstream ost;
    ost << "LogGenerator -g \"Request\" "
     "-p " << root_path << " -c 1 -l 5 -r >/dev/null";
    system(ost.str().c_str());
  }

  {
    std::ostringstream ost;
    ost << "LogGenerator -g \"Click\" "
     "-p " << root_path << " -c 1 -l 5 -r >/dev/null";
    system(ost.str().c_str());
  }

  route_group_processor = make_route_group_processor(false);
  composite_active_object->activate_object();
  route_group_processor->activate_object();

  sleep(10);

  std::set<std::string> entries;
  read_dir(request_dst_path, entries);
  read_dir(click_dst_path, entries);
  ASSERT_TRUE (entries.size() == 10);
}

TEST_EX(commit_mode, setup, teardown)
{
  {
    std::ostringstream ost;
    ost << "LogGenerator -g \"Request\" "
     "-p " << root_path << " -c 1 -l 5 -r >/dev/null";
    system(ost.str().c_str());
  }

  {
    std::ostringstream ost;
    ost << "LogGenerator -g \"Click\" "
     "-p " << root_path << " -c 1 -l 5 -r >/dev/null";
    system(ost.str().c_str());
  }

  {
    // make commit single commit file on start
    std::ofstream ofs(request_src_commit_path + "/~Request.20150320.151505.810429.15391749.1.0.commit.localhost");
    ASSERT_TRUE (ofs);
  }

  route_group_processor = make_route_group_processor(true);
  composite_active_object->activate_object();
  route_group_processor->activate_object();

  sleep(10);

  std::set<std::string> entries;
  read_dir(request_dst_path, entries);
  read_dir(click_dst_path, entries);
  ASSERT_TRUE (entries.size() == 21);
}

RUN_TESTS;
