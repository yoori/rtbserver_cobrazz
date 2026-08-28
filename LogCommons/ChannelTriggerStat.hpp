#pragma once

#include <iosfwd>
#include <LogCommons/LogCommons.hpp>
#include <LogCommons/StatCollector.hpp>

namespace AdServer::LogProcessing
{
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

    ChannelTriggerStatInnerKey_V_2_4(char type, std::uint32_t channel_trigger_id)
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
      return type_ == rhs.type_ && channel_trigger_id_ == rhs.channel_trigger_id_;
    }

    char type() const
    {
      return type_;
    }

    std::uint32_t channel_trigger_id() const
    {
      return channel_trigger_id_;
    }

    size_t hash() const
    {
      return hash_;
    }

    friend FixedBufStream<TabCategory>&
    operator>>(FixedBufStream<TabCategory>& is, ChannelTriggerStatInnerKey_V_2_4& key)
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
    std::uint32_t channel_trigger_id_;
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
      std::uint32_t channel_trigger_id,
      std::uint32_t channel_id,
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

    ChannelTriggerStatInnerKey(const ChannelTriggerStatInnerKey_V_2_4& key)
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
        channel_id_ == rhs.channel_id_ && type_ == rhs.type_;
    }

    std::uint32_t channel_trigger_id() const
    {
      return channel_trigger_id_;
    }

    std::uint32_t channel_id() const
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

    friend BufferWriter&
    operator<<(BufferWriter& out, const ChannelTriggerStatInnerKey& key)
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

    std::uint32_t channel_trigger_id_;
    std::uint32_t channel_id_;
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
    operator>>(FixedBufStream<TabCategory>& is, ChannelTriggerStatInnerData& data)
      /*throw(eh::Exception)*/;

    friend BufferWriter&
    operator<<(BufferWriter& out, const ChannelTriggerStatInnerData& data)
      /*throw(eh::Exception)*/;

  private:
    void invariant() const /*throw(eh::Exception)*/
    {
      if (!hits_)
      {
        throw ConstraintViolation("ChannelTriggerStatInnerData::invariant(): " "hits_ must be > 0");
      }
    }

    unsigned long hits_;
  };

  struct ChannelTriggerStatKey
  {
    ChannelTriggerStatKey(): sdate_(), colo_id_(), hash_() {}

    ChannelTriggerStatKey(const DayTimestamp& sdate, std::uint32_t colo_id)
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

    std::uint32_t colo_id() const
    {
      return colo_id_;
    }

    size_t hash() const
    {
      return hash_;
    }

    friend std::istream&
    operator>>(std::istream& is, ChannelTriggerStatKey& key);

    friend BufferWriter&
    operator<<(BufferWriter& out, const ChannelTriggerStatKey& key)
      /*throw(eh::Exception)*/;

  private:
    void calc_hash_()
    {
      Generics::Murmur64Hash hasher(hash_);
      sdate_.hash_add(hasher);
      hash_add(hasher, colo_id_);
    }

    DayTimestamp sdate_;
    std::uint32_t colo_id_;
    size_t hash_;
  };

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
} // namespace AdServer::LogProcessing
