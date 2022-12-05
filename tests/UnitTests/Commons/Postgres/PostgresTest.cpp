#include"PostgresTest.hpp"
#include<iostream>
#include<fstream>
#include <array>
#include<Generics/Rand.hpp>
#include<Generics/Uuid.hpp>
#include<Generics/AppUtils.hpp>
#include<Commons/Postgres/Connection.hpp>
#include<Commons/Postgres/Environment.hpp>
#include<Commons/Postgres/ConnectionPool.hpp>
#include<Commons/Postgres/Lob.hpp>
#include<Commons/Postgres/SqlStream.hpp>
#include<Logger/StreamLogger.hpp>
#include<Generics/TaskRunner.hpp>

namespace
{
  const char* sql_1 = "CREATE TABLE "
    "pg_temp.PostgressTestTable (column_text text, column_integer integer, "
    "column_numeric numeric(8, 2), column_date date, column_time time, "
    "column_timestamp timestamp, column_char character, "
    "column_defnumeric numeric);";
  const char* sql_2 = "INSERT INTO pg_temp.PostgressTestTable values "
    "('string', 12, 23.45, '2013-06-03', '08:42:35.645183', "
    "'2011-12-06 18:04:18.044621', 'A', 0.01)";
  const char* sql_3 = "SELECT * from pg_temp.PostgressTestTable order by 1";
  const char* drop_table = "DROP TABLE ";
  const char* sql_5 = "INSERT INTO pg_temp.PostgressTestTable values "
    "($1, $2, $3, $4, $5, $6, $7, $8)";
  const char* sql_6 = "SELECT * from Channel order by 1";
  const char* sql_7 = "CREATE TABLE "
    "pg_temp.PostgressTestTable (column1 numeric, column2 numeric, "
    "column3 numeric, column4 numeric, column5 numeric, "
    "column6 numeric, column7 numeric, column8 numeric, column9 numeric);";
  const char* sql_8 = "INSERT INTO pg_temp.PostgressTestTable values "
    "($1, $2, $3, $4, $5, $6, $7, $8, $9)";
  const char* sql_create_lob_table = "CREATE TABLE "
    "PostgresTestLobTable (id integer, link oid)";
  const char* sql_insert_into_lob_table = "INSERT INTO "
    "PostgresTestLobTable values ($1, $2)";
  const char* sql_select_from_lob_table = "SELECT * FROM PostgresTestLobTable";
  const char* sql_create_obj_table = "create table pg_temp.TestObjTable ("
    "action_date date, action_time time, action_time_stamp timestamp, "
    "ccg_id int, action_id bigint, action_revenue numeric, name text, "
    "optional text);";
  const char* sql_create_obj_func = "create type pg_temp.test_obj_type as ("
    "action_date date, action_time time, action_time_stamp timestamp, "
    "ccg_id int, action_id bigint, action_revenue numeric, name text, "
    "optional text);"
    "create or replace function pg_temp.test_obj_func(p_data "
    "pg_temp.test_obj_type[]) returns void as $$ insert into "
    "pg_temp.TestObjTable select * from unnest(p_data) $$ language sql;";
  const char* sql_exec_obj_func = "SELECT pg_temp.test_obj_func($1::pg_temp.test_obj_type[])";

  const char* sql_query_performance = "SELECT * FROM generate_series(1, 10000000);";

  const char* sql_check_active_query = "SELECT state from pg_stat_activity where query = $1";

  const Generics::Time POSTGRES_EPOCH_DATE(
    String::SubString("2000-01-01"), "%Y-%m-%d");
}

namespace AdServer
{
  namespace UnitTests
  {

    class ConnectionActionTask: public Generics::TaskGoal 
    {
    public:
      ConnectionActionTask(
        PostgresTest* test,
        Commons::Postgres::Environment* env,
        Generics::TaskRunner* task_runner) noexcept
        : TaskGoal(task_runner), test_(test), env_(env)
      {
      }

      virtual void execute() noexcept;

    private:
      virtual ~ConnectionActionTask() noexcept {};
      PostgresTest *test_; 
      Commons::Postgres::Environment* env_;
    };

