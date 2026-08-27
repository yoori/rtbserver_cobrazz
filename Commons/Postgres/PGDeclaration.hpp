#pragma once

#include<eh/Exception.hpp>

namespace AdServer::Commons::Postgres
{
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
  DECLARE_EXCEPTION(ConnectionError, Exception);
  DECLARE_EXCEPTION(SqlException, Exception);
}
