#include <Generics/AppUtils.hpp>
#include <Generics/Proc.hpp>

#include <Commons/Oracle/Environment.hpp>
#include <Commons/Oracle/Connection.hpp>
#include <Commons/Oracle/Statement.hpp>

using namespace AdServer::Commons::Oracle;

namespace
{
  const char USAGE[] =
    "USAGE: OcciTimedOutTest [OPTIONS]"
    "Options: "
    "  --help : show help\n"
    "  --db : oracle database sid (//oracle.ocslab.com/addbdev.ocslab.com)\n"
    "  --user : oracle user name (adserver)\n"
    "  --pwd, --password : oracle user password (adserver)"
    "  --num : number of test loops for each case (by default 10)"
    "  --no-timeout : specific mode of tests running without timeouts (for get expected RSS diffs)";
}

class MemChecker
{
public:
  MemChecker(std::ostream& out, const char* FUN, bool debug_memory)
    : debug_memory_(debug_memory)
  {
    Generics::Proc::memory_status(base_vsize_, base_rss_);
    out << FUN << ": start with VSIZE = " << base_vsize_ <<
      ", RSS = " << base_rss_;
    if(debug_memory)
    {
      base_alloc_ = AllocController::instance().sum();
      out << ", ALLOC = " << base_alloc_;
    }
    out << std::endl;
  }

  std::ostream&
  print_diff(std::ostream& out) const
  {
    unsigned long cur_vsize, cur_rss;
    Generics::Proc::memory_status(cur_vsize, cur_rss);

    out << "(dVSIZE = " << (cur_vsize - base_vsize_) <<
      ", dRSS = " << (cur_rss - base_rss_);
    if(debug_memory_)
    {
      out << ", dALLOC = " << AllocController::instance().sum() - base_alloc_;
    }
    out << ")";
    return out;
  }

protected:
  const bool debug_memory_;
  unsigned long base_vsize_, base_rss_, base_alloc_;
};

int
plsql_timeout_test(
  Environment* env,
  const ConnectionDescription& conn_descr,
  unsigned long loop_num,
  bool no_timeout,
  bool debug_memory)
{
  static const char* FUN = "plsql_timeout_test()";

  int res = 0;

  try
  {
    MemChecker mem_checker(std::cout, FUN, debug_memory);

    {
      // PL/SQL block
      for(unsigned long i = 0; i < loop_num; ++i)
      {
        bool timed_out = false;

        std::ostringstream sql_str;
        sql_str << "BEGIN dbms_lock.sleep(" << (no_timeout ? 1 : 1000) << "); END;";
        Connection_var conn = env->create_connection(conn_descr);
        Statement_var stmt = conn->create_statement(sql_str.str().c_str());

        Generics::Time timeout(no_timeout ? 1000 : 1);
        Generics::Time start_time(Generics::Time::get_time_of_day());

        try
        {
          stmt->execute(&timeout);
        }
        catch(const TimedOut& ex)
        {
          timed_out = true;
        }
  
        Generics::Time fin_time(Generics::Time::get_time_of_day());

        if(no_timeout)
        {
          std::cout << FUN << ": " << (fin_time - start_time);
          mem_checker.print_diff(std::cout) << std::endl;
        }
        else
        {
          if(!timed_out)
          {
            std::cerr << FUN << ": isn't timed out" << std::endl;
            return 1;
          }
          else if(((fin_time - start_time) - timeout) > Generics::Time(1))
          {
            std::cerr << FUN << ": isn't timed out: exec time = " <<
              (fin_time - start_time) << " > 1 sec" << std::endl;
            return 1;
          }
          else
          {
            std::cout << FUN << ": timed out successfully: " <<
              (fin_time - start_time);
            mem_checker.print_diff(std::cout) << std::endl;
          }

          // check connection status
          bool conn_terminated = false;

          try
          {
            Statement_var stmt = conn->create_statement("SELECT 1 FROM DUAL");
          }
          catch(const NotSupported& ex)
          {
            conn_terminated = true;
          }

          if(!conn_terminated)
          {
            std::cerr << FUN << ": statement isn't terminated after timed out" << std::endl;
            return 1;
          }
        }
      }    
    }

    if(!no_timeout)
    {
      // PL/SQL block no timeout
      Connection_var conn = env->create_connection(conn_descr);

      for(unsigned long i = 0; i < 2; ++i)
      {
        Statement_var stmt = conn->create_statement(
          "BEGIN dbms_lock.sleep(10); END;");

        bool timed_out = false;
  
        try
        {
          Generics::Time timeout(15);
          stmt->execute(&timeout);
        }
        catch(const TimedOut& ex)
        {
          timed_out = true;
        }

        if(timed_out)
        {
          std::cerr << FUN << ": request(" << i << ") timed out, but must be executed." << std::endl;
          return 1;
        }

        std::cout << "request(" << i << ") completed successfully." << std::endl;
      }
    }
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << FUN << ": caught unexpected eh::Exception: " << ex.what() << std::endl;
    res = 1;
  }

  return res;
}

