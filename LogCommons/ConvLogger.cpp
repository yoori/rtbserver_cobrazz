
#include <LogCommons/LogCommons.hpp>
#include "ConvLogger.hpp"

namespace AdServer
{
namespace LogProcessing
{
  template <> const char*
  ConvTraits::B::base_name_ = "Conv";

  template <> const char*
  ConvTraits::B::signature_ = "Conv";

  template <> const char*
  ConvTraits::B::current_version_ = "3.6";

  FixedBufStream<TabCategory>&
    operator>>(FixedBufStream<TabCategory>& is,
      ConvData& data)
    /*throw(eh::Exception)*/
  {
    is >> data.time_;
    is >> data.user_id_;
    is >> data.action_id_;
    return is;
  }

  std::ostream&
  operator<<(std::ostream& os, const ConvData& data)
    /*throw(eh::Exception)*/
  {
    os << data.time_ << '\t';
    os << data.user_id_ << '\t';
    os << data.action_id_ << '\t';
    return os;
  }
} // namespace LogProcessing
} // namespace AdServer

