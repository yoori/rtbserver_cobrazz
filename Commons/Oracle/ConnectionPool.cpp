#include "Environment.hpp"
#include "ConnectionPool.hpp"

namespace AdServer
{
namespace Commons
{
namespace Oracle
{
  //
  // PooledConnection
  //
  PooledConnection::PooledConnection(
    Environment* environment,
    ConnectionOwner* connection_owner,
    const ConnectionDescription& conn)
    /*throw(SqlException, ConnectionError)*/
    : Connection(environment, connection_owner, conn)
  {}

  void
  PooledConnection::unpool() noexcept
  {
    environment_->connection_owner_()->own_connection(this);
    owner_(environment_->connection_owner_());
  }

  //
  // ConnectionPool
  //
  ConnectionPool::ConnectionPool(
    Environment* environment,
    const ConnectionDescription& conn,
    unsigned long max_connections)
    /*throw(SqlException, ConnectionError)*/
    : environment_(ReferenceCounting::add_ref(environment)),
      conn_descr_(conn),
      max_connections_(max_connections)
  {
    if(max_connections_ > 0)
    {
      free_connections_.reserve(max_connections_);
    }
  }

  Connection_var
  ConnectionPool::get_connection(bool wait_free_connection)
    /*throw(MaxConnectionsReached, NonActive)*/
  {
    // connection destroy lock free_connections_lock_
    Connection_var result_connection;
    bool create_connection = false;

    {
      Sync::ConditionalGuard guard(free_connection_cond_);

      while((result_connection = check_connection_i_(
          create_connection,
          wait_free_connection)).in() == 0 &&
        !create_connection)
      {
        guard.wait();
      }
    }

    if(create_connection)
    {
      Sync::PosixGuard create_connection_guard(create_lock_);

      try
      {
        result_connection = create_connection_i_();
      }
      catch(...)
      {
        create_connection_canceled_i_();
        throw;
      }
    }

    return result_connection;
  }

  Connection_var
  ConnectionPool::check_connection_i_(
    bool& create_connection,
    bool wait_free_connection)
    /*throw(MaxConnectionsReached, NonActive)*/
  {
    create_connection = false;

    if(!free_connections_.empty())
    {
      Connection_var result_connection = free_connections_.back();
      free_connections_.pop_back();
      return result_connection;
    }

    if(max_connections_ == 0 ||
       open_connections_i_() < max_connections_)
    {
      create_connection = true;
    }
    else if(!wait_free_connection)
    {
      throw MaxConnectionsReached("");
    }

    return Connection_var();
  }

  bool
  ConnectionPool::add_free_connection_(Connection* conn) noexcept
  {
    Sync::PosixGuard guard(free_connection_cond_);

    if(open_connections_i_() < max_connections() &&
       add_free_connection_i_(conn))
    {
      free_connection_cond_.broadcast();
      return true;
    }

    return false;
  }

  bool
  ConnectionPool::add_free_connection_i_(Connection* conn)
    noexcept
  {
    free_connections_.push_back(ReferenceCounting::add_ref(conn));
    return true;
  }

  //
  // StandartConnectionPool
  //
  StandartConnectionPool::StandartConnectionPool(
    Environment* environment,
    const ConnectionDescription& conn,
    unsigned long max_connections = 0)
    /*throw(SqlException, ConnectionError)*/
    : ConnectionPool(
        environment,
        conn,
        max_connections),
      busy_connections_(0)
  {}

  void
  StandartConnectionPool::own_connection(Connection* conn)
    /*throw(NonActive)*/
  {
    conn->owner_(environment_->connection_owner_());
    add_free_connection_(conn);
  }

  bool
  StandartConnectionPool::destroy_connection(Connection* conn) noexcept
  {
    return !add_free_connection_(conn);
  }

  void
  StandartConnectionPool::connection_destroyed() noexcept
  {
    busy_connections_ += -1;
  }

