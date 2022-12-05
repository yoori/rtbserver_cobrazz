
#ifndef __AUTOTESTS_COMMONS_CONNECTION_HPP
#define __AUTOTESTS_COMMONS_CONNECTION_HPP

#include <string>
#include <Generics/Uncopyable.hpp>
#include <Generics/Time.hpp>
#include "Logger.hpp"

namespace AutoTest
{
  namespace DBC
  {

    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    class IQuery;
    class IResult;

    /*
     * @class TextField
     * @brief Simple text field implementation
     */
    class TextField
    {
    public:

      /**
       * @brief Default constructor.
       */
      TextField();

      /**
       * @brief Constructor.
       *
       * @param text initial
       */
      explicit TextField(
        const std::string& initial);
      
      /**
       * @brief Set text.
       *
       * @param text value
       */
      void
      set(
        const std::string& value);

      /**
       * @brief Get text.
       *
       * @return text
       */
      const std::string& get() const;

      /**
       * @brief Clear field.
       */
      void clear();

    protected:
      std::string text_;
    };


    /**
    * @class IConn
    * @brief connecton interface
    */
    class IConn
    {
    public:
      /**
       * @brief Destructor.
       */
      virtual ~IConn() = 0;

      /**
       * @brief Close connection.
       */
      virtual void close() = 0;

      /**
       * @brief Commit changes.
       */
      virtual void commit () = 0;

      /**
       * @brief Create and get query.
       *
       * @param query statement
       * @return query interface
       */
      virtual IQuery*
      get_query(
        const std::string& statement) = 0;
    };

    /**
    * @class Conn
    * @brief IConn holder
    */
    class Conn : private Generics::Uncopyable
    {
    public:
      /**
       * @brief Constructor.
       *
       * @param holded connection 
       */
      Conn(IConn* conn = 0);

      /**
       * @brief Destructor.
       */
      ~Conn();

      /**
       * @brief Convert to IConn.
       */
      operator IConn&();

      /**
       * @brief Convert to const IConn.
       */
      operator const IConn&() const;

      /**
       * @brief Open postgres connection.
       *
       * @param user
       * @param password
       * @param host
       * @param database
       */
      static IConn*
      open_pq(
        const std::string& user,
        const std::string& pswd,
        const std::string& host,
        const std::string& db);

      /**
       * @brief Open Oracle connection.
       *
       * @param user
       * @param password
       * @param database
       */
      static IConn*
      open_ora(
        const std::string& user,
        const std::string& pswd,
        const std::string& db);

      /**
       * @brief Assignment from IConn.
       *
       * @param IConn
       */
      Conn&
      operator=(
        IConn* conn);

      /**
       * @brief Close connection.
       */
      void close();

      /**
       * @brief Commit changes.
       */
      void commit();

      /**
       * @brief Create & get query.
       *
       * @param SQL statement
       * @return IQuery
       */
      IQuery*
      query(
        const std::string& statement);

      /**
       * @brief Ask query.
       *
       * @param SQL statement
       * @param result
       */
      template<typename T>
      bool
      ask(
        const std::string& statement,
        T& value);

      /**
       * @brief Ask query with argument.
       *
       * @param SQL statement
       * @param argument
       * @param result
       */
      template<typename Arg1, typename T>
      bool
      ask(
        const std::string& statement,
        const Arg1& arg1,
        T& value);

      /**
       * @brief Ask query with arguments.
       *
       * @param SQL statement
       * @param argument 1
       * @param argument 2
       * @param result
       */
      template<typename Arg1, typename Arg2, typename T>
      bool
      ask(
        const std::string& statement,
        const Arg1& arg1,
        const Arg2& arg2,
        T& value);
      
    protected:
      IConn* conn_;
    };

    /**
    * @class IBasicQueryStream
    * @brief parameters setter interface
    */
    class IBasicQueryStream
    {
    public:
      /**
       * @brief Set char parameter.
       *
       * @param parameter value
       */
      virtual void set(char value) = 0;

