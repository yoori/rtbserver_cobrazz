#ifndef AD_SERVER_LOG_PROCESSING_TAG_REQUEST_HPP
#define AD_SERVER_LOG_PROCESSING_TAG_REQUEST_HPP


#include <iosfwd>
#include <Generics/Time.hpp>
#include <Generics/Rand.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <Commons/UserInfoManip.hpp>
#include <Commons/StringHolder.hpp>
#include "LogCommons.hpp"
#include "StatCollector.hpp"

#include "AdRequestLogger.hpp" // for user_id_distribution_hash()

namespace AdServer {
namespace LogProcessing {

class TagRequestData_V_3_3
{
protected:
  typedef Aux_::StringIoWrapper StringT;

public:
  class OptInSection
  {
    class Data: public ReferenceCounting::AtomicImpl
    {
    public:
      Data()
      :
        site_id(),
        isp_time(),
        user_id(),
        page_load_id(),
        ad_shown(),
        profile_referer()
      {
      }

      bool operator==(const Data& rhs) const
      {
        return &rhs == this ||
          (site_id == rhs.site_id &&
           isp_time == rhs.isp_time &&
           user_id == rhs.user_id &&
           page_load_id == rhs.page_load_id &&
           ad_shown == rhs.ad_shown &&
           profile_referer == rhs.profile_referer);
      }

      unsigned long site_id;
      SecondsTimestamp isp_time;
      UserId user_id;
      OptionalUlong page_load_id;
      bool ad_shown;
      bool profile_referer;

    protected:
      virtual
      ~Data() noexcept = default;
    };

    typedef ReferenceCounting::SmartPtr<Data> DataPtr;

  public:
    OptInSection(): data_(new Data) {}

    OptInSection(
      unsigned long site_id,
      const SecondsTimestamp& isp_time,
      const UserId& user_id,
      const OptionalUlong& page_load_id,
      bool ad_shown,
      bool profile_referer
    )
    :
      data_(new Data)
    {
      data_->site_id = site_id;
      data_->isp_time = isp_time;
      data_->user_id = user_id;
      data_->page_load_id = page_load_id;
      data_->ad_shown = ad_shown;
      data_->profile_referer = profile_referer;
    }

    bool operator==(const OptInSection& rhs) const
    {
      return &rhs == this || data_ == rhs.data_ || *data_ == *rhs.data_;
    }

    unsigned long site_id() const
    {
      return data_->site_id;
    }

    const SecondsTimestamp& isp_time() const
    {
      return data_->isp_time;
    }

    const UserId& user_id() const
    {
      return data_->user_id;
    }

    const OptionalUlong& page_load_id() const
    {
      return data_->page_load_id;
    }

    bool ad_shown() const
    {
      return data_->ad_shown;
    }

    bool profile_referer() const
    {
      return data_->profile_referer;
    }

    friend FixedBufStream<TabCategory>&
    operator>>(FixedBufStream<TabCategory>& is, OptInSection& opt_in_sect);

    // FXIME: remove
    /*
    friend std::istream&
    operator>>(std::istream& is, OptInSection& opt_in_sect);
    */

    friend std::ostream&
    operator<<(std::ostream& os, const OptInSection& opt_in_sect);

    void invariant() const /*throw(eh::Exception)*/
    {
      /*
      if (data_->user_id.is_null())
      {
        Stream::Error es;
        es << "PassbackSection::invariant(): user_id must not be NULL";
        throw ConstraintViolation(es);
      }
      */
    }

  private:
    DataPtr data_;
  };

  typedef OptionalValue<OptInSection> OptInSectionOptional;

  TagRequestData_V_3_3() noexcept
  :
    time_(),
    colo_id_(),
    tag_id_(),
    size_id_(),
    ext_tag_id_(),
    referer_(),
    full_referer_hash_(),
    user_status_(),
    country_(),
    passback_request_id_(),
    floor_cost_(FixedNumber::ZERO),
    opt_in_section_()
  {
  }

