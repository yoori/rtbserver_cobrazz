#ifndef AD_SERVER_LOG_PROCESSING_ACTION_STAT_HPP
#define AD_SERVER_LOG_PROCESSING_ACTION_STAT_HPP


#include <iosfwd>
#include <istream>
#include <ostream>
#include <Generics/Time.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <Commons/UserInfoManip.hpp>
#include "LogCommons.hpp"
#include "StatCollector.hpp"

namespace AdServer {
namespace LogProcessing {

namespace v3_2_0_28
{
  class ActionStatInnerKey
  {
  public:
    ActionStatInnerKey()
    :
      action_request_id_(),
      request_id_(),
      cc_id_(),
      hash_()
    {}

    ActionStatInnerKey(
      const RequestId& action_request_id,
      const RequestId& request_id,
      unsigned long cc_id
    )
    :
      action_request_id_(action_request_id),
      request_id_(request_id),
      cc_id_(cc_id),
      hash_()
    {
      calc_hash_();
    }

    bool operator==(const ActionStatInnerKey& rhs) const
    {
      return this == &rhs ||
        (action_request_id_ == rhs.action_request_id_ &&
        request_id_ == rhs.request_id_ &&
        cc_id_ == rhs.cc_id_);
    }

    const RequestId& action_request_id() const
    {
      return action_request_id_;
    }

    const RequestId& request_id() const
    {
      return request_id_;
    }

    unsigned long cc_id() const
    {
      return cc_id_;
    }

    size_t hash() const
    {
      return hash_;
    }

    friend FixedBufStream<TabCategory>&
    operator>>(FixedBufStream<TabCategory>& is, ActionStatInnerKey& key);

    friend std::ostream&
    operator<<(std::ostream& os, const ActionStatInnerKey& key);

  private:
    void calc_hash_()
    {
      Generics::Murmur64Hash hasher(hash_);
      hash_add(hasher, cc_id_);
      hash_add(hasher, static_cast<const RequestId&>(action_request_id_));
      hash_add(hasher, static_cast<const RequestId&>(request_id_));
    }

    RequestIdIoWrapper action_request_id_;
    RequestIdIoWrapper request_id_;
    unsigned long cc_id_;
    size_t hash_;
  };

  class ActionStatInnerData
  {
    typedef StringIoWrapperOptional OptionalStringT;

  public:
    typedef AdServer::LogProcessing::FixedNumber FixedNum;

    ActionStatInnerData()
    {}

    ActionStatInnerData(
      unsigned long action_id,
      unsigned long tag_id,
      const std::string& order_id,
      const std::string& country_code,
      const std::string& referrer,
      const DayHourTimestamp& imp_date,
      const FixedNum& cur_value
    )
    :
      holder_(
        new DataHolder(
          action_id,
          tag_id,
          order_id,
          country_code,
          referrer,
          imp_date,
          OptionalDayHourTimestamp(),
          cur_value
        )
      )
    {}

    ActionStatInnerData(
      unsigned long action_id,
      unsigned long tag_id,
      const std::string& order_id,
      const std::string& country_code,
      const std::string& referrer,
      const DayHourTimestamp& imp_date,
      const DayHourTimestamp& click_date,
      const FixedNum& cur_value
    )
    :
      holder_(
        new DataHolder(
          action_id,
          tag_id,
          order_id,
          country_code,
          referrer,
          imp_date,
          click_date,
          cur_value
        )
      )
    {}

    bool operator==(const ActionStatInnerData& data) const
    {
      return holder_.in() == data.holder_.in() ||
        (holder_->action_id == data.holder_->action_id &&
        holder_->tag_id == data.holder_->tag_id &&
        holder_->order_id == data.holder_->order_id &&
        holder_->country_code == data.holder_->country_code &&
        holder_->referrer == data.holder_->referrer &&
        holder_->imp_date == data.holder_->imp_date &&
        holder_->click_date == data.holder_->click_date &&
        holder_->cur_value == data.holder_->cur_value);
    }

    ActionStatInnerData& operator+=(const ActionStatInnerData& rhs)
    {
      if (!holder_->click_date.present() && rhs.holder_->click_date.present())
      {
        holder_ = rhs.holder_;
      }
      return *this;
    }