    PostgresTest::PostgresTest(int argc, char* argv[]) /*throw(BadParams)*/
      : file_logger_()
    {
      Generics::AppUtils::Args args;
      Generics::AppUtils::Option<unsigned long> verbose(0);
      Generics::AppUtils::StringOption test_case;
      Generics::AppUtils::StringOption db;
      try
      {
        args.add(
          Generics::AppUtils::equal_name("verbose") ||
          Generics::AppUtils::short_name("v"),
          verbose);
        args.add(
          Generics::AppUtils::equal_name("test") ||
          Generics::AppUtils::short_name("t"),
          test_case);
        args.add(
          Generics::AppUtils::equal_name("database") ||
          Generics::AppUtils::short_name("d"),
          db);
          args.parse(argc - 1, argv + 1);
      }
      catch(const eh::Exception& e)
      {
        throw BadParams(e.what());
      }
      verbose_ = *verbose;
      if (db.installed())
      {
        database_ = *db;
      }
      else
      {
        database_ = "host=stat-dev1.ocslab.com port=5432 dbname=ads_dev user=test_ads password=adserver";
      }
      if (test_case.installed())
      {
        if(*test_case == "con" || *test_case == "connection")
        {
          test_case_ = TC_CON;
        }
        else if(*test_case == "env" || *test_case == "environment")
        {
          test_case_ = TC_ENV;
        }
        else if(*test_case == "res" || *test_case == "resultset")
        {
          test_case_ = TC_RES;
        }
        else if(*test_case == "pool")
        {
          test_case_ = TC_POOL;
        }
        else if(*test_case == "async")
        {
          test_case_ = TC_ASYNC;
        }
        else if(*test_case == "numeric")
        {
          test_case_ = TC_NUMERIC;
        }
        else if(*test_case == "obj")
        {
          test_case_ = TC_OBJ;
        }
        else if(*test_case == "lob")
        {
          test_case_ = TC_LOB;
        }
        else if(*test_case == "query-perf")
        {
          test_case_ = TC_QUERY_PERFORMANCE;
        }
        else if(*test_case == "active-query")
        {
          test_case_ = TC_ACTIVE_QUERY;
        }
        else
        {
          Stream::Error err;
          err << "Bad test case '" << *test_case << "'" << std::endl;
          throw BadParams(err);
        }
      }
      else
      {
        test_case_ = TC_ALL;
      }
      //create logger
      Logging::OStream::Config error_config(std::cerr, ::Logging::Logger::NOTICE);
      Logging::OStream::Config out_config(std::cout, ::Logging::Logger::TRACE);
      Logger_var err_logger = new Logging::OStream::Logger(std::move(error_config));
      Logger_var out_logger = new Logging::OStream::Logger(std::move(out_config));
      std::array<Logging::Logger_var, 2> loggers{{
        Logging::Logger_var(new Logging::SeveritySelectorLogger(
          ::Logging::Logger::ERROR, err_logger)),
        Logging::Logger_var(new Logging::SeveritySelectorLogger(
          out_logger, LEVEL_LOW, LEVEL_LOW + verbose_))}};
      file_logger_ =
        new Logging::DistributorLogger(loggers.begin(), loggers.end());
    }

    int PostgresTest::run() noexcept
    {
      log_("Start test");
      int ret_value = 0;
      if(test_case_ == TC_CON || test_case_ == TC_ALL)
      {
        log_("Connection test case");
        ret_value += connection_test();
      }
      if(test_case_ == TC_RES || test_case_ == TC_ALL)
      {
        log_("ResultSet test case async in text format");
        ret_value += resultset_test(
          false,
          Commons::Postgres::TEXT_FORMAT);
        log_("ResultSet test case async in binary format");
        ret_value += resultset_test(
          false,
          Commons::Postgres::BINARY_FORMAT);
        log_("ResultSet test case sync in text format");
        ret_value += resultset_test(
          true,
          Commons::Postgres::TEXT_FORMAT);
        log_("ResultSet test case sync in binary format");
        ret_value += resultset_test(
          true,
          Commons::Postgres::BINARY_FORMAT);
      }
      if(test_case_ == TC_ENV || test_case_ == TC_ALL)
      {
        log_("Environment test case");
        ret_value += environment_test();
      }
      if(test_case_ == TC_POOL || test_case_ == TC_ALL)
      {
        log_("Pool test case");
        ret_value += pool_test();
      }
      if(test_case_ == TC_ASYNC || test_case_ == TC_ALL)
      {
        log_("Pool test case");
        ret_value += no_db_test();
      }
      if(test_case_ == TC_NUMERIC || test_case_ == TC_ALL)
      {
        log_("Pool test case");
        ret_value += numeric_test();
      }
      if(test_case_ == TC_OBJ || test_case_ == TC_ALL)
      {
        log_("Object test case");
        ret_value += object_test();
      }
      if(test_case_ == TC_LOB || test_case_ == TC_ALL)
      {
        log_("Pool test case");
        ret_value += lob_test();
      }
      if(test_case_ == TC_QUERY_PERFORMANCE || test_case_ == TC_ALL)
      {
        log_("Query performance test case");
        ret_value += query_perf_test();
      }
      if(test_case_ == TC_ACTIVE_QUERY || test_case_ == TC_ALL)
      {
        log_("Active query test case");
        ret_value += active_query_test();
      }
      log_("Stop test");
      return ret_value;
    }

    void PostgresTest::log_(const char* message, unsigned int level) noexcept
    {
      file_logger_->log(String::SubString(message), level); 
    }

    void PostgresTest::generate_record_(Record& rec) noexcept
    {
      std::ostringstream ostr;
      ostr << "string_" << Generics::safe_rand();
      rec.string_value = ostr.str();
      rec.int_value = Generics::safe_rand();
      rec.num_value = TestDecimal(
        Generics::safe_rand() % 2,
        Generics::safe_rand() % 100000,
        Generics::safe_rand() % 100);
      rec.date_value = Generics::Time(Generics::safe_rand());
      rec.date_value -= rec.date_value.tv_sec % 86400;
      rec.time_value = Generics::Time(Generics::safe_rand() % 86400);
      rec.timestamp_value = Generics::Time(
        Generics::safe_rand(), Generics::safe_rand() % 1000000);
        //644527776, 700300);
        //291538254, 626021);
      rec.char_value = 'A' + Generics::safe_rand() % ('Z'-'A');
      rec.defnum_value = TestDecimal2(
        Generics::safe_rand() % 2,
        Generics::safe_rand() % 100,
        Generics::safe_rand() % 100000000);
    }

    PostgresTest::CharArray PostgresTest::generate_uid_data_(
      size_t count, unsigned& size)
      noexcept
    {
      CharArray array;
      size = 0;
      unsigned offset = 0;
      for(size_t i = 0; i < count; i++)
      {
        Generics::Uuid uid = Generics::Uuid::create_random_based();
        std::string str_uid= uid.to_string();
        if (i == 0)
        {
          size = (str_uid.size() + 1) * count;
          array.reset(size);
        }
        if (offset + str_uid.size() < size)
        {
          memcpy(array.get() + offset, str_uid.data(), str_uid.size());
          offset += str_uid.size();
          array[offset++] = '\n';
        }
        else
        {
          size = offset;
          break;
        }
      }
      return array;
    }

