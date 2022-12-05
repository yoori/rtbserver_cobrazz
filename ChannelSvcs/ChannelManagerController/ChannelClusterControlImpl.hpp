

#ifndef CHANNEL_CLUSTER_CONTROLLER_HPP
#define CHANNEL_CLUSTER_CONTROLLER_HPP

#include <ReferenceCounting/ReferenceCounting.hpp>
#include <eh/Exception.hpp>
#include <Generics/ActiveObject.hpp>
#include <CORBACommons/CorbaAdapters.hpp>
#include <CORBACommons/ProcessControl.hpp>
#include <ChannelSvcs/ChannelManagerController/ChannelClusterControl.hpp>

  class ChannelClusterSessionFactoryImpl :
    public virtual OBV_AdServer::ChannelSvcs::ChannelClusterSession,
    public virtual CORBACommons::ReferenceCounting::CorbaRefCountImpl<
      CORBA::ValueFactoryBase>
  {
  public:
    ChannelClusterSessionFactoryImpl(
      Generics::ActiveObjectCallback* callback = 0) noexcept;

    virtual ~ChannelClusterSessionFactoryImpl() noexcept {};

    virtual CORBA::ValueBase* create_for_unmarshal();

    void report_error(
      Generics::ActiveObjectCallback::Severity severity,
      const String::SubString& description,
      const char* error_code = 0) noexcept;

  private:
    Generics::ActiveObjectCallback_var callback_;

  };

namespace AdServer
{
namespace ChannelSvcs
{
  class ChannelClusterSessionImpl:
    public virtual OBV_AdServer::ChannelSvcs::ChannelClusterSession
  {
  public:
    ChannelClusterSessionImpl(Generics::ActiveObjectCallback* callback)
      noexcept;

    ChannelClusterSessionImpl(
      const AdServer::ChannelSvcs::ProcessControlDescriptionSeq& descr)
      noexcept;

    virtual ~ChannelClusterSessionImpl() noexcept{};

    virtual CORBACommons::IProcessControl::ALIVE_STATUS is_alive() noexcept;

    virtual void shutdown(CORBA::Boolean) noexcept;

    virtual char* comment() /*throw(CORBACommons::OutOfMemory)*/;

    void report_error(
      Generics::ActiveObject* /*object*/,
      Generics::ActiveObjectCallback::Severity /*severity*/,
      const char* /*description*/,
      const char* /*error_code*/) noexcept {} ;

  private:
    Generics::ActiveObjectCallback_var callback_;
  };

  class ChannelClusterSessionFactory
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    static void init(
      CORBACommons::CorbaClientAdapter& corba_client_adapter,
      Generics::ActiveObjectCallback* callback)
      /*throw(eh::Exception)*/;
  };
}
}

#endif //CHANNEL_CLUSTER_CONTROLLER_HPP

