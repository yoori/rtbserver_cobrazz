#ifndef AD_SERVER_LOG_PROCESSING_WEBWISE_DISCOVER_ITEMS_HPP
#define AD_SERVER_LOG_PROCESSING_WEBWISE_DISCOVER_ITEMS_HPP

#include <sstream>
#include <Generics/Time.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include "LogCommons.hpp"
#include "StatCollector.hpp"


namespace AdServer {
namespace LogProcessing {

class WebwiseDiscoverItemsKey
{
  typedef Aux_::StringIoWrapper StringT;

public:
  WebwiseDiscoverItemsKey()
  :
    item_id_(),
    hash_()
  {
  }

  WebwiseDiscoverItemsKey(
    const std::string &item_id
  )
  :
    item_id_(item_id),
    hash_()
  {
    calc_hash();
  }

  bool operator==(const WebwiseDiscoverItemsKey &rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return item_id_ == rhs.item_id_;
  }

  const StringT& item_id() const
  {
    return item_id_;
  }

  size_t hash() const
  {
    return hash_;
  }

  friend std::istream& operator>>(std::istream &is,
    WebwiseDiscoverItemsKey &key);
  friend std::ostream& operator<<(std::ostream &os,
    const WebwiseDiscoverItemsKey &key) /*throw(eh::Exception)*/;

private:
  void calc_hash()
  {
    Generics::Murmur64Hash hasher(hash_);
    hash_add(hasher, item_id_);
  }

  void invariant() const /*throw(eh::Exception)*/
  {
    if (item_id_.empty())
    {
      throw ConstraintViolation("WebwiseDiscoverItemsKey::invariant(): "
        "item_id_ must be non-empty");
    }
  }

  StringT item_id_;
  size_t hash_;
};

class WebwiseDiscoverItemsData
{
  typedef Aux_::StringIoWrapper StringT;

public:
  WebwiseDiscoverItemsData()
  {}

  WebwiseDiscoverItemsData(
    const std::string &title,
    const SecondsTimestamp &pub_date,
    const std::string &link,
    const std::string &language
    )
    : holder_(new DataHolder(
        title,
        pub_date,
        link,
        language))
  {
    if (holder_->language.length() > 3)
    {
      holder_->language.resize(3);
    }
  }

  bool operator==(const WebwiseDiscoverItemsData &rhs) const
  {
    if (holder_.in() == rhs.holder_.in())
    {
      return true;
    }
    return holder_->title == rhs.holder_->title &&
      holder_->pub_date == rhs.holder_->pub_date &&
      holder_->link == rhs.holder_->link &&
      holder_->language == rhs.holder_->language;
  }

  WebwiseDiscoverItemsData& operator+=(const WebwiseDiscoverItemsData &rhs)
  {
    holder_ = rhs.holder_;
    return *this;
  }

  const StringT& title() const
  {
    return holder_->title;
  }

  const SecondsTimestamp& pub_date() const
  {
    return holder_->pub_date;
  }

  const StringT& link() const
  {
    return holder_->link;
  }

  const StringT& language() const
  {
    return holder_->language;
  }

  friend std::istream& operator>>(std::istream &is,
    WebwiseDiscoverItemsData &data);
  friend std::ostream& operator<<(std::ostream &os,
    const WebwiseDiscoverItemsData &data);

private:
  struct DataHolder: public ReferenceCounting::AtomicImpl
  {
    DataHolder() {}

    DataHolder(
      const std::string& title_val,
      const SecondsTimestamp& pub_date_val,
      const std::string& link_val,
      const std::string& language_val)
      : title(title_val),
        pub_date(pub_date_val),
        link(link_val),
        language(language_val)
    {}

    StringT title;
    SecondsTimestamp pub_date;
    StringT link;
    StringT language;

  private:
    virtual ~DataHolder() noexcept {}
  };

  typedef ReferenceCounting::SmartPtr<DataHolder> DataHolder_var;

private:
  DataHolder_var holder_;
};


typedef StatCollector<WebwiseDiscoverItemsKey, WebwiseDiscoverItemsData>
  WebwiseDiscoverItemsCollector;

typedef LogDefaultTraits<WebwiseDiscoverItemsCollector, false>
  WebwiseDiscoverItemsTraits;



} // namespace LogProcessing
} // namespace AdServer

#endif /* AD_SERVER_LOG_PROCESSING_WEBWISE_DISCOVER_ITEMS_HPP */

