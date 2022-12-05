#ifndef FRONTENDS_ADFRONTEND_REQUESTINFO_HPP_
#define FRONTENDS_ADFRONTEND_REQUESTINFO_HPP_

#include <string>

#include <Generics/Time.hpp>
#include <Generics/Uuid.hpp>

#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>
#include <Commons/Containers.hpp>
#include <Commons/UserInfoManip.hpp>
#include <Frontends/FrontendCommons/Location.hpp>

namespace AdServer
{
  struct RequestInfo
  {
    RequestInfo()
      : random(0),
        do_opt_out(false),
        do_passback(false),
        have_uid_cookie(false),
        secure(false),
        assigned_client_id(false),
        remove_merged_uid(false),
        test_request(false),
        log_as_test(false),
        disable_fraud_detection(false),
        silent_match(false),
        partly_match(false),
        tag_id(0),
        user_status(AdServer::CampaignSvcs::US_UNDEFINED),
        full_referer_hash(0),
        short_referer_hash(0),
        search_engine_id(0),
        last_colo_id(0),
        colo_id(0),
        passback_by_colocation(true),
        debug_ccg(0),
        no_match(false),
        no_result(false),
        keywords_normalized(false),
        page_load_id(0),
        opt_in_cookie(0),
        ccid(0)
    {}

    unsigned long random;
    bool do_opt_out;
    bool do_passback;
    bool have_uid_cookie;
    bool secure;

    bool assigned_client_id;
    bool remove_merged_uid;
    bool test_request;
    bool log_as_test;
    bool disable_fraud_detection;
    bool silent_match;
    bool partly_match;

    Commons::Optional<bool> set_uid;

    unsigned long tag_id;
    std::string ext_tag_id;
    FrontendCommons::Location_var location;
    FrontendCommons::CoordLocation_var coord_location;
    AdServer::Commons::RequestId request_id;
    mutable AdServer::Commons::UserId client_id;
    AdServer::CampaignSvcs::UserStatus user_status;
    AdServer::Commons::UserId temp_client_id;
    AdServer::Commons::UserId household_client_id;
    AdServer::Commons::UserId merge_persistent_client_id;

    std::string referer;
    std::string referer_url_words;
    unsigned long full_referer_hash;
    unsigned long short_referer_hash;
    std::string allowable_referer;
    std::string client_app;
    std::string client_app_version;
    std::string peer_ip;
    std::string require_debug_info;
    std::string web_browser;
    std::string curct;
    std::set<unsigned long> platform_ids;
    std::string platform;
    std::string full_platform;
    std::string original_url;
    std::string passback_type;
    std::string passback_url;

    std::string page_words;

    unsigned long search_engine_id;
    std::string search_words;

    std::string full_text_words;

    std::string format;

    Generics::Time current_time;

    int last_colo_id;
    int colo_id;
    bool passback_by_colocation;

    unsigned long debug_ccg;

    bool no_match;
    bool no_result;
    bool keywords_normalized;

    std::string signed_client_id;

    std::string original_user_agent;
    std::string user_agent;

    unsigned long page_load_id; // pl - parameter
    int opt_in_cookie;
    unsigned long ccid;

    Commons::Optional<unsigned long> up_expand_space;
    Commons::Optional<unsigned long> right_expand_space;
    Commons::Optional<unsigned long> down_expand_space;
    Commons::Optional<unsigned long> left_expand_space;
    Commons::Optional<unsigned long> tag_visibility;
    std::string request_token;
    std::string preclick_url;
    std::string pub_impr_track_url;
    std::string pub_param;

    AdServer::CampaignSvcs::ChannelIdArray hit_channel_ids;
  };
}

#endif /* FRONTENDS_ADFRONTEND_REQUESTINFO_HPP_ */
