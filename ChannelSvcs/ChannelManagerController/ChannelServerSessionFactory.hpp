#pragma once

#include <ChannelSvcs/ChannelManagerController/ChannelSessionFactory.hpp>
#include <ChannelSvcs/ChannelManagerController/ChannelLoadSessionFactory.hpp>

namespace AdServer::ChannelSvcs
{
  /** ChannelServerSessionFactory */
  class ChannelServerSessionFactory
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    static void init(
        const CORBACommons::CorbaClientAdapter& corba_client_adapter,
        ChannelServerSessionFactoryImpl_var* server_session_factory,
        ChannelLoadSessionFactoryImpl_var* load_session_factory,
        Generics::ActiveObjectCallback* callback,
        Logging::Logger* stat_logger,
        unsigned long count_match_threads = 12,
        unsigned long count_update_threads = 40,
        unsigned long check_period = 30)
        /*throw(eh::Exception)*/;
  };

} // namespace AdServer::ChannelSvcs
