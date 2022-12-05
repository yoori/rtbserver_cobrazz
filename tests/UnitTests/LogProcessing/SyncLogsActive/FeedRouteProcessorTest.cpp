#include <dirent.h>
#include <sys/stat.h>
#include <set>
#include <string>

#include "../../TestHelpers.hpp"

#include <Logger/StreamLogger.hpp>

#include <LogProcessing/SyncLogs/FeedRouteProcessor.hpp>

using namespace AdServer::LogProcessing;

namespace
{
  std::string host("localhost");
  const LocalInterfaceChecker host_checker(host);
  Logging::Logger_var logger(new Logging::Null::Logger());
  Utils::ErrorPool error_logger(1024, logger);

  const std::string root_path = "./.FeedRouteProcessorTest";
  const std::string dst_path = root_path + "/Out";
  const std::string src_path = root_path + "/Request";

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

  Generics::ActiveObjectCallback_var callback = new ThrowCallback();

  Generics::TaskRunner_var pool_task_runner =
    new Generics::TaskRunner(callback, 1, 1);
}

void
setup() noexcept
{
  ::system(("rm -r " + root_path + " 2>/dev/null").c_str());
  ::system(("mkdir -p " + dst_path + " 2>/dev/null").c_str());
  ::system(("mkdir -p " + src_path + " 2>/dev/null").c_str());
  pool_task_runner->activate_object();
}

void
teardown() noexcept
{
  pool_task_runner->deactivate_object();
  pool_task_runner->wait_object();
}

BasicFeedRouteProcessor_var
make_route_processor(
  BasicFeedRouteProcessor::FetchType fetch_type,
  unsigned long tries_per_file = 1,
  RouteBasicHelper* dest_router = destination_host_router,
  const char* dst_dir = dst_path.c_str())
  noexcept
{
  return BasicFeedRouteProcessor_var(
    new FeedRouteProcessor(
      &error_logger,
      host_checker,
      (src_path + (fetch_type == BasicFeedRouteProcessor::FT_FULL ? "/Request*" : "/Request")).c_str(),
      dest_router,
      dst_dir,
      tries_per_file,
      "cp ##SRC_PATH## ##DST_PATH##",
      FeedRouteProcessor::CT_GENERIC,
      "cp ##SRC_PATH## ##DST_PATH##",
      FeedRouteProcessor::CT_GENERIC,
      true,
      true,
      false,
      ST_ROUND_ROBIN,
      "",
      fetch_type));
}

BasicFeedRouteProcessor_var
make_threaded_route_processor(
  BasicFeedRouteProcessor::FetchType fetch_type,
  unsigned long tries_per_file = 1,
  RouteBasicHelper* dest_router = destination_host_router,
  const char* dst_dir = dst_path.c_str())
  noexcept
{
  return BasicFeedRouteProcessor_var(//new FeedRouteProcessor(
    new ThreadPoolFeedRouteProcessor(
      &error_logger,
      host_checker,
      (src_path + (fetch_type == BasicFeedRouteProcessor::FT_FULL ? "/Request*" : "/Request")).c_str(),
      dest_router,
      dst_dir,
      tries_per_file,
      "cp ##SRC_PATH## ##DST_PATH##",
      FeedRouteProcessor::CT_GENERIC,
      "cp ##SRC_PATH## ##DST_PATH##",
      FeedRouteProcessor::CT_GENERIC,
      true,
      true,
      false,
      ST_ROUND_ROBIN,
      pool_task_runner,
      "",
      fetch_type));
}

TEST_EX(classic_fetch, setup, teardown)
{
  std::ostringstream ost;
  ost << "LogGenerator -g \"Request\" "
   "-p " << root_path << " -c 1 -l 5 -r >/dev/null";
  system(ost.str().c_str());

  BasicFeedRouteProcessor_var route_processor =
    make_route_processor(BasicFeedRouteProcessor::FT_FULL);

  route_processor->activate_object();
  route_processor->process();

  std::set<std::string> entries;
  read_dir(src_path, entries);
  ASSERT_TRUE (entries.empty());

  read_dir(dst_path, entries);
  ASSERT_TRUE (entries.size() == 5);

  route_processor->deactivate_object();
  route_processor->wait_object();
}

TEST_EX(new_fetch, setup, teardown)
{
  std::ostringstream ost;
  ost << "LogGenerator -g \"Request\" "
   "-p " << root_path << " -c 1 -l 5 -r >/dev/null";
  system(ost.str().c_str());

  BasicFeedRouteProcessor_var route_processor =
    make_route_processor(BasicFeedRouteProcessor::FT_NEW);

  route_processor->activate_object();
  route_processor->process();

  std::set<std::string> entries;
  read_dir(src_path, entries);
  ASSERT_TRUE (entries.empty());

  read_dir(src_path + "/Intermediate", entries);
  ASSERT_TRUE (entries.empty());

  read_dir(dst_path, entries);
  ASSERT_TRUE (entries.size() == 5);

  route_processor->deactivate_object();
  route_processor->wait_object();
}

