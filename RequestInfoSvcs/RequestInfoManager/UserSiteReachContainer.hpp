#ifndef _REQUEST_INFO_SVCS_USER_SITE_REACH_CONTAINER_IMPL_HPP_
#define _REQUEST_INFO_SVCS_USER_SITE_REACH_CONTAINER_IMPL_HPP_

#include <list>
#include <string>

#include <ReferenceCounting/ReferenceCounting.hpp>

#include <eh/Exception.hpp>

#include <Logger/Logger.hpp>
#include <Sync/SyncPolicy.hpp>
#include <Generics/Time.hpp>

#include <Generics/MemBuf.hpp>

#include <Commons/Algs.hpp>
#include <ProfilingCommons/ProfileMap/ProfileMapFactory.hpp>
#include <ProfilingCommons/PlainStorageAdapters.hpp>
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

    typedef ReferenceCounting::SmartPtr<SiteReachProcessor>
      SiteReachProcessor_var;

    const Generics::Time USER_SITE_REACH_DEFAULT_EXPIRE_TIME(180*24*60*60); // 180 days

    /**
     * UserSiteReachContainer
     * contains logic of site reach match processing
     * check appearing of site_id
     *   delegate appear processing to SiteReachProcessor
     */
    class UserSiteReachContainer:
      public virtual TagRequestProcessor,
      public virtual ReferenceCounting::AtomicImpl
    {
    public:
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    public:
      UserSiteReachContainer(
        Logging::Logger* logger,
        SiteReachProcessor* site_reach_processor,
        const char* file_base_path,
        const char* file_prefix,
        ProfilingCommons::ProfileMapFactory::Cache* cache,
        const Generics::Time& expire_time =
          USER_SITE_REACH_DEFAULT_EXPIRE_TIME,
        const Generics::Time& extend_time_period = Generics::Time::ZERO)
        /*throw(Exception)*/;

      Generics::ConstSmartMemBuf_var
      get_profile(const AdServer::Commons::UserId& user_id)
        /*throw(Exception)*/;

      void clear_expired_users() /*throw(Exception)*/;

      virtual void process_tag_request(
        const TagRequestInfo& request_info)
        /*throw(TagRequestProcessor::Exception)*/;

    protected:
      virtual ~UserSiteReachContainer() noexcept;

    private:
      typedef ProfilingCommons::TransactionProfileMap<
        AdServer::Commons::UserId>
        UserSiteReachMap;

      typedef ReferenceCounting::SmartPtr<UserSiteReachMap>
        UserSiteReachMap_var;

    private:
      void process_tag_request_(
        const TagRequestInfo& request_info) /*throw(Exception)*/;

    private:
      Logging::Logger_var logger_;
      Generics::Time expire_time_;
      UserSiteReachMap_var user_map_;
      SiteReachProcessor_var site_reach_processor_;
    };

    typedef ReferenceCounting::SmartPtr<UserSiteReachContainer>
      UserSiteReachContainer_var;

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

#endif /*_REQUEST_INFO_SVCS_USER_SITE_REACH_CONTAINER_IMPL_HPP_*/
