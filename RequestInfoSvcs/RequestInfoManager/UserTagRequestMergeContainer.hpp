#pragma once

#include <eh/Exception.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Generics/Time.hpp>
#include <Logger/Logger.hpp>

#include <Commons/UserInfoManip.hpp>
#include <ProfilingCommons/ProfileMap/TransactionProfileMap.hpp>

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
    public virtual Generics::RefCountableCompositeActiveObject
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    static const Generics::Time DEFAULT_EXPIRE_TIME;

      UserTagRequestMergeContainer(
        Logging::Logger* logger,
        TagRequestGroupProcessor* tag_request_group_processor,
        const Generics::Time& time_merge_bound,
        const char* rocksdb_path,
        const Generics::Time& expire_time =
          Generics::Time(DEFAULT_EXPIRE_TIME))
      /*throw(Exception)*/;

    Generics::ConstSmartMemBuf_var
    get_profile(const AdServer::Commons::UserId& user_id)
      /*throw(Exception)*/;

    virtual void
    process_tag_request(const TagRequestInfo&)
      /*throw(TagRequestProcessor::Exception)*/;

    virtual AdServer::Commons::Awaitable<void>
    co_process_tag_request(const TagRequestInfo&);

    void clear_expired() /*throw(Exception)*/;

  protected:
    virtual ~UserTagRequestMergeContainer() noexcept {}

  private:
    using UserMap = ProfilingCommons::TransactionProfileMap<
      AdServer::Commons::UserId>;

    using UserMap_var = ReferenceCounting::SmartPtr<UserMap>;

  private:
    void process_tag_request_trans_(
      TagRequestGroupProcessor::TagRequestGroupInfoList&
        tag_request_group_info_list,
      const TagRequestInfo& tag_request_info)
      /*throw(Exception)*/;

    AdServer::Commons::Awaitable<void>
    co_process_tag_request_trans_(
      TagRequestGroupProcessor::TagRequestGroupInfoList&
        tag_request_group_info_list,
      const TagRequestInfo& tag_request_info);

  private:
    Logging::FLogger_var logger_;
    const TagRequestGroupProcessor_var tag_request_group_processor_;
    const Generics::Time time_merge_bound_;
    const Generics::Time expire_time_;
    UserMap_var user_map_;
  };

  using UserTagRequestMergeContainer_var =
    ReferenceCounting::SmartPtr<UserTagRequestMergeContainer>;
}
}
