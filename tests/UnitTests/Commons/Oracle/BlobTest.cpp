#include <math.h>
#include <Generics/AppUtils.hpp>
#include <Generics/Proc.hpp>
#include <Commons/Oracle/Environment.hpp>

using namespace AdServer::Commons::Oracle;

namespace
{
  const char USAGE[] =
    "USAGE: OracleBlobTest [OPTIONS]"
    "Options: "
    "  --help : show help\n"
    "  --db : oracle database sid (//oracle.ocslab.com/addbdev.ocslab.com)\n"
    "  --user : oracle user name (adserver)\n"
    "  --pwd, --password : oracle user password (adserver)\n"
    "  --start : start size of blob, if doesn't use default cases for checking\n"
    "  --step  : step of incresing size of blob(0)\n"
    "  --iter  : count iteration (1)\n";
}

int check_blob(
  const char* db,
  const char* user,
  const char* pwd,
  unsigned long size_start,
  unsigned long size_step,
  unsigned long size_iters)
{
  std::string FUN;
  
  {
    std::ostringstream fun_str;
    fun_str << "check_blob(start size = " << size_start <<
      ", size step = " << size_step <<
      ", size iterations = " << size_iters << ")";
    FUN = fun_str.str();
  }
  
  int rownum = 0;
  try
  {
    Environment_var env = Environment::create_environment(
      (Environment::EnvironmentMode)(
        Environment::EM_THREADED_MUTEXED |
        Environment::EM_OBJECT /*|
        Environment::EM_EVENTS*/));

    Connection_var conn = env->create_connection(
      ConnectionDescription(user, pwd, db));

    try
    {
      Statement_var stmt = conn->create_statement(
        "DROP TABLE ADSERVER_BLOB_TEST_TABLE");

      stmt->execute();
    }
    catch(...)
    {}

    try
    {
      Statement_var stmt = conn->create_statement(
        "CREATE TABLE ADSERVER_BLOB_TEST_TABLE ("
          "value_size NUMBER, "
          "value BLOB )");

      stmt->execute();
    }
    catch(const eh::Exception& ex)
    {
      std::cerr << FUN << ": on creating blob table caught eh::Exception: "
        << ex.what() << std::endl;
    }
    
    {
      // insert set of blobs
      for(unsigned long i = 0; i < size_iters; ++i)
      {
        Statement_var stmt = conn->create_statement(
          "INSERT INTO ADSERVER_BLOB_TEST_TABLE ("
            "value_size, value ) VALUES( :1, :2)");

        unsigned long sz = size_start + size_step * i;
        stmt->set_uint(1, sz);
        std::string buf(sz, 'A');
        stmt->set_blob(2, Lob(buf.c_str(), sz, false));
        stmt->execute();
      }

      conn->commit();
    }
    
    {
      Generics::Timer exec_timer;
      exec_timer.start();

      Statement_var stmt = conn->create_statement(
        "SELECT value_size, value, length(value) "
        "FROM ADSERVER_BLOB_TEST_TABLE "
        "ORDER BY value_size");
      ResultSet_var result_set = stmt->execute_query();

      exec_timer.stop();
      
      Generics::Timer fetch_timer;
      fetch_timer.start();

      unsigned long expected_size = size_start;

      while(result_set->next())
      {
        Lob lob = result_set->get_blob(2);

        if(lob.length != expected_size)
        {
          std::cerr << FUN << ": unexpected blob size selected: " << lob.length <<
            " instead " << expected_size << ", row = " << rownum <<
            ", db blob size = " << result_set->get_uint(3) <<
            std::endl;
          return 1;
        }
        
#       define CHECK_BLOB_CONTENT
#       ifdef CHECK_BLOB_CONTENT
        for(unsigned long t = 0; t < expected_size; ++t)
        {
          if(static_cast<const char*>(lob.buffer)[t] != 'A')
          {
            std::cerr << FUN << ": incorrect lob content." << std::endl;
          }
        }
#       endif

        expected_size += size_step;
        ++rownum;
      }

      fetch_timer.stop();

      std::cout << FUN << ":" << std::endl <<
        "  exec time = " << exec_timer.elapsed_time() << std::endl <<
        "  fetch time = " << fetch_timer.elapsed_time() << std::endl <<
        "  size avg = " << (1 + size_iters) * size_start / 2 << std::endl <<
        "  fetch time avg = " <<
          fetch_timer.elapsed_time() / (size_iters ? size_iters : 1) <<
          std::endl <<
        "  count = " << size_iters << std::endl <<
        "  transfer speed = " << ((double)size_iters * (2*size_start + size_step * size_iters)/ 2 / (
          (double)fetch_timer.elapsed_time().tv_sec * 1000000 +
            fetch_timer.elapsed_time().tv_usec)) * 1000000/ (1024 * 1024) << " (Mb/sec) " <<
            std::endl;
    }
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << FUN << ": caught unexpected eh::Exception on row " << rownum
      << ": " << ex.what() << std::endl;
    return 1;
  }

  return 0;
}

int
main(int argc, char* argv[])
{
  int res = 0;
  
  Generics::AppUtils::StringOption opt_ora_server(
    "//oraads/addbads.ocslab.com");
  Generics::AppUtils::StringOption opt_ora_user("ads_3");
  Generics::AppUtils::StringOption opt_ora_pwd("adserver");
  Generics::AppUtils::StringOption opt_start;
  Generics::AppUtils::StringOption opt_step("0");
  Generics::AppUtils::StringOption opt_iter("1");

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
    Generics::AppUtils::equal_name("start"),
    opt_start);

  args.add(
    Generics::AppUtils::equal_name("step"),
    opt_step);

  args.add(
    Generics::AppUtils::equal_name("iter"),
    opt_iter);

  args.parse(argc - 1, argv + 1);

  std::cout <<  "Use to connect '" << opt_ora_server->c_str() 
    << "' as user '" << opt_ora_user->c_str()
    << "' with password '" <<  opt_ora_pwd->c_str() << "'." << std::endl; 

  if(opt_start.installed())
  {
    unsigned long start, step, iteration;
    start = strtol(opt_start->c_str(), 0, 10);
    step = strtol(opt_step->c_str(), 0, 10);
    iteration = strtol(opt_iter->c_str(), 0, 10);

    res = check_blob(
      opt_ora_server->c_str(),
      opt_ora_user->c_str(),
      opt_ora_pwd->c_str(),
      start,
      step,
      iteration);
  }
  else
  {

    res += check_blob(
      opt_ora_server->c_str(),
      opt_ora_user->c_str(),
      opt_ora_pwd->c_str(),
      66000,
      0,
      1);

    res += check_blob(
      opt_ora_server->c_str(),
      opt_ora_user->c_str(),
      opt_ora_pwd->c_str(),
      1000,
      1000,
      56);

    res += check_blob(
      opt_ora_server->c_str(),
      opt_ora_user->c_str(),
      opt_ora_pwd->c_str(),
      100000,
      10000,
      50);
    
    res += check_blob(
      opt_ora_server->c_str(),
      opt_ora_user->c_str(),
      opt_ora_pwd->c_str(),
      20*1024*1024,
      0,
      1);
  }

  return res;
}