  TagRequestData_V_3_3(
    const SecondsTimestamp& time,
    unsigned long colo_id,
    unsigned long tag_id,
    const OptionalUlong& size_id,
    const StringT& ext_tag_id,
    const StringT& referer,
    const OptionalUlong& full_referer_hash,
    char user_status,
    const std::string& country,
    const RequestId& passback_request_id,
    const FixedNumber& floor_cost,
    const OptInSectionOptional& opt_in_section
  )
  :
    time_(time),
    colo_id_(colo_id),
    tag_id_(tag_id),
    size_id_(size_id),
    ext_tag_id_(ext_tag_id),
    referer_(referer),
    full_referer_hash_(full_referer_hash),
    user_status_(user_status),
    country_(country),
    passback_request_id_(passback_request_id),
    floor_cost_(floor_cost),
    opt_in_section_(opt_in_section),
    random_(Generics::safe_rand())
  {
    invariant();
    country_.get().resize(2);
  }

  bool operator==(const TagRequestData_V_3_3& data) const
  {
    if (this == &data)
    {
      return true;
    }
    return time_ == data.time_ &&
      colo_id_ == data.colo_id_ &&
      tag_id_ == data.tag_id_ &&
      size_id_ == data.size_id_ &&
      referer_ == data.referer_ &&
      ext_tag_id_ == data.ext_tag_id_ &&
      full_referer_hash_ == data.full_referer_hash_ &&
      user_status_ == data.user_status_ &&
      country_ == data.country_ &&
      passback_request_id_ == data.passback_request_id_ &&
      floor_cost_ == data.floor_cost_ &&
      opt_in_section_ == data.opt_in_section_;
  }

  const SecondsTimestamp& time() const
  {
    return time_;
  }

  unsigned long colo_id() const
  {
    return colo_id_;
  }

  unsigned long tag_id() const
  {
    return tag_id_;
  }

  const OptionalUlong& size_id() const
  {
    return size_id_;
  }

  const std::string& ext_tag_id() const
  {
    return ext_tag_id_.get();
  }

  const StringT& referer() const
  {
    return referer_;
  }

  const OptionalUlong& full_referer_hash() const noexcept
  {
    return full_referer_hash_;
  }

  char user_status() const
  {
    return user_status_;
  }

  const std::string& country() const
  {
    return country_.get();
  }

  const RequestId& passback_request_id() const
  {
    return passback_request_id_;
  }

  const FixedNumber& floor_cost() const
  {
    return floor_cost_;
  }

  const OptInSectionOptional& opt_in_section() const
  {
    return opt_in_section_;
  }

  unsigned long distrib_hash() const
  {
    if (opt_in_section_.present())
    {
      return user_id_distribution_hash(opt_in_section_.get().user_id());
    }
    if (!passback_request_id_.is_null())
    {
      return AdServer::Commons::uuid_distribution_hash(passback_request_id_);
    }
    return random_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, TagRequestData_V_3_3& data)
    /*throw(eh::Exception)*/;

private:
  void invariant() const /*throw(eh::Exception)*/
  {
    if (!colo_id_)
    {
      Stream::Error es;
      es << "TagRequestData_V_3_3::invariant(): colo_id_ must be > 0";
      throw ConstraintViolation(es);
    }
    if (referer_.empty())
    {
      Stream::Error es;
      es << "TagRequestData_V_3_3::invariant(): referer_ must be non-empty";
      throw ConstraintViolation(es);
    }
    if (!is_valid_user_status(user_status_))
    {
      Stream::Error es;
      es << "TagRequestData_V_3_3::invariant(): user_status_ "
        "has invalid value '" << user_status_ << '\'';
      throw ConstraintViolation(es);
    }
    if (opt_in_section_.present())
    {
      opt_in_section_.get().invariant();
    }
  }

  SecondsTimestamp time_;
  unsigned long colo_id_;
  unsigned long tag_id_;
  OptionalUlong size_id_;
  StringIoWrapperOptional ext_tag_id_;
  StringT referer_;
  OptionalUlong full_referer_hash_;
  char user_status_;
  StringIoWrapperOptional country_;
  RequestId passback_request_id_;
  FixedNumber floor_cost_;
  OptInSectionOptional opt_in_section_;
  unsigned long random_;
};

typedef SeqCollector<TagRequestData_V_3_3, true> TagRequestCollector_V_3_3;

class TagRequestData_V_3_5
{
protected:
  typedef Aux_::StringIoWrapper StringT;

public:
  class OptInSection
  {
    class Data: public ReferenceCounting::AtomicImpl
    {
    public:
      Data()
      :
        site_id(),
        isp_time(),
        user_id(),
        page_load_id(),
        ad_shown(),
        profile_referer(),
        user_agent(new AdServer::Commons::StringHolder(""))
      {
      }

