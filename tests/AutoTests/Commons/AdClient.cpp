
#include <tests/AutoTests/Commons/Common.hpp>

namespace AutoTest
{
  namespace
  {
    static const char UID_COOKIE_NAME[] = "uid";

    enum KeysEnum
    {
      KE_PUBLICKEY,
      KE_PUBLICTEMPKEY,
      KE_PRIVATEKEY,
      KE_PRIVATETEMPKEY
    };

    struct UrlAction
    {
      UrlAction(const std::string& u, const Generics::Time& t)
        : url(u), time(t) {}

      std::string url;
      Generics::Time time;
    };

    typedef std::vector<UrlAction> UrlsList;
  }


  std::string get_key_path(KeysEnum key_type)
  {
    std::string key_path = "/opt/foros/server"; // default unixcommons path

    char* unix_commons_env = getenv("unix_commons_root");
    if(unix_commons_env)
      key_path = unix_commons_env;
    key_path += "/share/uuid_keys";

    switch(key_type)
    {
    case KE_PUBLICKEY:
      key_path += "/public.der";
      break;
    case KE_PUBLICTEMPKEY:
      key_path += "/public_temp.der";
      break;
    case KE_PRIVATEKEY:
      key_path += "/private.der";
      break;
    case KE_PRIVATETEMPKEY:
      key_path += "/private_temp.der";
      break;
    }
    return key_path;
  }

  // class UuidGenarator_

  UuidGenarator_::UuidGenarator_() :
    temporary_generator_(
      get_key_path(KE_PRIVATETEMPKEY).c_str()),
    persistent_generator_(
      get_key_path(KE_PRIVATEKEY).c_str()),
    temporary_verifier_(
      get_key_path(KE_PUBLICTEMPKEY).c_str()),
    persistent_verifier_(
      get_key_path(KE_PUBLICKEY).c_str())
  { }

  std::string
  UuidGenarator_::generate(bool is_temporary)
  {
    return is_temporary?
      temporary_generator_.generate().str():
      persistent_generator_.generate().str();
  }

  Generics::SignedUuid
  UuidGenarator_::verify(
    const char* uid, bool is_temporary)
  {
    return is_temporary?
      temporary_verifier_.verify(String::SubString(uid)):
      persistent_verifier_.verify(String::SubString(uid));
  }

  std::string generate_uid(bool is_temporary)
  {
    return Generics::Singleton<UuidGenarator_>::instance().generate(is_temporary);
  }

  std::string prepare_uid(
    const std::string& uid,
    UuidUsingEnum uid_using,
    bool is_temporary)
  {
    Generics::SignedUuid suid =
      Generics::Singleton<UuidGenarator_>::instance().verify(
        uid.c_str(), is_temporary);
    switch (uid_using)
    {
      case UUE_ADMIN_PARAMETER:
        return "\\" + suid.uuid().to_string();
      case UUE_ADMIN_PARAMVALUE:
        return suid.uuid().to_string();
      default:
        std::string uid_escaped;
        String::StringManip::mark(
          suid.uuid().to_string().c_str(),
          uid_escaped,
          String::AsciiStringManip::REGEX_META,
          '\\');
        return uid_escaped;
    }
  }

  const std::string&
  get_client_address(BaseUnit* test,
                     unsigned short flags)
    /*throw(eh::Exception)*/
  {
    ClusterTypeEnum cluster = flags & UF_CENTRAL_FRONTEND
      ? CTE_CENTRAL
      : !(flags & UF_AD_FRONTEND) & test->get_config().check_service(CTE_PROFILING, STE_FRONTEND)
        ? CTE_PROFILING
        : test->get_config().check_service(CTE_ALL_REMOTE, STE_FRONTEND)
          ? CTE_ALL_REMOTE
          : CTE_ALL;

    return flags & UF_FRONTEND_MINOR?
      test->get_config().get_service(cluster, STE_FRONTEND, 1).address:
      test->get_config().get_service(cluster, STE_FRONTEND).address;
  }

  // class AdClient

  AdClient AdClient::create_user(
    BaseUnit* test,
    unsigned short flags)
    /*throw(eh::Exception)*/
  {
    AutoTest::AdClient
      client(
        get_client_address(test, flags).c_str(),
        flags & UF_NULL_LOGGER?
          BaseAdClient::LT_NULL: BaseAdClient::LT_BASE,
        test);
    client.process_request(OptOutRequest().op("in"));
    return client;
  }

