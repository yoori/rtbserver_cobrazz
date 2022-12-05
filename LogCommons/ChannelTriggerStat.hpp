#ifndef AD_SERVER_LOG_PROCESSING_CHANNEL_TRIGGER_STAT_HPP
#define AD_SERVER_LOG_PROCESSING_CHANNEL_TRIGGER_STAT_HPP


#include <iosfwd>
#include <String/AsciiStringManip.hpp>
#include <Commons/StringHolder.hpp>
#include <LogCommons/LogCommons.hpp>
#include <LogCommons/StatCollector.hpp>

namespace AdServer {
namespace LogProcessing {

class ChannelTriggerStatInnerKey_V_1_2
{
public:
  ChannelTriggerStatInnerKey_V_1_2()
  :
    channel_id_(),
    type_(),
    trigger_(),
    hash_()
  {
  }

  ChannelTriggerStatInnerKey_V_1_2(
    unsigned long channel_id,
    char type,
    AdServer::Commons::StringHolder* trigger
  )
  :
    channel_id_(channel_id),
    type_(type),
    trigger_(ReferenceCounting::add_ref(trigger)),
    hash_()
  {
    calc_hash_();
  }

  bool operator==(const ChannelTriggerStatInnerKey_V_1_2& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return channel_id_ == rhs.channel_id_ &&
      type_ == rhs.type_ &&
      trigger_->str() == rhs.trigger_->str();
  }

  unsigned long channel_id() const
  {
    return channel_id_;
  }

  char type() const
  {
    return type_;
  }

  const std::string& trigger() const
  {
    return trigger_->str();
  }

  size_t hash() const
  {
    return hash_;
  }

  friend FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is,
    ChannelTriggerStatInnerKey_V_1_2& key)
    /*throw(eh::Exception)*/;

  friend std::ostream&
  operator<<(std::ostream& os, const ChannelTriggerStatInnerKey_V_1_2& key)
    /*throw(eh::Exception)*/;

private:
  void calc_hash_()
  {
    Generics::Murmur64Hash hasher(hash_);
    hash_add(hasher, channel_id_);
    hash_add(hasher, trigger_->str());
    hash_add(hasher, type_);
  }

  bool type_is_valid_() const
  {
    return type_ == 'C' || type_ == 'P' || type_ == 'S' || type_ == 'U';
  }

  void invariant() const /*throw(eh::Exception)*/
  {
    const String::AsciiStringManip::Char1Category<'\n'> illegal_chars;

    if (!type_is_valid_())
    {
      Stream::Error es;
      es << "ChannelTriggerStatInnerKey_V_1_2::invariant(): type_ "
         << "has invalid value '" << type_ << '\'';
      throw ConstraintViolation(es);
    }
    String::SubString tr(trigger_->str());
    if (tr.empty())
    {
      throw ConstraintViolation("ChannelTriggerStatInnerKey_V_1_2::"
        "invariant(): trigger_ must be non-empty");
    }
    const char* ptr = illegal_chars.find_owned(tr.begin(), tr.end());
    if (ptr != tr.end())
    {
      Stream::Error es;
      es << "ChannelTriggerStatInnerKey_V_1_2::invariant(): trigger_ contains "
         << "illegal char with code " << std::showbase << std::hex
         << unsigned(*ptr);
      throw ConstraintViolation(es);
    }
  }

  unsigned long channel_id_;
  char type_;
  AdServer::Commons::StringHolder_var trigger_;
  size_t hash_;
};

class ChannelTriggerStatInnerKey_V_2_4
{
public:
  ChannelTriggerStatInnerKey_V_2_4()
  :
    type_(),
    channel_trigger_id_(),
    hash_()
  {
  }

  ChannelTriggerStatInnerKey_V_2_4(
    char type,
    unsigned long channel_trigger_id
  )
  :
    type_(type),
    channel_trigger_id_(channel_trigger_id),
    hash_()
  {
    calc_hash_();
  }

  bool operator==(const ChannelTriggerStatInnerKey_V_2_4& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return type_ == rhs.type_ &&
      channel_trigger_id_ == rhs.channel_trigger_id_;
  }

  char type() const
  {
    return type_;
  }

  unsigned long channel_trigger_id() const
  {
    return channel_trigger_id_;
  }

  size_t hash() const
  {
    return hash_;
  }

  friend FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is,
    ChannelTriggerStatInnerKey_V_2_4& key)
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
    return type_ == 'P' || type_ == 'R' || type_ == 'S' || type_ == 'U';
  }

  void invariant() const /*throw(eh::Exception)*/
  {
    if (!type_is_valid_())
    {
      Stream::Error es;
      es << "ChannelTriggerStatInnerKey_V_2_4::invariant(): type_ "
         << "has invalid value '" << type_ << '\'';
      throw ConstraintViolation(es);
    }
  }

  char type_;
  unsigned long channel_trigger_id_;
  size_t hash_;
};

