
#include "DbConnectionFactory.hpp"

namespace AdServer {
namespace LogProcessing {

void
PostgresConnectionFactoryImpl::initialize(const char* conn)
{
  if (initialized_)
  {
    return;
  }

  if (!env_.in() || !conn_pool_.in())
  {
    env_ = new Commons::Postgres::Environment(conn);
    conn_pool_ = env_->create_connection_pool();
  }
  initialized_ = true;
}

Commons::Postgres::Connection_var
PostgresConnectionFactoryImpl::get_connection()
{
  if (!initialized_ || !conn_pool_.in())
  {
    throw DbConnectionFactory::Exception(
            "AdServer::LogProcessing::PostgresConnectionFactoryImpl::"
            "get_connection(): Not initialized"
          );
  }

  Commons::Postgres::Connection_var conn;
  try
  {
    conn = conn_pool_->get_connection();
  }
  catch (const Commons::Postgres::NotActive&)
  {
    throw NotActive(""); // FIXME
  }
  return conn;
}

void PostgresConnectionFactoryImpl::activate_object()
  /*throw(ActiveObject::AlreadyActive)*/
{
  if (conn_pool_.in())
  {
    conn_pool_->activate_object();
  }
}

void PostgresConnectionFactoryImpl::deactivate_object()
  noexcept
{
  if (conn_pool_.in())
  {
    conn_pool_->deactivate_object();
  }
}

void PostgresConnectionFactoryImpl::wait_object()
  noexcept
{
  if (conn_pool_.in())
  {
    conn_pool_->wait_object();
  }
}

bool PostgresConnectionFactoryImpl::active()
  noexcept
{
  return conn_pool_.in() ? conn_pool_->active() : false;
}

} // namespace LogProcessing
} // namespace AdServer