TEST_EX(commited_fetch, setup, teardown)
{
  std::ostringstream ost;
  ost << "LogGenerator -g \"Request\" "
   "-p " << root_path << " -c 1 -l 5 >/dev/null";
  system(ost.str().c_str());

  BasicFeedRouteProcessor_var route_processor =
    make_route_processor(BasicFeedRouteProcessor::FT_COMMITED);

  route_processor->activate_object();
  route_processor->process();

  std::set<std::string> entries;
  read_dir(src_path, entries);
  ASSERT_TRUE (entries.empty());

  read_dir(src_path + "/Intermediate", entries);
  ASSERT_TRUE (entries.empty());

  read_dir(dst_path, entries);
  ASSERT_TRUE (entries.size() == 5);

  route_processor->deactivate_object();
  route_processor->wait_object();
}

namespace
{
  class AttemptFailedRouteHelper:
    public RouteBasicHelper,
    public ReferenceCounting::AtomicImpl
  {
  public:
    AttemptFailedRouteHelper(std::size_t fail_attempt_repeats)
      /*throw(Exception)*/
      : RouteBasicHelper(ST_ROUND_ROBIN),
        fail_attempt_repeats_(fail_attempt_repeats),
        attempt_num_(0)
    {}

    virtual std::string
    get_dest_host(const char*)
      /*throw(NotAvailable)*/
    {
      if (attempt_num_ == fail_attempt_repeats_)
      {
        ::system(("mkdir -p " + dst_path + "/attempt").c_str());
      }

      ++attempt_num_;
      return "-";
    }

    virtual void
    bad_host(const std::string&) noexcept
    {}

  protected:
    virtual
    ~AttemptFailedRouteHelper() noexcept
    {}

  private:
    const std::size_t fail_attempt_repeats_;
    std::size_t attempt_num_;
  };
}

TEST_EX(commited_fetch_first_attempt_failed, setup, teardown)
{
  std::ostringstream ost;
  ost << "LogGenerator -g \"Request\" "
   "-p " << root_path << " -c 1 -l 5 >/dev/null";
  system(ost.str().c_str());

  RouteBasicHelper_var destination_host_router_first_attempt_failed =
    new AttemptFailedRouteHelper(1);

  BasicFeedRouteProcessor_var route_processor =
    make_route_processor(
      BasicFeedRouteProcessor::FT_COMMITED,
      2,
      destination_host_router_first_attempt_failed,
      (dst_path + "/attempt").c_str());

  route_processor->activate_object();
  route_processor->process();

  std::set<std::string> entries;
  read_dir(src_path, entries);
  ASSERT_TRUE (entries.empty());

  read_dir(src_path + "/Intermediate", entries);
  ASSERT_TRUE (entries.empty());

  read_dir(dst_path + "/attempt", entries);
  ASSERT_TRUE (entries.size() == 5);

  route_processor->deactivate_object();
  route_processor->wait_object();
}

void commited_fetch_first_process_failed_(
  BasicFeedRouteProcessor_var route_processor)
{
  std::ostringstream ost;
  ost << "LogGenerator -g \"Request\" "
   "-p " << root_path << " -c 1 -l 5 >/dev/null";
  system(ost.str().c_str());

  route_processor->activate_object();
  route_processor->process();
  route_processor->process();

  std::set<std::string> entries;
  read_dir(src_path, entries);
  ASSERT_TRUE (entries.empty());

  read_dir(src_path + "/Intermediate", entries);
  ASSERT_TRUE (entries.empty());

  read_dir(dst_path + "/attempt", entries);
  ASSERT_TRUE (entries.size() == 5);

  route_processor->deactivate_object();
  route_processor->wait_object();
}

TEST_EX(commited_fetch_first_process_failed, setup, teardown)
{
  RouteBasicHelper_var destination_host_router_first_process_failed =
    new AttemptFailedRouteHelper(3);

  BasicFeedRouteProcessor_var route_processor =
    make_route_processor(
      BasicFeedRouteProcessor::FT_COMMITED,
      2,
      destination_host_router_first_process_failed,
      (dst_path + "/attempt").c_str());
  commited_fetch_first_process_failed_(route_processor);
}

TEST_EX(threaded_commited_fetch_first_process_failed, setup, teardown)
{
  RouteBasicHelper_var destination_host_router_first_process_failed =
    new AttemptFailedRouteHelper(3);

  BasicFeedRouteProcessor_var route_processor =
    make_threaded_route_processor(
      BasicFeedRouteProcessor::FT_COMMITED,
      2,
      destination_host_router_first_process_failed,
      (dst_path + "/attempt").c_str());
  commited_fetch_first_process_failed_(route_processor);
}

TEST(FileHashDeterminer_incorrect_filename)
{
  FileHashDeterminer file_hash_determiner(".*\\.##HASH##");

  unsigned long hash = 0;
  ASSERT_TRUE (file_hash_determiner.get_hash(
    hash,
    "ActionRequest.20140701.075238.611701.07479402.1.116") );

  ASSERT_EQUALS(hash, 116U);

  ASSERT_FALSE (file_hash_determiner.get_hash(
    hash,
    "ActionRequest.20140701.075238.611701.07479402.1.116.C"));
}

RUN_TESTS;
