#ifndef AD_SERVER_LOG_PROCESSING_ACTION_REQUEST_HPP
#define AD_SERVER_LOG_PROCESSING_ACTION_REQUEST_HPP


#include <LogCommons/LogCommons.hpp>
#include <LogCommons/StatCollector.hpp>

namespace AdServer {
namespace LogProcessing {

class ActionRequestInnerKey
{
  typedef StringIoWrapperOptional OptionalStringT;

public:
  ActionRequestInnerKey() {}

  ActionRequestInnerKey(
    unsigned long action_id,
    const std::string& country_code,
    const std::string& action_referrer_url,
    const char user_status
  )
  :
    holder_(
      new DataHolder(
        action_id,
        country_code,
        action_referrer_url,
        user_status
      )
    )
  {
  }

  bool operator==(const ActionRequestInnerKey& key) const
  {
    if (this == &key || holder_.in() == key.holder_.in())
    {
      return true;
    }
    return *holder_ == *key.holder_;
  }

  unsigned long action_id() const
  {
    return holder_->action_id;
  }

  const std::string& country_code() const
  {
    return holder_->country_code.get();
  }

  const std::string& action_referrer_url() const
  {
    return holder_->action_referrer_url.get();
  }

  char user_status() const
  {
    return holder_->user_status;
  }

  size_t hash() const
  {
    return holder_->hash;
  }

private:
  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, ActionRequestInnerKey& key)
    /*throw(eh::Exception)*/;

  friend
  std::ostream&
  operator<<(std::ostream& os, const ActionRequestInnerKey& key)
    /*throw(eh::Exception)*/;

  class DataHolder: public ReferenceCounting::AtomicImpl
  {
  public:
    DataHolder() noexcept
    :
      action_id(),
      country_code(),
      user_status(),
      hash()
    {}

    DataHolder(
      unsigned long action_id_val,
      const std::string& country_code_val,
      const std::string& action_referrer_url_val,
      const char user_status_val
    )
    :
      action_id(action_id_val),
      country_code(country_code_val),
      user_status(user_status_val)
    {
      action_referrer_url.set("");
      trim(action_referrer_url.get(), action_referrer_url_val, 2048);
      convert_opt_in_status_to_user_status();
      calc_hash_();
    }

    bool operator==(const DataHolder& key) const
    {
      return action_id == key.action_id &&
        country_code == key.country_code &&
        action_referrer_url == key.action_referrer_url &&
        user_status == key.user_status;
    }

    void calc_hash_()
    {
      Generics::Murmur64Hash hasher(hash);
      hash_add(hasher, action_id);
      country_code.hash_add(hasher);
      action_referrer_url.hash_add(hasher);
      hash_add(hasher, user_status);
    }

    void convert_opt_in_status_to_user_status()
    {
      if (user_status == 'N')
      {
        user_status = 'U';
      }
    }

    void invariant() const /*throw(eh::Exception)*/
    {
      if (!is_valid_user_status(user_status))
      {
        Stream::Error es;
        es << "ActionRequestInnerKey::invariant(): user_status "
          "has invalid value '" << user_status << '\'';
        throw ConstraintViolation(es);
      }
    }

    template <typename ARCHIVE_>
    void serialize(ARCHIVE_& ar)
    {
      ar & action_id;
      ar & country_code;
      ar & action_referrer_url;
      ar ^ user_status;
      convert_opt_in_status_to_user_status();
    }

    unsigned long action_id;
    OptionalStringT country_code;
    OptionalStringT action_referrer_url;
    char user_status;
    size_t hash;

  protected:
    virtual
    ~DataHolder() noexcept = default;
  };

  typedef ReferenceCounting::AssertPtr<DataHolder>::Ptr DataHolder_var;

  DataHolder_var holder_;
};

class ActionRequestInnerData
{
public:
  typedef AdServer::LogProcessing::FixedNumber FixedNum;

  ActionRequestInnerData()
  :
    action_request_count_(),
    cur_value_(FixedNum::ZERO)
  {
  }

  ActionRequestInnerData(
    unsigned long action_request_count,
    const FixedNum& cur_value
  )
  :
    action_request_count_(action_request_count),
    cur_value_(cur_value)
  {
  }

  bool operator==(const ActionRequestInnerData& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return action_request_count_ == rhs.action_request_count_ &&
      cur_value_ == rhs.cur_value_;
  }

  ActionRequestInnerData&
  operator+=(const ActionRequestInnerData& rhs)
  {
    action_request_count_ += rhs.action_request_count_;
    cur_value_ += rhs.cur_value_;
    return *this;
  }

  unsigned long action_request_count() const
  {
    return action_request_count_;
  }

  const FixedNum& cur_value() const
  {
    return cur_value_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, ActionRequestInnerData& data);

  friend
  std::ostream&
  operator<<(std::ostream& os, const ActionRequestInnerData& data);

private:
  unsigned long action_request_count_;
  FixedNum cur_value_;
};

struct ActionRequestKey
{
  ActionRequestKey(): sdate_(), colo_id_(), hash_() {}

  ActionRequestKey(
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

  bool operator==(const ActionRequestKey& rhs) const
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

  friend
  std::istream&
  operator>>(std::istream& is, ActionRequestKey& key);

  friend
  std::ostream&
  operator<<(std::ostream& os, const ActionRequestKey& key)
    /*throw(eh::Exception)*/;

private:
  void calc_hash_()
  {
    Generics::Murmur64Hash hasher(hash_);
    sdate_.hash_add(hasher);
    hash_add(hasher, colo_id_);
  }

  DayHourTimestamp sdate_;
  unsigned long colo_id_;
  size_t hash_;
};

typedef StatCollector<
          ActionRequestInnerKey, ActionRequestInnerData, false, true
        > ActionRequestInnerCollector;

typedef ActionRequestInnerCollector ActionRequestData;

typedef StatCollector<ActionRequestKey, ActionRequestData>
  ActionRequestCollector;

typedef LogDefaultTraits<ActionRequestCollector> ActionRequestTraits;

} // namespace LogProcessing
} // namespace AdServer

#endif /* AD_SERVER_LOG_PROCESSING_ACTION_REQUEST_HPP */

