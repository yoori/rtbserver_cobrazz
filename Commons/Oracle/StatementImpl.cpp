#include <orl.h>
#include <oci.h>
#include <oratypes.h>

namespace AdServer {
namespace Commons {
namespace Oracle
{
  namespace
  {
    const unsigned long SET_ARRAY_LOCK_BULK = 1000;

    struct NumberFormatProvider
    {
      NumberFormatProvider()
        : max_integer_digits_(1024),
          max_fraction_digits_(1024),
          format_source_(max_integer_digits_ + max_fraction_digits_ + 1, '9')
      {
        format_source_[max_integer_digits_] = '.';
      }

      String::SubString
      get(unsigned long integer_digits, unsigned long fraction_digits)
      {
        return String::SubString(
          format_source_.c_str() + max_integer_digits_ - integer_digits,
          integer_digits + (fraction_digits ? fraction_digits + 1 : 0));
      }

    private:
      unsigned long max_integer_digits_;
      unsigned long max_fraction_digits_;
      std::string format_source_;
    };

    NumberFormatProvider number_format_provider;

    unsigned long oci_type_to_sqlttype(Statement::DataTypes type)
    {
      switch(type)
      {
      case Statement::CHAR:
      case Statement::STRING:
        return SQLT_STR;
      case Statement::NUMBER:
      case Statement::INT:
      case Statement::UNSIGNED_INT:
      case Statement::FLOAT:
        return SQLT_VNU;
      case Statement::DATE:
        return SQLT_ODT;
      case Statement::TIMESTAMP:
        return SQLT_TIMESTAMP;
      case Statement::BLOB:
        return SQLT_BLOB;
      }

      return 0;
    }

    unsigned long oci_type_to_datatype(Statement::DataTypes type)
      noexcept
    {
      switch(type)
      {
      case Statement::CHAR:
      case Statement::STRING:
        return OCI_TYPECODE_VARCHAR2;
      case Statement::NUMBER:
      case Statement::INT:
      case Statement::UNSIGNED_INT:
      case Statement::FLOAT:
        return OCI_TYPECODE_NUMBER;
      case Statement::DATE:
      case Statement::TIMESTAMP:
        return OCI_TYPECODE_DATE;
      case Statement::BLOB:
        return OCI_TYPECODE_BLOB;
      }

      return 0;
    }

    template<unsigned long TypeId>
    void create_oci_datetime(
      const char* fun,
      OCIEnv* env_handle,
      OCIError* error_handle,
      const Generics::Time& date,
      OCIDescriptorPtr<OCIDateTime, TypeId>& datetime_handle)
      /*throw(SqlException)*/
    {
      sword result;

      if((result = OCIDescriptorAlloc(
            (dvoid*)env_handle,
            (dvoid**)&datetime_handle.fill(),
            (ub4)TypeId,
            (size_t)0,
            (dvoid **)0)) != OCI_SUCCESS)
      {
        throw_oci_error(fun, "OCIDescriptorAlloc", result);
      }

      Generics::ExtendedTime ex_time(date.get_gm_time());

      if((result = OCIDateTimeConstruct(
            env_handle,
            error_handle,
            datetime_handle.get(),
            ex_time.tm_year + 1900,
            ex_time.tm_mon + 1,
            ex_time.tm_mday,
            ex_time.tm_hour,
            ex_time.tm_min,
            ex_time.tm_sec,
            ex_time.tm_usec * 1000,
            (text*)"",
            0)) != OCI_SUCCESS)
      {
        throw_oci_error(fun, "OCIDateTimeConstruct", result);
      }
    }

    void create_oci_number_from_text(
      const char* fun,
      OCIError* error_handle,
      const std::string& number_text,
      OCINumber& oci_number)
      /*throw(Exception, SqlException, NotSupported)*/
    {
      std::string::size_type precision_pos = number_text.find('.');

      String::SubString number_format = number_format_provider.get(
        precision_pos != std::string::npos ?
          precision_pos : number_text.size(),
        precision_pos != std::string::npos ?
          number_text.size() - precision_pos - 1 : 0);

      sword result;
      if((result = OCINumberFromText(
            error_handle,
            (text*)number_text.c_str(),
            number_text.size(),
            reinterpret_cast<const text*>(number_format.data()),
            number_format.length(),
            (text*)"",
            0,
            &oci_number)) != OCI_SUCCESS)
      {
        throw_oci_error(fun, "OCINumberFromText", result);
      }
    }
  }

  /** ParamHolder's */
  struct ParamCommonHolder: public Statement::ParamValueHolder
  {
    virtual ~ParamCommonHolder() noexcept {}
    sb2 indicator;
    ub2 data_len;
  };

  struct ParamNumberHolder: public ParamCommonHolder
  {
    virtual ~ParamNumberHolder() noexcept {}
    OCINumber oci_number;
  };

  struct ParamDateTimeHolder: public ParamCommonHolder
  {
    virtual ~ParamDateTimeHolder() noexcept {}
    OCIDescriptorPtr<OCIDateTime, OCI_DTYPE_DATE> datetime_handle;
  };

