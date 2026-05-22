#pragma once

#include <memory>

#include <CORBACommons/CorbaAdapters.hpp>
#include <CORBACommons/ObjectPool.hpp>

#include <ChannelSvcs/ChannelManagerController/ChannelManagerController.hpp>
#include <ChannelSvcs/ChannelManagerController/ChannelServerSessionFactory.hpp>
#include <ChannelSvcs/ChannelClient/ChannelGrpcAlgs.hpp>
#include <ChannelServerGrpc.grpc-client.hpp>

namespace AdServer::ChannelSvcs
{
  class ChannelCorbaClient:
    public ChannelServerGrpcAsyncClient
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    ChannelCorbaClient(
      const CORBACommons::CorbaObjectRefList& controller_refs,
      const CORBACommons::CorbaClientAdapter* corba_client_adapter,
      Generics::ActiveObjectCallback* callback)
      /*throw(Exception)*/;

    ~ChannelCorbaClient() noexcept override = default;

    void match(
      const adserver::channel_svcs::channel_server::MatchRequest& request,
      MatchCallback callback) override;

    void get_ccg_traits(
      const adserver::channel_svcs::channel_server::GetCcgTraitsRequest& request,
      GetCcgTraitsCallback callback) override;

    void set_sources(
      const adserver::channel_svcs::channel_server::SetSourcesRequest& request,
      SetSourcesCallback callback) override;

    void set_proxy_sources(
      const adserver::channel_svcs::channel_server::SetProxySourcesRequest& request,
      SetProxySourcesCallback callback) override;

  protected:
    static void pack_get_ccg_traits_request_(
      const ChannelIdSeq& source,
      adserver::channel_svcs::channel_server::GetCcgTraitsRequest& target);

    static ChannelServerBase::CCGKeywordSeq* unpack_get_ccg_traits_response_(
      const adserver::channel_svcs::channel_server::GetCcgTraitsResponse&
        source);

  private:
    void match(
      const ChannelServerBase::MatchQuery& query,
      ChannelServerBase::MatchResult_out result)
      /*throw(Exception)*/;

    ChannelServerBase::CCGKeywordSeq* get_ccg_traits(
      const ChannelIdSeq& ids)
      /*throw(Exception)*/;

  private:
    struct PoolConfig:
      public CORBACommons::ObjectPoolRefConfiguration
    {
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

      PoolConfig(
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
      ChannelServerBase,
      PoolConfig>
      PoolImpl;

    typedef PoolImpl::ObjectHandlerType ChannelServerHandler;

    typedef std::unique_ptr<PoolImpl> PoolImplPtr;

  private:
    ChannelServer_var direct_channel_server_;
    ChannelServerSessionFactoryImpl_var channel_session_factory_;
    PoolImplPtr pool_;
  };

  using ChannelCorbaClientPtr = std::shared_ptr<ChannelCorbaClient>;
}

namespace FrontendCommons
{
  using ChannelCorbaClient = AdServer::ChannelSvcs::ChannelCorbaClient;
  using ChannelCorbaClientPtr = AdServer::ChannelSvcs::ChannelCorbaClientPtr;
}