  AdClient AdClient::create_user(
    BaseUnit* test,
    const AutoTest::Time& debug_time,
    unsigned short flags)
    /*throw(eh::Exception)*/
  {
    AutoTest::AdClient
      client(
        get_client_address(test, flags).c_str(),
        flags & UF_NULL_LOGGER?
          BaseAdClient::LT_NULL: BaseAdClient::LT_BASE,
        test);
    client.process_request(OptOutRequest().op("in").debug_time(debug_time));
    return client;
  }

  AdClient AdClient::create_undef_user(
    BaseUnit* test,
    unsigned short flags)
    /*throw(eh::Exception)*/
  {
    std::string uid = AutoTest::generate_uid(false);
    AutoTest::AdClient
      client(
        get_client_address(test, flags).c_str(),
        flags & UF_NULL_LOGGER?
          BaseAdClient::LT_NULL: BaseAdClient::LT_BASE,
        test);
    client.set_uid(uid, false);
    return client;
  }

  AdClient AdClient::create_probe_user(
      BaseUnit* test,
      unsigned short flags)
      /*throw(eh::Exception)*/
  {
    AutoTest::AdClient
      client(
        get_client_address(test, flags).c_str(),
        flags & UF_NULL_LOGGER?
          BaseAdClient::LT_NULL: BaseAdClient::LT_BASE,
        test);
    client.set_probe_uid();
    return client;
  }


  AdClient AdClient::create_nonoptin_user(
    BaseUnit* test,
    unsigned short flags)
    /*throw(eh::Exception)*/
  {
    AutoTest::AdClient
      client(
        get_client_address(test, flags).c_str(),
        flags & UF_NULL_LOGGER?
          BaseAdClient::LT_NULL: BaseAdClient::LT_BASE,
        test);
    return client;
  }

  AdClient AdClient::create_optout_user(
    BaseUnit* test,
    unsigned short flags)
    /*throw(eh::Exception)*/
  {
    AutoTest::AdClient
      client(
        AutoTest::AdClient::create_nonoptin_user(test, flags));
    client.add_http_header("Referer", "autotests.webwise.com");
    client.process_request(OptOutRequest().op("out"));
    return client;
  }

  AdClient AdClient::create_optout_user(
    BaseUnit* test,
    const AutoTest::Time& debug_time,
    unsigned short flags)
    /*throw(eh::Exception)*/
  {
    AutoTest::AdClient
      client(
        AutoTest::AdClient::create_nonoptin_user(test, flags));
    client.process_request(OptOutRequest().op("out").debug_time(debug_time));
    return client;
  }

  bool AdClient::uses_profiling_cluster() /*throw(eh::Exception)*/
  {
    if (unit_->get_config().check_service(CTE_PROFILING, STE_FRONTEND))
    {
      const GlobalConfig::ServiceList& frontends =
        unit_->get_config().get_services(CTE_PROFILING, STE_FRONTEND);
      for (auto it = frontends.begin(); it != frontends.end(); ++it)
      {
        if (get_base_url() == it->address)
        {
          return true;
        }
      }
    }
    return false;
  }

  void AdClient::process_request(const BaseRequest& request, const char* detail_info)
    /*throw(eh::Exception)*/
  {
    set_headers_(request.headers());

    process_request_(request.url().c_str(), detail_info);
  }

  void AdClient::process_request(const NSLookupRequest& request, const char* detail_info)
    /*throw(eh::Exception)*/
  {
    set_headers_(request.headers());

    process_request_(
      (uses_profiling_cluster()
        ? request.profiling_url()
        : request.url()).c_str(),
      detail_info);
  }

  unsigned int AdClient::process(const BaseRequest& request, bool suppress_exceptions)
    /*throw(eh::Exception)*/
  {
    set_headers_(request.headers());

    return process(request.url(), suppress_exceptions);
  }

  unsigned int AdClient::process(const NSLookupRequest& request, bool suppress_exceptions)
    /*throw(eh::Exception)*/
  {
    set_headers_(request.headers());

    return process(
      uses_profiling_cluster()
        ? request.profiling_url()
        : request.url(),
      suppress_exceptions);
  }

