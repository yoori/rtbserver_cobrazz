#include <math.h>
#include <Generics/AppUtils.hpp>
#include <Generics/Proc.hpp>
#include <Generics/Decimal.hpp>
#include <Commons/Oracle/Environment.hpp>

using namespace AdServer::Commons::Oracle;

namespace
{
  const char USAGE[] =
    "USAGE: OracleExecuteTest [OPTIONS]"
    "Options: "
    "  --help : show help\n"
    "  --db : oracle database sid (//oracle.ocslab.com/addbdev.ocslab.com)\n"
    "  --user : oracle user name (adserver)\n"
    "  --pwd, --password : oracle user password (adserver)";
}

struct ActiveObjectCallbackEmpty:
  public Generics::ActiveObjectCallback,
  public ReferenceCounting::AtomicImpl
{
  virtual void
  report_error(
    Generics::ActiveObject*,
    Generics::ActiveObjectCallback::Severity,
    const String::SubString&, const char*) noexcept
  {}
};

struct TestObject: public Object
{
  typedef Generics::Decimal<uint32_t, 22, 3> DecimalType;
  
  TestObject(
    unsigned long op1_ulong,
    long op2_long,
    double op3_double,
    const char* op5_string,
    const Generics::Time& op6_date,
    const Generics::Time& op7_timestamp,
    const DecimalType& op8_decimal)
    : op1_ulong_(op1_ulong),
      op2_long_(op2_long),
      op3_double_(op3_double),
      op5_string_(op5_string),
      op6_date_(op6_date),
      op7_timestamp_(op7_timestamp),
      op8_decimal_(op8_decimal)
  {}

  virtual ~TestObject() noexcept {}
  
  virtual const char* getSQLTypeName() const /*throw(eh::Exception)*/
  {
    return "ADSERVER_ORACLE_TEST_TYPE";
  }

  virtual void writeSQL(SqlStream& stream)
    /*throw(eh::Exception, SqlException)*/
  {
    stream.set_ulong(op1_ulong_);
    stream.set_long(op2_long_);
    stream.set_double(op3_double_);
    stream.set_string(op5_string_);
    stream.set_date(op6_date_);
    stream.set_date(op7_timestamp_);
    stream.set_null(AdServer::Commons::Oracle::Statement::DATE);
    stream.set_decimal(op8_decimal_);
  }
  
  virtual void readSQL(SqlStream& /*stream*/)
    /*throw(eh::Exception, SqlException)*/
  {}

  std::ostream& print(std::ostream& out, const char* space) const
  {
    out <<
      space << "op_ulong = " << op1_ulong_ << std::endl <<
      space << "op_long = " << op2_long_ << std::endl <<
      space << "op_double = " << op3_double_ << std::endl <<
      space << "op_string = " << op5_string_ << std::endl <<
      space << "op_date = " << op6_date_ << std::endl <<
      space << "op_timestamp = " << op7_timestamp_ << std::endl <<
      space << "op_decimal = " << op8_decimal_.str() << std::endl;
    
    return out;
  }
  
  bool operator==(const TestObject& right) const
  {
    return op1_ulong_ == right.op1_ulong_ &&
      op2_long_ == right.op2_long_ &&
      ::fabs(op3_double_ - right.op3_double_) < 0.00000001 &&
      op5_string_ == right.op5_string_ &&
      op6_date_ == right.op6_date_ &&
      op7_timestamp_ == right.op7_timestamp_ &&
      op8_decimal_ == right.op8_decimal_;
  }
  
  unsigned long op1_ulong_;
  long op2_long_;
  double op3_double_;
  std::string op5_string_;
  Generics::Time op6_date_;
  Generics::Time op7_timestamp_;
  DecimalType op8_decimal_;
};

typedef ReferenceCounting::SmartPtr<TestObject> TestObject_var;

int check_auto_channel_ops(
  const char* db,
  const char* user,
  const char* pwd)
{
//static const char* FUN = "check_auto_channel_ops()";
  
  try
  {
    Environment_var env = Environment::create_environment(
      (Environment::EnvironmentMode)(
        Environment::EM_THREADED_MUTEXED |
        Environment::EM_OBJECT /*|
        Environment::EM_EVENTS*/));

    Connection_var conn = env->create_connection(
      ConnectionDescription(user, pwd, db));

    enum
    {
      ACCOUNT_ID = 1,
      CHANNEL_NAME,
      CHANNEL_QUERY,
      CHANNEL_ANNOTATION,
      TRIGGER_LIST_CONTENT,
      LANGUAGE,
      NEWS_COUNT,
      ANNOTATION_STEM
    };

    Statement_var stmt = conn->create_statement(
      "BEGIN "
        "ADSERVERAUTOCHANNEL.CREATE_CHANNEL(:1, :2, :3, :4, :5, :6, :7, :8); "
      "END;");

    const char keyword[] = "test";
    const char language[] = "en";
    unsigned long account_id_ = 5647;
    unsigned long text_count = 0;
    stmt->set_string(CHANNEL_QUERY, keyword);
    stmt->set_blob(TRIGGER_LIST_CONTENT, Lob(keyword, strlen(keyword), false));
    stmt->set_uint(ACCOUNT_ID, account_id_);
    stmt->set_string(CHANNEL_NAME, keyword);
    stmt->set_string(CHANNEL_ANNOTATION, keyword);
    stmt->set_string(LANGUAGE, language);
    stmt->set_uint(NEWS_COUNT, text_count);
    stmt->set_string(ANNOTATION_STEM, keyword);
    stmt->execute();

    conn->commit();
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << "ERROR: " << ex.what() << std::endl;
  }

  return 0;
}

