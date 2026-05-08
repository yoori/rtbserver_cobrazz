#pragma once

#include<eh/Exception.hpp>

namespace AdServer
{
namespace Commons
{
  namespace Postgres
  {
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    DECLARE_EXCEPTION(ConnectionError, Exception);
    DECLARE_EXCEPTION(SqlException, Exception);
  }
}
}
