
#include "ChannelInventoryEstimationStat.hpp"
#include <LogCommons/BufferWriter.hpp>
#include <LogCommons/LogCommons.ipp>

namespace AdServer::LogProcessing
{

  template <>
  const char* ChannelInventoryEstimationStatTraits::B::base_name_ =
    "ChannelInventoryEstimationStat";
  template <>
  const char* ChannelInventoryEstimationStatTraits::B::signature_ =
    "ChannelInventoryEstimationStat";
  template <>
  const char* ChannelInventoryEstimationStatTraits::B::current_version_ =
    "1.1"; // Last change: AdServer v2.2

  const ChannelInventoryEstimationStatInnerKey::LevelT
    ChannelInventoryEstimationStatInnerKey::max_level_value_("2.0");

  std::istream&
  operator>>(std::istream& is, ChannelInventoryEstimationStatKey& key)
  {
    is >> key.sdate_;
    read_eol(is);
    is >> key.colo_id_;
    key.calc_hash_();
    return is;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const ChannelInventoryEstimationStatKey& key)
    /*throw(eh::Exception)*/
  {
    out << key.sdate_ << '\n' << key.colo_id_;
    return out;
  }

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, ChannelInventoryEstimationStatInnerKey& key)
  {
    is >> key.channel_id_;
    is >> key.level_;
    if (is)
    {
      key.invariant();
      key.calc_hash_();
    }
    return is;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const ChannelInventoryEstimationStatInnerKey& key)
  {
    key.invariant();
    out << key.channel_id_ << '\t';
    out << key.level_;
    return out;
  }

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, ChannelInventoryEstimationStatInnerData& data)
  {
    is >> data.users_regular_;
    is >> data.users_from_now_;
    return is;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const ChannelInventoryEstimationStatInnerData& data)
  {
    out << data.users_regular_ << '\t';
    out << data.users_from_now_;
    return out;
  }

} // namespace AdServer::LogProcessing
