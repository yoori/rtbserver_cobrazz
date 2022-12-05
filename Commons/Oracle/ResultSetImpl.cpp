#include <orl.h>
#include <oci.h>
#include <oratypes.h>

namespace
{
  const unsigned long MAX_GET_NUMBER_SIZE = 1024;
  const char GET_NUMBER_FORMAT[] = "FM9.99999999999999999999999999999999999999EEEE";
}

namespace AdServer {
namespace Commons {
namespace Oracle
{
  namespace
  {
    void oci_number_to_string(
      OCIError* error_handle,
      std::ostream& str,
      const OCINumber* oci_number)
      noexcept
    {
      char buf[MAX_GET_NUMBER_SIZE];
      ub4 buf_size = sizeof(buf);

      sword result;
      if((result = OCINumberToText(
            error_handle,
            oci_number,
            reinterpret_cast<const text*>(GET_NUMBER_FORMAT),
            sizeof(GET_NUMBER_FORMAT) - 1,
            0, // nls params
            0, // nls params length
            &buf_size,
            reinterpret_cast<text*>(buf))) != OCI_SUCCESS)
      {
        oci_error_text(str, "oci_number_to_string", "OCINumberToText", result, error_handle);
      }
      else
      {
        str << buf;
      }
    }
  }

  ResultSet::ResultSet(
    Statement* statement,
    unsigned long fetch_size)
    /*throw(SqlException)*/
    : statement_(ReferenceCounting::add_ref(statement)),
      fetch_count_(fetch_size),
      rows_fetched_(0),
      current_row_(0),
      is_eod_(false)
  {
    static const char* FUN = "ResultSet::ResultSet()";

    sword result;

    // allocate an error handle
    if((result = OCIHandleAlloc(
          statement_->connection_->environment_->environment_handle_.get(),
          (void **) &error_handle_.fill(),
          OCI_HTYPE_ERROR,
          0, // extra memory to allocate
          0)) != OCI_SUCCESS)
    {
      throw_oci_error(FUN, "OCIHandleAlloc", result);
    }

    ub4 count = columns_count_();
    columns_.reserve(count);

    OCIStmt* rs_handle = statement_->stmt_handle_.get();

    for(ub4 i = 0; i < count; ++i)
    {
      // get next column info
      OCIDescriptorPtr<OCIParam, OCI_DTYPE_PARAM> param_handle;
      ub2 oci_data_type = 0;
      ub4 size = 0;

      if((result = OCIParamGet(
            rs_handle,
            OCI_HTYPE_STMT,
            error_handle_.get(),
            reinterpret_cast<void **>(&param_handle.fill()),
            i + 1)) != OCI_SUCCESS)
      {
        throw_oci_error(FUN, "OCIParamGet", result, error_handle_.get());
      }

      // oci data type
      if((result = OCIAttrGet(
            param_handle.get(),
            OCI_DTYPE_PARAM,
            &oci_data_type,
            0,
            OCI_ATTR_DATA_TYPE,
            error_handle_.get())) != OCI_SUCCESS)
      {
        throw_oci_error(FUN, "OCIAttrGet", result, error_handle_.get());
      }

      if((result = OCIAttrGet(
            param_handle.get(),
            OCI_DTYPE_PARAM,
            &size,
            0,
            OCI_ATTR_DATA_SIZE,
            error_handle_.get())) != OCI_SUCCESS)
      {
        throw_oci_error(FUN, "OCIAttrGet", result, error_handle_.get());
      }

      if(param_handle.get())
      {
        param_handle.reset(0);

        unsigned long oci_type;

	// decide the format we want oci to return data in (oci_type member)
        switch (oci_data_type)
        {
        case SQLT_INT: // integer
	case SQLT_LNG: // long
	case SQLT_UIN: // unsigned int
        case SQLT_NUM: // numeric
        case SQLT_FLT: // float
        case SQLT_VNU: // numeric with length
	case SQLT_PDN: // packed decimal
          oci_type = SQLT_VNU;
          size = sizeof (OCINumber);
          break;
        case SQLT_DAT: // date
	case SQLT_ODT: // oci date
          oci_type = SQLT_ODT;
          size = sizeof(OCIDate);
          break;
        case SQLT_DATE:
        case SQLT_TIME:
        case SQLT_TIME_TZ:
          oci_type = SQLT_TIME;
          size = sizeof(OCIDateTime*);
          break;
        case SQLT_TIMESTAMP:
        case SQLT_TIMESTAMP_TZ:
        case SQLT_TIMESTAMP_LTZ:
          oci_type = SQLT_TIMESTAMP;
          size = sizeof(OCIDateTime*);
          break;
        case SQLT_CHR: // character string
        case SQLT_STR: // zero-terminated string
        case SQLT_VCS: // variable-character string
        case SQLT_AFC: // ansi fixed char
        case SQLT_AVC: // ansi var char
        case SQLT_VST: // oci string type
          oci_type = SQLT_STR;
          size = size + 1;
          break;
        case SQLT_BLOB:
          oci_type = SQLT_BLOB;
          size = sizeof(OCILobLocator*);
          break;
        default:
          Stream::Error error;
          error << FUN << ": unknown column type = " << oci_data_type
            << " at column #" << i;
          throw SqlException(error);
	};

        Column_var c = new Column(
          statement_->connection_->environment_,
          oci_type,
          size,
          fetch_size);

        columns_.push_back(c);
      }
    }

    // define all columns
    ub4 position = 1;

    for (ColumnList::iterator it = columns_.begin();
         it != columns_.end(); ++it)
    {
      sword result;

      OCIDefine* define_handle = 0;

      if((result = OCIDefineByPos(
            rs_handle,
            &define_handle,
            error_handle_.get(),
            position++,
            (*it)->fetch_buffer.get(),
            (*it)->size, // fetch size for a single row (NOT for several)
            (*it)->oci_type,
            (*it)->indicators.get(),
            (*it)->data_lens.get(),
            0, // ptr to array of column-level return codes
            OCI_DEFAULT)) != OCI_SUCCESS)
      {
        throw_oci_error(FUN, "OCIDefineByPos", result, error_handle_.get());
      }
    }
  }

