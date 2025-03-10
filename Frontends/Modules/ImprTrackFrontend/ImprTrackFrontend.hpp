#ifndef _AD_FRONTENDS_IMPR_TRACK_FRONTEND_IMPR_TRACK_FRONTEND_HPP_
#define _AD_FRONTENDS_IMPR_TRACK_FRONTEND_IMPR_TRACK_FRONTEND_HPP_

#include <string>
#include <set>

#include <eh/Exception.hpp>

#include <ReferenceCounting/AtomicImpl.hpp>
#include <Logger/Logger.hpp>
#include <Logger/DistributorLogger.hpp>
#include <Generics/FileCache.hpp>
#include <CORBACommons/CorbaAdapters.hpp>

#include <Commons/UserInfoManip.hpp>
#include <Commons/Containers.hpp>
#include <Commons/TextTemplateCache.hpp>
#include <ChannelSvcs/ChannelManagerController/ChannelServerSessionFactory.hpp>
#include <UserInfoSvcs/UserInfoManagerController/UserInfoManagerController.hpp>
#include <UserInfoSvcs/UserBindServer/UserBindServer.hpp>

#include <xsd/Frontends/FeConfig.hpp>

#include <Frontends/FrontendCommons/HTTPUtils.hpp>
#include <Frontends/FrontendCommons/CampaignManagersPool.hpp>
#include <Frontends/FrontendCommons/RequestMatchers.hpp>
#include <Frontends/FrontendCommons/ChannelServerSessionPool.hpp>
#include <Frontends/FrontendCommons/UserInfoClient.hpp>
#include <Frontends/FrontendCommons/UserBindClient.hpp>
#include <Frontends/FrontendCommons/CookieManager.hpp>
#include <Frontends/FrontendCommons/FrontendInterface.hpp>
#include <Frontends/FrontendCommons/FCGI.hpp>
#include <Frontends/FrontendCommons/FrontendTaskPool.hpp>
#include <Frontends/FrontendCommons/GrpcContainer.hpp>

#include "RequestInfoFiller.hpp"

namespace AdServer
{
namespace ImprTrack
{
  namespace Configuration
  {
    using namespace xsd::AdServer::Configuration;
  }

  struct ChannelMatch
  {
    ChannelMatch(
      unsigned long channel_id,
      unsigned long channel_trigger_id)
      : channel_id(channel_id),
        channel_trigger_id(channel_trigger_id)
      {}

    ChannelMatch(const ChannelMatch&) = default;
    ChannelMatch& operator=(const ChannelMatch&) = default;

    bool operator<(const ChannelMatch& right) const
    {
      return
        (channel_id < right.channel_id ||
          (channel_id == right.channel_id &&
            channel_trigger_id < right.channel_trigger_id));
    }

    unsigned long channel_id;
    unsigned long channel_trigger_id;
  };

