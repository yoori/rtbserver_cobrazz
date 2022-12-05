#include <Commons/CorbaAlgs.hpp>
#include "UserTriggerMatchProfileProviderImpl.hpp"

namespace AdServer
{
namespace RequestInfoSvcs
{
  UserTriggerMatchProfileProviderImpl::UserTriggerMatchProfileProviderImpl(
    const CORBACommons::CorbaClientAdapter* corba_client_adapter,
    const CORBACommons::CorbaObjectRefList& expression_matcher_refs,
    Commons::HostDistributionFile* host_distr,
    const char* self_host_name,
    UserTriggerMatchContainer* self_provider,
    unsigned long common_chunks_number)
    noexcept
    : self_provider_(ReferenceCounting::add_ref(self_provider)),
      common_chunks_number_(common_chunks_number)
  {
    Commons::HostDistributionFile::IndexToHostMap chunk_id_to_host = host_distr->all_indexes();

    for(Commons::HostDistributionFile::IndexToHostMap::const_iterator iter =
          chunk_id_to_host.begin();
        iter != chunk_id_to_host.end();
        ++iter)
    {
      if (iter->second == self_host_name)
      {
        own_chunks_.insert(iter->first);
        continue;
      }

      for(CORBACommons::CorbaObjectRefList::const_iterator ref_it =
            expression_matcher_refs.begin();
          ref_it != expression_matcher_refs.end(); ++ref_it)
      {
        if(ref_it->object_ref.find(iter->second) != std::string::npos)
        {
          chunks_ref_map_.insert(
          std::make_pair(
            iter->first,
            ExpressionMatcherRef(corba_client_adapter, *ref_it)));
          break;
        }
      }
    }
  }

  Generics::ConstSmartMemBuf_var
  UserTriggerMatchProfileProviderImpl::get_user_profile(
    const AdServer::Commons::UserId& user_id)
    /*throw(Exception)*/
  {
    static const char* FUN = "UserTriggerMatchProfileProviderImpl::get_user_profile()";

    unsigned long chunk_index = AdServer::Commons::uuid_distribution_hash(user_id) % common_chunks_number_;

    if(own_chunks_.count(chunk_index) > 0)
    {
      try
      {
        return self_provider_->get_user_profile(user_id);
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN <<
          ": Can't get profile, caught eh::Exception: " << ex.what();
        throw Exception(ostr);
      }
    }
    else
    {
      try
      {
        AdServer::RequestInfoSvcs::UserTriggerMatchProfile_var profile;

        if(chunks_ref_map_[chunk_index]->get_user_trigger_match_profile(
             user_id.to_string().c_str(),
             true,
             profile.out()))
        {
          return Generics::transfer_membuf(
            CorbaAlgs::unpack_mem_buf(profile.in()));
        }

        return Generics::ConstSmartMemBuf_var();
      }
      catch(const AdServer::RequestInfoSvcs::ExpressionMatcher::ImplementationException& e)
      {
        Stream::Error ostr;
        ostr << FUN <<
          ": Can't get profile, caught ExpressionMatcher::ImplementationException: " <<
          e.description;
        throw Exception(ostr);
      }
      catch(const AdServer::RequestInfoSvcs::ExpressionMatcher::NotReady& e)
      {
        Stream::Error ostr;
        ostr << FUN <<
          ": Can't get profile, caught ExpressionMatcher::NotReady";
        throw Exception(ostr);
      }
      catch(const CORBA::SystemException& e)
      {
        Stream::Error ostr;
        ostr << FUN <<
          ": Can't get profile, caught CORBA::SystemException: " << e;
        throw Exception(ostr);
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN <<
          ": Can't get profile, caught eh::Exception: " << ex.what();
        throw Exception(ostr);
      }
    }
  }
}
}