  unsigned long
  ResultSet::columns_count_() const /*throw(SqlException)*/
  {
    static const char* FUN = "ResultSet::columns_count_()";

    sword result;
    ub4 count = 0;

    if((result = OCIAttrGet(
          statement_->stmt_handle_.get(),
          OCI_HTYPE_STMT,
          &count,
          0,
          OCI_ATTR_PARAM_COUNT,
          error_handle_.get())) != OCI_SUCCESS)
    {
      throw_oci_error(FUN, "OCIAttrGet", result, error_handle_.get());
    }

    return count;
  }

  unsigned long
  ResultSet::rows_count() const /*throw(SqlException)*/
  {
    static const char* FUN = "ResultSet::rows_count()";

    OCIStmt* rs_handle = statement_->stmt_handle_.get();

    sword result;
    ub4 count = 0;

    if((result = OCIAttrGet (
          rs_handle,
          OCI_HTYPE_STMT,
          &count,
          0,
          OCI_ATTR_ROW_COUNT,
          error_handle_.get())) != OCI_SUCCESS)
    {
      throw_oci_error(FUN, "OCIAttrGet", result, error_handle_.get());
    }

    return count;
  }

  bool ResultSet::next() /*throw(TimedOut, SqlException, NotSupported)*/
  {
    static const char* FUN = "ResultSet::next()";

    check_terminated_(FUN);

    OCIStmt* rs_handle = statement_->stmt_handle_.get();

    ++current_row_;
    if(current_row_ > rows_fetched_)
    {
      if (!is_eod_)
      {
        // fetch new block of rows; fetch_rows will set is_eod on true
        // when last block if rows has been fetched; will also update rows_fetched
        sword result;
        ub4 old_rows_count = rows_fetched_;

        TimeoutControl timer(statement_->use_timeout_(0));

        columns_[0]->clear_fetch_cells_();
        columns_[0]->init_fetch_cells_();

        while((result = OCIStmtFetch2(
          rs_handle,
          error_handle_.get(),
          fetch_count_,
          OCI_FETCH_NEXT,
          1,
          OCI_DEFAULT)) == OCI_STILL_EXECUTING)
        {
          if(!timer.sleep_step())
          {
            result = OCIStmtFetch2(
              rs_handle,
              error_handle_.get(),
              0,
              OCI_FETCH_NEXT,
              1,
              OCI_DEFAULT);

            if(result != OCI_SUCCESS &&
              result != OCI_NO_DATA &&
              result != OCI_SUCCESS_WITH_INFO)
            {
              throw_oci_error(FUN, "OCIStmtFetch2(cancel)", result);
            }

            statement_->handle_timeout_();

            throw_timeout_error(FUN,
              "OCIStmtFetch2", timer.passed_time(), timer.timeout());
          }

/*
          std::cerr << "OCIStmtFetch2 recall:";
          for(unsigned long i = 0; i < fetch_count_; ++i)
          {
            OCILobLocator* oci_lob =
              const_cast<OCILobLocator*>(
                *(reinterpret_cast<const OCILobLocator**>(
                    columns_[0]->fetch_buffer.get()) + i));

            std::cerr << " " << oci_lob;
          }
          std::cerr << std::endl;
*/
        }

        if (result == OCI_SUCCESS ||
            result == OCI_NO_DATA ||
            result == OCI_SUCCESS_WITH_INFO)
        {
          rows_fetched_ = rows_count();
          if (rows_fetched_ - old_rows_count != fetch_count_)
          {
            is_eod_ = true;
          }
        }
        else
        {
          Stream::Error err;
          oci_error_text(err, FUN, "OCIStmtFetch", result, error_handle_.get());
          err << ", rows fetched = " << rows_fetched_ <<
            " current row = " << current_row_;
          throw SqlException(err);
        }
      }
      else
      {
        return false;
      }
    }

    if(current_row_ > rows_fetched_)
    {
      return false;
    }

    return true;
  }