  class Frontend:
    private FrontendCommons::HTTPExceptions,
    private Logging::LoggerCallbackHolder,
    public FrontendCommons::FrontendTaskPool,
    public virtual ReferenceCounting::AtomicImpl
  {
  private:
    using Exception = FrontendCommons::HTTPExceptions::Exception;

  public:
    using TaskProcessor = userver::engine::TaskProcessor;
    using GrpcContainerPtr = FrontendCommons::GrpcContainerPtr;

  public:
    Frontend(
      TaskProcessor& helper_task_processor,
      const GrpcContainerPtr& grpc_container,
      Configuration* frontend_config,
      Logging::Logger* logger,
      CommonModule* common_module,
      FrontendCommons::HttpResponseFactory* response_factory)
      /*throw(eh::Exception)*/;

    virtual bool
    will_handle(const String::SubString& uri) noexcept;

    void
    handle_request_(
      FrontendCommons::HttpRequestHolder_var request_holder,
      FrontendCommons::HttpResponseWriter_var response_writer)
      noexcept;

    /** Performs initialization for the module child process. */
    virtual void
    init() /*throw(eh::Exception)*/;

    /** Performs shutdown for the module child process. */
    virtual void
    shutdown() noexcept;

  private:
    struct TraceLevel
    {
      enum
      {
        LOW = Logging::Logger::TRACE,
        MIDDLE,
        HIGH
      };
    };

    typedef Generics::FileCache<> FileCache;
    typedef FileCache::Cache_var FileCachePtr;

    typedef Configuration::FeConfig::CommonFeConfiguration_type
      CommonFeConfiguration;

    typedef Configuration::FeConfig::ImprTrackFeConfiguration_type
      ImprTrackFeConfiguration;

    typedef std::unique_ptr<CommonFeConfiguration> CommonConfigPtr;
    typedef std::unique_ptr<ImprTrackFeConfiguration> ConfigPtr;

    typedef CORBACommons::ObjectPoolRefConfiguration
      UserBindServerPoolConfig;
    typedef CORBACommons::ObjectPool<
      AdServer::UserInfoSvcs::UserBindServer, UserBindServerPoolConfig>
      UserBindServerPool;
    typedef std::unique_ptr<UserBindServerPool> UserBindServerPoolPtr;
    typedef UserBindServerPool::ObjectHandlerType UserBindServerHandler;

    struct BindURLRule: public ReferenceCounting::AtomicImpl
    {
      bool use_keywords;
      Generics::GnuHashSet<Generics::StringHashAdapter> keywords;
      Commons::TextTemplate_var url_template;

    protected:
      virtual ~BindURLRule() noexcept
      {};
    };

    typedef ReferenceCounting::SmartPtr<BindURLRule> BindURLRule_var;
    typedef std::vector<BindURLRule_var> BindURLRuleArray;

    typedef std::vector<Commons::TextTemplate_var>
      TextTemplateArray;

    class MatchChannelsTask;

  private:
    virtual ~Frontend() noexcept;

    void
    parse_config_() /*throw(Exception)*/;

    int
    handle_request_(
      const FrontendCommons::HttpRequest& request,
      FrontendCommons::HttpResponse& response) noexcept;

    RequestInfoFiller::EncryptionKeys_var
    read_keys_(
      const xsd::AdServer::Configuration::EncryptionKeysType& src)
      /*throw(eh::Exception)*/;

    void
    report_bad_user_(const RequestInfo& request_info)
      noexcept;

    void
    match_channels_(
      const AdServer::Commons::UserId& user_id,
      const AdServer::Commons::UserId& cookie_user_id,
      const Generics::Time& now,
      const std::vector<CORBA::ULong>& campaign_ids,
      const std::vector<CORBA::ULong>& advertiser_ids,
      const String::SubString& peer_ip,
      const std::list<std::string>& markers)
      noexcept;

    void
    fill_match_request_info_(
      AdServer::CampaignSvcs::CampaignManager::MatchRequestInfo& mri,
      const AdServer::Commons::UserId& user_id,
      const Generics::Time& now,
      const std::vector<ChannelMatch>& trigger_match_result_page_channels,
      const std::vector<std::uint32_t>& history_match_result_channel_ids,
      const String::SubString& peer_ip_val)
      const noexcept;

  private:
    TaskProcessor& helper_task_processor_;
    const GrpcContainerPtr grpc_container_;

    // configuration
    std::string config_file_;

    CommonConfigPtr common_config_;
    ConfigPtr config_;
    Configuration_var frontend_config_;

    CommonModule_var common_module_;

    std::unique_ptr<RequestInfoFiller> request_info_filler_;
    std::unique_ptr<
      FrontendCommons::CookieManager<FrontendCommons::HttpRequest, FrontendCommons::HttpResponse> > cookie_manager_;
    FileCachePtr track_pixel_;
    std::string track_pixel_content_type_;
    BindURLRuleArray bind_url_rules_;

    typedef std::unique_ptr<GeoIPMapping::IPMapCity2> IPMapPtr;
    IPMapPtr ip_map_;

    // external services
    CORBACommons::CorbaClientAdapter_var corba_client_adapter_;
    FrontendCommons::CampaignManagersPool<Exception> campaign_managers_;
    ChannelServerSessionFactoryImpl_var server_session_factory_;
    std::unique_ptr<FrontendCommons::ChannelServerSessionPool>
      channel_servers_;
    FrontendCommons::UserBindClient_var user_bind_client_;
    FrontendCommons::UserInfoClient_var user_info_client_;

    Generics::TaskRunner_var task_runner_;

    Generics::StringHashAdapter track_template_file_;
    Commons::TextTemplateCache_var template_files_;
  };
}
}

// Inlines
namespace AdServer
{
namespace ImprTrack
{
  inline
  Frontend::~Frontend() noexcept
  {}
}
} // namespace AdServer

#endif // _AD_FRONTENDS_IMPR_TRACK_FRONTEND_IMPR_TRACK_FRONTEND_HPP_
