
#include "KeywordHitStat.hpp"
#include <LogCommons/LogCommons.ipp>

namespace AdServer
{
namespace LogProcessing
{
  template <> const char*
  KeywordHitStatTraits::B::base_name_ = "KeywordHitStat";

  template <> const char*
  KeywordHitStatTraits::B::signature_ = "KeywordHitStat";

  template <> const char*
  KeywordHitStatTraits::B::current_version_ = "3.5";

  std::istream&
  operator>>(std::istream& is, KeywordHitStatKey& key)
  {
    is >> key.sdate_;
    key.calc_hash_();
    return is;
  }

  std::ostream&
  operator<<(std::ostream& os, const KeywordHitStatKey& key)
    /*throw(eh::Exception)*/
  {
    os << key.sdate_;
    return os;
  }

  std::istream&
  operator>>(std::istream& is, KeywordHitStatInnerKey& key)
  {
    Aux_::StringIoWrapper keyword_wrapper;
    is >> keyword_wrapper;
    key.keyword_ = new AdServer::Commons::StringHolder(
      std::move(keyword_wrapper));
    key.calc_hash_();
    return is;
  }

  std::ostream&
  operator<<(std::ostream& os, const KeywordHitStatInnerKey& key)
  {
    os << Aux_::StringIoWrapper(key.keyword_->str());
    return os;
  }

  std::istream&
  operator>>(std::istream& is, KeywordHitStatInnerData& data)
  {
    is >> data.hits_;
    return is;
  }

  std::ostream&
  operator<<(std::ostream& os, const KeywordHitStatInnerData& data)
  {
    os << data.hits_;
    return os;
  }
} // namespace LogProcessing
} // namespace AdServer

