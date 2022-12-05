
#include "ImpNotify.hpp"
#include <LogCommons/LogCommons.ipp>

namespace AdServer {
namespace LogProcessing {

template <> const char*
ImpNotifyTraits::B::base_name_ = "ImpNotify";
template <> const char*
ImpNotifyTraits::B::signature_ = "ImpNotify";
template <> const char*
ImpNotifyTraits::B::current_version_ = "3.3";

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, ImpNotifyData& data)
  /*throw(eh::Exception)*/
{
  is >> data.time;
  is >> data.user_id;
  is >> data.channels;
  is >> data.revenue;
  if (is)
  {
    data.invariant();
  }
  return is;
}

std::ostream&
operator<<(std::ostream& os, const ImpNotifyData& data)
  /*throw(eh::Exception)*/
{
  data.invariant();
  os << data.time << '\t';
  os << data.user_id << '\t';
  os << data.channels << '\t';
  os << data.revenue;
  return os;
}

} // namespace LogProcessing
} // namespace AdServer