      bool operator==(const Data& rhs) const
      {
        return &rhs == this ||
          (site_id == rhs.site_id &&
           isp_time == rhs.isp_time &&
           user_id == rhs.user_id &&
           page_load_id == rhs.page_load_id &&
           ad_shown == rhs.ad_shown &&
           profile_referer == rhs.profile_referer &&
           user_agent->str() == rhs.user_agent->str());
      }

      unsigned long site_id;
      SecondsTimestamp isp_time;
      UserId user_id;
      OptionalUlong page_load_id;
      bool ad_shown;
      bool profile_referer;
      Commons::StringHolder_var user_agent;

    protected:
      virtual
      ~Data() noexcept = default;
    };

    typedef ReferenceCounting::SmartPtr<Data> DataPtr;

    static const std::size_t MAX_USER_AGENT_LEN_ = 4000;

  public:
    OptInSection(): data_(new Data) {}

    OptInSection(
      unsigned long site_id,
      const SecondsTimestamp& isp_time,
      const UserId& user_id,
      const OptionalUlong& page_load_id,
      bool ad_shown,
      bool profile_referer,
      const Commons::StringHolder_var& user_agent
    )
    :
      data_(new Data)
    {
      data_->site_id = site_id;
      data_->isp_time = isp_time;
      data_->user_id = user_id;
      data_->page_load_id = page_load_id;
      data_->ad_shown = ad_shown;
      data_->profile_referer = profile_referer;
      if (user_agent.in())
      {
        data_->user_agent = user_agent;
        if (data_->user_agent->str().length() > MAX_USER_AGENT_LEN_)
        {
          std::string tmp;
          trim(tmp, data_->user_agent->str(), MAX_USER_AGENT_LEN_);
          data_->user_agent = new Commons::StringHolder(std::move(tmp));
        }
      }
      else
      {
        data_->user_agent = new Commons::StringHolder(std::string());
      }
    }

    bool operator==(const OptInSection& rhs) const
    {
      return &rhs == this || data_ == rhs.data_ || *data_ == *rhs.data_;
    }

    unsigned long site_id() const
    {
      return data_->site_id;
    }

    const SecondsTimestamp& isp_time() const
    {
      return data_->isp_time;
    }

    const UserId& user_id() const
    {
      return data_->user_id;
    }

    const OptionalUlong& page_load_id() const
    {
      return data_->page_load_id;
    }

    bool ad_shown() const
    {
      return data_->ad_shown;
    }

    bool profile_referer() const
    {
      return data_->profile_referer;
    }

    const std::string& user_agent() const
    {
      return data_->user_agent->str();
    }

    friend FixedBufStream<TabCategory>&
    operator>>(FixedBufStream<TabCategory>& is, OptInSection& opt_in_sect);

    friend std::ostream&
    operator<<(std::ostream& os, const OptInSection& opt_in_sect);

    void invariant() const /*throw(eh::Exception)*/
    {
      /*
      if (data_->user_id.is_null())
      {
        Stream::Error es;
        es << "PassbackSection::invariant(): user_id must not be NULL";
        throw ConstraintViolation(es);
      }
      */
    }

  private:
    DataPtr data_;
  };

  typedef OptionalValue<OptInSection> OptInSectionOptional;

  TagRequestData_V_3_5() noexcept
  :
    time_(),
    colo_id_(),
    tag_id_(),
    size_id_(),
    ext_tag_id_(),
    referer_(),
    full_referer_hash_(),
    user_status_(),
    country_(),
    passback_request_id_(),
    floor_cost_(FixedNumber::ZERO),
    urls_(),
    opt_in_section_()
  {
  }

  TagRequestData_V_3_5(
    const SecondsTimestamp& time,
    unsigned long colo_id,
    unsigned long tag_id,
    const OptionalUlong& size_id,
    const StringT& ext_tag_id,
    const StringT& referer,
    const OptionalUlong& full_referer_hash,
    char user_status,
    const std::string& country,
    const RequestId& passback_request_id,
    const FixedNumber& floor_cost,
    const StringList& urls,
    const OptInSectionOptional& opt_in_section
  )
  :
    time_(time),
    colo_id_(colo_id),
    tag_id_(tag_id),
    size_id_(size_id),
    ext_tag_id_(ext_tag_id),
    referer_(referer),
    full_referer_hash_(full_referer_hash),
    user_status_(user_status),
    country_(country),
    passback_request_id_(passback_request_id),
    floor_cost_(floor_cost),
    urls_(urls),
    opt_in_section_(opt_in_section),
    random_(Generics::safe_rand())
  {
    invariant();
    country_.get().resize(2);
  }