  struct ParamTimestampHolder: public ParamCommonHolder
  {
    virtual ~ParamTimestampHolder() noexcept {}
    OCIDescriptorPtr<OCIDateTime, OCI_DTYPE_TIMESTAMP> datetime_handle;
    void* buf;
  };

  struct ParamStringHolder: public ParamCommonHolder
  {
    virtual ~ParamStringHolder() noexcept {}
    std::string oci_text;
  };

  struct ParamLobHolder: public ParamCommonHolder
  {
    ParamLobHolder(const Lob& lob_val) noexcept
      : lob(lob_val)
    {}

    virtual ~ParamLobHolder() noexcept {}

    static sword oci_in_bind_calback(
      dvoid* ctx,
      OCIBind* /*bindp*/,
      ub4 /*iter*/,
      ub4 /*index*/,
      dvoid** bufpp,
      ub4* alenp,
      ub1* piecep,
      dvoid** indpp)
    {
      ParamLobHolder* holder = static_cast<ParamLobHolder*>(ctx);
      *bufpp = const_cast<void*>(static_cast<const void*>(holder->lob.buffer));
      *alenp = holder->lob.length;
      *piecep = OCI_ONE_PIECE;
      *indpp = 0;
      return OCI_CONTINUE;
    }

    Lob lob;
  };

  struct ParamArrayHolder: public ParamCommonHolder
  {
    ParamArrayHolder(Statement* stmt) noexcept
      : statement(stmt),
        oci_named_type(
          statement->connection_->environment_->environment_handle_.get(),
          statement->error_handle_.get()),
        collection_handle(
          statement->connection_->environment_->environment_handle_.get(),
          statement->error_handle_.get())
    {}

    virtual ~ParamArrayHolder() noexcept {}

    Statement* statement;
    OCIObjectPtr<OCIType, true> oci_named_type;
    OCIObjectPtr<OCIColl> collection_handle;
    void* buf;
  };

  struct ParamStreamHolder: public ParamCommonHolder
  {
    ParamStreamHolder(Statement* stmt) noexcept
      : statement(stmt),
        oci_named_type(
          statement->connection_->environment_->environment_handle_.get(),
          statement->error_handle_.get()),
        collection_handle(
          statement->connection_->environment_->environment_handle_.get(),
          statement->error_handle_.get())
    {}

    virtual ~ParamStreamHolder() noexcept {}

    Statement* statement;
    OCIObjectPtr<OCIType, true> oci_named_type;
    OCIObjectPtr<OCIColl> collection_handle;
    void* buf;
  };

  /** Statement */
  Statement::Statement(
    Connection* connection,
    const char* query)
    /*throw(Exception, SqlException)*/
    : connection_(ReferenceCounting::add_ref(connection)),
      query_(query)
  {
    static const char* FUN = "Oracle::Statement::Statement()";

    sword result;

    if((result = OCIHandleAlloc(
          connection_->environment_->environment_handle_.get(),
          (void**)&stmt_handle_.fill(),
          OCI_HTYPE_STMT,
          0,
          0)) != OCI_SUCCESS)
    {
      throw_oci_error(FUN, "OCIHandleAlloc", result);
    }

    // allocate an error handle
    if((result = OCIHandleAlloc(
          connection_->environment_->environment_handle_.get(),
          (void **) &error_handle_.fill(),
          OCI_HTYPE_ERROR,
          0,
          0)) != OCI_SUCCESS)
    {
      throw_oci_error(FUN, "OCIHandleAlloc", result);
    }

    if((result = OCIStmtPrepare(
          stmt_handle_.get(),
          error_handle_.get(),
          (text*)query,
          strlen(query),
          OCI_NTV_SYNTAX,
          OCI_DEFAULT)) != OCI_SUCCESS)
    {
      throw_oci_error(
        FUN, "OCIStmtPrepare", result, error_handle_.get());
    }

    ub2 stmt_type;

    if((result = OCIAttrGet(
          stmt_handle_.get(),
          OCI_HTYPE_STMT,
          &stmt_type,
          0,
          OCI_ATTR_STMT_TYPE,
          error_handle_.get())) != OCI_SUCCESS)
    {
      throw_oci_error(
        FUN, "OCIAttrGet", result, error_handle_.get());
    }

    type_ = stmt_type;
  }

  void
  Statement::handle_timeout_() /*throw(SqlException, ConnectionError)*/
  {
    connection_->cancel();

    // free OCIStmt with OCI_ERROR skipping (for force mem leaks)
    stmt_handle_.reset(0, true);
  }

  bool
  Statement::execute_(const Generics::Time* timeout)
    /*throw(TimedOut, SqlException, NotSupported)*/
  {
    static const char* FUN = "Statement::execute_()";

    if (connection_->is_terminated_())
    {
      Stream::Error error;
      error << FUN << CONN_TERMINATED_ERROR;
      throw NotSupported(error);
    }

    ub4 iters = (type_ == OCI_STMT_SELECT) ? 0 : 1;
    sword result;

    TimeoutControl timer(use_timeout_(timeout));

    bool no_data = false;

    while((result = OCIStmtExecute(
          connection_->svc_context_handle_.get(),
          stmt_handle_.get(),
          error_handle_.get(),
          iters,
          0,
          0,
          0,
          OCI_DEFAULT)) != OCI_SUCCESS)
    {
      if(result != OCI_STILL_EXECUTING)
      {
        throw_oci_error(
          FUN, "OCIStmtExecute", result, error_handle_.get());
      }

      if(!timer.sleep_step())
      {
        handle_timeout_();

        throw_timeout_error(FUN,
          "OCIStmtExecute", timer.passed_time(), timer.timeout());
      }
    }

    /*
    OCICacheFree(
      connection_->environment_->environment_handle_.get(),
      error_handle_.get(),
      0);
//  connection_->svc_context_handle_.get());
*/
    return !no_data;
  }

