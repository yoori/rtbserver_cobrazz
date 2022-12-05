
#ifndef __AUTOTESTS_COMMONS_ADMINS_REQUESTKEYWORDPROFILEADMIN_HPP
#define __AUTOTESTS_COMMONS_ADMINS_REQUESTKEYWORDPROFILEADMIN_HPP

#include "Admins.hpp"

namespace AutoTest
{
  class RequestKeywordProfileAdmin:  
    public BaseAdminCmd<RequestKeywordProfileAdmin, 20>  
  {  
    public:    
      class Expected      
      {      
        friend class RequestKeywordProfileAdmin;        
        public:        
          Expected() {}          
          Expected& time (const std::string& val);          
          Expected& colo_id (const std::string& val);          
          Expected& tag_id (const std::string& val);          
          Expected& ccg_id (const std::string& val);          
          Expected& cc_id (const std::string& val);          
          Expected& channel_id (const std::string& val);          
          Expected& position (const std::string& val);          
          Expected& num_shown (const std::string& val);          
          Expected& page_last_match (const std::string& val);          
          Expected& page_hourly_matches (const std::string& val);          
          Expected& page_daily_matches (const std::string& val);          
          Expected& page_weekly_matches (const std::string& val);          
          Expected& search_last_match (const std::string& val);          
          Expected& search_hourly_matches (const std::string& val);          
          Expected& search_daily_matches (const std::string& val);          
          Expected& search_weekly_matches (const std::string& val);          
          Expected& ccg_imps (const std::string& val);          
          Expected& cc_imps (const std::string& val);          
          Expected& channel_imps (const std::string& val);          
          Expected& click_done (const std::string& val);          
                
          enum FieldName          
          {          
            TIME = 0,            
            COLO_ID,            
            TAG_ID,            
            CCG_ID,            
            CC_ID,            
            CHANNEL_ID,            
            POSITION,            
            NUM_SHOWN,            
            PAGE_LAST_MATCH,            
            PAGE_HOURLY_MATCHES,            
            PAGE_DAILY_MATCHES,            
            PAGE_WEEKLY_MATCHES,            
            SEARCH_LAST_MATCH,            
            SEARCH_HOURLY_MATCHES,            
            SEARCH_DAILY_MATCHES,            
            SEARCH_WEEKLY_MATCHES,            
            CCG_IMPS,            
            CC_IMPS,            
            CHANNEL_IMPS,            
            CLICK_DONE            
          };          
        protected:        
          FieldIndexMap values_;          
      };      
        
    public:    
      typedef BaseAdminCmd<RequestKeywordProfileAdmin, FIELDS_COUNT> Base;    
        
      void make_cmd (const char* address,      
        const char* requestid      
      )      
      {      
        address_ = address;        
        AdminParams params;        
        {        
          params.push_back(AdminParamPair("", requestid ? requestid : ""));          
        }        
        make_admin_cmd(*this, "print-request-keyword-match", address, params, static_cast<size_t>(RequestInfoManager));      
        add_cmd_i("--align");      
      }      
      RequestKeywordProfileAdmin(      
        const char* address,      
        const char* requestid      
      )      
      {      
        make_cmd(address, requestid);      
      }      
      RequestKeywordProfileAdmin(      
        const std::string& address,      
        const char* requestid      
      )      
      {      
        make_cmd(address.c_str(), requestid);      
      }      
      RequestKeywordProfileAdmin(      
        const char* address,      
        const std::string& requestid      
      )      
      {      
        make_cmd(address, requestid.c_str());      
      }      
      RequestKeywordProfileAdmin(      
        const std::string& address,      
        const std::string& requestid      
      )      
      {      
        make_cmd(address.c_str(), requestid.c_str());      
      }      
      const char* time (unsigned int i = 0);      
      const char* colo_id (unsigned int i = 0);      
      const char* tag_id (unsigned int i = 0);      
      const char* ccg_id (unsigned int i = 0);      
      const char* cc_id (unsigned int i = 0);      
      const char* channel_id (unsigned int i = 0);      
      const char* position (unsigned int i = 0);      
      const char* num_shown (unsigned int i = 0);      
      const char* page_last_match (unsigned int i = 0);      
      const char* page_hourly_matches (unsigned int i = 0);      
      const char* page_daily_matches (unsigned int i = 0);      
      const char* page_weekly_matches (unsigned int i = 0);      
      const char* search_last_match (unsigned int i = 0);      
      const char* search_hourly_matches (unsigned int i = 0);      
      const char* search_daily_matches (unsigned int i = 0);      
      const char* search_weekly_matches (unsigned int i = 0);      
      const char* ccg_imps (unsigned int i = 0);      
      const char* cc_imps (unsigned int i = 0);      
      const char* channel_imps (unsigned int i = 0);      
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
  RequestKeywordProfileAdmin::Expected&  
  RequestKeywordProfileAdmin::Expected::time(const std::string& val)  
  {  
     values_.insert(std::make_pair(TIME, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  RequestKeywordProfileAdmin::time (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::TIME].c_str();  
  }  
  inline  
  RequestKeywordProfileAdmin::Expected&  
  RequestKeywordProfileAdmin::Expected::colo_id(const std::string& val)  
  {  
     values_.insert(std::make_pair(COLO_ID, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  RequestKeywordProfileAdmin::colo_id (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::COLO_ID].c_str();  
  }  
  inline  
  RequestKeywordProfileAdmin::Expected&  
  RequestKeywordProfileAdmin::Expected::tag_id(const std::string& val)  
  {  
     values_.insert(std::make_pair(TAG_ID, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  RequestKeywordProfileAdmin::tag_id (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::TAG_ID].c_str();  
  }  
  inline  
  RequestKeywordProfileAdmin::Expected&  
  RequestKeywordProfileAdmin::Expected::ccg_id(const std::string& val)  
  {  
     values_.insert(std::make_pair(CCG_ID, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  RequestKeywordProfileAdmin::ccg_id (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::CCG_ID].c_str();  
  }  
  inline  
  RequestKeywordProfileAdmin::Expected&  
  RequestKeywordProfileAdmin::Expected::cc_id(const std::string& val)  
  {  
     values_.insert(std::make_pair(CC_ID, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  RequestKeywordProfileAdmin::cc_id (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::CC_ID].c_str();  
  }  
  inline  
  RequestKeywordProfileAdmin::Expected&  
  RequestKeywordProfileAdmin::Expected::channel_id(const std::string& val)  
  {  
     values_.insert(std::make_pair(CHANNEL_ID, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  RequestKeywordProfileAdmin::channel_id (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::CHANNEL_ID].c_str();  
  }  
  inline  
  RequestKeywordProfileAdmin::Expected&  
  RequestKeywordProfileAdmin::Expected::position(const std::string& val)  
  {  
     values_.insert(std::make_pair(POSITION, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  RequestKeywordProfileAdmin::position (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::POSITION].c_str();  
  }  
  inline  
  RequestKeywordProfileAdmin::Expected&  
  RequestKeywordProfileAdmin::Expected::num_shown(const std::string& val)  
  {  
     values_.insert(std::make_pair(NUM_SHOWN, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  RequestKeywordProfileAdmin::num_shown (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::NUM_SHOWN].c_str();  
  }  
  inline  
  RequestKeywordProfileAdmin::Expected&  
  RequestKeywordProfileAdmin::Expected::page_last_match(const std::string& val)  
  {  
     values_.insert(std::make_pair(PAGE_LAST_MATCH, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  RequestKeywordProfileAdmin::page_last_match (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::PAGE_LAST_MATCH].c_str();  
  }  
  inline  
  RequestKeywordProfileAdmin::Expected&  
  RequestKeywordProfileAdmin::Expected::page_hourly_matches(const std::string& val)  
  {  
     values_.insert(std::make_pair(PAGE_HOURLY_MATCHES, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  RequestKeywordProfileAdmin::page_hourly_matches (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::PAGE_HOURLY_MATCHES].c_str();  
  }  
  inline  
  RequestKeywordProfileAdmin::Expected&  
  RequestKeywordProfileAdmin::Expected::page_daily_matches(const std::string& val)  
  {  
     values_.insert(std::make_pair(PAGE_DAILY_MATCHES, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  RequestKeywordProfileAdmin::page_daily_matches (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::PAGE_DAILY_MATCHES].c_str();  
  }  
  inline  
  RequestKeywordProfileAdmin::Expected&  
  RequestKeywordProfileAdmin::Expected::page_weekly_matches(const std::string& val)  
  {  
     values_.insert(std::make_pair(PAGE_WEEKLY_MATCHES, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  RequestKeywordProfileAdmin::page_weekly_matches (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::PAGE_WEEKLY_MATCHES].c_str();  
  }  
  inline  
  RequestKeywordProfileAdmin::Expected&  
  RequestKeywordProfileAdmin::Expected::search_last_match(const std::string& val)  
  {  
     values_.insert(std::make_pair(SEARCH_LAST_MATCH, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  RequestKeywordProfileAdmin::search_last_match (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::SEARCH_LAST_MATCH].c_str();  
  }  
  inline  
  RequestKeywordProfileAdmin::Expected&  
  RequestKeywordProfileAdmin::Expected::search_hourly_matches(const std::string& val)  
  {  
     values_.insert(std::make_pair(SEARCH_HOURLY_MATCHES, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  RequestKeywordProfileAdmin::search_hourly_matches (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::SEARCH_HOURLY_MATCHES].c_str();  
  }  
  inline  
  RequestKeywordProfileAdmin::Expected&  
  RequestKeywordProfileAdmin::Expected::search_daily_matches(const std::string& val)  
  {  
     values_.insert(std::make_pair(SEARCH_DAILY_MATCHES, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  RequestKeywordProfileAdmin::search_daily_matches (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::SEARCH_DAILY_MATCHES].c_str();  
  }  
  inline  
  RequestKeywordProfileAdmin::Expected&  
  RequestKeywordProfileAdmin::Expected::search_weekly_matches(const std::string& val)  
  {  
     values_.insert(std::make_pair(SEARCH_WEEKLY_MATCHES, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  RequestKeywordProfileAdmin::search_weekly_matches (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::SEARCH_WEEKLY_MATCHES].c_str();  
  }  
  inline  
  RequestKeywordProfileAdmin::Expected&  
  RequestKeywordProfileAdmin::Expected::ccg_imps(const std::string& val)  
  {  
     values_.insert(std::make_pair(CCG_IMPS, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  RequestKeywordProfileAdmin::ccg_imps (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::CCG_IMPS].c_str();  
  }  
  inline  
  RequestKeywordProfileAdmin::Expected&  
  RequestKeywordProfileAdmin::Expected::cc_imps(const std::string& val)  
  {  
     values_.insert(std::make_pair(CC_IMPS, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  RequestKeywordProfileAdmin::cc_imps (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::CC_IMPS].c_str();  
  }  
  inline  
  RequestKeywordProfileAdmin::Expected&  
  RequestKeywordProfileAdmin::Expected::channel_imps(const std::string& val)  
  {  
     values_.insert(std::make_pair(CHANNEL_IMPS, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  RequestKeywordProfileAdmin::channel_imps (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::CHANNEL_IMPS].c_str();  
  }  
  inline  
  RequestKeywordProfileAdmin::Expected&  
  RequestKeywordProfileAdmin::Expected::click_done(const std::string& val)  
  {  
     values_.insert(std::make_pair(CLICK_DONE, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  RequestKeywordProfileAdmin::click_done (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::CLICK_DONE].c_str();  
  }  
}
#endif  // __AUTOTESTS_COMMONS_ADMINS_REQUESTKEYWORDPROFILEADMIN_HPP

