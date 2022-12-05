#ifndef AD_SERVER_LOG_PROCESSING_CHANNEL_IMP_INVENTORY_HPP
#define AD_SERVER_LOG_PROCESSING_CHANNEL_IMP_INVENTORY_HPP


#include "LogCommons.hpp"
#include "StatCollector.hpp"
#include <LogCommons/GenericLogIoImpl.hpp>

namespace AdServer {
namespace LogProcessing {

class ChannelImpInventoryInnerKey
{
public:
  enum class CCGType
  {
    DISPLAY = 0,
    TEXT
  };

public:
  ChannelImpInventoryInnerKey()
  :
    channel_id_(),
    ccg_type_(),
    hash_()
  {}

  ChannelImpInventoryInnerKey(
    unsigned long channel_id,
    CCGType ccg_type
  )
  :
    channel_id_(channel_id),
    ccg_type_(ccg_type),
    hash_()
  {
    calc_hash_();
  }

  bool operator==(const ChannelImpInventoryInnerKey& rhs) const
  {
    return channel_id_ == rhs.channel_id_ &&
      ccg_type_ == rhs.ccg_type_;
  }

  unsigned long channel_id() const
  {
    return channel_id_;
  }

  char ccg_type() const
  {
    return (ccg_type_ == CCGType::DISPLAY ? 'D' : 'T');
  }

  size_t hash() const
  {
    return hash_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, ChannelImpInventoryInnerKey& key)
    /*throw(eh::Exception)*/;

  friend
  std::ostream&
  operator<<(std::ostream& os, const ChannelImpInventoryInnerKey& key)
    /*throw(eh::Exception)*/;

private:
  void
  calc_hash_()
  {
    hash_ = (channel_id_ << 1) | static_cast<long>(ccg_type_);
  }

  static
  CCGType
  char_to_ccg_type_(char ccg_type)
  {
    if (ccg_type == 'D')
    {
      return CCGType::DISPLAY;
    }
    else if (ccg_type == 'T')
    {
      return CCGType::TEXT;
    }

    Stream::Error es;
    es << "ChannelImpInventoryInnerKey::char_to_ccg_type_(): ccg_type_ "
       << "has invalid value '" << ccg_type << '\'';
    throw ConstraintViolation(es);
  }

private:
  unsigned long channel_id_;
  CCGType ccg_type_;
  size_t hash_;
};

class ChannelImpInventoryInnerData
{
public:
  typedef AdServer::LogProcessing::FixedNumber FixedNum;

  struct Counter
  {
    Counter()
    :
      imps(FixedNum::ZERO),
      user_count(FixedNum::ZERO),
      value(FixedNum::ZERO)
    {
    }

    Counter(
      const FixedNum& imps_val,
      const FixedNum& user_count_val,
      const FixedNum& revenue_val
    )
    :
      imps(imps_val),
      user_count(user_count_val),
      value(revenue_val)
    {
    }

    bool operator==(const Counter& cntr) const
    {
      if (this == &cntr)
      {
        return true;
      }
      return imps == cntr.imps &&
        user_count == cntr.user_count &&
        value == cntr.value;
    }

    const Counter& operator+=(const Counter& rhs)
    {
      imps += rhs.imps;
      user_count += rhs.user_count;
      value += rhs.value;
      return *this;
    }

    FixedNum imps;
    FixedNum user_count;
    FixedNum value;
  };

  ChannelImpInventoryInnerData()
  :
    clicks_(),
    actions_(),
    revenue_(FixedNum::ZERO),
    impops_user_count_(FixedNum::ZERO),
    imp_count_(),
    imp_other_count_(),
    no_impops_count_(),
    meaningful_data_()
  {
  }

  ChannelImpInventoryInnerData(
    unsigned long clicks,
    unsigned long actions,
    const FixedNum& revenue,
    const FixedNum& impops_user_count,
    const Counter& imp_count,
    const Counter& imp_other_count,
    const Counter& no_impops_count
  )
  :
    clicks_(clicks),
    actions_(actions),
    revenue_(revenue),
    impops_user_count_(impops_user_count),
    imp_count_(imp_count),
    imp_other_count_(imp_other_count),
    no_impops_count_(no_impops_count),
    meaningful_data_(true)
  {
  }

  bool operator==(const ChannelImpInventoryInnerData& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return meaningful_data_ == rhs.meaningful_data_ &&
      clicks_ == rhs.clicks_ &&
      actions_ == rhs.actions_ &&
      revenue_ == rhs.revenue_ &&
      impops_user_count_ == rhs.impops_user_count_ &&
      imp_count_ == rhs.imp_count_ &&
      imp_other_count_ == rhs.imp_other_count_ &&
      no_impops_count_ == rhs.no_impops_count_;
  }