      /**
       * @brief Set int parameter.
       *
       * @param parameter value
       */
      virtual void set (int value) = 0;


      /**
       * @brief Set bool parameter.
       *
       * @param parameter value
       */
      virtual void set (bool value) = 0;

      /**
       * @brief Set uint parameter.
       *
       * @param parameter value
       */
      virtual void set (unsigned int value) = 0;

      /**
       * @brief Set float parameter.
       *
       * @param parameter value
       */
      virtual void set (float value) = 0;

      /**
       * @brief Set double parameter.
       *
       * @param parameter value
       */
      virtual void set (double value) = 0;

      /**
       * @brief Set string parameter.
       *
       * @param parameter value
       */
      virtual void set (const char* value);

      /**
       * @brief Set string parameter.
       *
       * @param parameter value
       */
      virtual void set (const std::string& value) = 0;

      /**
       * @brief Set date parameter.
       *
       * @param parameter value
       */
      virtual void set (const Generics::ExtendedTime& value) = 0;

      /**
       * @brief Set timestamp parameter.
       *
       * @param parameter value
       */
      virtual void set (const Generics::Time& value) = 0;

      /**
       * @brief Set long(text) parameter.
       *
       * @param parameter value
       */
      virtual void set (const TextField& value) = 0;

      /**
       * @brief Set int parameter to null.
       */
      virtual void null (int value) = 0;

      /**
       * @brief Set bool parameter to null.
       */
      virtual void null (bool value) = 0;

      /**
       * @brief Set uint parameter to null.
       */
      virtual void null (unsigned int value) = 0;

      /**
       * @brief Set float parameter to null.
       */
      virtual void null (float value) = 0;

      /**
       * @brief Set double parameter to null.
       */
      virtual void null (double value) = 0;

      /**
       * @brief Set string parameter to null.
       */
      virtual void null (const char* value);
      
      /**
       * @brief Set string parameter to null.
       */
      virtual void null (const std::string& value) = 0;

      /**
       * @brief Set date parameter to null.
       */
      virtual void null (const Generics::ExtendedTime& value) = 0;

      /**
       * @brief Set timestamp parameter to null.
       */
      virtual void null (const Generics::Time& value) = 0;

      /**
       * @brief Set long(text) parameter to null.
       */
      virtual void null (const TextField& value) = 0;

      /**
       * @brief Get count of initialized parameters.
       *
       * @return count parameters.
       */
      virtual unsigned int size() const = 0;

      /**
       * @brief Process some actions after update.
       */
      virtual void flush() = 0;

    protected:
      /**
       * @brief Destructor.
       */
      virtual ~IBasicQueryStream() = 0;
    };

    /**
    * @class IQuery
    * @brief query interface
    */
    class IQuery
    {
    public:
      /**
       * @brief Destructor.
       */
      virtual ~IQuery() = 0;

      /**
       * @brief Ask & get query result.
       *
       * @return result
       */
      virtual IResult* ask_result () = 0;

      /**
       * @brief Execute query.
       *
       * @return execution status
       */      
      virtual int update () = 0;

      /**
       * @brief throw exception on query.
       *
       * @param error message
       */      
      virtual void throw_exception(
        const std::string& message) = 0;

      /**
       * @brief Get query stream.
       *
       * @return stream
       */      
      virtual IBasicQueryStream* query_stream () = 0;
    };
      
    /**
    * @class QueryStream
    * @brief mixing to wrap Query with overloaded parameters setters
    */
    template<typename T>
    class QueryStream
    {
    public:
      /**
       * @brief Set char parameter.
       *
       * @param parameter value
       * @return query
       */     
      T&
      set(
        char value);

      /**
       * @brief Set int parameter.
       *
       * @param parameter value
       * @return query
       */     
      T& set(
        int value);

      /**
       * @brief Set bool parameter.
       *
       * @param parameter value
       * @return query
       */     
      T& set(
        bool value);

