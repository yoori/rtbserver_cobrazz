#pragma once

#include <list>
#include <string>

#include <ReferenceCounting/ReferenceCounting.hpp>

#include <eh/Exception.hpp>

#include <Logger/Logger.hpp>
#include <Sync/SyncPolicy.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Generics/Time.hpp>

#include <Generics/MemBuf.hpp>

#include <Commons/Algs.hpp>
#include <ProfilingCommons/ProfileMap/TransactionProfileMap.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/Algs.hpp>

#include "TagRequestProcessor.hpp"

namespace AdServer
{
  namespace RequestInfoSvcs
  {
    struct SiteReachProcessor: public virtual ReferenceCounting::Interface
    {
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

      struct SiteReachInfo
      {
        IdAppearanceList appearance_list;

        bool operator==(const SiteReachInfo& right) const;

        void print(std::ostream& ostr, const char* offset) const;
      };

      virtual void process_site_reach(const SiteReachInfo& reach_info)
        /*throw(Exception)*/ = 0;
    };

    using SiteReachProcessor_var =
      ReferenceCounting::SmartPtr<SiteReachProcessor>;

    const Generics::Time USER_SITE_REACH_DEFAULT_EXPIRE_TIME(180*24*60*60); // 180 days

    /**
     * UserSiteReachContainer
     * contains logic of site reach match processing
     * check appearing of site_id
     *   delegate appear processing to SiteReachProcessor
     */
    class UserSiteReachContainer:
      public virtual TagRequestProcessor,
      public virtual Generics::RefCountableCompositeActiveObject
    {
    public:
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    public:
      UserSiteReachContainer(
        Logging::Logger* logger,
        SiteReachProcessor* site_reach_processor,
        const char* rocksdb_path,
        const Generics::Time& expire_time =
          USER_SITE_REACH_DEFAULT_EXPIRE_TIME)
        /*throw(Exception)*/;

      Generics::ConstSmartMemBuf_var
      get_profile(const AdServer::Commons::UserId& user_id)
        /*throw(Exception)*/;

      void clear_expired_users() /*throw(Exception)*/;

      virtual void process_tag_request(
        const TagRequestInfo& request_info)
        /*throw(TagRequestProcessor::Exception)*/;

      virtual AdServer::Commons::Awaitable<void>
      co_process_tag_request(
        const TagRequestInfo& request_info);

    protected:
      virtual ~UserSiteReachContainer() noexcept;

    private:
      using UserSiteReachMap = ProfilingCommons::TransactionProfileMap<
        AdServer::Commons::UserId>;

      using UserSiteReachMap_var =
        ReferenceCounting::SmartPtr<UserSiteReachMap>;

    private:
      void process_tag_request_(
        const TagRequestInfo& request_info) /*throw(Exception)*/;

      AdServer::Commons::Awaitable<void>
      co_process_tag_request_(
        const TagRequestInfo& request_info);

    private:
      Logging::Logger_var logger_;
      Generics::Time expire_time_;
      UserSiteReachMap_var user_map_;
      SiteReachProcessor_var site_reach_processor_;
    };

    using UserSiteReachContainer_var =
      ReferenceCounting::SmartPtr<UserSiteReachContainer>;

  } /* RequestInfoSvcs */
} /* AdServer */

namespace AdServer
{
  namespace RequestInfoSvcs
  {
    inline
    bool
    SiteReachProcessor::SiteReachInfo::operator==(
      const SiteReachProcessor::SiteReachInfo& right) const
    {
      return
        appearance_list.size() == right.appearance_list.size() &&
        std::equal(appearance_list.begin(),
          appearance_list.end(), right.appearance_list.begin());
    }

    inline
    void
    SiteReachProcessor::SiteReachInfo::print(
      std::ostream& ostr, const char* offset) const
    {
      ostr << offset << "appearance_list: ";
      Algs::print(ostr, appearance_list.begin(), appearance_list.end());
      ostr << std::endl;
    }
  }
}
