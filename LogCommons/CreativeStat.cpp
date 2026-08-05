
#include "CreativeStat.hpp"
#include <LogCommons/BufferWriter.hpp>
#include <LogCommons/LogCommons.ipp>

namespace AdServer {
namespace LogProcessing {

template <> const char* CreativeStatTraits::B::base_name_ =
  "CreativeStat";
template <> const char* CreativeStatTraits::B::signature_ =
  "CreativeStat";
template <> const char* CreativeStatTraits::B::current_version_ =
  "3.5";

const CreativeStatInnerKey::DeliveryThresholdT
  CreativeStatInnerKey::max_delivery_threshold_value_("1.0");

const CreativeStatInnerKey_V_3_3::DeliveryThresholdT
  CreativeStatInnerKey_V_3_3::max_delivery_threshold_value_("1.0");

CreativeStatInnerKey_V_3_3::CreativeStatInnerKey_V_3_3()
  : colo_id_(),
    publisher_account_id_(),
    tag_id_(),
    size_id_(),
    country_code_(),
    adv_account_id_(),
    campaign_id_(),
    ccg_id_(),
    cc_id_(),
    ccg_rate_id_(),
    colo_rate_id_(),
    site_rate_id_(),
    currency_exchange_id_(),
    delivery_threshold_(DeliveryThresholdT::ZERO),
    num_shown_(),
    position_(),
    test_(),
    fraud_(),
    walled_garden_(),
    user_status_(),
    geo_channel_id_(),
    device_channel_id_(),
    ctr_reset_id_(),
    hid_profile_(),
    hash_()
{}

CreativeStatInnerKey_V_3_3::CreativeStatInnerKey_V_3_3(
  unsigned long colo_id,
  unsigned long publisher_account_id,
  unsigned long tag_id,
  const OptionalUlong& size_id,
  const std::string& country_code,
  unsigned long adv_account_id,
  unsigned long campaign_id,
  unsigned long ccg_id,
  unsigned long cc_id,
  unsigned long ccg_rate_id,
  unsigned long colo_rate_id,
  unsigned long site_rate_id,
  unsigned long currency_exchange_id,
  const DeliveryThresholdT& delivery_threshold,
  unsigned short num_shown,
  unsigned short position,
  bool test,
  bool fraud,
  bool walled_garden,
  char user_status,
  const GeoChannelIdOptional& geo_channel_id,
  const DeviceChannelIdOptional& device_channel_id,
  unsigned long ctr_reset_id,
  bool hid_profile)
  /*throw(eh::Exception)*/
  : colo_id_(colo_id),
    publisher_account_id_(publisher_account_id),
    tag_id_(tag_id),
    size_id_(size_id),
    country_code_(country_code),
    adv_account_id_(adv_account_id),
    campaign_id_(campaign_id),
    ccg_id_(ccg_id),
    cc_id_(cc_id),
    ccg_rate_id_(ccg_rate_id),
    colo_rate_id_(colo_rate_id),
    site_rate_id_(site_rate_id),
    currency_exchange_id_(currency_exchange_id),
    delivery_threshold_(delivery_threshold),
    num_shown_(num_shown),
    position_(position),
    test_(test),
    fraud_(fraud),
    walled_garden_(walled_garden),
    user_status_(user_status),
    geo_channel_id_(geo_channel_id),
    device_channel_id_(device_channel_id),
    ctr_reset_id_(ctr_reset_id),
    hid_profile_(hid_profile),
    hash_()
{
  calc_hash_();
}

bool
CreativeStatInnerKey_V_3_3::operator==(
  const CreativeStatInnerKey_V_3_3& rhs) const
{
  return this == &rhs ||
    (colo_id_ == rhs.colo_id_ &&
      publisher_account_id_ == rhs.publisher_account_id_ &&
      tag_id_ == rhs.tag_id_ &&
      size_id_ == rhs.size_id_ &&
      country_code_.get() == rhs.country_code_.get() &&
      adv_account_id_ == rhs.adv_account_id_ &&
      campaign_id_ == rhs.campaign_id_ &&
      ccg_id_ == rhs.ccg_id_ &&
      cc_id_ == rhs.cc_id_ &&
      ccg_rate_id_ == rhs.ccg_rate_id_ &&
      colo_rate_id_ == rhs.colo_rate_id_ &&
      site_rate_id_ == rhs.site_rate_id_ &&
      currency_exchange_id_ == rhs.currency_exchange_id_ &&
      delivery_threshold_ == rhs.delivery_threshold_ &&
      num_shown_ == rhs.num_shown_ &&
      position_ == rhs.position_ &&
      test_ == rhs.test_ &&
      fraud_ == rhs.fraud_ &&
      walled_garden_ == rhs.walled_garden_ &&
      user_status_ == rhs.user_status_ &&
      geo_channel_id_ == rhs.geo_channel_id_ &&
      device_channel_id_ == rhs.device_channel_id_ &&
      ctr_reset_id_ == rhs.ctr_reset_id_ &&
      hid_profile_ == rhs.hid_profile_);
}

unsigned long
CreativeStatInnerKey_V_3_3::colo_id() const
{
  return colo_id_;
}

unsigned long
CreativeStatInnerKey_V_3_3::publisher_account_id() const
{
  return publisher_account_id_;
}

unsigned long
CreativeStatInnerKey_V_3_3::tag_id() const
{
  return tag_id_;
}

const OptionalUlong&
CreativeStatInnerKey_V_3_3::size_id() const
{
  return size_id_;
}

const std::string&
CreativeStatInnerKey_V_3_3::country_code() const
{
  return country_code_.get();
}

unsigned long
CreativeStatInnerKey_V_3_3::adv_account_id() const
{
  return adv_account_id_;
}

unsigned long
CreativeStatInnerKey_V_3_3::campaign_id() const
{
  return campaign_id_;
}

unsigned long
CreativeStatInnerKey_V_3_3::ccg_id() const
{
  return ccg_id_;
}

unsigned long
CreativeStatInnerKey_V_3_3::cc_id() const
{
  return cc_id_;
}

unsigned long
CreativeStatInnerKey_V_3_3::ccg_rate_id() const
{
  return ccg_rate_id_;
}

unsigned long
CreativeStatInnerKey_V_3_3::colo_rate_id() const
{
  return colo_rate_id_;
}

unsigned long
CreativeStatInnerKey_V_3_3::site_rate_id() const
{
  return site_rate_id_;
}

unsigned long
CreativeStatInnerKey_V_3_3::currency_exchange_id() const
{
  return currency_exchange_id_;
}

const CreativeStatInnerKey_V_3_3::DeliveryThresholdT&
CreativeStatInnerKey_V_3_3::delivery_threshold() const
{
  return delivery_threshold_;
}

unsigned short
CreativeStatInnerKey_V_3_3::num_shown() const
{
  return num_shown_;
}

unsigned short
CreativeStatInnerKey_V_3_3::position() const
{
  return position_;
}

bool
CreativeStatInnerKey_V_3_3::test() const
{
  return test_;
}

bool
CreativeStatInnerKey_V_3_3::fraud() const
{
  return fraud_;
}

bool
CreativeStatInnerKey_V_3_3::walled_garden() const
{
  return walled_garden_;
}

char
CreativeStatInnerKey_V_3_3::user_status() const
{
  return user_status_;
}

const CreativeStatInnerKey_V_3_3::GeoChannelIdOptional&
CreativeStatInnerKey_V_3_3::geo_channel_id() const
{
  return geo_channel_id_;
}

const CreativeStatInnerKey_V_3_3::DeviceChannelIdOptional&
CreativeStatInnerKey_V_3_3::device_channel_id() const
{
  return device_channel_id_;
}

unsigned long
CreativeStatInnerKey_V_3_3::ctr_reset_id() const
{
  return ctr_reset_id_;
}

bool
CreativeStatInnerKey_V_3_3::hid_profile() const
{
  return hid_profile_;
}

size_t
CreativeStatInnerKey_V_3_3::hash() const
{
  return hash_;
}

void
CreativeStatInnerKey_V_3_3::invariant() const
  /*throw(eh::Exception)*/
{
  static const char FUNC[] = "CreativeStatInnerKey_V_3_3::invariant()";
  if (!num_shown_)
  {
    Stream::Error es;
    es << FUNC << ": num_shown must be > 0";
    throw InvalidArgValue(es);
  }
  if (!position_)
  {
    Stream::Error es;
    es << FUNC << ": position must be > 0";
    throw InvalidArgValue(es);
  }
  if (!is_valid_user_status(user_status_))
  {
    Stream::Error es;
    es << FUNC << ": user_status_ has invalid value '" << user_status_ << '\'';
    throw ConstraintViolation(es);
  }
  if (!delivery_threshold_is_valid_())
  {
    Stream::Error es;
    es << FUNC << ": delivery_threshold_ has invalid value '"
      << delivery_threshold_ << '\'';
    throw ConstraintViolation(es);
  }
}

void
CreativeStatInnerKey_V_3_3::calc_hash_()
{
  Generics::Murmur64Hash hasher(hash_);
  hash_add(hasher, colo_id_);
  hash_add(hasher, publisher_account_id_);
  hash_add(hasher, tag_id_);
  size_id_.hash_add(hasher);
  hash_add(hasher, adv_account_id_);
  hash_add(hasher, campaign_id_);
  hash_add(hasher, ccg_id_);
  hash_add(hasher, cc_id_);
  hash_add(hasher, ccg_rate_id_);
  hash_add(hasher, colo_rate_id_);
  hash_add(hasher, site_rate_id_);
  hash_add(hasher, currency_exchange_id_);
  hash_add(hasher, delivery_threshold_);
  hash_add(hasher, ctr_reset_id_);
  geo_channel_id_.hash_add(hasher);
  device_channel_id_.hash_add(hasher);
  hash_add(hasher, country_code_.get());
  hash_add(hasher, num_shown_);
  hash_add(hasher, position_);
  hash_add(hasher, test_);
  hash_add(hasher, fraud_);
  hash_add(hasher, walled_garden_);
  hash_add(hasher, user_status_);
  hash_add(hasher, hid_profile_);
}

bool
CreativeStatInnerKey_V_3_3::delivery_threshold_is_valid_() const
{
  return delivery_threshold_ >= DeliveryThresholdT::ZERO &&
    delivery_threshold_ <= max_delivery_threshold_value_;
}

CreativeStatInnerKey::CreativeStatInnerKey()
  : colo_id_(),
    publisher_account_id_(),
    tag_id_(),
    size_id_(),
    country_code_(),
    adv_account_id_(),
    campaign_id_(),
    ccg_id_(),
    cc_id_(),
    ccg_rate_id_(),
    colo_rate_id_(),
    site_rate_id_(),
    currency_exchange_id_(),
    delivery_threshold_(DeliveryThresholdT::ZERO),
    num_shown_(),
    position_(),
    test_(),
    fraud_(),
    walled_garden_(),
    user_status_(),
    geo_channel_id_(),
    device_channel_id_(),
    ctr_reset_id_(),
    hid_profile_(),
    viewability_(),
    hash_()
{}

CreativeStatInnerKey::CreativeStatInnerKey(
  unsigned long colo_id,
  unsigned long publisher_account_id,
  unsigned long tag_id,
  const OptionalUlong& size_id,
  const std::string& country_code,
  unsigned long adv_account_id,
  unsigned long campaign_id,
  unsigned long ccg_id,
  unsigned long cc_id,
  unsigned long ccg_rate_id,
  unsigned long colo_rate_id,
  unsigned long site_rate_id,
  unsigned long currency_exchange_id,
  const DeliveryThresholdT& delivery_threshold,
  unsigned short num_shown,
  unsigned short position,
  bool test,
  bool fraud,
  bool walled_garden,
  char user_status,
  const GeoChannelIdOptional& geo_channel_id,
  const DeviceChannelIdOptional& device_channel_id,
  unsigned long ctr_reset_id,
  bool hid_profile,
  long viewability)
  /*throw(eh::Exception)*/
  : colo_id_(colo_id),
    publisher_account_id_(publisher_account_id),
    tag_id_(tag_id),
    size_id_(size_id),
    country_code_(country_code),
    adv_account_id_(adv_account_id),
    campaign_id_(campaign_id),
    ccg_id_(ccg_id),
    cc_id_(cc_id),
    ccg_rate_id_(ccg_rate_id),
    colo_rate_id_(colo_rate_id),
    site_rate_id_(site_rate_id),
    currency_exchange_id_(currency_exchange_id),
    delivery_threshold_(delivery_threshold),
    num_shown_(num_shown),
    position_(position),
    test_(test),
    fraud_(fraud),
    walled_garden_(walled_garden),
    user_status_(user_status),
    geo_channel_id_(geo_channel_id),
    device_channel_id_(device_channel_id),
    ctr_reset_id_(ctr_reset_id),
    hid_profile_(hid_profile),
    viewability_(viewability),
    hash_()
{
  calc_hash_();
}

CreativeStatInnerKey::CreativeStatInnerKey(
  const CreativeStatInnerKey_V_3_3& key)
  : colo_id_(key.colo_id()),
    publisher_account_id_(key.publisher_account_id()),
    tag_id_(key.tag_id()),
    size_id_(key.size_id()),
    country_code_(key.country_code()),
    adv_account_id_(key.adv_account_id()),
    campaign_id_(key.campaign_id()),
    ccg_id_(key.ccg_id()),
    cc_id_(key.cc_id()),
    ccg_rate_id_(key.ccg_rate_id()),
    colo_rate_id_(key.colo_rate_id()),
    site_rate_id_(key.site_rate_id()),
    currency_exchange_id_(key.currency_exchange_id()),
    delivery_threshold_(key.delivery_threshold()),
    num_shown_(key.num_shown()),
    position_(key.position()),
    test_(key.test()),
    fraud_(key.fraud()),
    walled_garden_(key.walled_garden()),
    user_status_(key.user_status()),
    geo_channel_id_(key.geo_channel_id()),
    device_channel_id_(key.device_channel_id()),
    ctr_reset_id_(key.ctr_reset_id()),
    hid_profile_(key.hid_profile()),
    viewability_(-1),
    hash_()
{
  calc_hash_();
}

bool
CreativeStatInnerKey::operator==(const CreativeStatInnerKey& rhs) const
{
  return this == &rhs ||
    (colo_id_ == rhs.colo_id_ &&
      publisher_account_id_ == rhs.publisher_account_id_ &&
      tag_id_ == rhs.tag_id_ &&
      size_id_ == rhs.size_id_ &&
      country_code_.get() == rhs.country_code_.get() &&
      adv_account_id_ == rhs.adv_account_id_ &&
      campaign_id_ == rhs.campaign_id_ &&
      ccg_id_ == rhs.ccg_id_ &&
      cc_id_ == rhs.cc_id_ &&
      ccg_rate_id_ == rhs.ccg_rate_id_ &&
      colo_rate_id_ == rhs.colo_rate_id_ &&
      site_rate_id_ == rhs.site_rate_id_ &&
      currency_exchange_id_ == rhs.currency_exchange_id_ &&
      delivery_threshold_ == rhs.delivery_threshold_ &&
      num_shown_ == rhs.num_shown_ &&
      position_ == rhs.position_ &&
      test_ == rhs.test_ &&
      fraud_ == rhs.fraud_ &&
      walled_garden_ == rhs.walled_garden_ &&
      user_status_ == rhs.user_status_ &&
      geo_channel_id_ == rhs.geo_channel_id_ &&
      device_channel_id_ == rhs.device_channel_id_ &&
      ctr_reset_id_ == rhs.ctr_reset_id_ &&
      hid_profile_ == rhs.hid_profile_ &&
      viewability_ == rhs.viewability_);
}

unsigned long
CreativeStatInnerKey::colo_id() const
{
  return colo_id_;
}

unsigned long
CreativeStatInnerKey::publisher_account_id() const
{
  return publisher_account_id_;
}

unsigned long
CreativeStatInnerKey::tag_id() const
{
  return tag_id_;
}

const OptionalUlong&
CreativeStatInnerKey::size_id() const
{
  return size_id_;
}

const std::string&
CreativeStatInnerKey::country_code() const
{
  return country_code_.get();
}

unsigned long
CreativeStatInnerKey::adv_account_id() const
{
  return adv_account_id_;
}

unsigned long
CreativeStatInnerKey::campaign_id() const
{
  return campaign_id_;
}

unsigned long
CreativeStatInnerKey::ccg_id() const
{
  return ccg_id_;
}

unsigned long
CreativeStatInnerKey::cc_id() const
{
  return cc_id_;
}

unsigned long
CreativeStatInnerKey::ccg_rate_id() const
{
  return ccg_rate_id_;
}

unsigned long
CreativeStatInnerKey::colo_rate_id() const
{
  return colo_rate_id_;
}

unsigned long
CreativeStatInnerKey::site_rate_id() const
{
  return site_rate_id_;
}

unsigned long
CreativeStatInnerKey::currency_exchange_id() const
{
  return currency_exchange_id_;
}

const CreativeStatInnerKey::DeliveryThresholdT&
CreativeStatInnerKey::delivery_threshold() const
{
  return delivery_threshold_;
}

unsigned short
CreativeStatInnerKey::num_shown() const
{
  return num_shown_;
}

unsigned short
CreativeStatInnerKey::position() const
{
  return position_;
}

bool
CreativeStatInnerKey::test() const
{
  return test_;
}

bool
CreativeStatInnerKey::fraud() const
{
  return fraud_;
}

bool
CreativeStatInnerKey::walled_garden() const
{
  return walled_garden_;
}

char
CreativeStatInnerKey::user_status() const
{
  return user_status_;
}

const CreativeStatInnerKey::GeoChannelIdOptional&
CreativeStatInnerKey::geo_channel_id() const
{
  return geo_channel_id_;
}

const CreativeStatInnerKey::DeviceChannelIdOptional&
CreativeStatInnerKey::device_channel_id() const
{
  return device_channel_id_;
}

unsigned long
CreativeStatInnerKey::ctr_reset_id() const
{
  return ctr_reset_id_;
}

bool
CreativeStatInnerKey::hid_profile() const
{
  return hid_profile_;
}

long
CreativeStatInnerKey::viewability() const
{
  return viewability_;
}

size_t
CreativeStatInnerKey::hash() const
{
  return hash_;
}

void
CreativeStatInnerKey::invariant() const
  /*throw(eh::Exception)*/
{
  static const char FUNC[] = "CreativeStatInnerKey::invariant()";
  if (!num_shown_)
  {
    Stream::Error es;
    es << FUNC << ": num_shown must be > 0";
    throw InvalidArgValue(es);
  }
  if (!position_)
  {
    Stream::Error es;
    es << FUNC << ": position must be > 0";
    throw InvalidArgValue(es);
  }
  if (!is_valid_user_status(user_status_))
  {
    Stream::Error es;
    es << FUNC << ": user_status_ has invalid value '" << user_status_ << '\'';
    throw ConstraintViolation(es);
  }
  if (!delivery_threshold_is_valid_())
  {
    Stream::Error es;
    es << FUNC << ": delivery_threshold_ has invalid value '"
      << delivery_threshold_ << '\'';
    throw ConstraintViolation(es);
  }
}

void
CreativeStatInnerKey::calc_hash_()
{
  Generics::Murmur64Hash hasher(hash_);
  hash_add(hasher, colo_id_);
  hash_add(hasher, publisher_account_id_);
  hash_add(hasher, tag_id_);
  size_id_.hash_add(hasher);
  hash_add(hasher, adv_account_id_);
  hash_add(hasher, campaign_id_);
  hash_add(hasher, ccg_id_);
  hash_add(hasher, cc_id_);
  hash_add(hasher, ccg_rate_id_);
  hash_add(hasher, colo_rate_id_);
  hash_add(hasher, site_rate_id_);
  hash_add(hasher, currency_exchange_id_);
  hash_add(hasher, delivery_threshold_);
  hash_add(hasher, ctr_reset_id_);
  geo_channel_id_.hash_add(hasher);
  device_channel_id_.hash_add(hasher);
  hash_add(hasher, country_code_.get());
  hash_add(hasher, num_shown_);
  hash_add(hasher, position_);
  hash_add(hasher, test_);
  hash_add(hasher, fraud_);
  hash_add(hasher, walled_garden_);
  hash_add(hasher, user_status_);
  hash_add(hasher, hid_profile_);
  hash_add(hasher, viewability_);
}

bool
CreativeStatInnerKey::delivery_threshold_is_valid_() const
{
  return delivery_threshold_ >= DeliveryThresholdT::ZERO &&
    delivery_threshold_ <= max_delivery_threshold_value_;
}

CreativeStatInnerData::CreativeStatInnerData()
  : unverified_imps_(),
    imps_(),
    clicks_(),
    actions_(),
    adv_amount_(FixedNum::ZERO),
    pub_amount_(FixedNum::ZERO),
    isp_amount_(FixedNum::ZERO),
    adv_comm_amount_(FixedNum::ZERO),
    pub_comm_amount_(FixedNum::ZERO),
    adv_payable_comm_amount_(FixedNum::ZERO),
    pub_advcurrency_amount_(FixedNum::ZERO),
    isp_advcurrency_amount_(FixedNum::ZERO)
{}

CreativeStatInnerData::CreativeStatInnerData(
  long unverified_imps,
  long imps,
  long clicks,
  long actions,
  const FixedNum& adv_amount,
  const FixedNum& pub_amount,
  const FixedNum& isp_amount,
  const FixedNum& adv_comm_amount,
  const FixedNum& pub_comm_amount,
  const FixedNum& adv_payable_comm_amount,
  const FixedNum& pub_advcurrency_amount,
  const FixedNum& isp_advcurrency_amount)
  : unverified_imps_(unverified_imps),
    imps_(imps),
    clicks_(clicks),
    actions_(actions),
    adv_amount_(adv_amount),
    pub_amount_(pub_amount),
    isp_amount_(isp_amount),
    adv_comm_amount_(adv_comm_amount),
    pub_comm_amount_(pub_comm_amount),
    adv_payable_comm_amount_(adv_payable_comm_amount),
    pub_advcurrency_amount_(pub_advcurrency_amount),
    isp_advcurrency_amount_(isp_advcurrency_amount)
{}

bool
CreativeStatInnerData::operator==(const CreativeStatInnerData& rhs) const
{
  return this == &rhs ||
    (unverified_imps_ == rhs.unverified_imps_ &&
      imps_ == rhs.imps_ &&
      clicks_ == rhs.clicks_ &&
      actions_ == rhs.actions_ &&
      adv_amount_ == rhs.adv_amount_ &&
      pub_amount_ == rhs.pub_amount_ &&
      isp_amount_ == rhs.isp_amount_ &&
      adv_comm_amount_ == rhs.adv_comm_amount_ &&
      pub_comm_amount_ == rhs.pub_comm_amount_ &&
      adv_payable_comm_amount_ == rhs.adv_payable_comm_amount_ &&
      pub_advcurrency_amount_ == rhs.pub_advcurrency_amount_ &&
      isp_advcurrency_amount_ == rhs.isp_advcurrency_amount_);
}

CreativeStatInnerData&
CreativeStatInnerData::operator+=(const CreativeStatInnerData& rhs)
{
  unverified_imps_ += rhs.unverified_imps_;
  imps_ += rhs.imps_;
  clicks_ += rhs.clicks_;
  actions_ += rhs.actions_;
  adv_amount_ += rhs.adv_amount_;
  pub_amount_ += rhs.pub_amount_;
  isp_amount_ += rhs.isp_amount_;
  adv_comm_amount_ += rhs.adv_comm_amount_;
  pub_comm_amount_ += rhs.pub_comm_amount_;
  adv_payable_comm_amount_ += rhs.adv_payable_comm_amount_;
  pub_advcurrency_amount_ += rhs.pub_advcurrency_amount_;
  isp_advcurrency_amount_ += rhs.isp_advcurrency_amount_;
  return *this;
}

long
CreativeStatInnerData::unverified_imps() const
{
  return unverified_imps_;
}

long
CreativeStatInnerData::imps() const
{
  return imps_;
}

long
CreativeStatInnerData::clicks() const
{
  return clicks_;
}

long
CreativeStatInnerData::actions() const
{
  return actions_;
}

const CreativeStatInnerData::FixedNum&
CreativeStatInnerData::adv_amount() const
{
  return adv_amount_;
}

const CreativeStatInnerData::FixedNum&
CreativeStatInnerData::pub_amount() const
{
  return pub_amount_;
}

const CreativeStatInnerData::FixedNum&
CreativeStatInnerData::isp_amount() const
{
  return isp_amount_;
}

const CreativeStatInnerData::FixedNum&
CreativeStatInnerData::adv_comm_amount() const
{
  return adv_comm_amount_;
}

const CreativeStatInnerData::FixedNum&
CreativeStatInnerData::pub_comm_amount() const
{
  return pub_comm_amount_;
}

const CreativeStatInnerData::FixedNum&
CreativeStatInnerData::adv_payable_comm_amount() const
{
  return adv_payable_comm_amount_;
}

const CreativeStatInnerData::FixedNum&
CreativeStatInnerData::pub_advcurrency_amount() const
{
  return pub_advcurrency_amount_;
}

const CreativeStatInnerData::FixedNum&
CreativeStatInnerData::isp_advcurrency_amount() const
{
  return isp_advcurrency_amount_;
}

CreativeStatKey::CreativeStatKey()
{}

CreativeStatKey::CreativeStatKey(
  const DayHourTimestamp& sdate,
  const DayHourTimestamp& adv_sdate)
  : sdate_(sdate),
    adv_sdate_(adv_sdate),
    hash_()
{
  calc_hash_();
}

CreativeStatKey::CreativeStatKey(const DayHourTimestamp& timestamp)
  : sdate_(timestamp),
    adv_sdate_(sdate_),
    hash_()
{
  calc_hash_();
}

bool
CreativeStatKey::operator==(const CreativeStatKey& rhs) const
{
  return this == &rhs ||
    (sdate_ == rhs.sdate_ && adv_sdate_ == rhs.adv_sdate_);
}

const DayHourTimestamp&
CreativeStatKey::sdate() const
{
  return sdate_;
}

const DayHourTimestamp&
CreativeStatKey::adv_sdate() const
{
  return adv_sdate_;
}

size_t
CreativeStatKey::hash() const
{
  return hash_;
}

void
CreativeStatKey::calc_hash_()
{
  Generics::Murmur64Hash hasher(hash_);
  sdate_.hash_add(hasher);
  adv_sdate_.hash_add(hasher);
}

std::istream&
operator>>(std::istream& is, CreativeStatKey& key)
{
  is >> key.sdate_;
  read_eol(is);
  is >> key.adv_sdate_;
  key.calc_hash_();
  return is;
}

std::ostream&
operator<<(std::ostream& os, const CreativeStatKey& key)
  /*throw(eh::Exception)*/
{
  os << key.sdate_ << '\n' << key.adv_sdate_;
  return os;
}

BufferWriter&
operator<<(BufferWriter& out, const CreativeStatKey& key)
  /*throw(eh::Exception)*/
{
  out << key.sdate_ << '\n' << key.adv_sdate_;
  return out;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, CreativeStatInnerKey& key)
{
  TokenizerInputArchive<> ia(is);
  ia >> key;
  key.calc_hash_();
  return is;
}

BufferWriter&
operator<<(BufferWriter& out, const CreativeStatInnerKey& key)
  /*throw(eh::Exception)*/
{
  BufferTabOutputArchive archive(out);
  archive << key;
  return out;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, CreativeStatInnerData& data)
  /*throw(eh::Exception)*/
{
  TokenizerInputArchive<Aux_::NoInvariants> ia(is);
  ia >> data;
  return is;
}

std::ostream&
operator<<(std::ostream& os, const CreativeStatInnerData& data)
  /*throw(eh::Exception)*/
{
  SimpleTabOutputArchive oa(os);
  oa << data;
  return os;
}

BufferWriter&
operator<<(BufferWriter& out, const CreativeStatInnerData& data)
  /*throw(eh::Exception)*/
{
  SimpleBufferTabOutputArchive archive(out);
  archive << data;
  return out;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, CreativeStatInnerKey_V_3_3& key)
{
  TokenizerInputArchive<> ia(is);
  ia >> key;
  key.calc_hash_();
  return is;
}

} // namespace LogProcessing
} // namespace AdServer