int
select_plsql_sleep_timeout_test(
  Environment* env,
  const ConnectionDescription& conn_descr,
  unsigned long loop_num,
  bool no_timeout,
  bool debug_memory)
{
  static const char* FUN = "select_plsql_sleep_timeout_test()";

  int res = 0;

  try
  {
    MemChecker mem_checker(std::cout, FUN, debug_memory);

    {
      // SELECT WITH PL/SQL timeouts test
      Connection_var conn = env->create_connection(conn_descr);

      {
        Statement_var stmt = conn->create_statement(
          "CREATE OR REPLACE FUNCTION ADSERVER_ORACLE_TEST_SLEEP(n IN NUMBER) "
            "RETURN NUMBER IS "
          "BEGIN "
            "dbms_lock.sleep(n); "
            "return 1; "
          "END;");
        stmt->execute();
      }
    }

    for(unsigned long i = 0; i < loop_num; ++i)
    {
      // SELECT WITH PL/SQL timeouts test
      Connection_var conn = env->create_connection(conn_descr);

      {
        std::ostringstream sql_str;
        sql_str << "SELECT ADSERVER_ORACLE_TEST_SLEEP(" <<
          (no_timeout ? 1 : 1000) << ") FROM DUAL";
        Statement_var stmt = conn->create_statement(
          sql_str.str().c_str());

        bool timed_out = false;

        Generics::Time start_time = Generics::Time::get_time_of_day();

        try
        {
          Generics::Time timeout(no_timeout ? 1000 : 1);
          ResultSet_var rs = stmt->execute_query(&timeout);
          while(rs->next()) {}
        }
        catch(const TimedOut& ex)
        {
          timed_out = true;
        }

        Generics::Time fin_time = Generics::Time::get_time_of_day();

        if(no_timeout)
        {
          std::cout << FUN << ": " << (fin_time - start_time);
          mem_checker.print_diff(std::cout) << std::endl;
        }
        else if(timed_out)
        {
          std::cout << FUN << ": 'select' request timed out successfully: " <<
            (fin_time - start_time);
          mem_checker.print_diff(std::cout) << std::endl;
        }
        else
        {
          std::cerr << FUN << ": 'select' request isn't timed out." << std::endl;
          res = 1;
        }
      }
    }
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << FUN << ": caught unexpected eh::Exception: " << ex.what() << std::endl;
    res = 1;
  }

  try
  {
    Connection_var conn = env->create_connection(conn_descr);
    Statement_var stmt = conn->create_statement(
      "DROP FUNCTION ADSERVER_ORACLE_TEST_SLEEP");
    stmt->execute();
  }
  catch(const eh::Exception&)
  {}
  
  return res;
}

int
select_timeout_test(
  Environment* env,
  const ConnectionDescription& conn_descr,
  unsigned long loop_num,
  bool debug_memory)
{
  static const char* FUN = "select_timeout_test()";

  int res = 0;

  try
  {
    MemChecker mem_checker(std::cout, FUN, debug_memory);

    for(unsigned long i = 0; i < loop_num; ++i)
    {
      Connection_var conn = env->create_connection(conn_descr);

      Statement_var stmt = conn->create_statement(
        "SELECT * FROM all_objects a, all_objects b, all_objects c, all_objects d, all_objects e");

      bool timed_out = false;

      Generics::Time start_time = Generics::Time::get_time_of_day();

      try
      {
        Generics::Time timeout(1);
        ResultSet_var rs = stmt->execute_query(&timeout);
        while(rs->next()) {}
      }
      catch(const TimedOut& ex)
      {
        timed_out = true;
      }

      Generics::Time fin_time = Generics::Time::get_time_of_day();
      unsigned long cur_vsize, cur_rss;
      Generics::Proc::memory_status(cur_vsize, cur_rss);

      if(timed_out)
      {
        std::cout << FUN << ": 'select' request timed out successfully: " <<
          (fin_time - start_time);
        mem_checker.print_diff(std::cout) << std::endl;
      }
      else
      {
        std::cerr << FUN << ": 'select' request isn't timed out." << std::endl;
        res = 1;
      }
    }
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << FUN << ": caught unexpected eh::Exception: " << ex.what() << std::endl;
    res = 1;
  }
  
  return res;
}

int
check_timeouts(
  const ConnectionDescription& conn,
  unsigned long loop_num,
  bool no_timeout,
  bool debug_memory)
{
  static const char* FUN = "check_timeouts()";

  Environment_var env;

  try
  {
    env = Environment::create_environment(
      (Environment::EnvironmentMode)(
        Environment::EM_THREADED_MUTEXED |
        Environment::EM_OBJECT),
      0,
      debug_memory);
  }
  catch(const eh::Exception& ex)
  { 
    std::cerr << FUN << ": caught unexpected eh::Exception: " << ex.what() << std::endl;
    return 1;
  }

  int res = 0;
  res += plsql_timeout_test(
    env, conn, loop_num, no_timeout, debug_memory);
  res += select_plsql_sleep_timeout_test(
    env, conn, loop_num, no_timeout, debug_memory);

  if(!no_timeout)
  {
    res += select_timeout_test(env, conn, loop_num, debug_memory);
  }

  return res;
}

int
main(int argc, char** argv)
{
  int res = 0;
  
  Generics::AppUtils::StringOption opt_ora_server(
    "//oraads/addbads.ocslab.com");
  Generics::AppUtils::StringOption opt_ora_user("ads_3");
  Generics::AppUtils::StringOption opt_ora_pwd("adserver");
  Generics::AppUtils::Option<unsigned long> opt_loop_num(10);
  Generics::AppUtils::CheckOption opt_no_timeout;
  Generics::AppUtils::CheckOption opt_debug_memory;

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
    Generics::AppUtils::equal_name("num"),
    opt_loop_num);

  args.add(
    Generics::AppUtils::equal_name("no-timeout"),
    opt_no_timeout);

  args.add(
    Generics::AppUtils::equal_name("debug-memory"),
    opt_debug_memory);

  args.parse(argc - 1, argv + 1);

  std::cout << "use to connect '" << *opt_ora_server <<
    "' as user '" << *opt_ora_user <<
    "'." << std::endl; 

  res += check_timeouts(
    ConnectionDescription(
      opt_ora_user->c_str(),
      opt_ora_pwd->c_str(),
      opt_ora_server->c_str()),
    *opt_loop_num,
    opt_no_timeout.enabled(),
    opt_debug_memory.enabled());
  
  return res;
}