  ResultSet_var
  Statement::execute_query_(
    const Generics::Time* timeout,
    unsigned long fetch_size)
    /*throw(TimedOut, Exception, SqlException, NotSupported)*/
  {
    static const char* FUN = "Statement::execute_query_()";

    check_terminated_(FUN);
    if(!execute_(timeout))
    {
      Stream::Error err;
      err << FUN << ": no data (valno) for query";
      throw Exception(err);
    }

    return new ResultSet(this, fetch_size);
  }

  void
  Statement::set_char(unsigned int col_index, char ch)
    /*throw(Exception, SqlException, NotSupported)*/
  {
    static const char* FUN = "Statement::set_char()";

    check_terminated_(FUN);

    char str[2] = { ch, 0 };
    set_string(col_index, str);
  }

  void
  Statement::set_date(unsigned int col_index, const Generics::Time& date)
    /*throw(Exception, SqlException, NotSupported)*/
  {
    static const char* FUN = "Statement::set_date()";

    check_terminated_(FUN);

    ParamDateTimeHolder* oci_datetime_holder = new ParamDateTimeHolder();
    params_.push_back(ParamValueHolder_var(oci_datetime_holder));

    create_oci_datetime(
      FUN,
      connection_->environment_->environment_handle_.get(),
      error_handle_.get(),
      date,
      oci_datetime_holder->datetime_handle);

    oci_datetime_holder->indicator = 0;
    oci_datetime_holder->data_len = sizeof(OCIDate);

    bind_(
      col_index,
      SQLT_ODT,
      oci_datetime_holder->datetime_handle.get(),
      sizeof(OCIDate),
      &oci_datetime_holder->indicator,
      &oci_datetime_holder->data_len);
  }

  void
  Statement::set_timestamp(
    unsigned int col_index, const Generics::Time& date)
    /*throw(Exception, SqlException, NotSupported)*/
  {
    static const char* FUN = "Statement::set_timestamp()";

    check_terminated_(FUN);

    ParamTimestampHolder* oci_datetime_holder = new ParamTimestampHolder();
    params_.push_back(ParamValueHolder_var(oci_datetime_holder));

    create_oci_datetime(
      FUN,
      connection_->environment_->environment_handle_.get(),
      error_handle_.get(),
      date,
      oci_datetime_holder->datetime_handle);

    oci_datetime_holder->indicator = 0;
    oci_datetime_holder->data_len = sizeof(OCIDateTime*);
    oci_datetime_holder->buf = oci_datetime_holder->datetime_handle.get();

    bind_(
      col_index,
      SQLT_TIMESTAMP,
      &oci_datetime_holder->buf,
      sizeof(OCIDateTime*),
      &oci_datetime_holder->indicator,
      &oci_datetime_holder->data_len);
  }

  void
  Statement::set_string(unsigned int col_index, const std::string& str)
    /*throw(SqlException, NotSupported)*/
  {
    static const char* FUN = "Statement::set_string()";

    check_terminated_(FUN);

    ParamStringHolder* oci_string_holder = new ParamStringHolder();
    params_.push_back(ParamValueHolder_var(oci_string_holder));
    oci_string_holder->oci_text = str;
    oci_string_holder->indicator = 0;
    oci_string_holder->data_len = oci_string_holder->oci_text.size() + 1;

    bind_(
      col_index,
      SQLT_STR,
      oci_string_holder->oci_text.c_str(),
      oci_string_holder->oci_text.size() + 1,
      &oci_string_holder->indicator,
      &oci_string_holder->data_len);
  }

  void
  Statement::set_uint(unsigned int col_index, unsigned int val)
    /*throw(SqlException, NotSupported)*/
  {
    static const char* FUN = "Statement::set_uint()";

    check_terminated_(FUN);

    ParamNumberHolder* oci_number_holder = new ParamNumberHolder();
    OCINumber& oci_num = oci_number_holder->oci_number;
    params_.push_back(ParamValueHolder_var(oci_number_holder));

    sword result;

    if((result = OCINumberFromInt(
          error_handle_.get(),
          &val,
          sizeof(unsigned int),
          OCI_NUMBER_UNSIGNED,
          &oci_num)) != OCI_SUCCESS)
    {
      throw_oci_error(FUN, "OCINumberFromInt", result);
    }

    oci_number_holder->indicator = 0;
    oci_number_holder->data_len = sizeof(OCINumber);

    bind_(
      col_index,
      SQLT_VNU,
      &oci_num,
      sizeof(OCINumber),
      &oci_number_holder->indicator,
      &oci_number_holder->data_len);
  }

