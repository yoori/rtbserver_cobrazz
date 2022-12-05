
#include "SearchEngineStat.hpp"
#include <LogCommons/LogCommons.ipp>

namespace AdServer {
namespace LogProcessing {

template <> const char*
SearchEngineStatTraits::B::base_name_ = "SearchEngineStat";

template <> const char*
SearchEngineStatTraits::B::signature_ = "SearchEngineStat";

template <> const char* SearchEngineStatTraits::B::current_version_ = "2.5";

std::istream&
operator>>(std::istream& is, SearchEngineStatKey& key)
  /*throw(eh::Exception)*/
{
  is >> key.sdate_;
  read_eol(is);
  is >> key.colo_id_;
  key.calc_hash_();
  return is;
}

std::ostream&
operator<<(std::ostream& os, const SearchEngineStatKey& key)
  /*throw(eh::Exception)*/
{
  os << key.sdate_ << '\n' << key.colo_id_;
  return os;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, SearchEngineStatInnerKey& key)
  /*throw(eh::Exception)*/
{
  is >> key.search_engine_id_;
  is >> key.host_name_;
  key.invariant_();
  key.calc_hash_();
  return is;
}

std::ostream&
operator<<(std::ostream& os, const SearchEngineStatInnerKey& key)
  /*throw(eh::Exception)*/
{
  key.invariant_();
  os << key.search_engine_id_ << '\t';
  os << key.host_name_;
  return os;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, SearchEngineStatInnerData& data)
  /*throw(eh::Exception)*/
{
  is >> data.hits_;
  is >> data.hits_empty_page_;
  return is;
}

std::ostream&
operator<<(std::ostream& os, const SearchEngineStatInnerData& data)
  /*throw(eh::Exception)*/
{
  os << data.hits_ << '\t';
  os << data.hits_empty_page_;
  return os;
}

} // namespace LogProcessing
} // namespace AdServer

