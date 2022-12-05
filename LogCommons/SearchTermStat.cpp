
#include "SearchTermStat.hpp"
#include <LogCommons/LogCommons.ipp>

namespace AdServer {
namespace LogProcessing {

template <> const char*
SearchTermStatTraits::B::base_name_ = "SearchTermStat";

template <> const char*
SearchTermStatTraits::B::signature_ = "SearchTermStat";

template <> const char*
SearchTermStatTraits::B::current_version_ = "2.5";

std::istream&
operator>>(std::istream& is, SearchTermStatKey& key)
  /*throw(eh::Exception)*/
{
  is >> key.sdate_;
  read_eol(is);
  is >> key.colo_id_;
  key.calc_hash_();
  return is;
}

std::ostream&
operator<<(std::ostream& os, const SearchTermStatKey& key)
  /*throw(eh::Exception)*/
{
  os << key.sdate_ << '\n' << key.colo_id_;
  return os;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, SearchTermStatInnerKey& key)
  /*throw(eh::Exception)*/
{
  is >> key.search_term_;
  key.invariant_();
  key.calc_hash_();
  return is;
}

std::ostream&
operator<<(std::ostream& os, const SearchTermStatInnerKey& key)
  /*throw(eh::Exception)*/
{
  key.invariant_();
  os << key.search_term_;
  return os;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, SearchTermStatInnerData& data)
  /*throw(eh::Exception)*/
{
  is >> data.hits_;
  return is;
}

std::ostream&
operator<<(std::ostream& os, const SearchTermStatInnerData& data)
  /*throw(eh::Exception)*/
{
  os << data.hits_;
  return os;
}

} // namespace LogProcessing
} // namespace AdServer

