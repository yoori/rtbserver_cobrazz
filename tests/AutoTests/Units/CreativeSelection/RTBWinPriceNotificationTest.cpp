
#include "RTBWinPriceNotificationTest.hpp"

REFLECT_UNIT(RTBWinPriceNotificationTest) (
  "CreativeSelection",
  AUTO_TEST_SLOW );

namespace
{
  typedef AutoTest::UserBindRequest UserBindRequest;
  typedef AutoTest::OpenRTBRequest OpenRTBRequest;
  typedef AutoTest::TanxRequest TanxRequest;
  typedef AutoTest::AdClient AdClient;

  class OpenRTB;
  class TanX;
  class OpenX;
  class Allyes;
  class LiveRail;

  template<typename RTB>
  struct RTBTraits
  {};

  // Common traits for RTBs that use OpenRTB protocol
  template<>
  struct RTBTraits<OpenRTB>
  {
    typedef AutoTest::OpenRTBRequest Request;
    typedef AutoTest::OpenRTBResponseChecker ResponseChecker;
    typedef ResponseChecker::Expected Expected;
    typedef unsigned long SetterArgType;
    typedef Expected& (Expected::* ExpectedAdSetter)(SetterArgType);
  };

  template<>
  struct RTBTraits<TanX>
  {
    typedef AutoTest::TanxRequest Request;
    typedef AutoTest::TanxResponseChecker ResponseChecker;
    typedef ResponseChecker::Expected Expected;
    typedef std::string SetterArgType;
    typedef Expected& (Expected::* ExpectedAdSetter)(const SetterArgType&);
  };

  // OpenX uses OpenRTB proto
  template<>
  struct RTBTraits<OpenX> : public RTBTraits<OpenRTB>
  {};

  // Allyes uses OpenRTB proto
  template<>
  struct RTBTraits<Allyes> : public RTBTraits<OpenRTB>
  {};

  // LiveRail uses OpenRTB proto
  template<>
  struct RTBTraits<LiveRail> : public RTBTraits<OpenRTB>
  {};

  template<typename RTB>
  struct RTBParamsMapping
  {
    // Request
    static const typename RTBTraits<RTB>::Request::Member aid;
    static const typename RTBTraits<RTB>::Request::Member src;
    static const typename RTBTraits<RTB>::Request::Member url;
    static const typename RTBTraits<RTB>::Request::Member user_id;
    static const typename RTBTraits<RTB>::Request::Member size;

    // Response
    static const typename RTBTraits<RTB>::ExpectedAdSetter expected_ad_id;
    static std::string get_ad_instance(
      const typename RTBTraits<RTB>::ResponseChecker& checker);
    static std::string get_nurl(
      const typename RTBTraits<RTB>::ResponseChecker& checker);
  };

  template<typename RTB>
  struct RTBMacro
  {
    static const char* win_price_macro;
    static const char* click_macro;
  };

  // Default RTB params mapping
  template<typename RTB> const typename RTBTraits<RTB>::Request::Member
  RTBParamsMapping<RTB>::aid = &RTBTraits<RTB>::Request::aid;

  template<typename RTB> const typename RTBTraits<RTB>::Request::Member
  RTBParamsMapping<RTB>::src = &RTBTraits<RTB>::Request::src;

  template<typename RTB> const typename RTBTraits<RTB>::Request::Member
  RTBParamsMapping<RTB>::url = &RTBTraits<RTB>::Request::url;

  template<typename RTB> const typename RTBTraits<RTB>::Request::Member
  RTBParamsMapping<RTB>::user_id = &RTBTraits<RTB>::Request::external_user_id;

  template<typename RTB> const typename RTBTraits<RTB>::ExpectedAdSetter
  RTBParamsMapping<RTB>::expected_ad_id = &RTBTraits<RTB>::Expected::creative_id;

  template<typename RTB> const typename RTBTraits<RTB>::Request::Member
  RTBParamsMapping<RTB>::size = &RTBTraits<RTB>::Request::debug_size;

  template<typename RTB> std::string
    RTBParamsMapping<RTB>::get_nurl(
      const typename RTBTraits<RTB>::ResponseChecker& checker)
  {
    return checker.bids().nurl;
  }

  // Specializations
  // TanX
  template<> const RTBTraits<TanX>::Request::Member
  RTBParamsMapping<TanX>::user_id = &RTBTraits<TanX>::Request::tid;

  template<> std::string
    RTBParamsMapping<TanX>::get_ad_instance(
       const RTBTraits<TanX>::ResponseChecker& checker)
  {
    return checker.ad().html_snippet();
  };

  template<> std::string
    RTBParamsMapping<TanX>::get_nurl(
       const RTBTraits<TanX>::ResponseChecker& /*checker*/)
  {
    return std::string(); // There's no 'notification url' param for TanX
  };

  template<>
  const char* RTBMacro<TanX>::win_price_macro = "%%SETTLE_PRICE%%";

  template<>
  const char* RTBMacro<TanX>::click_macro = "%%CLICK_URL_PRE_UNENC%%";

  // OpenRTB
  template<> const RTBTraits<OpenRTB>::Request::Member
  RTBParamsMapping<OpenRTB>::url = &RTBTraits<OpenRTB>::Request::referer;

  template<> const RTBTraits<OpenRTB>::ExpectedAdSetter
  RTBParamsMapping<OpenRTB>::expected_ad_id = &RTBTraits<OpenRTB>::Expected::adid;