  unsigned int AdClient::process(const std::string& request, bool suppress_exceptions)
    /*throw(eh::Exception)*/
  {
    if(suppress_exceptions)
    {
      try
      {
        process_request(request);
      }
      catch (eh::Exception& e)
      {
        AutoTest::Logger::thlog().stream(Logging::Logger::INFO, "AdClient") <<
          "Exception: " << e.what();
      }
      catch (...)
      {}
    }
    else
    {
      process_request(request);
    }
    return req_status();
  }

  void AdClient::process_request(const char* url, const char* detail_info)
      /*throw(eh::Exception)*/
  {
    process_request_(url, detail_info);
  }


  void AdClient::process_request_(const char* url, const char* detail_info)
    /*throw(eh::Exception)*/
  {
    stored_request_info_ = "";
    if (detail_info)
    {
      stored_request_info_ += "(";
      stored_request_info_ += detail_info;
      stored_request_info_ += ")";
    }

    logger().stream(Logging::Logger::TRACE) <<
     "User#" << client_index_ <<
     ". Sending request#" <<
      request_count_ << " " << stored_request_info_ << "...";

    {
      BaseAdClient::process_request(url);
      std::string debug_info_string;
      find_header_value("Debug-Info", debug_info_string);
      debug_info.parse(debug_info_string);
    }

    logger().stream(Logging::Logger::TRACE) <<
      "User#" << client_index_ <<
      ". Sending request#" <<
      request_count_++ << " " << stored_request_info_ << " done.";
  }

  void AdClient::set_headers_(
    const HTTP::HeaderList& headers)
    /*throw(eh::Exception)*/
  {
    for ( HTTP::HeaderList::const_iterator
            it = headers.begin();
          it != headers.end();++it)
    {
      if (it->name != "Cookie")
      {
        add_http_header(it->name, it->value);
      }
    }
  }

  void
  AdClient::process_post_request(
    const std::string& url,
    const std::string& body,
    const char* detail_info)
    /*throw(eh::Exception)*/
  {
    stored_request_info_ = "";
    if (detail_info)
    {
      stored_request_info_ += "(";
      stored_request_info_ += detail_info;
      stored_request_info_ += ")";
    }

    logger().stream(Logging::Logger::TRACE) <<
      "User#" << client_index_ <<
      ". Sending POST request#" <<
      request_count_ << " " << stored_request_info_ << "...";

    BaseAdClient::process_request(url, body, HTTP::HTTP_Connection::HM_Post);

    logger().stream(Logging::Logger::TRACE) <<
      "User#" << client_index_ <<
      ". Sending POST request#" <<
      request_count_++ << " " << stored_request_info_ << " done.";
  }

  unsigned int
  AdClient::process_post(
    const BaseRequest& request,
    bool suppress_exceptions)
    /*throw(eh::Exception)*/
  {
    set_headers_(request.headers());

    request.set_decoder(request_);

    if(suppress_exceptions)
    {
      try
      {
        process_post_request(request.url().c_str(), request.body());
      }
      catch (const BaseAdClient::Exception& e)
      {
        logger().stream(Logging::Logger::TRACE) << "Exception: " << e.what();
      }
    }
    else
    {
      process_post_request(request.url().c_str(), request.body());
    }
    return req_status();
  }

  void AdClient::repeat_request(const char* detail_info) /*throw(eh::Exception)*/
  {
    std::string url = get_stored_url();

    set_headers_(stored_request_->request_headers());

    if (!detail_info)
    {
      process_request_(url.c_str(), stored_request_info_.c_str());
    }
    else
    {
      process_request_(url.c_str(), detail_info);
    }
  }

  void
  AdClient::process_request(const std::string url, const char* detail_info)
    /*throw(eh::Exception)*/
  {
    process_request_(url.c_str(), detail_info);
  }

  void
  AdClient::set_probe_uid()
    /*throw(eh::Exception)*/
  {
    get_cookies().clear();
    set_uid(PROBE_UID, false);
  }

  void
  AdClient::merge(
    const TemporaryAdClient& client_temp,
    const BaseRequest& request)
      /*throw(eh::Exception)*/
  {
    merge_(client_temp, request.url());
  }