int check_object_ops(
  bool perf_mode,
  const char* db,
  const char* user,
  const char* pwd)
{
  static const char* FUN = "check_object_ops()";
  
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
        "DROP PACKAGE ADSERVER_ORACLE_TEST_PACKAGE");
      stmt->execute();
    }
    catch(...)
    {}
    
    try
    {
      Statement_var stmt = conn->create_statement(
        "DROP TYPE ADSERVER_ORACLE_TEST_COLL");
      stmt->execute();
    }
    catch(...)
    {}

    try
    {
      Statement_var stmt = conn->create_statement(
        "DROP TYPE ADSERVER_ORACLE_TEST_TYPE");
      stmt->execute();
    }
    catch(...)
    {}
      
    try
    {
      Statement_var stmt = conn->create_statement(
        "DROP TABLE ADSERVER_ORACLE_TEST_TABLE");
      stmt->execute();
    }
    catch(...)
    {}

    try
    {
      Statement_var stmt = conn->create_statement(
        "CREATE TABLE ADSERVER_ORACLE_TEST_TABLE ("
          "op_ulong NUMBER, "
          "op_long NUMBER, "
          "op_double NUMBER, "
          "op_string VARCHAR2(1024),"
          "op_date DATE, "
          "op_timestamp TIMESTAMP, "
          "op_null_date DATE, "
          "op_decimal NUMBER)");

      stmt->execute();
    }
    catch(...)
    {}

    std::cout << FUN << ": created ADSERVER_ORACLE_TEST_TABLE table." << std::endl;

    try
    {
      Statement_var stmt = conn->create_statement(
        "CREATE OR REPLACE TYPE ADSERVER_ORACLE_TEST_TYPE AS OBJECT ("
          "op_ulong NUMBER, "
          "op_long NUMBER, "
          "op_double NUMBER, "
          "op_string VARCHAR2(1024),"
          "op_date DATE, "
          "op_timestamp TIMESTAMP, "
          "op_null_date DATE, "
          "op_decimal NUMBER "
          ")");
      stmt->execute();
    }
    catch(...)
    {
      std::cerr << FUN << ": can't create ADSERVER_ORACLE_TEST_TYPE object type." << std::endl;
      throw;
    }

    std::cout << FUN << ": created ADSERVER_ORACLE_TEST_TYPE object type." << std::endl;

    {
      Statement_var stmt = conn->create_statement(
        "CREATE OR REPLACE TYPE ADSERVER_ORACLE_TEST_COLL AS "
          "TABLE OF ADSERVER_ORACLE_TEST_TYPE");
      stmt->execute();
    }

    std::cout << FUN << ": created ADSERVER_ORACLE_TEST_COLL collection type." << std::endl;

    {
      Statement_var stmt = conn->create_statement(
        "CREATE OR REPLACE PACKAGE ADSERVER_ORACLE_TEST_PACKAGE AS "
          "PROCEDURE test(coll ADSERVER_ORACLE_TEST_COLL); "
        "END;");
      stmt->execute();
    }

    std::cout << FUN << ": created ADSERVER_ORACLE_TEST_PACKAGE package." << std::endl;

    {
      Statement_var stmt = conn->create_statement(
        "CREATE OR REPLACE PACKAGE BODY ADSERVER_ORACLE_TEST_PACKAGE AS "
          "PROCEDURE test(coll ADSERVER_ORACLE_TEST_COLL) IS "
            "i NUMBER; "
          "BEGIN "
            "IF coll.COUNT > 0 THEN "
              "FOR i IN coll.FIRST .. coll.LAST LOOP "
                "INSERT INTO ADSERVER_ORACLE_TEST_TABLE ("
                  "op_ulong, "
                  "op_long, "
                  "op_double, "
                  "op_string, "
                  "op_date, "
                  "op_timestamp, "
                  "op_null_date, "
                  "op_decimal "
                  ") "
                "VALUES("
                  "coll(i).op_ulong, "
                  "coll(i).op_long, "
                  "coll(i).op_double, "
                  "coll(i).op_string, "
                  "coll(i).op_date, "
                  "coll(i).op_timestamp, "
                  "coll(i).op_null_date, "
                  "coll(i).op_decimal "
                  "); "
              "END LOOP; "
            "END IF;"
          "END; "
        "END;");
      stmt->execute();
    }
          
    std::cout << FUN << ": created ADSERVER_ORACLE_TEST_PACKAGE package body." << std::endl;

    /* bind array of objects */
    std::vector<TestObject_var> vec;
    Generics::Time date(String::SubString("01:01:01 2009-01-01"),
      "%H:%M:%S %Y-%m-%d");
    Generics::Time timestamp(date);
    timestamp.tv_usec = 1111;

    int objects_count = perf_mode ? 30000 : 3;

    for(int i = 0; i < objects_count / 3; ++i)
    {
      vec.push_back(
        new TestObject(
          i*3 + 1, -1, 1.1, "test1",
          date, timestamp, TestObject::DecimalType(String::SubString("111.111"))));
      TestObject::DecimalType d(String::SubString("11111111111111111.111"));

      vec.push_back(
        new TestObject(
          i*3 + 2, -2, 0.0001, "test2",
          date, timestamp, d));
      vec.push_back(
        new TestObject(
          i*3 + 3, 1, 0.000001, "",
          date, timestamp, TestObject::DecimalType(String::SubString("0.111"))));
    }

    Generics::CPUTimer cpu_timer;
    cpu_timer.start();

    {
      Statement_var stmt = conn->create_statement(
        "BEGIN "
          "ADSERVER_ORACLE_TEST_PACKAGE.test(:1); "
        "END;");
      std::vector<Object_var> ovec;
      for(std::vector<TestObject_var>::const_iterator it = vec.begin();
          it != vec.end(); ++it)
      {
        ovec.push_back(*it);
      }
      stmt->set_array(1, ovec, "ADSERVER_ORACLE_TEST_COLL");
      stmt->execute();
      conn->commit();
    }

    cpu_timer.stop();

    std::cout << FUN << ": executed array insert, cpu time = " <<
      cpu_timer.elapsed_time() << std::endl;
    
    {
      std::vector<TestObject_var> check_vec;
      Statement_var stmt = conn->create_statement(
        "SELECT "
          "op_ulong, op_long, op_double, "
          "op_string, op_date, op_timestamp, op_null_date, "
          "op_decimal "
        "FROM ADSERVER_ORACLE_TEST_TABLE ORDER BY op_ulong");
      ResultSet_var result_set = stmt->execute_query();

      while(result_set->next())
      {
        if(!result_set->is_null(7))
        {
          std::cerr << FUN << ": ERROR: not null selected for op_null_date: " <<
            result_set->get_timestamp(8).get_gm_time() <<
            std::endl;
        }

        check_vec.push_back(
          new TestObject(
            result_set->get_uint(1),
            result_set->get_int(2),
            result_set->get_double(3),
            result_set->get_string(4).c_str(),
            result_set->get_date(5),
            result_set->get_timestamp(6),
            result_set->get_decimal<TestObject::DecimalType>(8)));
      }

      if(check_vec.size() != vec.size())
      {
        std::cerr << FUN << ": ERROR: incorrect result size." << std::endl;
      }

      std::vector<TestObject_var>::const_iterator check_it = check_vec.begin();
      for(std::vector<TestObject_var>::const_iterator it = vec.begin();
          it != vec.end(); ++it, ++check_it)
      {
        if(!(*(*it) == *(*check_it)))
        {
          std::cerr << FUN << ": ERROR: unexpected value selected #" <<
            (it - vec.begin()) << ": " << std::endl;
          (*check_it)->print(std::cerr, "    ") << std::endl;
          std::cerr << "  Instead: " << std::endl;
          (*it)->print(std::cerr, "    ") << std::endl;
        }
      }
    }
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << FUN << ": caught unexpected eh::Exception: " << ex.what() << std::endl;
    return 1;
  }

  return 0;
}

