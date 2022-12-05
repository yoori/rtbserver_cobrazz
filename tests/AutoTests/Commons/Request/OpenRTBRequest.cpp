
#include "OpenRTBRequest.hpp"

namespace AutoTest
{
  namespace OpenRtb
  {

    const TagDescriptor EMPTY_TAG = {"", ""};
    const TagDescriptor STRING_TAG = {"\"", "\""};
    const TagDescriptor STRUCT_TAG = {"{", "}"};
    const TagDescriptor ARRAY_TAG = {"[", "]"};

  }

  template<>
  OpenRTBRequest::ParamsArray<OpenRTBRequest::ImpGroup>::ParamsArray(
    size_t size,
    OpenRTBRequest* request,
    BaseParamsContainer* group,
    const char* name,
    unsigned short flags) :
    Base(request, group, name),
    current_index_(size)
  {
    for (size_t i = 0; i < size; i++)
    {
      this->parameters_[i] =
        ParamsGenerator<OpenRTBRequest::ImpGroup>()(
          this->request_, this, strof(i).c_str(), flags);
    }
  }

  // OpenRTBRequest::Parameter

  OpenRTBRequest::Parameter::Parameter(
    BaseParamsContainer* container,
    const char* name,
    OpenRtb::TagConst tag,
    OpenRtb::EscapeJSON escape) :
    StringParam(container, name),
    tag_(tag),
    escape_(escape)
  { }

  OpenRTBRequest::Parameter::Parameter(
    BaseParamsContainer* container,
    const Parameter& other) :
    StringParam(container, other),
    tag_(other.tag_),
    escape_(other.escape_)
  { }

  OpenRTBRequest::Parameter::~Parameter() noexcept
  { }

  void
  OpenRTBRequest::Parameter::print(
    std::ostream& out,
    unsigned long indent,
    bool print_name) const
  {
    if (!empty())
    {
      if (print_name)
      {
        out << std::string(indent, ' ') << "\"" << name_ <<  "\" : ";
      }
      std::string escaped_str = escape_ == OpenRtb::JSON_ESCAPE
        ? String::StringManip::json_escape(String::SubString(raw_str()))
        : raw_str();
      out << tag_.begin << escaped_str << tag_.end;
    }
  }

  bool
  OpenRTBRequest::Parameter::print(
    std::ostream&,
    const char*,
    const char*) const
  {
    return false;
  }

  OpenRtb::TagConst
  OpenRTBRequest::Parameter::tag() const
  {
    return tag_;
  }

  // Group

  OpenRTBRequest::Group::Group(
    BaseParamsContainer* container,
    const char* name,
    bool required) :
    Parameter(container, name, OpenRtb::STRUCT_TAG),
    required_(required)
  { }

  OpenRTBRequest::Group::~Group() noexcept
  { }

  void
  OpenRTBRequest::Group::print(
    std::ostream& out,
    unsigned long indent,
    bool print_name) const
  {
    if (!name_.empty() && print_name)
    {
      out << std::string(indent, ' ') << "\"" << name_ << "\" : ";
    }

    out << tag_.begin;

    bool first = true;
    for(auto i = params_.cbegin(); i != params_.cend(); i++)
    {
      if (!(*i)->empty())
      {
        if (first)
        {
          first = false;
        }
        else
        {
          out << ",";
        }
        out << std::endl;
        static_cast<const Parameter*>(*i)->print(out, indent+2, true);
      }
    }

    out << (name_.empty()? '\n': ' ') << tag_.end;
  }

  bool
  OpenRTBRequest::Group::empty() const
  {
    for(auto i = params_.cbegin(); i != params_.cend(); i++)
    {
      if (!(*i)->empty() || required_) return false;
    }
    return true;
  }

  bool
  OpenRTBRequest::Group::need_encode() const
  {
    return false;
  }

  void
  OpenRTBRequest::Group::set_param_val(
        const String::SubString&)
  {
    throw GroupAccessError("Can't set value for group parameter");
  }

  //OpenRTBRequest::ImpGroup