  void
  Statement::set_double(unsigned int col_index, double val)
    /*throw(SqlException, NotSupported)*/
  {
    static const char* FUN = "Statement::set_double()";

    check_terminated_(FUN);

    ParamNumberHolder* oci_number_holder = new ParamNumberHolder();
    OCINumber& oci_num = oci_number_holder->oci_number;
    params_.push_back(ParamValueHolder_var(oci_number_holder));

    sword result;
    if((result = OCINumberFromReal(
          error_handle_.get(),
          &val,
          sizeof(double),
          &oci_num)) != OCI_SUCCESS)
    {
      throw_oci_error(FUN, "OCINumberFromReal", result);
    }

    oci_number_holder->indicator = 0;
    oci_number_holder->data_len = sizeof(OCINumber);

    bind_(
      col_index,
      SQLT_VNU,
      &oci_num,
      sizeof(OCINumber),
      &oci_number_holder->indicator,
      &oci_number_holder->data_len);
  }

  void
  Statement::set_number_from_string(
    unsigned int col_index,
    const char* val)
    /*throw(Exception, SqlException, NotSupported)*/
  {
    static const char* FUN = "Statement::set_number_from_string()";

    check_terminated_(FUN);

    ParamNumberHolder* oci_number_holder = new ParamNumberHolder();
    OCINumber& oci_number = oci_number_holder->oci_number;
    params_.push_back(ParamValueHolder_var(oci_number_holder));

    create_oci_number_from_text(
      FUN,
      error_handle_.get(),
      val,
      oci_number_holder->oci_number);

    oci_number_holder->indicator = 0;
    oci_number_holder->data_len = sizeof(OCINumber);

    bind_(
      col_index,
      SQLT_VNU,
      &oci_number,
      sizeof(OCINumber),
      &oci_number_holder->indicator,
      &oci_number_holder->data_len);
  }

  void
  Statement::set_blob(unsigned int col_index, const Lob& val)
    /*throw(Exception, SqlException, NotSupported)*/
  {
    static const char* FUN = "Statement::set_blob()";

    check_terminated_(FUN);

    unsigned long lob_len = val.length;

    ParamLobHolder* oci_lob_holder = new ParamLobHolder(val);
    params_.push_back(ParamValueHolder_var(oci_lob_holder));

    oci_lob_holder->indicator = 0;

    if(lob_len < 0xFFFF)
    {
      oci_lob_holder->data_len = oci_lob_holder->lob.length;

      bind_(
        col_index,
        SQLT_LBI,
        oci_lob_holder->lob.buffer,
        oci_lob_holder->lob.length,
        &oci_lob_holder->indicator,
        &oci_lob_holder->data_len);
    }
    else
    {
      sword result;
      OCIBind* bind_handle; // Bind handle can't be freed

      if((result = OCIBindByPos(
            stmt_handle_.get(),
            &bind_handle,
            error_handle_.get(),
            col_index,
            0,
            oci_lob_holder->lob.length,
            SQLT_LBI,
            (sb2*)0,
            (ub2*)0,
            (ub2*)0,
            0,
            (ub4*)0,
            OCI_DATA_AT_EXEC)) != OCI_SUCCESS)
      {
        throw_oci_error(FUN, "OCIBindByPos", result, error_handle_.get());
      }

      if((result = OCIBindDynamic(
            bind_handle,
            error_handle_.get(),
            static_cast<void*>(oci_lob_holder),
            ParamLobHolder::oci_in_bind_calback,
            0,
            0)) != OCI_SUCCESS)
      {
        throw_oci_error(FUN, "OCIBindByPos", result, error_handle_.get());
      }
    }
  }

  void
  Statement::set_null(unsigned int col_index, DataTypes type)
    /*throw(Exception, SqlException, NotSupported)*/
  {
    static const char* FUN = "Statement::set_null()";

    check_terminated_(FUN);

    unsigned long oci_data_type = oci_type_to_sqlttype(type);
    bind_(col_index, oci_data_type, 0, 0, 0, 0);
  }

