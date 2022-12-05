#include "Environment.hpp"

namespace AdServer
{
namespace Commons
{
namespace Oracle
{
  // AllocController_
  AllocController_* AllocController_::instance_ = new AllocController_();

#ifdef _USE_OCCI

  dvoid* count_malloc(dvoid *, size_t size)
  {
    dvoid* ret = ::malloc(size);
    AllocController::instance().add(ret, size);
    return ret;
  }

  dvoid* count_realloc(dvoid *, dvoid* ptr, size_t size)
  {
    dvoid* ret = ::realloc(ptr, size);
    AllocController::instance().remove(ptr);
    AllocController::instance().add(ret, size);
    return ret;
  }

  void count_free(dvoid*, dvoid *ptr)
  {
    AllocController::instance().remove(ptr);
    ::free(ptr);
  }

  // Environment
  Environment_var
  Environment::create_environment(
    EnvironmentMode /*mode*/,
    const Generics::Time* timeout,
    bool debug_memory)
    /*throw(SqlException, NotSupported)*/
  {
    return new Environment(0, timeout, debug_memory);
  }

  Environment::Environment(
    unsigned int /*mode*/,
    const Generics::Time* timeout,
    bool debug_memory)
    /*throw(SqlException)*/
    : timeout_(timeout ? *timeout : Generics::Time::ZERO)
  {
    static const char* FUN = "Oracle::Environment::Environment()";

    sword result;

    // allocate an environment handle
    if((result = OCIEnvCreate(
          &environment_handle_.fill(),
          OCI_OBJECT | OCI_THREADED,
          0, // context
          debug_memory ? count_malloc : 0, // malloc
          debug_memory ? count_realloc : 0, // realloc
          debug_memory ? count_free : 0, // free
          0, // extra memory to allocate
          0) // pointer to user-memory
        ) != OCI_SUCCESS)
    {
      throw_oci_error(FUN, "OCIEnvCreate", result);
    }
  }

  SwitchableEnvironment_var
  SwitchableEnvironment::create_environment(
    EnvironmentMode mode,
    const Generics::Time* timeout ,
    bool debug_memory)
    /*throw(SqlException, NotSupported)*/
  {
    return new SwitchableEnvironment(mode, timeout, debug_memory);
  }

#else // !defined _USE_OCCI

  Environment_var
  Environment::create_environment(
    EnvironmentMode /*mode*/,
    const Generics::Time* /*timeout*/,
    bool /*debug_memory*/)
    /*throw(SqlException, NotSupported)*/
  {
    throw NotSupported(ORACLE_NOT_SUPPORTED);
  }

  Environment::Environment(
    unsigned int /*mode*/,
    const Generics::Time* /*timeout*/,
    bool /*debug_memory*/)
    /*throw(SqlException)*/
  {}

  SwitchableEnvironment_var
  SwitchableEnvironment::create_environment(
    EnvironmentMode /*mode*/,
    const Generics::Time* /*timeout*/,
    bool /*debug_memory*/)
    /*throw(SqlException, NotSupported)*/
  {
    throw NotSupported(ORACLE_NOT_SUPPORTED);
  }

#endif

  Environment::~Environment() noexcept
  {}

  Connection_var
  Environment::create_connection(
    const ConnectionDescription& conn)
    /*throw(SqlException, NonActive, ConnectionError)*/
  {
    return new Connection(this, 0, conn);
  }

  ConnectionPool_var
  Environment::create_connection_pool(
    const ConnectionDescription& conn,
    int max_conn)
    /*throw(SqlException)*/
  {
    return new StandartConnectionPool(
      this,
      conn,
      max_conn);
  }

  void
  Environment::own_connection(Connection*)
    /*throw(NonActive)*/
  {}

  bool
  Environment::destroy_connection(Connection*) noexcept
  {
    return true;
  }

  void
  Environment::connection_destroyed() noexcept
  {}

  ConnectionOwner_var
  Environment::connection_owner_() noexcept
  {
    return ReferenceCounting::add_ref(static_cast<ConnectionOwner*>(this));
  }

  //
  // class SwitchableEnvironment
  //
  SwitchableEnvironment::SwitchableEnvironment(
    unsigned int mode,
    const Generics::Time* timeout,
    bool debug_memory)
    /*throw(SqlException)*/
    : Environment(mode, timeout, debug_memory),
      count_(new CountActiveObject),
      children_(new Generics::CompositeSetActiveObject)
  {
    add_child_object(count_);
    add_child_object(children_);
  }

  Connection_var
  SwitchableEnvironment::create_connection(
    const ConnectionDescription& conn)
    /*throw(SqlException, NonActive, ConnectionError)*/
  {
    static const char* FUN = "SwitchableEnvironment::create_connection()";

    if (active() && count_->add_active_count(1))
    {
      try
      {
        return new Connection(this, this, conn);
      }
      catch (...)
      {
        count_->add_active_count(-1);
        throw;
      }
    }

    Stream::Error error;
    error << FUN << ": object is not active";
    throw NonActive(error);
  }

  ConnectionPool_var
  SwitchableEnvironment::create_connection_pool(
    const ConnectionDescription& conn,
    int max_conn)
    /*throw(SqlException)*/
  {
    SwitchableConnectionPool_var res(
      new SwitchableConnectionPool(
        this,
        children_,
        conn,
        max_conn));

    children_->add_child_object(res);

    return res;
  }

  void
  SwitchableEnvironment::own_connection(Connection*)
    /*throw(NonActive)*/
  {
    // assert: NonActive can't be thrown here by design
    // As long as any connection calls own_connection there is at least one
    // not deactivated pool, so count_->wait_object is not called yet (but
    // count_->deactivate_object may be called).
    count_->add_active_count(1, true);
  }

  bool
  SwitchableEnvironment::destroy_connection(Connection*)
    noexcept
  {
    return true;
  }

  void
  SwitchableEnvironment::connection_destroyed()
    noexcept
  {
    count_->add_active_count(-1);
  }
}
}
}
