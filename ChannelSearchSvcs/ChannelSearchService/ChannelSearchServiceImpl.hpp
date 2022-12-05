#ifndef _AD_SERVER_CHANNEL_SEARCH_SVCS_CHANNEL_SEARCH_SERVICE_IMPL_HPP_
#define _AD_SERVER_CHANNEL_SEARCH_SVCS_CHANNEL_SEARCH_SERVICE_IMPL_HPP_


#include <eh/Exception.hpp>

#include <ReferenceCounting/ReferenceCounting.hpp>

#include <Logger/Logger.hpp>
#include <Sync/SyncPolicy.hpp>
#include <Generics/ActiveObject.hpp>
#include <Generics/Scheduler.hpp>
#include <Language/SegmentorCommons/SegmentorInterface.hpp>

#include <CORBACommons/CorbaAdapters.hpp>
#include <CORBACommons/ServantImpl.hpp>

#include <CampaignSvcs/CampaignCommons/ExpressionChannelIndex.hpp>
#include <CampaignSvcs/CampaignServer/CampaignServer.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignSvcsVersionAdapter.hpp>

#include <ChannelSvcs/ChannelManagerController/ChannelManagerController.hpp>
#include <ChannelSvcs/ChannelManagerController/ChannelServerSessionFactory.hpp>
#include <ChannelSearchSvcs/ChannelSearchService/ChannelSearchService_s.hpp>

#include <xsd/ChannelSearchSvcs/ChannelSearchServiceConfig.hpp>

#include "ChannelMatcher.hpp"

namespace AdServer
{
  namespace ChannelSearchSvcs
  {
    class ChannelSearchServiceImpl:
      virtual public CORBACommons::ReferenceCounting::ServantImpl<
        POA_AdServer::ChannelSearchSvcs::ChannelSearch>,
      virtual public Generics::CompositeActiveObject
    {
    public:
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

      typedef xsd::AdServer::Configuration::ChannelSearchServiceConfigType
        ConfigType;

      typedef AdServer::CampaignSvcs::ChannelIdSet ChannelIdSet;

    public:
      ChannelSearchServiceImpl(
        Generics::ActiveObjectCallback* callback,
        Logging::Logger* logger,
        const ConfigType& config)
        /*throw(Exception, eh::Exception)*/;

      virtual ~ChannelSearchServiceImpl() noexcept;

      virtual
      AdServer::ChannelSearchSvcs::ChannelSearchResultSeq*
      search(const char *phrase)
        /*throw(AdServer::ChannelSearchSvcs::ChannelSearch::ImplementationException)*/;

      virtual
      AdServer::ChannelSearchSvcs::ChannelSearchResultSeq*
      wsearch(const CORBA::WChar* phrase)
        /*throw(AdServer::ChannelSearchSvcs::ChannelSearch::ImplementationException)*/;

      virtual
      AdServer::ChannelSearchSvcs::MatchInfo*
      match(const char *url, const char* phrase, CORBA::Long channels_count)
        /*throw(AdServer::ChannelSearchSvcs::ChannelSearch::ImplementationException)*/;

      virtual
      AdServer::ChannelSearchSvcs::WMatchInfo*
      wmatch(const CORBA::WChar *url, const CORBA::WChar *phrase, CORBA::Long channels_count)
        /*throw(AdServer::ChannelSearchSvcs::ChannelSearch::ImplementationException)*/;

    private:
      typedef Sync::Policy::PosixThread SyncPolicy;

      typedef AdServer::CampaignSvcs::ExpressionChannelIndex_var
        ExpressionChannelIndex_var;

      typedef CORBACommons::ObjectPoolRefConfiguration
        CampaignServerPoolConfig;

      typedef CORBACommons::ObjectPool<
        AdServer::CampaignSvcs::CampaignServer,
        CampaignServerPoolConfig>
        CampaignServerPool;

      typedef std::unique_ptr<CampaignServerPool> CampaignServerPoolPtr;

      struct ChannelServerSessionPoolConfig:
        public CORBACommons::ObjectPoolRefConfiguration
      {
        ChannelServerSessionPoolConfig(
          const CORBACommons::CorbaClientAdapter* corba_client_adapter,
          ChannelServerSessionFactoryImpl* factory) noexcept
          : ObjectPoolRefConfiguration(corba_client_adapter),
            channel_session_factory_(::ReferenceCounting::add_ref(factory)),
            resolver(corba_client_adapter)
        {}

        struct Resolver
        {
          Resolver(
            const CORBACommons::CorbaClientAdapter* corba_client_adapter)
            noexcept;

          template <typename PoolType>
          PoolType*
          resolve(const ObjectRef& ref)
            /*throw(Exception)*/;

        private:
          CORBACommons::CorbaClientAdapter_var c_adapter_;
        };

        ChannelServerSessionFactoryImpl_var channel_session_factory_;
        Resolver resolver;
      };

      typedef
        CORBACommons::ObjectPool<
          AdServer::ChannelSvcs::ChannelServerBase,
          ChannelServerSessionPoolConfig>
        ChannelServerSessionPool;

      typedef std::unique_ptr<ChannelServerSessionPool>
        ChannelServerSessionPoolPtr;
      typedef ChannelServerSessionPool::ObjectHandlerType ChannelServerHandler;

      class UpdateExpressionChannelsTask: public Generics::TaskGoal
      {
      public:
        UpdateExpressionChannelsTask(
          ChannelSearchServiceImpl* channel_search_service_impl,
          Generics::TaskRunner* task_runner)
          noexcept;

        virtual void execute() noexcept;

      protected:
        ChannelSearchServiceImpl* channel_search_service_impl_;
        Generics::TaskRunner* task_runner_;
      };

    private:
      static CampaignServerPoolPtr
      resolve_campaign_servers_(
        const CORBACommons::CorbaClientAdapter* corba_client_adapter,
        const ConfigType& config)
        /*throw(Exception, eh::Exception)*/;

      void
      resolve_channel_session_(const ConfigType &config,
        Generics::ActiveObjectCallback* callback)
        /*throw(Exception)*/;

      void update_expression_channels_() noexcept;

      void merge_trigger_channels_(
        ChannelIdSet& result_channels,
        const AdServer::ChannelSvcs::ChannelServerBase::MatchResult& match_result)
        /*throw(eh::Exception)*/;

      void
      channel_session_match_(
        const AdServer::ChannelSvcs::ChannelServerBase::MatchQuery& query,
        AdServer::ChannelSvcs::ChannelServerBase::MatchResult_var&
          match_result)
        /*throw(
          AdServer::ChannelSearchSvcs::ChannelSearch::ImplementationException)*/;

    private:
      Logging::Logger_var logger_;

      CORBACommons::CorbaClientAdapter_var corba_client_adapter_;
      Generics::TaskRunner_var task_runner_;
      Generics::Planner_var scheduler_;

      CampaignServerPoolPtr campaign_servers_;
      const unsigned SERVICE_INDEX_;

      ChannelServerSessionFactoryImpl_var server_session_factory_;
      ChannelServerSessionPoolPtr channel_manager_controllers_;

      ChannelMatcher_var channel_matcher_;
    };