  template<> std::string
    RTBParamsMapping<OpenRTB>::get_ad_instance(
       const RTBTraits<OpenRTB>::ResponseChecker& checker)
  {
    return checker.bids().adm;
  };

  //OpenX
  template<>
  struct RTBParamsMapping<OpenX> : public RTBParamsMapping<OpenRTB>
  {};

  template<>
  const char* RTBMacro<OpenX>::win_price_macro = "{winning_price}";

  template<>
  const char* RTBMacro<OpenX>::click_macro = "{clickurl}";

  //Allyes
  template<>
  struct RTBParamsMapping<Allyes> : public RTBParamsMapping<OpenRTB>
  {};

  template<>
  const char* RTBMacro<Allyes>::win_price_macro = "${AUCTION_PRICE}";

  template<>
  const char* RTBMacro<Allyes>::click_macro = "{!ssp_click_url}";

  //LiveRail
  template<>
  struct RTBParamsMapping<LiveRail> : public RTBParamsMapping<OpenRTB>
  {};

  template<>
  const char* RTBMacro<LiveRail>::win_price_macro = "$WINNING_PRICE";

  template<>
  const char* RTBMacro<LiveRail>::click_macro = "";

  // Helper funcs
  double
  isp_amount(
    double adv_amount,
    double pub_amount,
    double rate,
    double colo_rs,
    double colo_rate = 1)
  {
    return colo_rs * colo_rate * (adv_amount - pub_amount) / rate ;
  }

  bool get_token_value(const String::SubString& src, const char* token, std::string& value)
  {
    String::SubString::SizeType pos = src.find(token);
    if (pos == String::SubString::NPOS)
    {
      return false;
    }
    pos += strlen(token);

    String::SubString::SizeType eol_pos = src.find('\n', pos);

    String::SubString result(src.substr(pos, eol_pos - pos));

    String::StringManip::trim(result);

    if (!result.empty() && result[0] == '=')
    {
      result.erase_front(1);// remove '=' symbol from result
      String::StringManip::trim(result);
      result.assign_to(value);
      return true;
    }

    return false;
  };

  void
  js_decode(std::string& text) /*throw(String::StringManip::InvalidFormatException)*/
  {
    std::string::size_type size = text.size();
    if (!size)
    {
      return;
    }

    char* dest = &text[0];
    const char* src = dest;

    while (size-- > 0)
    {
      if (*src == '\\')
      {
        switch (src[1])
        {
          case 'x':
            if (size < 4 || !String::AsciiStringManip::HEX_NUMBER(src[2]) || !String::AsciiStringManip::HEX_NUMBER(src[3]))
            {
              Stream::Error ostr;
              ostr << FNS << "broken encoding in '" << src << "'";
              throw String::StringManip::InvalidFormatException(ostr);
            }

            *dest++ = String::AsciiStringManip::hex_to_char(src[2], src[3]);
            src += 4;
            size -= 3;
            break;
          case 'u':
            if (!strncmp(&src[2], "2028", 4))
            {
              strcpy(dest, "\xE2\x80\xA8");
              dest += 3;
              src += 6;
              size -= 3;
            }
            else if (!strncmp(&src[2], "2029", 4))
            {
              strcpy(dest, "\xE2\x80\xA9");
              dest += 3;
              src += 6;
              size -= 3;
            }
            else
            {
              *dest++ = *src++;
            }
            break;
          default:
            Stream::Error ostr;
            ostr << FNS << "broken encoding in '" << src << "'";
            throw String::StringManip::InvalidFormatException(ostr);
        }
      }
      else
      {
        *dest++ = *src++;
      }
    }
    text.resize(dest - &text[0]);
  }
};

