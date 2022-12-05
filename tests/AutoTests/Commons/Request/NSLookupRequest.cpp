
#include "NSLookupRequest.hpp"

namespace AutoTest
{
  const char* NSLookupRequest::BASE_URL = "/get";

  const char* NSLookupRequest::PROFILING_URL = "/services/profile";

  const char* NSLookupRequest::APP_DEFAULT = "PS";

  const char* NSLookupRequest::VERSION_DEFAULT = "1.3.0-3.ssv1";

  const unsigned long  NSLookupRequest::RANDOM_DEFAULT = 0;

  const unsigned short NSLookupRequest::XINFOPSID_DEFAULT = 0;

  const char* NSLookupRequest::FORMAT_DEFAULT = "unit-test";

  const char* NSLookupRequest::DEBUG_INFO_DEFAULT = "header";

  const char* NSLookupRequest::DEFAULT_COUNTRY = "gn";
  
  NSLookupRequest::NSLookupRequest(bool set_defs) :
    BaseRequest(BASE_URL, BaseRequest::RT_ENCODED),
    debug_info(this, "require-debug-info", DEBUG_INFO_DEFAULT, true),
    app(this, "a", APP_DEFAULT, true),
    version(this, "v", VERSION_DEFAULT, true),
    random(this, "rnd", RANDOM_DEFAULT, set_defs),
    xinfopsid(this, "xinfopsid", XINFOPSID_DEFAULT, set_defs),
    format(this, "fm", FORMAT_DEFAULT, set_defs),
    referer_kw(this, "kw"),
    referer(this, "Referer"),
    ft(this, "ft"),
    search(this, "Referer"),
    muid(this, "muid"),
    tuid(this, "tuid"),
    hid(this, "hid"),
    uid (this, "uid"),
    tid(this, "t"),
    tag_inv(this, "tag.inv"),
    colo(this, "c"),
    debug_time(this, "debug-time"),
    setuid(this, "setuid"),
    orig(this, "orig"),
    testrequest(this, "testrequest", 0, set_defs),
    passback(this, "pb"),
    pt(this, "pt"),
    debug_nofraud(this, "debug.nofraud"),
    loc_coord(this, "l.c"),
    loc_name(this, "l.n", DEFAULT_COUNTRY, set_defs),
    pl(this, "pl"),
    kn(this, "kn"),
    vis(this, "vis"),
    user_agent(this, "User-Agent", DEFAULT_USER_AGENT, set_defs),
    preclick(this, "preclick"),
    debug_ip(this, "debug.ip"),
    rm_tuid(this, "rm-muid")
  { }
  
  NSLookupRequest::NSLookupRequest(const NSLookupRequest& other) :
    BaseRequest(BASE_URL, BaseRequest::RT_ENCODED),
    debug_info(this, other.debug_info),
    app(this, other.app),
    version(this, other.version),
    random(this, other.random),
    xinfopsid(this, other.xinfopsid),
    format(this, other.format),
    referer_kw(this, other.referer_kw),
    referer(this, other.referer),
    ft(this, other.ft),
    search(this, other.search),
    muid(this, other.muid),
    tuid(this, other.tuid),
    hid(this, other.hid),
    uid (this, other.uid),
    tid(this, other.tid),
    tag_inv(this, other.tag_inv),
    colo(this, other.colo),
    debug_time(this, other.debug_time),
    setuid(this, other.setuid),
    orig(this, other.orig),
    testrequest(this, other.testrequest),
    passback(this, other.passback),
    pt(this, other.pt),
    debug_nofraud(this, other.debug_nofraud),
    loc_coord(this, other.loc_coord),
    loc_name(this, other.loc_name),
    pl(this, other.pl),
    kn(this, other.kn),
    vis(this, other.vis),
    user_agent(this, other.user_agent),
    preclick(this, other.preclick),
    debug_ip(this, other.debug_ip),
    rm_tuid(this, other.rm_tuid)
  { }

  std::string NSLookupRequest::profiling_url() const
  {
    std::ostringstream out;
    out << PROFILING_URL;
    print_params_(out);
    return out.str();
  }
} // namespace AutoTest



