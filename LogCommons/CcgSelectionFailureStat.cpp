
#include "CcgSelectionFailureStat.hpp"
#include <LogCommons/BufferWriter.hpp>
#include <LogCommons/LogCommons.ipp>

namespace AdServer::LogProcessing
{

  template <> const char* CcgSelectionFailureStatTraits::B::base_name_ = "CCGSelectionFailureStat";
  template <> const char* CcgSelectionFailureStatTraits::B::signature_ = "CCGSelectionFailureStat";
  template <> const char* CcgSelectionFailureStatTraits::B::current_version_ = "3.1";

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, CcgSelectionFailureStatInnerKey& key)
    /*throw(eh::Exception)*/
  {
    is >> key.ccg_id_;
    is >> key.combination_mask_;
    key.calc_hash_();
    return is;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const CcgSelectionFailureStatInnerKey& key)
    /*throw(eh::Exception)*/
  {
    out << key.ccg_id_ << '\t';
    out << key.combination_mask_;
    return out;
  }

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, CcgSelectionFailureStatInnerData& data)
    /*throw(eh::Exception)*/
  {
    is >> data.requests_;
    return is;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const CcgSelectionFailureStatInnerData& data)
    /*throw(eh::Exception)*/
  {
    out << data.requests_;
    return out;
  }

} // namespace AdServer::LogProcessing
