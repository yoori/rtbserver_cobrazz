#ifndef FRONTENDS_ADFRONTENDGRPC_DEBUGSINKGRPC_HPP
#define FRONTENDS_ADFRONTENDGRPC_DEBUGSINKGRPC_HPP

// STD
#include <string>

// UNIXCOMMONS
#include <Generics/GnuHashTable.hpp>
#include <GeoIP/IPMap.hpp>
#include <HTTP/Http.hpp>
#include <Logger/Logger.hpp>

// THIS
#include <Commons/Containers.hpp>
#include <Commons/LogReferrerUtils.hpp>
#include <Frontends/CommonModule/CommonModule.hpp>
#include <Frontends/FrontendCommons/FCGI.hpp>
#include <Frontends/FrontendCommons/HTTPUtils.hpp>
#include <Frontends/FrontendCommons/RequestMatchers.hpp>
#include <Frontends/FrontendCommons/RequestParamProcessor.hpp>
#include <Frontends/FrontendCommons/UserAgentMatcher.hpp>
#include <Frontends/Modules/AdFrontend/RequestInfo.hpp>

namespace AdServer::Grpc
{
  class DebugInfoStatus;

  using RequestInfoParamProcessor = FrontendCommons::RequestParamProcessor<RequestInfo>;
  using RequestInfoParamProcessor_var = ReferenceCounting::SmartPtr<RequestInfoParamProcessor>;

  class RequestInfoFiller: public FrontendCommons::HTTPExceptions
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    struct ColoFlags
    {
      unsigned long flags;
      bool hid_profile;
    };

    class ColoFlagsMap final:
      public std::map<unsigned long, ColoFlags>,
      public ReferenceCounting::AtomicImpl
    {
    protected:
      virtual ~ColoFlagsMap() noexcept {}
    };

    using ColoFlagsMap_var = ReferenceCounting::SmartPtr<ColoFlagsMap>;

  public:
    RequestInfoFiller(
      Logging::Logger* logger,
      unsigned long colo_id,
      CommonModule* common_module,
      const char* geo_ip_path,
      const char* user_agent_filter_path,
      SetUidController* set_uid_controller,
      std::set<std::string>* ip_list,
      const std::set<int>& colo_list,
      Commons::LogReferrer::Setting log_referrer_site_stas_setting)
      /*throw(eh::Exception)*/;

    void fill(RequestInfo& request_info,
      DebugInfoStatus* debug_info,
      const FrontendCommons::HttpRequest& request) const
      /*throw(InvalidParamException, ForbiddenException, Exception)*/;

    const Logging::Logger_var& logger() const noexcept
    {
      return logger_;
    }

    void colo_flags(ColoFlagsMap* colo_flags) noexcept;

    void adapt_client_id_(
      const String::SubString& in,
      RequestInfo& request_info,
      bool persistent,
      bool household,
      bool merged_persistent,
      bool rewrite_persistent) const
      /*throw(InvalidParamException)*/;

    typedef Generics::GnuHashTable<
      Generics::SubStringHashAdapter, RequestInfoParamProcessor_var>
      ParamProcessorMap;

  private:
    typedef Sync::Policy::PosixThread SyncPolicy;

    typedef std::unique_ptr<GeoIPMapping::IPMapCity2> IPMapPtr;

  private:
    ColoFlagsMap_var
    get_colocations_() const noexcept;

    void sign_client_id_(
      std::string& signed_uid,
      const AdServer::Commons::UserId& uid) const
      noexcept;

    void add_processor_(
      bool headers,
      bool parameters,
      const String::SubString& name,
      const RequestInfoParamProcessor_var& processor)
      noexcept;

    void add_processors_(
      bool headers,
      bool parameters,
      const std::initializer_list<String::SubString>& names,
      const RequestInfoParamProcessor_var& processor)
      noexcept;

  private:
    Logging::Logger_var logger_;
    unsigned long colo_id_;
    std::set<std::string> ip_list_;
    std::set<int> colo_list_;
    bool use_ip_list_;
    Commons::LogReferrer::Setting log_referrer_setting_;

    CommonModule_var common_module_;
    IPMapPtr ip_map_;
    FrontendCommons::UserAgentMatcher user_agent_matcher_;
    SetUidController_var set_uid_controller_;

    mutable SyncPolicy::Mutex colocations_lock_;
    ColoFlagsMap_var colocations_;

    ParamProcessorMap header_processors_;
    ParamProcessorMap param_processors_;
    ParamProcessorMap cookie_processors_;
  };
} // namespace AdServer::Grpc

namespace AdServer::Grpc
{
  inline
  void RequestInfoFiller::colo_flags(ColoFlagsMap* colo_flags) noexcept
  {
    ColoFlagsMap_var colo_flags_tmp = ReferenceCounting::add_ref(colo_flags);

    SyncPolicy::WriteGuard lock(colocations_lock_);
    colocations_.swap(colo_flags_tmp);
  }

  inline
  RequestInfoFiller::ColoFlagsMap_var RequestInfoFiller::get_colocations_() const noexcept
  {
    SyncPolicy::ReadGuard lock(colocations_lock_);
    return colocations_;
  }
} // namespace AdServer::Grpc

#endif /*FRONTENDS_ADFRONTENDGRPC_DEBUGSINKGRPC_HPP*/
