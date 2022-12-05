#ifndef AD_SERVER_LOG_PROCESSING_CHANNEL_TRIGGER_IMP_STAT_HPP
#define AD_SERVER_LOG_PROCESSING_CHANNEL_TRIGGER_IMP_STAT_HPP


#include <iosfwd>
#include <Generics/Time.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <LogCommons/LogCommons.hpp>
#include <LogCommons/StatCollector.hpp>

namespace AdServer {
namespace LogProcessing {

class ChannelTriggerImpStatInnerKey_V_2_3
{
public:
  ChannelTriggerImpStatInnerKey_V_2_3()
  :
    channel_trigger_id_(),
    type_(),
    hash_()
  {
  }

  ChannelTriggerImpStatInnerKey_V_2_3(
    unsigned long channel_trigger_id,
    char type
  )
  :
    channel_trigger_id_(channel_trigger_id),
    type_(type),
    hash_()
  {
    calc_hash_();
  }

  bool operator==(const ChannelTriggerImpStatInnerKey_V_2_3& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return channel_trigger_id_ == rhs.channel_trigger_id_ &&
      type_ == rhs.type_;
  }

  unsigned long channel_trigger_id() const
  {
    return channel_trigger_id_;
  }

  char type() const
  {
    return type_;
  }

  size_t hash() const
  {
    return hash_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is,
    ChannelTriggerImpStatInnerKey_V_2_3& key)
    /*throw(eh::Exception)*/;

private:
  void calc_hash_()
  {
    Generics::Murmur64Hash hasher(hash_);
    hash_add(hasher, channel_trigger_id_);
    hash_add(hasher, type_);
  }

  bool type_is_valid_() const
  {
    if (type_ == 'P' || type_ == 'S' || type_ == 'U' || type_ == 'R')
    {
      return true;
    }
    return false;
  }

  void invariant() const /*throw(eh::Exception)*/
  {
    if (!type_is_valid_())
    {
      Stream::Error es;
      es << "ChannelTriggerImpStatInnerKey_V_2_3::invariant(): type_ "
         << "has invalid value '" << type_ << '\'';
      throw ConstraintViolation(es);
    }
  }

  unsigned long channel_trigger_id_;
  char type_;
  size_t hash_;
};

class ChannelTriggerImpStatInnerKey
{
public:
  ChannelTriggerImpStatInnerKey()
  :
    channel_trigger_id_(),
    channel_id_(),
    type_(),
    hash_()
  {
  }

  ChannelTriggerImpStatInnerKey(
    unsigned long channel_trigger_id,
    unsigned long channel_id,
    char type
  )
  :
    channel_trigger_id_(channel_trigger_id),
    channel_id_(channel_id),
    type_(type),
    hash_()
  {
    calc_hash_();
  }

  ChannelTriggerImpStatInnerKey(
    const ChannelTriggerImpStatInnerKey_V_2_3& key
  )
  :
    channel_trigger_id_(key.channel_trigger_id()),
    channel_id_(),
    type_(key.type()),
    hash_()
  {
    calc_hash_();
  }

  bool operator==(const ChannelTriggerImpStatInnerKey& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return channel_trigger_id_ == rhs.channel_trigger_id_ &&
      channel_id_ == rhs.channel_id_ &&
      type_ == rhs.type_;
  }

  unsigned long channel_trigger_id() const
  {
    return channel_trigger_id_;
  }

  unsigned long channel_id() const
  {
    return channel_id_;
  }

  char type() const
  {
    return type_;
  }

  size_t hash() const
  {
    return hash_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is,
    ChannelTriggerImpStatInnerKey& key)
    /*throw(eh::Exception)*/;

  friend
  std::ostream&
  operator<<(std::ostream& os, const ChannelTriggerImpStatInnerKey& key)
    /*throw(eh::Exception)*/;

private:
  void calc_hash_()
  {
    Generics::Murmur64Hash hasher(hash_);
    hash_add(hasher, channel_trigger_id_);
    hash_add(hasher, channel_id_);
    hash_add(hasher, type_);
  }