  void
  Statement::set_number_array(
    unsigned int ind,
    std::vector<std::string>& vec,
    const char* type)
    /*throw(eh::Exception, TimedOut, Exception, SqlException, NotSupported)*/
  {
    static const char* FUN = "Statement::set_number_array()";

    check_terminated_(FUN);

    ParamArrayHolder* oci_array_holder = new ParamArrayHolder(this);
    params_.push_back(ParamValueHolder_var(oci_array_holder));

    sword result;
    OCIBind* bind_handle; // Bind handle can't be freed
    describe_type_(type, oci_array_holder->oci_named_type);

    if((result = OCIObjectNew(
          connection_->environment_->environment_handle_.get(),
          error_handle_.get(),
          connection_->svc_context_handle_.get(),
          OCI_TYPECODE_VARRAY,
          oci_array_holder->oci_named_type.get(),
          0,
          OCI_DURATION_SESSION,
          false,
          reinterpret_cast<void**>(
            &oci_array_holder->collection_handle.fill()))) != OCI_SUCCESS)
    {
      throw_oci_error(FUN, "OCIObjectNew", result, error_handle_.get());
    }

    oci_array_holder->buf = oci_array_holder->collection_handle.get();

    {
      unsigned long i = 0;
      std::unique_ptr<Sync::Policy::PosixThread::WriteGuard> guard_holder;

      for(std::vector<std::string>::const_iterator it =
            vec.begin();
          it != vec.end(); ++it, ++i)
      {
        if(i % SET_ARRAY_LOCK_BULK == 0)
        {
          guard_holder.reset(nullptr);
          guard_holder.reset(
            new Sync::Policy::PosixThread::WriteGuard(
              connection_->environment_->create_object_lock_));
        }

        OCINumber oci_number;
        create_oci_number_from_text(
          FUN,
          error_handle_.get(),
          *it,
          oci_number);

        OCICollAppend(
          connection_->environment_->environment_handle_.get(),
          error_handle_.get(),
          (dvoid*)&oci_number,
          (dvoid*)0,
          (OCIColl*)oci_array_holder->collection_handle.get());
      }
    }

    if((result = OCIBindByPos(
          stmt_handle_.get(),
          &bind_handle,
          error_handle_.get(),
          ind,
          oci_array_holder->collection_handle.get(),
          0,
          SQLT_NTY,
          0,
          0,
          0,
          0,
          (ub4*)0,
          OCI_DEFAULT)) != OCI_SUCCESS)
    {
      throw_oci_error(FUN, "OCIBindByPos", result, error_handle_.get());
    }

    if((result = OCIBindObject(
          bind_handle,
          error_handle_.get(),
          oci_array_holder->oci_named_type.get(),
          reinterpret_cast<dvoid**>(&oci_array_holder->buf),
          0,
          0,
          0)) != OCI_SUCCESS)
    {
      throw_oci_error(FUN, "OCIBindObject", result, error_handle_.get());
    }
  }

  void
  Statement::set_array(
    unsigned int ind,
    std::vector<std::string>& vec,
    const char* type)
    /*throw(eh::Exception, TimedOut, Exception, SqlException, NotSupported)*/
  {
    static const char* FUN = "Statement::set_array(1)";

    check_terminated_(FUN);

    ParamArrayHolder* oci_array_holder = new ParamArrayHolder(this);
    params_.push_back(ParamValueHolder_var(oci_array_holder));

    sword result;
    OCIBind* bind_handle; // Bind handle can't be freed
    describe_type_(type, oci_array_holder->oci_named_type);

    if((result = OCIObjectNew(
          connection_->environment_->environment_handle_.get(),
          error_handle_.get(),
          connection_->svc_context_handle_.get(),
          OCI_TYPECODE_VARRAY,
          oci_array_holder->oci_named_type.get(),
          0,
          OCI_DURATION_SESSION,
          false,
          reinterpret_cast<void**>(
            &oci_array_holder->collection_handle.fill()))) != OCI_SUCCESS)
    {
      throw_oci_error(FUN, "OCIObjectNew", result, error_handle_.get());
    }

    oci_array_holder->buf = oci_array_holder->collection_handle.get();

    {
      unsigned long i = 0;
      std::unique_ptr<Sync::Policy::PosixThread::WriteGuard> guard_holder;

      for(std::vector<std::string>::const_iterator it =
            vec.begin();
          it != vec.end(); ++it, ++i)
      {
        if(i % SET_ARRAY_LOCK_BULK == 0)
        {
          guard_holder.reset(nullptr);
          guard_holder.reset(
            new Sync::Policy::PosixThread::WriteGuard(
              connection_->environment_->create_object_lock_));
        }

        OCIObjectPtr<OCIString, false> oci_string(
          connection_->environment_->environment_handle_.get(),
          error_handle_.get());

        if((result = OCIStringAssignText(
              connection_->environment_->environment_handle_.get(),
              error_handle_.get(),
              (text*)it->c_str(),
              it->size(),
              &oci_string.fill())) != OCI_SUCCESS)
        {
          throw_oci_error(FUN,
            "OCIStringAssignText", result, error_handle_.get());
        }

        OCICollAppend(
          connection_->environment_->environment_handle_.get(),
          error_handle_.get(),
          (dvoid*)oci_string.get(),
          (dvoid*)0,
          (OCIColl*)oci_array_holder->collection_handle.get());
      }
    }

    if((result = OCIBindByPos(
          stmt_handle_.get(),
          &bind_handle,
          error_handle_.get(),
          ind,
          oci_array_holder->collection_handle.get(),
          0,
          SQLT_NTY,
          0,
          0,
          0,
          0,
          (ub4*)0,
          OCI_DEFAULT)) != OCI_SUCCESS)
    {
      throw_oci_error(FUN, "OCIBindByPos", result, error_handle_.get());
    }

    if((result = OCIBindObject(
          bind_handle,
          error_handle_.get(),
          oci_array_holder->oci_named_type.get(),
          reinterpret_cast<dvoid**>(&oci_array_holder->buf),
          0,
          0,
          0)) != OCI_SUCCESS)
    {
      throw_oci_error(FUN, "OCIBindObject", result, error_handle_.get());
    }
  }