    std::string PostgresTest::generate_file_name_() noexcept
    {
      Generics::Uuid rand_uid = Generics::Uuid::create_random_based();
      std::string file_name = "/tmp/" + rand_uid.to_string();
      return file_name;
    }

    std::string PostgresTest::write_uid_file_(char* buf, unsigned size) /*throw(Exception)*/
    {
      std::string file_name = generate_file_name_();
      std::ofstream my_file(
        file_name.c_str(),
        std::ios::out | std::ios::binary | std::ios::trunc);
      my_file.write(buf, size);
      if (my_file.bad())
      {
        my_file.close();
        Stream::Error err;
        err << "can't write file '" << file_name << "', error = " << errno;
        throw Exception(err);
      }
      my_file.close();
      return file_name;
    }

    PostgresTest::CharArray PostgresTest::read_file_(
      const std::string& file_name,
      unsigned size)
      /*throw(Exception)*/
    {
      CharArray array(size);
      std::ifstream my_file(
        file_name.c_str(),
        std::ios::in | std::ios::binary);
      my_file.read(array.get(), size);
      if (my_file.bad())
      {
        my_file.close();
        Stream::Error err;
        err << "can't read file '" << file_name << "', error = " << errno;
        throw Exception(err);
      }
      char symb;
      my_file.read(&symb, 1);
      if (my_file.bad() || !my_file.eof())
      {
        my_file.close();
        Stream::Error err;
        err << "It isn't end of file '" << file_name << "', error = " << errno;
        throw Exception(err);
      }
      my_file.close();
      return array;
    }

    void PostgresTest::compary_content_(
      const char* array1, const char* array2, unsigned array_size)
      /*throw(Exception)*/
    {
      if (memcmp(array1, array2, array_size) != 0)
      {
        throw Exception("arrays are different");
      }
    }

    int PostgresTest::lob_test() noexcept
    {
      int ret_value = 0;
      Commons::Postgres::Environment_var env;
      Commons::Postgres::Connection_var con;
      unsigned array_size = 0, buf_size;
      std::string in_name, out_name;
      try
      {
        CharArray array = generate_uid_data_(110, array_size);
        buf_size = array_size;
        CharArray array2;
        in_name = write_uid_file_(array.get(), array_size);
        log_("Creating environment");
        env = new Commons::Postgres::Environment(database_.c_str());
        log_("Creating connection");
        con = env->create_connection();
        log_("Create test table PostgresTestLobTable");
        AdServer::Commons::Postgres::ResultSet_var res =
          con->execute_query(sql_create_lob_table);

        Oid oid = InvalidOid;
        Commons::Postgres::Lob lob(con, oid);
        try
        {
          log_("Importing file to lob", LEVEL_MIDDLE);
          lob.import_file(in_name.c_str());
          if (lob.oid() == oid)
          {
            throw Exception("Invalid oid");
          }
          else
          {
            Stream::Error msg;
            oid = lob.oid();
            msg << "oid = " << oid;
            log_(msg.str().str().c_str(), LEVEL_MIDDLE);
          }
          log_("Create insert statement", LEVEL_HIGH);
          AdServer::Commons::Postgres::Statement_var stm =
            new AdServer::Commons::Postgres::Statement(sql_insert_into_lob_table);
          stm->set_value(1, 1);
          stm->set_value(2, oid);
          res = con->execute_statement(stm);
          log_("Execute statement", LEVEL_HIGH);
          res = con->execute_query(sql_select_from_lob_table);
          log_("Execute select query", LEVEL_HIGH);
          while(res->next())
          {
            unsigned int id = res->get_number<unsigned int>(1);
            if (id != 1)
            {
              continue;
            }
            oid = res->get_number<Oid>(2);
            if(oid != lob.oid())
            {
              Stream::Error err;
              err << "wrong oid, expect " << lob.oid() << " but got " <<  oid;
              throw Exception(err);
            }
            {
              log_("Getting whole lob", LEVEL_MIDDLE);
              Commons::Postgres::LobHolder holder = res->get_blob(2);
              if (holder.length != array_size)
              {
                Stream::Error err;
                err << "length isn't equeal, expected " << array_size
                  << " but got " <<  holder.length;
                throw Exception(err);
              }
              compary_content_(array.get(), holder.buffer, holder.length);
            }
            out_name = generate_file_name_();
            log_("Exporting log to file", LEVEL_MIDDLE);
            lob.export_file(out_name.c_str());
            array2 = read_file_(out_name, array_size);
            compary_content_(array.get(), array2.get(), array_size);
            log_("Reading lob file", LEVEL_MIDDLE);
            lob.read(array2.get(), array_size);
            compary_content_(array.get(), array2.get(), array_size);
            log_("Get possion1 in lob file", LEVEL_MIDDLE);
            pg_int64 pos = lob.tell();
            if (static_cast<unsigned>(pos) != array_size)
            {
              Stream::Error err;
              err << "tell gave size = " << pos << ", expected = " << array_size;
              throw Exception(err);
            }
            array_size /= 2;
            log_("Truncating lob file", LEVEL_MIDDLE);
            lob.truncate(array_size);
            log_("Get lob length", LEVEL_MIDDLE);
            pos = lob.length();
            if (pos != array_size)
            {
              Stream::Error err;
              err << "length = " << pos << ", expected = " << array_size;
              throw Exception(err);
            }
            log_("Seeking in lob file", LEVEL_MIDDLE);
            pos = lob.seek(0, SEEK_END);
            if (pos != array_size)
            {
              Stream::Error err;
              err << "seek gave size = " << pos << ", expected = " << array_size;
              throw Exception(err);
            }
            pos = lob.tell();
            if (static_cast<unsigned>(pos) != array_size)
            {
              Stream::Error err;
              err << "tell gave size = " << pos << ", expected = " << array_size;
              throw Exception(err);
            }
            log_("Writing to lob file", LEVEL_MIDDLE);
            lob.write(array.get() + array_size, buf_size - array_size);
            log_("Seeking in lob file", LEVEL_MIDDLE);
            pos = lob.seek(0, SEEK_SET);
            if (pos != 0)
            {
              Stream::Error err;
              err << "second seek gave size = " << pos << ", expected = 0";
              throw Exception(err);
            }
            log_("Reading from lob file", LEVEL_MIDDLE);
            lob.read(array2.get(), buf_size);
            compary_content_(array.get(), array2.get(), buf_size);
          }
          log_("Erase lob", LEVEL_MIDDLE);
          lob.unlink();
          log_("Crate lob", LEVEL_MIDDLE);
          lob.create();
            //std::cout << "id = " << id << " oid = " << oid << std::endl;
        }
        catch(const eh::Exception& e)
        {
          file_logger_->sstream(LEVEL_ERROR) << e.what() << std::endl;
          ret_value = 1;
        }
        log_("Erase lob", LEVEL_MIDDLE);
        lob.unlink();
        log_("Drop table", LEVEL_MIDDLE);
        drop_table_(con, 1, String::SubString("PostgresTestLobTable"));
        if (!in_name.empty())
        {
          file_logger_->sstream(LEVEL_MIDDLE) << "File1: " << in_name;
          if(unlink(in_name.c_str()) != 0)
          {
            file_logger_->sstream(LEVEL_ERROR)
              << "can't unlink " << in_name << " error code = " << errno;
          }
        }
        if (!out_name.empty())
        {
          file_logger_->sstream(LEVEL_MIDDLE) << "File2: " << out_name;
          if(unlink(out_name.c_str()) != 0)
          {
            file_logger_->sstream(LEVEL_ERROR)
              << "can't unlink " << out_name << " error code = " << errno;
          }
        }
      }
      catch(AdServer::Commons::Postgres::Exception& e)
      {
        file_logger_->sstream(LEVEL_ERROR) << e.what() << std::endl;
        drop_table_(con, 1, String::SubString("PostgresTestLobTable"));
        return 1;
      }
      catch(const Exception& e)
      {
        file_logger_->sstream(LEVEL_ERROR) << e.what() << std::endl;
        drop_table_(con, 1, String::SubString("PostgresTestLobTable"));
        return 1;
      }
      return ret_value;
    }

