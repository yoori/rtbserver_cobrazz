
namespace AutoTest
{
  namespace DBC
  {
    // Conn class

    template<typename T>
    inline
    bool
    Conn::ask(
      const std::string& statement,
      T& value)
    {
      Query q(query(statement));
      Result result(q.ask());
      if(result.next())
      {
        result.get(value);
        return true;
      }
      return false;
    }

    template<typename Arg1, typename T>
    inline
    bool
    Conn::ask(
      const std::string& statement,
      const Arg1& arg1,
      T& value)
    {
      AutoTest::Logger::thlog().debug_trace("ask query: " + statement);
      Query q(query(statement));
      q.set(arg1);
      Result result(q.ask());
      if(result.next())
      {
        result.get(value);
        return true;
      }
      return false;
    }

    template<typename Arg1, typename Arg2, typename T>
    inline
    bool
    Conn::ask(
      const std::string& statement,
      const Arg1& arg1,
      const Arg2& arg2,
      T& value)
    {
      AutoTest::Logger::thlog().debug_trace("ask query: " + statement);
      Query q(query(statement));
      q.set(arg1);
      q.set(arg2);
      Result result(q.ask());
      if(result.next())
      {
        result.get(value);
        return true;
      }
      return false;
    }

    // Query class
    template<typename Getter>
    bool
    Query::get_one(
      Getter getter)
    {
      try
      {
        Result result(ask());
        if(result.next())
        {
          getter(result);
          if(result.next())
          {
            throw_exception("got more then one row on request");
          }
          return true;
        }
        return false;
      }
      catch(const std::exception& e)
      {
        throw_exception(e.what());
      }
      return false;
    }

    // QueryStream class
    
    template<typename T>
    inline
    IBasicQueryStream*
    QueryStream<T>::query_stream() noexcept
    {
      return static_cast<T*>(this)->query_stream();
    }

    template<typename T>
    inline
    T&
    QueryStream<T>::null(
      int v)
    {
      query_stream()->null(v);
      return *static_cast<T*>(this);
    }

    template<typename T>
    inline
    T&
    QueryStream<T>::set(
      char value)
    {
      query_stream()->set(value);
      return *static_cast<T*>(this);
    }

    template<typename T>
    inline
    T&
    QueryStream<T>::null(
      bool v)
    {
      query_stream()->null(v);
      return *static_cast<T*>(this);
    }

    template<typename T>
    inline
    T&
    QueryStream<T>::set(
      bool value)
    {
      query_stream()->set(value);
      return *static_cast<T*>(this);
    }

    template<typename T>
    inline
    T&
    QueryStream<T>::set(
      int value)
    {
      query_stream()->set(value);
      return *static_cast<T*>(this);
    }

    template<typename T>
    inline
    T&
    QueryStream<T>::null(
      unsigned int v)
    {
      query_stream()->null(v);
      return *static_cast<T*>(this);
    }

    template<typename T>
    inline
    T&
    QueryStream<T>::set(
      unsigned int value)
    {
      query_stream()->set(value);
      return *static_cast<T*>(this);
    }

    template<typename T>
    inline
    T&
    QueryStream<T>::null(
      float v)
    {
      query_stream()->null(v);
      return *static_cast<T*>(this);
    }

    template<typename T>
    inline
    T&
    QueryStream<T>::set(
      float value)
    {
      query_stream()->set(value);
      return *static_cast<T*>(this);
    }

    template<typename T>
    inline
    T&
    QueryStream<T>::null(
      double v)
    {
      query_stream()->null(v);
      return *static_cast<T*>(this);
    }

    template<typename T>
    inline
    T&
    QueryStream<T>::set(
      double value)
    {
      query_stream()->set(value);
      return *static_cast<T*>(this);
    }

    template<typename T>
    inline
    T&
    QueryStream<T>::null(
      const char* v)
    {
      query_stream()->null(v);
      return *static_cast<T*>(this);
    }

    template<typename T>
    inline
    T&
    QueryStream<T>::set(
      const char*  value)
    {
      query_stream()->set(value);
      return *static_cast<T*>(this);
    }

    template<typename T>
    inline
    T&
    QueryStream<T>::null(
      const std::string& v)
    {
      query_stream()->null(v);
      return *static_cast<T*>(this);
    }

    template<typename T>
    inline
    T&
    QueryStream<T>::set(
      const std::string&  value)
    {
      query_stream()->set(value);
      return *static_cast<T*>(this);
    }

    template<typename T>
    inline
    T&
    QueryStream<T>::null(
      const Generics::ExtendedTime& v)
    {
      query_stream()->null(v);
      return *static_cast<T*>(this);
    }

    template<typename T>
    inline
    T&
    QueryStream<T>::set(
      const Generics::ExtendedTime&  value)
    {
      query_stream()->set(value);
      return *static_cast<T*>(this);
    }

    template<typename T>
    inline
    T&
    QueryStream<T>::null(
      const Generics::Time& v)
    {
      query_stream()->null(v);
      return *static_cast<T*>(this);
    }

    template<typename T>
    inline
    T&
    QueryStream<T>::set(
      const Generics::Time&  value)
    {
      query_stream()->set(value);
      return *static_cast<T*>(this);
    }


    template<typename T>
    inline
    T&
    QueryStream<T>::null(
      const TextField& v)
    {
      query_stream()->null(v);
      return *static_cast<T*>(this);
    }

    template<typename T>
    inline
    T&
    QueryStream<T>::set(
      const TextField& value)
    {
      query_stream()->set(value);
      return *static_cast<T*>(this);
    }

    // GetResult
    template<typename T>
    GetResult<T>::GetResult(
      T* object, Method method) :
      object_(object),
      method_(method)
    { }

    template<typename T>
    void
    GetResult<T>::operator()(
      Result& result)
    {
      (object_->*method_)(result);
    }

    template<typename T>
    GetResult<T>
    result_getter (
      T* object,
      void (T::*method)(Result&))
    {
      return GetResult<T> (object, method);
    }

  }
}
