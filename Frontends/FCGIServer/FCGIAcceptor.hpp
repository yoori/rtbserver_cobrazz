#pragma once

#include <list>
#include <pthread.h>

#include <boost/asio.hpp>
#include <boost/asio/local/stream_protocol.hpp>

#include <Generics/Time.hpp>
#include <Generics/CompositeActiveObject.hpp>

#include <Frontends/FrontendCommons/FrontendInterface.hpp>

#include "WorkerStatsObject.hpp"

namespace AdServer::Frontends
{
  class FCGIAcceptor:
    public Generics::CompositeActiveObject,
    public ReferenceCounting::AtomicImpl
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    FCGIAcceptor(
      Logging::Logger* logger,
      FrontendCommons::FrontendInterface* frontend,
      Generics::ActiveObjectCallback* callback,
      const String::SubString& bind_address,
      unsigned long backlog,
      unsigned long accept_threads)
      /*throw(eh::Exception)*/;

    FrontendCommons::FrontendInterface*
    handler() noexcept;

    Logging::Logger*
    logger() noexcept;

    virtual void
    activate_object()
      /*throw(Exception, eh::Exception)*/;

    virtual void
    deactivate_object()
      /*throw(Exception, eh::Exception)*/;

    virtual void
    wait_object()
      /*throw(Exception, eh::Exception)*/;

  protected:
    struct State;
    typedef ReferenceCounting::SmartPtr<State> State_var;

    class Worker;
    typedef ReferenceCounting::SmartPtr<Worker> Worker_var;

    class AcceptActiveObject;

    class FCGIResponseWriter;

    typedef Sync::Policy::PosixThread WorkersSyncPolicy;
    typedef std::deque<Worker_var> WorkerArray;

    class Connection;
    typedef std::shared_ptr<Connection> Connection_var;

    typedef boost::asio::local::stream_protocol::endpoint EndpointType;
    typedef boost::asio::local::stream_protocol::acceptor AcceptorType;

  protected:
    virtual
    ~FCGIAcceptor() noexcept;

    void
    create_accept_stub_();

    void
    handle_accept_(
      const Connection_var& accepted_connection,
      const boost::system::error_code& error);

  private:
    const Logging::Logger_var logger_;
    const FrontendCommons::Frontend_var frontend_;

    WorkerStatsObject_var worker_stats_object_;
    State_var state_;

    //std::shared_ptr<boost::asio::io_service> accept_io_service_;
    std::shared_ptr<boost::asio::io_service> io_service_;
    std::shared_ptr<AcceptorType> acceptor_;
  };
} // namespace AdServer::Frontends