  bool operator==(const TagRequestData_V_3_5& data) const
  {
    if (this == &data)
    {
      return true;
    }
    return time_ == data.time_ &&
      colo_id_ == data.colo_id_ &&
      tag_id_ == data.tag_id_ &&
      size_id_ == data.size_id_ &&
      referer_ == data.referer_ &&
      ext_tag_id_ == data.ext_tag_id_ &&
      full_referer_hash_ == data.full_referer_hash_ &&
      user_status_ == data.user_status_ &&
      country_ == data.country_ &&
      passback_request_id_ == data.passback_request_id_ &&
      floor_cost_ == data.floor_cost_ &&
      urls_ == data.urls_ &&
      opt_in_section_ == data.opt_in_section_;
  }

  const SecondsTimestamp& time() const
  {
    return time_;
  }

  unsigned long colo_id() const
  {
    return colo_id_;
  }

  unsigned long tag_id() const
  {
    return tag_id_;
  }

  const OptionalUlong& size_id() const
  {
    return size_id_;
  }

  const std::string& ext_tag_id() const
  {
    return ext_tag_id_.get();
  }

  const StringT& referer() const
  {
    return referer_;
  }

  const OptionalUlong& full_referer_hash() const noexcept
  {
    return full_referer_hash_;
  }

  char user_status() const
  {
    return user_status_;
  }

  const std::string& country() const
  {
    return country_.get();
  }

  const RequestId& passback_request_id() const
  {
    return passback_request_id_;
  }

  const FixedNumber& floor_cost() const
  {
    return floor_cost_;
  }

  const StringList& urls() const
  {
    return urls_;
  }

  const OptInSectionOptional& opt_in_section() const
  {
    return opt_in_section_;
  }

  unsigned long distrib_hash() const
  {
    if (opt_in_section_.present())
    {
      return user_id_distribution_hash(opt_in_section_.get().user_id());
    }
    if (!passback_request_id_.is_null())
    {
      return AdServer::Commons::uuid_distribution_hash(passback_request_id_);
    }
    return random_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, TagRequestData_V_3_5& data)
    /*throw(eh::Exception)*/;

  friend
  std::ostream&
  operator<<(std::ostream& os, const TagRequestData_V_3_5& data)
    /*throw(eh::Exception)*/;

private:
  void invariant() const /*throw(eh::Exception)*/
  {
    if (!colo_id_)
    {
      Stream::Error es;
      es << "TagRequestData_V_3_5::invariant(): colo_id_ must be > 0";
      throw ConstraintViolation(es);
    }
    if (referer_.empty())
    {
      Stream::Error es;
      es << "TagRequestData_V_3_5::invariant(): referer_ must be non-empty";
      throw ConstraintViolation(es);
    }
    if (!is_valid_user_status(user_status_))
    {
      Stream::Error es;
      es << "TagRequestData_V_3_5::invariant(): user_status_ "
        "has invalid value '" << user_status_ << '\'';
      throw ConstraintViolation(es);
    }
    if (opt_in_section_.present())
    {
      opt_in_section_.get().invariant();
    }
  }

  SecondsTimestamp time_;
  unsigned long colo_id_;
  unsigned long tag_id_;
  OptionalUlong size_id_;
  StringIoWrapperOptional ext_tag_id_;
  StringT referer_;
  OptionalUlong full_referer_hash_;
  char user_status_;
  StringIoWrapperOptional country_;
  RequestId passback_request_id_;
  FixedNumber floor_cost_;
  StringList urls_;
  OptInSectionOptional opt_in_section_;
  unsigned long random_;
};

typedef SeqCollector<TagRequestData_V_3_5, true> TagRequestCollector_V_3_5;

class TagRequestData
{
protected:
  typedef Aux_::StringIoWrapper StringT;

public:
  class OptInSection
  {
    class Data: public ReferenceCounting::AtomicImpl
    {
    public:
      Data()
      :
        site_id(),
        user_id(),
        page_load_id(),
        ad_shown(),
        profile_referer(),
        user_agent()
      {
      }