int check_object_ops_memory_holding(
  const char* db,
  const char* user,
  const char* pwd)
{
  static const char* FUN = "check_object_ops_memory_holding()";
  
  try
  {
    /* check OCI memory holding */
    unsigned long base_vsize, base_rss;
    Generics::Proc::memory_status(base_vsize, base_rss);
    
    std::cout << FUN << ": to check memory holding: VSIZE = " << base_vsize
      << ", RSS = " << base_rss << std::endl;

    Environment_var env = Environment::create_environment(
      (Environment::EnvironmentMode)(
        Environment::EM_THREADED_MUTEXED |
        Environment::EM_OBJECT /*|
        Environment::EM_EVENTS*/));

    Connection_var conn = env->create_connection(
      ConnectionDescription(user, pwd, db));

    unsigned long cur_vsize, cur_rss;

    for(int g = 0; g < 15; ++g)
    {
      for(int i = 0; i < 5; ++i)
      {
        /* bind array of objects */
        std::vector<TestObject_var> vec;
        Generics::Time date(String::SubString("01:01:01 2009-01-01"),
          "%H:%M:%S %Y-%m-%d");
        Generics::Time timestamp(date);
        timestamp.tv_usec = 1111;

        for(int j = 0; j < 1000; ++j)
        {
          vec.push_back(
            new TestObject(
              1, -1, 1.1, "test1",
              date, timestamp,
              TestObject::DecimalType(String::SubString("111.111"))));
        }
    
        {
          Statement_var stmt = conn->create_statement(
            "BEGIN "
              "ADSERVER_ORACLE_TEST_PACKAGE.test(:1); "
            "END;");
          std::vector<Object_var> ovec;
          for(std::vector<TestObject_var>::const_iterator it = vec.begin();
              it != vec.end(); ++it)
          {
            ovec.push_back(*it);
          }
          stmt->set_array(1, ovec, "ADSERVER_ORACLE_TEST_COLL");
          stmt->execute();
          conn->commit();
        }

        {
          Statement_var stmt = conn->create_statement(
            "DELETE FROM ADSERVER_ORACLE_TEST_TABLE");
          stmt->execute();
          conn->commit();
        }
      }

      Generics::Proc::memory_status(cur_vsize, cur_rss);
      std::cout << "step #" << g << ": delta VSIZE = " << (cur_vsize - base_vsize)
        << ", delta RSS = " << (cur_rss - base_rss) << std::endl;

/*
      if(cur_rss - base_rss > 450000)
      {
        std::cerr << FUN << ": delta RSS = " << (cur_rss - base_rss)
          << " greatest then expected 450000" << std::endl;
        return 1;
      }
      */
    }

    return 0;
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << FUN << ": caught eh::Exception: " << ex.what() << std::endl;
    return 1;
  } 
}