  void
  AdClient::merge(
    const TemporaryAdClient& client_temp,
    const NSLookupRequest& request)
      /*throw(eh::Exception)*/
  {
    merge_(client_temp,
           uses_profiling_cluster()
            ? request.profiling_url()
            : request.url());
  }

  void AdClient::merge_(const TemporaryAdClient& client_temp,
                        const std::string& url)
      /*throw(eh::Exception)*/
  {
    process_request(
      url + "&tuid=" + client_temp.get_tuid(),
      "Merge");
  }


  void
  AdClient::do_ad_requests(
    const CreativeList& ccids,
    const ConsequenceActionList& action_list,
    const HTTP::HeaderList& headers)
    /*throw(eh::Exception)*/
  {

    UrlsList urls;

    for (ConsequenceActionList::const_iterator
           action = action_list.begin();
           action != action_list.end(); ++action)
    {
      if (TRACK == action->action)
      {
        if (!debug_info.track_pixel_url.empty())
        {
          urls.push_back(
            UrlAction(
              debug_info.track_pixel_url,
              action->time));
        }
        else
          throw RequestProcessError("track_pixel_url empty");
      }
      else if (CLICK_FIRST == action->action)
      {
        if (!debug_info.selected_creatives.empty())
        {
          urls.push_back(
            UrlAction(
              debug_info.selected_creatives.first().click_url,
              action->time));
        }
        else
          throw RequestProcessError("selected_creatives is empty");
      }
      else
      {
        for (CreativeList::const_iterator ccid = ccids.begin();
             ccid != ccids.end(); ++ccid)
        {
          DebugInfo::SelectedCreativesList::const_iterator creative =
            debug_info.selected_creatives.find(*ccid);
          if (creative != debug_info.selected_creatives.end())
          {
            switch (action->action)
            {
              case CLICK :
              {
                if (!creative->click_url.empty())
                {
                  urls.push_back(
                    UrlAction(
                      creative->click_url,
                      action->time));
                }
                else
                    throw RequestProcessError("ccid#" +
                      *ccid + " click_url empty");
                break;
              }
              case ACTION :
              {
                urls.push_back(
                  UrlAction(
                    AutoTest::ActionRequest().
                    cid(creative->cmp_id).
                    url(),
                    action->time));
                break;
              }
              case NON_EMPTY_ACTION:
              {
                if (!creative->action_adv_url.empty())
                {
                  urls.push_back(
                    UrlAction(
                      creative->action_adv_url,
                      action->time));
                }
                break;
              }

              case WAIT :
              {
                //sleep
                urls.push_back(
                  UrlAction(
                    "",
                    Generics::Time::ZERO == action->time ?
                    Generics::Time(
                      get_global_params().
                      TimeOuts().ad_logs_delivery_timeout())
                    : action->time));
                break;
              }
              default : break;
            }
          }
          else
          {
            throw RequestProcessError("ccid#" + *ccid + " not found");
          }
        }
      }
    }

    for (UrlsList::const_iterator url = urls.begin();
         url != urls.end(); ++url)
    {
      if (url->url.empty())
      {
        Shutdown::instance().wait(url->time.tv_sec);
      }
      else
      {
        // Restore headers
        set_headers_(headers);

        if (Generics::Time::ZERO == url->time)
        {
          process_request(url->url);
        }
        else
        {
          std::string time_str(
            url->time.get_gm_time().format(AutoTest::DEBUG_TIME_FORMAT));
          std::string mime_time_str;
          String::StringManip::mime_url_encode(time_str, mime_time_str);
          if (url->url.find("*amp*") != std::string::npos)
          {
            process_request(url->url + "*amp*debug-time*eql*" + time_str);
          }
          else
          {
            process_request(url->url + "&debug-time=" + mime_time_str);
          }
        }
      }
    }
  }



  void
  AdClient::do_ad_requests(
    const NSLookupRequest& request,
    const CreativeList& ccids,
    const ConsequenceActionList& action_list,
    unsigned long count)
    /*throw(eh::Exception)*/
  {
    for (unsigned long i = 0; i < count; ++i)
    {

      process_request(request);

      do_ad_requests(ccids, action_list, request.headers());

    }
  }

