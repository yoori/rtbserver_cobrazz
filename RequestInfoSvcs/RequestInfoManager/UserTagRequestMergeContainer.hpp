#ifndef REQUESTINFOSVCS_REQUESTINFOMANAGER_TAGREQUESTMERGECONTAINER_HPP
#define REQUESTINFOSVCS_REQUESTINFOMANAGER_TAGREQUESTMERGECONTAINER_HPP

#include <eh/Exception.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <Generics/Time.hpp>
#include <Logger/Logger.hpp>

#include <Commons/UserInfoManip.hpp>
#include <ProfilingCommons/ProfileMap/ProfileMapFactory.hpp>
#include <ProfilingCommons/PlainStorageAdapters.hpp>

#include "TagRequestProcessor.hpp"
#include "TagRequestGroupProcessor.hpp"

namespace AdServer
{
namespace RequestInfoSvcs
{
  /** UserTagRequestMergeContainer
   * merge input tag requests into groups
   */
  class UserTagRequestMergeContainer:
    public virtual TagRequestProcessor,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    static const Generics::Time DEFAULT_EXPIRE_TIME;

    UserTagRequestMergeContainer(
      Logging::Logger* logger,
      TagRequestGroupProcessor* tag_request_group_processor,
      const Generics::Time& time_merge_bound,
      const char* file_base_path,
      const char* file_prefix,
      ProfilingCommons::ProfileMapFactory::Cache* cache,
      const Generics::Time& expire_time =
        Generics::Time(DEFAULT_EXPIRE_TIME),
      const Generics::Time& extend_time_period = Generics::Time::ZERO)
      /*throw(Exception)*/;

    Generics::ConstSmartMemBuf_var
    get_profile(const AdServer::Commons::UserId& user_id)
      /*throw(Exception)*/;

    virtual void
    process_tag_request(const TagRequestInfo&)
      /*throw(TagRequestProcessor::Exception)*/;

    void clear_expired() /*throw(Exception)*/;

  protected:
    virtual ~UserTagRequestMergeContainer() noexcept {}

  private:
    typedef ProfilingCommons::TransactionProfileMap<
      AdServer::Commons::UserId>
      UserMap;

    typedef ReferenceCounting::SmartPtr<UserMap> UserMap_var;

  private:
    void process_tag_request_trans_(
      TagRequestGroupProcessor::TagRequestGroupInfoList&
        tag_request_group_info_list,
      const TagRequestInfo& tag_request_info)
      /*throw(Exception)*/;

  private:
    Logging::FLogger_var logger_;
    const TagRequestGroupProcessor_var tag_request_group_processor_;
    const Generics::Time time_merge_bound_;
    const Generics::Time expire_time_;
    UserMap_var user_map_;
  };

  typedef ReferenceCounting::SmartPtr<UserTagRequestMergeContainer>
    UserTagRequestMergeContainer_var;
}
}

#endif /*REQUESTINFOSVCS_REQUESTINFOMANAGER_TAGREQUESTMERGECONTAINER_HPP*/
