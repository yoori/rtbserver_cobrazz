#ifndef POSTGRES_ENVIRONMENT_HPP
#define POSTGRES_ENVIRONMENT_HPP

#include<string>
#include<Generics/CompositeActiveObject.hpp>
#include<Commons/Postgres/Connection.hpp>
#include<Commons/CountActiveObject.hpp>
#include<Sync/SyncPolicy.hpp>

namespace AdServer
{
namespace Commons
{
  namespace Postgres
  {
    class ConnectionPool;

    typedef ReferenceCounting::SmartPtr<ConnectionPool>
      ConnectionPool_var;

    class Environment:
      public Generics::CompositeActiveObject,
      public virtual ReferenceCounting::AtomicImpl,
      protected ConnectionOwner
    {
    public:
      explicit
      Environment(const char* conninfo) noexcept;

      Connection_var
      create_connection(ConnectionOwner* owner = 0)
        /*throw(ConnectionError, eh::Exception)*/;

      ConnectionPool_var
      create_connection_pool(unsigned long max_conn = 0)
        /*throw(ConnectionError)*/;

      virtual bool
      destroy_connection(Connection*) noexcept;

      virtual void
      connection_destroyed() noexcept;

      friend class ConnectionPool;
    protected:
      virtual
      ~Environment() noexcept = default;

    private:
      std::string conn_info_;

      CountActiveObject_var count_;
      Generics::CompositeSetActiveObject_var children_;
    };

    typedef ReferenceCounting::SmartPtr<Environment> Environment_var;
  }
}
}
#endif //POSTGRES_ENVIRONMENT_HPP