  void Statement::describe_type_(
    const char* full_type_name,
    OCIObjectPtr<OCIType, true>& type_info)
    /*throw(TimedOut, SqlException)*/
  {
    static const char* FUN = "Statement::describe_type_()";

    std::string type_name = (
      full_type_name[0] == '.' ? full_type_name + 1 : full_type_name);

    sword result;
    while((result = OCITypeByName(
          connection_->environment_->environment_handle_.get(),
          error_handle_.get(),
          connection_->svc_context_handle_.get(),
          0,
          0,
          (text*)type_name.c_str(),
          (ub4)type_name.size(),
          0,
          0,
          OCI_DURATION_SESSION,
          OCI_TYPEGET_ALL,
          &type_info.fill()) == OCI_STILL_EXECUTING))
    {
    }

    if(result != OCI_SUCCESS)
    {
      throw_oci_error(FUN, "OCITypeByName", result, error_handle_.get());
    }
  }

  class OCIObjectFillCache
  {
  public:
    OCIObjectFillCache(Statement* statement)
      : statement_(statement)
    {};

    virtual ~OCIObjectFillCache() noexcept
    {
      static const char* FUN = "OCIObjectFillCache::~OCIObjectFillCache()";

      try
      {
        OCIEnv* environment_handle =
          statement_->connection_->environment_->environment_handle_.get();

        OCIError* error_handle = statement_->error_handle_.get();

        for(std::map<std::string, Cell>::iterator it =
              secondary_check_.begin();
            it != secondary_check_.end(); ++it)
        {
          sword result;
          if((result = OCIObjectFree(
                environment_handle,
                error_handle,
                it->second.oci_object,
                OCI_OBJECTFREE_FORCE)) != OCI_SUCCESS)
          {
            throw_oci_error(FUN,
              "OCIObjectFree", result, statement_->error_handle_.get());
          }

          OCIObjectPtr<OCIType, true> oci_type(
            environment_handle,
            error_handle,
            it->second.oci_type);
        }
      }
      catch(const eh::Exception& ex)
      {
        std::cerr << ex.what() << std::endl;
        assert(0);
      }
    }

    void get_for_fill(
      OCIType*& result_oci_type,
      void*& result_oci_object,
      const char* object_type_name)
      /*throw(SqlException)*/
    {
      static const char* FUN = "OCIObjectFillCache::get_for_fill()";

      const Cell* result_cell = 0;

      // fast check: main use case when equal type name string pointers is
      // equal
      std::map<const char*, Cell>::const_iterator fit =
        primary_check_.find(object_type_name);
      if(fit != primary_check_.end())
      {
        result_cell = &(fit->second);
      }
      else
      {
        std::map<std::string, Cell>::const_iterator sit =
          secondary_check_.find(object_type_name);
        if(sit != secondary_check_.end())
        {
          // don't insert into primary_check_
          result_cell = &(fit->second);
        }
        else
        {
          OCIEnv* environment_handle =
            statement_->connection_->environment_->environment_handle_.get();

          OCIObjectPtr<OCIType, true> oci_type(
            environment_handle, statement_->error_handle_.get());
          statement_->describe_type_(object_type_name, oci_type);

          Cell new_cell;

          int result;

          // create transient object (table = null, value = false)
          if((result = OCIObjectNew(
            environment_handle,
            statement_->error_handle_.get(),
            statement_->connection_->svc_context_handle_.get(),
            OCI_TYPECODE_OBJECT,
            oci_type.get(),
            0, // table = null
            OCI_DURATION_SESSION,
            false, // value = false
            &new_cell.oci_object)) != OCI_SUCCESS)
          {
            throw_oci_error(FUN,
              "OCIObjectNew", result, statement_->error_handle_.get());
          }

          // no throw section {
          new_cell.oci_type = oci_type.release();

          primary_check_.insert(std::make_pair(object_type_name, new_cell));
          result_cell = &(secondary_check_.insert(
            std::make_pair(object_type_name, new_cell)).first->second);
          // }
        }
      }

      result_oci_type = result_cell->oci_type;
      result_oci_object = result_cell->oci_object;
    }

  private:
    struct Cell
    {
      OCIType* oci_type;
      void* oci_object;
    };

  private:
    Statement* statement_;

    std::map<const char*, Cell> primary_check_;
    std::map<std::string, Cell> secondary_check_;
  };