template<typename RTB, size_t Count>
RTBWinPriceNotificationTest::StoredRequests
RTBWinPriceNotificationTest::process_requests_(
  const CaseRequest (&requests)[Count])
{
  StoredRequests stored_requests;
  for (size_t i = 0; i < Count; ++i)
  {
    AdClient adexchange(AdClient::create_user(this));

    UserBindRequest user_bind;
    user_bind.ssp_user_id(adexchange.get_uid());
    if (requests[i].src)
    { user_bind.src(requests[i].src); }

    adexchange.process_request(user_bind);

    // to avoid min_RTB_user_age check
    adexchange.process_post(
      typename RTBTraits<RTB>::Request().debug_time(now_ - 24*60*60));

    if (requests[i].referer_kw)
    {
      AutoTest::NSLookupRequest nslookup_request;
      nslookup_request.referer_kw(fetch_string(requests[i].referer_kw));
      adexchange.process_request(nslookup_request);
    }

    typename RTBTraits<RTB>::Request request;
    RTBParamsMapping<RTB>::user_id(request, adexchange.get_uid());
    request.debug_time(now_).ip(fetch_string("GUINEA/IP"));

    if (requests[i].aid)
    { RTBParamsMapping<RTB>::aid(request, fetch_int(requests[i].aid)); }

    if (requests[i].src)
    { RTBParamsMapping<RTB>::src(request, requests[i].src); }

    if (requests[i].referer)
    { RTBParamsMapping<RTB>::url(request, fetch_string(requests[i].referer)); }

    if (requests[i].size)
    { RTBParamsMapping<RTB>::size(request, fetch_string(requests[i].size).c_str()); }

    //request.debug_ccg(fetch_int("LiveRail/CCG#8"));

    adexchange.process_post(request);

    typename RTBTraits<RTB>::Expected expected;

    if (requests[i].ad_id)
    {
      typename RTBTraits<RTB>::SetterArgType expected_ad;
      fetch(expected_ad, requests[i].ad_id);
      (expected.*(RTBParamsMapping<RTB>::expected_ad_id))(expected_ad);
    }

    typename RTBTraits<RTB>::ResponseChecker checker(adexchange, expected);

    FAIL_CONTEXT(checker.check(), "Checking expected ad");

    std::string ad_instance = RTBParamsMapping<RTB>::get_ad_instance(checker);

    AdClient client(AdClient::create_nonoptin_user(this));

    if (requests[i].flags & CF_INST_REQ)
    {
      // Make inst request
      const String::SubString REGEXP("^<(script|iframe)\\s+.*\\bsrc=\"([^\\s]+)\".*");
      String::RegEx re(REGEXP);
      String::RegEx::Result result;

      FAIL_CONTEXT(AutoTest::predicate_checker(re.search(result, ad_instance)),
        "Got unexpected ad markup");

      FAIL_CONTEXT(AutoTest::equal_checker(3, result.size()).check(),
        "got unexpected ad markup");

      HTTP::BrowserAddress inst_request(result[2]);

      client.process_request(inst_request.url());

      FAIL_CONTEXT(AutoTest::equal_checker(200, client.req_status()).check(),
        "Inst request status check");

      String::StringManip::mime_url_decode(
        client.req_response_data(), ad_instance);
    }

    if (requests[i].flags & (CF_NURL_REQ | CF_STORE_NURL))
    {
      std::string nurl;
      String::StringManip::replace(
        RTBParamsMapping<RTB>::get_nurl(checker), nurl,
        String::SubString(RTBMacro<RTB>::win_price_macro),
        String::SubString(requests[i].win_price));

      FAIL_CONTEXT(AutoTest::predicate_checker(!nurl.empty()),
        "Server must return non empty notification url");

      if (!(requests[i].flags & CF_NO_TPARAM))
      {
        FAIL_CONTEXT(AutoTest::equal_checker(
          nurl.find("t=n"),
          std::string::npos,
          AutoTest::CT_NOT_EQUAL).check(),
          "ImpTrack request must contain t=n param");
      }

      nurl += "&debug-time=" + encoded_now_;

      if (requests[i].flags & CF_NURL_REQ)
      {
        client.process_request(nurl);
        FAIL_CONTEXT(AutoTest::equal_checker(200, client.req_status()).check(),
          "ImpTrack request status check");
      }

      if (requests[i].flags & CF_STORE_NURL)
      {
        stored_requests.push_back(nurl);
      }
    }

    std::string imp_track_token;
    if (ad_instance.empty())
    {
      get_token_value(client.req_response_data(), "TRACKPIXEL", imp_track_token);
    }
    else
    {
      FAIL_CONTEXT(
        AutoTest::predicate_checker(
          get_token_value(ad_instance, "TRACKPIXEL", imp_track_token)),
          "Can't get TRACKPIXEL token value from response");
    }

    if (requests[i].flags & (CF_IMPTRACK_REQ | CF_STORE_IMPTRACK))
    {
      std::string imp_track_url;

      FAIL_CONTEXT(AutoTest::predicate_checker(!imp_track_token.empty()),
        "Server must return non empty track pixel url");

      if (requests[i].win_price)
      {
        String::StringManip::replace(
          imp_track_token, imp_track_url,
          String::SubString(RTBMacro<RTB>::win_price_macro),
          String::SubString(requests[i].win_price));
      }

      imp_track_url += "&debug-time=" + encoded_now_;

      if (requests[i].flags & CF_IMPTRACK_REQ)
      {
        client.process_request(imp_track_url);
        FAIL_CONTEXT(AutoTest::equal_checker(200, client.req_status()).check(),
          "ImpTrack request status check");
      }

      if(requests[i].flags & CF_STORE_IMPTRACK)
      {
        stored_requests.push_back(imp_track_url);
      }
    }
    else
    {
      FAIL_CONTEXT(AutoTest::predicate_checker(imp_track_token.empty()),
        "Server must return empty track pixel url");
    }

    if (requests[i].flags & CF_CLICK_REQ)
    {
      std::string click_url,
                  encoded_click_url;
      FAIL_CONTEXT(
        AutoTest::predicate_checker(
          get_token_value(ad_instance, "CLICK", click_url)),
        "Can't get CLICK token value from response");

      // Delete click macro
      String::StringManip::replace(
        click_url, encoded_click_url,
        String::SubString(RTBMacro<RTB>::click_macro),
        String::SubString());

      String::StringManip::mime_url_decode(
        encoded_click_url, click_url);

      client.process_request(click_url + "*amp*debug-time*eql*" + encoded_now_);
    }

    if (requests[i].flags & CF_STORE_CLICKS)
    {
      std::string creatives;
      FAIL_CONTEXT(AutoTest::predicate_checker(get_token_value(ad_instance, "CREATIVES_JSON", creatives)),
                   "Can't get CREATIVES_JSON token value from response");

      JsonValue root_value;
      JsonAllocator json_allocator;
      char* parse_end = 0;

      js_decode(creatives);
      FAIL_CONTEXT(AutoTest::predicate_checker(json_parse(&creatives[0], &parse_end, &root_value, json_allocator) == JSON_PARSE_OK),
                   "Can't parse CREATIVES_JSON token value from response");

      size_t size = stored_requests.size();
      for (JsonIterator it1 = begin(root_value); it1 != end(root_value); ++it1)
      {
        for (JsonIterator it2 = begin(it1->value); it2 != end(it1->value); ++it2)
        {
          if (std::string(it2->key) == "CLICK")
          {
            std::string url;
            String::StringManip::replace(it2->value.toString(), url, String::SubString(RTBMacro<RTB>::click_macro), String::SubString());
            String::StringManip::mime_url_decode(url);
            stored_requests.push_back(url);
          }
        }
      }

      FAIL_CONTEXT(AutoTest::predicate_checker(stored_requests.size() != size),
                   "Can't get CLICK keys from CREATIVES_JSON");
    }
  }

  return stored_requests;
}

