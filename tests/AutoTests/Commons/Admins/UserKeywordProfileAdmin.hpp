
#ifndef __AUTOTESTS_COMMONS_ADMINS_USERKEYWORDPROFILEADMIN_HPP
#define __AUTOTESTS_COMMONS_ADMINS_USERKEYWORDPROFILEADMIN_HPP

#include "Admins.hpp"

namespace AutoTest
{
  class UserKeywordProfileAdmin:  
    public BaseAdminCmd<UserKeywordProfileAdmin, 6>  
  {  
    public:    
      class Expected      
      {      
        friend class UserKeywordProfileAdmin;        
        public:        
          Expected() {}          
          Expected& page_matches (const std::string& val);          
          Expected& search_matches (const std::string& val);          
          Expected& ccg_imps (const std::string& val);          
          Expected& cc_imps (const std::string& val);          
          Expected& channel_imps (const std::string& val);          
          Expected& requests (const std::string& val);          
                
          enum FieldName          
          {          
            PAGE_MATCHES = 0,            
            SEARCH_MATCHES,            
            CCG_IMPS,            
            CC_IMPS,            
            CHANNEL_IMPS,            
            REQUESTS            
          };          
        protected:        
          FieldIndexMap values_;          
      };      
        
    public:    
      typedef BaseAdminCmd<UserKeywordProfileAdmin, FIELDS_COUNT> Base;    
        
      void make_cmd (const char* address,      
        const char* uuid      
      )      
      {      
        address_ = address;        
        AdminParams params;        
        {        
          params.push_back(AdminParamPair("", uuid ? uuid : ""));          
        }        
        make_admin_cmd(*this, "print-user-keyword-match", address, params, static_cast<size_t>(RequestInfoManager));      
        add_cmd_i("--align");      
      }      
      UserKeywordProfileAdmin(      
        const char* address,      
        const char* uuid      
      )      
      {      
        make_cmd(address, uuid);      
      }      
      UserKeywordProfileAdmin(      
        const std::string& address,      
        const char* uuid      
      )      
      {      
        make_cmd(address.c_str(), uuid);      
      }      
      UserKeywordProfileAdmin(      
        const char* address,      
        const std::string& uuid      
      )      
      {      
        make_cmd(address, uuid.c_str());      
      }      
      UserKeywordProfileAdmin(      
        const std::string& address,      
        const std::string& uuid      
      )      
      {      
        make_cmd(address.c_str(), uuid.c_str());      
      }      
      const char* page_matches (unsigned int i = 0);      
      const char* search_matches (unsigned int i = 0);      
      const char* ccg_imps (unsigned int i = 0);      
      const char* cc_imps (unsigned int i = 0);      
      const char* channel_imps (unsigned int i = 0);      
      const char* requests (unsigned int i = 0);      
      bool check(const Expected& expected, bool exist = true)      
        /*throw(eh::Exception)*/;      
      bool check(const std::string& expected, bool exist = true)      
        /*throw(eh::Exception)*/;      
  };  
    
}
namespace AutoTest
{
  inline  
  UserKeywordProfileAdmin::Expected&  
  UserKeywordProfileAdmin::Expected::page_matches(const std::string& val)  
  {  
     values_.insert(std::make_pair(PAGE_MATCHES, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  UserKeywordProfileAdmin::page_matches (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::PAGE_MATCHES].c_str();  
  }  
  inline  
  UserKeywordProfileAdmin::Expected&  
  UserKeywordProfileAdmin::Expected::search_matches(const std::string& val)  
  {  
     values_.insert(std::make_pair(SEARCH_MATCHES, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  UserKeywordProfileAdmin::search_matches (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::SEARCH_MATCHES].c_str();  
  }  
  inline  
  UserKeywordProfileAdmin::Expected&  
  UserKeywordProfileAdmin::Expected::ccg_imps(const std::string& val)  
  {  
     values_.insert(std::make_pair(CCG_IMPS, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  UserKeywordProfileAdmin::ccg_imps (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::CCG_IMPS].c_str();  
  }  
  inline  
  UserKeywordProfileAdmin::Expected&  
  UserKeywordProfileAdmin::Expected::cc_imps(const std::string& val)  
  {  
     values_.insert(std::make_pair(CC_IMPS, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  UserKeywordProfileAdmin::cc_imps (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::CC_IMPS].c_str();  
  }  
  inline  
  UserKeywordProfileAdmin::Expected&  
  UserKeywordProfileAdmin::Expected::channel_imps(const std::string& val)  
  {  
     values_.insert(std::make_pair(CHANNEL_IMPS, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  UserKeywordProfileAdmin::channel_imps (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::CHANNEL_IMPS].c_str();  
  }  
  inline  
  UserKeywordProfileAdmin::Expected&  
  UserKeywordProfileAdmin::Expected::requests(const std::string& val)  
  {  
     values_.insert(std::make_pair(REQUESTS, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  UserKeywordProfileAdmin::requests (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::REQUESTS].c_str();  
  }  
}
#endif  // __AUTOTESTS_COMMONS_ADMINS_USERKEYWORDPROFILEADMIN_HPP