  bool ResultSet::is_null(unsigned int ind) const
    /*throw(SqlException, NotSupported)*/
  {
    static const char* FUN = "ResultSet::is_null()";

    check_column_index_(ind);

    check_terminated_(FUN);

    ub2 row_no = static_cast<ub2>((current_row_ - 1) % fetch_count_);
    return (columns_[ind - 1]->indicators[row_no] == -1);
  }

  ResultSet::Column::Column(
    Environment* environment_val,
    long oci_type_val,
    unsigned long size_val,
    unsigned long fetch_size_val)
    /*throw(SqlException, eh::Exception)*/
    : environment(environment_val),
      oci_type(oci_type_val),
      size(size_val),
      fetch_size(fetch_size_val)
  {
    init_();
  }

  void ResultSet::Column::init_() /*throw(SqlException, eh::Exception)*/
  {
    fetch_buffer.reset(size*fetch_size);
    init_fetch_cells_();

    indicators.reset(fetch_size);
    data_lens.reset(fetch_size);
  }

  void ResultSet::Column::init_fetch_cells_() /*throw(SqlException)*/
  {
    static const char* FUN = "ResultSet::Column::init_fetch_cells_()";

    if(oci_type == SQLT_TIME ||
       oci_type == SQLT_TIMESTAMP)
    {
      ub4 dtype = (oci_type == SQLT_TIME ? OCI_DTYPE_TIME : OCI_DTYPE_TIMESTAMP);

      for(unsigned long i = 0; i < fetch_size; ++i)
      {
        sword result;

        if((result = OCIDescriptorAlloc(
              (dvoid*)environment->environment_handle_.get(),
              (dvoid**)reinterpret_cast<OCIDateTime**>(
                fetch_buffer.get() + size * i),
              dtype,
              (size_t)0,
              (dvoid**)0)) != OCI_SUCCESS)
        {
          throw_oci_error(FUN, "OCIDescriptorAlloc", result);
        }
      }
    }
    else if(oci_type == SQLT_BLOB)
    {
      for(unsigned long i = 0; i < fetch_size; ++i)
      {
        sword result;

        if((result = OCIDescriptorAlloc(
              (dvoid*)environment->environment_handle_.get(),
              (dvoid**)reinterpret_cast<OCILobLocator**>(
                fetch_buffer.get() + size * i),
              (ub4)OCI_DTYPE_LOB,
              (size_t)0,
              (dvoid **)0)) != OCI_SUCCESS)
        {
          throw_oci_error(FUN, "OCIDescriptorAlloc", result);
        }
      }
    }
  }