template<size_t Count>
void RTBWinPriceNotificationTest::select_current_stats_(
  const CaseStats (&expected)[Count],
  Stats& stats,
  Diffs& diffs)
{
  for (size_t i = 0; i < Count; ++i)
  {
    Stat::Key key;
    key.colo_id(fetch_int("Colocation"));

    if (expected[i].pub_account_id)
    { key.pub_account_id(fetch_int(expected[i].pub_account_id)); }

    if (expected[i].tag_id)
    { key.tag_id(fetch_int(expected[i].tag_id)); }

    if (expected[i].cc_id)
    { key.cc_id(fetch_int(expected[i].cc_id)); }

    Stat stat(key);
    stat.description("#" + strof(i + 1));
    stat.select(pq_conn_);
    stats.push_back(stat);

    diffs.push_back(
      Diff().
        requests(expected[i].requests).
        imps(expected[i].imps).
        clicks(expected[i].clicks).
        adv_amount(expected[i].adv_amount).
        pub_amount(expected[i].pub_amount).
        isp_amount(expected[i].isp_amount));
  }
}

void
RTBWinPriceNotificationTest::tanx_()
{
  add_descr_phrase("Start TanX pricing case");

  const char e_prise_2100[] = "AQAAAAAAAFTZ6DsAAAAAAAAJzFcN%2ByK%2Blw%3D%3D";
  const double d_prise_2100 = 0.021;
  const char e_prise_0[]  = "AQAAAAAAAFRqNMUAAAAAAACFre3T76kAuQ%3D%3D";

  const double adv_price = fetch_float("Global/CPMs/CPM") / 1000;
  const double adv_rate = fetch_float("AdvertiserRate");
  const double revenue_share = fetch_float("RevenueShare");

  // win_price must be less than display_adv_price * pub_rate / adv_rate =
  //   // 2100 * 10 / 65 ~ 323.08
  const CaseRequest REQUESTS[] = {
    { "TanX/Account", "script-tanx", "TanX/120x240", "URL", "TanX/Creatives/120x240", CF_INST_REQ | CF_IMPTRACK_REQ, e_prise_2100 },
    { "TanX/Account", 0, "TanX/728x90", "URL", "TanX/Creatives/728x90", CF_IMPTRACK_REQ, e_prise_0 },
    { "OpenRTB/Account", 0, "TanX/120x240", "URL", "TanX/Creatives/120x240", CF_IMPTRACK_REQ, e_prise_2100 },
    { "DefaultRTB/Account", 0, "TanX/728x90", "URL", "TanX/Creatives/728x90", CF_IMPTRACK_REQ, e_prise_2100 }
  };

  const CaseStats EXPECTED_STATS[] = {
    { "TanX/Account", 0, "TanX/CCIDs/120x240",
      1, 1, 0, adv_price, d_prise_2100, isp_amount(adv_price, d_prise_2100, adv_rate, revenue_share ) },
    { "TanX/Account", 0, "TanX/CCIDs/728x90",
      1, 1, 0, adv_price, 0, isp_amount(adv_price, 0, adv_rate, revenue_share ) },
    { "OpenRTB/Account", 0, "TanX/CCIDs/120x240",
      1, 1, 0, adv_price, 0, isp_amount(adv_price, 0, adv_rate, revenue_share ) },
    { "DefaultRTB/Account", 0, "TanX/CCIDs/728x90",
      1, 1, 0, adv_price, 0, isp_amount(adv_price, 0, adv_rate, revenue_share ) },
  };

  Stats stats;
  Diffs diffs;

  select_current_stats_(EXPECTED_STATS, stats, diffs);
  process_requests_<TanX>(REQUESTS);
  ADD_WAIT_CHECKER("Waiting expected stats for RequestStatsHourly",
    AutoTest::stats_diff_checker(pq_conn_, diffs, stats));
}