class ChannelTriggerStatInnerKey
{
public:
  ChannelTriggerStatInnerKey()
  :
    channel_trigger_id_(),
    channel_id_(),
    type_(),
    hash_()
  {
  }

  ChannelTriggerStatInnerKey(
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

  ChannelTriggerStatInnerKey(const ChannelTriggerStatInnerKey& init)
    : channel_trigger_id_(init.channel_trigger_id_),
      channel_id_(init.channel_id_),
      type_(init.type_),
      hash_(init.hash_)
  {}

  ChannelTriggerStatInnerKey(
    const ChannelTriggerStatInnerKey_V_2_4& key
  )
  :
    channel_trigger_id_(key.channel_trigger_id()),
    channel_id_(),
    type_(key.type()),
    hash_()
  {
    calc_hash_();
  }

  bool operator==(const ChannelTriggerStatInnerKey& rhs) const
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

  friend FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, ChannelTriggerStatInnerKey& key)
    /*throw(eh::Exception)*/;

  friend std::ostream&
  operator<<(std::ostream& os, const ChannelTriggerStatInnerKey& key)
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
    return type_ == 'P' || type_ == 'R' || type_ == 'S' || type_ == 'U';
  }

  void invariant() const /*throw(eh::Exception)*/
  {
    if (!type_is_valid_())
    {
      Stream::Error es;
      es << "ChannelTriggerStatInnerKey::invariant(): type_ "
         << "has invalid value '" << type_ << '\'';
      throw ConstraintViolation(es);
    }
  }

  unsigned long channel_trigger_id_;
  unsigned long channel_id_;
  char type_;
  size_t hash_;
};

class ChannelTriggerStatInnerData
{
public:
  ChannelTriggerStatInnerData()
  :
    hits_()
  {
  }

  explicit
  ChannelTriggerStatInnerData(unsigned long hits)
  :
    hits_(hits)
  {
  }

  bool operator==(const ChannelTriggerStatInnerData& rhs) const
  {
    return hits_ == rhs.hits_;
  }

  ChannelTriggerStatInnerData&
  operator+=(const ChannelTriggerStatInnerData& rhs)
  {
    hits_ += rhs.hits_;
    return *this;
  }

  unsigned long hits() const
  {
    return hits_;
  }

  friend FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is,
    ChannelTriggerStatInnerData& data)
    /*throw(eh::Exception)*/;

  friend std::ostream&
  operator<<(std::ostream& os, const ChannelTriggerStatInnerData& data)
    /*throw(eh::Exception)*/;

private:
  void invariant() const /*throw(eh::Exception)*/
  {
    if (!hits_)
    {
      throw ConstraintViolation("ChannelTriggerStatInnerData::invariant(): "
        "hits_ must be > 0");
    }
  }

  unsigned long hits_;
};

struct ChannelTriggerStatKey
{
  ChannelTriggerStatKey(): sdate_(), colo_id_(), hash_() {}

  ChannelTriggerStatKey(
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

  bool operator==(const ChannelTriggerStatKey& rhs) const
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

  friend std::istream&
  operator>>(std::istream& is, ChannelTriggerStatKey& key);

  friend std::ostream&
  operator<<(std::ostream& os, const ChannelTriggerStatKey& key)
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
          ChannelTriggerStatKey,
          StatCollector<
            ChannelTriggerStatInnerKey_V_1_2,
            ChannelTriggerStatInnerData,
            false,
            true
          >
        > ChannelTriggerStatCollector_V_1_2;

typedef StatCollector<
          ChannelTriggerStatKey,
          StatCollector<
            ChannelTriggerStatInnerKey_V_2_4,
            ChannelTriggerStatInnerData,
            false,
            true
          >
        > ChannelTriggerStatCollector_V_2_4;

typedef StatCollector<
          ChannelTriggerStatKey,
          StatCollector<
            ChannelTriggerStatInnerKey,
            ChannelTriggerStatInnerData,
            false,
            true
          >
        > ChannelTriggerStatCollector;

struct ChannelTriggerStatTraits: LogDefaultTraits<ChannelTriggerStatCollector>
{
  template <class FUNCTOR_>
  static
  void
  for_each_old(FUNCTOR_& f) /*throw(eh::Exception)*/
  {
    f.template operator()<ChannelTriggerStatCollector_V_2_4, true>("2.4");
  }
};

} // namespace LogProcessing
} // namespace AdServer

#endif /* AD_SERVER_LOG_PROCESSING_CHANNEL_TRIGGER_STAT_HPP */

