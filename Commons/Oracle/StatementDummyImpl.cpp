namespace AdServer {
namespace Commons {
namespace Oracle
{
  //
  // Statement
  //
  Statement::Statement(
    Connection* /*connection*/,
    const char* /*query*/)
    /*throw(Exception, SqlException)*/
  {}

  bool
  Statement::execute_(const Generics::Time* /*timeout*/)
    /*throw(TimedOut, SqlException, NotSupported)*/
  {
    return false;
  }

  ResultSet_var
  Statement::execute_query_(
    const Generics::Time* /*timeout*/,
    unsigned long /*fetch_size*/)
    /*throw(TimedOut, Exception, SqlException, NotSupported)*/
  {
    return ResultSet_var();
  }

  void
  Statement::set_char(unsigned int /*col_index*/, char /*ch*/)
    /*throw(Exception, SqlException, NotSupported)*/
  {}

  void
  Statement::set_date(unsigned int /*col_index*/, const Generics::Time& /*date*/)
    /*throw(Exception, SqlException, NotSupported)*/
  {}

  void
  Statement::set_timestamp(
    unsigned int /*col_index*/, const Generics::Time& /*date*/)
    /*throw(Exception, SqlException, NotSupported)*/
  {}

  void
  Statement::set_string(unsigned int /*col_index*/, const std::string& /*str*/)
    /*throw(SqlException, NotSupported)*/
  {}

  void
  Statement::set_uint(unsigned int /*col_index*/, unsigned int /*val*/)
    /*throw(SqlException, NotSupported)*/
  {}

  void
  Statement::set_double(unsigned int /*col_index*/, double /*val*/)
    /*throw(SqlException, NotSupported)*/
  {}

  void
  Statement::set_number_from_string(
    unsigned int /*col_index*/,
    const char* /*val*/)
    /*throw(Exception, SqlException, NotSupported)*/
  {}

  void
  Statement::set_blob(unsigned int /*col_index*/, const Lob& /*val*/)
    /*throw(Exception, SqlException, NotSupported)*/
  {}

  void
  Statement::set_null(unsigned int /*col_index*/, DataTypes /*type*/)
    /*throw(Exception, SqlException, NotSupported)*/
  {}

  void
  Statement::set_number_array(
    unsigned int /*ind*/,
    std::vector<std::string>& /*vec*/,
    const char* /*type*/)
    /*throw(eh::Exception, TimedOut, Exception, SqlException, NotSupported)*/
  {}

  void
  Statement::set_array(
    unsigned int /*ind*/,
    std::vector<std::string>& /*vec*/,
    const char* /*type*/)
    /*throw(eh::Exception, TimedOut, Exception, SqlException, NotSupported)*/
  {}

  void
  Statement::set_array(
    unsigned int /*ind*/,
    std::vector<Object_var>& /*vec*/,
    const char* /*type*/)
    /*throw(eh::Exception, TimedOut, Exception, SqlException, NotSupported)*/
  {}
  
  void
  Statement::bind_(
    unsigned long /*col_index*/,
    unsigned long /*oci_type*/,
    const void* /*buf*/,
    unsigned long /*size*/,
    void* /*indicator*/,
    void* /*data_len*/) /*throw(SqlException)*/
  {}

  //
  // class SqlStream
  //

  SqlStream::SqlStream(Statement* /*statement*/, void* /*type*/)
    /*throw(SqlException, NotSupported)*/
  {}

  SqlStream::~SqlStream() noexcept
  {}

  void
  SqlStream::set_date(const Generics::Time& /*date*/)
    /*throw(Exception, SqlException)*/
  {}

  void
  SqlStream::set_string(const std::string& /*str*/)
    /*throw(SqlException)*/
  {}

  void SqlStream::set_long(long /*val*/) /*throw(SqlException)*/
  {}

  void SqlStream::set_ulong(unsigned long /*val*/) /*throw(SqlException)*/
  {}

  void SqlStream::set_double(double /*val*/) /*throw(SqlException)*/
  {}

  void SqlStream::set_number_from_string(
    const char* /*val*/)
    /*throw(Exception, SqlException)*/
  {}

  void SqlStream::set_null(Statement::DataTypes /*type*/)
    /*throw(Exception, SqlException, NotSupported)*/
  {}

  void SqlStream::close_(void* /*obj*/) /*throw(SqlException)*/
  {}

  void* SqlStream::object_ind_() noexcept
  {
    return 0;
  }

  void SqlStream::set_object_attr_(
    const char* /*fun*/, void* /*value*/, unsigned long /*type_id*/)
    /*throw(SqlException)*/
  {}
}
}
}

