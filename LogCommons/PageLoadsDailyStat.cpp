
#include "PageLoadsDailyStat.hpp"
#include <LogCommons/LogCommons.ipp>

namespace AdServer {
namespace LogProcessing {

template <> const char* PageLoadsDailyStatTraits::B::base_name_ =
  "PageLoadsDailyStat";
template <> const char* PageLoadsDailyStatTraits::B::signature_ =
  "PageLoadsDailyStat";
template <> const char* PageLoadsDailyStatTraits::B::current_version_ =
  "3.3";

std::istream&
operator>>(std::istream& is, PageLoadsDailyStatKey& key)
{
  is >> key.sdate_;
  read_eol(is);
  is >> key.colo_id_;
  if (is)
  {
    key.invariant();
    key.calc_hash_();
  }
  return is;
}

std::ostream&
operator<<(std::ostream& os, const PageLoadsDailyStatKey& key)
  /*throw(eh::Exception)*/
{
  key.invariant();
  os << key.sdate_ << '\n' << key.colo_id_;
  return os;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, PageLoadsDailyStatInnerKey& key)
  /*throw(eh::Exception)*/
{
  key.holder_ = new PageLoadsDailyStatInnerKey::DataHolder();
  is >> key.holder_->site_id;
  is >> key.holder_->country;
  is >> key.holder_->tags;
  if (is)
  {
    key.invariant();
    key.calc_hash_();
  }
  return is;
}

std::ostream&
operator<<(std::ostream& os, const PageLoadsDailyStatInnerKey& key)
  /*throw(eh::Exception)*/
{
  key.invariant();
  os << key.holder_->site_id << '\t';
  os << key.holder_->country << '\t';
  os << key.holder_->tags;
  return os;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, PageLoadsDailyStatInnerData& data)
  /*throw(eh::Exception)*/
{
  is >> data.page_loads_;
  is >> data.utilized_page_loads_;
  return is;
}

std::ostream&
operator<<(std::ostream& os, const PageLoadsDailyStatInnerData& data)
  /*throw(eh::Exception)*/
{
  os << data.page_loads_ << '\t';
  os << data.utilized_page_loads_;
  return os;
}

std::istream&
operator>>(std::istream& is, PageLoadsDailyStatKey_V_1_0& key)
{
  is >> key.sdate_;
  read_eol(is);
  is >> key.colo_id_;
  if (is)
  {
    key.invariant();
    key.calc_hash_();
  }
  return is;
}

} // namespace LogProcessing
} // namespace AdServer