  void AdClient::do_ad_requests(
    const NSLookupRequest& request,
    const CreativeList& ccids,
    unsigned long action_flags,
    unsigned long count)
    /*throw(eh::Exception)*/
  {
    ConsequenceActionList actions;

     for( ConsequenceActionType action = TRACK;
          action <= WAIT; action = ConsequenceActionType( action << 1 ) )
     {
       if (action_flags & action)
         actions.push_back(action);
     }

     do_ad_requests(request, ccids, actions, count);
  }


  void AdClient::checker_wrapper(Checker& checker) /*throw(TestFailed)*/
  {
    try
    {
      checker.check();
    }
    catch (eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << e.what() << ", last request url " << get_stored_url();
      throw TestFailed(ostr);
    }
  }

  // Cookie
  bool AdClient::has_cookie (const std::string& cookie_name)
  {
    std::string cookie_value;
    return get_cookies().find_value(cookie_name, cookie_value) &&
      !cookie_value.empty();
  }

  bool AdClient::has_cookie (
    const std::string& cookie_name,
    const std::string& expected)
  {
    std::string cookie_value;
    if(get_cookies().find_value(cookie_name, cookie_value) &&
       !cookie_value.empty())
    {
      return cookie_value == expected;
    }
    return false;
  }

  bool AdClient::has_host_cookies ()
  {
    AutoTest::Cookie::UnitCookieList
      cookies(
        get_cookies(),
        get_host());
    return !cookies.empty();
  }

  bool AdClient::has_host_cookie (const std::string& cookie_name)
  {
    AutoTest::Cookie::UnitCookieList
      cookies(
        get_cookies(),
        get_host());

    std::string cookie_value;
    return
      cookies.find_value(cookie_name, cookie_value) && !cookie_value.empty();
  }

  bool AdClient::has_host_cookie (
    const std::string& cookie_name,
    const std::string& expected,
    bool present)
  {
    AutoTest::Cookie::UnitCookieList
      cookies(
        get_cookies(),
        get_host());

    std::string cookie_value;
    if(cookies.find_value(cookie_name, cookie_value) && !cookie_value.empty())
    {
      if (present)
      {
        return cookie_value == expected;
      }
      return cookie_value != expected;
    }
    return false;
  }

  bool AdClient::has_domain_cookies ()
  {
    AutoTest::Cookie::UnitCookieList
      cookies(
        get_cookies(),
        get_domain());

    return !cookies.empty();
  }

  bool AdClient::has_domain_cookie (const std::string& cookie_name)
  {
    AutoTest::Cookie::UnitCookieList
      cookies(
        get_cookies(),
        get_domain());

    std::string cookie_value;
    return
      cookies.find_value(cookie_name, cookie_value) && !cookie_value.empty();
  }

  bool AdClient::has_domain_cookie (
    const std::string& cookie_name,
    const std::string& expected,
    bool present)
  {
    AutoTest::Cookie::UnitCookieList
      cookies(
        get_cookies(),
        get_domain());

    std::string cookie_value;
    if(cookies.find_value(cookie_name, cookie_value) && !cookie_value.empty())
    {
      if (present)
      {
        return cookie_value == expected;
      }
      return cookie_value != expected;
    }
    return false;
  }

  void AdClient::get_cookie_value(
    const char *cookie_name,
    std::string& cookie_value) const
    /*throw(CookieNotFound)*/
  {
    bool found;
    try
    {
      found = get_cookies().find_value(cookie_name, cookie_value);
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << "Cookie " << cookie_name << " not found, because " <<
        e.what() << ", last request url " << get_stored_url();
      throw CookieNotFound(ostr);
    }
    if (!found)
    {
      Stream::Error ostr;
      ostr << "Cookie '" << cookie_name
        << "' not found, last request url "
        << get_stored_url();
      throw CookieNotFound(ostr);
    }
  }

  void AdClient::set_cookie_value(
    const char *cookie_name,
    const char *cookie_value,
    bool fail_if_not_exists)
    /*throw(eh::Exception)*/
  {
    if (fail_if_not_exists)
    {
      std::string tmp_value;
      get_cookie_value(cookie_name, tmp_value);
    }
    get_cookies().set_value(cookie_name, cookie_value);
  }

  std::string
  AdClient::get_uid () const
    /*throw(CookieNotFound)*/
  {
    std::string ret;
    get_cookie_value(UID_COOKIE_NAME, ret);
    return ret;
  }

