#include <sys/socket.h>
#include <Generics/FDSetSize.hpp>

#include "Connection.hpp"
#include "Environment.hpp"
#include "Statement.hpp"

namespace AdServer {
namespace Commons {
namespace Oracle {

  Connection::Connection(
    Environment* environment,
    ConnectionOwner* owner,
    const ConnectionDescription& conn)
    /*throw(SqlException, ConnectionError)*/
    : environment_(ReferenceCounting::add_ref(environment)),
      connection_owner_(ReferenceCounting::add_ref(owner)),
      timeout_(conn.statement_timeout),
      broken_(false),
      terminated_(0)
  {
#ifdef _USE_OCCI
    static const char* FUN = "Oracle::Connection::Connection()";

    sword result;

    // allocate an error handle
    if((result = OCIHandleAlloc(
          environment_->environment_handle_.get(),
          (void **) &error_handle_.fill(),
          OCI_HTYPE_ERROR,
          0, // extra memory to allocate
          0)) != OCI_SUCCESS)
    {
      throw_oci_error(FUN, "OCIHandleAlloc", result);
    }

    // allocate a server handle
    if((result = OCIHandleAlloc(
          environment_->environment_handle_.get(),
          (void **) &server_handle_.fill(),
          OCI_HTYPE_SERVER,
          0, // extra memory to allocate
          0)) != OCI_SUCCESS)
    {
      throw_oci_error(FUN, "OCIHandleAlloc", result);
    }

    if((result = OCIServerAttach(
          server_handle_.get(),
          error_handle_.get(),
          (text*)conn.db.c_str(),
          conn.db.size(),
          OCI_DEFAULT)) != OCI_SUCCESS)
    {
      throw_oci_error(FUN, "OCIServerAttach", result, error_handle_.get());
    }

    try
    {
      // allocate a service handle
      if((result = OCIHandleAlloc(
            environment_->environment_handle_.get(),
            (void **) &svc_context_handle_.fill(),
            OCI_HTYPE_SVCCTX,
            0, // extra memory to allocate
            0)) != OCI_SUCCESS)
      {
        throw_oci_error(FUN, "OCIHandleAlloc", result, error_handle_.get());
      }

      // set the server attribute in the service context handle
      if((result = OCIAttrSet(
            svc_context_handle_.get(),
            OCI_HTYPE_SVCCTX,
            server_handle_.get(),
            sizeof(OCIServer*),
            OCI_ATTR_SERVER,
            error_handle_.get())) != OCI_SUCCESS)
      {
        throw_oci_error(FUN, "OCIAttrSet", result, error_handle_.get());
      }

      // allocate a user session handle
      if((result = OCIHandleAlloc(
            environment_->environment_handle_.get(),
            (void **)&session_handle_.fill(),
            OCI_HTYPE_SESSION,
            0, // extra memory to allocate
            0)) != OCI_SUCCESS)
      {
        throw_oci_error(FUN, "OCIHandleAlloc", result);
      }

      // set username and password attributes in user session handle
      if((result = OCIAttrSet(
            session_handle_.get(),
            OCI_HTYPE_SESSION,
            (text*)conn.user_name.c_str(),
            conn.user_name.size(),
            OCI_ATTR_USERNAME,
            error_handle_.get())) != OCI_SUCCESS)
      {
        throw_oci_error(FUN, "OCIAttrSet", result, error_handle_.get());
      }

      if((result = OCIAttrSet(
            session_handle_.get(),
            OCI_HTYPE_SESSION,
            (text*)conn.password.c_str(),
            conn.password.size(),
            OCI_ATTR_PASSWORD,
            error_handle_.get())) != OCI_SUCCESS)
      {
        throw_oci_error(FUN, "OCIAttrSet", result, error_handle_.get());
      }

      {
        // start the session
        Sync::Policy::PosixThread::WriteGuard lock(environment->oci_session_begin_lock_);
        // workaround for 'Patch 5893079': NLS GLOS NEEDS TO BE MUTEXED IN KPUSSI AND KPUSCN0

        if((result = OCISessionBegin(
              svc_context_handle_.get(),
              error_handle_.get(),
              session_handle_.get(),
              OCI_CRED_RDBMS,
              OCI_DEFAULT)) != OCI_SUCCESS)
        {
          throw_oci_error(FUN, "OCISessionBegin", result, error_handle_.get());
        }
      }

      try
      {
        // set the user session attribute in the service context handle
        if((result = OCIAttrSet(
              svc_context_handle_.get(),
              OCI_HTYPE_SVCCTX,
              session_handle_.get(),
              sizeof(OCISession*),
              OCI_ATTR_SESSION,
              error_handle_.get())) != OCI_SUCCESS)
        {
          throw_oci_error(FUN, "OCIAttrSet", result, error_handle_.get());
        }

        ub1 attr_value = 1;

        if((result = OCIAttrSet(
              server_handle_.get(),
              OCI_HTYPE_SERVER,
              &attr_value,
              sizeof(attr_value),
              OCI_ATTR_NONBLOCKING_MODE,
              error_handle_.get())) != OCI_SUCCESS)
        {
          throw_oci_error(FUN, "OCIAttrSet", result, error_handle_.get());
        }

        if(!conn.schema.empty())
        {
          // switch default schema
          std::string after_session_sql =
            std::string("alter session set current_schema=") +
            conn.schema;

          try
          {
            execute_(after_session_sql.c_str());
          }
          catch(const eh::Exception& ex)
          {
            Stream::Error ostr;
            ostr << FUN << ": can't switch schema with using '" <<
              after_session_sql << "': " << ex.what();
            throw SqlException(ostr);
          }
        }
      }
      catch(...)
      {
        try
        {
          if((result = OCISessionEnd(
                svc_context_handle_.get(),
                error_handle_.get(),
                session_handle_.get(),
                OCI_DEFAULT)) != OCI_SUCCESS)
          {
            throw_oci_error(FUN, "OCISessionEnd", result, error_handle_.get());
          }
        }
        catch(const eh::Exception& ex)
        {
          std::cerr << ex.what() << std::endl;
        }

        throw;
      }
    }
    catch(...)
    {
      try
      {
        if((result = OCIServerDetach(
              server_handle_.get(),
              error_handle_.get(),
              OCI_DEFAULT)) != OCI_SUCCESS)
        {
          throw_oci_error(FUN, "OCIServerDetach", result, error_handle_.get());
        }
      }
      catch(const eh::Exception& ex)
      {
        std::cerr << ex.what() << std::endl;
      }

      throw;
    }

#endif
  }

