
#include "ColoUpdateStat.hpp"
#include <LogCommons/LogCommons.ipp>

namespace AdServer {
namespace LogProcessing {

template <> const char* ColoUpdateStatTraits::B::base_name_ = "ColoUpdateStat";
template <> const char* ColoUpdateStatTraits::B::signature_ = "ColoUpdateStat";
template <> const char* ColoUpdateStatTraits::B::current_version_ = "1.0";

FixedBufStream<TabCategory>&
operator>>(
  FixedBufStream<TabCategory>& is,
  ColoUpdateStatData::Version& version
)
  /*throw(eh::Exception)*/
{
  is >> static_cast<StringIoWrapperOptional&>(version);
  version.parse_();
  return is;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, ColoUpdateStatKey& key)
{
  is >> key.colo_id_;
  return is;
}

std::ostream&
operator<<(std::ostream& os, const ColoUpdateStatKey& key)
  /*throw(eh::Exception)*/
{
  os << key.colo_id_;
  return os;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, ColoUpdateStatData& data)
{
  is >> data.last_channels_update_;
  is >> data.last_campaigns_update_;
  is >> data.version_;
  return is;
}

std::ostream&
operator<<(std::ostream& os, const ColoUpdateStatData& data)
{
  os << data.last_channels_update_ << '\t';
  os << data.last_campaigns_update_ << '\t';
  os << data.version_;
  return os;
}

} // namespace LogProcessing
} // namespace AdServer