  void
  AdClient::set_uid (
    const std::string& value,
    bool fail_if_not_exists)
    /*throw(eh::Exception)*/
  {
    if (fail_if_not_exists)
    {
      std::string uid_value;
      get_cookie_value(UID_COOKIE_NAME, uid_value);
    }

    std::string domain = get_domain();

    if (domain.empty())
    {
      throw InvalidAdClientConf(
        "Can't set UID cookie for empty domain");
    }
    get_cookies().set_value(UID_COOKIE_NAME, value, domain);
  }


  // class TemporaryAdClient

  TemporaryAdClient TemporaryAdClient::create_user(
    BaseUnit* test,
    unsigned short flags)
    /*throw(eh::Exception)*/
  {
    TemporaryAdClient
      client(
        get_client_address(test, flags).c_str(),
        flags & UF_NULL_LOGGER?
          BaseAdClient::LT_NULL: BaseAdClient::LT_BASE,
        test);
    return client;
  }


  TemporaryAdClient::TemporaryAdClient(
    const char* base_url,
    LoggerType log_type,
    BaseUnit* test)
      /*throw(Exception, eh::Exception)*/ :
    AdClient(base_url, log_type, test),
    tuid_(AutoTest::generate_uid())
  { }

  TemporaryAdClient::TemporaryAdClient(const TemporaryAdClient& client)
      /*throw(Exception, eh::Exception)*/ :
    AdClient(client),
    tuid_(client.tuid_)
  { }

  TemporaryAdClient::~TemporaryAdClient() noexcept
  { }

  void TemporaryAdClient::process_request_(const char* url,
                                          const char* detail_info)
    /*throw(eh::Exception)*/
  {
    if (is_valid_request(url))
    {
      std::string turl(url);
      // add tuid directly
      turl+="&tuid=" + tuid_;
      AdClient::process_request_(turl.c_str(), detail_info);
    }
    else
    {
      Stream::Error ostr;
      ostr << "Temorary client can't send request "
          "with tuid or setuid parametes '" << url <<
          "'";
      throw InvalidRequest(ostr);
    }
  }


  void TemporaryAdClient::repeat_request(const char* detail_info)
    /*throw(eh::Exception)*/
  {
    std::string url = get_stored_url();
    if (!detail_info)
    {
      AdClient::process_request_(url.c_str(), stored_request_info_.c_str());
    }
    else
    {
      AdClient::process_request_(url.c_str(), detail_info);
    }
  }

  const std::string& TemporaryAdClient::get_tuid() const
  {
    return tuid_;
  }

  void TemporaryAdClient::set_uid (
    const std::string&, bool)
    /*throw(eh::Exception)*/
  {
   Stream::Error ostr;
   ostr << "Temorary client can't set cookie " <<
       "(TemporaryAdClient::set_uid is invalid call)" ;
   throw InvalidCall(ostr);
  }

  void TemporaryAdClient::set_cookie_value(
    const char *, const char *, bool)
    /*throw(eh::Exception)*/
  {
    Stream::Error ostr;
    ostr << "Temorary client can't set cookie " <<
        "(TemporaryAdClient::set_cookie_value is invalid call)" ;
    throw InvalidCall(ostr);
  }

  void
  TemporaryAdClient::merge_(
   const TemporaryAdClient&,
   const std::string& )
     /*throw(eh::Exception)*/
  {
    Stream::Error ostr;
    ostr << "Temorary client can't merge with other temporary client" <<
      "(TemporaryAdClient::merge is invalid call)" ;
    throw InvalidCall(ostr);
  }

  bool
  TemporaryAdClient::is_valid_request(const char* url)
  {
    String::RegEx re_setuid(String::SubString("services/nslookup\?.*&setuid=1"));
    String::RegEx re_tuid(String::SubString("&tuid"));
    String::SubString u(url);
    return !(re_setuid.match(u) || re_tuid.match(u));
  }

  AdClient&
  AdClient::operator= (const AdClient& client)
  {
    BaseAdClient::operator=(client);
    request_count_ = client.request_count_;
    client_index_ = client.client_index_;
    return *this;
  }

  Params
  AdClient::get_global_params()
    noexcept
  {
    return unit_->get_global_params();
  }

  Locals
  AdClient::get_local_params()
    noexcept
  {
    return unit_->get_local_params();
  }
}