void
RTBWinPriceNotificationTest::openx_()
{
  add_descr_phrase("Start OpenX pricing case");

  const double display_adv_price = fetch_float("Global/CPMs/CPM") / 1000;
  const double text_adv_price1 = fetch_float("Global/CPMs/TextCPM#1") / 1000;
  const double text_adv_price2 = fetch_float("Global/CPMs/TextCPM#2") / 1000;
  const double adv_rate = fetch_float("AdvertiserRate");
  const double revenue_share = fetch_float("RevenueShare");

  const CaseRequest REQUESTS[] = {
    { "OpenX/Account", "body-openx", "OpenX/120x240", "URL", "OpenX/CREATIVEIDs/120x240", CF_IMPTRACK_REQ,
      "AAABSF-N74RrAXoKDD6twkm6GvtyOT-MZJtv4w" }, // 0
    { "OpenX/Account", "body-openx", "OpenX/121x241", "URL", "OpenX/CREATIVEIDs/121x241", CF_STORE_IMPTRACK, 0 },
    { "OpenX/Account", "body-openx", "OpenX/122x242", "URL", "OpenX/CREATIVEIDs/122x242", 0, 0 },
    { "OpenX/Account", "body-openx", "OpenX/469x61", "URL", "OpenX/CREATIVEIDs/469x61", CF_IMPTRACK_REQ,
      "AAABSFqK8NDmTomWnooz1fTMSirr3D5Twbtwmw" },
    { "OpenX/Account", "body-openx", "OpenX/471x63", "URL", "OpenX/CREATIVEIDs/471x63", CF_IMPTRACK_REQ,
      "AAABSTg8gEy8WY1DJooGzh2bGRJOH3qbCjXmpw" },
    { "DefaultRTB/Account", "body-openx", "OpenX/120x240", "URL", "OpenX/CREATIVEIDs/120x240", CF_IMPTRACK_REQ,
      "AAABTEuiJAky9i1Fef34IZccI6jIzxI0uVvyow" }, // 1000
    { "OpenX/Account", "body-openx", "OpenX/470x62", "URL", "OpenX/CREATIVEIDs/470x62", CF_IMPTRACK_REQ,
      "AAABTEukEOQX8-PuD2nFFPVWbfYHa38_Cn_XPA" }, // 2000
    { "OpenX/Account", "body-openx", "OpenX/472x64", "Search", "Global/CREATIVEIDs/Text", CF_IMPTRACK_REQ,
      "AAABTEuk04JJ4p_52uhJO9B-tGrx4A8Ptmk0Jw" } // 2200
  };

  const CaseStats EXPECTED_STATS[] = {
    { "OpenX/Account", "OpenX/RTB/Tags/120x240", "OpenX/CCIDs/120x240",
      1, 1, 0, display_adv_price, 0, isp_amount(display_adv_price, 0, adv_rate, revenue_share) },
    { "OpenX/Account", "OpenX/RTB/Tags/121x241", "OpenX/CCIDs/121x241", 1, 0, 0, 0, 0, 0 },
    { "OpenX/Account", "OpenX/RTB/Tags/122x242", "OpenX/CCIDs/122x242",
      1, 1, 0, display_adv_price, 0, isp_amount(display_adv_price, 0, adv_rate, revenue_share) },
    { "OpenX/Account", "OpenX/RTB/Tags/469x61", "OpenX/CCIDs/469x61",
      1, 1, 0, display_adv_price, 0, isp_amount(display_adv_price, 0, adv_rate, revenue_share) },
    { "OpenX/Account", "OpenX/RTB/Tags/471x63", "OpenX/CCIDs/471x63",
      1, 1, 0, display_adv_price, 0, isp_amount(display_adv_price, 0, adv_rate, revenue_share) },
    { "DefaultRTB/Account", "OpenX/DEFAULT_RTB/Tags/120x240", "OpenX/CCIDs/120x240",
      1, 1, 0, display_adv_price, 0, isp_amount(display_adv_price, 0, adv_rate, revenue_share) },
    { "OpenX/Account", "OpenX/RTB/Tags/470x62", "OpenX/CCIDs/470x62",
      1, 1, 0, display_adv_price, .002, isp_amount(display_adv_price, .002, adv_rate, revenue_share) },
    { "OpenX/Account", "OpenX/RTB/Tags/472x64", "Global/CCIDs/Text",
      1, 1, 0, 0, .000733, isp_amount(0, .000733, adv_rate, revenue_share) },
    { "OpenX/Account", "OpenX/RTB/Tags/472x64", "Global/CCIDs/ChannelText#1",
      1, 1, 0, text_adv_price1, .000733, isp_amount(text_adv_price1, .000733, adv_rate, revenue_share) },
    { "OpenX/Account", "OpenX/RTB/Tags/472x64", "Global/CCIDs/ChannelText#2",
      1, 1, 0, text_adv_price2, .000733, isp_amount(text_adv_price2, .000733, adv_rate, revenue_share) }
  };

  Stats stats;
  Diffs diffs;

  select_current_stats_(EXPECTED_STATS, stats, diffs);
  process_requests_<OpenX>(REQUESTS);
  ADD_WAIT_CHECKER("Waiting expected stats for RequestStatsHourly",
    AutoTest::stats_diff_checker(pq_conn_, diffs, stats));
}