int check_result_set(
  const char* db,
  const char* user,
  const char* pwd)
{
  static const char* FUN = "check_result_set()";

  try
  {
    {
      Environment_var env = Environment::create_environment(
        (Environment::EnvironmentMode)(
          Environment::EM_THREADED_MUTEXED |
          Environment::EM_OBJECT /*|
          Environment::EM_EVENTS*/));

      std::cout << FUN << ": connect to '" << db << "' as user '" << user
      << "' with password '" << pwd << "'." << std::endl; 
      Connection_var conn = env->create_connection(
        ConnectionDescription(user, pwd, db));

      try
      {
        Statement_var stmt = conn->create_statement(
          "DROP TABLE ADSERVER_ORACLE_TEST_CASE2");
        stmt->execute();
      }
      catch(...)
      {}

      try
      {
        Statement_var stmt = conn->create_statement(
          "DROP TYPE ADSERVER_ORACLE_TEST_STR_COLL");
        stmt->execute();
      }
      catch(...)
      {}
      
      {
        Statement_var stmt = conn->create_statement(
          "CREATE OR REPLACE TYPE ADSERVER_ORACLE_TEST_STR_COLL AS "
            "TABLE OF VARCHAR2(1024)");
        stmt->execute();
      }

      std::cout << FUN << ": create ADSERVER_ORACLE_TEST_CASE2 table." << std::endl;

      {
        Statement_var stmt = conn->create_statement(
          "CREATE TABLE ADSERVER_ORACLE_TEST_CASE2 ("
            "p1 NUMBER,"
            "p2 VARCHAR2(1000),"
            "p3 DATE, "
            "p4 TIMESTAMP, "
            "p5 BLOB, "
            "p6 DATE "
            ")");
        stmt->execute();
      }

      std::cout << FUN << ": check empty result set." << std::endl;
      
      {
        // empty result set test
        Statement_var stmt = conn->create_statement(
          "SELECT p1 FROM ADSERVER_ORACLE_TEST_CASE2");
        ResultSet_var result_set = stmt->execute_query();
        int count = 0;

        while(result_set->next())
        {
          ++count;
        }
        if(count > 0)
        {
          std::cerr << "ERROR: empty result set error" << std::endl;
        }
      }

      std::cout << FUN << ": insert control records." << std::endl;

      Generics::Time time1(String::SubString("1999-01-02 01:02:03"),
        "%Y-%m-%d %H:%M:%S");
      Generics::Time time2(String::SubString("1999-01-02 01:02:03"),
        "%Y-%m-%d %H:%M:%S");
      time2.tv_usec = 1111;
      
      for(unsigned long i = 0; i < 30; ++i)
      {
        Statement_var stmt = conn->create_statement(
          "INSERT INTO ADSERVER_ORACLE_TEST_CASE2 ("
            "p1, p2, p3, p4, p5, p6) VALUES("
            ":1, :2, :3, :4, :5, :6)");
        stmt->set_uint(1, i);
        if(i % 2 == 0)
        {
          stmt->set_string(2, "TT");
        }
        else
        {
          stmt->set_null(2, Statement::STRING);
        }
        
        stmt->set_date(3, time1);
        stmt->set_timestamp(4, time2);
        stmt->set_blob(5, Lob("TEST", 4, false));
        stmt->set_null(6, Statement::DATE);
        stmt->execute();
      }
      
      for(unsigned long i = 9999999; i < 10000002; ++i)
      {
        Statement_var stmt = conn->create_statement(
          "INSERT INTO ADSERVER_ORACLE_TEST_CASE2 ("
            "p1, p2, p3, p4, p5, p6) VALUES("
            ":1, :2, :3, :4, :5, :6)");
        stmt->set_uint(1, i);
        if(i % 2 == 0)
        {
          stmt->set_string(2, "TT2");
        }
        else
        {
          stmt->set_null(2, Statement::STRING);
        }
        
        stmt->set_date(3, time1);
        stmt->set_timestamp(4, time2);
        stmt->set_blob(5, Lob("TEST", 4, false));
        stmt->set_null(6, Statement::DATE);
        stmt->execute();
      }

      conn->commit();

      std::cout << FUN << ": select control records." << std::endl;

      {
        Statement_var stmt = conn->create_statement(
          "SELECT p1, p2, p3, p4, p5 "
          "FROM ADSERVER_ORACLE_TEST_CASE2 WHERE p1 < 10000 order by p1");
        ResultSet_var result_set = stmt->execute_query();
        unsigned long count = 0;
        while(result_set->next())
        {
          std::ostringstream count_str;
          count_str << TestObject::DecimalType(false, count, 0).str();
          
          unsigned long p1_uint = result_set->get_uint(1);
          double p1_double = result_set->get_double(1);
          TestObject::DecimalType p1_num =
            result_set->get_decimal<TestObject::DecimalType>(1);
          std::string p1_num_str = p1_num.str();

          if(p1_uint != count ||
             ::fabs(p1_double - count) > 0.0001 ||
             p1_num_str != count_str.str())
          {
            std::cerr << "incorrect number selected: "
                      << "(uint) = " << p1_uint
                      << ", (double) = " << p1_double
                      << ", (number) = " << p1_num_str << std::endl;
          }

          if(count % 2 == 0)
          {
            if(result_set->is_null(2))
            {
              std::cerr << "incorrect string selected: 'NULL' instead 'TT'" << std::endl;
            }
            else
            {
              std::string p2 = result_set->get_string(2);
              if(p2 != "TT")
              {
                std::cerr << "incorrect string selected: '" << p2 << "' instead 'TT'" << std::endl;
              }
            }
          }
          else
          {
            if(!result_set->is_null(2))
            {
              std::cerr << "incorrect string selected: '"
                << result_set->get_string(2) << "' instead null" << std::endl;
            }
          }
          
          Generics::Time p3 = result_set->get_date(3);
          if(p3 != time1)
          {
            std::cerr << "incorrect time(p3) selected: '"
                      << p3.get_gm_time()
                      << "' instead '"
                      << time1.get_gm_time() << "'" << std::endl;
          }
          
          Generics::Time p4 = result_set->get_timestamp(4);
          if(p4 != time2)
          {
            std::cerr << "incorrect timestamp(p4) selected: '"
                      << p4.get_gm_time()
                      << "' instead '"
                      << time2.get_gm_time() << "'" << std::endl;
          }

          Lob p5_lob = result_set->get_blob(5);
          if(p5_lob.length != 4 &&
             strncmp(p5_lob.buffer, "TEST", 4) != 0)
          {
            std::cerr << "incorrect blob(p5) selected" << std::endl;
          }
          
          ++count;
        }

        if(count != 30)
        {
          std::cerr << "selected " << count << " records instead 30." << std::endl;
        }
      }

      std::cout << FUN << ": select with number array binding." << std::endl;

      {
        Statement_var stmt = conn->create_statement(
          "SELECT p1 FROM ADSERVER_ORACLE_TEST_CASE2 WHERE p1 IN "
            "(SELECT * FROM TABLE(:1))"
            "ORDER BY p1");
        std::vector<unsigned long> vec;
        vec.push_back(1);
        stmt->set_array(1, vec, "AD_IDLIST");
        ResultSet_var result_set = stmt->execute_query();
        int count = 0;
        while(result_set->next())
        {
          ++count;
        }
        if(count != 1)
        {
          std::cerr << "selected " << count << " records instead 1 (array bind)." << std::endl;
        }
      }

      std::cout << FUN << ": select with number array binding (2)." << std::endl;

      {
        Statement_var stmt = conn->create_statement(
          "SELECT p1 FROM ADSERVER_ORACLE_TEST_CASE2 WHERE p1 IN "
            "(SELECT * FROM TABLE(:1))"
            "ORDER BY p1");
        std::vector<unsigned long> vec;
        vec.push_back(1);
        vec.push_back(2);
        vec.push_back(3);
        stmt->set_array(1, vec, "AD_IDLIST");
        ResultSet_var result_set = stmt->execute_query();
        int count = 0;
        while(result_set->next())
        {
          ++count;
        }
        if(count != 3)
        {
          std::cerr << "selected " << count << " records instead 3 (array bind with size 3)." << std::endl;
        }
      }

      std::cout << FUN << ": select with big numbers array binding (3)." << std::endl;

      {
        Statement_var stmt = conn->create_statement(
          "SELECT p1 FROM ADSERVER_ORACLE_TEST_CASE2 WHERE p1 IN "
            "(SELECT * FROM TABLE(:1))"
            "ORDER BY p1");
        std::vector<unsigned long> vec;
        vec.push_back(9999999);
        vec.push_back(10000000);
        vec.push_back(10000001);
        stmt->set_array(1, vec, "AD_IDLIST");
        ResultSet_var result_set = stmt->execute_query();
        int count = 0;
        while(result_set->next())
        {
          ++count;
        }
        if(count != 3)
        {
          std::cerr << "selected " << count << " records instead 3 (array bind with size 3)." << std::endl;
        }
      }

      std::cout << FUN << ": select empty result set with number array binding." << std::endl;

      {
        Statement_var stmt = conn->create_statement(
          "SELECT p1 FROM ADSERVER_ORACLE_TEST_CASE2 WHERE p1 IN "
            "(SELECT * FROM TABLE(:1))"
            "ORDER BY p1");
        std::vector<unsigned long> vec;
        stmt->set_array(1, vec, "AD_IDLIST");
        ResultSet_var result_set = stmt->execute_query();
        int count = 0;
        while(result_set->next())
        {
          ++count;
        }
        if(count != 0)
        {
          std::cerr << "selected " << count << " records instead 1 (empty array bind)."
            << std::endl;
        }
      }

      std::cout << FUN << ": select result set with string array binding." << std::endl;

      {
        Statement_var stmt = conn->create_statement(
          "SELECT p2 FROM ADSERVER_ORACLE_TEST_CASE2 WHERE p2 IN "
            "(SELECT * FROM TABLE(:1))");

        std::vector<std::string> vec;
        vec.push_back("TT");
        stmt->set_array(1, vec, "ADSERVER_ORACLE_TEST_STR_COLL");
        ResultSet_var result_set = stmt->execute_query();
        int count = 0;
        while(result_set->next())
        {
          std::string val = result_set->get_string(1);
          
          if(val != "TT")
          {
            std::cerr << "selected string '" << val << "' records instead 'TT'."
              << std::endl;
            return 1;
          }
          ++count;
        }
        if(count != 15)
        {
          std::cerr << "selected " << count << " records instead 15."
            << std::endl;
          return 1;
        }
      }
    }
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << FUN << ": caught unexpected eh::Exception: " << ex.what() << std::endl;
    return 1;
  }
  
  return 0;
}