      /**
       * @brief Set uint parameter.
       *
       * @param parameter value
       * @return query
       */     
      T&
      set(
        unsigned int value);

      /**
       * @brief Set float parameter.
       *
       * @param parameter value
       * @return query
       */     
      T&
      set(
        float value);

      /**
       * @brief Set double parameter.
       *
       * @param parameter value
       * @return query
       */     
      T&
      set(
        double value);

      /**
       * @brief Set string parameter.
       *
       * @param parameter value
       * @return query
       */ 
      T&
      set(
        const char* value);

      /**
       * @brief Set string parameter.
       *
       * @param parameter value
       * @return query
       */ 
      T&
      set(
        const std::string& value);

      /**
       * @brief Set date parameter.
       *
       * @param parameter value
       * @return query
       */ 
      T&
      set(
        const Generics::ExtendedTime& value);

      /**
       * @brief Set timestamp parameter.
       *
       * @param parameter value
       * @return query
       */
      T&
      set(
        const Generics::Time& value);

      /**
       * @brief Set long(text) parameter.
       *
       * @param parameter value
       * @return query
       */
      T&
      set(
        const TextField& value);

      /**
       * @brief Set int parameter to null.
       *
       * @return query
       */
      T&
      null(
        int value);
      
      /**
       * @brief Set bool parameter to null.
       *
       * @return query
       */
      T&
      null(
        bool value);


      /**
       * @brief Set uint parameter to null.
       *
       * @return query
       */
      T&
      null(
        unsigned int value);

      /**
       * @brief Set float parameter to null.
       *
       * @return query
       */
      T&
      null(
        float value);

      /**
       * @brief Set double parameter to null.
       *
       * @return query
       */
      T&
      null(
        double value);

      /**
       * @brief Set string parameter to null.
       *
       * @return query
       */
      T&
      null(
        const char* value);

      /**
       * @brief Set string parameter to null.
       *
       * @return query
       */
      T&
      null(
        const std::string& value);

      /**
       * @brief Set date parameter to null.
       *
       * @return query
       */
      T&
      null(
        const Generics::ExtendedTime& value);

      /**
       * @brief Set timestamp parameter to null.
       *
       * @return query
       */
      T& null(const Generics::Time& value);

      /**
       * @brief Set long(text) parameter to null.
       *
       * @return query
       */
      T& null(const TextField& value);

    protected:

      /**
       * @brief Get query stream interface.
       *
       * @return stream interface
       */
      IBasicQueryStream* query_stream() noexcept;
    };

    /**
    * @class Query
    * @brief IQuery holder
    */
    class Query :
      private Generics::Uncopyable,
      public QueryStream<Query>
    {
    public:
      /**
       * @brief Constructor.
       *
       * @param holded query
       */
      explicit Query(IQuery* query);

      /**
       * @brief Destructor.
       */
      virtual ~Query();

      /**
       * @brief Convert to IQuery.
       */
      operator IQuery&();

      /**
       * @brief Convert to const IQuery.
       */
      operator const IQuery&() const;

      /**
       * @brief Ask query and get result.
       *
       * @return result interface
       */
      IResult* ask();

      /**
       * @brief Execute query.
       *
       * @return execution status
       */
      int update();

      /**
       * @brief throw exception on query.
       *
       * @param error message
       */      
      void throw_exception(
        const std::string& message);

      /**
       * @brief Get query stream interface.
       *
       * @param query stream interface
       */            
      IBasicQueryStream* query_stream();

      /**
       * @brief Process some actions after update.
       */
      void flush();

      /**
       * @brief Get one row from result set
       *        Also check that result contain only one row.
       *
       * @return true if success
       */      
      template<class Getter>
      bool get_one (Getter getter);
      
    protected:
      IQuery* query_;
    };

    /**
    * @class IResult
    * @brief interface to query result
    */
    class IResult
    {
    public:
      /**
       * @brief Destructor.
       */
      virtual ~IResult () = 0;

