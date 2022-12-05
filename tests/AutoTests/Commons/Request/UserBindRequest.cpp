
#include "UserBindRequest.hpp"

namespace AutoTest
{
  const char* UserBindRequest::BASE_URL = "/userbind";

  UserBindRequest::UserBindRequest() :
    BaseRequest(BASE_URL, BaseRequest::RT_ENCODED),
    ssp_user_id(this, "ssp_user_id"),
    id(this, "id"),
    tid(this, "tid"),
    src(this, "src"),
    pbf(this, "pbf"),
    gi(this, "gi"),
    uid(this, "uid"),
    user_agent(this, "User-Agent"),
    referer(this, "Referer"),
    x_forwarded_for(this, "x-Forwarded-For")
  { }
  
  UserBindRequest::UserBindRequest(const UserBindRequest& other) :
    BaseRequest(BASE_URL, BaseRequest::RT_ENCODED),
    ssp_user_id(this, other.ssp_user_id),
    id(this, other.id),
    tid(this, other.tid),
    src(this, other.src),
    pbf(this, other.pbf),
    gi(this, other.gi),
    uid(this, other.uid),
    user_agent(this, other.user_agent),
    referer(this, other.referer),
    x_forwarded_for(this, other.x_forwarded_for)
  { }
} // namespace AutoTest