int check_result_set_memory_holding(
  const char* db,
  const char* user,
  const char* pwd)
{
  static const char* FUN = "check_result_set_memory_holding()";

  try
  {
    Environment_var env = Environment::create_environment(
      (Environment::EnvironmentMode)(
        Environment::EM_THREADED_MUTEXED |
        Environment::EM_OBJECT /*|
        Environment::EM_EVENTS*/));

    Connection_var conn = env->create_connection(
      ConnectionDescription(user, pwd, db));
    
    {
      // simple memory holding test
      unsigned long base_vsize_u, base_rss_u;
      Generics::Proc::memory_status(base_vsize_u, base_rss_u);
      long base_rss = static_cast<long>(base_rss_u);
      long base_vsize = static_cast<long>(base_vsize_u);
      long prev_rss = 0;
    
      std::cout << FUN << ": to check memory holding: VSIZE = " << base_vsize
         << ", RSS = " << base_rss << std::endl;

      for(int g = 0; g < 6; ++g)
      {
        // memory holding test
        Generics::Timer timer;
        timer.start();

        const unsigned long COUNT = 100;
      
        for(unsigned long i = 0; i < COUNT; ++i)
        {
          Statement_var stmt = conn->create_statement(
            "SELECT "
              "op_ulong, op_long, op_double, "
              "op_string, op_date, op_timestamp, op_string, op_decimal "
            "FROM ADSERVER_ORACLE_TEST_TABLE ORDER BY op_ulong");
          ResultSet_var result_set = stmt->execute_query();

          while(result_set->next())
          {
            result_set->get_uint(1);
            result_set->get_int(2);
            result_set->get_double(3);
        
            result_set->get_string(4).c_str();
            result_set->get_date(5);
            result_set->get_timestamp(6);
            result_set->get_decimal<TestObject::DecimalType>(8);
          }
        }

        timer.stop();

        unsigned long cur_vsize_u, cur_rss_u;
        Generics::Proc::memory_status(cur_vsize_u, cur_rss_u);
        long cur_rss = static_cast<long>(cur_rss_u);
        long cur_vsize = static_cast<long>(cur_vsize_u);

        std::cout << "step #" << g << ": execution time = "
          << timer.elapsed_time() << "(avg = "
          << (timer.elapsed_time() / COUNT)
          << "), delta VSIZE = " << (cur_vsize - base_vsize)
          << ", delta RSS = " << (cur_rss - base_rss)
          << std::endl;

        if(cur_rss - base_rss > 220000)
        {
          std::cerr << FUN << ": delta RSS = " << (cur_rss - base_rss)
            << " greatest then expected 220000" << std::endl;
          return 1;
        }
      
        if(g > 4 && cur_rss - prev_rss != 0)
        {
          std::cerr << FUN << ": RSS continue grow" << std::endl;
          return 1;
        }
      
        prev_rss = cur_rss;
      }
    }
    
    {
      // memory holding by array binding test
      unsigned long base_vsize_u, base_rss_u;
      Generics::Proc::memory_status(base_vsize_u, base_rss_u);
      long base_vsize = static_cast<long>(base_vsize_u);
      long base_rss = static_cast<long>(base_rss_u);
      long prev_rss = 0;
    
      std::cout << FUN << ": to check memory holding (with array binding): VSIZE = " << base_vsize
         << ", RSS = " << base_rss << std::endl;

      for(int g = 0; g < 6; ++g)
      {
        Generics::Timer timer;
        timer.start();

        const unsigned long COUNT = 100;

        for(unsigned long i = 0; i < COUNT; ++i)
        {
          Statement_var stmt = conn->create_statement(
            "SELECT "
              "op_ulong, op_long, op_double, "
              "op_string, op_date, op_timestamp, op_string "
            "FROM ADSERVER_ORACLE_TEST_TABLE "
            "WHERE op_ulong IN (SELECT * FROM TABLE(:1)) "
            "ORDER BY op_ulong");

          std::vector<unsigned long> vec;
          for(int i = 0; i < 200; ++i)
          {
            vec.push_back(i);
          }
          
          stmt->set_array(1, vec, "AD_IDLIST");

          ResultSet_var result_set = stmt->execute_query();

          while(result_set->next())
          {
            result_set->get_uint(1);
            result_set->get_int(2);
            result_set->get_double(3);
        
            result_set->get_string(4).c_str();
            result_set->get_date(5);
            result_set->get_timestamp(6);
          }
        }

        timer.stop();

        unsigned long cur_vsize_u, cur_rss_u;
        Generics::Proc::memory_status(cur_vsize_u, cur_rss_u);
        long cur_vsize = static_cast<long>(cur_vsize_u);
        long cur_rss = static_cast<long>(cur_rss_u);

        std::cout << "step #" << g << ": execution time = "
          << timer.elapsed_time() << "(avg = "
          << (timer.elapsed_time() / COUNT)
          << "), delta VSIZE = " << (cur_vsize - base_vsize)
          << ", delta RSS = " << (cur_rss - base_rss)
          << std::endl;

        /*
        if(cur_rss - base_rss > 100000)
        {
          std::cerr << FUN << ": delta RSS = " << (cur_rss - base_rss)
            << " greatest then expected 100000" << std::endl;
          return 1;
        }
        */
        
        if(g > 4 && cur_rss - prev_rss != 0)
        {
          std::cerr << FUN << ": RSS continue grow" << std::endl;
          return 1;
        }
      
        prev_rss = cur_rss;
      }
    }

    return 0;
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << FUN << ": caught unexpected eh::Exception: " << ex.what() << std::endl;
    return 1;
  }
}

