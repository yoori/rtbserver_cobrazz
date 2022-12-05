#include <Sync/PosixLock.hpp>
#include <Generics/ArrayAutoPtr.hpp>

#include "Statement.hpp"
#include "Connection.hpp"
#include "Environment.hpp"
#include "TimeoutControl.hpp"


#ifdef _USE_OCCI
#  include "StatementImpl.cpp"
#else // !defined _USE_OCCI
#  include "StatementDummyImpl.cpp"
#endif

namespace AdServer {
namespace Commons {
namespace Oracle
{
  /** Statement */
  Statement::~Statement() noexcept
  {}

  void
  Statement::execute(const Generics::Time* timeout)
    /*throw(SqlException, NotSupported, TimedOut)*/
  {
    execute_(timeout);
  }

  ResultSet_var
  Statement::execute_query(
    const Generics::Time* timeout,
    unsigned long fetch_size)
    /*throw(Exception, SqlException, TimedOut, NotSupported)*/
  {
    return execute_query_(timeout, fetch_size);
  }

  bool
  Statement::is_terminated_() noexcept
  {
    return connection_->is_terminated_();
  }

  void Statement::check_terminated_(const char* fun)
    /*throw(NotSupported)*/
  {
    if(connection_->is_terminated_())
    {
      Stream::Error error;
      error << fun << CONN_TERMINATED_ERROR;
      throw NotSupported(error);
    }
  }

  const Generics::Time* Statement::use_timeout_(
    const Generics::Time* timeout) const
    noexcept
  {
    if(timeout)
    {
      return timeout;
    }

    if(connection_->timeout_ != Generics::Time::ZERO)
    {
      return &connection_->timeout_;
    }

    if(connection_->environment_->timeout_ !=
       Generics::Time::ZERO)
    {
      return &connection_->environment_->timeout_;
    }

    return 0;
  }
  
  /** SqlStream */
  void SqlStream::set_int(int val) /*throw(SqlException)*/
  {
    set_long(val);
  }

  void SqlStream::set_uint(unsigned int val) /*throw(SqlException)*/
  {
    set_ulong(val);
  }
} /*Oracle*/
} /*Commons*/
} /*AdServer*/

