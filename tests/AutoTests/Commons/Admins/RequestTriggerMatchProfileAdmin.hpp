
#ifndef __AUTOTESTS_COMMONS_ADMINS_REQUESTTRIGGERMATCHPROFILEADMIN_HPP
#define __AUTOTESTS_COMMONS_ADMINS_REQUESTTRIGGERMATCHPROFILEADMIN_HPP

#include "Admins.hpp"

namespace AutoTest
{
  class RequestTriggerMatchProfileAdmin:  
    public BaseAdminCmd<RequestTriggerMatchProfileAdmin, 6>  
  {  
    public:    
      class Expected      
      {      
        friend class RequestTriggerMatchProfileAdmin;        
        public:        
          Expected() {}          
          Expected& request_id (const std::string& val);          
          Expected& time (const std::string& val);          
          Expected& page_matches (const std::string& val);          
          Expected& search_matches (const std::string& val);          
          Expected& url_matches (const std::string& val);          
          Expected& click_done (const std::string& val);          
                
          enum FieldName          
          {          
            REQUEST_ID = 0,            
            TIME,            
            PAGE_MATCHES,            
            SEARCH_MATCHES,            
            URL_MATCHES,            
            CLICK_DONE            
          };          
        protected:        
          FieldIndexMap values_;          
      };      
        
    public:    
      typedef BaseAdminCmd<RequestTriggerMatchProfileAdmin, FIELDS_COUNT> Base;    
        
      void make_cmd (const char* address,      
        const char* uid      
      )      
      {      
        address_ = address;        
        AdminParams params;        
        if (uid)        
        {        
          params.push_back(AdminParamPair("--user_id=", uid));          
        }        
        make_admin_cmd(*this, "print-request-trigger-match", address, params, static_cast<size_t>(ExpressionMatcher));      
      }      
      RequestTriggerMatchProfileAdmin(      
        const char* address,      
        const char* uid      
      )      
      {      
        make_cmd(address, uid);      
      }      
      RequestTriggerMatchProfileAdmin(      
        const std::string& address,      
        const char* uid      
      )      
      {      
        make_cmd(address.c_str(), uid);      
      }      
      RequestTriggerMatchProfileAdmin(      
        const char* address,      
        const std::string& uid      
      )      
      {      
        make_cmd(address, uid.c_str());      
      }      
      RequestTriggerMatchProfileAdmin(      
        const std::string& address,      
        const std::string& uid      
      )      
      {      
        make_cmd(address.c_str(), uid.c_str());      
      }      
      const char* request_id (unsigned int i = 0);      
      const char* time (unsigned int i = 0);      
      const char* page_matches (unsigned int i = 0);      
      const char* search_matches (unsigned int i = 0);      
      const char* url_matches (unsigned int i = 0);      
      const char* click_done (unsigned int i = 0);      
      bool check(const Expected& expected, bool exist = true)      
        /*throw(eh::Exception)*/;      
      bool check(const std::string& expected, bool exist = true)      
        /*throw(eh::Exception)*/;      
  };  
    
}
namespace AutoTest
{
  inline  
  RequestTriggerMatchProfileAdmin::Expected&  
  RequestTriggerMatchProfileAdmin::Expected::request_id(const std::string& val)  
  {  
     values_.insert(std::make_pair(REQUEST_ID, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  RequestTriggerMatchProfileAdmin::request_id (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::REQUEST_ID].c_str();  
  }  
  inline  
  RequestTriggerMatchProfileAdmin::Expected&  
  RequestTriggerMatchProfileAdmin::Expected::time(const std::string& val)  
  {  
     values_.insert(std::make_pair(TIME, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  RequestTriggerMatchProfileAdmin::time (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::TIME].c_str();  
  }  
  inline  
  RequestTriggerMatchProfileAdmin::Expected&  
  RequestTriggerMatchProfileAdmin::Expected::page_matches(const std::string& val)  
  {  
     values_.insert(std::make_pair(PAGE_MATCHES, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  RequestTriggerMatchProfileAdmin::page_matches (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::PAGE_MATCHES].c_str();  
  }  
  inline  
  RequestTriggerMatchProfileAdmin::Expected&  
  RequestTriggerMatchProfileAdmin::Expected::search_matches(const std::string& val)  
  {  
     values_.insert(std::make_pair(SEARCH_MATCHES, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  RequestTriggerMatchProfileAdmin::search_matches (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::SEARCH_MATCHES].c_str();  
  }  
  inline  
  RequestTriggerMatchProfileAdmin::Expected&  
  RequestTriggerMatchProfileAdmin::Expected::url_matches(const std::string& val)  
  {  
     values_.insert(std::make_pair(URL_MATCHES, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  RequestTriggerMatchProfileAdmin::url_matches (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::URL_MATCHES].c_str();  
  }  
  inline  
  RequestTriggerMatchProfileAdmin::Expected&  
  RequestTriggerMatchProfileAdmin::Expected::click_done(const std::string& val)  
  {  
     values_.insert(std::make_pair(CLICK_DONE, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  RequestTriggerMatchProfileAdmin::click_done (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::CLICK_DONE].c_str();  
  }  
}
#endif  // __AUTOTESTS_COMMONS_ADMINS_REQUESTTRIGGERMATCHPROFILEADMIN_HPP

