#ifndef ORACLE_CONNECTIONPOOL_HPP
#define ORACLE_CONNECTIONPOOL_HPP

#include <vector>
#include <Generics/CompositeActiveObject.hpp>
#include <Commons/CountActiveObject.hpp>

#include "OraException.hpp"
#include "Connection.hpp"

namespace AdServer
{
namespace Commons
{
namespace Oracle
{
  class ConnectionPool;

  //
  // PooledConnection
  //   extend Connection with ability to unlink it from ConnectionPool
  //
  class PooledConnection: public Connection
  {
  public:
    // remove connection from pool
    void unpool() noexcept;

  protected:
    friend class StandartConnectionPool;
    friend class SwitchableConnectionPool;

    PooledConnection(
      Environment* environment,
      ConnectionOwner* owner,
      const ConnectionDescription& conn)
      /*throw(SqlException, ConnectionError)*/;

    virtual ~PooledConnection() noexcept {};
  };

  typedef ReferenceCounting::SmartPtr<PooledConnection>
    PooledConnection_var;

  //
  // ConnectionPool
  //
  class ConnectionPool:
    public virtual ReferenceCounting::AtomicImpl,
    protected ConnectionOwner
  {
  public:
    DECLARE_EXCEPTION(MaxConnectionsReached, eh::DescriptiveException);

    virtual Connection_var
    get_connection(bool wait = true)
      /*throw(MaxConnectionsReached, NonActive)*/;

    virtual unsigned long busy_connections() const noexcept = 0;

    unsigned long open_connections() const noexcept;

    unsigned long max_connections() const noexcept;

  protected:
    ConnectionPool(
      Environment* environment,
      const ConnectionDescription& conn,
      unsigned long max_connections = 0)
      /*throw(SqlException, ConnectionError)*/;

    virtual ~ConnectionPool() noexcept;

    unsigned long open_connections_i_() const noexcept;

    bool add_free_connection_(Connection* conn) noexcept;

    virtual bool
    add_free_connection_i_(Connection* conn) noexcept;

    // if check_connection_i_ returned create_connection = true
    // will be called one of create_connection_i_,
    // create_connection_canceled_i_ methods
    virtual Connection_var
    check_connection_i_(
      bool& create_connection,
      bool wait_free_connection)
      /*throw(MaxConnectionsReached, NonActive)*/;

    virtual Connection_var
    create_connection_i_()
      /*throw(ConnectionError, NonActive)*/ = 0;

    virtual void
    create_connection_canceled_i_() noexcept = 0;

  protected:
    friend class PooledConnection;

    typedef std::vector<Connection_var> ConnectionList;

  protected:
    const ReferenceCounting::SmartPtr<Environment> environment_;
    const ConnectionDescription conn_descr_;
    const unsigned long max_connections_;

    mutable Sync::Condition free_connection_cond_;
    ConnectionList free_connections_;

    // serialize connections creation
    Sync::PosixMutex create_lock_;
  };

  typedef ReferenceCounting::SmartPtr<ConnectionPool>
    ConnectionPool_var;

  //
  // StandartConnectionPool
  //
  class StandartConnectionPool: public ConnectionPool
  {
  public:
    virtual unsigned long
    busy_connections() const noexcept;

  protected:
    friend class Environment;

  protected:
    StandartConnectionPool(
      Environment* environment,
      const ConnectionDescription& conn,
      unsigned long max_connections)
      /*throw(SqlException, ConnectionError)*/;

    virtual ~StandartConnectionPool() noexcept;

  protected:
    virtual void
    own_connection(Connection* conn) /*throw(NonActive)*/;

    virtual bool
    destroy_connection(Connection* conn) noexcept;

    virtual void
    connection_destroyed() noexcept;

    virtual Connection_var
    check_connection_i_(
      bool& create_connection,
      bool wait_free_connection)
      /*throw(MaxConnectionsReached, NonActive)*/;

    virtual Connection_var
    create_connection_i_()
      /*throw(ConnectionError, NonActive)*/;

    virtual void
    create_connection_canceled_i_() noexcept;

  private:
    Algs::AtomicInt busy_connections_;
  };

  //
  // SwitchableConnectionPool
  //
  class SwitchableConnectionPool:
    public ConnectionPool,
    public CountActiveObject,
    protected Generics::RemovableActiveObject
  {
  public:
    virtual void deactivate_object() noexcept;

    virtual unsigned long busy_connections() const noexcept;

  protected:
    friend class SwitchableEnvironment;

  protected:
    SwitchableConnectionPool(
      Environment* environment,
      Generics::ActiveObjectChildRemover* child_remover,
      const ConnectionDescription& conn,
      unsigned long max_connections = 0)
      /*throw(SqlException)*/;

    virtual ~SwitchableConnectionPool() noexcept;

    // ConnectionOwner
    virtual void
    own_connection(Connection* conn) /*throw(NonActive)*/;

    virtual bool
    destroy_connection(Connection* conn) noexcept;

    virtual void
    connection_destroyed() noexcept;

    virtual bool
    add_free_connection_i_(Connection* conn) noexcept;

    virtual Connection_var
    check_connection_i_(
      bool& create_connection,
      bool wait_free_connection)
      /*throw(MaxConnectionsReached, NonActive)*/;

    virtual Connection_var
    create_connection_i_()
      /*throw(ConnectionError, NonActive)*/;

    void
    create_connection_canceled_i_() noexcept;

    virtual
    void
    before_remove_child_() noexcept;
  };

  typedef ReferenceCounting::SmartPtr<SwitchableConnectionPool>
    SwitchableConnectionPool_var;
}
}
}

namespace AdServer
{
namespace Commons
{
namespace Oracle
{
  //
  // ConnectionPool
  //
  inline
  ConnectionPool::~ConnectionPool() noexcept
  {}

  inline
  unsigned long
  ConnectionPool::open_connections() const noexcept
  {
    Sync::PosixGuard lock(free_connection_cond_);
    return open_connections_i_();
  }

  inline
  unsigned long
  ConnectionPool::max_connections() const noexcept
  {
    return max_connections_;
  }

  inline
  unsigned long
  ConnectionPool::open_connections_i_() const noexcept
  {
    return free_connections_.size() + busy_connections();
  }

  //
  // StandartConnectionPool
  //
  inline
  StandartConnectionPool::~StandartConnectionPool() noexcept
  {}

  inline
  unsigned long
  StandartConnectionPool::busy_connections() const noexcept
  {
    return busy_connections_;
  }
}
}
}

#endif /*ORACLE_CONNECTIONPOOL_HPP*/
