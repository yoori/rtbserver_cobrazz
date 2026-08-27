#pragma once

#include <iosfwd>
#include <istream>
#include <ostream>

#include <Generics/CRC.hpp>
#include <Generics/SimpleDecimal.hpp>
#include <Generics/Time.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

#include "LogCommons.hpp"
#include "StatCollector.hpp"

namespace AdServer::LogProcessing
{
  class CreativeStatInnerKey_V_3_3
  {
  public:
    typedef Generics::SimpleDecimal<uint32_t, 6, 5> DeliveryThresholdT;
    typedef OptionalValue<std::uint32_t> GeoChannelIdOptional;
    typedef OptionalValue<std::uint32_t> DeviceChannelIdOptional;

    CreativeStatInnerKey_V_3_3();

    CreativeStatInnerKey_V_3_3(
      std::uint32_t colo_id,
      std::uint32_t publisher_account_id,
      std::uint32_t tag_id,
      const OptionalUInt32& size_id,
      const std::string& country_code,
      std::uint32_t adv_account_id,
      std::uint32_t campaign_id,
      std::uint32_t ccg_id,
      std::uint32_t cc_id,
      std::uint32_t ccg_rate_id,
      std::uint32_t colo_rate_id,
      std::uint32_t site_rate_id,
      std::uint32_t currency_exchange_id,
      const DeliveryThresholdT& delivery_threshold,
      unsigned short num_shown,
      unsigned short position,
      bool test,
      bool fraud,
      bool walled_garden,
      char user_status,
      const GeoChannelIdOptional& geo_channel_id,
      const DeviceChannelIdOptional& device_channel_id,
      std::uint32_t ctr_reset_id,
      bool hid_profile)
      /*throw(eh::Exception)*/;

    bool
    operator==(const CreativeStatInnerKey_V_3_3& rhs) const;

    std::uint32_t
    colo_id() const;

    std::uint32_t
    publisher_account_id() const;

    std::uint32_t
    tag_id() const;

    const OptionalUInt32&
    size_id() const;

    const std::string&
    country_code() const;

    std::uint32_t
    adv_account_id() const;

    std::uint32_t
    campaign_id() const;

    std::uint32_t
    ccg_id() const;

    std::uint32_t
    cc_id() const;

    std::uint32_t
    ccg_rate_id() const;

    std::uint32_t
    colo_rate_id() const;

    std::uint32_t
    site_rate_id() const;

    std::uint32_t
    currency_exchange_id() const;

    const DeliveryThresholdT&
    delivery_threshold() const;

    unsigned short
    num_shown() const;

    unsigned short
    position() const;

    bool
    test() const;

    bool
    fraud() const;

    bool
    walled_garden() const;

    char
    user_status() const;

    const GeoChannelIdOptional&
    geo_channel_id() const;

    const DeviceChannelIdOptional&
    device_channel_id() const;

    std::uint32_t
    ctr_reset_id() const;

    bool
    hid_profile() const;

    size_t
    hash() const;

    friend FixedBufStream<TabCategory>&
    operator>>(FixedBufStream<TabCategory>& is, CreativeStatInnerKey_V_3_3& key);

    friend std::ostream&
    operator<<(std::ostream& os, const CreativeStatInnerKey_V_3_3& key)
      /*throw(eh::Exception)*/;

    template <class Archive>
    void
    serialize(Archive& archive);

    void
    invariant() const /*throw(eh::Exception)*/;

  private:
    void
    calc_hash_();

    bool
    delivery_threshold_is_valid_() const;

    static const DeliveryThresholdT max_delivery_threshold_value_;

    std::uint32_t colo_id_;
    std::uint32_t publisher_account_id_;
    std::uint32_t tag_id_;
    OptionalUInt32 size_id_;
    EmptyHolder<SpacesString> country_code_;
    std::uint32_t adv_account_id_;
    std::uint32_t campaign_id_;
    std::uint32_t ccg_id_;
    std::uint32_t cc_id_;
    std::uint32_t ccg_rate_id_;
    std::uint32_t colo_rate_id_;
    std::uint32_t site_rate_id_;
    std::uint32_t currency_exchange_id_;
    DeliveryThresholdT delivery_threshold_;
    unsigned short num_shown_;
    unsigned short position_;
    bool test_;
    bool fraud_;
    bool walled_garden_;
    char user_status_;
    GeoChannelIdOptional geo_channel_id_;
    DeviceChannelIdOptional device_channel_id_;
    std::uint32_t ctr_reset_id_;
    bool hid_profile_;
    size_t hash_;
  };

