#include <tests/AutoTests/Commons/Common.hpp>

namespace AutoTest
{
  namespace DBC
  {

    //  interface destructors
    IConn::~IConn()
    { }

    IBasicQueryStream::~IBasicQueryStream()
    { }

    void
    IBasicQueryStream::set(
      const char* value)
    {
      set(std::string(value));
    }

    void
    IBasicQueryStream::null(
      const char*)
    {
      null(std::string());
    }
        
    IQuery::~IQuery()
    { }
    
    IResult::~IResult()
    { }

    // TextField class

    TextField::TextField() :
      text_()
    { }

    TextField::TextField(
      const std::string& initial) :
      text_(initial)
    { }

    void
    TextField::set(
      const std::string& value)
    {
      text_ = value;
    }

    const std::string&
    TextField::get() const
    {
      return text_;
    }

    void
    TextField::clear()
    {
      text_.clear();
    }

    // Conn class
    
    Conn::Conn(
      IConn* conn) :
      conn_(conn)
    { }
    
    Conn::~Conn()
    {
      delete conn_;
    }

    Conn::operator IConn&()
    {
      return *conn_;
    }

    Conn::operator const IConn&() const
    {
      return *conn_;
    }

    Conn&
    Conn::operator=(
      IConn* conn)
    {
      if (conn_)
      {
        delete conn_;
      }
      conn_ = conn;
      return *this;
    }

    void
    Conn::close()
    {
      conn_->close();
    }

    void
    Conn::commit()
    {
      conn_->commit();
    }

    IQuery*
    Conn::query(
      const std::string& statement)
    {
      return conn_->get_query(statement);
    }

    IConn*
    Conn::open_pq(
      const std::string& user,
      const std::string& pswd,
      const std::string& host,
      const std::string& db)
    {
      return
        new AutoTest::PQ::Conn(
          user, pswd, host, db);
    }

    // Query class
    
    Query::Query(
      IQuery* query) :
      query_(query)
    { }
    
    Query::~Query()
    {
      delete query_;
    }
    
    Query::operator IQuery&()
    {
      return *query_;
    }
    
    Query::operator const IQuery&() const
    {
      return *query_;
    }

    IResult*
    Query::ask()
    {
      return query_->ask_result();
    }

    int
    Query::update()
    {
      return query_->update();
    }

    IBasicQueryStream*
    Query::query_stream()
    {
      return query_->query_stream();
    }

    void
    Query::flush()
    {
      query_stream()->flush();
    }
    
    void
    Query::throw_exception(
      const std::string& message)
    {
      query_->throw_exception(message);
    }

    // Result class
    Result::Result(
      IResult* result) :
      result_(result)
    { }
    
     Result::~Result()
     {
       delete result_;
     }

    Result::operator IResult&()
    {
      return *result_;
    }

    Result::operator const IResult&() const
    {
      return *result_;
    }
    
    bool
    Result::next()
    {
      return result_->next();
    }

    bool
    Result::is_null()
    {
      return result_->is_null();
    }

    Result&
    Result::get(
      int& value)
    {
      result_->get(value);
      return *this;
    }

    Result&
    Result::get(
      bool& value)
    {
      result_->get(value);
      return *this;
    }
    
    Result&
    Result::get(
      unsigned int& value)
    {
      result_->get(value);
      return *this;
    }

    Result& Result::get(
      float& value)
    {
      result_->get(value);
      return *this;
    }
    
    Result& Result::get(
      double& value)
    {
      result_->get(value);
      return *this;
    }
    
    Result&
    Result::get(
      std::string& value)
    {
      result_->get(value);
      return *this;
    }
    
    Result& Result::get(
      Generics::ExtendedTime& value)
    {
      result_->get(value);
      return *this;
    }
    
    Result& Result::get(
      Generics::Time& value)
    {
      result_->get(value);
      return *this;
    }
    
    Result& Result::get(
      TextField& value)
    {
      result_->get(value);
      return *this;
    }
    
  }
}