    int PostgresTest::object_test() noexcept
    {
      int ret_value = 0;
      Commons::Postgres::Environment_var env;
      Commons::Postgres::Connection_var con;
      try
      {
        log_("Creating environment");
        env = new Commons::Postgres::Environment(database_.c_str());
        log_("Creating connection");
        con = env->create_connection();
        log_("Create test table TestObjTable");
        AdServer::Commons::Postgres::ResultSet_var res =
          con->execute_query(sql_create_obj_table);
        log_("Create function test_obj_func");
        res = con->execute_query(sql_create_obj_func);
        log_("Create statement for test_obj_func");
        AdServer::Commons::Postgres::Statement_var stm = 
          new AdServer::Commons::Postgres::Statement(sql_exec_obj_func);
        Commons::Postgres::ObjectVector objs;
        init_objs_(objs);
        stm->set_array(1, objs);
        log_("Execute statement for test_obj_func");
        res = con->execute_statement(stm, true);
        log_("Drop table", LEVEL_MIDDLE);
        drop_table_(con, 1, String::SubString("pg_temp.TestObjTable"));
      }
      catch(AdServer::Commons::Postgres::Exception& e)
      {
        file_logger_->sstream(LEVEL_ERROR) << e.what() << std::endl;
        drop_table_(con, 1, String::SubString("TestObjTable"));
        return 1;
      }
      catch(const Exception& e)
      {
        file_logger_->sstream(LEVEL_ERROR) << e.what() << std::endl;
        drop_table_(con, 1, String::SubString("TestObjTable"));
        return 1;
      }
      return ret_value;
    }

