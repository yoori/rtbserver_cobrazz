
#ifndef __AUTOTESTS_COMMONS_ADMINS_SITEREACHPROFILEADMIN_HPP
#define __AUTOTESTS_COMMONS_ADMINS_SITEREACHPROFILEADMIN_HPP

#include "Admins.hpp"

namespace AutoTest
{
  class SiteReachProfileAdmin:  
    public BaseAdminCmd<SiteReachProfileAdmin, 3>  
  {  
    public:    
      class Expected      
      {      
        friend class SiteReachProfileAdmin;        
        public:        
          Expected() {}          
          Expected& last_request_day (const std::string& val);          
          Expected& daily_appear_lists (const std::string& val);          
          Expected& monthly_appear_lists (const std::string& val);          
                
          enum FieldName          
          {          
            LAST_REQUEST_DAY = 0,            
            DAILY_APPEAR_LISTS,            
            MONTHLY_APPEAR_LISTS            
          };          
        protected:        
          FieldIndexMap values_;          
      };      
        
    public:    
      typedef BaseAdminCmd<SiteReachProfileAdmin, FIELDS_COUNT> Base;    
        
      void make_cmd (const char* address,      
        const char* uuid,      
        RequestInfoSrv service      
      )      
      {      
        address_ = address;        
        AdminParams params;        
        {        
          params.push_back(AdminParamPair("", uuid ? uuid : ""));          
        }        
        make_admin_cmd(*this, "print-site-reach", address, params, static_cast<size_t>(service));      
        add_cmd_i("--align");      
      }      
      SiteReachProfileAdmin(      
        const char* address,      
        const char* uuid,      
        RequestInfoSrv service = RequestInfoManager      
      )      
      {      
        make_cmd(address, uuid, service);      
      }      
      SiteReachProfileAdmin(      
        const std::string& address,      
        const char* uuid,      
        RequestInfoSrv service = RequestInfoManager      
      )      
      {      
        make_cmd(address.c_str(), uuid, service);      
      }      
      SiteReachProfileAdmin(      
        const char* address,      
        const std::string& uuid,      
        RequestInfoSrv service = RequestInfoManager      
      )      
      {      
        make_cmd(address, uuid.c_str(), service);      
      }      
      SiteReachProfileAdmin(      
        const std::string& address,      
        const std::string& uuid,      
        RequestInfoSrv service = RequestInfoManager      
      )      
      {      
        make_cmd(address.c_str(), uuid.c_str(), service);      
      }      
      const char* last_request_day (unsigned int i = 0);      
      const char* daily_appear_lists (unsigned int i = 0);      
      const char* monthly_appear_lists (unsigned int i = 0);      
      bool check(const Expected& expected, bool exist = true)      
        /*throw(eh::Exception)*/;      
      bool check(const std::string& expected, bool exist = true)      
        /*throw(eh::Exception)*/;      
  };  
    
}
namespace AutoTest
{
  inline  
  SiteReachProfileAdmin::Expected&  
  SiteReachProfileAdmin::Expected::last_request_day(const std::string& val)  
  {  
     values_.insert(std::make_pair(LAST_REQUEST_DAY, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  SiteReachProfileAdmin::last_request_day (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::LAST_REQUEST_DAY].c_str();  
  }  
  inline  
  SiteReachProfileAdmin::Expected&  
  SiteReachProfileAdmin::Expected::daily_appear_lists(const std::string& val)  
  {  
     values_.insert(std::make_pair(DAILY_APPEAR_LISTS, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  SiteReachProfileAdmin::daily_appear_lists (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::DAILY_APPEAR_LISTS].c_str();  
  }  
  inline  
  SiteReachProfileAdmin::Expected&  
  SiteReachProfileAdmin::Expected::monthly_appear_lists(const std::string& val)  
  {  
     values_.insert(std::make_pair(MONTHLY_APPEAR_LISTS, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  SiteReachProfileAdmin::monthly_appear_lists (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::MONTHLY_APPEAR_LISTS].c_str();  
  }  
}
#endif  // __AUTOTESTS_COMMONS_ADMINS_SITEREACHPROFILEADMIN_HPP

