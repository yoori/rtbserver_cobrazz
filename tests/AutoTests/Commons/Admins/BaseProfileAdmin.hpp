
#ifndef __AUTOTESTS_COMMONS_ADMINS_BASEPROFILEADMIN_HPP
#define __AUTOTESTS_COMMONS_ADMINS_BASEPROFILEADMIN_HPP

#include "Admins.hpp"

namespace AutoTest
{
  class BaseProfileAdmin:  
    public BaseAdminCmd<BaseProfileAdmin, 20>  
  {  
    static const unsigned long DEFAULT_SKIP = 1;  
    public:    
      class Expected      
      {      
        friend class BaseProfileAdmin;        
        public:        
          Expected() {}          
          Expected& version (const std::string& val);          
          Expected& create_time (const std::string& val);          
          Expected& history_time (const std::string& val);          
          Expected& ignore_fraud_time (const std::string& val);          
          Expected& last_request_time (const std::string& val);          
          Expected& session_start_time (const std::string& val);          
          Expected& colo_timestamps (const std::string& val);          
          Expected& page_ht_candidates (const std::string& val);          
          Expected& page_history_matches (const std::string& val);          
          Expected& page_history_visits (const std::string& val);          
          Expected& page_session_matches (const std::string& val);          
          Expected& search_ht_candidates (const std::string& val);          
          Expected& search_history_matches (const std::string& val);          
          Expected& search_history_visits (const std::string& val);          
          Expected& search_session_matches (const std::string& val);          
          Expected& url_ht_candidates (const std::string& val);          
          Expected& url_history_matches (const std::string& val);          
          Expected& url_history_visits (const std::string& val);          
          Expected& url_session_matches (const std::string& val);          
          Expected& audience_channels (const std::string& val);          
                
          enum FieldName          
          {          
            VERSION = 0,            
            CREATE_TIME,            
            HISTORY_TIME,            
            IGNORE_FRAUD_TIME,            
            LAST_REQUEST_TIME,            
            SESSION_START_TIME,            
            COLO_TIMESTAMPS,            
            PAGE_HT_CANDIDATES,            
            PAGE_HISTORY_MATCHES,            
            PAGE_HISTORY_VISITS,            
            PAGE_SESSION_MATCHES,            
            SEARCH_HT_CANDIDATES,            
            SEARCH_HISTORY_MATCHES,            
            SEARCH_HISTORY_VISITS,            
            SEARCH_SESSION_MATCHES,            
            URL_HT_CANDIDATES,            
            URL_HISTORY_MATCHES,            
            URL_HISTORY_VISITS,            
            URL_SESSION_MATCHES,            
            AUDIENCE_CHANNELS            
          };          
        protected:        
          FieldIndexMap values_;          
      };      
        
    public:    
      typedef BaseAdminCmd<BaseProfileAdmin, FIELDS_COUNT> Base;    
        
      void make_cmd (const char* address,      
        const char* uid,      
        bool temp,      
        UserInfoSrv service      
      )      
      {      
        address_ = address;        
        AdminParams params;        
        if (uid)        
        {        
          if (temp)          
          {          
            params.push_back(AdminParamPair("--tuid=", uid));            
          }          
          else          
          {          
            params.push_back(AdminParamPair("--uid=", uid));            
          }          
        }        
        make_admin_cmd(*this, "print-base", address, params, static_cast<size_t>(service));      
      }      
      BaseProfileAdmin(      
        const char* address,      
        const char* uid,      
        bool temp = false,      
        UserInfoSrv service = UserInfoManagerController      
      )      
        :Base(DEFAULT_SKIP)      
      {      
        make_cmd(address, uid, temp, service);      
      }      
      BaseProfileAdmin(      
        const std::string& address,      
        const char* uid,      
        bool temp = false,      
        UserInfoSrv service = UserInfoManagerController      
      )      
        :Base(DEFAULT_SKIP)      
      {      
        make_cmd(address.c_str(), uid, temp, service);      
      }      
      BaseProfileAdmin(      
        const char* address,      
        const std::string& uid,      
        bool temp = false,      
        UserInfoSrv service = UserInfoManagerController      
      )      
        :Base(DEFAULT_SKIP)      
      {      
        make_cmd(address, uid.c_str(), temp, service);      
      }      
      BaseProfileAdmin(      
        const std::string& address,      
        const std::string& uid,      
        bool temp = false,      
        UserInfoSrv service = UserInfoManagerController      
      )      
        :Base(DEFAULT_SKIP)      
      {      
        make_cmd(address.c_str(), uid.c_str(), temp, service);      
      }      
      const char* version (unsigned int i = 0);      
      const char* create_time (unsigned int i = 0);      
      const char* history_time (unsigned int i = 0);      
      const char* ignore_fraud_time (unsigned int i = 0);      
      const char* last_request_time (unsigned int i = 0);      
      const char* session_start_time (unsigned int i = 0);      
      const char* colo_timestamps (unsigned int i = 0);      
      const char* page_ht_candidates (unsigned int i = 0);      
      const char* page_history_matches (unsigned int i = 0);      
      const char* page_history_visits (unsigned int i = 0);      
      const char* page_session_matches (unsigned int i = 0);      
      const char* search_ht_candidates (unsigned int i = 0);      
      const char* search_history_matches (unsigned int i = 0);      
      const char* search_history_visits (unsigned int i = 0);      
      const char* search_session_matches (unsigned int i = 0);      
      const char* url_ht_candidates (unsigned int i = 0);      
      const char* url_history_matches (unsigned int i = 0);      
      const char* url_history_visits (unsigned int i = 0);      
      const char* url_session_matches (unsigned int i = 0);      
      const char* audience_channels (unsigned int i = 0);      
      bool check(const Expected& expected, bool exist = true)      
        /*throw(eh::Exception)*/;      
      bool check(const std::string& expected, bool exist = true)      
        /*throw(eh::Exception)*/;      
  };  
    
}
namespace AutoTest
{
  inline  
  BaseProfileAdmin::Expected&  
  BaseProfileAdmin::Expected::version(const std::string& val)  
  {  
     values_.insert(std::make_pair(VERSION, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  BaseProfileAdmin::version (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::VERSION].c_str();  
  }  
  inline  
  BaseProfileAdmin::Expected&  
  BaseProfileAdmin::Expected::create_time(const std::string& val)  
  {  
     values_.insert(std::make_pair(CREATE_TIME, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  BaseProfileAdmin::create_time (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::CREATE_TIME].c_str();  
  }  
  inline  
  BaseProfileAdmin::Expected&  
  BaseProfileAdmin::Expected::history_time(const std::string& val)  
  {  
     values_.insert(std::make_pair(HISTORY_TIME, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  BaseProfileAdmin::history_time (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::HISTORY_TIME].c_str();  
  }  
  inline  
  BaseProfileAdmin::Expected&  
  BaseProfileAdmin::Expected::ignore_fraud_time(const std::string& val)  
  {  
     values_.insert(std::make_pair(IGNORE_FRAUD_TIME, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  BaseProfileAdmin::ignore_fraud_time (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::IGNORE_FRAUD_TIME].c_str();  
  }  
  inline  
  BaseProfileAdmin::Expected&  
  BaseProfileAdmin::Expected::last_request_time(const std::string& val)  
  {  
     values_.insert(std::make_pair(LAST_REQUEST_TIME, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  BaseProfileAdmin::last_request_time (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::LAST_REQUEST_TIME].c_str();  
  }  
  inline  
  BaseProfileAdmin::Expected&  
  BaseProfileAdmin::Expected::session_start_time(const std::string& val)  
  {  
     values_.insert(std::make_pair(SESSION_START_TIME, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  BaseProfileAdmin::session_start_time (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::SESSION_START_TIME].c_str();  
  }  
  inline  
  BaseProfileAdmin::Expected&  
  BaseProfileAdmin::Expected::colo_timestamps(const std::string& val)  
  {  
     values_.insert(std::make_pair(COLO_TIMESTAMPS, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  BaseProfileAdmin::colo_timestamps (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::COLO_TIMESTAMPS].c_str();  
  }  
  inline  
  BaseProfileAdmin::Expected&  
  BaseProfileAdmin::Expected::page_ht_candidates(const std::string& val)  
  {  
     values_.insert(std::make_pair(PAGE_HT_CANDIDATES, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  BaseProfileAdmin::page_ht_candidates (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::PAGE_HT_CANDIDATES].c_str();  
  }  
  inline  
  BaseProfileAdmin::Expected&  
  BaseProfileAdmin::Expected::page_history_matches(const std::string& val)  
  {  
     values_.insert(std::make_pair(PAGE_HISTORY_MATCHES, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  BaseProfileAdmin::page_history_matches (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::PAGE_HISTORY_MATCHES].c_str();  
  }  
  inline  
  BaseProfileAdmin::Expected&  
  BaseProfileAdmin::Expected::page_history_visits(const std::string& val)  
  {  
     values_.insert(std::make_pair(PAGE_HISTORY_VISITS, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  BaseProfileAdmin::page_history_visits (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::PAGE_HISTORY_VISITS].c_str();  
  }  
  inline  
  BaseProfileAdmin::Expected&  
  BaseProfileAdmin::Expected::page_session_matches(const std::string& val)  
  {  
     values_.insert(std::make_pair(PAGE_SESSION_MATCHES, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  BaseProfileAdmin::page_session_matches (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::PAGE_SESSION_MATCHES].c_str();  
  }  
  inline  
  BaseProfileAdmin::Expected&  
  BaseProfileAdmin::Expected::search_ht_candidates(const std::string& val)  
  {  
     values_.insert(std::make_pair(SEARCH_HT_CANDIDATES, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  BaseProfileAdmin::search_ht_candidates (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::SEARCH_HT_CANDIDATES].c_str();  
  }  
  inline  
  BaseProfileAdmin::Expected&  
  BaseProfileAdmin::Expected::search_history_matches(const std::string& val)  
  {  
     values_.insert(std::make_pair(SEARCH_HISTORY_MATCHES, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  BaseProfileAdmin::search_history_matches (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::SEARCH_HISTORY_MATCHES].c_str();  
  }  
  inline  
  BaseProfileAdmin::Expected&  
  BaseProfileAdmin::Expected::search_history_visits(const std::string& val)  
  {  
     values_.insert(std::make_pair(SEARCH_HISTORY_VISITS, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  BaseProfileAdmin::search_history_visits (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::SEARCH_HISTORY_VISITS].c_str();  
  }  
  inline  
  BaseProfileAdmin::Expected&  
  BaseProfileAdmin::Expected::search_session_matches(const std::string& val)  
  {  
     values_.insert(std::make_pair(SEARCH_SESSION_MATCHES, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  BaseProfileAdmin::search_session_matches (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::SEARCH_SESSION_MATCHES].c_str();  
  }  
  inline  
  BaseProfileAdmin::Expected&  
  BaseProfileAdmin::Expected::url_ht_candidates(const std::string& val)  
  {  
     values_.insert(std::make_pair(URL_HT_CANDIDATES, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  BaseProfileAdmin::url_ht_candidates (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::URL_HT_CANDIDATES].c_str();  
  }  
  inline  
  BaseProfileAdmin::Expected&  
  BaseProfileAdmin::Expected::url_history_matches(const std::string& val)  
  {  
     values_.insert(std::make_pair(URL_HISTORY_MATCHES, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  BaseProfileAdmin::url_history_matches (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::URL_HISTORY_MATCHES].c_str();  
  }  
  inline  
  BaseProfileAdmin::Expected&  
  BaseProfileAdmin::Expected::url_history_visits(const std::string& val)  
  {  
     values_.insert(std::make_pair(URL_HISTORY_VISITS, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  BaseProfileAdmin::url_history_visits (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::URL_HISTORY_VISITS].c_str();  
  }  
  inline  
  BaseProfileAdmin::Expected&  
  BaseProfileAdmin::Expected::url_session_matches(const std::string& val)  
  {  
     values_.insert(std::make_pair(URL_SESSION_MATCHES, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  BaseProfileAdmin::url_session_matches (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::URL_SESSION_MATCHES].c_str();  
  }  
  inline  
  BaseProfileAdmin::Expected&  
  BaseProfileAdmin::Expected::audience_channels(const std::string& val)  
  {  
     values_.insert(std::make_pair(AUDIENCE_CHANNELS, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  BaseProfileAdmin::audience_channels (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::AUDIENCE_CHANNELS].c_str();  
  }  
}
#endif  // __AUTOTESTS_COMMONS_ADMINS_BASEPROFILEADMIN_HPP

