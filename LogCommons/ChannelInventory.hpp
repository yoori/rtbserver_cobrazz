#ifndef AD_SERVER_LOG_PROCESSING_CHANNEL_INVENTORY_HPP
#define AD_SERVER_LOG_PROCESSING_CHANNEL_INVENTORY_HPP


#include "LogCommons.hpp"
#include "StatCollector.hpp"
#include <LogCommons/GenericLogIoImpl.hpp>

namespace AdServer {
namespace LogProcessing {

class ChannelInventoryInnerKey
{
public:
  explicit
  ChannelInventoryInnerKey(
    unsigned long channel_id = 0
  )
  :
    channel_id_(channel_id)
  {
  }

  bool operator==(const ChannelInventoryInnerKey& rhs) const
  {
    return channel_id_ == rhs.channel_id_;
  }

  unsigned long channel_id() const
  {
    return channel_id_;
  }

  size_t hash() const
  {
    return channel_id_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, ChannelInventoryInnerKey& key)
    /*throw(eh::Exception)*/;

  friend
  std::ostream&
  operator<<(std::ostream& os, const ChannelInventoryInnerKey& key)
    /*throw(eh::Exception)*/;

private:
  unsigned long channel_id_;
};

class ChannelInventoryInnerData
{
public:
  typedef AdServer::LogProcessing::FixedNumber FixedNum;

  ChannelInventoryInnerData()
  :
    sum_user_ecpm_(FixedNum::ZERO),
    active_user_count_(FixedNum::ZERO),
    total_user_count_(FixedNum::ZERO),
    meaningful_data_()
  {
  }

  ChannelInventoryInnerData(
    const FixedNum& sum_user_ecpm,
    const FixedNum& active_user_count,
    const FixedNum& total_user_count
  )
  :
    sum_user_ecpm_(sum_user_ecpm),
    active_user_count_(active_user_count),
    total_user_count_(total_user_count),
    meaningful_data_(true)
  {
  }

  bool operator==(const ChannelInventoryInnerData& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return meaningful_data_ == rhs.meaningful_data_ &&
      sum_user_ecpm_ == rhs.sum_user_ecpm_ &&
      active_user_count_ == rhs.active_user_count_ &&
      total_user_count_ == rhs.total_user_count_;
  }

  ChannelInventoryInnerData&
  operator+=(const ChannelInventoryInnerData& rhs)
  {
    sum_user_ecpm_ += rhs.sum_user_ecpm_;
    active_user_count_ += rhs.active_user_count_;
    total_user_count_ += rhs.total_user_count_;
    meaningful_data_ = true;
    return *this;
  }

  const FixedNum& sum_user_ecpm() const
  {
    return sum_user_ecpm_;
  }

  const FixedNum& active_user_count() const
  {
    return active_user_count_;
  }

  const FixedNum& total_user_count() const
  {
    return total_user_count_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, ChannelInventoryInnerData& data)
    /*throw(eh::Exception)*/;

  friend
  std::ostream&
  operator<<(std::ostream& os, const ChannelInventoryInnerData& data)
    /*throw(eh::Exception)*/;

private:
  FixedNum sum_user_ecpm_;
  FixedNum active_user_count_;
  FixedNum total_user_count_;
  bool meaningful_data_; // workaround to prevent StatCollector from ignoring
                         // ChannelInventory entries with zero data
};

struct ChannelInventoryKey
{
  ChannelInventoryKey(): sdate_(), colo_id_(), hash_() {}

  ChannelInventoryKey(
    const DayTimestamp& sdate,
    unsigned long colo_id
  )
  :
    sdate_(sdate),
    colo_id_(colo_id),
    hash_()
  {
    calc_hash_();
  }

  bool operator==(const ChannelInventoryKey& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return sdate_ == rhs.sdate_ && colo_id_ == rhs.colo_id_;
  }

  const DayTimestamp& sdate() const
  {
    return sdate_;
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
  operator>>(std::istream& is, ChannelInventoryKey& key);

  friend
  std::ostream&
  operator<<(std::ostream& os, const ChannelInventoryKey& key)
    /*throw(eh::Exception)*/;

private:
  void calc_hash_()
  {
    Generics::Murmur64Hash hasher(hash_);
    sdate_.hash_add(hasher);
    hash_add(hasher, colo_id_);
  }

  DayTimestamp sdate_;
  unsigned long colo_id_;
  size_t hash_;
};

typedef StatCollector<
          ChannelInventoryInnerKey, ChannelInventoryInnerData, false, true
        > ChannelInventoryInnerCollector;

typedef ChannelInventoryInnerCollector ChannelInventoryData;

typedef StatCollector<ChannelInventoryKey, ChannelInventoryData>
  ChannelInventoryCollector;

typedef LogDefaultTraits<ChannelInventoryCollector> ChannelInventoryTraits;

} // namespace LogProcessing
} // namespace AdServer

#endif /* AD_SERVER_LOG_PROCESSING_CHANNEL_INVENTORY_HPP */

