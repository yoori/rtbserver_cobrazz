#ifndef COMMONS_ZMQSOCKETHOLDER_HPP_
#define COMMONS_ZMQSOCKETHOLDER_HPP_

#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <Sync/SyncPolicy.hpp>
#include <Commons/zmq.hpp>

namespace AdServer
{
namespace Commons
{
  struct ZmqSocketHolder: public ReferenceCounting::AtomicImpl
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    typedef Sync::Policy::PosixThread SyncPolicy;

    struct SendGuard
    {
    public:
      SendGuard(ZmqSocketHolder& holder);

      virtual
      ~SendGuard() noexcept;

      bool
      send(
        zmq::message_t& msg,
        int flags,
        const char* part_name)
        /*throw(eh::Exception)*/;

    protected:
      ZmqSocketHolder& socket_holder_;
    };

  public:
    ZmqSocketHolder(zmq::context_t& zmq_context, const char* address)
      /*throw(eh::Exception)*/;

    ZmqSocketHolder(zmq::context_t& zmq_context, int type)
      /*throw(eh::Exception)*/;

    zmq::socket_t&
    sock_i()
    {
      return sock_;
    }

  protected:
    virtual ~ZmqSocketHolder() noexcept
    {}

    void
    init_sock_();

  protected:
    SyncPolicy::Mutex lock_;
    zmq::socket_t sock_;
  };

  typedef ReferenceCounting::SmartPtr<ZmqSocketHolder>
    ZmqSocketHolder_var;
}
}

#endif /*COMMONS_ZMQSOCKETHOLDER_HPP_*/