  bool type_is_valid_() const
  {
    if (type_ == 'P' || type_ == 'S' || type_ == 'U' || type_ == 'R')
    {
      return true;
    }
    return false;
  }

  void invariant() const /*throw(eh::Exception)*/
  {
    if (!type_is_valid_())
    {
      Stream::Error es;
      es << "ChannelTriggerImpStatInnerKey::invariant(): type_ "
         << "has invalid value '" << type_ << '\'';
      throw ConstraintViolation(es);
    }
  }

  unsigned long channel_trigger_id_;
  unsigned long channel_id_;
  char type_;
  size_t hash_;
};

class ChannelTriggerImpStatInnerData
{
public:
  typedef AdServer::LogProcessing::FixedNumber FixedNum;

  ChannelTriggerImpStatInnerData()
  :
    approximated_imps_(FixedNum::ZERO),
    approximated_clicks_(FixedNum::ZERO)
  {
  }

  ChannelTriggerImpStatInnerData(
    const FixedNum& approximated_imps,
    const FixedNum& approximated_clicks
  )
  :
    approximated_imps_(approximated_imps),
    approximated_clicks_(approximated_clicks)
  {
  }

  bool operator==(const ChannelTriggerImpStatInnerData& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return approximated_imps_ == rhs.approximated_imps_ &&
      approximated_clicks_ == rhs.approximated_clicks_;
  }

  ChannelTriggerImpStatInnerData&
  operator+=(const ChannelTriggerImpStatInnerData& rhs)
  {
    approximated_imps_ += rhs.approximated_imps_;
    approximated_clicks_ += rhs.approximated_clicks_;
    return *this;
  }

  const FixedNum& approximated_imps() const
  {
    return approximated_imps_;
  }

  const FixedNum& approximated_clicks() const
  {
    return approximated_clicks_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is,
    ChannelTriggerImpStatInnerData& data)
    /*throw(eh::Exception)*/;

  friend
  std::ostream&
  operator<<(std::ostream& os, const ChannelTriggerImpStatInnerData& data)
    /*throw(eh::Exception)*/;

private:
  void invariant() const /*throw(eh::Exception)*/
  {
  }

  FixedNum approximated_imps_;
  FixedNum approximated_clicks_;
};

struct ChannelTriggerImpStatKey
{
  ChannelTriggerImpStatKey(): sdate_(), colo_id_(), hash_() {}

  ChannelTriggerImpStatKey(
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

  bool operator==(const ChannelTriggerImpStatKey& rhs) const
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
  operator>>(std::istream& is, ChannelTriggerImpStatKey& key);

  friend
  std::ostream&
  operator<<(std::ostream& os, const ChannelTriggerImpStatKey& key)
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
          ChannelTriggerImpStatKey,
          StatCollector<
            ChannelTriggerImpStatInnerKey_V_2_3,
            ChannelTriggerImpStatInnerData,
            false,
            true
          >
        > ChannelTriggerImpStatCollector_V_2_3;

typedef StatCollector<
          ChannelTriggerImpStatInnerKey,
          ChannelTriggerImpStatInnerData,
          false,
          true
        > ChannelTriggerImpStatInnerCollector;

typedef ChannelTriggerImpStatInnerCollector ChannelTriggerImpStatData;

typedef StatCollector<
          ChannelTriggerImpStatKey,
          ChannelTriggerImpStatData
        > ChannelTriggerImpStatCollector;

struct ChannelTriggerImpStatTraits:
  LogDefaultTraits<ChannelTriggerImpStatCollector>
{
  template <class FUNCTOR_>
  static
  void
  for_each_old(FUNCTOR_& f) /*throw(eh::Exception)*/
  {
    f.template operator()<ChannelTriggerImpStatCollector_V_2_3, true>("2.3");
  }
};

} // namespace LogProcessing
} // namespace AdServer

#endif /* AD_SERVER_LOG_PROCESSING_CHANNEL_TRIGGER_IMP_STAT_HPP */

