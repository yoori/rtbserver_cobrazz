#pragma once

#include <iosfwd>
#include <utility>
#include <Generics/Time.hpp>
#include <Generics/Rand.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <Commons/UserInfoManip.hpp>
#include <Commons/StringHolder.hpp>

#include "LogCommons.hpp"
#include "StatCollector.hpp"
#include "AdRequestLogger.hpp"

namespace AdServer::LogProcessing
{
  struct TagRequestData
  {
    using StringT = Aux_::StringIoWrapper;

    struct OptInSection
    {
      static const std::size_t MAX_USER_AGENT_LEN_ = 4000;
      static const std::string EMPTY_STRING_;

      OptInSection() = default;

      OptInSection(
        std::uint32_t site_id,
        const UserId& user_id,
        const OptionalUInt64& page_load_id,
        bool ad_shown,
        bool profile_referer,
        const Commons::StringHolder_var& user_agent)
        : site_id_(site_id),
          user_id_(user_id),
          page_load_id_(page_load_id),
          ad_shown_(ad_shown),
          profile_referer_(profile_referer),
          user_agent_(user_agent)
      {
        if (user_agent_.in() && user_agent_->str().length() > MAX_USER_AGENT_LEN_)
        {
          std::string tmp;
          trim(tmp, user_agent_->str(), MAX_USER_AGENT_LEN_);
          user_agent_ = new Commons::StringHolder(std::move(tmp));
        }
      }

      bool operator==(const OptInSection& rhs) const
      {
        return &rhs == this ||
          (site_id_ == rhs.site_id_ &&
           user_id_ == rhs.user_id_ &&
           page_load_id_ == rhs.page_load_id_ &&
           ad_shown_ == rhs.ad_shown_ &&
           profile_referer_ == rhs.profile_referer_ &&
           ((!user_agent_.in() && !rhs.user_agent_.in()) ||
             (user_agent_.in() && rhs.user_agent_.in() &&
               user_agent_->str() == rhs.user_agent_->str())));
      }

      std::uint32_t site_id() const
      {
        return site_id_;
      }

      const UserId& user_id() const
      {
        return user_id_;
      }

      const OptionalUInt64& page_load_id() const
      {
        return page_load_id_;
      }

      bool ad_shown() const
      {
        return ad_shown_;
      }

      bool profile_referer() const
      {
        return profile_referer_;
      }

      const std::string&
      user_agent() const &
      {
        return user_agent_.in() ? user_agent_->str() : EMPTY_STRING_;
      }

      friend FixedBufStream<TabCategory>&
      operator>>(FixedBufStream<TabCategory>& is, OptInSection& opt_in_sect);

      friend BufferWriter&
      operator<<(BufferWriter& out, const OptInSection& opt_in_sect);

      std::uint32_t site_id_{};
      UserId user_id_;
      OptionalUInt64 page_load_id_;
      bool ad_shown_{};
      bool profile_referer_{};
      Commons::StringHolder_var user_agent_;
    };

    using OptInSectionOptional = OptionalValue<OptInSection>;

    TagRequestData() noexcept
      : time_(),
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
    {}

    TagRequestData(const TagRequestData&) = delete;
    TagRequestData& operator=(const TagRequestData&) = delete;

    TagRequestData(TagRequestData&&) noexcept = default;
    TagRequestData& operator=(TagRequestData&&) noexcept = default;

    TagRequestData(
      const SecondsTimestamp& time,
      const SecondsTimestamp& isp_time,
      bool test_request,
      std::uint32_t colo_id,
      std::uint32_t tag_id,
      const OptionalUInt32& size_id,
      const StringT& ext_tag_id,
      const StringT& referer,
      const OptionalUInt64& full_referer_hash,
      char user_status,
      const std::string& country,
      const RequestId& passback_request_id,
      const FixedNumber& floor_cost,
      StringArray urls,
      OptInSectionOptional opt_in_section)
      : time_(time),
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
        urls_(std::move(urls)),
        opt_in_section_(std::move(opt_in_section)),
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
        urls_ == data.urls_ && opt_in_section_ == data.opt_in_section_;
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

    std::uint32_t colo_id() const
    {
      return colo_id_;
    }

    std::uint32_t tag_id() const
    {
      return tag_id_;
    }

    const OptionalUInt32& size_id() const
    {
      return size_id_;
    }

    const std::string& ext_tag_id() const &
    {
      return ext_tag_id_.get();
    }

    const StringT& referer() const &
    {
      return referer_;
    }

    const OptionalUInt64& full_referer_hash() const noexcept
    {
      return full_referer_hash_;
    }

    char user_status() const
    {
      return user_status_;
    }

    const std::string& country() const &
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

    const StringArray& urls() const &
    {
      return urls_;
    }

    const OptInSectionOptional& opt_in_section() const &
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
    BufferWriter&
    operator<<(BufferWriter& out, const TagRequestData& data)
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
    }

  public:
    SecondsTimestamp time_;
    SecondsTimestamp isp_time_;
    bool test_request_;
    std::uint32_t colo_id_;
    std::uint32_t tag_id_;
    OptionalUInt32 size_id_;
    StringIoWrapperOptional ext_tag_id_;
    StringT referer_;
    OptionalUInt64 full_referer_hash_;
    char user_status_;
    StringIoWrapperOptional country_;
    RequestId passback_request_id_;
    FixedNumber floor_cost_;
    StringArray urls_;
    OptInSectionOptional opt_in_section_;

  private:
    unsigned long random_{Generics::safe_rand()};
  };

  typedef SeqCollector<TagRequestData, true> TagRequestCollector;

  struct TagRequestTraits: LogDefaultTraits<TagRequestCollector, false, false>
  {
    typedef MoveSeqDistributeStrategy<TagRequestTraits> DistributeStrategyType;

    typedef GenericLogIoHelperImpl<TagRequestTraits> IoHelperType;
  };
} // namespace AdServer::LogProcessing