  ChannelImpInventoryInnerData&
  operator+=(const ChannelImpInventoryInnerData& rhs)
  {
    clicks_ += rhs.clicks_;
    actions_ += rhs.actions_;
    revenue_ += rhs.revenue_;
    impops_user_count_ += rhs.impops_user_count_;
    imp_count_ += rhs.imp_count_;
    imp_other_count_ += rhs.imp_other_count_;
    no_impops_count_ += rhs.no_impops_count_;
    meaningful_data_ = true;
    return *this;
  }

  unsigned long clicks() const
  {
    return clicks_;
  }

  unsigned long actions() const
  {
    return actions_;
  }

  const FixedNum& revenue() const
  {
    return revenue_;
  }

  const FixedNum& impops_user_count() const
  {
    return impops_user_count_;
  }

  const Counter& imp_count() const
  {
    return imp_count_;
  }

  const Counter& imp_other_count() const
  {
    return imp_other_count_;
  }

  const Counter& no_impops_count() const
  {
    return no_impops_count_;
  }

  template <typename Archive>
  void
  serialize(Archive& ar)
  {
    ar & clicks_;
    ar & actions_;
    ar & revenue_;
    ar & impops_user_count_;
    ar & imp_count_;
    ar & imp_other_count_;
    ar ^ no_impops_count_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is,
    ChannelImpInventoryInnerData& data)
    /*throw(eh::Exception)*/;

  friend
  std::ostream&
  operator<<(std::ostream& os, const ChannelImpInventoryInnerData& data)
    /*throw(eh::Exception)*/;

public:
  //
  // Mediators
  //

  /*
   * OneImpopUserAppearCounter
   */
  struct OneImpopUserAppearCounter
  {
    FixedNum user_count;

    OneImpopUserAppearCounter(const FixedNum& user_count_val)
      : user_count(user_count_val)
    {}
  };

  explicit
  ChannelImpInventoryInnerData(const OneImpopUserAppearCounter& rhs)
    :
    clicks_(),
    actions_(),
    revenue_(FixedNum::ZERO),
    impops_user_count_(rhs.user_count),
    imp_count_(),
    imp_other_count_(),
    no_impops_count_(),
    meaningful_data_()
  {}

  ChannelImpInventoryInnerData&
  operator+= (const OneImpopUserAppearCounter& rhs)
  {
    impops_user_count_ += rhs.user_count;
    return *this;
  }

  /*
   * OneImpUserAppearCounter
   */
  struct OneImpUserAppearCounter
  {
    FixedNum user_count;

    OneImpUserAppearCounter(const FixedNum& user_count_val)
      : user_count(user_count_val)
    {}
  };

  explicit
  ChannelImpInventoryInnerData(const OneImpUserAppearCounter& rhs)
    :
    clicks_(),
    actions_(),
    revenue_(FixedNum::ZERO),
    impops_user_count_(FixedNum::ZERO),
    imp_count_(FixedNum::ZERO, rhs.user_count, FixedNum::ZERO),
    imp_other_count_(),
    no_impops_count_(),
    meaningful_data_()
  {}

  ChannelImpInventoryInnerData&
  operator+= (const OneImpUserAppearCounter& rhs)
  {
    imp_count_.user_count += rhs.user_count;
    return *this;
  }

  /*
   * OneImpOtherUserAppearCounter
   */
  struct OneImpOtherUserAppearCounter
  {
    FixedNum user_count;

    OneImpOtherUserAppearCounter(const FixedNum& user_count_val)
      : user_count(user_count_val)
    {}
  };

  explicit
  ChannelImpInventoryInnerData(const OneImpOtherUserAppearCounter& rhs)
    :
    clicks_(),
    actions_(),
    revenue_(FixedNum::ZERO),
    impops_user_count_(FixedNum::ZERO),
    imp_count_(),
    imp_other_count_(FixedNum::ZERO, rhs.user_count, FixedNum::ZERO),
    no_impops_count_(),
    meaningful_data_()
  {}

  ChannelImpInventoryInnerData&
  operator+= (const OneImpOtherUserAppearCounter& rhs)
  {
    imp_other_count_.user_count += rhs.user_count;
    return *this;
  }

  /*
   * OneImpopNoImpUserAppearCounter
   */
  struct OneImpopNoImpUserAppearCounter
  {
    FixedNum user_count;

    OneImpopNoImpUserAppearCounter(const FixedNum& user_count_val)
      : user_count(user_count_val)
    {}
  };

  explicit
  ChannelImpInventoryInnerData(const OneImpopNoImpUserAppearCounter& rhs)
    :
    clicks_(),
    actions_(),
    revenue_(FixedNum::ZERO),
    impops_user_count_(FixedNum::ZERO),
    imp_count_(),
    imp_other_count_(),
    no_impops_count_(FixedNum::ZERO, rhs.user_count, FixedNum::ZERO),
    meaningful_data_()
  {}
	 
