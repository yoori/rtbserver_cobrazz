
#include "UserAgentStat.hpp"
#include <LogCommons/LogCommons.ipp>

namespace AdServer {
namespace LogProcessing {

template <> const char* UserAgentStatTraits::B::base_name_ = "UserAgentStat";
template <> const char* UserAgentStatTraits::B::signature_ = "UserAgentStat";
template <> const char* UserAgentStatTraits::B::current_version_ = "2.5";

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, UserAgentStatInnerKey& key)
  /*throw(eh::Exception)*/
{
  Aux_::StringIoWrapper user_agent_wrapper;
  is >> user_agent_wrapper;
  key.user_agent_ =
    new AdServer::Commons::StringHolder(std::move(user_agent_wrapper));
  key.invariant();
  key.calc_hash_();
  return is;
}

std::ostream&
operator<<(std::ostream& os, const UserAgentStatInnerKey& key)
  /*throw(eh::Exception)*/
{
  key.invariant();
  os << Aux_::StringIoWrapper(key.user_agent_->str());
  return os;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, UserAgentStatInnerData& data)
  /*throw(eh::Exception)*/
{
  data.holder_ = new UserAgentStatInnerData::DataHolder;
  is >> data.requests_;
  is >> data.holder_->channels;
  is >> data.holder_->platforms;
  return is;
}

std::ostream&
operator<<(std::ostream& os, const UserAgentStatInnerData& data)
  /*throw(eh::Exception)*/
{
  os << data.requests_ << '\t';
  os << data.holder_->channels << '\t';
  os << data.holder_->platforms;
  return os;
}

} // namespace LogProcessing
} // namespace AdServer

