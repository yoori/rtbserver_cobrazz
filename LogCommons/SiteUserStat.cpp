
#include "SiteUserStat.hpp"
#include <LogCommons/CollectorBundle.hpp>
#include <LogCommons/LogCommons.ipp>

namespace AdServer {
namespace LogProcessing {

template <> const char* SiteUserStatTraits::B::base_name_ = "SiteUserStat";
template <> const char* SiteUserStatTraits::B::signature_ = "SiteUserStat";
template <> const char* SiteUserStatTraits::B::current_version_ = "2.5";

std::istream&
operator>>(std::istream& is, SiteUserStatKey& key)
{
  is >> key.isp_sdate_;
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
operator<<(std::ostream& os, const SiteUserStatKey& key)
{
  key.invariant();
  os << key.isp_sdate_ << '\n' << key.colo_id_;
  return os;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, SiteUserStatInnerKey& key)
{
  is >> key.site_id_;
  is >> key.last_appearance_date_;
  if (is)
  {
    key.invariant();
    key.calc_hash_();
  }
  return is;
}

std::ostream&
operator<<(std::ostream& os, const SiteUserStatInnerKey& key)
{
  key.invariant();
  os << key.site_id_ << '\t';
  os << key.last_appearance_date_;
  return os;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, SiteUserStatInnerData& data)
{
  is >> data.unique_users_;
  return is;
}

std::ostream&
operator<<(std::ostream& os, const SiteUserStatInnerData& data)
{
  os << data.unique_users_;
  return os;
}

void
SiteStatToSiteUserStatLoader::load(
  std::istream& is,
  const CollectorBundleFileGuard_var& file_handle
)
  /*throw(Exception)*/
{
  try
  {
    CollectorT collector;
    OldCollectorT::KeyT old_key;
    OldCollectorT::DataT old_data;
    is >> old_key;
    read_eol(is);
    is >> old_data;
    if (is.eof())
    {
      CollectorT::KeyT key(old_key.isp_sdate(), old_key.colo_id());
      CollectorT::DataT data;
      for (OldCollectorT::DataT::const_iterator it = old_data.begin();
        it != old_data.end(); ++it)
      {
        if (it->second.daily_reach() < it->second.monthly_reach())
        {
          Stream::Error es;
          es << __PRETTY_FUNCTION__ << ": SiteStat::daily_reach is less than "
            "SiteStat::monthly_reach";
          throw Exception(es);
        }
        unsigned long reach_diff =
          it->second.daily_reach() - it->second.monthly_reach();
        if (reach_diff)
        {
          CollectorT::DataT::KeyT
            inner_key(it->first.site_id(),
              old_key.isp_sdate().time() -= ONE_DAY);
          data.add(inner_key, CollectorT::DataT::DataT(reach_diff));
        }
        if (it->second.monthly_reach())
        {
          CollectorT::DataT::KeyT
            inner_key(it->first.site_id(),
              old_key.isp_sdate().time() -= ONE_MONTH);
          data.add(inner_key,
            CollectorT::DataT::DataT(it->second.monthly_reach()));
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

