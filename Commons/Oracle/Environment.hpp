#ifndef ORACLE_ENVIRONMENT_HPP
#define ORACLE_ENVIRONMENT_HPP

#include <set>
#include <eh/Exception.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/Singleton.hpp>
#include <Sync/SyncPolicy.hpp>
#include <Commons/CountActiveObject.hpp>

#include "OraException.hpp"
#include "InnerEntities.hpp"
#include "Connection.hpp"
#include "ConnectionPool.hpp"

namespace AdServer
{
namespace Commons
{
  namespace Oracle
  {
    class Connection;

    typedef ReferenceCounting::SmartPtr<Connection> Connection_var;

    struct AllocController_
    {
      struct AllocDescr
      {
        AllocDescr(size_t size_val, unsigned long num_val);

        size_t size;
        unsigned long num;
      };

      typedef std::map<const void*, AllocDescr> AllocMap;

      AllocController_();

      void add(const void* ptr, size_t size);

      void remove(const void* ptr);

      size_t sum() const;

      void print(std::ostream& out) const;

      static AllocController_& instance();

      static AllocController_* instance_;

      mutable Sync::PosixMutex mutex_;
      AllocMap allocated;
      unsigned long num;
    };

    typedef AllocController_ AllocController;

    //
    // class Environment
    //
    class Environment:
      public virtual ReferenceCounting::AtomicImpl,
      protected ConnectionOwner
    {
    public:
      enum EnvironmentMode
      {
        EM_DEFAULT = 0x01,
        EM_OBJECT = 0x02,
        EM_SHARED = 0x04,
        EM_NO_USERCALLBACKS = 0x08,
        EM_THREADED_MUTEXED = 0x10,
        EM_THREADED_UNMUTEXED = 0x20,
        EM_EVENTS = 0x40,
        EM_USE_LDAP = 0x80
      };

      static ReferenceCounting::SmartPtr<Environment>
      create_environment(
        EnvironmentMode mode = EM_DEFAULT,
        const Generics::Time* timeout = 0,
        bool debug_memory = false)
        /*throw(SqlException, NotSupported)*/;

      virtual Connection_var
      create_connection(
        const ConnectionDescription& conn)
        /*throw(SqlException, NonActive, ConnectionError)*/;

      virtual ConnectionPool_var
      create_connection_pool(
        const ConnectionDescription& conn,
        int max_conn = 0 // no limits
        )
        /*throw(SqlException)*/;

    protected:
      friend class PooledConnection;
      friend class StandartConnectionPool;
      friend class SwitchableConnectionPool;
      friend class Connection;
      friend class Statement;
      friend class ResultSet;
      friend class SqlStream;
      friend class ParamArrayHolder;
      friend class ParamStreamHolder;
      friend class OCIObjectFillCache;

      Environment(
        unsigned int mode,
        const Generics::Time* timeout,
        bool debug_memory = false)
        /*throw(SqlException)*/;

      virtual ~Environment() noexcept;

      virtual ConnectionOwner_var connection_owner_() noexcept;

      // ConnectionOwner
      virtual void
      own_connection(Connection*) /*throw(NonActive)*/;

      virtual bool
      destroy_connection(Connection*) noexcept;

      virtual void
      connection_destroyed() noexcept;

    private:
      const Generics::Time timeout_;
      Sync::Policy::PosixThread::Mutex oci_session_begin_lock_;
      Sync::Policy::PosixThread::Mutex create_object_lock_;
#ifdef _USE_OCCI
      OCIHandlePtr<OCIEnv, OCI_HTYPE_ENV> environment_handle_;
#endif
    };

    typedef ReferenceCounting::SmartPtr<Environment>
      Environment_var;

    //
    // class SwitchableEnvironment
    //
    class SwitchableEnvironment:
      public Environment,
      public Generics::CompositeActiveObject
    {
    public:
      // Resolve ambiguity between Environment and SimpleActiveObject.
      static ReferenceCounting::SmartPtr<SwitchableEnvironment>
      create_environment(
        EnvironmentMode mode = EM_DEFAULT,
        const Generics::Time* timeout = 0,
        bool debug_memory = false)
        /*throw(SqlException, NotSupported)*/;

      virtual Connection_var
      create_connection(
        const ConnectionDescription& conn)
        /*throw(SqlException, NonActive, ConnectionError)*/;

      virtual ConnectionPool_var
      create_connection_pool(
        const ConnectionDescription& conn,
        int max_conn = 0 // no limits
        )
        /*throw(SqlException)*/;

    protected:
      SwitchableEnvironment(
        unsigned int mode,
        const Generics::Time* timeout = 0,
        bool debug_memory = false)
        /*throw(SqlException)*/;

      virtual ~SwitchableEnvironment() noexcept
      {}

      virtual void
      own_connection(Connection* conn) /*throw(NonActive)*/;

      virtual bool
      destroy_connection(Connection* conn) noexcept;

      virtual void
      connection_destroyed() noexcept;

    private:
      CountActiveObject_var count_;
      Generics::CompositeSetActiveObject_var children_;
    };

    typedef ReferenceCounting::SmartPtr<SwitchableEnvironment>
      SwitchableEnvironment_var;
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
  // AllocController_
  //
  inline
  AllocController_::AllocDescr::AllocDescr(
    size_t size_val, unsigned long num_val)
    : size(size_val), num(num_val)
  {}

  inline
  AllocController_::AllocController_(): num(0)
  {}

  inline
  void
  AllocController_::add(const void* ptr, size_t size)
  {
    Sync::PosixGuard lock(mutex_);
//  assert(num != 10);
    allocated.insert(std::make_pair(ptr, AllocDescr(size, num++)));
  }

  inline
  void
  AllocController_::remove(const void* ptr)
  {
    Sync::PosixGuard lock(mutex_);
    AllocMap::iterator it = allocated.find(ptr);
    assert(it != allocated.end());
    allocated.erase(it);
  }

  inline
  size_t
  AllocController_::sum() const
  {
    size_t ret = 0;
    Sync::PosixGuard lock(mutex_);
    for(AllocMap::const_iterator it = allocated.begin();
        it != allocated.end(); ++it)
    {
      ret += it->second.size;
    }
    return ret;
  }

  inline
  void
  AllocController_::print(std::ostream& out) const
  {
    typedef std::map<unsigned long, AllocDescr> NumOrderedAllocMap;

    out << "Sum: " << sum() << std::endl;

    if(allocated.empty())
    {
      out << "  no blocks" << std::endl;
    }

    NumOrderedAllocMap res;
    for(AllocMap::const_iterator it = allocated.begin();
        it != allocated.end(); ++it)
    {
      res.insert(std::make_pair(it->second.num, it->second));
    }

    for(NumOrderedAllocMap::const_iterator it = res.begin();
        it != res.end(); ++it)
    {
      out << "  #" << it->first << " : " << it->second.size << std::endl;
    }
  }

  inline
  AllocController_&
  AllocController_::instance()
  {
    return *instance_;
  }
}
}
}

#endif /*ORACLE_ENVIRONMENT_HPP*/
