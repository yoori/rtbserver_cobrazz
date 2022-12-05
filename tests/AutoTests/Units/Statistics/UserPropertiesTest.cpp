
#include "UserPropertiesTest.hpp"

REFLECT_UNIT(UserPropertiesTest) (
  "Statistics",
  AUTO_TEST_SLOW
);

namespace
{

  typedef AutoTest::TemporaryAdClient TemporaryAdClient;
  typedef AutoTest::AdClient AdClient;

  const char USER_AGENT_HTTP_HEADER[] = "User-Agent";
  const char DEFAULT_FORMAT[] = "unit-test-imp";
  const char DEFAULT_COUNTRY[] = "ug";

  std::string get_property_name(const std::string& name)
  {
    return name.substr(0, name.find("/"));
  }

  const char * UserStatus[] = { "I", "U", "O", "I", "P" };

  typedef void (*ClientGenerator)(
    BaseUnit*, const AutoTest::NSLookupRequest&,
    const AutoTest::CreativeList&,
    const AutoTest::ConsequenceActionList&,
    unsigned long);

  const ClientGenerator UserGenerators[] =
  {
    &AutoTest::ClientGenerator<AdClient,
      &AdClient::create_user>::do_ad_requests,
    &AutoTest::ClientGenerator<AdClient,
      &AdClient::create_nonoptin_user>::do_ad_requests,
    &AutoTest::ClientGenerator<AdClient,
      &AdClient::create_optout_user>::do_ad_requests,
    &AutoTest::ClientGenerator<TemporaryAdClient>::do_ad_requests,
    &AutoTest::ClientGenerator<AdClient,
      &AdClient::create_probe_user>::do_ad_requests
  };

}

namespace ORM = ::AutoTest::ORM;


const std::string UserPropertiesTest::PropertyKey::key() const
{
  return get_property_name(name) +
    user_status + value;
}

bool UserPropertiesTest::PropertyKey::operator<(
  const PropertyKey &other) const
{
  return key() < other.key();
}

void UserPropertiesTest::set_up()
{
  add_descr_phrase("Setup");
}

void UserPropertiesTest::tear_down()
{
  add_descr_phrase("Tear down");
}

bool
UserPropertiesTest::run()
{

  AUTOTEST_CASE(
    os_browser_case(),
    "OS & Browser property");

  AUTOTEST_CASE(
    country_case(),
    "Country property");

  AUTOTEST_CASE(
    version_case(),
    "ClientVersion property");

  AUTOTEST_CASE(
    app_props_case(),
    "Client property");

  AUTOTEST_CASE(
    user_status_case(),
    "User statuses");
  
  AUTOTEST_CASE(
    inactive_tag_case(),
    "Inactive tags");
  
  AUTOTEST_CASE(
    no_ads_isp_case(),
    "ISP.optout_serving = NONE");

  AUTOTEST_CASE(
    probe_case(),
    "Probe user");

  AUTOTEST_CASE(
    upvalue_case(),
    "OS & Browser limit");
  
  return true;
}

void
UserPropertiesTest::os_browser_case()
{
  // OS & Browser property, use "User-Agent" HTTP property for enabling this property
  const Property PROPERTIES[] =
  {
    // Mozilla/5.0 (X11; U; Linux amd64; rv:5.0) Gecko/20100101 Firefox/5.0 (Debian)
    // OsVersion =  Linux
    // BrowserVersion = Firefox 5.0
    {
      "OsVersion/1,BrowserVersion/1", &NSLookupRequest::user_agent,
      "UserAgent/1", "TID", "KWD1", "CC/02",
      PSE_OPTIN, 80, 60, 40, 20, 80, 0
    },

    // Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1; .NET CLR 1.1.4322; .NET CLR 2.0.50727; InfoPath.1)
    // OsVersion = Windows NT 5.1
    // BrowserVersion = IE 6.0
    {
      "OsVersion/2,BrowserVersion/2", &NSLookupRequest::user_agent,
      "UserAgent/2", "TID", "KWD1", "CC/02",
      PSE_OPTIN, 140, 70, 35, 20, 140, 0
    },

    // Mozilla/5.0 (Windows; U; Windows NT 5.1; SV1; .NET CLR 1.1.4322; .NET CLR 2.0.50727; InfoPath.1) Gecko/20071008
    // OsVersion = Windows NT 5.1
    // BrowserVersion = Mozilla 5.0
    {
      "OsVersion/3,BrowserVersion/3", &NSLookupRequest::user_agent,
      "UserAgent/3", "TID", "KWD1", "CC/02",
      PSE_OPTIN, 60, 30, 15, 10, 60, 0
    },

    // Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; ru; rv:1.8.1.8)
    // OsVersion = Windows NT 5.1
    // BrowserVersion = IE 6.0
    {
      "OsVersion/4,BrowserVersion/4", &NSLookupRequest::user_agent,
      "UserAgent/4", "TID", "KWD1", "CC/02",
      PSE_OPTIN, 20, 10, 5, 1, 20, 0
    },

    // Mozilla/5.0 (Macintosh; Intel Mac OS X 10_7_3) AppleWebKit/534.55.3 (KHTML, like Gecko) Version/5.1.3 Safari/534.53.10
    // OsVersion = Mac OS X 10.7.3
    // BrowserVersion = Safari 5.1.3
    {
      "OsVersion/5,BrowserVersion/5", &NSLookupRequest::user_agent,
      "UserAgent/5", "TID", "KWD1", "CC/02",
      PSE_OPTIN, 40, 20, 10, 2, 40, 0
    },

    // Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/536.6 (KHTML, like Gecko) Chrome/20.0.1092.0 Safari/536.6
    // OsVersion = Windows NT 6.1
    // BrowserVersion = Chrome 20.0.1092.0
    {
      "OsVersion/6,BrowserVersion/6", &NSLookupRequest::user_agent,
      "UserAgent/6", "TID", "KWD1", "CC/02",
      PSE_OPTIN, 70, 35, 14, 3, 70, 0
    },
 
  };

  test_case(PROPERTIES,"COLO/OS");
  
}

