// @file Oracle/StatementTest.cpp

#include <iostream>

#include <Generics/AppUtils.hpp>
#include <Commons/Oracle/Environment.hpp>
#include <Commons/Oracle/Connection.hpp>
#include <Commons/Oracle/Statement.hpp>
#include <Commons/Oracle/ResultSet.hpp>

using namespace AdServer;

struct ConnHolder: public ReferenceCounting::DefaultImpl<>
{
  virtual ~ConnHolder() noexcept
  {}
  
  Commons::Oracle::Connection_var conn;
};

typedef ReferenceCounting::SmartPtr<ConnHolder> ConnHolder_var;

Generics::Singleton<ConnHolder, ConnHolder_var> conn_holder;

namespace
{

  const char DEFAULT_DB[] = "//oraclept/addbpt.ocslab.com";
  Generics::AppUtils::StringOption db(DEFAULT_DB);

  const char DEFAULT_USER[] = "ads_3";
  Generics::AppUtils::StringOption user(DEFAULT_USER);

  const char DEFAULT_PASSWORD[] = "adserver";
  Generics::AppUtils::StringOption password(DEFAULT_PASSWORD);

  const char DEFAULT_QUERY[] = "";
  Generics::AppUtils::StringOption query(
    DEFAULT_QUERY);

  const char DEFAULT_SQL[] = "";
  Generics::AppUtils::StringOption sql(
    DEFAULT_SQL);

  Generics::AppUtils::Option<unsigned long> opt_execute_timeout;

  const char USAGE[] =
    "Usage:\n" 
    "  OracleStatementTest --db=\"location\" -u user -p password -q query -s sql\n"
    "  --db : The location string to identify data base.\n"
    "  -u, --user : The user name.\n"
    "  -p, --password : The password of the user.\n"
    "  -q, --query : The text of SQL query (that fetching some data).\n"
    "  -s, --sql : The text of SQL statement (script).\n"
    "  -h           Show this help.";

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
}

void
init(int& argc, char**& argv)
  /*throw(eh::Exception)*/
{
  using namespace Generics::AppUtils;
  Args args;
  CheckOption opt_help;

  args.add(equal_name("db"), db);
  args.add(equal_name("user") || short_name("u"), user);
  args.add(equal_name("password") || short_name("p"), password);
  args.add(equal_name("query") || short_name("q"), query);
  args.add(equal_name("sql") || short_name("s"), sql);
  args.add(equal_name("help") || short_name("h"), opt_help);
  args.add(equal_name("timeout") || short_name("t"), opt_execute_timeout);
  args.parse(argc - 1, argv + 1);

  if (opt_help.enabled() || (!query.installed() && !sql.installed()))
  {
    std::cout << USAGE << std::endl;
  }
}

int
main(int argc, char* argv[]) noexcept
{
  enum
  {
    INIT,
    DO_QUERY,
    DO_SQL
  } status = INIT;

  try
  {
    init(argc, argv);

    Commons::Oracle::Environment_var occi_environment =
      Commons::Oracle::Environment::create_environment(
        static_cast<Commons::Oracle::Environment::EnvironmentMode>(
          Commons::Oracle::Environment::EM_THREADED_MUTEXED |
          Commons::Oracle::Environment::EM_OBJECT));

    Generics::Time execute_timeout(
      opt_execute_timeout.installed() ? *opt_execute_timeout : 0);

    Commons::Oracle::Connection_var connection =
      occi_environment->create_connection(
        Commons::Oracle::ConnectionDescription(
          (*user).c_str(),
          (*password).c_str(),
          (*db).c_str(),
          "", // default schema
          execute_timeout != Generics::Time::ZERO ? &execute_timeout : 0));

    conn_holder.instance().conn = connection;

    if (query.installed())
    {
      status = DO_QUERY;
      // perform query and fetch all data
      Commons::Oracle::Statement_var sql_query =
        connection->create_statement((*query).c_str());

      Commons::Oracle::ResultSet_var rs =
        sql_query->execute_query();

      while (rs->next());
    }
    if (sql.installed())
    {
      status = DO_SQL;
      // perform sql script
      Commons::Oracle::Statement_var sql_statement =
        connection->create_statement((*sql).c_str());

      sql_statement->execute();
    }    
  }
  catch (const eh::Exception& ex)
  {
    if (status == DO_QUERY)
    {
      std::cerr << "Failure with query:\n" << *query << std::endl;
    }
    if (status == DO_SQL)
    {
      std::cerr << "Failure with SQL:\n" << *sql << std::endl;
    }
    std::cerr << ex.what() << std::endl;
    return -1;
  }

  return 0;
}
