#ifndef AD_SERVER_LOG_PROCESSING_DB_CONNECTION_FACTORY_HPP
#define AD_SERVER_LOG_PROCESSING_DB_CONNECTION_FACTORY_HPP


#include <string>

#include <eh/Exception.hpp>

#include <Generics/ActiveObject.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>

#include <Commons/Postgres/Environment.hpp>
#include <Commons/Postgres/Connection.hpp>
#include <Commons/Postgres/ConnectionPool.hpp>

namespace AdServer {
namespace LogProcessing {

class DbConnectionFactory: public ReferenceCounting::AtomicImpl
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
  DECLARE_EXCEPTION(NotActive, Exception);

protected:
  virtual ~DbConnectionFactory() noexcept {}
};

typedef ReferenceCounting::SmartPtr<DbConnectionFactory>
  DbConnectionFactory_var;

class PostgresConnectionFactoryImpl:
  public Generics::ActiveObject,
  public DbConnectionFactory
{
public:
  PostgresConnectionFactoryImpl(): initialized_() {}

  void
  initialize(const char* conn);

  Commons::Postgres::Connection_var get_connection();

  void activate_object() /*throw(ActiveObject::AlreadyActive)*/;

  void deactivate_object() noexcept;

  void wait_object() noexcept;

  bool active() noexcept;

private:
  virtual
  ~PostgresConnectionFactoryImpl() noexcept {}

  Commons::Postgres::Environment_var env_;
  Commons::Postgres::ConnectionPool_var conn_pool_;
  bool initialized_;
};

typedef ReferenceCounting::SmartPtr<PostgresConnectionFactoryImpl>
  PostgresConnectionFactoryImpl_var;

} // namespace LogProcessing
} // namespace AdServer

#endif /* AD_SERVER_LOG_PROCESSING_DB_CONNECTION_FACTORY_HPP */

