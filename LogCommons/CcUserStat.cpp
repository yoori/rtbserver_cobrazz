
#include "CcUserStat.hpp"
#include <LogCommons/BufferWriter.hpp>
#include <LogCommons/LogCommons.ipp>

namespace AdServer::LogProcessing
{

  template <> const char* CcUserStatTraits::B::base_name_ = "CCUserStat";
  template <> const char* CcUserStatTraits::B::signature_ = "CCUserStat";
  template <> const char* CcUserStatTraits::B::current_version_ = "2.5";

  std::istream&
  operator>>(std::istream& is, CcUserStatKey& key)
  {
    is >> key.adv_sdate_;
    read_eol(is);
    is >> key.colo_id_;
    if (is)
    {
      key.invariant();
      key.calc_hash_();
    }
    return is;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const CcUserStatKey& key)
    /*throw(eh::Exception)*/
  {
    key.invariant();
    out << key.adv_sdate_ << '\n' << key.colo_id_;
    return out;
  }

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, CcUserStatInnerKey& key)
    /*throw(eh::Exception)*/
  {
    is >> key.cc_id_;
    is >> key.last_appearance_date_;
    if (is)
    {
      key.invariant();
      key.calc_hash_();
    }
    return is;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const CcUserStatInnerKey& key)
    /*throw(eh::Exception)*/
  {
    key.invariant();
    out << key.cc_id_ << '\t';
    out << key.last_appearance_date_;
    return out;
  }

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, CcUserStatInnerData& data)
    /*throw(eh::Exception)*/
  {
    is >> data.unique_users_;
    return is;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const CcUserStatInnerData& data)
    /*throw(eh::Exception)*/
  {
    out << data.unique_users_;
    return out;
  }

} // namespace AdServer::LogProcessing
