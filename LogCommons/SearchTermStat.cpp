
#include "SearchTermStat.hpp"
#include <LogCommons/BufferWriter.hpp>
#include <LogCommons/LogCommons.ipp>

namespace AdServer::LogProcessing
{

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

  BufferWriter&
  operator<<(BufferWriter& out, const SearchTermStatKey& key)
    /*throw(eh::Exception)*/
  {
    out << key.sdate_ << '\n' << key.colo_id_;
    return out;
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

  BufferWriter&
  operator<<(BufferWriter& out, const SearchTermStatInnerKey& key)
    /*throw(eh::Exception)*/
  {
    key.invariant_();
    out << key.search_term_;
    return out;
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

  BufferWriter&
  operator<<(BufferWriter& out, const SearchTermStatInnerData& data)
    /*throw(eh::Exception)*/
  {
    out << data.hits_;
    return out;
  }

} // namespace AdServer::LogProcessing
