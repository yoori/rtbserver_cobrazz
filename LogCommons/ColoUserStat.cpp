
#include "ColoUserStat.hpp"
#include <LogCommons/CollectorBundle.hpp>
#include <LogCommons/LogCommons.ipp>

namespace AdServer {
namespace LogProcessing {

template <> const char* ColoUserStatTraits::B::base_name_ =
  "ColoUserStat";
template <> const char* ColoUserStatTraits::B::signature_ =
  "ColoUserStat";
template <> const char* ColoUserStatTraits::B::current_version_ =
  "2.8.1";

template <> const char* GlobalColoUserStatTraits::B::base_name_ =
  "GlobalColoUserStat";
template <> const char* GlobalColoUserStatTraits::B::signature_ =
  "GlobalColoUserStat";
template <> const char* GlobalColoUserStatTraits::B::current_version_ =
  "2.8.1";

std::istream&
operator>>(std::istream& is, ColoUserStatKey& key)
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
operator<<(std::ostream& os, const ColoUserStatKey& key)
  /*throw(eh::Exception)*/
{
  key.invariant();
  os << key.sdate_ << '\n' << key.colo_id_;
  return os;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, ColoUserStatInnerKey& key)
  /*throw(eh::Exception)*/
{
  is >> key.create_date_;
  is >> key.last_appearance_date_;
  if (is)
  {
    key.invariant();
    key.calc_hash_();
  }
  return is;
}

std::ostream&
operator<<(std::ostream& os, const ColoUserStatInnerKey& key)
  /*throw(eh::Exception)*/
{
  key.invariant();
  os << key.create_date_ << '\t';
  os << key.last_appearance_date_;
  return os;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, ColoUserStatInnerKey_V_2_5& key)
  /*throw(eh::Exception)*/
{
  is >> key.create_date_;
  is >> key.last_appearance_date_;
  if (is)
  {
    key.invariant();
    key.calc_hash_();
  }
  return is;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, ColoUserStatInnerData& data)
  /*throw(eh::Exception)*/
{
  is >> data.unique_users_;
  is >> data.network_unique_users_;
  is >> data.profiling_unique_users_;
  is >> data.unique_hids_;
  return is;
}

std::ostream&
operator<<(std::ostream& os, const ColoUserStatInnerData& data)
  /*throw(eh::Exception)*/
{
  os << data.unique_users_ << '\t';
  os << data.network_unique_users_ << '\t';
  os << data.profiling_unique_users_ << '\t';
  os << data.unique_hids_;
  return os;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, ColoUserStatInnerData_V_2_5& data)
  /*throw(eh::Exception)*/
{
  is >> data.unique_users_;
  is >> data.network_unique_users_;
  return is;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, ColoUserStatInnerData_V_2_6& data)
  /*throw(eh::Exception)*/
{
  is >> data.unique_users_;
  is >> data.network_unique_users_;
  is >> data.unique_hids_;
  return is;
}

void
ColoUsers_To_Colo_Or_GlobalColo_UserStatLoader::load(
  std::istream& is,
  const CollectorBundleFileGuard_var& file_handle
)
  /*throw(Exception)*/
{
  if (!cus_bundle_)
  {
    Stream::Error es;
    es << __PRETTY_FUNCTION__ << ": ColoUserStat bundle is NULL";
    throw Exception(es);
  }
  if (!gcus_bundle_)
  {
    Stream::Error es;
    es << __PRETTY_FUNCTION__ << ": GlobalColoUserStat bundle is NULL";
    throw Exception(es);
  }
  try
  {
    C_U_S_CollectorT cus_collector;
    G_C_U_S_CollectorT gcus_collector;
    OldCollectorT old_collector;
    is >> old_collector;
    if (is.eof())
    {
      for (OldCollectorT::const_iterator it = old_collector.begin();
        it != old_collector.end(); ++it)
      {
        if (it->first.isp_sdate().present())
        {
          add_colo_user_stat_data_(
            it->first.isp_sdate().get(),
            it->first.colo_id(),
            it->first.created(),
            it->second,
            cus_collector
          );
        }
        else
        {
          add_colo_user_stat_data_(
            it->first.sdate().get(),
            it->first.colo_id(),
            it->first.created(),
            it->second,
            gcus_collector
          );
        }
      }
    }
    cus_bundle_->merge(cus_collector, file_handle);
    gcus_bundle_->merge(gcus_collector, file_handle);
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

