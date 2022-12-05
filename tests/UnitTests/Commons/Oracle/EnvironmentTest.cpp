#include <iostream>
#include <cstdlib>
#include <cstring>
#include <string>

#include <eh/Exception.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <Generics/TaskRunner.hpp>
#include <Generics/AppUtils.hpp>

#include <Commons/Oracle/Environment.hpp>
#include <Commons/Oracle/Connection.hpp>
#include <Commons/Oracle/Statement.hpp>
#include <Commons/Oracle/ResultSet.hpp>

#include "Barrier.hpp"

using namespace AdServer::Commons;
using namespace AdServer::Commons::Oracle;

static
void
usage(const char *prog)
{
  std::cout << "USAGE: " << prog << " [OPTIONS]"
    "Options: "
    "  --help : show help" << std::endl <<
    "  --db : oracle database sid (//oracle.ocslab.com/addbdev.ocslab.com)" <<
      std::endl <<
    "  --user : oracle user name (adserver)" << std::endl <<
    "  --pwd, --password : oracle user password (adserver)" << std::endl;
}

template<typename ConnectionCreatorType>
class TestJob:
  public Generics::Task,
  public ReferenceCounting::AtomicImpl
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  TestJob(
    Generics::ActiveObject* controller,
    ConnectionCreatorType conn_creator,
    Barrier* barrier) noexcept
    : controller_(ReferenceCounting::add_ref(controller)),
      conn_creator_(conn_creator),
      barrier_(barrier)
  {}

  void execute() noexcept;

protected:
  virtual ~TestJob() noexcept
  {}

private:
  Generics::ActiveObject_var controller_;
  ConnectionCreatorType conn_creator_;
  Barrier* barrier_;
};

static
void
execute_query(Connection* conn) /*throw(eh::Exception)*/
{
  Statement_var stmt = conn->create_statement("SELECT 1 FROM DUAL");
  ResultSet_var rs = stmt->execute_query();
  while (rs->next())
  {}
}

template<typename ConnectionCreatorType>
void
TestJob<ConnectionCreatorType>::execute() noexcept
{
  try
  {
    // Phase 1: object is not active, create methods should fail.
    barrier_->wait();

    try
    {
      Connection_var conn = conn_creator_();

      Stream::Error error;
      error << "SwitchableEnvironment::create_connection(): "
        "object is not active, but method succeeded" <<
        std::endl;
      throw Exception(error);
    }
    catch(const Oracle::NonActive&)
    {}

    // Phase 2: activate object.  Queries should pass.
    if (barrier_->wait())
    {
      controller_->activate_object();
    }
    barrier_->wait();

    Connection_var conn1 = conn_creator_();
    execute_query(conn1);

    // Phase 3: deactivate object.  Queries on existing connections
    // should still work, but new calls to create methods should fail.
    if (barrier_->wait())
    {
      controller_->deactivate_object();
    }

    barrier_->wait();

    execute_query(conn1);

    try
    {
      Connection_var conn = conn_creator_();
      Stream::Error error;
      error << "create_connection didn't failed on inactive object" <<
        std::endl;
      throw Exception(error);
    }
    catch(const Oracle::NonActive&)
    {}

    // Phase 4: free existing connections and do wait_object().
    if (barrier_->wait())
    {
      conn1.reset();
      
      controller_->wait_object();
    }
    else
    {
      execute_query(conn1);
      conn1.reset();
    }
  }
  catch (eh::Exception &e)
  {
    std::cerr << "Exception: " << e.what() << std::endl;
  }
}

class Callback :
  public Generics::ActiveObjectCallback,
  public ReferenceCounting::AtomicImpl
{
public:
  void report_error(
    Generics::ActiveObjectCallback::Severity severity,
    const String::SubString& description, const char* error_code = "" ) noexcept
  {
    try
    {
      std::cerr << severity << "(" << error_code << "): " <<
        description << std::endl;
    }
    catch (...) {}
  }
};

class EnvironmentConnectionCreator
{
public:
  EnvironmentConnectionCreator(
    Environment* env,
    const ConnectionDescription& conn_descr)
    : env_(ReferenceCounting::add_ref(env)),
      conn_descr_(conn_descr)
  {}

  Connection_var operator()()
  {
    return env_->create_connection(conn_descr_);
  }

protected:
  Environment_var env_;
  ConnectionDescription conn_descr_;
};

class PoolConnectionCreator
{
public:
  PoolConnectionCreator(
    ConnectionPool* connection_pool)
    : connection_pool_(ReferenceCounting::add_ref(connection_pool))
  {}

  Connection_var operator()()
  {
    return connection_pool_->get_connection(false);
  }

protected:
  ConnectionPool_var connection_pool_;
};