  class CreativeStatInnerKey
  {
  public:
    typedef Generics::SimpleDecimal<uint32_t, 6, 5> DeliveryThresholdT;
    typedef OptionalValue<std::uint32_t> GeoChannelIdOptional;
    typedef OptionalValue<std::uint32_t> DeviceChannelIdOptional;

    CreativeStatInnerKey();

    CreativeStatInnerKey(
      std::uint32_t colo_id,
      std::uint32_t publisher_account_id,
      std::uint32_t tag_id,
      const OptionalUInt32& size_id,
      const std::string& country_code,
      std::uint32_t adv_account_id,
      std::uint32_t campaign_id,
      std::uint32_t ccg_id,
      std::uint32_t cc_id,
      std::uint32_t ccg_rate_id,
      std::uint32_t colo_rate_id,
      std::uint32_t site_rate_id,
      std::uint32_t currency_exchange_id,
      const DeliveryThresholdT& delivery_threshold,
      unsigned short num_shown,
      unsigned short position,
      bool test,
      bool fraud,
      bool walled_garden,
      char user_status,
      const GeoChannelIdOptional& geo_channel_id,
      const DeviceChannelIdOptional& device_channel_id,
      std::uint32_t ctr_reset_id,
      bool hid_profile,
      long viewability)
      /*throw(eh::Exception)*/;

    CreativeStatInnerKey(const CreativeStatInnerKey_V_3_3& key);

    bool
    operator==(const CreativeStatInnerKey& rhs) const;

    std::uint32_t
    colo_id() const;

    std::uint32_t
    publisher_account_id() const;

    std::uint32_t
    tag_id() const;

    const OptionalUInt32&
    size_id() const;

    const std::string&
    country_code() const;

    std::uint32_t
    adv_account_id() const;

    std::uint32_t
    campaign_id() const;

    std::uint32_t
    ccg_id() const;

    std::uint32_t
    cc_id() const;

    std::uint32_t
    ccg_rate_id() const;

    std::uint32_t
    colo_rate_id() const;

    std::uint32_t
    site_rate_id() const;

    std::uint32_t
    currency_exchange_id() const;

    const DeliveryThresholdT&
    delivery_threshold() const;

    unsigned short
    num_shown() const;

    unsigned short
    position() const;

    bool
    test() const;

    bool
    fraud() const;

    bool
    walled_garden() const;

    char
    user_status() const;

    const GeoChannelIdOptional&
    geo_channel_id() const;

    const DeviceChannelIdOptional&
    device_channel_id() const;

    std::uint32_t
    ctr_reset_id() const;

    bool
    hid_profile() const;

    long
    viewability() const;

    size_t
    hash() const;

    friend FixedBufStream<TabCategory>&
    operator>>(FixedBufStream<TabCategory>& is, CreativeStatInnerKey& key);

    friend BufferWriter&
    operator<<(BufferWriter& out, const CreativeStatInnerKey& key)
      /*throw(eh::Exception)*/;

    template <class Archive>
    void
    serialize(Archive& archive);

    void
    invariant() const /*throw(eh::Exception)*/;

  private:
    void
    calc_hash_();

    bool
    delivery_threshold_is_valid_() const;

    static const DeliveryThresholdT max_delivery_threshold_value_;

    std::uint32_t colo_id_;
    std::uint32_t publisher_account_id_;
    std::uint32_t tag_id_;
    OptionalUInt32 size_id_;
    EmptyHolder<SpacesString> country_code_;
    std::uint32_t adv_account_id_;
    std::uint32_t campaign_id_;
    std::uint32_t ccg_id_;
    std::uint32_t cc_id_;
    std::uint32_t ccg_rate_id_;
    std::uint32_t colo_rate_id_;
    std::uint32_t site_rate_id_;
    std::uint32_t currency_exchange_id_;
    DeliveryThresholdT delivery_threshold_;
    unsigned short num_shown_;
    unsigned short position_;
    bool test_;
    bool fraud_;
    bool walled_garden_;
    char user_status_;
    GeoChannelIdOptional geo_channel_id_;
    DeviceChannelIdOptional device_channel_id_;
    std::uint32_t ctr_reset_id_;
    bool hid_profile_;
    long viewability_;
    size_t hash_;
  };