void
UserPropertiesTest::country_case()
{
  // 'country' parameter of the ns-lookup request
  const Property PROPERTIES[] =
  {
    {
      "CountryCode/01", &NSLookupRequest::loc_name,
      "CountryCode/01", "TID", "KWD1", "CC/01",
      PSE_OPTIN, 20, 10, 5, 2, 20, 0
    },
    {
      "CountryCode/02", &NSLookupRequest::loc_name,
      "CountryCode/02", "TID", "KWD1", "CC/02",
      PSE_OPTIN, 8, 4, 2, 1, 8, 0
    },
    {
      "CountryCode/03", &NSLookupRequest::loc_name,
      "CountryCode/03", "TID", "KWD1", 0,
      PSE_OPTIN, 16, 0, 0, 0, 0, 0
    }
  };

  test_case(PROPERTIES, "COLO/COUNTRY");
  
}

void
UserPropertiesTest::version_case()
{
  const Property PROPERTIES[] =
  {
    {
      "ClientVersion/01", &NSLookupRequest::version,
      "ClientVersion/01", "TID", "KWD1", "CC/02",
      PSE_OPTIN, 16, 8, 4, 2, 16, 0
    },
    {
      "ClientVersion/02", &NSLookupRequest::version,
      "ClientVersion/02", "TID", "KWD1", "CC/02",
      PSE_OPTIN, 31, 15, 6, 3, 31, 0
    }
  };

  test_case(PROPERTIES, "COLO/VERSION");
    
}

void
UserPropertiesTest::app_props_case()
{
  // 'app' parameter of the ns-lookup request
  const Property PROPERTIES[] =
  {
    {
      "Client/01",  &NSLookupRequest::app, "Client/01",
      "TID", "KWD1", "CC/02",
      PSE_OPTIN, 22, 11, 5, 4, 22, 0
    },
    {
      "Client/02", &NSLookupRequest::app, "Client/02",
      "TID", "KWD1", "CC/02",
      PSE_OPTIN, 18, 6, 3, 2, 18, 0
    }
  };

  test_case(PROPERTIES, "COLO/CLIENT_PROP");
  
}

void
UserPropertiesTest::user_status_case()
{
  const Property PROPERTIES[] =
  {
    {
      "ClientVersion/01,Client/01,OsVersion/Null,BrowserVersion/Null,CountryCode/02",
      &NSLookupRequest::referer, "URL2", "TID", 0, "CC/03",
      PSE_NON_OPTIN, 10, 5, 2, 0, 10, 0
    },
    {
      "ClientVersion/01,Client/01,OsVersion/Null,BrowserVersion/Null,CountryCode/02",
      &NSLookupRequest::referer, "URL2", "TID", 0, "CC/03",
      PSE_OPTOUT, 15, 14, 12, 0, 15, 0
    },
    {
      "ClientVersion/01,Client/01,OsVersion/Null,BrowserVersion/Null,CountryCode/01",
      &NSLookupRequest::loc_name, "CountryCode/01", 0, "KWD2", 0,
      PSE_TEMPORATY, 20, 0, 0, 0, 0, 20
    }
  };

  test_case(PROPERTIES, "COLO/STATUS");

}