      bool operator==(const Data& rhs) const
      {
        return &rhs == this ||
          (site_id == rhs.site_id &&
           user_id == rhs.user_id &&
           page_load_id == rhs.page_load_id &&
           ad_shown == rhs.ad_shown &&
           profile_referer == rhs.profile_referer &&
           ((!user_agent.in() && !rhs.user_agent.in()) ||
             (user_agent.in() && rhs.user_agent.in() &&
               user_agent->str() == rhs.user_agent->str()))
           );
      }

      unsigned long site_id;
      UserId user_id;
      OptionalUlong page_load_id;
      bool ad_shown;
      bool profile_referer;
      Commons::StringHolder_var user_agent;

    protected:
      virtual
      ~Data() noexcept = default;
    };

    typedef ReferenceCounting::SmartPtr<Data> DataPtr;

    static const std::size_t MAX_USER_AGENT_LEN_ = 4000;
    static const std::string EMPTY_STRING_;

  public:
    OptInSection(): data_(new Data) {}

    OptInSection(
      unsigned long site_id,
      const UserId& user_id,
      const OptionalUlong& page_load_id,
      bool ad_shown,
      bool profile_referer,
      const Commons::StringHolder_var& user_agent
    )
    :
      data_(new Data)
    {
      data_->site_id = site_id;
      data_->user_id = user_id;
      data_->page_load_id = page_load_id;
      data_->ad_shown = ad_shown;
      data_->profile_referer = profile_referer;
      if (user_agent.in())
      {
        data_->user_agent = user_agent;
        if (data_->user_agent->str().length() > MAX_USER_AGENT_LEN_)
        {
          std::string tmp;
          trim(tmp, data_->user_agent->str(), MAX_USER_AGENT_LEN_);
          data_->user_agent = new Commons::StringHolder(std::move(tmp));
        }
      }
      else
      {
        data_->user_agent = Commons::StringHolder_var();
      }
    }

    OptInSection(
      const TagRequestData_V_3_3::OptInSection& opt_in_section
    )
    :
      data_(new Data)
    {
      data_->site_id = opt_in_section.site_id();
      data_->user_id = opt_in_section.user_id();
      data_->page_load_id = opt_in_section.page_load_id();
      data_->ad_shown = opt_in_section.ad_shown();
      data_->profile_referer = opt_in_section.profile_referer();
    }

    OptInSection(
      const TagRequestData_V_3_5::OptInSection& opt_in_section
    )
    :
      data_(new Data)
    {
      data_->site_id = opt_in_section.site_id();
      data_->user_id = opt_in_section.user_id();
      data_->page_load_id = opt_in_section.page_load_id();
      data_->ad_shown = opt_in_section.ad_shown();
      data_->profile_referer = opt_in_section.profile_referer();
    }

    bool operator==(const OptInSection& rhs) const
    {
      return &rhs == this || data_ == rhs.data_ || *data_ == *rhs.data_;
    }

    unsigned long site_id() const
    {
      return data_->site_id;
    }

    const UserId& user_id() const
    {
      return data_->user_id;
    }

    const OptionalUlong& page_load_id() const
    {
      return data_->page_load_id;
    }

    bool ad_shown() const
    {
      return data_->ad_shown;
    }

    bool profile_referer() const
    {
      return data_->profile_referer;
    }

    const std::string&
    user_agent() const
    {
      return data_->user_agent.in() ?
        data_->user_agent->str() : EMPTY_STRING_;
    }

    friend FixedBufStream<TabCategory>&
    operator>>(FixedBufStream<TabCategory>& is, OptInSection& opt_in_sect);

    friend std::ostream&
    operator<<(std::ostream& os, const OptInSection& opt_in_sect);

    void invariant() const /*throw(eh::Exception)*/
    {
      /*
      if (data_->user_id.is_null())
      {
        Stream::Error es;
        es << "PassbackSection::invariant(): user_id must not be NULL";
        throw ConstraintViolation(es);
      }
      */
    }

  private:
    DataPtr data_;
  };

  typedef OptionalValue<OptInSection> OptInSectionOptional;

  TagRequestData() noexcept
  :
    time_(),
    isp_time_(),
    test_request_(),
    colo_id_(),
    tag_id_(),
    size_id_(),
    ext_tag_id_(),
    referer_(),
    full_referer_hash_(),
    user_status_(),
    country_(),
    passback_request_id_(),
    floor_cost_(FixedNumber::ZERO),
    urls_(),
    opt_in_section_()
  {
  }

