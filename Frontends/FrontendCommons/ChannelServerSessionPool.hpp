#ifndef CHANNELSERVERSESSIONPOOL_HPP
#define CHANNELSERVERSESSIONPOOL_HPP

#include <CORBACommons/CorbaAdapters.hpp>
#include <CORBACommons/ObjectPool.hpp>

#include <ChannelSvcs/ChannelManagerController/ChannelManagerController.hpp>
#include <ChannelSvcs/ChannelManagerController/ChannelServerSessionFactory.hpp>

namespace FrontendCommons
{
  class ChannelServerSessionPool
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    ChannelServerSessionPool(
      const CORBACommons::CorbaObjectRefList& controller_refs,
      const CORBACommons::CorbaClientAdapter* corba_client_adapter,
      Generics::ActiveObjectCallback* callback)
      /*throw(Exception)*/;

    void match(
      const AdServer::ChannelSvcs::ChannelServerBase::MatchQuery& query,
      AdServer::ChannelSvcs::ChannelServerBase::MatchResult_out result)
      /*throw(Exception)*/;

    AdServer::ChannelSvcs::ChannelServerBase::CCGKeywordSeq*
    get_ccg_traits(
      const AdServer::ChannelSvcs::ChannelIdSeq& ids)
      /*throw(Exception)*/;

  private:
    struct ChannelServerSessionPoolConfig:
      public CORBACommons::ObjectPoolRefConfiguration
    {
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

      ChannelServerSessionPoolConfig(
        const CORBACommons::CorbaClientAdapter* corba_client_adapter) noexcept
        : ObjectPoolRefConfiguration(corba_client_adapter),
          resolver(corba_client_adapter)
      {}

      struct Resolver
      {
        Resolver(
          const CORBACommons::CorbaClientAdapter* corba_client_adapter)
          noexcept;

        template<typename PoolType>
        PoolType*
        resolve(const ObjectRef& ref)
          /*throw(Exception)*/;

      private:
        CORBACommons::CorbaClientAdapter_var corba_client_adapter_;
      };

      Resolver resolver;
    };

    typedef CORBACommons::ObjectPool<
      AdServer::ChannelSvcs::ChannelServerBase,
      ChannelServerSessionPoolConfig>
      ChannelServerSessionPoolImpl;

    typedef ChannelServerSessionPoolImpl::ObjectHandlerType
      ChannelServerSessionHandler;

    typedef std::unique_ptr<ChannelServerSessionPoolImpl>
      ChannelServerSessionPoolImplPtr;

  private:
    ChannelServerSessionFactoryImpl_var channel_session_factory_;
    ChannelServerSessionPoolImplPtr pool_;
  };
}

#endif /*CHANNELSERVERSESSIONPOOL_HPP*/
