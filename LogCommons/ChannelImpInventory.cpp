
#include "ChannelImpInventory.hpp"
#include <LogCommons/BufferWriter.hpp>
#include <LogCommons/LogCommons.ipp>

namespace AdServer::LogProcessing
{

  template <> const char*
  ChannelImpInventoryTraits::B::base_name_ = "ChannelImpInventory";

  template <> const char*
  ChannelImpInventoryTraits::B::signature_ = "ChannelImpInventory";

  template <> const char*
  ChannelImpInventoryTraits::B::current_version_ = "1.0";

  std::istream&
  operator>>(std::istream& is, ChannelImpInventoryKey& key)
  {
    is >> key.sdate_;
    read_eol(is);
    is >> key.colo_id_;
    key.calc_hash_();
    return is;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const ChannelImpInventoryKey& key)
    /*throw(eh::Exception)*/
  {
    out << key.sdate_ << '\n' << key.colo_id_;
    return out;
  }

  inline
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, ChannelImpInventoryData::DataT::Counter& cntr)
    /*throw(eh::Exception)*/
  {
    is >> cntr.imps;
    is >> cntr.user_count;
    is >> cntr.value;
    return is;
  }

  inline
  BufferWriter&
  operator<<(BufferWriter& out, const ChannelImpInventoryData::DataT::Counter& cntr)
    /*throw(eh::Exception)*/
  {
    out << cntr.imps << '\t';
    out << cntr.user_count << '\t';
    out << cntr.value;
    return out;
  }

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, ChannelImpInventoryInnerKey& key)
    /*throw(eh::Exception)*/
  {
    is >> key.channel_id_;
    char ccg_type = '\0';
    is >> ccg_type;
    key.ccg_type_ = ChannelImpInventoryInnerKey::char_to_ccg_type_(ccg_type);

    if (is)
    {
      key.calc_hash_();
    }

    return is;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const ChannelImpInventoryInnerKey& key)
    /*throw(eh::Exception)*/
  {
    out << key.channel_id_ << '\t';
    out << key.ccg_type();
    return out;
  }

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, ChannelImpInventoryInnerData& data)
    /*throw(eh::Exception)*/
  {
    TokenizerInputArchive<Aux_::NoInvariants> ia(is);
    ia >> data;
    data.meaningful_data_ = true;
    return is;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const ChannelImpInventoryInnerData& data)
    /*throw(eh::Exception)*/
  {
    SimpleBufferTabOutputArchive oa(out);
    oa << data;
    return out;
  }

} // namespace AdServer::LogProcessing
