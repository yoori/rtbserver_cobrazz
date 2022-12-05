namespace AutoTest
{
  namespace ORM
  {
    namespace DB = ::AutoTest::DBC;

    // IORMValue class

    inline
    bool
    IORMValue::changed() const
    {
      return _changed;
    }

    inline
    IORMValue&
    IORMValue::change()
    {
      _changed = true;
      return *this;
    }

    inline
    IORMValue&
    IORMValue::null()
    {
      _changed = true;
      _null    = true;
      return *this;
    }

    inline
    IORMValue&
    IORMValue::unnull()
    {
      _changed = true;
      _null    = false;
      return *this;
    }

    inline
    bool
    IORMValue::is_null() const
    {
      return _null;
    }

    inline
    DB::Query& 
    operator<<(
      DB::Query& left,
      IORMValue& value)
    {
      return value.set(left);
    }

    inline
    DB::Result&
    operator>>(
      DB::Result& left,
      IORMValue& value)
    {
      return value.get(left);
    }

    // ORMValue class

    template<typename T>
    ORMValue<T>::ORMValue() :
      _value(T())
    {
      _changed = false;
      _null = true;
    }

    template<typename T>
    ORMValue<T>::ORMValue(
      const T& value) :
      _value(value)
    {
      _changed = false;
      _null = false;
    }
      
    template<typename T>
    ORMValue<T>::ORMValue(
      const ORMValue& from) :
      IORMValue (),
      _value(from._value)
    {
      _changed = from._changed;
      _null = from._null;
    }
      
    template<typename T>
    ORMValue<T>::~ORMValue()
     { }
      
    template<typename T>
    ORMValue<T>& ORMValue<T>::set(
      const T& value)
    {
      _value   = value;
      _null    = false;
      _changed = false;
      return *this;
    }
    
    template<typename T>
    DB::Result&
    ORMValue<T>::get(
      DB::Result& result)
    {
      _null = result.is_null();
      result.get(_value);
      _changed = false;
      return result;
    }
    
    template<typename T>
    DB::Query&
    ORMValue<T>::set(
      DB::Query& query)
    {
      if(_null)
      {
        query.null(_value);
      }
      else
      {
        query.set(_value);
      }
      _changed = false;
      return query;
    }

    template<typename T>
    template<typename Q>
    DB::QueryStream<Q>&
    ORMValue<T>::set(
      DB::QueryStream<Q>& query)
    {
      query.set(_value);
      _changed = false;
      return query;
    }

    template<typename T>
    ORMValue<T>&
    ORMValue<T>::operator=(
      const T& value)
    {
      _value   = value;
      _null    = false;
      _changed = true;
      return *this;
    }

    template<typename T>
    ORMValue<T>&
    ORMValue<T>::operator=(
      const ORMValue& v)
    {
      if (_value != v._value ||
        _null != v._null)
      {
        _value   = v._value;
        _null    = v._null;
        _changed = true;
      }
      return *this;
    }

    template<typename T>
    const T&
    ORMValue<T>::value () const
    {
      return _value;
    }
    
    template<typename T>
    const T&
    ORMValue<T>::operator* () const
    {
      return value();
    }
    
    template<typename T>
    std::ostream& operator<< (std::ostream& out, const ORMValue<T>& val)
    {
      if (val.is_null())
        out << "null";
      else
        out << val.value();
      return out;
    }

    template<typename T>
    DB::Query& 
    operator<<(
      DB::Query& left,
      ORMValue<T>& value)
    {
      return value.set(left);
    }

    template<typename Q, typename T>
    Q& 
    operator<<(
      Q& query,
      ORMValue<T>& value)
    {
      typedef DB::QueryStream<Q> Stream;
      return static_cast<Q&>(value.set(static_cast< Stream& >(query)));
    }

    template<typename T>
    DB::Result&
    operator>>(
      DB::Result& left,
      ORMValue<T>& value)
    {
      return value.get(left);
    }

    template<typename T>
    std::string
    strof(
      const ORMValue<T>& value)
    {
      if (value.is_null())
        return "null";
      else
        return ::strof(value.value());
    }

    template<typename T1, typename T2>
    bool
    operator==(
      const ORMValue<T1>& lhs,
      const ORMValue<T2>& rhs)
    {
      if (lhs.is_null() && rhs.is_null()) return true;
      if (lhs.is_null() || rhs.is_null()) return false;
      return lhs.value() == rhs.value();
    }

    // ORMString class

    inline
    ORMString&
    ORMString::operator=(
      const char* value)
    {
      _changed = true;
      _null = false;
      _value = value;
      return *this;
    }

    inline
    ORMString&
    ORMString::operator=(
      const std::string& value)
    {
      _changed = true;
      _null = false;
      _value = value;
      return *this;
    }

    inline
    const std::string&
    strof(
      const ORMString& str)
    {
      return str.value();
    }

    inline
    DB::Query& 
    operator<<(
      DB::Query& left,
      ORMString& value)
    {
      return value.set(left);
    }

    inline
    DB::Result&
    operator>>(
      DB::Result& left,
      ORMString& value)
    {
      return value.get(left);
    }

    // ORMText class
    
    inline
    void
    ORMText::set_empty()
    {
      _changed = true;
      _null = false;
      _value.clear();
    }

    inline
    ORMText&
    ORMText::operator=(
      const std::string& value)
    {
      _changed = true;
      _null = false;
      _value.set(value);
      return *this;
    }

    inline
    ORMText&
    ORMText::operator=(
      const ORMText& v)
    {
      _value.set(v._value.get());
      _null    = v._null;
      _changed = true;
      return *this;
    }

    inline
    std::ostream&
    operator<<(
      std::ostream& out,
      const ORMText& val)
    {
      if (val.is_null())
        out << "null";
      else
        out << "<TEXT>";
      return out;
    }

    inline
    std::string
    strof(
      const ORMText&)
    {
      return "<TEXT>";
    }

    // ORMDate class
    
    inline
    ORMDate&
    ORMDate::operator=(
      const ::AutoTest::Time& value)
    {
      _changed = true;
      _null = false;
      _value = value.get_gm_time();
      return *this;
    }
    
    inline  
    ORMDate&
    ORMDate::operator=(
      const value_type& value)
    {
      _changed = true;
      _null = false;
      _value = value;
      return *this;
    }

    inline
    ORMDate&
    ORMDate::trunc()
    {
      if (!_null)
      {
        _value = _value.get_date();
      }
      return *this;
    }
    
    inline
    void
    ORMDate::set_null()
    {
      _value = Generics::Time::ZERO.get_gm_time();
      _changed = true;
      _null = true;
    }
    
    inline         
    ORMDate& ORMDate::set_now()
    {
      _value =
        Generics::Time::get_time_of_day().
          get_gm_time().
            get_date();
      _changed = true;
      _null = false;
      return *this;
    }
    
    inline
    std::ostream&
    operator<<(
      std::ostream& out,
      const ORMDate& date)
    {
      if (date.is_null())
      {
        out << "null";
      }
      else
      {
        out << date.value().format(
          AutoTest::DEBUG_TIME_FORMAT);
      }
      return out;
    }

    inline
    std::string
    strof(
      ORMDate& date)
    {
      if (date.is_null())
      {
        return "null";
      }
      else
      {
        return date.value().format(
          AutoTest::DEBUG_TIME_FORMAT);
      
      }
    }

    // ORMTimestamp class
    
    inline
    ORMTimestamp&
    ORMTimestamp::operator=(
      const ::AutoTest::Time& value)
    {
      _changed = true;
      _null = false;
      _value = value;
      return *this;
    }

    inline
    ORMTimestamp&
    ORMTimestamp::operator=(
      const value_type& value)
    {
      _changed = true;
      _null = false;
      _value = value;
      return *this;
    }

    inline
    ORMTimestamp&
    ORMTimestamp::trunc()
    {
      if (!_null)
      {
        _value =
          AutoTest::Time(
            _value.get_gm_time().get_date());
      }
      return *this;
    }

    inline
    void
    ORMTimestamp::set_null()
    {
      _value = Generics::Time::ZERO;
      _changed = true;
      _null = true;
    }
    
    inline
    ORMTimestamp&
    ORMTimestamp::set_now()
    {
      _value = Generics::Time::get_time_of_day();
      _changed = true;
      _null = false;
      return *this;
    }
    
    inline
    std::ostream&
    operator<<(
      std::ostream& out,
      const ORMTimestamp& date)
    {
      if (date.is_null())
      {
        out << "null";
      }
      else
      {
        out << date.value().get_gm_time().format(
          AutoTest::DEBUG_TIME_FORMAT);
      }
      return out;
    }

    inline
    std::string
    strof(
      ORMTimestamp& date)
    {
      if (date.is_null())
      {
        return "null";
      }
      else
      {
        return date.value().get_gm_time().format(
          AutoTest::DEBUG_TIME_FORMAT);
      
      }
    }

    // ORMObject
    template <typename ConnectionType>
    ORMObject<ConnectionType>::ORMObject(
      DB::IConn& connection) :
      conn(connection)
    { }

    template <typename ConnectionType>
    ORMObject<ConnectionType>::ORMObject(
      const ORMObject& obj) :
      conn(obj.conn)
    { }

    template <typename ConnectionType>
    ORMObject<ConnectionType>::~ORMObject()
    { }

    template <typename ConnectionType>
    bool
    ORMObject<ConnectionType>::touch()
    {
      return false;
    }

    template <typename ConnectionType>
    bool
    ORMObject<ConnectionType>::select()
    {
      return false;
    }

    template <typename ConnectionType>
    bool
    ORMObject<ConnectionType>::insert(bool)
    {
      return false;
    }

    template <typename ConnectionType>
    bool
    ORMObject<ConnectionType>::update(bool)
    {
      return false;
    }

    template <typename ConnectionType>
    bool
    ORMObject<ConnectionType>::del()
    {
      return false;
    }

    template <typename ConnectionType>
    void
    ORMObject<ConnectionType>::log_in(
      Logger&, 
      unsigned long)
    { }

    template <typename ConnectionType>
    void ORMObject<ConnectionType>::log(
      unsigned long severity)
    {
      log_in(Logger::thlog(), severity);
    }

    template<typename T>
    inline
    T*
    null()
    {
      return 0;
    }

    // ORMRestorer class
    template <typename Entity>
    ORMRestorer<Entity>::ORMRestorer(
      DB::IConn& conn,
      unsigned long id) :
      Restorer(Restorer::EE_EXISTS),
      Entity(conn, id),
      stored_(conn, id)
    {
      stored_.select();
      Entity::select();
    }

    template <typename Entity>
    ORMRestorer<Entity>::ORMRestorer(
      const Entity& entity) :
      Restorer(Restorer::EE_EXISTS),
      Entity(entity),
      stored_(entity)
    {
      entity_type_ = stored_.select()?
        Restorer::EE_EXISTS: Restorer::EE_NOT_EXISTS;
    }

    template <typename Entity>
    ORMRestorer<Entity>::ORMRestorer(
      DB::IConn& conn) :
      Restorer(Restorer::EE_NOT_EXISTS),
      Entity(conn),
      stored_(conn)
    { }

    template <typename Entity>
    ORMRestorer<Entity>::~ORMRestorer() noexcept
    {
      try
      {
        restore();
      }
      catch(const eh::Exception& exc)
      {
        Logger::thlog().log(exc.what(), Logging::Logger::ERROR);
      }
    }
    
    template <typename Entity>
    void
    ORMRestorer<Entity>::restore()
    {
      //std::cerr << stored_ << std::endl;
      if (entity_type_ == Restorer::EE_NOT_EXISTS)
      {
        Entity::delet();
      }
      else
      {
        Entity::operator=(stored_);
        if (stored_.select())
        {
          Entity::update(false);
        }
        else
        {
          Entity::insert(false);
        }
      }
    }       
  }
}
