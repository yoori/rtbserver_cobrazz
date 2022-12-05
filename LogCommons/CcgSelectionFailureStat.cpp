
#include "CcgSelectionFailureStat.hpp"
#include <LogCommons/LogCommons.ipp>

namespace AdServer {
namespace LogProcessing {

template <> const char* CcgSelectionFailureStatTraits::B::base_name_ =
  "CCGSelectionFailureStat";
template <> const char* CcgSelectionFailureStatTraits::B::signature_ =
  "CCGSelectionFailureStat";
template <> const char* CcgSelectionFailureStatTraits::B::current_version_ =
  "3.1";

FixedBufStream<TabCategory>&
operator>>(
  FixedBufStream<TabCategory>& is,
  CcgSelectionFailureStatInnerKey& key
)
  /*throw(eh::Exception)*/
{
  is >> key.ccg_id_;
  is >> key.combination_mask_;
  key.calc_hash_();
  return is;
}

std::ostream&
operator<<(std::ostream& os, const CcgSelectionFailureStatInnerKey& key)
  /*throw(eh::Exception)*/
{
  os << key.ccg_id_ << '\t';
  os << key.combination_mask_;
  return os;
}

FixedBufStream<TabCategory>&
operator>>(
  FixedBufStream<TabCategory>& is,
  CcgSelectionFailureStatInnerData& data
)
  /*throw(eh::Exception)*/
{
  is >> data.requests_;
  return is;
}

std::ostream&
operator<<(std::ostream& os, const CcgSelectionFailureStatInnerData& data)
  /*throw(eh::Exception)*/
{
  os << data.requests_;
  return os;
}

} // namespace LogProcessing
} // namespace AdServer

