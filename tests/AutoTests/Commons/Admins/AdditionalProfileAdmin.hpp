
#ifndef __AUTOTESTS_COMMONS_ADMINS_ADDITIONALPROFILEADMIN_HPP
#define __AUTOTESTS_COMMONS_ADMINS_ADDITIONALPROFILEADMIN_HPP

#include "Admins.hpp"

namespace AutoTest
{
  class AdditionalProfileAdmin:  
    public BaseAdminCmd<AdditionalProfileAdmin, 19>  
  {  
    static const unsigned long DEFAULT_SKIP = 1;  
    public:    
      class Expected      
      {      
        friend class AdditionalProfileAdmin;        
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
            URL_SESSION_MATCHES            
          };          
        protected:        
          FieldIndexMap values_;          
      };      
        
    public:    
      typedef BaseAdminCmd<AdditionalProfileAdmin, FIELDS_COUNT> Base;    
        
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
        make_admin_cmd(*this, "print-add", address, params, static_cast<size_t>(service));      
      }      
      AdditionalProfileAdmin(      
        const char* address,      
        const char* uid,      
        bool temp = false,      
        UserInfoSrv service = UserInfoManagerController      
      )      
        :Base(DEFAULT_SKIP)      
      {      
        make_cmd(address, uid, temp, service);      
      }      
      AdditionalProfileAdmin(      
        const std::string& address,      
        const char* uid,      
        bool temp = false,      
        UserInfoSrv service = UserInfoManagerController      
      )      
        :Base(DEFAULT_SKIP)      
      {      
        make_cmd(address.c_str(), uid, temp, service);      
      }      
      AdditionalProfileAdmin(      
        const char* address,      
        const std::string& uid,      
        bool temp = false,      
        UserInfoSrv service = UserInfoManagerController      
      )      
        :Base(DEFAULT_SKIP)      
      {      
        make_cmd(address, uid.c_str(), temp, service);      
      }      
      AdditionalProfileAdmin(      
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
      bool check(const Expected& expected, bool exist = true)      
        /*throw(eh::Exception)*/;      
      bool check(const std::string& expected, bool exist = true)      
        /*throw(eh::Exception)*/;      
  };  
    
}
namespace AutoTest
{
  inline  
  AdditionalProfileAdmin::Expected&  
  AdditionalProfileAdmin::Expected::version(const std::string& val)  
  {  
     values_.insert(std::make_pair(VERSION, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  AdditionalProfileAdmin::version (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::VERSION].c_str();  
  }  
  inline  
  AdditionalProfileAdmin::Expected&  
  AdditionalProfileAdmin::Expected::create_time(const std::string& val)  
  {  
     values_.insert(std::make_pair(CREATE_TIME, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  AdditionalProfileAdmin::create_time (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::CREATE_TIME].c_str();  
  }  
  inline  
  AdditionalProfileAdmin::Expected&  
  AdditionalProfileAdmin::Expected::history_time(const std::string& val)  
  {  
     values_.insert(std::make_pair(HISTORY_TIME, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  AdditionalProfileAdmin::history_time (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::HISTORY_TIME].c_str();  
  }  
  inline  
  AdditionalProfileAdmin::Expected&  
  AdditionalProfileAdmin::Expected::ignore_fraud_time(const std::string& val)  
  {  
     values_.insert(std::make_pair(IGNORE_FRAUD_TIME, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  AdditionalProfileAdmin::ignore_fraud_time (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::IGNORE_FRAUD_TIME].c_str();  
  }  
  inline  
  AdditionalProfileAdmin::Expected&  
  AdditionalProfileAdmin::Expected::last_request_time(const std::string& val)  
  {  
     values_.insert(std::make_pair(LAST_REQUEST_TIME, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  AdditionalProfileAdmin::last_request_time (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::LAST_REQUEST_TIME].c_str();  
  }  
  inline  
  AdditionalProfileAdmin::Expected&  
  AdditionalProfileAdmin::Expected::session_start_time(const std::string& val)  
  {  
     values_.insert(std::make_pair(SESSION_START_TIME, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  AdditionalProfileAdmin::session_start_time (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::SESSION_START_TIME].c_str();  
  }  
  inline  
  AdditionalProfileAdmin::Expected&  
  AdditionalProfileAdmin::Expected::colo_timestamps(const std::string& val)  
  {  
     values_.insert(std::make_pair(COLO_TIMESTAMPS, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  AdditionalProfileAdmin::colo_timestamps (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::COLO_TIMESTAMPS].c_str();  
  }  
  inline  
  AdditionalProfileAdmin::Expected&  
  AdditionalProfileAdmin::Expected::page_ht_candidates(const std::string& val)  
  {  
     values_.insert(std::make_pair(PAGE_HT_CANDIDATES, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  AdditionalProfileAdmin::page_ht_candidates (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::PAGE_HT_CANDIDATES].c_str();  
  }  
  inline  
  AdditionalProfileAdmin::Expected&  
  AdditionalProfileAdmin::Expected::page_history_matches(const std::string& val)  
  {  
     values_.insert(std::make_pair(PAGE_HISTORY_MATCHES, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  AdditionalProfileAdmin::page_history_matches (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::PAGE_HISTORY_MATCHES].c_str();  
  }  
  inline  
  AdditionalProfileAdmin::Expected&  
  AdditionalProfileAdmin::Expected::page_history_visits(const std::string& val)  
  {  
     values_.insert(std::make_pair(PAGE_HISTORY_VISITS, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  AdditionalProfileAdmin::page_history_visits (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::PAGE_HISTORY_VISITS].c_str();  
  }  
  inline  
  AdditionalProfileAdmin::Expected&  
  AdditionalProfileAdmin::Expected::page_session_matches(const std::string& val)  
  {  
     values_.insert(std::make_pair(PAGE_SESSION_MATCHES, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  AdditionalProfileAdmin::page_session_matches (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::PAGE_SESSION_MATCHES].c_str();  
  }  
  inline  
  AdditionalProfileAdmin::Expected&  
  AdditionalProfileAdmin::Expected::search_ht_candidates(const std::string& val)  
  {  
     values_.insert(std::make_pair(SEARCH_HT_CANDIDATES, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  AdditionalProfileAdmin::search_ht_candidates (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::SEARCH_HT_CANDIDATES].c_str();  
  }  
  inline  
  AdditionalProfileAdmin::Expected&  
  AdditionalProfileAdmin::Expected::search_history_matches(const std::string& val)  
  {  
     values_.insert(std::make_pair(SEARCH_HISTORY_MATCHES, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  AdditionalProfileAdmin::search_history_matches (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::SEARCH_HISTORY_MATCHES].c_str();  
  }  
  inline  
  AdditionalProfileAdmin::Expected&  
  AdditionalProfileAdmin::Expected::search_history_visits(const std::string& val)  
  {  
     values_.insert(std::make_pair(SEARCH_HISTORY_VISITS, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  AdditionalProfileAdmin::search_history_visits (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::SEARCH_HISTORY_VISITS].c_str();  
  }  
  inline  
  AdditionalProfileAdmin::Expected&  
  AdditionalProfileAdmin::Expected::search_session_matches(const std::string& val)  
  {  
     values_.insert(std::make_pair(SEARCH_SESSION_MATCHES, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  AdditionalProfileAdmin::search_session_matches (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::SEARCH_SESSION_MATCHES].c_str();  
  }  
  inline  
  AdditionalProfileAdmin::Expected&  
  AdditionalProfileAdmin::Expected::url_ht_candidates(const std::string& val)  
  {  
     values_.insert(std::make_pair(URL_HT_CANDIDATES, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  AdditionalProfileAdmin::url_ht_candidates (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::URL_HT_CANDIDATES].c_str();  
  }  
  inline  
  AdditionalProfileAdmin::Expected&  
  AdditionalProfileAdmin::Expected::url_history_matches(const std::string& val)  
  {  
     values_.insert(std::make_pair(URL_HISTORY_MATCHES, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  AdditionalProfileAdmin::url_history_matches (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::URL_HISTORY_MATCHES].c_str();  
  }  
  inline  
  AdditionalProfileAdmin::Expected&  
  AdditionalProfileAdmin::Expected::url_history_visits(const std::string& val)  
  {  
     values_.insert(std::make_pair(URL_HISTORY_VISITS, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  AdditionalProfileAdmin::url_history_visits (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::URL_HISTORY_VISITS].c_str();  
  }  
  inline  
  AdditionalProfileAdmin::Expected&  
  AdditionalProfileAdmin::Expected::url_session_matches(const std::string& val)  
  {  
     values_.insert(std::make_pair(URL_SESSION_MATCHES, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  AdditionalProfileAdmin::url_session_matches (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::URL_SESSION_MATCHES].c_str();  
  }  
}
#endif  // __AUTOTESTS_COMMONS_ADMINS_ADDITIONALPROFILEADMIN_HPP

