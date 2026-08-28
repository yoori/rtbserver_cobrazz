
#include "ColoUpdateStat.hpp"
#include <LogCommons/BufferWriter.hpp>
#include <LogCommons/LogCommons.ipp>

namespace AdServer::LogProcessing
{

  template <> const char* ColoUpdateStatTraits::B::base_name_ = "ColoUpdateStat";
  template <> const char* ColoUpdateStatTraits::B::signature_ = "ColoUpdateStat";
  template <> const char* ColoUpdateStatTraits::B::current_version_ = "1.0";

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, ColoUpdateStatData::Version& version)
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

  BufferWriter&
  operator<<(BufferWriter& out, const ColoUpdateStatKey& key)
    /*throw(eh::Exception)*/
  {
    out << key.colo_id_;
    return out;
  }

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, ColoUpdateStatData& data)
  {
    is >> data.last_channels_update_;
    is >> data.last_campaigns_update_;
    is >> data.version_;
    return is;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const ColoUpdateStatData& data)
  {
    out << data.last_channels_update_ << '\t';
    out << data.last_campaigns_update_ << '\t';
    out << static_cast<const StringIoWrapperOptional&>(data.version_);
    return out;
  }

} // namespace AdServer::LogProcessing