void
UserPropertiesTest::inactive_tag_case()
{
  const Property PROPERTIES[] =
  {
    {
      "ClientVersion/01,Client/01,OsVersion/Null,BrowserVersion/Null,CountryCode/03",
      &NSLookupRequest::loc_name, 0, 0, "KWD1", 0,
      PSE_OPTIN, 10, 0, 0, 0, 0, 10
    },
    {
      "ClientVersion/01,Client/01,OsVersion/Null,BrowserVersion/Null,CountryCode/02",
      &NSLookupRequest::referer_kw, "KWD1", "TID", 0, "CC/02",
      PSE_OPTIN, 8, 6, 4, 1, 8, 0
    },
    {
      "ClientVersion/01,Client/01,OsVersion/Null,BrowserVersion/Null,CountryCode/02",
      &NSLookupRequest::referer_kw, "KWD1", "TIDDELETED", 0, 0,
      PSE_OPTIN, 5, 0, 0, 0, 0, 0
    },
    {
      "ClientVersion/01,Client/01,OsVersion/Null,BrowserVersion/Null,CountryCode/02",
      &NSLookupRequest::referer_kw, "KWD1", "TIDABSENT", 0, 0,
      PSE_OPTIN, 2, 0, 0, 0, 0, 0
    }
  };

  test_case(PROPERTIES, "COLO/INACTIVE");
}

void
UserPropertiesTest::no_ads_isp_case()
{
  const Property PROPERTIES[] =
  {
    {
      "ClientVersion/01,Client/01,OsVersion/Null,BrowserVersion/Null,CountryCode/02",
      &NSLookupRequest::referer, "URL1", "TID", 0, 0,
      PSE_OPTIN, 10, 0, 0, 0, 0, 0
    }
  };

  test_case(PROPERTIES, "COLO/NONE");
}

void UserPropertiesTest::probe_case()
{

  const Property PROPERTIES[] =
  {
    {
      "ClientVersion/01,Client/01,OsVersion/Null,BrowserVersion/Null,CountryCode/02",
      &NSLookupRequest::tid, "TID", 0, 0, 0,
      PSE_PROBE, 1, 0, 0, 0, 0, 0
    }
  };

  test_case(PROPERTIES, "COLO/PROBE");
}

void UserPropertiesTest::upvalue_case()
{
  std::string referer;

  String::StringManip::mime_url_encode(
    fetch_string("UserAgent/11"),
    referer);

  const Property PROPERTIES[] =
  {
    {
      "OsVersion/7,BrowserVersion/7,ClientVersion/01,Client/01,CountryCode/02",
      &NSLookupRequest::user_agent, "UserAgent/7", 0, 0, 0,
      PSE_OPTIN, 1, 0, 0, 0, 0, 1
    },
    {
      "OsVersion/8,BrowserVersion/8,ClientVersion/01,Client/01,CountryCode/02",
      &NSLookupRequest::user_agent, "UserAgent/8", 0, 0, 0,
      PSE_OPTIN, 1, 0, 0, 0, 0, 1
    },
    {
      "OsVersion/9,BrowserVersion/9,ClientVersion/01,Client/01,CountryCode/02",
      &NSLookupRequest::user_agent, "UserAgent/9", 0, 0, 0,
      PSE_OPTIN, 1, 0, 0, 0, 0, 1
    },
    {
      "OsVersion/10,BrowserVersion/10,ClientVersion/01,Client/01,CountryCode/02",
      &NSLookupRequest::user_agent, "UserAgent/10", 0, 0, 0,
      PSE_OPTIN, 1, 0, 0, 0, 0, 1
    },
    {
      "OsVersion/11,ClientVersion/01,Client/01,CountryCode/02",
      &NSLookupRequest::user_agent, "UserAgent/11", 0, 0, 0,
      PSE_OPTIN, 1, 0, 0, 0, 0, 1
    },
    {
      "OsVersion/12,ClientVersion/01,Client/01,CountryCode/02",
      &NSLookupRequest::user_agent, "UserAgent/12", 0, 0, 0,
      PSE_OPTIN, 1, 0, 0, 0, 0, 1
    },
    {
      "OsVersion/13,ClientVersion/01,Client/01,CountryCode/02",
      &NSLookupRequest::user_agent, "UserAgent/13", 0, 0, 0,
      PSE_OPTIN, 1, 0, 0, 0, 0, 1
    }
  };

  test_case(PROPERTIES, "COLO/UPVALUE");
}


template<unsigned long PropsSize>
unsigned long UserPropertiesTest::get_count(
  const UserPropertiesTest::Property (&properties)[PropsSize],
  const UserPropertiesTest::PropertyKey& property,
  Counter counter)
{
  unsigned long count = 0;
  for (unsigned int idx = 0; idx < PropsSize; idx++)
  {
    String::SubString s(properties[idx].exp_properties);
    String::StringManip::SplitComma tokenizer(s);

    String::SubString token;
    while(tokenizer.get_token(token))
    {
      if (get_property_name(property.name) == get_property_name(token.str()) &&
        property.value == fetch_string(token.str()) &&
        property.user_status == UserStatus[properties[idx].user_type])
      {
        count+= properties[idx].*(counter);
      }
    }
  }
  return count;
}