    unsigned long action_id() const
    {
      return holder_->action_id;
    }

    unsigned long tag_id() const
    {
      return holder_->tag_id;
    }

    const std::string& order_id() const
    {
      return holder_->order_id.get();
    }

    const OptionalStringT& country_code() const
    {
      return holder_->country_code;
    }

    const std::string& referrer() const
    {
      return holder_->referrer.get();
    }

    const DayHourTimestamp& imp_date() const
    {
      return holder_->imp_date;
    }

    const OptionalDayHourTimestamp& click_date() const
    {
      return holder_->click_date;
    }

    const FixedNum& cur_value() const
    {
      return holder_->cur_value;
    }

    friend FixedBufStream<TabCategory>&
    operator>>(FixedBufStream<TabCategory>& is, ActionStatInnerData& data)
      /*throw(eh::Exception)*/;

    friend std::ostream&
    operator<<(std::ostream& os, const ActionStatInnerData& data)
      /*throw(eh::Exception)*/;

  private:
    struct DataHolder: public ReferenceCounting::AtomicImpl
    {
      DataHolder()
        : action_id(0), tag_id(0)
      {}

      DataHolder(
        unsigned long action_id_val,
        unsigned long tag_id_val,
        const std::string& order_id_val,
        const std::string& country_code_val,
        const std::string& referrer_val,
        const DayHourTimestamp& imp_date_val,
        const OptionalDayHourTimestamp& click_date_val,
        const FixedNum& cur_value_val
      )
      :
        action_id(action_id_val),
        tag_id(tag_id_val),
        order_id(order_id_val),
        country_code(country_code_val),
        imp_date(imp_date_val),
        click_date(click_date_val),
        cur_value(cur_value_val)
      {
        referrer.set("");
        trim(referrer.get(), referrer_val, 2048);
      }

      unsigned long action_id;
      unsigned long tag_id;
      OptionalStringT order_id;
      OptionalStringT country_code;
      OptionalStringT referrer;
      DayHourTimestamp imp_date;
      OptionalDayHourTimestamp click_date;
      FixedNum cur_value;

    private:
      virtual ~DataHolder() noexcept {}
    };

    typedef ReferenceCounting::SmartPtr<DataHolder> DataHolder_var;

  private:
    DataHolder_var holder_;
  };

  struct ActionStatKey
  {
    ActionStatKey(): sdate_(), colo_id_(), hash_() {}

    ActionStatKey(
      const DayHourTimestamp& sdate,
      unsigned long colo_id
    )
    :
      sdate_(sdate),
      colo_id_(colo_id),
      hash_()
    {
      calc_hash_();
    }

    bool operator==(const ActionStatKey& rhs) const
    {
      if (&rhs == this)
      {
        return true;
      }
      return sdate_ == rhs.sdate_ && colo_id_ == rhs.colo_id_;
    }

    const DayHourTimestamp& sdate() const
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
    operator>>(std::istream& is, ActionStatKey& key);

    friend std::ostream&
    operator<<(std::ostream& os, const ActionStatKey& key) /*throw(eh::Exception)*/;

  private:
    void calc_hash_()
    {
      Generics::Murmur64Hash hasher(hash_);
      hash_add(hasher, colo_id_);
      sdate_.hash_add(hasher);
    }

    DayHourTimestamp sdate_;
    unsigned long colo_id_;
    size_t hash_;
  };

  typedef StatCollector<ActionStatInnerKey, ActionStatInnerData, false, true>
    ActionStatInnerCollector;

  typedef ActionStatInnerCollector ActionStatData;

  typedef StatCollector<ActionStatKey, ActionStatData> ActionStatCollector;
} // namespace v3_2_0_28

class ActionStatInnerKey
{
public:
  ActionStatInnerKey()
  :
    action_request_id_(),
    request_id_(),
    cc_id_(),
    hash_()
  {}

  ActionStatInnerKey(
    const RequestId& action_request_id,
    const RequestId& request_id,
    unsigned long cc_id
  )
  :
    action_request_id_(action_request_id),
    request_id_(request_id),
    cc_id_(cc_id),
    hash_()
  {
    calc_hash_();
  }