int check_time_ops(
  const char* db,
  const char* user,
  const char* pwd)
{
  static const char* FUN = "check_time_ops()";

  try
  {
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
          "DROP TABLE ADSERVER_ORACLE_TEST_TS");
        stmt->execute();
      }
      catch(...)
      {}

      std::cout << FUN << ": create ADSERVER_ORACLE_TEST_TS table." << std::endl;

      {
        Statement_var stmt = conn->create_statement(
          "CREATE TABLE ADSERVER_ORACLE_TEST_TS ("
            "p1 TIMESTAMP,"
            "p2 TIMESTAMP"
            ")");
        stmt->execute();
      }

      {
        Statement_var stmt = conn->create_statement(
          "INSERT INTO ADSERVER_ORACLE_TEST_TS ("
            "p1, p2) VALUES("
            "to_date('2008-08-04 17:01:28', 'YYYY.MM.DD HH24:MI:SS'), "
            "to_date('2008-08-04 17:01:28', 'YYYY.MM.DD HH24:MI:SS'))");
        stmt->execute();
      }

      conn->commit();

      std::cout << FUN << ": select result set for greatest timestamp." << std::endl;

      {
        Statement_var stmt = conn->create_statement(
          "SELECT p1 FROM ADSERVER_ORACLE_TEST_TS");

        ResultSet_var result_set = stmt->execute_query();

        while(result_set->next())
        {
          Generics::Time val = result_set->get_timestamp(1);
          if(val != Generics::Time(String::SubString("2008-08-04 17:01:28"),
            "%Y-%m-%d %H:%M:%S"))
          {        
            std::cerr << "Incorrect time selected: "
              << val.get_gm_time()
              << " instead 2008-08-04 17:01:28" << std::endl;
            return 1;
          }
        }
      }
    }
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << FUN << ": caught unexpected eh::Exception: " << ex.what() << std::endl;
    return 1;
  }
  
  return 0;
}