  TagRequestData(const TagRequestData& init) noexcept
    : time_(init.time_),
      isp_time_(init.isp_time_),
      test_request_(init.test_request_),
      colo_id_(init.colo_id_),
      tag_id_(init.tag_id_),
      size_id_(init.size_id_),
      ext_tag_id_(init.ext_tag_id_),
      referer_(init.referer_),
      full_referer_hash_(init.full_referer_hash_),
      user_status_(init.user_status_),
      country_(init.country_),
      passback_request_id_(init.passback_request_id_),
      floor_cost_(init.floor_cost_),
      urls_(init.urls_),
      opt_in_section_(init.opt_in_section_),
      random_(init.random_)
  {}

  TagRequestData(TagRequestData&& init) noexcept
    : time_(init.time_),
      isp_time_(init.isp_time_),
      test_request_(init.test_request_),
      colo_id_(init.colo_id_),
      tag_id_(init.tag_id_),
      size_id_(init.size_id_),
      ext_tag_id_(std::move(init.ext_tag_id_)),
      referer_(std::move(init.referer_)),
      full_referer_hash_(init.full_referer_hash_),
      user_status_(init.user_status_),
      country_(std::move(init.country_)),
      passback_request_id_(init.passback_request_id_),
      floor_cost_(init.floor_cost_),
      urls_(std::move(init.urls_)),
      opt_in_section_(std::move(init.opt_in_section_)),
      random_(init.random_)
  {}

  TagRequestData(
    const SecondsTimestamp& time,
    const SecondsTimestamp& isp_time,
    bool test_request,
    unsigned long colo_id,
    unsigned long tag_id,
    const OptionalUlong& size_id,
    const StringT& ext_tag_id,
    const StringT& referer,
    const OptionalUlong& full_referer_hash,
    char user_status,
    const std::string& country,
    const RequestId& passback_request_id,
    const FixedNumber& floor_cost,
    const StringList& urls,
    const OptInSectionOptional& opt_in_section
  )
  :
    time_(time),
    isp_time_(isp_time),
    test_request_(test_request),
    colo_id_(colo_id),
    tag_id_(tag_id),
    size_id_(size_id),
    ext_tag_id_(ext_tag_id),
    referer_(referer),
    full_referer_hash_(full_referer_hash),
    user_status_(user_status),
    country_(country),
    passback_request_id_(passback_request_id),
    floor_cost_(floor_cost),
    urls_(urls),
    opt_in_section_(opt_in_section),
    random_(Generics::safe_rand())
  {
    invariant();
    country_.get().resize(2);
  }

  TagRequestData(const TagRequestData_V_3_3& data)
  :
    time_(data.time()),
    isp_time_(data.opt_in_section().present() ?
      data.opt_in_section().get().isp_time() : data.time()),
    test_request_(),
    colo_id_(data.colo_id()),
    tag_id_(data.tag_id()),
    size_id_(data.size_id()),
    ext_tag_id_(data.ext_tag_id()),
    referer_(data.referer()),
    full_referer_hash_(data.full_referer_hash()),
    user_status_(data.user_status()),
    country_(data.country()),
    passback_request_id_(data.passback_request_id()),
    floor_cost_(FixedNumber::ZERO),
    urls_(),
    opt_in_section_(data.opt_in_section().present() ?
      data.opt_in_section().get() : OptInSectionOptional()),
    random_(Generics::safe_rand())
  {
    invariant();
    country_.get().resize(2);
  }

  TagRequestData(const TagRequestData_V_3_5& data)
  :
    time_(data.time()),
    isp_time_(data.opt_in_section().present() ?
      data.opt_in_section().get().isp_time() : data.time()),
    test_request_(),
    colo_id_(data.colo_id()),
    tag_id_(data.tag_id()),
    size_id_(data.size_id()),
    ext_tag_id_(data.ext_tag_id()),
    referer_(data.referer()),
    full_referer_hash_(data.full_referer_hash()),
    user_status_(data.user_status()),
    country_(data.country()),
    passback_request_id_(data.passback_request_id()),
    floor_cost_(data.floor_cost()),
    urls_(data.urls()),
    opt_in_section_(data.opt_in_section().present() ?
      data.opt_in_section().get() : OptInSectionOptional()),
    random_(Generics::safe_rand())
  {
    invariant();
    country_.get().resize(2);
  }