void
RTBWinPriceNotificationTest::allyes_()
{
  add_descr_phrase("Start Allyes pricing case");

  const double display_adv_price = fetch_float("Global/CPMs/CPM") / 1000;
  const double adv_rate = fetch_float("AdvertiserRate");
  const double revenue_share = fetch_float("RevenueShare");

  // win_price must be less than display_adv_price * pub_rate / adv_rate =
  // 2100 * 10 / 65 ~ 323.08
  const CaseRequest REQUESTS[] = {
    { "Allyes/Account", "body-allyes", "Allyes/120x240", "URL",
      "Allyes/CREATIVEIDs/120x240", CF_NURL_REQ | CF_STORE_IMPTRACK, "95" },

    { "Allyes/Account", "body-allyes", "Allyes/469x61", "URL",
      "Allyes/CREATIVEIDs/469x61", CF_STORE_NURL | CF_IMPTRACK_REQ, "95" },

    { "Allyes/Account", "body-allyes", "Allyes/470x62", "URL",
      "Allyes/CREATIVEIDs/470x62", CF_STORE_NURL | CF_IMPTRACK_REQ | CF_CLICK_REQ, "95" },

    { "Allyes/Account", "body-allyes", "Allyes/472x64", 0,
      "Global/CREATIVEIDs/ChannelText#1", CF_NURL_REQ | CF_STORE_IMPTRACK, "95" },

    { "Allyes/Account", "body-allyes", "Allyes/160x600", "URL",
      "Allyes/CREATIVEIDs/160x600", CF_NURL_REQ | CF_STORE_NURL, "95" }
  };

  const CaseStats EXPECTED_STATS[] = {
    { "Allyes/Account", 0, "Allyes/CCIDs/120x240", 1, 0, 0, 0, 0, 0 },

    { "Allyes/Account", 0, "Allyes/CCIDs/469x61", 1, 0, 0, 0, 0, 0 },

    { "Allyes/Account", 0, "Allyes/CCIDs/470x62", 1, 0, 0, 0, 0, 0 },

    { "Allyes/Account", 0, "Global/CCIDs/ChannelText#1", 1, 0, 0, 0, 0, 0 },

    { "Allyes/Account", 0, "Global/CCIDs/ChannelText#2", 1, 0, 0, 0, 0, 0 },

    { "Allyes/Account", 0, "Allyes/CCIDs/160x600",
      1, 1, 0, display_adv_price, .095, isp_amount(display_adv_price, .095, adv_rate, revenue_share) }
  };

  Stats stats;
  Diffs diffs;

  select_current_stats_(EXPECTED_STATS, stats, diffs);
  allyes_stored_requests_ = process_requests_<Allyes>(REQUESTS);
  ADD_WAIT_CHECKER("Waiting expected stats for RequestStatsHourly",
    AutoTest::stats_diff_checker(pq_conn_, diffs, stats));
}

void
RTBWinPriceNotificationTest::allyes_final_()
{
  add_descr_phrase("Process stored requests");

  const double display_adv_price = fetch_float("Global/CPMs/CPM") / 1000;
  const double text_adv_price1 = fetch_float("Global/CPMs/TextCPM#1") / 1000;
  const double text_adv_price2 = fetch_float("Global/CPMs/TextCPM#2") / 1000;
  const double adv_rate = fetch_float("AdvertiserRate");
  const double revenue_share = fetch_float("RevenueShare");

  const CaseStats EXPECTED_STATS[] = {
    { "Allyes/Account", 0, "Allyes/CCIDs/120x240",
      0, 1, 0, display_adv_price, .095, isp_amount(display_adv_price, .095, adv_rate, revenue_share) },

    { "Allyes/Account", 0, "Allyes/CCIDs/469x61",
      0, 1, 0, display_adv_price, .095, isp_amount(display_adv_price, .095, adv_rate, revenue_share) },

    { "Allyes/Account", 0, "Allyes/CCIDs/470x62", 
      0, 1, 1, display_adv_price, .095, isp_amount(display_adv_price, .095, adv_rate, revenue_share) },

    { "Allyes/Account", 0, "Global/CCIDs/ChannelText#1",
      0, 1, 0, text_adv_price1, .0475, isp_amount(text_adv_price1, .0475, adv_rate, revenue_share) },

    { "Allyes/Account", 0, "Global/CCIDs/ChannelText#2",
      0, 1, 0, text_adv_price2, .0475, isp_amount(text_adv_price1, .0475, adv_rate, revenue_share) },

    { "Allyes/Account", 0, "Allyes/CCIDs/160x600", 0, 0, 0, 0, 0, 0 }
  };

  Stats stats;
  Diffs diffs;

  select_current_stats_(EXPECTED_STATS, stats, diffs);

  AdClient client(AdClient::create_nonoptin_user(this));
  for (auto it = allyes_stored_requests_.cbegin();
       it != allyes_stored_requests_.cend(); ++it)
  {
    client.process_request(*it);
  }

  ADD_WAIT_CHECKER("Waiting expected stats for RequestStatsHourly",
    AutoTest::stats_diff_checker(pq_conn_, diffs, stats));
}