  void ResultSet::Column::clear_fetch_cells_() noexcept
  {
    if(oci_type == SQLT_TIME || oci_type == SQLT_TIMESTAMP)
    {
      ub4 dtype =
        (oci_type == SQLT_TIME ? OCI_DTYPE_TIME : OCI_DTYPE_TIMESTAMP);

      for(unsigned long i = 0; i < fetch_size; ++i)
      {
        OCIDescriptorFree(
          *reinterpret_cast<OCIDateTime**>(
            fetch_buffer.get() + size * i),
          dtype);
      }
    }
    else if(oci_type == SQLT_BLOB)
    {
      for(unsigned long i = 0; i < fetch_size; ++i)
      {
        OCIDescriptorFree(
          *reinterpret_cast<OCILobLocator**>(
            fetch_buffer.get() + size * i),
          (ub4)OCI_DTYPE_LOB);
      }
    }
  }

  Generics::Time
  ResultSet::get_date_(const void* buf) const
    /*throw(Exception, SqlException, NotSupported)*/
  {
    const OCIDate* oci_date = reinterpret_cast<const OCIDate*>(buf);
    sb2 year;
    ub1 month, day, hour, min, sec;

    OCIDateGetDate(
      oci_date,
      &year,
      &month,
      &day);

    OCIDateGetTime(
      oci_date,
      &hour,
      &min,
      &sec);

    return Generics::ExtendedTime(
      year,
      month,
      day,
      hour,
      min,
      sec,
      0);
  }

  Generics::Time
  ResultSet::get_datetime_(const void* buf) const
    /*throw(Exception, SqlException, NotSupported)*/
  {
    static const char* FUN = "ResultSet::get_datetime_()";

    OCIDateTime* oci_date = reinterpret_cast<OCIDateTime*>(
      const_cast<void*>(buf));
    sb2 year;
    ub1 month, day, hour, min, sec;
    ub4 fsec;
    sword result;

    if((result = OCIDateTimeGetDate(
          statement_->connection_->environment_->environment_handle_.get(),
          error_handle_.get(),
          oci_date,
          &year,
          &month,
          &day)) != OCI_SUCCESS)
    {
      throw_oci_error(FUN, "OCIDateTimeGetDate", result, error_handle_.get());
    }

    if((result = OCIDateTimeGetTime(
          statement_->connection_->environment_->environment_handle_.get(),
          error_handle_.get(),
          oci_date,
          &hour,
          &min,
          &sec,
          &fsec)) != OCI_SUCCESS)
    {
      throw_oci_error(FUN, "OCIDateTimeGetTime", result, error_handle_.get());
    }

    return Generics::ExtendedTime(
      year,
      month,
      day,
      hour,
      min,
      sec,
      fsec / 1000);
  }

  Generics::Time
  ResultSet::get_timestamp(unsigned int ind) const
    /*throw(Exception, SqlException, NotSupported)*/
  {
    static const char* FUN = "ResultSet::get_timestamp()";

    check_column_index_(ind);

    ub2 row_no = static_cast<ub2>((current_row_ - 1) % fetch_count_);
    const Column* column = columns_[ind - 1];
    if(column->indicators[row_no] != -1)
    {
      if(column->oci_type == SQLT_ODT)
      {
        const OCIDate* oci_date =
          reinterpret_cast<const OCIDate*>(
            reinterpret_cast<const char*>(
              column->fetch_buffer.get()) + row_no * column->size);
        return get_date_(oci_date);
      }
      else if(column->oci_type == SQLT_TIME ||
        column->oci_type == SQLT_TIMESTAMP)
      {
        const OCIDateTime** oci_date =
          reinterpret_cast<const OCIDateTime**>(
            column->fetch_buffer.get()) + row_no;
        return get_datetime_(*oci_date);
      }
    }

    throw_type_error(
      FUN, ind, column->oci_type, column->indicators[row_no] == -1);

    return Generics::Time();
  }

