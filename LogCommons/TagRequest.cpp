
#include "TagRequest.hpp"
#include "BufferWriter.hpp"

#include <LogCommons/LogCommons.ipp>

namespace AdServer::LogProcessing
{
  template <> const char* TagRequestTraits::B::base_name_ = "TagRequest";
  template <> const char* TagRequestTraits::B::signature_ = "TagRequest";
  template <> const char* TagRequestTraits::B::current_version_ = "3.5b";

  const std::string
  TagRequestData::OptInSection::EMPTY_STRING_ = std::string();

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, TagRequestData::OptInSection& opt_in_sect)
  {
    is >> opt_in_sect.site_id_;
    is >> opt_in_sect.user_id_;
    is >> opt_in_sect.page_load_id_;
    is >> opt_in_sect.ad_shown_;
    is >> opt_in_sect.profile_referer_;
    StringIoWrapperOptional ua_wrapper;
    is >> ua_wrapper;
    opt_in_sect.user_agent_ = new Commons::StringHolder(std::move(ua_wrapper.get()));
    return is;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const TagRequestData::OptInSection& opt_in_sect)
  {
    out << opt_in_sect.site_id() << '\t'
      << opt_in_sect.user_id() << '\t'
      << opt_in_sect.page_load_id() << '\t'
      << opt_in_sect.ad_shown() << '\t'
      << opt_in_sect.profile_referer() << '\t' << StringIoWrapperOptional(opt_in_sect.user_agent());
    return out;
  }

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, TagRequestData& data)
    /*throw(eh::Exception)*/
  {
    is >> data.time_;
    is >> data.isp_time_;
    is >> data.test_request_;
    is >> data.colo_id_;
    is >> data.tag_id_;
    is >> data.site_id_;
    is >> data.size_id_;
    is >> data.ext_tag_id_;
    is >> data.referer_;
    is >> data.full_referer_hash_;
    is >> data.user_status_;
    is >> data.country_;
    is >> data.passback_request_id_;
    is >> data.floor_cost_;

    String::SubString token = is.read_token();
    if (is.good())
    {
      parse_string_list(token, data.urls_, ' ');
    }

    data.opt_in_section_ = TagRequestData::OptInSectionOptional();
    is >> data.opt_in_section_;

    data.invariant();
    return is;
  }

  void
  TagRequestData::read_v_3_5a_(FixedBufStream<TabCategory>& is)
  {
    is >> time_;
    is >> isp_time_;
    is >> test_request_;
    is >> colo_id_;
    is >> tag_id_;
    is >> size_id_;
    is >> ext_tag_id_;
    is >> referer_;
    is >> full_referer_hash_;
    is >> user_status_;
    is >> country_;
    is >> passback_request_id_;
    is >> floor_cost_;

    String::SubString token = is.read_token();
    if (is.good())
    {
      parse_string_list(token, urls_, ' ');
    }

    opt_in_section_ = OptInSectionOptional();
    is >> opt_in_section_;
    site_id_ = opt_in_section_.present() ? opt_in_section_.get().site_id() : 0;
    invariant();
  }

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, TagRequestData_V_3_5a& data)
  {
    data.data_.read_v_3_5a_(is);
    return is;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const TagRequestData& data)
    /*throw(eh::Exception)*/
  {
    data.invariant();

    out << data.time_ << '\t'
      << data.isp_time_ << '\t'
      << data.test_request_ << '\t'
      << data.colo_id_ << '\t'
      << data.tag_id_ << '\t'
      << data.site_id_ << '\t'
      << data.size_id_ << '\t'
      << data.ext_tag_id_ << '\t'
      << data.referer_ << '\t'
      << data.full_referer_hash_ << '\t'
      << data.user_status_ << '\t'
      << data.country_ << '\t' << data.passback_request_id_ << '\t' << data.floor_cost_ << '\t';

    if (data.urls_.empty())
    {
      out << '-';
    }
    else
    {
      for (auto it = data.urls_.begin(); it != data.urls_.end(); ++it)
      {
        if (it != data.urls_.begin())
        {
          out << ' ';
        }
        out << *it;
      }
    }

    out << '\t' << data.opt_in_section_;
    return out;
  }
} // namespace AdServer::LogProcessing