int check_ADSC_2791(
  const char* db,
  const char* user,
  const char* pwd,
  const char* name)
{
  static const char* FUN = "check_ADSC_2791()";

  try
  {
    {
      Environment_var env = Environment::create_environment(
        (Environment::EnvironmentMode)(
          Environment::EM_THREADED_MUTEXED |
          Environment::EM_OBJECT /*|
          Environment::EM_EVENTS*/));

      Connection_var conn = env->create_connection(
        ConnectionDescription(user, pwd, db));

      std::cout << FUN << ": to insert channel." << std::endl;
      Statement_var stmt = conn->create_statement(
        "BEGIN "
          "DELETE FROM CHANNEL WHERE DISCOVER_QUERY = 'TEST_QUERY'; "
          "ADSERVERAUTOCHANNEL.CREATE_CHANNEL(:1, :2, :3, :4, :5, :6, :7, :8); "
        "END;");
      stmt->set_uint(1, 316);
      stmt->set_string(2, name);
      stmt->set_string(3, "TEST_QUERY");
      stmt->set_string(4, "TEST_ANNOTATION");
      stmt->set_string(5, "TEST_CONTENT");
      stmt->set_string(6, "");
      stmt->set_uint(7, 1);
      stmt->set_string(8, "TEST_ANNOTATION_STEM");
      stmt->execute();
      conn->commit();
      std::cout << FUN << ": channel inserted." << std::endl;
    }
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << FUN << ": caught unexpected eh::Exception: " << ex.what() << std::endl;
    return 1;
  }

  return 0;
}

