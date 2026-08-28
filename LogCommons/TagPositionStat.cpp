
#include "TagPositionStat.hpp"
#include <LogCommons/BufferWriter.hpp>
#include <LogCommons/LogCommons.ipp>

namespace AdServer::LogProcessing
{

  template <> const char* TagPositionStatTraits::B::base_name_ = "TagPositionStat";
  template <> const char* TagPositionStatTraits::B::signature_ = "TagPositionStat";
  template <> const char* TagPositionStatTraits::B::current_version_ = "2.8";

  std::istream&
  operator>>(std::istream& is, TagPositionStatKey& key)
    /*throw(eh::Exception)*/
  {
    is >> key.sdate_;
    read_eol(is);
    is >> key.colo_id_;
    key.calc_hash_();
    return is;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const TagPositionStatKey& key)
    /*throw(eh::Exception)*/
  {
    out << key.sdate_ << '\n' << key.colo_id_;
    return out;
  }

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, TagPositionStatInnerKey& key)
    /*throw(eh::Exception)*/
  {
    is >> key.tag_id_;
    is >> key.top_offset_;
    is >> key.left_offset_;
    is >> key.visibility_;
    is >> key.test_;
    key.calc_hash_();
    return is;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const TagPositionStatInnerKey& key)
    /*throw(eh::Exception)*/
  {
    out << key.tag_id_ << '\t'
      << key.top_offset_ << '\t'
      << key.left_offset_ << '\t' << key.visibility_ << '\t' << key.test_;
    return out;
  }

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, TagPositionStatInnerData& data)
    /*throw(eh::Exception)*/
  {
    is >> data.requests_;
    is >> data.imps_;
    is >> data.clicks_;
    return is;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const TagPositionStatInnerData& data)
    /*throw(eh::Exception)*/
  {
    out << data.requests_ << '\t' << data.imps_ << '\t' << data.clicks_;
    return out;
  }

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, TagPositionStatInnerKey_V_2_7& key)
    /*throw(eh::Exception)*/
  {
    is >> key.tag_id_;
    is >> key.top_offset_;
    is >> key.left_offset_;
    is >> key.visibility_;
    key.calc_hash_();
    return is;
  }

} // namespace AdServer::LogProcessing