  Connection_var
  StandartConnectionPool::check_connection_i_(
    bool& create_connection,
    bool wait_free_connection)
    /*throw(MaxConnectionsReached, NonActive)*/
  {
    Connection_var result_connection =
      ConnectionPool::check_connection_i_(
        create_connection, wait_free_connection);

    if(create_connection)
    {
      busy_connections_ += 1;
    }

    return result_connection;
  }

  Connection_var
  StandartConnectionPool::create_connection_i_()
    /*throw(ConnectionError, NonActive)*/
  {
    return new PooledConnection(
      environment_,
      environment_->connection_owner_(),
      conn_descr_);
  }

  void
  StandartConnectionPool::create_connection_canceled_i_()
    noexcept
  {
    busy_connections_ += -1;
  }

  //
  // SwitchableConnectionPool
  //
  SwitchableConnectionPool::SwitchableConnectionPool(
    Environment* environment,
    Generics::ActiveObjectChildRemover* child_remover,
    const ConnectionDescription& conn_descr,
    unsigned long max_connections)
    /*throw(SqlException)*/
    : ConnectionPool(environment, conn_descr, max_connections),
      Generics::RemovableActiveObject(child_remover)
  {}

  SwitchableConnectionPool::~SwitchableConnectionPool() noexcept
  {}

  void
  SwitchableConnectionPool::deactivate_object() noexcept
  {
    ConnectionList free_connections;

    {
      // add_active_count(1) can be called only inside free_connections_lock_ and
      // after active check
      // call of deactivate_object inside lock guarantees that
      // active count will not be increased after
      Sync::PosixGuard guard(free_connection_cond_);
      CountActiveObject::deactivate_object();
      free_connections_.swap(free_connections);
    }

    for(ConnectionList::iterator it = free_connections.begin();
        it != free_connections.end(); ++it)
    {
      (*it)->owner_(0);
    }

    // wakeup free connection waiters
    free_connection_cond_.broadcast();
    add_active_count(-free_connections.size());
  }

  unsigned long
  SwitchableConnectionPool::busy_connections() const noexcept
  {
    return active_count();
  }

  void
  SwitchableConnectionPool::own_connection(Connection* conn)
    /*throw(NonActive)*/
  {
    // ???
    conn->owner_(environment_);
    add_free_connection_(conn);
  }

  bool
  SwitchableConnectionPool::destroy_connection(Connection* conn)
    noexcept
  {
    return !add_free_connection_(conn);
  }

  void
  SwitchableConnectionPool::connection_destroyed() noexcept
  {
    add_active_count(-1);
  }

  bool
  SwitchableConnectionPool::add_free_connection_i_(
    Connection* conn) noexcept
  {
    // destroy connection (don't push to free connections) if
    // pool deactivated (wait connection destroy)
    if(active())
    {
      conn->owner_(0);
      ConnectionPool::add_free_connection_i_(conn);
      return true;
    }

    return false;
  }

  Connection_var
  SwitchableConnectionPool::check_connection_i_(
    bool& create_connection,
    bool wait_free_connection)
    /*throw(MaxConnectionsReached, NonActive)*/
  {
    if(!active())
    {
      throw NonActive("");
    }

    Connection_var result_connection =
      ConnectionPool::check_connection_i_(
        create_connection,
        wait_free_connection);

    if(create_connection)
    {
      // deactivation can't finished until new connection not released or cancelled
      add_active_count(1);
    }

    return result_connection;
  }

  Connection_var
  SwitchableConnectionPool::create_connection_i_()
    /*throw(ConnectionError, NonActive)*/
  {
    if(active())
    {
      return new PooledConnection(
        environment_,
        this,
        conn_descr_);
    }
    else
    {
      throw NonActive("");
    }
  }

  void
  SwitchableConnectionPool::create_connection_canceled_i_() noexcept
  {
    add_active_count(-1);
  }

  void
  SwitchableConnectionPool::before_remove_child_()
    noexcept
  {
    free_connections_.clear(); // unpooled connections
  }
}
}
}