int check_ORA_03106(
  const char* db,
  const char* user,
  const char* pwd,
  const char* /*name*/)
{
  static const char* FUN = "check_ORA_03106()";

  try
  {
    {
      Environment_var env = Environment::create_environment(
        (Environment::EnvironmentMode)(
          Environment::EM_THREADED_MUTEXED |
          Environment::EM_OBJECT /*|
          Environment::EM_EVENTS*/));

      for(int i = 0; i < 2; ++i)
      {
        Connection_var conn = env->create_connection(
          ConnectionDescription(user, pwd, db));

        Statement_var stmt = conn->create_statement(
          "SELECT "
            "wdt.wdtag_id, "
            "wdt.opted_in_content, "
            "wdt.opted_out_content, "
            "wdt.passback, "
            "wdt.look_n_feel_xml, "
            "wdt.logo_file, "
            "ADSERVERUTIL.cross_status("
              "ADSERVERUTIL.cross_status(s.adserver_status, a.adserver_status), "
              "wdt.status), "
            "greatest(s.version, a.version, wdt.version) "
          "FROM WDTag wdt "
            "LEFT JOIN v_Site s ON s.site_id = wdt.site_id "
            "LEFT JOIN v_Account a ON a.account_id = s.account_id");
        
        ResultSet_var rs = stmt->execute_query();

        while (rs->next())
        {
          /*
          unsigned long wdtag = rs->get_uint(1);
          char oi = rs->get_char(2);
          char oo = rs->get_char(3);
          std::string passback = rs->get_string(4);
          Lob ln = rs->get_blob(5);
          std::string logo_file = rs->get_string(6);
          char status = rs->get_char(7);
          Generics::Time tm = rs->get_timestamp(8);
          */
          /*
          {
            Statement_var sstmt = conn->create_statement(
              "SELECT "
              "wdt.wdtag_id, "
              "wdt.opted_in_content, "
              "wdt.opted_out_content, "
              "wdt.passback, "
              "wdt.look_n_feel_xml, "
              "wdt.logo_file, "
              "ADSERVERUTIL.cross_status("
                "ADSERVERUTIL.cross_status(s.adserver_status, a.adserver_status), "
                "wdt.status), "
              "greatest(s.version, a.version, wdt.version) "
            "FROM WDTag wdt "
              "LEFT JOIN v_Site s ON s.site_id = wdt.site_id "
              "LEFT JOIN v_Account a ON a.account_id = s.account_id");
        
            ResultSet_var srs = sstmt->execute_query();

            while (srs->next())
            {
              unsigned long swdtag = srs->get_uint(1);
              char soi = srs->get_char(2);
              char soo = srs->get_char(3);
              std::string spassback = srs->get_string(4);
              Lob sln = srs->get_blob(5);
              std::string slogo_file = srs->get_string(6);
              char sstatus = srs->get_char(7);
              Generics::Time stm = srs->get_timestamp(8);
              break;
            }
          }
          */
        }
      }
    }
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << FUN << ": caught unexpected eh::Exception: " << ex.what() << std::endl;
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
  Generics::AppUtils::StringOption opt_name;
  Generics::AppUtils::CheckOption opt_disable_check_memory_holding;
  Generics::AppUtils::CheckOption opt_perf_mode;

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
    Generics::AppUtils::equal_name("name"),
    opt_name);

  args.add(
    Generics::AppUtils::equal_name("no-mem-hold-check") ||
    Generics::AppUtils::short_name("nh"),
    opt_disable_check_memory_holding);

  args.add(
    Generics::AppUtils::equal_name("perf-mode"),
    opt_perf_mode);

  args.parse(argc - 1, argv + 1);

  Environment_var env = Environment::create_environment(
    (Environment::EnvironmentMode)(
      Environment::EM_THREADED_MUTEXED |
      Environment::EM_OBJECT /*|
      Environment::EM_EVENTS*/));

  if(!opt_perf_mode.enabled())
  {
    std::cout << "==== to check result set ops ====" << std::endl;
    res += check_result_set(
      opt_ora_server->c_str(),
      opt_ora_user->c_str(),
      opt_ora_pwd->c_str());

    std::cout << "==== to check date/time ops ====" << std::endl;
    res += check_time_ops(
      opt_ora_server->c_str(),
      opt_ora_user->c_str(),
      opt_ora_pwd->c_str());

    std::cout << "==== to check Object ops ====" << std::endl;
    res += check_object_ops(
      false,
      opt_ora_server->c_str(),
      opt_ora_user->c_str(),
      opt_ora_pwd->c_str());

    if(!opt_disable_check_memory_holding.enabled())
    {
      std::cout << "==== to check memory holding in result set ops ====" << std::endl;
      res += check_result_set_memory_holding(
        opt_ora_server->c_str(),
        opt_ora_user->c_str(),
        opt_ora_pwd->c_str());

      std::cout << "==== to check memory holding in Object ops ====" << std::endl;
      res += check_object_ops_memory_holding(
        opt_ora_server->c_str(),
        opt_ora_user->c_str(),
        opt_ora_pwd->c_str());
    }
  }
  else // perf mode
  {
    res += check_object_ops(
      true,
      opt_ora_server->c_str(),
      opt_ora_user->c_str(),
      opt_ora_pwd->c_str());
  }

  /*
  if(!opt_name->empty())
  {
    res += check_ADSC_2791(
      opt_ora_server->c_str(),
      opt_ora_user->c_str(),
      opt_ora_pwd->c_str(),
      opt_name->c_str());
  }
  
  res += check_ORA_03106(
    opt_ora_server->c_str(),
    opt_ora_user->c_str(),
    opt_ora_pwd->c_str(),
    opt_name->c_str());
  */
  
  return res;
}

