#include <Sync/PosixLock.hpp>
#include <Generics/ArrayAutoPtr.hpp>
#include <String/StringManip.hpp>

#include "TimeoutControl.hpp"
#include "ResultSet.hpp"
#include "Statement.hpp"
#include "Connection.hpp"
#include "Environment.hpp"

#ifdef _USE_OCCI
# include "ResultSetImpl.cpp"
#else
# include "ResultSetDummyImpl.cpp"
#endif

namespace AdServer {
namespace Commons {
namespace Oracle
{
  ResultSet::~ResultSet() noexcept
  {}
  
  void ResultSet::check_terminated_(const char* fun) const
    /*throw(NotSupported)*/
  {
    if(statement_->is_terminated_())
    {
      Stream::Error error;
      error << fun << CONN_TERMINATED_ERROR;
      throw NotSupported(error);
    }
  }

  ResultSet::Column::~Column() noexcept
  {
    clear_fetch_cells_();
  }

  Generics::Time ResultSet::get_date(unsigned int ind) const
    /*throw(Exception, SqlException, NotSupported)*/
  {
    return get_timestamp(ind);
  }
}
}
}