  Connection::~Connection() noexcept
  {
#ifdef _USE_OCCI
    static const char* FUN = "Connection::~Connection()";

    sword result;

    free_cache();

    try
    {
      if((result = OCISessionEnd(
            svc_context_handle_.get(),
            error_handle_.get(),
            session_handle_.get(),
            OCI_DEFAULT)) != OCI_SUCCESS && !broken_)
      {
        throw_oci_error(FUN, "OCISessionEnd", result, error_handle_.get());
      }

      if((result = OCIServerDetach(
            server_handle_.get(),
            error_handle_.get(),
            OCI_DEFAULT)) != OCI_SUCCESS)
      {
        throw_oci_error(FUN, "OCIServerDetach", result, error_handle_.get());
      }

      if(broken_)
      {
        OCIReset(server_handle_.get(), error_handle_.get());
        server_handle_.reset(0, true); // skip OCI_ERROR
      }
    }
    catch(const eh::Exception& ex)
    {
      std::cerr << ex.what() << std::endl;
    }
#endif

    if(connection_owner_.in())
    {
      connection_owner_->connection_destroyed();
    }
  }

#ifdef _USE_OCCI
  Statement_var
  Connection::create_statement(const char* query)
    /*throw(Exception, SqlException, ConnectionError, NotSupported)*/
  {
    static const char* FUN = "Oracle::Connection::create_statement()";

    if (terminated_)
    {
      Stream::Error error;
      error << FUN << CONN_TERMINATED_ERROR;
      throw NotSupported(error);
    }

    try
    {
      return new Statement(this, query);
    }
    catch (const Exception&)
    {
      throw;
    }
    catch (const eh::Exception& e)
    {
      Stream::Error error;
      error << FUN << ": eh::Exception caught: " << e.what();
      throw Exception(error);
    }
  }

#else

  Statement_var
  Connection::create_statement(const char* /*query*/)
    /*throw(Exception, SqlException, ConnectionError, NotSupported)*/
  {
    return Statement_var();
  }

#endif

  /*
  void
  Connection::set_stmt_cache_size(unsigned int size)
    /*throw(SqlException, ConnectionError, NotSupported)*/
  {
#ifdef _USE_OCCI
    static const char* FUN = "Connection::set_stmt_cache_size()";

    if (terminated_)
    {
      Stream::Error error;
      error << FUN << CONN_TERMINATED_ERROR;
      throw NotSupported(error);
    }

#endif
  }

  void
  Connection::flush_cache() /*throw(SqlException, ConnectionError, NotSupported)*/
  {
#ifdef _USE_OCCI
    static const char* FUN = "Oracle::Connection::flush_size()";

    if (terminated_)
    {
      Stream::Error error;
      error << FUN << CONN_TERMINATED_ERROR;
      throw NotSupported(error);
    }

#endif
  }
  */

  void
  Connection::commit() /*throw(SqlException, ConnectionError, NotSupported)*/
  {
#ifdef _USE_OCCI
    execute_("commit");
    free_cache();
#endif
  }