  void
  Statement::set_array(
    unsigned int ind,
    std::vector<Object_var>& vec,
    const char* type)
    /*throw(eh::Exception, TimedOut, Exception, SqlException, NotSupported)*/
  {
    static const char* FUN = "Statement::set_array(2)";

    check_terminated_(FUN);

    ParamStreamHolder* stream_holder = new ParamStreamHolder(this);
    params_.push_back(ParamValueHolder_var(stream_holder));

    sword result;
    OCIBind* bind_handle; // Bind handle can't be freed
    describe_type_(type, stream_holder->oci_named_type);

    if((result = OCIObjectNew(
          connection_->environment_->environment_handle_.get(),
          error_handle_.get(),
          connection_->svc_context_handle_.get(),
          OCI_TYPECODE_VARRAY,
          stream_holder->oci_named_type.get(),
          0,
          OCI_DURATION_SESSION,
          false,
          reinterpret_cast<void**>(
            &stream_holder->collection_handle.fill()))) != OCI_SUCCESS)
    {
      throw_oci_error(FUN, "OCIObjectNew", result, error_handle_.get(), type);
    }

    stream_holder->buf = stream_holder->collection_handle.get();

    OCIObjectFillCache obj_fill_cache(this);

    {
      unsigned long i = 0;
      std::unique_ptr<Sync::Policy::PosixThread::WriteGuard> guard_holder;

      for(std::vector<Object_var>::const_iterator it =
            vec.begin();
          it != vec.end(); ++it, ++i)
      {
        if(i % SET_ARRAY_LOCK_BULK == 0)
        {
          guard_holder.reset(nullptr);
          guard_holder.reset(
            new Sync::Policy::PosixThread::WriteGuard(
              connection_->environment_->create_object_lock_));
        }

        void* cur_oci_object = 0;
        OCIType* cur_oci_type = 0;

        obj_fill_cache.get_for_fill(
          cur_oci_type, cur_oci_object, (*it)->getSQLTypeName());

        /* fill object */
//      std::cerr << "In: " << (void*)cur_oci_type << std::endl;
        assert(cur_oci_object && cur_oci_type);

        SqlStream_var object_stream(new SqlStream(this, cur_oci_type));
        (*it)->writeSQL(*object_stream);
        object_stream->close_(cur_oci_object);

        // OCICollAppend make deep copy of object - and object can be changed/freed after
        OCICollAppend(
          connection_->environment_->environment_handle_.get(),
          error_handle_.get(),
          (dvoid*)cur_oci_object,
          (dvoid*)object_stream->object_ind_(),
          (OCIColl*)stream_holder->collection_handle.get());
      }
    }

    if((result = OCIBindByPos(
          stmt_handle_.get(),
          &bind_handle,
          error_handle_.get(),
          ind,
          0,
          0,
          SQLT_NTY,
          0,
          0,
          0,
          0,
          (ub4*)0,
          OCI_DEFAULT)) != OCI_SUCCESS)
    {
      throw_oci_error(FUN, "OCIBindByPos", result, error_handle_.get());
    }

    if((result = OCIBindObject(
          bind_handle,
          error_handle_.get(),
          stream_holder->oci_named_type.get(),
          reinterpret_cast<dvoid**>(&stream_holder->buf),
          0,
          0,
          0)) != OCI_SUCCESS)
    {
      throw_oci_error(FUN, "OCIBindObject", result, error_handle_.get());
    }

    stream_holder->oci_named_type.release();
    stream_holder->collection_handle.release();
  }

  void Statement::bind_(
    unsigned long col_index,
    unsigned long oci_type,
    const void* buf,
    unsigned long size,
    void* indicator,
    void* data_len) /*throw(SqlException)*/
  {
    static const char* FUN = "Statement::bind_()";

    sword result;
    OCIBind* bind_handle; // Bind handle can't be freed

    if((result = OCIBindByPos(
          stmt_handle_.get(),
          &bind_handle,
          error_handle_.get(),
          col_index,
          const_cast<dvoid*>(buf),
          size,
          oci_type,
          static_cast<sb2*>(indicator),
          static_cast<ub2*>(data_len),
          (ub2*)0,
          0,
          (ub4*)0,
          OCI_DEFAULT)) != OCI_SUCCESS)
    {
      throw_oci_error(FUN, "OCIBindByPos", result, error_handle_.get());
    }
  }

  /** SqlStream */
  SqlStream::SqlStream(Statement* statement, void* type)
    /*throw(SqlException, NotSupported)*/
    : statement_(ReferenceCounting::add_ref(statement)),
      oci_type_(static_cast<OCIType*>(type))
  {
    static const char* FUN = "SqlStream::SqlStream()";

    oci_any_data_ = 0;

    sword result;

    OCIDurationBegin(
      statement_->connection_->environment_->environment_handle_.get(),
      statement_->error_handle_.get(),
      statement_->connection_->svc_context_handle_.get(),
      OCI_DURATION_SESSION,
      &oci_duration_);

    if((result = OCIAnyDataBeginCreate(
          statement_->connection_->svc_context_handle_.get(),
          statement_->error_handle_.get(),
          OCI_TYPECODE_OBJECT,
          oci_type_,
          oci_duration_,
          &oci_any_data_)) != OCI_SUCCESS)
    {
      throw_oci_error(FUN,
        "OCIAnyDataBeginCreate", result, statement_->error_handle_.get());
    }
  }

  SqlStream::~SqlStream() noexcept
  {
    OCIDurationEnd(
      statement_->connection_->environment_->environment_handle_.get(),
      statement_->error_handle_.get(),
      statement_->connection_->svc_context_handle_.get(),
      oci_duration_);
  }

  void
  SqlStream::set_date(const Generics::Time& date)
    /*throw(Exception, SqlException)*/
  {
    static const char* FUN = "SqlStream::set_date()";

    OCIDescriptorPtr<OCIDateTime, OCI_DTYPE_TIMESTAMP> datetime_handle;

    create_oci_datetime(
      FUN,
      statement_->connection_->environment_->environment_handle_.get(),
      statement_->error_handle_.get(),
      date,
      datetime_handle);

    set_object_attr_(FUN, datetime_handle.get(), OCI_TYPECODE_TIMESTAMP); // OCI_TYPECODE_DATE);
  }