      /**
       * @brief Goto next row.
       *
       * @return true if success.
       */
      virtual bool next () = 0;

      /**
       * @brief Check row is null.
       *
       * @return true if null.
       */
      virtual bool is_null() = 0;

      /**
       * @brief Get int value.
       *
       * @param value.
       */
      virtual void
      get(
        int& value) = 0;

      /**
       * @brief Get bool value.
       *
       * @param value.
       */
      virtual void
      get(
        bool& value) = 0;

      /**
       * @brief Get uint value.
       *
       * @param value.
       */
      virtual void
      get(
        unsigned int& value) = 0;

      /**
       * @brief Get float value.
       *
       * @param value.
       */
      virtual void
      get(
        float& value) = 0;

      /**
       * @brief Get double value.
       *
       * @param value.
       */
      virtual void
      get(
        double& value) = 0;

      /**
       * @brief Get string value.
       *
       * @param value.
       */
      virtual void
      get(
        std::string& value) = 0;

      /**
       * @brief Get date value.
       *
       * @param value.
       */
      virtual void
      get(
        Generics::ExtendedTime& value) = 0;

      /**
       * @brief Get timestamp value.
       *
       * @param value.
       */
      virtual void get (Generics::Time& value) = 0;

      /**
       * @brief Get long(text) value.
       *
       * @param value.
       */
      virtual void get (TextField& value) = 0;
    };

    /**
    * @class Result
    * @brief this result wrap IResult with overloaded results getters, and hold its pointer
    */
    class Result : private Generics::Uncopyable
    {
    public:
      /**
       * @brief Constructor.
       *
       * @param holded result
       */
      explicit Result(IResult* result);

      /**
       * @brief Destructor.
       */
      virtual ~Result ();

      /**
       * @brief Convert to IResult
       */
      operator IResult&();

      /**
       * @brief Convert to const IResult
       */
      operator const IResult&() const;

      /**
       * @brief Goto next row.
       *
       * @return true if success.
       */
      bool next();

      /**
       * @brief Goto next row.
       *
       * @return true if success.
       */
      bool is_null();

      /**
       * @brief Get int value.
       *
       * @param value.
       * @return result set
       */
      Result&
      get(
        int& value);

      /**
       * @brief Get bool value.
       *
       * @param value.
       * @return result set
       */
      Result&
      get(
        bool& value);

      /**
       * @brief Get uint value.
       *
       * @param value.
       * @return result set
       */
      Result&
      get(
        unsigned int& value);

      /**
       * @brief Get float value.
       *
       * @param value.
       * @return result set
       */
      Result&
      get(
        float& value);

      /**
       * @brief Get double value.
       *
       * @param value.
       * @return result set
       */
      Result&
      get(
        double& value);

      /**
       * @brief Get string value.
       *
       * @param value.
       * @return result set
       */
      Result&
      get(
        std::string& value);

      /**
       * @brief Get date value.
       *
       * @param value.
       * @return result set
       */      
      Result&
      get(
        Generics::ExtendedTime& value);
      
      /**
       * @brief Get timestamp value.
       *
       * @param value.
       * @return result set
       */      
      Result&
      get(
        Generics::Time& value);
      
      /**
       * @brief Get long(text) value.
       *
       * @param value.
       * @return result set
       */      
      Result&
      get(
        TextField& value);
      
    protected:
      IResult* result_;

    };

    /**
    * @class GetResult
    * @brief gcc < 4.3 bug 7412 [DR 106] References to references in bind1st(mem_fun) workaround
    */
    template<typename T>
    class GetResult
    {
    public:
      typedef void (T::*Method)(Result&);
    protected:
      T*  object_;
      Method  method_;
    public:
      
      GetResult (T* object, Method method);
      
      void operator()(Result& result);
    };

    template<typename T>
    GetResult<T>
    result_getter (T* object, void (T::*method)(Result&));
  }
}

#include "Connection.ipp"

#endif //__AUTOTESTS_COMMONS_CONNECTION_HPP
