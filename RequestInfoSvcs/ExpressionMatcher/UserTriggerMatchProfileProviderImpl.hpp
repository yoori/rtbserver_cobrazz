#ifndef EXPRESSIONMATCHER_USERTRIGGERMATCHPROFILEPROVIDERIMPL_HPP
#define EXPRESSIONMATCHER_USERTRIGGERMATCHPROFILEPROVIDERIMPL_HPP

#include <list>
#include <vector>

#include <eh/Exception.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/DefaultImpl.hpp>
#include <Logger/Logger.hpp>
#include <Generics/Time.hpp>
#include <Generics/MemBuf.hpp>

#include <Commons/UserInfoManip.hpp>
#include <Commons/CorbaObject.hpp>
#include <Commons/HostDistribution.hpp>

#include "UserTriggerMatchContainer.hpp"
#include <RequestInfoSvcs/ExpressionMatcher/ExpressionMatcher.hpp>

namespace AdServer
{
namespace RequestInfoSvcs
{
  class UserTriggerMatchProfileProviderImpl:
    public UserTriggerMatchProfileProvider,
    public ReferenceCounting::AtomicImpl
  {
  public:
    UserTriggerMatchProfileProviderImpl(
      const CORBACommons::CorbaClientAdapter* corba_client_adapter,
      const CORBACommons::CorbaObjectRefList& expression_matcher_refs,
      Commons::HostDistributionFile* host_distr,
      const char* self_host_name,
      UserTriggerMatchContainer* self_provider,
      unsigned long common_chunks_number)
      noexcept;

    virtual
    Generics::ConstSmartMemBuf_var
    get_user_profile(const AdServer::Commons::UserId& user_id)
      /*throw(Exception)*/;

  protected:
    virtual ~UserTriggerMatchProfileProviderImpl() noexcept
    {}

  private:
    typedef AdServer::Commons::CorbaObject<
      AdServer::RequestInfoSvcs::ExpressionMatcher>
      ExpressionMatcherRef;

    typedef std::vector<ExpressionMatcherRef> ExpressionMatcherRefArray;

  private:
    std::map<unsigned long, ExpressionMatcherRef> chunks_ref_map_;
    std::set<unsigned long> own_chunks_;
    const UserTriggerMatchContainer_var self_provider_;
    unsigned long common_chunks_number_;
  };
}
}

#endif /*EXPRESSIONMATCHER_USERTRIGGERMATCHPROFILEPROVIDERIMPL_HPP*/
