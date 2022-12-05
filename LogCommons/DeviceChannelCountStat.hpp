#ifndef AD_SERVER_LOG_PROCESSING_DEVICE_CHANNEL_COUNT_STAT_HPP
#define AD_SERVER_LOG_PROCESSING_DEVICE_CHANNEL_COUNT_STAT_HPP


#include <iosfwd>
#include <Generics/Time.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <Stream/MemoryStream.hpp>
#include "LogCommons.hpp"
#include "StatCollector.hpp"


namespace AdServer {
namespace LogProcessing {

class DeviceChannelCountStatInnerKey
{
public:
  DeviceChannelCountStatInnerKey()
  :
    device_channel_id_(),
    channel_type_(),
    channel_count_(),
    hash_()
  {
  }

  DeviceChannelCountStatInnerKey(
    unsigned long device_channel_id,
    char channel_type,
    unsigned long channel_count
  )
  :
    device_channel_id_(device_channel_id),
    channel_type_(channel_type),
    channel_count_(channel_count),
    hash_()
  {
    calc_hash_();
  }

  bool operator==(const DeviceChannelCountStatInnerKey& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return device_channel_id_ == rhs.device_channel_id_ &&
      channel_type_ == rhs.channel_type_ &&
      channel_count_ == rhs.channel_count_;
  }

  unsigned long device_channel_id() const
  {
    return device_channel_id_;
  }

  char channel_type() const
  {
    return channel_type_;
  }

  unsigned long channel_count() const
  {
    return channel_count_;
  }

  size_t hash() const
  {
    return hash_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is,
    DeviceChannelCountStatInnerKey& key)
    /*throw(eh::Exception)*/;

  friend
  std::ostream&
  operator<<(std::ostream& os, const DeviceChannelCountStatInnerKey& key)
    /*throw(eh::Exception)*/;

private:
  void calc_hash_()
  {
    Generics::Murmur64Hash hasher(hash_);
    hash_add(hasher, device_channel_id_);
    hash_add(hasher, channel_count_);
    hash_add(hasher, channel_type_);
  }

  bool channel_type_is_valid_() const
  {
    if (channel_type_ == 'A' || channel_type_ == 'D')
    {
      return true;
    }
    return false;
  }

  void invariant() const /*throw(eh::Exception)*/
  {
    if (!channel_type_is_valid_())
    {
      Stream::Error es;
      es << "DeviceChannelCountStatInnerKey::invariant(): channel_type_ "
         << "has invalid value '" << channel_type_ << '\'';
      throw ConstraintViolation(es);
    }
  }

  unsigned long device_channel_id_;
  char channel_type_;
  unsigned long channel_count_;
  size_t hash_;
};

class DeviceChannelCountStatInnerData
{
public:
  DeviceChannelCountStatInnerData()
  :
    users_count_()
  {
  }

  explicit
  DeviceChannelCountStatInnerData(
    unsigned long users_count
  )
  :
    users_count_(users_count)
  {
  }

  bool operator==(const DeviceChannelCountStatInnerData& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return users_count_ == rhs.users_count_;
  }

  DeviceChannelCountStatInnerData&
  operator+=(const DeviceChannelCountStatInnerData& rhs)
  {
    users_count_ += rhs.users_count_;
    return *this;
  }

  unsigned long users_count() const
  {
    return users_count_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is,
    DeviceChannelCountStatInnerData& data)
    /*throw(eh::Exception)*/;

  friend
  std::ostream&
  operator<<(std::ostream& os, const DeviceChannelCountStatInnerData& data)
    /*throw(eh::Exception)*/;

private:
  unsigned long users_count_;
};

class DeviceChannelCountStatKey
{
public:
  DeviceChannelCountStatKey(): isp_sdate_(), colo_id_(), hash_() {}

  DeviceChannelCountStatKey(
    const DayTimestamp& isp_sdate,
    unsigned long colo_id
  )
  :
    isp_sdate_(isp_sdate),
    colo_id_(colo_id),
    hash_()
  {
    calc_hash_();
  }

  bool operator==(const DeviceChannelCountStatKey& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return isp_sdate_ == rhs.isp_sdate_ && colo_id_ == rhs.colo_id_;
  }

  const DayTimestamp& isp_sdate() const
  {
    return isp_sdate_;
  }

  unsigned long colo_id() const
  {
    return colo_id_;
  }

  size_t hash() const
  {
    return hash_;
  }

  friend
  std::istream&
  operator>>(std::istream& is, DeviceChannelCountStatKey& key);

  friend
  std::ostream&
  operator<<(std::ostream& os, const DeviceChannelCountStatKey& key)
    /*throw(eh::Exception)*/;

private:
  void calc_hash_()
  {
    Generics::Murmur64Hash hasher(hash_);
    hash_add(hasher, colo_id_);
    isp_sdate_.hash_add(hasher);
  }

  DayTimestamp isp_sdate_;
  unsigned long colo_id_;
  size_t hash_;
};


typedef StatCollector<
          DeviceChannelCountStatInnerKey,
          DeviceChannelCountStatInnerData,
          false,
          true
        > DeviceChannelCountStatInnerCollector;

typedef DeviceChannelCountStatInnerCollector DeviceChannelCountStatData;

typedef StatCollector<DeviceChannelCountStatKey, DeviceChannelCountStatData>
  DeviceChannelCountStatCollector;

typedef LogDefaultTraits<DeviceChannelCountStatCollector>
  DeviceChannelCountStatTraits;

} // namespace LogProcessing
} // namespace AdServer

#endif /* AD_SERVER_LOG_PROCESSING_DEVICE_CHANNEL_COUNT_STAT_HPP */