  template<typename IntType, unsigned int OciSign>
  IntType
  ResultSet::get_long_(unsigned int ind) const
    /*throw(InvalidValue, SqlException, NotSupported)*/
  {
    static const char* FUN = "ResultSet::get_long_()";

    check_column_index_(ind);

    ub2 row_no = static_cast<ub2>((current_row_ - 1) % fetch_count_);

    const Column* column = columns_[ind - 1];
    if(column->indicators[row_no] != -1)
    {
      if(column->oci_type == SQLT_VNU)
      {
        const OCINumber* oci_num = reinterpret_cast<const OCINumber*>(
          column->fetch_buffer.get()) + row_no;

        IntType value;
        sword result;

        if((result = OCINumberToInt(
              error_handle_.get(),
              oci_num,
              sizeof(IntType),
              OciSign,
              &value)) != OCI_SUCCESS)
        {
          Stream::Error err;
          oci_error_text(err, FUN, "OCINumberToInt", result, error_handle_.get());
          err << ", number value = '";
          oci_number_to_string(error_handle_.get(), err, oci_num);
          err << "'";
          throw InvalidValue(err);
        }

        return value;
      }
      else if(column->oci_type == SQLT_STR)
      {
        /* make atoi */
        IntType value;
        std::string str = get_string(ind);
        Stream::Parser istr(str);
        istr >> value;

        if(!istr.eof() || istr.fail())
        {
          Stream::Error err;
          err << FUN << ": can't convert string '" << str
            << "' to number, pos = " << ind << ".";
          throw InvalidValue(err);
        }

        return value;
      }
    }

    throw_type_error(
      FUN, ind, column->oci_type, column->indicators[row_no] == -1);

    return IntType();
  }

  unsigned int
  ResultSet::get_uint(unsigned int ind) const
    /*throw(InvalidValue, SqlException, NotSupported)*/
  {
    return get_long_<unsigned int, OCI_NUMBER_UNSIGNED>(ind);
  }

  int ResultSet::get_int(unsigned int ind) const
    /*throw(InvalidValue, SqlException, NotSupported)*/
  {
    return get_long_<int, OCI_NUMBER_SIGNED>(ind);
  }

  uint64_t ResultSet::get_uint64(unsigned int ind) const
    /*throw(InvalidValue, SqlException, NotSupported)*/
  {
    return get_long_<uint64_t, OCI_NUMBER_UNSIGNED>(ind);
  }

  int64_t ResultSet::get_int64(unsigned int ind) const
    /*throw(InvalidValue, SqlException, NotSupported)*/
  {
    return get_long_<int64_t, OCI_NUMBER_SIGNED>(ind);
  }

  double
  ResultSet::get_double(unsigned int ind) const
    /*throw(SqlException, NotSupported)*/
  {
    static const char* FUN = "ResultSet::get_double()";

    check_column_index_(ind);

    ub2 row_no = static_cast<ub2>((current_row_ - 1) % fetch_count_);
    const Column* column = columns_[ind - 1];

    if(column->indicators[row_no] != -1 &&
       column->oci_type == SQLT_VNU)
    {
      const OCINumber* oci_num = reinterpret_cast<const OCINumber*>(
        column->fetch_buffer.get()) + row_no;
      double value;
      sword result;

      if((result = OCINumberToReal(
            error_handle_.get(),
            oci_num,
            sizeof(double),
            &value)) != OCI_SUCCESS)
      {
        throw_oci_error(FUN, "OCINumberToReal", result, error_handle_.get());
      }

      return value;
    }

    throw_type_error(
      FUN, ind, column->oci_type, column->indicators[row_no] == -1);

    return 0;
  }

  std::string
  ResultSet::get_number_as_string(unsigned int ind) const
    /*throw(Overflow, Exception, SqlException)*/
  {
    static const char* FUN = "ResultSet::get_number_as_string()";

    check_column_index_(ind);

    ub2 row_no = static_cast<ub2>((current_row_ - 1) % fetch_count_);
    const Column* column = columns_[ind - 1];
    if(column->indicators[row_no] != -1 &&
       column->oci_type == SQLT_VNU)
    {
      const OCINumber* oci_num = reinterpret_cast<const OCINumber*>(
        column->fetch_buffer.get()) + row_no;

      ub4 buf_size = MAX_GET_NUMBER_SIZE;
      char buf[MAX_GET_NUMBER_SIZE];

      sword result;

      if((result = OCINumberToText(
            error_handle_.get(),
            oci_num,
            reinterpret_cast<const text*>(GET_NUMBER_FORMAT),
            sizeof(GET_NUMBER_FORMAT) - 1,
            0,
            0,
            &buf_size,
            reinterpret_cast<text*>(buf))) != OCI_SUCCESS)
      {
        Stream::Error ostr;
        oci_error_text(ostr, FUN, "OCINumberToText", result, error_handle_.get());
        throw Overflow(ostr);
      }

      return String::StringManip::trim_ret(
        String::SubString(buf, buf_size)).str();
    }

    throw_type_error(
      FUN, ind, column->oci_type, column->indicators[row_no] == -1);

    return "";
  }