  ActionStatInnerKey(const v3_2_0_28::ActionStatInnerKey& key)
  :
    action_request_id_(key.action_request_id()),
    request_id_(key.request_id()),
    cc_id_(key.cc_id()),
    hash_()
  {
    calc_hash_();
  }

  bool operator==(const ActionStatInnerKey& rhs) const
  {
    return this == &rhs ||
      (action_request_id_ == rhs.action_request_id_ &&
      request_id_ == rhs.request_id_ &&
      cc_id_ == rhs.cc_id_);
  }

  const RequestId& action_request_id() const
  {
    return action_request_id_;
  }

  const RequestId& request_id() const
  {
    return request_id_;
  }

  unsigned long cc_id() const
  {
    return cc_id_;
  }

  size_t hash() const
  {
    return hash_;
  }

  friend FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, ActionStatInnerKey& key);

  friend std::ostream&
  operator<<(std::ostream& os, const ActionStatInnerKey& key);

private:
  void calc_hash_()
  {
    Generics::Murmur64Hash hasher(hash_);
    hash_add(hasher, cc_id_);
    hash_add(hasher, static_cast<const RequestId&>(action_request_id_));
    hash_add(hasher, static_cast<const RequestId&>(request_id_));
  }

  RequestIdIoWrapper action_request_id_;
  RequestIdIoWrapper request_id_;
  unsigned long cc_id_;
  size_t hash_;
};

class ActionStatInnerData
{
  typedef StringIoWrapperOptional OptionalStringT;

public:
  typedef AdServer::LogProcessing::FixedNumber FixedNum;

  ActionStatInnerData()
    : holder_(new DataHolder())
  {}

  ActionStatInnerData(
    unsigned long action_id,
    unsigned long tag_id,
    const std::string& order_id,
    const std::string& country_code,
    const std::string& referrer,
    const DayHourTimestamp& imp_date,
    const FixedNum& cur_value,
    unsigned long device_channel_id
  )
  :
    holder_(
      new DataHolder(
        action_id,
        tag_id,
        order_id,
        country_code,
        referrer,
        imp_date,
        OptionalDayHourTimestamp(),
        cur_value,
        device_channel_id
      )
    )
  {}

  ActionStatInnerData(
    unsigned long action_id,
    unsigned long tag_id,
    const std::string& order_id,
    const std::string& country_code,
    const std::string& referrer,
    const DayHourTimestamp& imp_date,
    const DayHourTimestamp& click_date,
    const FixedNum& cur_value,
    unsigned long device_channel_id
  )
  :
    holder_(
      new DataHolder(
        action_id,
        tag_id,
        order_id,
        country_code,
        referrer,
        imp_date,
        click_date,
        cur_value,
        device_channel_id
      )
    )
  {}

  ActionStatInnerData(const v3_2_0_28::ActionStatInnerData& arg)
  :
    holder_(
      new DataHolder(
        arg.action_id(),
        arg.tag_id(),
        arg.order_id(),
        arg.country_code().get(),
        arg.referrer(),
        arg.imp_date(),
        arg.click_date(),
        arg.cur_value(),
        0UL
      )
    )
  {}

  bool operator==(const ActionStatInnerData& data) const
  {
    return holder_.in() == data.holder_.in() ||
      (holder_->action_id == data.holder_->action_id &&
      holder_->tag_id == data.holder_->tag_id &&
      holder_->order_id == data.holder_->order_id &&
      holder_->country_code == data.holder_->country_code &&
      holder_->referrer == data.holder_->referrer &&
      holder_->imp_date == data.holder_->imp_date &&
      holder_->click_date == data.holder_->click_date &&
      holder_->cur_value == data.holder_->cur_value &&
      holder_->device_channel_id == data.holder_->device_channel_id);
  }

  ActionStatInnerData& operator+=(const ActionStatInnerData& rhs)
  {
    if (!holder_->click_date.present() && rhs.holder_->click_date.present())
    {
      holder_ = rhs.holder_;
    }
    return *this;
  }

  unsigned long action_id() const
  {
    return holder_->action_id;
  }

  unsigned long tag_id() const
  {
    return holder_->tag_id;
  }

  const std::string& order_id() const
  {
    return holder_->order_id.get();
  }

