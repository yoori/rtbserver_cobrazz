#ifndef POSTGRES_CONNECTION_POOL_HPP
#define POSTGRES_CONNECTION_POOL_HPP

#include<deque>
#include<Generics/ActiveObject.hpp>
#include<Sync/PosixLock.hpp>
#include<Commons/CountActiveObject.hpp>
#include<Commons/Postgres/Connection.hpp>
#include<Commons/Postgres/Environment.hpp>
#include<Commons/Postgres/ResultSet.hpp>

namespace AdServer
{
namespace Commons
{
  namespace Postgres
  {
    class ConnectionPool:
      public CountActiveObject,
      protected ConnectionOwner,
      protected Generics::RemovableActiveObject
    {
    public:
      DECLARE_EXCEPTION(MaxConnectionsReached, Postgres::Exception);

      ConnectionPool(
        Environment* env,
        Generics::ActiveObjectChildRemover* child_remover,
        unsigned int max_connections)
        noexcept;

      virtual
      void
      deactivate_object() noexcept;

      Connection_var
      get_connection(bool wait = true)
      /*throw(Postgres::Exception,
             Postgres::NotActive,
             MaxConnectionsReached)*/;

      void bad_connection(Connection* conn) noexcept;

    protected:

      // connection pre destroy hook
      virtual bool
      destroy_connection(Connection* conn) noexcept;

      // connection post destroy hook
      virtual void
      connection_destroyed() noexcept;

      Connection_var create_connection_i_()
        /*throw(ConnectionError, eh::Exception)*/;

      Connection_var get_free_connection_i_()
        /*throw(ConnectionError, eh::Exception)*/;

      virtual
      ~ConnectionPool() noexcept {};
    private:
      typedef std::deque<Connection_var> ConnectionCont;
      Environment* env_;
      const int max_connections_;
      ConnectionCont free_connections_;
      mutable Sync::Condition free_connection_cond_;
    };
  }
}
}
#endif //POSTGRES_CONNECTION_POOL_HPP