    int PostgresTest::numeric_test() noexcept
    {
      int ret_value = 0;
      AdServer::Commons::Postgres::Connection_var con;
      try
      {
        con = new AdServer::Commons::Postgres::Connection(database_.c_str());
        {
          std::ostringstream mes;
          mes << "Created connection to '" << database_ << '\'';
          log_(mes.str().c_str());
        }

        log_("Create test table PostgressTestTable");
        AdServer::Commons::Postgres::ResultSet_var res =
          con->execute_query(sql_7);
        unsigned long mult = Generics::safe_rand() % 9 + 2;
        unsigned long whole = 0, fractional;
        log_("Try to insert values into table PostgressTestTable");
        do
        {
          log_("Create statement", LEVEL_HIGH);
          AdServer::Commons::Postgres::Statement_var stm = 
            new AdServer::Commons::Postgres::Statement(sql_8);
          fractional = 0;
          stm->set_value(1, TestDecimal2(false, whole, fractional));
          fractional = 1;
          for(size_t index = 9; index > 1; index--, fractional *= 10)
          {
            stm->set_value(index, TestDecimal2(false, whole, fractional));
          }
          log_("Execute statement", LEVEL_HIGH);
          res = con->execute_statement(stm);
          if(whole)
          {
            whole *= mult;
          }
          else
          {
            whole += mult;
          }
        } while(whole < 100000000);
        log_("Try to select values from table PostgressTestTable");
        AdServer::Commons::Postgres::Statement_var stm = 
          new AdServer::Commons::Postgres::Statement(sql_3);
        res = con->execute_statement(stm, false, Commons::Postgres::BINARY_FORMAT);
        log_("The values selected", LEVEL_HIGH);
        unsigned long line  = 0;
        whole = 0;
        while(res->next())
        {
          TestDecimal2 value;
          ++line;
          value = res->get_decimal<TestDecimal2>(1);
          fractional = 0;
          ret_value |= check_value_(
            __func__,
            value,
            TestDecimal2(false, whole, fractional));
          fractional = 1;
          for(size_t index = 9; index > 1; index--, fractional *= 10)
          {
            value = res->get_decimal<TestDecimal2>(index);
            ret_value |= check_value_(
              __func__,
              value,
              TestDecimal2(false, whole, fractional));
          }
          if(whole)
          {
            whole *= mult;
          }
          else
          {
            whole += mult;
          }
        }
      }
      catch(AdServer::Commons::Postgres::SqlException& e)
      {
        file_logger_->sstream(LEVEL_ERROR) << e.what() << std::endl;
        ret_value = 1;
      }
      catch(eh::Exception& e)
      {
        file_logger_->sstream(LEVEL_ERROR) << e.what() << std::endl;
        ret_value = 1;
      }
      drop_table_(con);
      return ret_value;
    }

    void PostgresTest::query_perf_one_row(
      AdServer::Commons::Postgres::Connection* con) noexcept
    {
      log_("Execute performance query in 'row-by-row' mode");
      AdServer::Commons::Postgres::Statement_var stm = 
        new AdServer::Commons::Postgres::Statement(sql_query_performance);
      Generics::Timer timer;
      timer.start();
      AdServer::Commons::Postgres::ResultSet_var res =
        con->execute_statement(stm, false, Commons::Postgres::BINARY_FORMAT);
      while(res->next())
      {
        //res->get_number<int>(1);
      }
      timer.stop();
      file_logger_->sstream(LEVEL_LOW)
        << "select query performance in mode 'row-by-row': "
        << timer.elapsed_time();
    }

    void PostgresTest::query_perf_all_row(
      AdServer::Commons::Postgres::Connection* con) noexcept
    {
      log_("Execute performance query in 'full fetch' mode");
      AdServer::Commons::Postgres::Statement_var stm = 
        new AdServer::Commons::Postgres::Statement(sql_query_performance);
      Generics::Timer timer;
      timer.start();
      AdServer::Commons::Postgres::ResultSet_var res =
        con->execute_statement(stm, true, Commons::Postgres::BINARY_FORMAT);
      while(res->next())
      {
        //res->get_number<int>(1);
      }
      timer.stop();
      file_logger_->sstream(LEVEL_LOW)
        << "select query performance in mode 'full fetch': "
        << timer.elapsed_time();
    }

    void PostgresTest::query_perf_cursor(
      AdServer::Commons::Postgres::Connection* con) noexcept
    {
      for(auto i = 1000; i < 10000001; i *= 4)
      {
        file_logger_->sstream(LEVEL_MIDDLE)
          << "Execute performance query in 'cursor(" << i << ")' mode";
        AdServer::Commons::Postgres::Statement_var stm = 
          new AdServer::Commons::Postgres::Statement(sql_query_performance);
        Generics::Timer timer;
        timer.start();
        AdServer::Commons::Postgres::ResultSet_var res =
          con->execute_statement_with_cursor(stm, i, Commons::Postgres::BINARY_FORMAT);
        while(res->next())
        {
          //res->get_number<int>(1);
        }
        timer.stop();
        file_logger_->sstream(LEVEL_LOW)
          << "Execute performance query in 'cursor(" << i << ")' mode: "
          << timer.elapsed_time();
      }
    }

    int PostgresTest::query_perf_test() noexcept
    {
      int ret_value = 0;
      AdServer::Commons::Postgres::Connection_var con;
      try
      {
        con = new AdServer::Commons::Postgres::Connection(database_.c_str());
        {
          std::ostringstream mes;
          mes << "Created connection to '" << database_ << '\'';
          log_(mes.str().c_str());
        }
        
        query_perf_one_row(con);
        query_perf_all_row(con);
        query_perf_cursor(con);

      }
      catch(AdServer::Commons::Postgres::SqlException& e)
      {
        file_logger_->sstream(LEVEL_ERROR) << e.what() << std::endl;
        ret_value = 1;
      }
      catch(eh::Exception& e)
      {
        file_logger_->sstream(LEVEL_ERROR) << e.what() << std::endl;
        ret_value = 1;
      }
      return ret_value;
    }

