
#include "ChannelPriceRange.hpp"
#include <LogCommons/LogCommons.ipp>

namespace AdServer {
namespace LogProcessing {

template <> const char* ChannelPriceRangeTraits::B::base_name_ =
  "ChannelPriceRange";
template <> const char* ChannelPriceRangeTraits::B::signature_ =
  "ChannelPriceRange";
template <> const char* ChannelPriceRangeTraits::B::current_version_ =
  "3.0";

FixedBufStream<TabCategory>&
operator>>(
  FixedBufStream<TabCategory>& is,
  ChannelPriceRangeInnerKey& key)
  /*throw(eh::Exception)*/
{
  Aux_::StringIoWrapper creative_size_wrapper;
  is >> creative_size_wrapper;
  key.creative_size_ = creative_size_wrapper;
  Aux_::StringIoWrapper country_code_wrapper;
  is >> country_code_wrapper;

  if (country_code_wrapper == "-")
  {
    key.country_code_ = std::string();
  }
  else
  {
    key.country_code_ = std::move(country_code_wrapper);
  }

  is >> key.channel_id_;
  is >> key.ecpm_;
  is >> key.colo_id_;
  key.calc_hash_();
  return is;
}

std::ostream&
operator<<(std::ostream& os, const ChannelPriceRangeInnerKey& key)
  /*throw(eh::Exception)*/
{
  os << Aux_::StringIoWrapper(key.creative_size_.str()) << '\t';
  if (key.country_code_.empty())
  {
    os << "-\t";
  }
  else
  {
    os << Aux_::StringIoWrapper(key.country_code_.str()) << '\t';
  }
  os << key.channel_id_ << '\t';
  os << key.ecpm_ << '\t';
  os << key.colo_id_;
  return os;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, ChannelPriceRangeInnerData& data)
  /*throw(eh::Exception)*/
{
  is >> data.unique_users_count_;
  is >> data.impops_;
  return is;
}

std::ostream&
operator<<(std::ostream& os, const ChannelPriceRangeInnerData& data)
  /*throw(eh::Exception)*/
{
  return os
    << data.unique_users_count_ << '\t'
    << data.impops_;
}

} // namespace LogProcessing
} // namespace AdServer

