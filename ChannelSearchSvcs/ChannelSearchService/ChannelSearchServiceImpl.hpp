#pragma once


#include <eh/Exception.hpp>

#include <ReferenceCounting/ReferenceCounting.hpp>

#include <Logger/Logger.hpp>
#include <Sync/SyncPolicy.hpp>
#include <Generics/ActiveObject.hpp>
#include <Generics/Scheduler.hpp>
#include <Language/SegmentorCommons/SegmentorInterface.hpp>

#include <Commons/BoostAsioContextRunActiveObject.hpp>
#include <Commons/Grpc/GrpcExecutor.hpp>
#include <CORBACommons/CorbaAdapters.hpp>
#include <CORBACommons/ObjectPool.hpp>
#include <CORBACommons/ServantImpl.hpp>

#include <CampaignSvcs/CampaignCommons/ExpressionChannelIndex.hpp>
#include <CampaignSvcs/CampaignServer/CampaignServer.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignSvcsVersionAdapter.hpp>

#include <ChannelSvcs/ChannelCommons/ChannelServer.hpp>
#include <ChannelSvcs/ChannelClient/ChannelDistributedGrpcClient.hpp>
#include <ChannelSearchSvcs/ChannelSearchService/ChannelSearchService_s.hpp>

#include <xsd/ChannelSearchSvcs/ChannelSearchServiceConfig.hpp>

#include "ChannelMatcher.hpp"

namespace AdServer::ChannelSearchSvcs
{
  class ChannelSearchServiceImpl:
    virtual public CORBACommons::ReferenceCounting::ServantImpl<
      POA_AdServer::ChannelSearchSvcs::ChannelSearch>,
    virtual public Generics::RefCountableActiveObject,
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

    typedef std::shared_ptr<AdServer::ChannelSvcs::ChannelDistributedGrpcClient>
      ChannelDistributedGrpcClientPtr;

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
    resolve_channel_session_(const ConfigType &config, Generics::ActiveObjectCallback* callback)
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
    std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor_;
    Generics::TaskRunner_var task_runner_;
    Generics::Planner_var scheduler_;

    CampaignServerPoolPtr campaign_servers_;
    const unsigned SERVICE_INDEX_;

    ChannelDistributedGrpcClientPtr channel_client_;

    ChannelMatcher_var channel_matcher_;
  };

  typedef ReferenceCounting::SmartPtr<ChannelSearchServiceImpl>
    ChannelSearchServiceImpl_var;

} // namespace AdServer::ChannelSearchSvcs

namespace AdServer::ChannelSearchSvcs
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

}