    int PostgresTest::active_query_test() noexcept
    {
      int ret_value = 0;
      AdServer::Commons::Postgres::Connection_var con1;
      AdServer::Commons::Postgres::Connection_var con2;
      try
      {
        con1 = new AdServer::Commons::Postgres::Connection(database_.c_str());
        con2 = new AdServer::Commons::Postgres::Connection(database_.c_str());
        file_logger_->sstream(LEVEL_LOW)
          << "Created connections to '" << database_ << '\'';
        AdServer::Commons::Postgres::Statement_var stm =
          new AdServer::Commons::Postgres::Statement(sql_6);
        log_("Prepared statement");
        AdServer::Commons::Postgres::ResultSet_var res =
          con1->execute_statement(stm, false);
        bool next_rec;
        for(auto i = 0; i < 5 && (next_rec = res->next()); ++i);
        if (next_rec)
        {
          res = nullptr;
          //check state of query in database
          AdServer::Commons::Postgres::Statement_var stm_check =
            new AdServer::Commons::Postgres::Statement(sql_check_active_query);
          stm_check->set_value(1, sql_6);
          res = con2->execute_statement(stm_check);
          if (res->next())
          {
            std::string state = res->get_string(1);
            file_logger_->sstream(LEVEL_MIDDLE)
              << "The state of query is '" << state << "'";
            if (state == "active")
            {
              ret_value = 1;
              file_logger_->sstream(LEVEL_ERROR) << "failed: query is active";
            }
          }
          else
          {
            file_logger_->sstream(LEVEL_HIGH)
              << "query '" << sql_6 << "' isn't found";
          }
        }
        else
        {
          file_logger_->sstream(LEVEL_ERROR) << "coudln't read 5 records";
          ret_value = 1;
        }
      }
      catch(AdServer::Commons::Postgres::SqlException& e)
      {
        file_logger_->sstream(LEVEL_ERROR) << e.what() << std::endl;
        ret_value = 1;
      }
      catch(eh::Exception& e)
      {
        file_logger_->sstream(LEVEL_ERROR) << e.what() << std::endl;
        ret_value = 1;
      }
      return ret_value;
    }

    int PostgresTest::connection_test() noexcept
    {
      try
      {
        AdServer::Commons::Postgres::Connection_var con =
          new AdServer::Commons::Postgres::Connection(database_.c_str());
        {
          std::ostringstream mes;
          mes << "Created connection to '" << database_ << '\'';
          log_(mes.str().c_str());
        }
        if(!con->is_free())
        {
          log_("Connection it isn't personal", 0);
          return 1;
        }
        return connection_actions_(con);
      }
      catch(const eh::Exception& e)
      {
        file_logger_->sstream(LEVEL_ERROR) << "Error on testing connection '"
          << database_ << "' :" << e.what();
      }
      return 1;
    }

    int PostgresTest::connection_actions_(
      AdServer::Commons::Postgres::Connection* con,
      int error)
      noexcept
    {
      int ret_value = 0;
      try
      {
        log_("Try to create test table PostgressTestTable");
        AdServer::Commons::Postgres::ResultSet_var res =
          con->execute_query(sql_1);
        log_("The table created");
        log_("Try to insert values into table PostgressTestTable");
        res = con->execute_query(sql_2);
        log_("The values inserted");
        log_("Create statement", LEVEL_HIGH);
        AdServer::Commons::Postgres::Statement_var stm = 
          new AdServer::Commons::Postgres::Statement(sql_3);
        res = con->execute_statement(stm);
        res.reset();//destroy res, only after this possible to make a new query in async mode
        log_("Try to select values from table PostgressTestTable");
        res = con->execute_statement(stm);
        res.reset();//destroy res, only after this possible to make a new query in async mode
        log_("The values selected", LEVEL_HIGH);
        res = con->execute_statement(stm);
        log_("The values selected", LEVEL_HIGH);
      }
      catch(AdServer::Commons::Postgres::SqlException& e)
      {
        file_logger_->sstream(error ? LEVEL_ERROR : LEVEL_MIDDLE) 
          << e.what() << std::endl;
        ret_value = error;
      }
      catch(const eh::Exception& e)
      {
        file_logger_->sstream(error ? LEVEL_ERROR : LEVEL_MIDDLE) 
          << "Error on connection actions '"
          << database_ << "' :" << e.what() << std::endl;
        ret_value = error;
      }
      ret_value |= drop_table_(con);
      return ret_value;
    }

    int PostgresTest::drop_table_(
      AdServer::Commons::Postgres::Connection* con,
      int error,
      const String::SubString& table)
      noexcept
    {
      if(!con)
      {
        return 0;
      }
      int ret_value = 0;
      try
      {//try to drop table
        {
          Stream::Error msg;
          msg << "Try to drop the table " << table;
          log_(msg.str().str().c_str());
        }
        {
          Stream::Error query;
          query << drop_table << table;
          AdServer::Commons::Postgres::ResultSet_var res =
            con->execute_query(query.str().str().c_str());
        }
        log_("The table dropped");
      }
      catch(AdServer::Commons::Postgres::SqlException& e)
      {
        file_logger_->sstream(error ? LEVEL_ERROR : LEVEL_MIDDLE) 
          << e.what() << std::endl;
        ret_value = error;
      }
      catch(const eh::Exception& e)
      {
        file_logger_->sstream(error ? LEVEL_ERROR : LEVEL_MIDDLE) 
          << "Error on dropping " << table << " table: "
          << e.what() << std::endl;
        ret_value = error;
      }
      return ret_value;
    }

    int PostgresTest::environment_test() noexcept
    {
      try
      {
        log_("Creating environment");
        Commons::Postgres::Environment_var env = 
          new Commons::Postgres::Environment(database_.c_str());
        log_("Creating connection");
        Commons::Postgres::Connection_var con = env->create_connection();
        return connection_actions_(con);
      }
      catch(const eh::Exception& e)
      {
        file_logger_->sstream(LEVEL_ERROR) 
          << "Error on testing environment '"
          << database_ << "' :" << e.what();
      }
      return 1;
    }