template<unsigned long PropsSize>
void UserPropertiesTest::test_case(
  const UserPropertiesTest::Property (&properties)[PropsSize],
  const char* colo_name)
{

  ORM::StatsList<ORM::UserPropertiesStats> stats;
  std::list<ORM::UserPropertiesStats::Diffs> diffs;
  
  // Initialize property keys
  PropertyKeys keys;
  for (unsigned int idx = 0; idx <  PropsSize; idx++)
  {
    String::SubString s(properties[idx].exp_properties);
    String::StringManip::SplitComma tokenizer(s);

    String::SubString token;
    while(tokenizer.get_token(token))
    {
      std::string prop(token.str());
      PropertyKey key;
      key.name = prop;
      key.value = fetch_string(prop);
      key.user_status = UserStatus[properties[idx].user_type];
      keys.insert(key);
    }

    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        properties[idx].requests >= properties[idx].impressions),
      "Requests must be more than inpressions");

    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        properties[idx].impressions >= properties[idx].clicks),
      "Impressions must be more than clicks");

    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        properties[idx].clicks >= properties[idx].actions),
      "Clicks must be more than actions");
  }

  // Initialize stats and diffs

  unsigned int idx = 0;
  for (PropertyKeys::const_iterator it = keys.begin();
       it != keys.end(); ++it, ++idx)
  {
    ORM::UserPropertiesStats stat;

    ORM::UserPropertiesStats::Key key;
    key.
      name(get_property_name(it->name)).
      value(it->value).
      colo_id(fetch_int(colo_name)).
      user_status(it->user_status).
      stimestamp(today);

    if (it->value.empty())
    {
      key.value("");
    }
        
    stat.key( key );

    stat.description(
      (std::string("UserProperties. ") + "'" + it->name + "'").c_str());

    stats.push_back(stat);

    unsigned long profiling =
      get_count(properties, *it, &Property::profiling_requests);

    ORM::UserPropertiesStats::Diffs diff;

    diff.imps(get_count(properties, *it, &Property::impressions));
    diff.clicks(get_count(properties, *it, &Property::clicks));
    diff.actions(get_count(properties, *it, &Property::actions));
    // Exclude profiling requests
    diff.requests(
      get_count(properties, *it, &Property::requests) -
        profiling);
    diff.imps_unverified(
      get_count(properties, *it, &Property::imps_unverified));
    diff.profiling_requests(profiling);

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      0,
      diff[3] + diff[5],
      AutoTest::CT_NOT_EQUAL).check(),
    "Invalid initialization "
    "(UserProperties.requests=0) of " +
      it->name + "='" + it->value + "'" );

    diffs.push_back(diff);
  }
  stats.select(pq_conn_);


  for (unsigned int prop_idx = 0; prop_idx < PropsSize; prop_idx++)
  {
    NSLookupRequest request;
    request.user_agent.clear();
    if (properties[prop_idx].tid)
    {
      request.tid =
        fetch_string(properties[prop_idx].tid);
    }
    request.colo = fetch_string(colo_name);
    if (properties[prop_idx].referer_kw)
    {
      request.referer_kw =
        fetch_string(properties[prop_idx].referer_kw);
    }
    request.format = DEFAULT_FORMAT;
    request.loc_name = DEFAULT_COUNTRY;
    request.debug_time = today;

    std::list<std::string> expected_ccs;
    if (properties[prop_idx].expected_ccid)
    {
      expected_ccs.push_back(
        fetch_string(properties[prop_idx].expected_ccid));
    }

    properties[prop_idx].request_param.clear(request);
    if (properties[prop_idx].request_param_value)
    {
      properties[prop_idx].request_param(
        request,
        fetch_string(properties[prop_idx].request_param_value));
    }
    
    const unsigned long counts[] =
    {
      properties[prop_idx].requests - properties[prop_idx].impressions,
      properties[prop_idx].impressions - properties[prop_idx].clicks,
      properties[prop_idx].clicks - properties[prop_idx].actions,
      properties[prop_idx].actions
    };

    AutoTest::ConsequenceActionList actions;
    AutoTest::ConsequenceActionType action = AutoTest::TRACK;

    for (size_t i = 0; i < countof(counts); ++i)
    {

      FAIL_CONTEXT(
        UserGenerators[properties[prop_idx].user_type](
          this, request, expected_ccs, actions,
          counts[i]),
        "Request#" + strof(prop_idx));

      actions.push_back(
        AutoTest::ConsequenceAction(
          action, today));

      action =
        static_cast<AutoTest::ConsequenceActionType>(
          action << 1);
    }
  }

  ADD_WAIT_CHECKER(
    "UserPropertiesStatsHourly check",
    AutoTest::stats_diff_checker(
      pq_conn_, diffs, stats));

}