void
RTBWinPriceNotificationTest::liverail_()
{
  add_descr_phrase("Start LiveRail pricing case");

  const double revenue_share  = fetch_float("RevenueShare");

  const double curr1_rate     = fetch_float("LiveRail/CURRENCY#1");
  const double curr2_rate     = fetch_float("LiveRail/CURRENCY#2");

  const double pub1_comm      = fetch_float("LiveRail/COMMISSIONS/PUBLISHER#1");
  const double pub2_comm      = fetch_float("LiveRail/COMMISSIONS/PUBLISHER#2");
  const double pub3_comm      = fetch_float("LiveRail/COMMISSIONS/PUBLISHER#3");

  const double pub1_curr_conv = 1;
  const double pub2_curr_conv = 1;
  const double pub3_curr_conv = curr2_rate/curr1_rate;

  const double adv1_comm      = fetch_float("LiveRail/COMMISSIONS/ADVERTISER#1");
  const double adv2_comm      = fetch_float("LiveRail/COMMISSIONS/ADVERTISER#2");

  const double cpm1           = fetch_float("LiveRail/CPM/CCG#1") / 1000;
  const double cpm2           = fetch_float("LiveRail/CPM/CCG#2") / 1000;

  const CaseRequest REQUESTS[] = {
    /**  1 */ { "LiveRail/PUBLISHER#2", "url-liverail",  "LiveRail/120x240", "LiveRail/URL#1", "LiveRail/CC#1", CF_NURL_REQ | CF_NO_TPARAM | CF_IMPTRACK_REQ, "1"},
    /**  2 */ { "LiveRail/PUBLISHER#2", "body-liverail", "LiveRail/469x61",  "LiveRail/URL#1", "LiveRail/CC#2", CF_IMPTRACK_REQ, "0"           },
    /**  3 */ { "LiveRail/PUBLISHER#2", "body-liverail", "LiveRail/470x62",  "LiveRail/URL#1", "LiveRail/CC#3", CF_IMPTRACK_REQ, "0.5"         },
    /**  4 */ { "LiveRail/PUBLISHER#2", "body-liverail", "LiveRail/120x240", "LiveRail/URL#2", "LiveRail/CC#6", CF_IMPTRACK_REQ, "0.75"        },
    /**  5 */ { "LiveRail/PUBLISHER#1", "body-liverail", "LiveRail/120x240", "LiveRail/URL#1", "LiveRail/CC#1", CF_IMPTRACK_REQ, "0.5"         },
    /**  6 */ { "LiveRail/PUBLISHER#1", "body-liverail", "LiveRail/120x240", "LiveRail/URL#2", "LiveRail/CC#6", CF_IMPTRACK_REQ, "0.5"         },
    /**  7 */ { "LiveRail/PUBLISHER#2", "body-liverail", "LiveRail/471x63",  "LiveRail/URL#1", "LiveRail/CC#4", CF_IMPTRACK_REQ, "0.151515151" },
    /**  8 */ { "LiveRail/PUBLISHER#3", "body-liverail", "LiveRail/120x240", "LiveRail/URL#1", "LiveRail/CC#1", CF_IMPTRACK_REQ, "0.2357"      },
    /**  9 */ { "LiveRail/PUBLISHER#3", "body-liverail", "LiveRail/469x61",  "LiveRail/URL#1", "LiveRail/CC#2", CF_IMPTRACK_REQ, "0.2349"      },
    /** 10 */ { "LiveRail/PUBLISHER#2", "body-liverail", "LiveRail/472x64",  nullptr,          "LiveRail/CC#8", CF_IMPTRACK_REQ|CF_STORE_CLICKS, "0.55", "LiveRail/KEYWORDS"},
    /** 11 */ { "LiveRail/PUBLISHER#1", "body-liverail", "LiveRail/472x64",  nullptr,          "LiveRail/CC#8", CF_IMPTRACK_REQ|CF_STORE_CLICKS, "0.55", "LiveRail/KEYWORDS"},
    /** 12 */ { "LiveRail/PUBLISHER#2", "body-liverail", "LiveRail/474x66",  "LiveRail/URL#1", "LiveRail/CC#5", CF_IMPTRACK_REQ, "1.01"        },
    /** 13 */ { "LiveRail/PUBLISHER#2", "body-liverail", "LiveRail/474x66",  "LiveRail/URL#2", "LiveRail/CC#7", CF_IMPTRACK_REQ, "0,5"         }
  };

  const CaseStats EXPECTED_STATS[] = {
    /**  1 */ { "LiveRail/PUBLISHER#2", "LiveRail/TAGS/PUBLISHER#2/120x240", "LiveRail/CC#1", 1, 1, 0, cpm1*(1-adv1_comm), cpm1*(1-adv1_comm)*1.0   *(1-pub2_comm)*pub2_curr_conv, isp_amount(cpm1*(1-adv1_comm), cpm1*(1-adv1_comm)*1.0   *(1-pub2_comm), curr1_rate, revenue_share)},
    /**  2 */ { "LiveRail/PUBLISHER#2", "LiveRail/TAGS/PUBLISHER#2/469x61",  "LiveRail/CC#2", 1, 1, 0, cpm1*(1-adv1_comm), cpm1*(1-adv1_comm)*0.0   *(1-pub2_comm)*pub2_curr_conv, isp_amount(cpm1*(1-adv1_comm), cpm1*(1-adv1_comm)*0.0   *(1-pub2_comm), curr1_rate, revenue_share)},
    /**  3 */ { "LiveRail/PUBLISHER#2", "LiveRail/TAGS/PUBLISHER#2/470x62",  "LiveRail/CC#3", 1, 1, 0, cpm1*(1-adv1_comm), cpm1*(1-adv1_comm)*0.6   *(1-pub2_comm)*pub2_curr_conv, isp_amount(cpm1*(1-adv1_comm), cpm1*(1-adv1_comm)*0.5   *(1-pub2_comm), curr1_rate, revenue_share)},
    /**  4 */ { "LiveRail/PUBLISHER#2", "LiveRail/TAGS/PUBLISHER#2/120x240", "LiveRail/CC#6", 1, 1, 0, cpm2*(1-adv2_comm), cpm2*(1-adv2_comm)*0.75  *(1-pub2_comm)*pub2_curr_conv, isp_amount(cpm2*(1-adv2_comm), cpm2*(1-adv2_comm)*0.75  *(1-pub2_comm), curr1_rate, revenue_share)},
    /**  5 */ { "LiveRail/PUBLISHER#1", "LiveRail/TAGS/PUBLISHER#1/120x240", "LiveRail/CC#1", 1, 1, 0, cpm1*(1-adv1_comm), cpm1*(1-adv1_comm)*0.5   *(1-pub1_comm)*pub1_curr_conv, isp_amount(cpm1*(1-adv1_comm), cpm1*(1-adv1_comm)*0.5   *(1-pub1_comm), curr1_rate, revenue_share)},
    /**  6 */ { "LiveRail/PUBLISHER#1", "LiveRail/TAGS/PUBLISHER#1/120x240", "LiveRail/CC#6", 1, 1, 0, cpm2*(1-adv2_comm), cpm2*(1-adv2_comm)*0.5   *(1-pub1_comm)*pub1_curr_conv, isp_amount(cpm2*(1-adv2_comm), cpm2*(1-adv2_comm)*0.5   *(1-pub1_comm), curr1_rate, revenue_share)},
    /**  7 */ { "LiveRail/PUBLISHER#2", "LiveRail/TAGS/PUBLISHER#2/471x63",  "LiveRail/CC#4", 1, 1, 0, cpm1*(1-adv1_comm), cpm1*(1-adv1_comm)*1     *(1-pub2_comm)*pub2_curr_conv, isp_amount(cpm1*(1-adv1_comm), cpm1*(1-adv1_comm)*1     *(1-pub2_comm), curr1_rate, revenue_share)},
    /**  8 */ { "LiveRail/PUBLISHER#3", "LiveRail/TAGS/PUBLISHER#3/120x240", "LiveRail/CC#1", 1, 1, 0, cpm1*(1-adv1_comm), cpm1*(1-adv1_comm)*0.2357*(1-pub3_comm)*pub3_curr_conv, isp_amount(cpm1*(1-adv1_comm), cpm1*(1-adv1_comm)*0.2357*(1-pub3_comm), curr1_rate, revenue_share)},
    /**  9 */ { "LiveRail/PUBLISHER#3", "LiveRail/TAGS/PUBLISHER#3/469x61",  "LiveRail/CC#2", 1, 1, 0, cpm1*(1-adv1_comm), cpm1*(1-adv1_comm)*0.2349*(1-pub3_comm)*pub3_curr_conv, isp_amount(cpm1*(1-adv1_comm), cpm1*(1-adv1_comm)*0.2349*(1-pub3_comm), curr1_rate, revenue_share)},
    /** 10 */ { "LiveRail/PUBLISHER#2", "LiveRail/TAGS/PUBLISHER#2/472x64",  "LiveRail/CC#8", 1, 1, 1, 1050.21,            1.21006595,                                             isp_amount(1050.21,            1.21006595,                              curr1_rate, revenue_share)},
    /** 10 */ { "LiveRail/PUBLISHER#2", "LiveRail/TAGS/PUBLISHER#2/472x64",  "LiveRail/CC#9", 1, 1, 1, 2100.11,            1.15506039,                                             isp_amount(2100.11,            1.15506039,                              curr1_rate, revenue_share)},
    /** 11 */ { "LiveRail/PUBLISHER#1", "LiveRail/TAGS/PUBLISHER#1/472x64",  "LiveRail/CC#8", 1, 1, 1, 1050.21,            0.84704617,                                             isp_amount(1050.21,            0.84704617,                              curr1_rate, revenue_share)},
    /** 11 */ { "LiveRail/PUBLISHER#1", "LiveRail/TAGS/PUBLISHER#1/472x64",  "LiveRail/CC#9", 1, 1, 1, 2100.11,            0.80854228,                                             isp_amount(2100.11,            0.80854228,                              curr1_rate, revenue_share)},
    /** 12 */ { "LiveRail/PUBLISHER#2", "LiveRail/TAGS/PUBLISHER#2/474x66",  "LiveRail/CC#5", 1, 1, 0, cpm1*(1-adv1_comm), cpm1*(1-adv1_comm)*1.0   *(1-pub2_comm)*pub2_curr_conv, isp_amount(cpm1*(1-adv1_comm), cpm1*(1-adv1_comm)*1.0   *(1-pub2_comm), curr1_rate, revenue_share)},
    /** 13 */ { "LiveRail/PUBLISHER#2", "LiveRail/TAGS/PUBLISHER#2/474x66",  "LiveRail/CC#7", 1, 1, 0, cpm2*(1-adv2_comm), cpm2*(1-adv2_comm)*1.0   *(1-pub2_comm)*pub2_curr_conv, isp_amount(cpm2*(1-adv2_comm), cpm2*(1-adv2_comm)*1.0   *(1-pub2_comm), curr1_rate, revenue_share)}
  };

  Stats stats;
  Diffs diffs;

  select_current_stats_(EXPECTED_STATS, stats, diffs);
  StoredRequests stored_requests = process_requests_<LiveRail>(REQUESTS);

  AdClient client(AdClient::create_nonoptin_user(this));
  for (auto it = stored_requests.begin(); it != stored_requests.cend(); ++it)
  {
    client.process_request(*it + "*amp*debug-time*eql*" + encoded_now_);
  }

  ADD_WAIT_CHECKER("Waiting expected stats for RequestStatsHourly", AutoTest::stats_diff_checker(pq_conn_, diffs, stats));
}

void
RTBWinPriceNotificationTest::set_up()
{
  String::StringManip::mime_url_encode(
    now_.get_gm_time().format(AutoTest::DEBUG_TIME_FORMAT),
    encoded_now_);
}

void
RTBWinPriceNotificationTest::pre_condition()
{

}


bool
RTBWinPriceNotificationTest::run()
{
  AUTOTEST_CASE(tanx_(), "TanX");
  AUTOTEST_CASE(openx_(), "OpenX");
  AUTOTEST_CASE(allyes_(), "Allyes");

  check();

  AUTOTEST_CASE(allyes_final_(), "Allyes");
  //AUTOTEST_CASE(liverail_(), "LiveRail");

  return true;
}

void
RTBWinPriceNotificationTest::post_condition()
{
}

void
RTBWinPriceNotificationTest::tear_down()
{}