  std::string
  ResultSet::get_string(unsigned int ind) const
    /*throw(SqlException, NotSupported)*/
  {
    static const char* FUN = "ResultSet::get_string()";

    check_column_index_(ind);

    ub2 row_no = static_cast<ub2>((current_row_ - 1) % fetch_count_);

    const Column* column = columns_[ind - 1];

    if(column->indicators[row_no] == -1)
    {
      /* specific for string null is equal empty string  */
      return "";
    }

    if(column->oci_type == SQLT_STR)
    {
      const char* oci_str = reinterpret_cast<const char*>(
        column->fetch_buffer.get()) + column->size * row_no;

      return oci_str;
    }
    else if(column->oci_type == SQLT_VNU)
    {
      return get_number_as_string(ind);
    }

    throw_type_error(
      FUN, ind, column->oci_type, column->indicators[row_no] == -1);

    return "";
  }

  char ResultSet::get_char(unsigned int ind) const
    /*throw(SqlException, InvalidValue)*/
  {
    static const char* FUN = "ResultSet::get_char()";

    std::string res = get_string(ind);
    if(res.size() != 1)
    {
      Stream::Error ostr;
      ostr << FUN << ": received string with length != 1";
      throw InvalidValue(ostr);
    }

    return res[0];
  }

  Lob ResultSet::get_blob(unsigned int ind) const
    /*throw(TimedOut, Exception, SqlException, NotSupported)*/
  {
    static const char* FUN = "ResultSet::get_blob()";

    check_column_index_(ind);

    sword result;

    ub2 row_no = static_cast<ub2>((current_row_ - 1) % fetch_count_);

    Column* column = columns_[ind - 1];
    if(column->indicators[row_no] != -1 &&
       column->oci_type == SQLT_BLOB)
    {
      OCILobLocator* oci_lob =
        const_cast<OCILobLocator*>(
          *(reinterpret_cast<const OCILobLocator**>(
            column->fetch_buffer.get()) + row_no));

      assert(oci_lob);

      ub4 lob_size;

      {
        TimeoutControl timer(statement_->use_timeout_(0));

        while((result = OCILobGetLength(
          statement_->connection_->svc_context_handle_.get(),
          error_handle_.get(),
          oci_lob,
          &lob_size)) == OCI_STILL_EXECUTING)
        {
          if(!timer.sleep_step())
          {
            statement_->handle_timeout_();

            throw_timeout_error(FUN,
              "OCILobGetLength", timer.passed_time(), timer.timeout());
          }
        }

        if(result != OCI_SUCCESS)
        {
          throw_oci_error(FUN, "OCILobGetLength", result, error_handle_.get());
        }
      }

      Generics::ArrayAutoPtr<char> lob_buf(lob_size);

      ub4 amtp = lob_size;

      TimeoutControl timer(statement_->use_timeout_(0));

      if(lob_size)
      {
        while((result = OCILobRead(
          statement_->connection_->svc_context_handle_.get(),
          error_handle_.get(),
          oci_lob,
          &amtp,
          1, // offset
          lob_buf.get(),
          lob_size,
          0,
          0,
          0,
          SQLCS_IMPLICIT)) == OCI_STILL_EXECUTING)
        {
          if(!timer.sleep_step())
          {
            statement_->handle_timeout_();

            throw_timeout_error(FUN,
              "OCILobRead", timer.passed_time(), timer.timeout());
          }
        }
      }

      if(result != OCI_SUCCESS)
      {
        throw_oci_error(FUN, "OCILobRead", result, error_handle_.get());
      }

      return Lob(lob_buf.release(), lob_size, true);
    }

    throw_type_error(
      FUN, ind, column->oci_type, column->indicators[row_no] == -1);

    return Lob("", 0, false);
  }

  void ResultSet::check_column_index_(unsigned long ind) const
    /*throw(SqlException)*/
  {
    if(ind == 0)
    {
      Stream::Error ostr;
      ostr << "Column index = 0 can't be used, first index is 1";
      throw SqlException(ostr);
    }

    if(ind > columns_.size())
    {
      Stream::Error ostr;
      ostr << "Incorrect column index: " << ind << ", max allowed: " <<
        columns_.size();
      throw SqlException(ostr);
    }
  }
} /*Oracle*/
} /*Commons*/
} /*AdServer*/
