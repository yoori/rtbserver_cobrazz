
#ifndef __AUTOTESTS_COMMONS_PGCONNECTION_HPP
#define __AUTOTESTS_COMMONS_PGCONNECTION_HPP

#include <string>
#include <vector>
#include <sstream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <libpq-fe.h>

#include <Generics/Uncopyable.hpp>
#include <tests/AutoTests/Commons/Connection.hpp>

namespace AutoTest
{
  namespace PQ
  {
    typedef std::pair< PGconn*, std::string> QueryTuple;
    typedef std::pair< std::string, PGresult*> ResultTuple;

    /**
     * @class Conn
     * @brief encapsulate connection, implements reopen semantics
     * warning! not thread safe
     */
    class Conn:
      private Generics::Uncopyable,
      public AutoTest::DBC::IConn
    {
    public:

      /**
       * @brief Default constructor.
       */
      Conn();

      /**
       * @brief Constructor.
       *
       * @param user
       * @param password
       * @param host
       * @param database
       */
      Conn(
        const std::string& user,
        const std::string& pswd,
        const std::string& host,
        const std::string& db);

      /**
       * @brief Destructor.
       */
      virtual ~Conn();

      
      /**
       * @brief Open connection.
       *
       * @param user
       * @param password
       * @param host
       * @param database
       */
      void open(
        const std::string& user,
        const std::string& pswd,
        const std::string& host,
        const std::string& db);

      /**
       * @brief Open connection.
       */
      void open  ();

      /**
       * @brief Close connection.
       */
      void close ();

      /**
       * @brief Commit changes.
       */
      void commit ();

      /**
       * @brief Create query.
       *
       * @param query statement
       * @return query tuple
       */
      QueryTuple
      query(
        const std::string& statement);

      /**
       * @brief Create query.
       *
       * @param query statement
       * @return query interface
       */
      AutoTest::DBC::IQuery*
      get_query (
        const std::string& statement);

    protected:

       /**
       * @brief Open connection.
       *
       * @param user
       * @param password
       * @param host
       * @param database
       */
      void open_(
        const std::string& user,
        const std::string& pswd,
        const std::string& host,
        const std::string& db);

      /**
       * @brief Open connection.
       */
      void open_ ();
      
      std::string user_;
      std::string pswd_;
      std::string host_;
      std::string db_;
      PGconn* connection_;
    };

    /**
    * @class BasicQueryStream
    * @brief overloaded parameters setters
    */
    class BasicQueryStream:
      public virtual AutoTest::DBC::IBasicQueryStream
    {
    public:
      /**
       * @brief Default constructor.
       */
      BasicQueryStream();

      /**
       * @brief Set char parameter.
       *
       * @param parameter value
       */
      void
      set(
        char value);

      /**
       * @brief Set int parameter.
       *
       * @param parameter value
       */
      void
      set(
        int value);

      /**
       * @brief Set bool parameter.
       *
       * @param parameter value
       */
      void
      set(
        bool value);

      /**
       * @brief Set uint parameter.
       *
       * @param parameter value
       */
      void
      set(
        unsigned int value);

      /**
       * @brief Set float parameter.
       *
       * @param parameter value
       */
      void
      set(
        float value);

      /**
       * @brief Set double parameter.
       *
       * @param parameter value
       */
      void
      set(
        double value);

      /**
       * @brief Set string parameter.
       *
       * @param parameter value
       */
      void
      set(
        const std::string& value);

      /**
       * @brief Set date parameter.
       *
       * @param parameter value
       */
      void
      set(
        const Generics::ExtendedTime& value);

      /**
       * @brief Set timestamp parameter.
       *
       * @param parameter value
       */
      void
      set(
        const Generics::Time& value);

      /**
       * @brief Set long(text) parameter.
       *
       * @param parameter value
       */
      void
      set(
        const AutoTest::DBC::TextField& value);

      /**
       * @brief Set char parameter to null.
       */
      void
      null(
        char value);

      /**
       * @brief Set int parameter to null.
       */
      void
      null(
        int value);

      /**
       * @brief Set bool parameter to null.
       */
      void
      null(
        bool value);


      /**
       * @brief Set uint parameter to null.
       */
      void
      null(
        unsigned int value);

      /**
       * @brief Set float parameter to null.
       */
      void
      null(
        float value);

      /**
       * @brief Set double parameter to null.
       */
      void
      null(
        double value);

      /**
       * @brief Set string parameter to null.
       */
      void
      null(
        const std::string& value);

      /**
       * @brief Set date parameter to null.
       */
      void
      null(
        const Generics::ExtendedTime& value);

      /**
       * @brief Set timestamp parameter to null.
       */
      void
      null(
        const Generics::Time& value);

      /**
       * @brief Set long(text) parameter to null.
       */
      void
      null(
        const AutoTest::DBC::TextField& value);

      /**
       * @brief Process some actions after update.
       * Nothing to Postgres.
       */
      void
      flush();

      /**
       * @brief Get count of initialized parameters.
       *
       * @return count parameters.
       */
      unsigned int
      size() const;

    protected:
      typedef std::vector<Oid> OidSeq;
      typedef std::vector<char> Buffer;
      typedef std::vector<int> IntSeq;

    protected:
      /**
       * @brief Set postgres value.
       *
       * @param postgres oid
       * @param parameter size
       * @param value
       */
      template<typename ValueType>
      void
      set_as_string_(
        Oid oid,
        unsigned long size,
        const ValueType& val);

      /**
       * @brief Set postgres value as string.
       *
       * @param postgres oid
       * @param parameter size
       * @param string presentation of value
       */
      void
      set_string_(
        Oid oid,
        unsigned long size,
        const std::string& val);

      /**
       * @brief Set postgres null value.
       *
       * @param postgres oid
       */
      void set_null_(Oid oid);

    protected:
      OidSeq oids_;
      IntSeq lengths_;
      IntSeq values_;
      Buffer buffer_;
    };