  const OptionalStringT& country_code() const
  {
    return holder_->country_code;
  }

  const std::string& referrer() const
  {
    return holder_->referrer.get();
  }

  const DayHourTimestamp& imp_date() const
  {
    return holder_->imp_date;
  }

  const OptionalDayHourTimestamp& click_date() const
  {
    return holder_->click_date;
  }

  const FixedNum& cur_value() const
  {
    return holder_->cur_value;
  }

  unsigned long device_channel_id() const
  {
    return holder_->device_channel_id;
  }

  friend FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, ActionStatInnerData& data)
    /*throw(eh::Exception)*/;

  friend std::ostream&
  operator<<(std::ostream& os, const ActionStatInnerData& data)
    /*throw(eh::Exception)*/;

private:
  struct DataHolder: public ReferenceCounting::AtomicImpl
  {
    DataHolder()
      : action_id(0), tag_id(0), order_id(), country_code(),
        imp_date(), click_date(), cur_value(FixedNum::ZERO),
        device_channel_id(0)
    {}

    DataHolder(
      unsigned long action_id_val,
      unsigned long tag_id_val,
      const std::string& order_id_val,
      const std::string& country_code_val,
      const std::string& referrer_val,
      const DayHourTimestamp& imp_date_val,
      const OptionalDayHourTimestamp& click_date_val,
      const FixedNum& cur_value_val,
      unsigned long device_channel_id_val
    )
    :
      action_id(action_id_val),
      tag_id(tag_id_val),
      order_id(order_id_val),
      country_code(country_code_val),
      imp_date(imp_date_val),
      click_date(click_date_val),
      cur_value(cur_value_val),
      device_channel_id(device_channel_id_val)
    {
      referrer.set("");
      trim(referrer.get(), referrer_val, 2048);
    }

    unsigned long action_id;
    unsigned long tag_id;
    OptionalStringT order_id;
    OptionalStringT country_code;
    OptionalStringT referrer;
    DayHourTimestamp imp_date;
    OptionalDayHourTimestamp click_date;
    FixedNum cur_value;
    unsigned long device_channel_id;

  private:
    virtual ~DataHolder() noexcept {}
  };

  typedef ReferenceCounting::SmartPtr<DataHolder> DataHolder_var;

private:
  DataHolder_var holder_;
};

struct ActionStatKey
{
  ActionStatKey(): sdate_(), colo_id_(), hash_() {}

  ActionStatKey(
    const DayHourTimestamp& sdate,
    unsigned long colo_id
  )
  :
    sdate_(sdate),
    colo_id_(colo_id),
    hash_()
  {
    calc_hash_();
  }

  ActionStatKey(const v3_2_0_28::ActionStatKey& key)
  :
    sdate_(key.sdate()),
    colo_id_(key.colo_id()),
    hash_()
  {
    calc_hash_();
  }

  bool operator==(const ActionStatKey& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return sdate_ == rhs.sdate_ && colo_id_ == rhs.colo_id_;
  }

  const DayHourTimestamp& sdate() const
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
  operator>>(std::istream& is, ActionStatKey& key);

  friend std::ostream&
  operator<<(std::ostream& os, const ActionStatKey& key) /*throw(eh::Exception)*/;

private:
  void calc_hash_()
  {
    Generics::Murmur64Hash hasher(hash_);
    hash_add(hasher, colo_id_);
    sdate_.hash_add(hasher);
  }

  DayHourTimestamp sdate_;
  unsigned long colo_id_;
  size_t hash_;
};

typedef StatCollector<ActionStatInnerKey, ActionStatInnerData, false, true>
  ActionStatInnerCollector;

typedef ActionStatInnerCollector ActionStatData;

typedef StatCollector<ActionStatKey, ActionStatData> ActionStatCollector;

struct ActionStatTraits: LogDefaultTraits<ActionStatCollector>
{
  template <class FUNCTOR_>
  static
  void
  for_each_old(FUNCTOR_& f) /*throw(eh::Exception)*/
  {
    f.template operator()<v3_2_0_28::ActionStatCollector, true>("3.2.0.28");
  }
};

} // namespace LogProcessing
} // namespace AdServer

#endif /* AD_SERVER_LOG_PROCESSING_ACTION_STAT_HPP */