    int PostgresTest::pool_test() noexcept
    {
      int res = 0;
      try
      {
        log_("Creating environment");
        Commons::Postgres::Environment_var env = 
          new Commons::Postgres::Environment(database_.c_str());
        log_("Creating connection pool");
        Commons::Postgres::ConnectionPool_var pool =
          env->create_connection_pool();
        log_("Activating pool");
        pool->activate_object();
        log_("Getting connection");
        Commons::Postgres::Connection_var con = pool->get_connection();
        res = connection_actions_(con);
        if(!res)
        {
          log_("Deactivating pool");
          pool->deactivate_object();
          con.reset();
          pool->wait_object();
          try
          {
            log_("Try to get a new connection on deactive pool");
            con = pool->get_connection();
            file_logger_->sstream(LEVEL_ERROR) 
              << "success getting connection  on deactive pool ";
            return 1;
          }
          catch(const Commons::Postgres::NotActive&)
          {//expected behavior
            log_("Didn't get the new connection");
          }
          log_("Activating pool");
          pool->activate_object();
          log_("Gettiing a new connection");
          con = pool->get_connection();
          log_("Connection actions on the new connection");
          res = connection_actions_(con);
          con.reset();
          log_("Deactivating pool");
          pool->deactivate_object();
          log_("Waiting pool");
          pool->wait_object();
        }
      }
      catch(const eh::Exception& e)
      {
        file_logger_->sstream(LEVEL_ERROR) 
          << "Error on testing pool '"
          << database_ << "' :" << e.what();
        res = 1;
      }
      return res;
    }

    void PostgresTest::report_error(
      Severity /*severity*/,
      const String::SubString& description,
      const char* /*error_code*/) noexcept
    {
      const std::string& str = description.str();
      log_(str.c_str(), 0); 
    }

    void ConnectionActionTask::execute() noexcept
    {
      test_->deactivate_env(env_);
    }

    void PostgresTest::deactivate_env(
      Commons::Postgres::Environment* env) noexcept
    {
      log_("Start task");
      log_("Deactivating environment");
      env->deactivate_object();
      log_("Waiting environment");
      env->wait_object();
      log_("End task");
    }

    int PostgresTest::resultset_test(
      bool sync,
      Commons::Postgres::DATA_FORMATS format)
      noexcept
    {
      int ret_value = 0;
      AdServer::Commons::Postgres::Connection_var con;
      try
      {
        con = new AdServer::Commons::Postgres::Connection(database_.c_str());
        {
          std::ostringstream mes;
          mes << "Created connection to '" << database_ << '\'';
          log_(mes.str().c_str());
        }

        log_("Create test table PostgressTestTable");
        AdServer::Commons::Postgres::ResultSet_var res =
          con->execute_query(sql_1);
        log_("Try to insert values into table PostgressTestTable");
        log_("Create statement", LEVEL_HIGH);
        AdServer::Commons::Postgres::Statement_var stm = 
          new AdServer::Commons::Postgres::Statement(sql_5);
        Record rec, rec_result;
        generate_record_(rec);
        if(verbose_ > 1)
        {
          Stream::Error err;
          err << "Generating values: '" << rec.string_value
            << "', " << rec.int_value << ", " << rec.num_value
            << ", " << rec.date_value << ", " << rec.time_value
            << ", " << rec.timestamp_value << ", " << rec.char_value 
            << ", " << rec.defnum_value << ".";
          log_(err.str().str().c_str(), LEVEL_HIGH);
        }
        stm->set_value(1, rec.string_value);
        stm->set_value(2, rec.int_value);
        stm->set_value(3, rec.num_value);
        stm->set_date(4, rec.date_value);
        stm->set_time(5, rec.time_value);
        stm->set_timestamp(6, rec.timestamp_value);
        stm->set_value(7, rec.char_value);
        stm->set_value(8, rec.defnum_value);
        res = con->execute_statement(stm, sync);
        log_("Try to select values from table PostgressTestTable");
        stm = new AdServer::Commons::Postgres::Statement(sql_3);
        res = con->execute_statement(stm, sync, format);
        log_("The values selected", LEVEL_HIGH);
        size_t count_records = 0;
        while(res->next())
        {
          Generics::Time date_from_timestamp;
          Generics::Time time_from_timestamp;
          Generics::Time calc_time;
          count_records++;
          rec_result.string_value = res->get_string(1);
          rec_result.int_value = res->get_number<unsigned int>(2);
          rec_result.num_value = res->get_decimal<TestDecimal>(3);
          rec_result.date_value = res->get_date(4);
          rec_result.time_value = res->get_time(5);
          rec_result.timestamp_value = res->get_timestamp(6);
          date_from_timestamp = res->get_date(6);
          time_from_timestamp = res->get_time(6);
          calc_time = rec_result.timestamp_value - Generics::Time(
            (rec_result.timestamp_value.tv_sec / 86400) * 86400); 
          rec_result.char_value = res->get_char(7);
          rec_result.defnum_value = res->get_decimal<TestDecimal2>(8);
          if(rec.string_value != rec_result.string_value)
          {
            Stream::Error err;
            err << "String values isn't equal: '" << rec.string_value 
              << "' != '" << rec_result.string_value << '\'';
            log_(err.str().str().c_str(), 0);
            ret_value |= 1;
          }
          if(rec.int_value != rec_result.int_value)
          {
            Stream::Error err;
            err << "Integer values isn't equal: '" << rec.int_value 
              << "' != '" << rec_result.int_value << '\'';
            log_(err.str().str().c_str(), 0);
            ret_value |= 1;
          }
          if(rec.num_value != rec_result.num_value)
          {
            Stream::Error err;
            err << "Decimal values isn't equal: '" << rec.num_value 
              << "' != '" << rec_result.num_value << '\'';
            log_(err.str().str().c_str(), 0);
            ret_value |= 1;
          }
          if(rec.date_value != rec_result.date_value)
          {
            Stream::Error err;
            err << "Date values isn't equal: '" << rec.date_value 
              << "' != '" << rec_result.date_value << '\'';
            log_(err.str().str().c_str(), 0);
            ret_value |= 1;
          }
          if(rec.time_value != rec_result.time_value)
          {
            Stream::Error err;
            err << "Time values isn't equal: '" << rec.time_value 
              << "' != '" << rec_result.time_value << '\'';
            log_(err.str().str().c_str(), 0);
            ret_value |= 1;
          }
          if(rec_result.timestamp_value.tv_sec / 86400 !=
             date_from_timestamp.tv_sec / 86400)
          {
            Stream::Error err;
            err << "Date in timestamp values isn't equal date: '"
              << rec_result.timestamp_value 
              << "' != '" << date_from_timestamp << '\'';
            log_(err.str().str().c_str(), 0);
            ret_value |= 1;
          }
          if(time_from_timestamp != calc_time)
          {
            Stream::Error err;
            err << "Time in timestamp values isn't equal time: '"
              << calc_time 
              << "' != '" << time_from_timestamp << '\'';
            log_(err.str().str().c_str(), 0);
            ret_value |= 1;
          }
          if(rec.timestamp_value != rec_result.timestamp_value)
          {
            Stream::Error err;
            err << "Timestamp values isn't equal: '" << rec.timestamp_value 
              << "' != '" << rec_result.timestamp_value << "', Postgres epoche is '"
              << POSTGRES_EPOCH_DATE << '\'';
            log_(err.str().str().c_str(), 0);
            ret_value |= 1;
          }
          if(rec.char_value != rec_result.char_value)
          {
            Stream::Error err;
            err << "Char values isn't equal: '" << rec.char_value 
              << "' != '" << rec_result.char_value << '\'';
            log_(err.str().str().c_str(), 0);
            ret_value |= 1;
          }
          if(rec.int_value != rec_result.int_value)
          {
            Stream::Error err;
            err << "String values isn't equal: '" << rec.int_value 
              << "' != '" << rec_result.int_value << '\'';
            log_(err.str().str().c_str(), 0);
            ret_value |= 1;
          }
          if(rec.defnum_value != rec_result.defnum_value)
          {
            Stream::Error err;
            err << "Default decimal values isn't equal: '"
              << rec.defnum_value << "' != '"
              << rec_result.defnum_value << '\'';
            log_(err.str().str().c_str(), 0);
            ret_value |= 1;
          }
        }
        if (count_records != 1)
        {
          file_logger_->sstream(LEVEL_ERROR) 
            << "Unexpected count of rows " << res->rows() << " in resultset";
        }
        log_("Try to insert second record into table PostgressTestTable");
        res = con->execute_query(sql_2);
        log_("The values inserted");
        stm = new AdServer::Commons::Postgres::Statement(sql_3);
        res = con->execute_statement(stm, sync);
        count_records = 0;
        while (res->next())
        {
          count_records++;
        }
        if (count_records != 2)
        {
          file_logger_->sstream(LEVEL_ERROR) 
            << "Unexpected count of rows " << res->rows() << " in resultset";
        }
        drop_table_(con);
      }
      catch(AdServer::Commons::Postgres::SqlException& e)
      {
        file_logger_->sstream(LEVEL_ERROR) << e.what();
        drop_table_(con);
        return 1;
      }
      catch(eh::Exception& e)
      {
        file_logger_->sstream(LEVEL_ERROR) << e.what();
        drop_table_(con);
        return 1;
      }
      return ret_value;
    }