  bool operator==(const TagRequestData& data) const
  {
    if (this == &data)
    {
      return true;
    }
    return time_ == data.time_ &&
      isp_time_ == data.isp_time_ &&
      test_request_ == data.test_request_ &&
      colo_id_ == data.colo_id_ &&
      tag_id_ == data.tag_id_ &&
      size_id_ == data.size_id_ &&
      referer_ == data.referer_ &&
      ext_tag_id_ == data.ext_tag_id_ &&
      full_referer_hash_ == data.full_referer_hash_ &&
      user_status_ == data.user_status_ &&
      country_ == data.country_ &&
      passback_request_id_ == data.passback_request_id_ &&
      floor_cost_ == data.floor_cost_ &&
      urls_ == data.urls_ &&
      opt_in_section_ == data.opt_in_section_;
  }

  const SecondsTimestamp& time() const
  {
    return time_;
  }

  const SecondsTimestamp& isp_time() const
  {
    return isp_time_;
  }

  bool test_request() const
  {
    return test_request_;
  }

  unsigned long colo_id() const
  {
    return colo_id_;
  }

  unsigned long tag_id() const
  {
    return tag_id_;
  }

  const OptionalUlong& size_id() const
  {
    return size_id_;
  }

  const std::string& ext_tag_id() const
  {
    return ext_tag_id_.get();
  }

  const StringT& referer() const
  {
    return referer_;
  }

  const OptionalUlong& full_referer_hash() const noexcept
  {
    return full_referer_hash_;
  }

  char user_status() const
  {
    return user_status_;
  }

  const std::string& country() const
  {
    return country_.get();
  }

  const RequestId& passback_request_id() const
  {
    return passback_request_id_;
  }

  const FixedNumber& floor_cost() const
  {
    return floor_cost_;
  }

  const StringList& urls() const
  {
    return urls_;
  }

  const OptInSectionOptional& opt_in_section() const
  {
    return opt_in_section_;
  }

  unsigned long distrib_hash() const
  {
    if (opt_in_section_.present())
    {
      return user_id_distribution_hash(opt_in_section_.get().user_id());
    }
    if (!passback_request_id_.is_null())
    {
      return AdServer::Commons::uuid_distribution_hash(passback_request_id_);
    }
    return random_;
  }

  friend FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, TagRequestData& opt_in_sect)
    /*throw(eh::Exception)*/;

  friend
  std::ostream&
  operator<<(std::ostream& os, const TagRequestData& data)
    /*throw(eh::Exception)*/;

private:
  void invariant() const /*throw(eh::Exception)*/
  {
    if (!colo_id_)
    {
      Stream::Error es;
      es << "TagRequestData::invariant(): colo_id_ must be > 0";
      throw ConstraintViolation(es);
    }
    if (referer_.empty())
    {
      Stream::Error es;
      es << "TagRequestData::invariant(): referer_ must be non-empty";
      throw ConstraintViolation(es);
    }
    if (!is_valid_user_status(user_status_))
    {
      Stream::Error es;
      es << "TagRequestData::invariant(): user_status_ "
        "has invalid value '" << user_status_ << '\'';
      throw ConstraintViolation(es);
    }
    if (opt_in_section_.present())
    {
      opt_in_section_.get().invariant();
    }
  }

  SecondsTimestamp time_;
  SecondsTimestamp isp_time_;
  bool test_request_;
  unsigned long colo_id_;
  unsigned long tag_id_;
  OptionalUlong size_id_;
  StringIoWrapperOptional ext_tag_id_;
  StringT referer_;
  OptionalUlong full_referer_hash_;
  char user_status_;
  StringIoWrapperOptional country_;
  RequestId passback_request_id_;
  FixedNumber floor_cost_;
  StringList urls_;
  OptInSectionOptional opt_in_section_;
  unsigned long random_;
};

typedef SeqCollector<TagRequestData, true> TagRequestCollector;

struct TagRequestTraits: LogDefaultTraits<TagRequestCollector, false, false>
{
  template <typename Functor>
  static
  void
  for_each_old(Functor& f) /*throw(eh::Exception)*/
  {
    f.template operator()<TagRequestCollector_V_3_3>("3.3");
    f.template operator()<TagRequestCollector_V_3_5>("3.5");
  }

  typedef GenericLogIoHelperImpl<TagRequestTraits> IoHelperType;
};

} // namespace LogProcessing
} // namespace AdServer

#endif /* AD_SERVER_LOG_PROCESSING_TAG_REQUEST_HPP */