    typedef ReferenceCounting::SmartPtr<ChannelSearchServiceImpl>
      ChannelSearchServiceImpl_var;

  } // namespace ChannelSearchSvcs
} // namespace AdServer

namespace AdServer
{
namespace ChannelSearchSvcs
{
  inline
  ChannelSearchServiceImpl::
  UpdateExpressionChannelsTask::UpdateExpressionChannelsTask(
    ChannelSearchServiceImpl* channel_search_service_impl,
    Generics::TaskRunner* task_runner)
    noexcept
    : Generics::TaskGoal(task_runner),
      channel_search_service_impl_(channel_search_service_impl)
  {}

  inline
  void
  ChannelSearchServiceImpl::
  UpdateExpressionChannelsTask::execute() noexcept
  {
    channel_search_service_impl_->update_expression_channels_();
  }

  inline
  ChannelSearchServiceImpl::ChannelServerSessionPoolConfig::Resolver::Resolver(
    const CORBACommons::CorbaClientAdapter* corba_client_adapter)
    noexcept
    : c_adapter_(
        ::ReferenceCounting::add_ref(corba_client_adapter))
  {
  }

  template <typename PoolType>
  PoolType*
  ChannelSearchServiceImpl::ChannelServerSessionPoolConfig::Resolver::resolve(
    const ObjectRef& ref)
    /*throw(Exception)*/
  {
    static const char* FUN = "ChannelSearchServiceImpl::"
      "ChannelServerSessionPoolConfig::Resolver::resolve()";
    try
    {
      CORBA::Object_var obj = c_adapter_->resolve_object(ref);

      AdServer::ChannelSvcs::ChannelManagerController_var manager =
        AdServer::ChannelSvcs::ChannelManagerController::_narrow(obj);
      if(CORBA::is_nil(manager))
      {
        return 0;
      }

      AdServer::ChannelSvcs::ChannelServerSession_var channel_session =
        manager->get_channel_session();

      return AdServer::ChannelSvcs::ChannelServerSession::_narrow(
        channel_session);
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN <<
        ": failed to resolve channel manager controller reference '" <<
        ref.object_ref << "'. eh::Exception caught: " << ex.what();
      throw Exception(ostr);
    }
    catch (const AdServer::ChannelSvcs::ImplementationException& ex)
    {
      Stream::Error ostr;
      ostr << FUN <<
        ": failed to resolve channel manager controller reference '" <<
        ref.object_ref << "'. AdServer::ChannelSvcs::"
          "ImplementationException caught: " <<
        ex.description;
      throw Exception(ostr);
    }
    catch (const CORBA::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN <<
        ": failed to resolve channel manager controller reference '" <<
        ref.object_ref << "'. CORBA::Exception caught: " << ex;
      throw Exception(ostr);
    }
  }
}
}

#endif /* _AD_SERVER_CHANNEL_SEARCH_SVCS_CHANNEL_SEARCH_SERVICE_IMPL_HPP_ */

