
#include "CampaignUserStat.hpp"
#include <LogCommons/BufferWriter.hpp>
#include <LogCommons/LogCommons.ipp>

namespace AdServer::LogProcessing
{

  template <> const char*
  CampaignUserStatTraits::B::base_name_ = "CampaignUserStat";

  template <> const char*
  CampaignUserStatTraits::B::signature_ = "CampaignUserStat";

  template <> const char*
  CampaignUserStatTraits::B::current_version_ = "2.5";

  std::istream&
  operator>>(std::istream& is, CampaignUserStatKey& key)
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
  operator<<(BufferWriter& out, const CampaignUserStatKey& key)
  {
    key.invariant();
    out << key.adv_sdate_ << '\n' << key.colo_id_;
    return out;
  }

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, CampaignUserStatInnerKey& key)
  {
    is >> key.cmp_id_;
    is >> key.last_appearance_date_;
    if (is)
    {
      key.invariant();
      key.calc_hash_();
    }
    return is;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const CampaignUserStatInnerKey& key)
  {
    key.invariant();
    out << key.cmp_id_ << '\t';
    out << key.last_appearance_date_;
    return out;
  }

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, CampaignUserStatInnerData& data)
  {
    is >> data.unique_users_;
    return is;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const CampaignUserStatInnerData& data)
  {
    out << data.unique_users_;
    return out;
  }

} // namespace AdServer::LogProcessing