  OpenRTBRequest::ImpGroup::ImpGroup(
    OpenRTBRequest* request,
    BaseParamsContainer* group,
    const char* name,
    unsigned short flags) :
    Group(group, name),
    banner_(this, "banner", flags & OpenRtb::RF_SEND_BANNER),
    ext_(&banner_, "ext"),
    video_(this, "video"),
    video_ext_(&video_, "ext"),
    request_(request),
    width(request, &banner_, "w"),
    height(request, &banner_, "h"),
    battr(request, &banner_, "battr"),
    pos(request, &banner_, "pos"),
    min_cpm_price(request, this, "bidfloor"),
    min_cpm_price_currency_code(request, this, "bidfloorcur"),
    id(request, this, "id", "imp-test", flags & OpenRtb::RF_SET_DEFS),
    matching_ad_id(request, &ext_, "matching_ad_id"),
    type(request, &ext_, "type"),
    secure(request, this, "secure"),
    mimes         (request, &video_,     "mimes"         ),
    minduration   (request, &video_,     "minduration"   ),
    maxduration   (request, &video_,     "maxduration"   ),
    protocol      (request, &video_,     "protocol"      ),
    playbackmethod(request, &video_,     "playbackmethod"),
    companionad   (request, &video_,     "companionad"   ),
    startdelay    (request, &video_,     "startdelay"    ),
    linearity     (request, &video_,     "linearity"     ),
    video_height  (request, &video_,     "h"             ),
    video_width   (request, &video_,     "w"             ),
    video_pos     (request, &video_,     "pos"           ),
    video_battr   (request, &video_,     "battr"         ),
    ext_adtype    (request, &video_ext_, "adtype"        )
  { }

  OpenRTBRequest::ImpGroup::ImpGroup(
    OpenRTBRequest* request,
    BaseParamsContainer* group,
    const ImpGroup& other) :
    Group(group, other.name_.c_str()),
    banner_(this, "banner", true),
    ext_(&banner_, "ext"),
    video_(this, "video"),
    video_ext_(&video_, "ext"),
    request_(request),
    width(request, &banner_, other.width),
    height(request, &banner_, other.height),
    battr(request, &banner_, other.battr),
    pos(request, &banner_, other.pos),
    min_cpm_price(request, this, other.min_cpm_price),
    min_cpm_price_currency_code(request, this, other.min_cpm_price_currency_code),
    id(request, this, other.id),
    matching_ad_id(request, &ext_, other.matching_ad_id),
    type(request, &ext_, other.type),
    secure(request, this, other.secure),
    mimes         (request, &video_,     other.mimes         ),
    minduration   (request, &video_,     other.minduration   ),
    maxduration   (request, &video_,     other.maxduration   ),
    protocol      (request, &video_,     other.protocol      ),
    playbackmethod(request, &video_,     other.playbackmethod),
    companionad   (request, &video_,     other.companionad   ),
    startdelay    (request, &video_,     other.startdelay    ),
    linearity     (request, &video_,     other.linearity     ),
    video_height  (request, &video_,     other.video_height  ),
    video_width   (request, &video_,     other.video_width   ),
    video_pos     (request, &video_,     other.video_pos     ),
    video_battr   (request, &video_,     other.video_battr   ),
    ext_adtype    (request, &video_ext_, other.ext_adtype    )
  { }

  OpenRTBRequest::ImpGroup::~ImpGroup() noexcept
  { }

  // OpenRTBRequest

  const char* OpenRTBRequest::BASE_URL = "/openrtb";
  const char* OpenRTBRequest::DEFAULT_IP = "195.91.155.102";

