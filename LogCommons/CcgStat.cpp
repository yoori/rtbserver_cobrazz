
#include "CcgStat.hpp"
#include <LogCommons/CollectorBundle.hpp>
#include <LogCommons/LogCommons.ipp>

namespace AdServer {
namespace LogProcessing {

template <> const char* CcgStatTraits::B::base_name_ = "CCGStat";
template <> const char* CcgStatTraits::B::signature_ = "CCGStat";
template <> const char* CcgStatTraits::B::current_version_ = "2.6";

template <> const char* CcStatTraits::B::base_name_ = "CCStat";
template <> const char* CcStatTraits::B::signature_ = "CCStat";
template <> const char* CcStatTraits::B::current_version_ = "2.6";

template <> const char* CampaignStatTraits::B::base_name_ =
  "CampaignStat";
template <> const char* CampaignStatTraits::B::signature_ =
  "CampaignStat";
template <> const char* CampaignStatTraits::B::current_version_ =
  "1.1"; // Last change: AdServer v2.1

template <> const char* AdvertiserUserStatTraits::B::base_name_ =
  "AdvertiserUserStat";
template <> const char* AdvertiserUserStatTraits::B::signature_ =
  "AdvertiserUserStat";
template <> const char* AdvertiserUserStatTraits::B::current_version_ =
  "2.5";

std::istream&
operator>>(std::istream& is, ReachStatKey& key)
{
  is >> key.sdate_;
  read_eol(is);
  is >> key.colo_id_;
  key.calc_hash_();
  return is;
}

std::ostream&
operator<<(std::ostream& os, const ReachStatKey& key)
{
  os << key.sdate_ << '\n' << key.colo_id_;
  return os;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, ReachStatInnerKey& key)
{
  is >> key.id_;
  return is;
}

std::ostream&
operator<<(std::ostream& os, const ReachStatInnerKey& key)
{
  os << key.id_;
  return os;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, ReachStatInnerData& data)
{
  is >> data.daily_reach_;
  is >> data.monthly_reach_;
  is >> data.total_reach_;
  return is;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, CcgStatInnerData& data)
{
  is >> data.auctions_lost_;
  return is;
}

std::ostream&
operator<<(std::ostream& os, const CcgStatInnerData& data)
{
  os << data.auctions_lost_;
  return os;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, CcgStatInnerData_V_1_2& data)
{
  is >> static_cast<ReachStatInnerData&>(data);
  is >> data.auctions_lost_;
  return is;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, CcgStatInnerData_V_2_3& data)
{
  is >> static_cast<ReachStatInnerData&>(data);
  is >> data.impops_;
  is >> data.auctions_lost_;
  return is;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, CcStatInnerData_V_1_2& data)
{
  is >> static_cast<ReachStatInnerData&>(data);
  is >> data.auctions_lost_;
  return is;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, CcStatInnerData_V_2_3& data)
{
  is >> static_cast<ReachStatInnerData&>(data);
  is >> data.impops_;
  is >> data.auctions_lost_;
  return is;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, CcStatInnerData& data)
{
  is >> data.auctions_lost_;
  return is;
}

std::ostream&
operator<<(std::ostream& os, const CcStatInnerData& data)
{
  os << data.auctions_lost_;
  return os;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, AdvertiserUserStatInnerKey& key)
{
  is >> key.adv_account_id_;
  is >> key.last_appearance_date_;
  key.calc_hash_();
  return is;
}

std::ostream&
operator<<(std::ostream& os, const AdvertiserUserStatInnerKey& key)
{
  os << key.adv_account_id_ << '\t';
  os << key.last_appearance_date_;
  return os;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, AdvertiserUserStatInnerData& data)
{
  is >> data.unique_users_;
  is >> data.text_unique_users_;
  is >> data.display_unique_users_;
  return is;
}

std::ostream&
operator<<(std::ostream& os, const AdvertiserUserStatInnerData& data)
{
  os << data.unique_users_ << '\t';
  os << data.text_unique_users_ << '\t';
  os << data.display_unique_users_;
  return os;
}

FixedBufStream<TabCategory>&
operator>>(
  FixedBufStream<TabCategory>& is,
  AdvertiserUserStatInnerData_V_1_0& data
)
{
  is >> data.reach_;
  is >> data.text_reach_;
  is >> data.display_reach_;
  return is;
}

void
AdvertiserUserStat_V_1_0_To_CurrentLoader::load(
  std::istream& is,
  const CollectorBundleFileGuard_var& file_handle
)
  /*throw(Exception)*/
{
  try
  {
    CollectorT collector;
    CollectorT::KeyT key;
    OldCollectorT::DataT old_data;
    is >> key;
    read_eol(is);
    is >> old_data;
    if (is.eof())
    {
      CollectorT::DataT data;
      for (OldCollectorT::DataT::const_iterator it = old_data.begin();
        it != old_data.end(); ++it)
      {
        // Reach conversion
        {
          const OldCollectorT::DataT::DataT::ReachRecord& reach =
            it->second.reach();
          if (reach.daily_reach() < reach.monthly_reach())
          {
            Stream::Error es;
            es << __PRETTY_FUNCTION__ << ": AdvertiserUserStat_V_1_0::"
              "reach::daily_reach is less than "
              "AdvertiserUserStat_V_1_0::reach::monthly_reach";
            throw Exception(es);
          }
          unsigned long reach_diff =
            reach.daily_reach() - reach.monthly_reach();
          if (reach_diff)
          {
            CollectorT::DataT::KeyT
              inner_key(it->first.adv_account_id(), key.time() -= ONE_DAY);
            data.add(inner_key, CollectorT::DataT::DataT(reach_diff, 0, 0));
          }
          if (reach.monthly_reach() < reach.total_reach())
          {
            Stream::Error es;
            es << __PRETTY_FUNCTION__ << ": AdvertiserUserStat_V_1_0::"
              "reach::monthly_reach is less than "
              "AdvertiserUserStat_V_1_0::reach::total_reach";
            throw Exception(es);
          }
          reach_diff = reach.monthly_reach() - reach.total_reach();
          if (reach_diff)
          {
            CollectorT::DataT::KeyT
              inner_key(it->first.adv_account_id(), key.time() -= ONE_MONTH);
            data.add(inner_key, CollectorT::DataT::DataT(reach_diff, 0, 0));
          }
          if (reach.total_reach())
          {
            CollectorT::DataT::KeyT
              inner_key(it->first.adv_account_id(), OptionalDayTimestamp());
            data.add(inner_key,
              CollectorT::DataT::DataT(reach.total_reach(), 0, 0));
          }
        }
        // Text reach conversion
        {
          const OldCollectorT::DataT::DataT::ReachRecord& reach =
            it->second.text_reach();
          if (reach.daily_reach() < reach.monthly_reach())
          {
            Stream::Error es;
            es << __PRETTY_FUNCTION__ << ": AdvertiserUserStat_V_1_0::"
              "text_reach::daily_reach is less than "
              "AdvertiserUserStat_V_1_0::text_reach::monthly_reach";
            throw Exception(es);
          }
          unsigned long reach_diff =
            reach.daily_reach() - reach.monthly_reach();
          if (reach_diff)
          {
            CollectorT::DataT::KeyT
              inner_key(it->first.adv_account_id(), key.time() -= ONE_DAY);
            data.add(inner_key, CollectorT::DataT::DataT(0, reach_diff, 0));
          }
          if (reach.monthly_reach() < reach.total_reach())
          {
            Stream::Error es;
            es << __PRETTY_FUNCTION__ << ": AdvertiserUserStat_V_1_0::"
              "text_reach::monthly_reach is less than "
              "AdvertiserUserStat_V_1_0::text_reach::total_reach";
            throw Exception(es);
          }
          reach_diff = reach.monthly_reach() - reach.total_reach();
          if (reach_diff)
          {
            CollectorT::DataT::KeyT
              inner_key(it->first.adv_account_id(), key.time() -= ONE_MONTH);
            data.add(inner_key, CollectorT::DataT::DataT(0, reach_diff, 0));
          }
          if (reach.total_reach())
          {
            CollectorT::DataT::KeyT
              inner_key(it->first.adv_account_id(), OptionalDayTimestamp());
            data.add(inner_key,
              CollectorT::DataT::DataT(0, reach.total_reach(), 0));
          }
        }
        // Display reach conversion
        {
          const OldCollectorT::DataT::DataT::ReachRecord& reach =
            it->second.display_reach();
          if (reach.daily_reach() < reach.monthly_reach())
          {
            Stream::Error es;
            es << __PRETTY_FUNCTION__ << ": AdvertiserUserStat_V_1_0::"
              "display_reach::daily_reach is less than "
              "AdvertiserUserStat_V_1_0::display_reach::monthly_reach";
            throw Exception(es);
          }
          unsigned long reach_diff =
            reach.daily_reach() - reach.monthly_reach();
          if (reach_diff)
          {
            CollectorT::DataT::KeyT
              inner_key(it->first.adv_account_id(), key.time() -= ONE_DAY);
            data.add(inner_key, CollectorT::DataT::DataT(0, 0, reach_diff));
          }
          if (reach.monthly_reach() < reach.total_reach())
          {
            Stream::Error es;
            es << __PRETTY_FUNCTION__ << ": AdvertiserUserStat_V_1_0::"
              "display_reach::monthly_reach is less than "
              "AdvertiserUserStat_V_1_0::display_reach::total_reach";
            throw Exception(es);
          }
          reach_diff = reach.monthly_reach() - reach.total_reach();
          if (reach_diff)
          {
            CollectorT::DataT::KeyT
              inner_key(it->first.adv_account_id(), key.time() -= ONE_MONTH);
            data.add(inner_key, CollectorT::DataT::DataT(0, 0, reach_diff));
          }
          if (reach.total_reach())
          {
            CollectorT::DataT::KeyT
              inner_key(it->first.adv_account_id(), OptionalDayTimestamp());
            data.add(inner_key,
              CollectorT::DataT::DataT(0, 0, reach.total_reach()));
          }
        }
      }
      collector.add(key, data);
    }
    bundle_->merge(collector, file_handle);
  }
  catch (const Exception&)
  {
    throw;
  }
  catch (const eh::Exception& ex)
  {
    Stream::Error es;
    es << __PRETTY_FUNCTION__ << ": Caught eh::Exception. "
       << ": " << ex.what();
    throw Exception(es);
  }
  catch (...)
  {
    Stream::Error es;
    es << __PRETTY_FUNCTION__ << ": Caught unknown exception.";
    throw Exception(es);
  }
  if (!is.eof())
  {
    Stream::Error es;
    es << __PRETTY_FUNCTION__ << ": Error: Malformed log file "
       << "(extra data at the of file)";
    throw Exception(es);
  }
}

} // namespace LogProcessing
} // namespace AdServer