    int PostgresTest::no_db_test() noexcept
    {
      unsigned long count_records = 0;
      try
      {
        Generics::TaskRunner_var task_runner(new Generics::TaskRunner(this, 1));
        task_runner->activate_object();

        log_("Creating environment");
        Commons::Postgres::Environment_var env = 
          new Commons::Postgres::Environment(database_.c_str());
        log_("Creating connection pool");
        Commons::Postgres::ConnectionPool_var pool =
          env->create_connection_pool();
        log_("Activating pool");
        pool->activate_object();
        log_("Getting connection");
        Commons::Postgres::Connection_var con = pool->get_connection();
        Generics::Task_var task(
          new ConnectionActionTask(this, env, task_runner));
        AdServer::Commons::Postgres::Statement_var stm =
          new AdServer::Commons::Postgres::Statement(sql_6);
        log_("Prepared statement");
        try
        {
          task_runner->enqueue_task(task);
          AdServer::Commons::Postgres::ResultSet_var res =
            con->execute_statement(stm, false);
          log_("Finish query");
          while(res->next())
          {
            count_records++;
            if(count_records % 1000 == 0)
            {
              file_logger_->sstream(LEVEL_HIGH)
                << "Fetched " << count_records << " records";
            }
          }
          log_("Fetched all records");
          con.reset();
        }
        catch(const eh::Exception& e)
        {
          Stream::Error err;
          err << "caught exception : " << e.what() << ", fetched "
            << count_records << " records";
          log_(err.str().str().c_str());
          return 1;
        }
        pool->deactivate_object();
        task_runner->deactivate_object();
        pool->wait_object();
        task_runner->wait_object();
        return 0;
      }
      catch(const eh::Exception& e)
      {
        Stream::Error err;
        err << __func__ << ": " << e.what();
        log_(err.str().str().c_str(), 0);
        return 1;
      }
    }
    
  }
}

int main(int argc, char* argv[])
{
  try
  {
    AdServer::UnitTests::PostgresTest test(argc, argv);
    return test.run();
  }
  catch(const AdServer::UnitTests::PostgresTest::BadParams& e)
  {
    std::cerr << e.what() << std::endl;
    return 1;
  }
}

