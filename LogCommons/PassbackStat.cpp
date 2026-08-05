
#include "PassbackStat.hpp"
#include <LogCommons/BufferWriter.hpp>
#include <LogCommons/LogCommons.ipp>

namespace AdServer {
namespace LogProcessing {

template <>
const char* PassbackStatTraits::B::base_name_ = "PassbackStat";
template <>
const char* PassbackStatTraits::B::signature_ = "PassbackStat";
template <>
const char* PassbackStatTraits::B::current_version_ = "3.3";

std::istream&
operator>>(std::istream& is, PassbackStatKey& key)
{
  is >> key.sdate_;
  read_eol(is);
  is >> key.colo_id_;
  key.calc_hash_();
  return is;
}

std::ostream&
operator<<(std::ostream& os, const PassbackStatKey& key)
  /*throw(eh::Exception)*/
{
  os << key.sdate_ << '\n' << key.colo_id_;
  return os;
}

BufferWriter&
operator<<(BufferWriter& out, const PassbackStatKey& key)
  /*throw(eh::Exception)*/
{
  out << key.sdate_ << '\n' << key.colo_id_;
  return out;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, PassbackStatInnerKey& key)
  /*throw(eh::Exception)*/
{
  is >> key.user_status_;
  is >> key.country_code_;
  is >> key.tag_id_;
  is >> key.size_id_;
  if (is)
  {
    key.invariant();
    key.calc_hash_();
  }
  return is;
}

std::ostream&
operator<<(std::ostream& os, const PassbackStatInnerKey& key)
  /*throw(eh::Exception)*/
{
  key.invariant();
  os << key.user_status_ << '\t';
  os << key.country_code_ << '\t';
  os << key.tag_id_ << '\t';
  os << key.size_id_;
  return os;
}

BufferWriter&
operator<<(BufferWriter& out, const PassbackStatInnerKey& key)
  /*throw(eh::Exception)*/
{
  key.invariant();
  out << key.user_status_ << '\t'
    << key.country_code_ << '\t'
    << key.tag_id_ << '\t'
    << key.size_id_;
  return out;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, PassbackStatInnerKey_V_2_4& key)
  /*throw(eh::Exception)*/
{
  is >> key.user_status_;
  is >> key.country_code_;
  is >> key.tag_id_;
  if (is)
  {
    key.invariant();
    key.calc_hash_();
  }
  return is;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, PassbackStatInnerData& data)
  /*throw(eh::Exception)*/
{
  is >> data.requests_;
  return is;
}

std::ostream&
operator<<(std::ostream& os, const PassbackStatInnerData& data)
  /*throw(eh::Exception)*/
{
  os << data.requests_;
  return os;
}

BufferWriter&
operator<<(BufferWriter& out, const PassbackStatInnerData& data)
  /*throw(eh::Exception)*/
{
  out << data.requests_;
  return out;
}

} // namespace LogProcessing
} // namespace AdServer