    /**
    * @class Query
    * @brief postgres query implementation
    */
    class Query :
      private Generics::Uncopyable,
      public AutoTest::DBC::IQuery,
      public BasicQueryStream
    {
    public:
      /**
       * @brief Constructor.
       *
       * @param query tuple
       */
      explicit Query(const QueryTuple& tuple);

      /**
       * @brief Destructor.
       *
       * @param query tuple
       */
      virtual ~Query();

      /**
       * @brief Ask & get query result.
       *
       * @return result tuple
       */
      ResultTuple
      ask();

      /**
       * @brief Ask & get query result.
       *
       * @return result interface
       */
      AutoTest::DBC::IResult*
      ask_result ();

      /**
       * @brief Execute query.
       *
       * @return execution status
       */      
      int
      update ();
      
      /**
       * @brief throw exception on query.
       *
       * @param error message
       */      
      void
      throw_exception(
        const std::string& message);

      /**
       * @brief Get query stream.
       *
       * @return stream
       */      
      IBasicQueryStream* query_stream();

    protected:
      PGconn*     connection_;
      std::string statement_;
    };

    /**
    * @class Result
    * @brief this result wrap PGresult with overloaded results getters, and hold its pointer
    */
    class Result:
      private Generics::Uncopyable,
      public AutoTest::DBC::IResult
    {
    public:
      /**
       * @brief Constructor.
       *
       * @param result tuple
       */
      explicit Result(const ResultTuple& tuple);

      /**
       * @brief Destructor.
       *
       * @param query tuple
       */
      virtual ~Result ();

      /**
       * @brief Goto next row.
       *
       * @return true if success.
       */
      bool
      next();

      /**
       * @brief Check row is null.
       *
       * @return true if null.
       */
      bool
      is_null();

      /**
       * @brief Get int value.
       *
       * @param value.
       */
      void
      get(
        int& value);


      /**
       * @brief Get bool value.
       *
       * @param value.
       */
      void
      get(
        bool& value);

      /**
       * @brief Get uint value.
       *
       * @param value.
       */
      void
      get(
        unsigned int& value);

      /**
       * @brief Get float value.
       *
       * @param value.
       */
      void
      get(
        float& value);

      /**
       * @brief Get double value.
       *
       * @param value.
       */
      void
      get(
        double& value);

      /**
       * @brief Get string value.
       *
       * @param value.
       */
      void
      get(
        std::string& value);

      /**
       * @brief Get date value.
       *
       * @param value.
       */
      void
      get(
        Generics::ExtendedTime& value);

      /**
       * @brief Get time value.
       *
       * @param value.
       */
      void
      get(
        Generics::Time& value);

      /**
       * @brief Get long(text) value.
       *
       * @param value.
       */
      void
      get(
        AutoTest::DBC::TextField& value);

      /**
       * @brief Get result.
       *
       * @param Postgres result set.
       */
      PGresult*
      result_set();
      
    protected:
      std::string statement_;
      PGresult* result_set_;
      int row_index_;
      int value_index_;
      int last_;
    };
  }
}

#include "PGConnection.ipp"

#endif //__AUTOTESTS_COMMONS_PGCONNECTION_HPP