  void
  SqlStream::set_string(const std::string& str)
    /*throw(SqlException)*/
  {
    static const char* FUN = "SqlStream::set_string()";

    sword result;

    if(!str.empty())
    {
      OCIObjectPtr<OCIString> value(
        statement_->connection_->environment_->environment_handle_.get(),
        statement_->error_handle_.get());

      if((result = OCIStringAssignText(
            statement_->connection_->environment_->environment_handle_.get(),
            statement_->error_handle_.get(),
            (text*)str.c_str(),
            str.size(),
            &value.fill())) != OCI_SUCCESS)
      {
        throw_oci_error(FUN,
          "OCIStringAssignText", result, statement_->error_handle_.get());
      }

      set_object_attr_(FUN, value.get(), OCI_TYPECODE_VARCHAR2);
    }
    else
    {
      set_object_attr_(FUN, 0, OCI_TYPECODE_VARCHAR2);
    }
  }

  void SqlStream::set_long(long val) /*throw(SqlException)*/
  {
    static const char* FUN = "SqlStream::set_long()";

    OCINumber oci_num;
    sword result;

    if((result = OCINumberFromInt(
          statement_->error_handle_.get(),
          &val,
          sizeof(long),
          OCI_NUMBER_SIGNED,
          &oci_num)) != OCI_SUCCESS)
    {
      throw_oci_error(FUN, "OCINumberFromInt", result);
    }

    set_object_attr_(FUN, &oci_num, OCI_TYPECODE_NUMBER);
  }

  void SqlStream::set_ulong(unsigned long val) /*throw(SqlException)*/
  {
    static const char* FUN = "SqlStream::set_ulong()";

    OCINumber oci_number;
    sword result;

    if((result = OCINumberFromInt(
          statement_->error_handle_.get(),
          &val,
          sizeof(unsigned long),
          OCI_NUMBER_UNSIGNED,
          &oci_number)) != OCI_SUCCESS)
    {
      throw_oci_error(FUN, "OCINumberFromInt", result);
    }

    set_object_attr_(FUN, &oci_number, OCI_TYPECODE_NUMBER);
  }

  void SqlStream::set_double(double val) /*throw(SqlException)*/
  {
    static const char* FUN = "SqlStream::set_double()";

    OCINumber oci_number;
    sword result;

    if((result = OCINumberFromReal(
          statement_->error_handle_.get(),
          &val,
          sizeof(double),
          &oci_number)) != OCI_SUCCESS)
    {
      throw_oci_error(FUN, "OCINumberFromReal", result);
    }

    set_object_attr_(FUN, &oci_number, OCI_TYPECODE_NUMBER);
  }

  void SqlStream::set_number_from_string(
    const char* val)
    /*throw(Exception, SqlException)*/
  {
    static const char* FUN = "SqlStream::set_number()";

    OCINumber oci_number;

    create_oci_number_from_text(
      FUN,
      statement_->error_handle_.get(),
      val,
      oci_number);

    set_object_attr_(FUN, &oci_number, OCI_TYPECODE_NUMBER);
  }

  void SqlStream::set_null(Statement::DataTypes type)
    /*throw(Exception, SqlException, NotSupported)*/
  {
    static const char* FUN = "SqlStream::set_null()";

    unsigned long oci_datatype = oci_type_to_datatype(type);
    set_object_attr_(FUN, 0, oci_datatype);
  }

  void SqlStream::close_(void* obj) /*throw(SqlException)*/
  {
    static const char* FUN = "SqlStream::close_()";

    assert(obj);

    sword result;

    if((result = OCIAnyDataEndCreate(
          statement_->connection_->svc_context_handle_.get(),
          statement_->error_handle_.get(),
          oci_any_data_)) != OCI_SUCCESS)
    {
      throw_oci_error(FUN,
        "OCIAnyDataEndCreate", result, statement_->error_handle_.get());
    }

    // TODO: check of set_.. counter (<MAX_OBJECT_FIELDS)
    OCIInd* indp = obj_ind_;
    ub4 len;

    if((result = OCIAnyDataAccess(
          statement_->connection_->svc_context_handle_.get(),
          statement_->error_handle_.get(),
          oci_any_data_,
          OCI_TYPECODE_OBJECT,
          oci_type_,
          &indp,
          &obj,
          &len)) != OCI_SUCCESS)
    {
      throw_oci_error(FUN,
        "OCIAnyDataAccess", result, statement_->error_handle_.get());
    }
  }

  void* SqlStream::object_ind_() noexcept
  {
    return obj_ind_;
  }

  void SqlStream::set_object_attr_(
    const char* fun, void* value, unsigned long type_id)
    /*throw(SqlException)*/
  {
    indicators_.push_back(value ? OCI_IND_NOTNULL : OCI_IND_NULL);
    sword result;

    if((result = OCIAnyDataAttrSet(
          statement_->connection_->svc_context_handle_.get(),
          statement_->error_handle_.get(),
          oci_any_data_,
          type_id,
          0,
          &*indicators_.rbegin(),
          value,
          0,
          false)) != OCI_SUCCESS)
    {
      throw_oci_error(fun,
        "OCIAnyDataAttrSet", result, statement_->error_handle_.get());
    }
  }
} /*Oracle*/
} /*Commons*/
} /*AdServer*/