  ChannelImpInventoryInnerData&
  operator+= (const OneImpopNoImpUserAppearCounter& rhs)
  {
    no_impops_count_.user_count += rhs.user_count;
    return *this;
  }

  /*
   * ImpCountRevenueCounter
   */
  struct ImpCountRevenueCounter
  {
    FixedNum revenue;

    ImpCountRevenueCounter(const FixedNum& revenue_val)
      : revenue(revenue_val)
    {}
  };

  explicit
  ChannelImpInventoryInnerData(const ImpCountRevenueCounter& rhs)
    :
    clicks_(),
    actions_(),
    revenue_(FixedNum::ZERO),
    impops_user_count_(FixedNum::ZERO),
    imp_count_(FixedNum::ZERO, FixedNum::ZERO, rhs.revenue),
    imp_other_count_(),
    no_impops_count_(),
    meaningful_data_()
  {}

  ChannelImpInventoryInnerData&
  operator+= (const ImpCountRevenueCounter& rhs)
  {
    imp_count_.value += rhs.revenue;
    return *this;
  }

  /*
   * ImpOtherCountImpsAndRevenueCounter
   */
  struct ImpOtherImpsAndRevenueCounter
  {
    FixedNum imps;
    FixedNum revenue;

    ImpOtherImpsAndRevenueCounter(
      const FixedNum& imps_val,
      const FixedNum& revenue_val)
      : imps(imps_val), revenue(revenue_val)
    {}
  };

  explicit
  ChannelImpInventoryInnerData(const ImpOtherImpsAndRevenueCounter& rhs)
    : 	 
    clicks_(),
    actions_(),
    revenue_(FixedNum::ZERO),
    impops_user_count_(FixedNum::ZERO),
    imp_count_(),
    imp_other_count_(rhs.imps, FixedNum::ZERO, rhs.revenue),
    no_impops_count_(),
    meaningful_data_()
  {}

  ChannelImpInventoryInnerData&
  operator+= (const ImpOtherImpsAndRevenueCounter& rhs)
  {
    imp_other_count_.imps += rhs.imps;
    imp_other_count_.value += rhs.revenue;
    return *this;
  }

  /*
   * NoImpopsCountImpsAndRevenueCounter
   */
  struct NoImpopsImpsAndRevenueCounter
  {
    FixedNum imps;
    FixedNum revenue;

    NoImpopsImpsAndRevenueCounter(
      const FixedNum& imps_val,
      const FixedNum& revenue_val)
      : imps(imps_val), revenue(revenue_val)
    {}
  };

  explicit
  ChannelImpInventoryInnerData(const NoImpopsImpsAndRevenueCounter& rhs)
    :
    clicks_(),
    actions_(),
    revenue_(FixedNum::ZERO),
    impops_user_count_(FixedNum::ZERO),
    imp_count_(),
    imp_other_count_(),
    no_impops_count_(rhs.imps, FixedNum::ZERO, rhs.revenue),
    meaningful_data_()
  {}

  ChannelImpInventoryInnerData&
  operator+= (const NoImpopsImpsAndRevenueCounter& rhs)
  {
    no_impops_count_.imps += rhs.imps;
    no_impops_count_.value += rhs.revenue;
    return *this;
  }

private:
  unsigned long clicks_;
  unsigned long actions_;
  FixedNum revenue_;
  FixedNum impops_user_count_;
  Counter imp_count_;
  Counter imp_other_count_;
  Counter no_impops_count_;
  bool meaningful_data_; // workaround to prevent StatCollector from ignoring
                         // ChannelImpInventory entries with zero data
};

struct ChannelImpInventoryKey
{
  ChannelImpInventoryKey(): sdate_(), colo_id_(), hash_() {}

  ChannelImpInventoryKey(
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

  bool operator==(const ChannelImpInventoryKey& rhs) const
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
  operator>>(std::istream& is, ChannelImpInventoryKey& key);

  friend
  std::ostream&
  operator<<(std::ostream& os, const ChannelImpInventoryKey& key)
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
          ChannelImpInventoryInnerKey,
          ChannelImpInventoryInnerData,
          false,
          true
        > ChannelImpInventoryInnerCollector;

typedef ChannelImpInventoryInnerCollector ChannelImpInventoryData;

typedef StatCollector<ChannelImpInventoryKey, ChannelImpInventoryData>
  ChannelImpInventoryCollector;

typedef LogDefaultTraits<ChannelImpInventoryCollector> ChannelImpInventoryTraits;

} // namespace LogProcessing
} // namespace AdServer

#endif /* AD_SERVER_LOG_PROCESSING_CHANNEL_IMP_INVENTORY_HPP */