void
run(const ConnectionDescription& conn_descr,
  unsigned long threads,
  unsigned long pools)
  /*throw(eh::Exception)*/
{
  {
    std::cout << "to test SwitchableEnvironment" << std::endl;

    // SwitchableEnvironment testing
    SwitchableEnvironment_var env =
      SwitchableEnvironment::create_environment(
        SwitchableEnvironment::EM_THREADED_MUTEXED);

    Generics::ActiveObjectCallback_var callback(new Callback);
    Generics::TaskRunner_var crew(
      new Generics::TaskRunner(callback, threads));

    Barrier barrier(threads);
    EnvironmentConnectionCreator conn_creator(env, conn_descr);

    for (size_t i = 0; i < threads; ++i)
    {
      crew->enqueue_task(Generics::Task_var(
        new TestJob<EnvironmentConnectionCreator>(env, conn_creator, &barrier)));
    }

    crew->activate_object();
    crew->wait_for_queue_exhausting();
    crew->deactivate_object();
    crew->wait_object();
  }

  {
    std::cout << "to test ConnectionPool(unlimited) for " << pools << " pools at " <<
      threads << " threads" << std::endl;

    // ConnectionPool testing
    SwitchableEnvironment_var env =
      SwitchableEnvironment::create_environment(
        SwitchableEnvironment::EM_THREADED_MUTEXED);

    Generics::ActiveObjectCallback_var callback(new Callback);
    Generics::TaskRunner_var crew(
      new Generics::TaskRunner(callback, threads));

    Barrier barrier(threads);

    for(unsigned long pool_i = 0; pool_i < pools; ++pool_i)
    {
      ConnectionPool_var connection_pool = env->create_connection_pool(
        conn_descr);
      PoolConnectionCreator conn_creator(connection_pool);

      for (size_t i = threads * pool_i / pools;
           i < std::min(threads * (pool_i + 1) / pools, threads); ++i)
      {
        crew->enqueue_task(Generics::Task_var(
          new TestJob<PoolConnectionCreator>(env, conn_creator, &barrier)));
      }
    }

    crew->activate_object();
    crew->wait_for_queue_exhausting();
    crew->deactivate_object();
    crew->wait_object();
  }

  {
    std::cout << "to test ConnectionPool(limited) for " << pools << " pools at " <<
      threads << " threads" << std::endl;

    // ConnectionPool testing
    SwitchableEnvironment_var env =
      SwitchableEnvironment::create_environment(
        SwitchableEnvironment::EM_THREADED_MUTEXED);

    Generics::ActiveObjectCallback_var callback(new Callback);
    Generics::TaskRunner_var crew(
      new Generics::TaskRunner(callback, threads));

    Barrier barrier(threads);

    for(unsigned long pool_i = 0; pool_i < pools; ++pool_i)
    {
      ConnectionPool_var connection_pool = env->create_connection_pool(
        conn_descr,
        1 + threads * (pool_i + 1) / pools -
          threads * pool_i / pools // max connections
        );
      PoolConnectionCreator conn_creator(connection_pool);

      for (size_t i = threads * pool_i / pools;
           i < threads * (pool_i + 1) / pools; ++i)
      {
        crew->enqueue_task(Generics::Task_var(
          new TestJob<PoolConnectionCreator>(env, conn_creator, &barrier)));
      }
    }

    crew->activate_object();
    crew->wait_for_queue_exhausting();
    crew->deactivate_object();
    crew->wait_object();
  }
}

int
main(int argc, const char *argv[])
{
  try
  {
    Generics::AppUtils::StringOption opt_ora_server(
      "//oraads/addbads.ocslab.com");
    Generics::AppUtils::StringOption opt_ora_user("ads_3");
    Generics::AppUtils::StringOption opt_ora_pwd("adserver");
    Generics::AppUtils::Option<unsigned long> opt_threads(10);
    Generics::AppUtils::Option<unsigned long> opt_pools(2);
    Generics::AppUtils::CheckOption opt_help;
    Generics::AppUtils::Args args(-1);
    args.add(
      Generics::AppUtils::equal_name("db"),
      opt_ora_server);
    args.add(
      Generics::AppUtils::equal_name("user"),
      opt_ora_user);
    args.add(
      Generics::AppUtils::equal_name("pwd") ||
      Generics::AppUtils::equal_name("password"),
      opt_ora_pwd); 
    args.add(
      Generics::AppUtils::equal_name("threads"),
      opt_threads);
    args.add(
      Generics::AppUtils::equal_name("pools"),
      opt_pools);
    args.add(
      Generics::AppUtils::equal_name("help"),
      opt_help);

    args.parse(argc - 1, argv + 1);
    if(opt_help.enabled())
    {
      usage(argv[0]);
      return 0;
    }

    run(
      ConnectionDescription(
        opt_ora_user->c_str(),
        opt_ora_pwd->c_str(),
        opt_ora_server->c_str()),
      *opt_threads,
      *opt_pools);

    return 0;
  }
  catch (Exception &e)
  {
    std::cerr << "Exception: " << e.what() << std::endl;

    return 1;
  }
}