  OpenRTBRequest::OpenRTBRequest(unsigned short flags) :
    BaseRequest(BASE_URL, BaseRequest::RT_ENCODED),
    body_(this, ""),
    imp(1, this, &body_, "imp", flags),
    site_(&body_, "site"),
    site_ext_(&site_, "ext"),
    device_(&body_, "device"),
    user_(&body_, "user"),
    ext_(&body_, "ext"),
    content_type(
      this,
      "Content-Type",
      "application/json",
      flags & OpenRtb::RF_SET_DEFS),
    width(this, imp[0].width),
    height(this, imp[0].height),
    battr(this, imp[0].battr),
    pos(this, imp[0].pos),
    min_cpm_price(this, imp[0].min_cpm_price),
    min_cpm_price_currency_code(this, imp[0].min_cpm_price_currency_code),
    id(this, imp[0].id),
    bcat(this, &body_, "bcat"),
    cat(this, &site_, "cat"),
    referer(this, &site_, "page"),
    ip(this, &device_, "ip", DEFAULT_IP, flags & OpenRtb::RF_SET_DEFS),
    user_agent(this, &device_, "ua", DEFAULT_USER_AGENT, flags & OpenRtb::RF_SET_DEFS),
    external_user_id(this, &user_, "id", "1234", flags & OpenRtb::RF_SET_DEFS ),
    user_id(this, &user_, "buyeruid"),
    is_test(this, &ext_, "is_test"),
    is_test_string(this, &ext_, "is_test"),
    request_id(this, &body_, "id", "request-test", flags & OpenRtb::RF_SET_DEFS),
    matching_ad_id(this, imp[0].matching_ad_id),
    type(this, imp[0].type),
    secure(this, imp[0].secure),
    mimes         (this, imp[0].mimes         ),
    minduration   (this, imp[0].minduration   ),
    maxduration   (this, imp[0].maxduration   ),
    protocol      (this, imp[0].protocol      ),
    playbackmethod(this, imp[0].playbackmethod),
    companionad   (this, imp[0].companionad   ),
    startdelay    (this, imp[0].startdelay    ),
    linearity     (this, imp[0].linearity     ),
    video_height  (this, imp[0].video_height  ),
    video_width   (this, imp[0].video_width   ),
    video_pos     (this, imp[0].video_pos     ),
    video_battr   (this, imp[0].video_battr   ),
    ext_adtype    (this, imp[0].ext_adtype    ),
    ssl_enabled   (this, &site_ext_, "ssl_enabled"),
    aid(this, "aid"),
    src(this, "src"),
    random(this, "random"),
    debug_ccg(this, "debug.ccg"),
    debug_time(this, "debug.time"),
    debug_size(this, "debug.size")
  { }

  OpenRTBRequest::OpenRTBRequest(
    const OpenRTBRequest& other) :
    BaseRequest(BASE_URL, BaseRequest::RT_ENCODED),
    body_(this, ""),
    imp(this, &body_, other.imp),
    site_(&body_, "site"),
    site_ext_(&site_, "ext"),
    device_(&body_, "device"),
    user_(&body_, "user"),
    ext_(&body_, "ext"),
    content_type(this, other.content_type),
    width(this, imp[0].width),
    height(this, imp[0].height),
    battr(this, imp[0].battr),
    pos(this, imp[0].pos),
    min_cpm_price(this, imp[0].min_cpm_price),
    min_cpm_price_currency_code(this, imp[0].min_cpm_price_currency_code),
    id(this, imp[0].id),
    bcat(this, &body_, other.bcat),
    cat(this, &site_, other.cat),
    referer(this, &site_, other.referer),
    ip(this, &device_, other.ip),
    user_agent(this, &device_, other.user_agent),
    external_user_id(this, &user_, other.external_user_id),
    user_id(this, &user_, other.user_id),
    is_test(this, &ext_, other.is_test),
    is_test_string(this, &ext_, other.is_test_string),
    request_id(this, &body_, other.request_id),
    matching_ad_id(this, imp[0].matching_ad_id),
    type(this, imp[0].type),
    secure(this, imp[0].secure),
    mimes         (this, imp[0].mimes         ),
    minduration   (this, imp[0].minduration   ),
    maxduration   (this, imp[0].maxduration   ),
    protocol      (this, imp[0].protocol      ),
    playbackmethod(this, imp[0].playbackmethod),
    companionad   (this, imp[0].companionad   ),
    startdelay    (this, imp[0].startdelay    ),
    linearity     (this, imp[0].linearity     ),
    video_height  (this, imp[0].video_height  ),
    video_width   (this, imp[0].video_width   ),
    video_pos     (this, imp[0].video_pos     ),
    video_battr   (this, imp[0].video_battr   ),
    ext_adtype    (this, imp[0].ext_adtype    ),
    ssl_enabled   (this, &site_ext_, other.ssl_enabled),
    aid(this, other.aid),
    src(this, other.src),
    random(this, other.random),
    debug_ccg(this, other.debug_ccg),
    debug_time(this, other.debug_time),
    debug_size(this, other.debug_size)
  { }

  OpenRTBRequest::~OpenRTBRequest() noexcept
  { }

  std::string
  OpenRTBRequest::body() const
  {
    std::ostringstream body;
    body_.print(body, 0, false);
    return body.str();
  }
}