  void
  Connection::cancel() /*throw(SqlException, ConnectionError)*/
  {
#ifdef _USE_OCCI
    static const char* FUN = "Connection::cancel()";

    if (terminated_)
    {
      Stream::Error error;
      error << FUN << CONN_TERMINATED_ERROR;
      throw NotSupported(error);
    }

    terminate();

    OCIHandlePtr<OCIError, OCI_HTYPE_ERROR> local_error_handle;

    int result;

    if((result = OCIHandleAlloc(
         (dvoid*)environment_->environment_handle_.get(),
         (dvoid**)&local_error_handle.fill(),
         (ub4)OCI_HTYPE_ERROR,
         (size_t)0,
         (dvoid**)0)) != OCI_SUCCESS)
    {
      throw_oci_error(
        FUN, "OCIHandleAlloc", result);
    }

    // OCIBreak: legal way to break connection from documentation,
    //   don't work, but call it because we don't know what internal values, flags, ...
    //   it modificate

    result = OCIBreak(
      svc_context_handle_.get(),
      local_error_handle.get());

    if(result != OCI_SUCCESS)
    {
      throw_oci_error(
        FUN, "OCIBreak", result, local_error_handle.get());
    }

    // emulate operation cancel in sqlplus way:
    // tcp packet with PSH, URG: 21
    Lda_Def lda;

    if((result = OCISvcCtxToLda( // no server round trip
          svc_context_handle_.get(),
          error_handle_.get(),
          &lda)) != OCI_SUCCESS)
    {
      throw_oci_error(FUN, "OCISvcCtxToLda", result, error_handle_.get());
    }

    int socket_descriptor;
    ognfd(&lda, &socket_descriptor);

    const Generics::Time SEND_TIMEOUT(1); // 1 second
    const unsigned char URG_MESSAGE[] = { 0x21 };

    Generics::Time wait_timeout(SEND_TIMEOUT);
    Generics::FDSet fset;
    FD_ZERO(&fset);
    FD_SET(socket_descriptor, &fset);
    Generics::Time start_time = Generics::Time::get_time_of_day();

    while(wait_timeout > Generics::Time::ZERO)
    {
      int select_result = ::select(socket_descriptor + 1,
        0, &fset, 0, &wait_timeout);
      if(select_result < 0 && errno != EINTR)
      {
        // connection closed or ...
        break;
      }

      if(select_result > 0 &&
         FD_ISSET(socket_descriptor, &fset))
      {
        ::send(socket_descriptor,
          URG_MESSAGE, sizeof(URG_MESSAGE), MSG_OOB);
        break; // error independent
      }

      Generics::Time now = Generics::Time::get_time_of_day();
      wait_timeout = SEND_TIMEOUT - (now - start_time);
    }

    if((result = OCILdaToSvcCtx( // no server round trip
          &svc_context_handle_.inout(),
          error_handle_.get(),
          &lda)) != OCI_SUCCESS)
    {
      throw_oci_error(FUN, "OCILdaToSvcCtx", result, error_handle_.get());
    }

    // close socket for force server feedback waiting on OCIReset and
    //   its possible hanging at dead connection
    // (broken_ can't be used instead reset - give memory leaks)
    ::close(socket_descriptor);

    OCIReset(server_handle_.get(), error_handle_.get());

#endif
  }

  void
  Connection::free_cache() noexcept
  {
#ifdef _USE_OCCI
//  static const char* FUN = "Connection::free_cache()";

    OCICacheFree(
      environment_->environment_handle_.get(),
      error_handle_.get(),
      svc_context_handle_.get());

#endif
  }

  void
  Connection::rollback() /*throw(SqlException, ConnectionError, NotSupported)*/
  {
#ifdef _USE_OCCI
    execute_("rollback");
    free_cache();
#endif
  }

  void
  Connection::terminate() noexcept
  {
    terminated_ = 1;
  }

  void
  Connection::owner_(ConnectionOwner* owner) noexcept
  {
    if(owner != connection_owner_.in())
    {
      if(connection_owner_.in())
      {
        ConnectionOwner_var pass_destroy_to_owner;
        connection_owner_.swap(pass_destroy_to_owner);

        pass_destroy_to_owner->destroy_connection(this);
        pass_destroy_to_owner->connection_destroyed();
      }

      connection_owner_ = ReferenceCounting::add_ref(owner);
    }
  }

  void
  Connection::execute_(const char* sql) /*throw(SqlException)*/
  {
    static const char* FUN = "Oracle::Connection::execute_()";

    try
    {
      Statement_var statement = create_statement(sql);
      statement->execute();
    }
    catch (const Exception&)
    {
      throw;
    }
    catch (const eh::Exception& e)
    {
      Stream::Error error;
      error << FUN << ": eh::Exception caught: " << e.what();
      throw Exception(error);
    }
  }

  void
  Connection::delete_this_() const noexcept
  {
    Connection* non_const_this = const_cast<Connection*>(this);

    if (connection_owner_)
    {
      ConnectionOwner_var pass_destroy_to_owner(
        std::move(non_const_this->connection_owner_));
      if (!pass_destroy_to_owner->destroy_connection(non_const_this))
      {
        return;
      }
      non_const_this->connection_owner_ = std::move(pass_destroy_to_owner);
    }

    AtomicImpl::delete_this_();
  }
}
}
}

