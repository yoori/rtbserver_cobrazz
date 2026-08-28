
#include "CcgUserStat.hpp"
#include <LogCommons/BufferWriter.hpp>
#include <LogCommons/LogCommons.ipp>

namespace AdServer::LogProcessing
{

  template <> const char* CcgUserStatTraits::B::base_name_ = "CCGUserStat";
  template <> const char* CcgUserStatTraits::B::signature_ = "CCGUserStat";
  template <> const char* CcgUserStatTraits::B::current_version_ = "2.5";

  std::istream&
  operator>>(std::istream& is, CcgUserStatKey& key)
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
  operator<<(BufferWriter& out, const CcgUserStatKey& key)
    /*throw(eh::Exception)*/
  {
    key.invariant();
    out << key.adv_sdate_ << '\n' << key.colo_id_;
    return out;
  }

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, CcgUserStatInnerKey& key)
    /*throw(eh::Exception)*/
  {
    is >> key.ccg_id_;
    is >> key.last_appearance_date_;
    if (is)
    {
      key.invariant();
      key.calc_hash_();
    }
    return is;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const CcgUserStatInnerKey& key)
    /*throw(eh::Exception)*/
  {
    key.invariant();
    out << key.ccg_id_ << '\t';
    out << key.last_appearance_date_;
    return out;
  }

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, CcgUserStatInnerData& data)
    /*throw(eh::Exception)*/
  {
    is >> data.unique_users_;
    return is;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const CcgUserStatInnerData& data)
    /*throw(eh::Exception)*/
  {
    out << data.unique_users_;
    return out;
  }

} // namespace AdServer::LogProcessing
