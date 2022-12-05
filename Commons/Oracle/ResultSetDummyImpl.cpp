namespace AdServer {
namespace Commons {
namespace Oracle
{
  ResultSet::ResultSet(
    Statement* /*statement*/,
    unsigned long /*fetch_size*/)
    /*throw(SqlException)*/
  {}

  unsigned long ResultSet::columns_count_() const /*throw(SqlException)*/
  {
    return 0;
  }
  
  unsigned long ResultSet::rows_count() const /*throw(SqlException)*/
  {
    return 0;
  }

  bool ResultSet::next() /*throw(TimedOut, SqlException, NotSupported)*/
  {
    return true;
  }

  bool
  ResultSet::is_null(unsigned int /*ind*/) const
    /*throw(SqlException, NotSupported)*/
  {
    return false;
  }

  ResultSet::Column::Column(
    Environment* /*environment_val*/,
    long /*oci_type_val*/,
    unsigned long /*size_val*/,
    unsigned long /*fetch_size_val*/)
    /*throw(SqlException, eh::Exception)*/
  {}

  void ResultSet::Column::init_() /*throw(SqlException, eh::Exception)*/
  {}

  void ResultSet::Column::init_fetch_cells_() /*throw(SqlException)*/
  {}

  void ResultSet::Column::clear_fetch_cells_() noexcept
  {}
  
  Generics::Time
  ResultSet::get_date_(const void* /*buf*/) const
    /*throw(Exception, SqlException, NotSupported)*/
  {
    return Generics::Time();
  }
  
  Generics::Time
  ResultSet::get_datetime_(const void* /*buf*/) const
    /*throw(Exception, SqlException, NotSupported)*/
  {
    return Generics::Time();
  }

  Generics::Time
  ResultSet::get_timestamp(unsigned int /*ind*/) const
    /*throw(Exception, SqlException, NotSupported)*/
  {
    return Generics::Time();
  }

  template<typename IntType, unsigned int OciSign>
  IntType ResultSet::get_long_(unsigned int /*ind*/) const
    /*throw(InvalidValue, SqlException, NotSupported)*/
  {
    return 0;
  }

  unsigned int
  ResultSet::get_uint(unsigned int /*ind*/) const
    /*throw(InvalidValue, SqlException, NotSupported)*/
  {
    return 0;
  }
  
  int
  ResultSet::get_int(unsigned int /*ind*/) const
    /*throw(InvalidValue, SqlException, NotSupported)*/
  {
    return 0;
  }

  uint64_t
  ResultSet::get_uint64(unsigned int /*ind*/) const
    /*throw(InvalidValue, SqlException, NotSupported)*/
  {
    return 0;
  }
  
  int64_t
  ResultSet::get_int64(unsigned int /*ind*/) const
    /*throw(InvalidValue, SqlException, NotSupported)*/
  {
    return 0;
  }

  double
  ResultSet::get_double(unsigned int /*ind*/) const
    /*throw(SqlException, NotSupported)*/
  {
    return 0;
  }

  std::string
  ResultSet::get_number_as_string(unsigned int /*ind*/) const
    /*throw(Overflow, Exception, SqlException)*/
  {
    return "";
  }

  std::string
  ResultSet::get_string(unsigned int /*ind*/) const
    /*throw(SqlException, NotSupported)*/
  {
    return 0;
  }

  char
  ResultSet::get_char(unsigned int /*ind*/) const
    /*throw(SqlException, InvalidValue)*/
  {
    return 0;
  }

  Lob
  ResultSet::get_blob(unsigned int /*ind*/) const
    /*throw(TimedOut, Exception, SqlException, NotSupported)*/
  {
    return Lob("", 0, false);
  }  

  void
  ResultSet::check_column_index_(unsigned long /*ind*/) const
    /*throw(SqlException)*/
  {}
} /*Oracle*/
} /*Commons*/
} /*AdServer*/