  class CreativeStatInnerData
  {
  public:
    typedef AdServer::LogProcessing::FixedNumber FixedNum;

    CreativeStatInnerData();

    CreativeStatInnerData(
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
      const FixedNum& isp_advcurrency_amount);

    bool
    operator==(const CreativeStatInnerData& rhs) const;

    CreativeStatInnerData&
    operator+=(const CreativeStatInnerData& rhs);

    long
    unverified_imps() const;

    long
    imps() const;

    long
    clicks() const;

    long
    actions() const;

    const FixedNum&
    adv_amount() const;

    const FixedNum&
    pub_amount() const;

    const FixedNum&
    isp_amount() const;

    const FixedNum&
    adv_comm_amount() const;

    const FixedNum&
    pub_comm_amount() const;

    const FixedNum&
    adv_payable_comm_amount() const;

    const FixedNum&
    pub_advcurrency_amount() const;

    const FixedNum&
    isp_advcurrency_amount() const;

    friend FixedBufStream<TabCategory>&
    operator>>(FixedBufStream<TabCategory>& is, CreativeStatInnerData& data)
      /*throw(eh::Exception)*/;

    friend std::ostream&
    operator<<(std::ostream& os, const CreativeStatInnerData& data)
      /*throw(eh::Exception)*/;

    friend BufferWriter&
    operator<<(BufferWriter& out, const CreativeStatInnerData& data)
      /*throw(eh::Exception)*/;

    template <class Archive>
    void
    serialize(Archive& archive);

  private:
    long unverified_imps_;
    long imps_;
    long clicks_;
    long actions_;
    FixedNum adv_amount_;
    FixedNum pub_amount_;
    FixedNum isp_amount_;
    FixedNum adv_comm_amount_;
    FixedNum pub_comm_amount_;
    FixedNum adv_payable_comm_amount_;
    FixedNum pub_advcurrency_amount_;
    FixedNum isp_advcurrency_amount_;
  };

  class CreativeStatKey
  {
  public:
    CreativeStatKey();

    CreativeStatKey(const DayHourTimestamp& sdate, const DayHourTimestamp& adv_sdate);

    CreativeStatKey(const DayHourTimestamp& timestamp);

    bool
    operator==(const CreativeStatKey& rhs) const;

    const DayHourTimestamp&
    sdate() const;

    const DayHourTimestamp&
    adv_sdate() const;

    size_t
    hash() const;

    friend std::istream&
    operator>>(std::istream& is, CreativeStatKey& key);

    friend std::ostream&
    operator<<(std::ostream& os, const CreativeStatKey& key)
      /*throw(eh::Exception)*/;

    friend BufferWriter&
    operator<<(BufferWriter& out, const CreativeStatKey& key)
      /*throw(eh::Exception)*/;

  private:
    void
    calc_hash_();

    DayHourTimestamp sdate_;
    DayHourTimestamp adv_sdate_;
    size_t hash_;
  };

  typedef CreativeStatInnerData CreativeStatInnerData_V_3_3;

  typedef StatCollector<
    CreativeStatInnerKey_V_3_3,
    CreativeStatInnerData_V_3_3,
    false,
    true> CreativeStatInnerCollector_V_3_3;

  typedef CreativeStatKey CreativeStatKey_V_3_3;
  typedef CreativeStatInnerCollector_V_3_3 CreativeStatData_V_3_3;

  typedef StatCollector<CreativeStatKey_V_3_3, CreativeStatData_V_3_3>
    CreativeStatCollector_V_3_3;

  typedef StatCollector<CreativeStatInnerKey, CreativeStatInnerData, false, true>
    CreativeStatInnerCollector;

  typedef CreativeStatInnerCollector CreativeStatData;

  typedef StatCollector<CreativeStatKey, CreativeStatData>
    CreativeStatCollector;

  struct CreativeStatTraits: LogDefaultTraits<CreativeStatCollector>
  {
    template <class Functor>
    static void
    for_each_old(Functor& functor) /*throw(eh::Exception)*/;
  };
}

#include "CreativeStat.tpp"
