
#ifndef __AUTOTESTS_COMMONS_ADMINS_INVENTORYPROFILEADMIN_HPP
#define __AUTOTESTS_COMMONS_ADMINS_INVENTORYPROFILEADMIN_HPP

#include "Admins.hpp"

namespace AutoTest
{
  class InventoryProfileAdmin:  
    public BaseAdminCmd<InventoryProfileAdmin, 4>  
  {  
    public:    
      class Expected      
      {      
        friend class InventoryProfileAdmin;        
        public:        
          Expected() {}          
          Expected& user_id (const std::string& val);          
          Expected& imp_count (const std::string& val);          
          Expected& imp_count(unsigned long val);          
          Expected& total_channels (const std::string& val);          
          Expected& total_channels (const ComparableStringList& val);          
          Expected& last_request_time (const std::string& val);          
                
          enum FieldName          
          {          
            USER_ID = 0,            
            IMP_COUNT,            
            TOTAL_CHANNELS,            
            LAST_REQUEST_TIME            
          };          
        protected:        
          FieldIndexMap values_;          
      };      
        
    public:    
      typedef BaseAdminCmd<InventoryProfileAdmin, FIELDS_COUNT> Base;    
        
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
        make_admin_cmd(*this, "print", address, params, static_cast<size_t>(ExpressionMatcher));      
      }      
      InventoryProfileAdmin(      
        const char* address,      
        const char* uid      
      )      
      {      
        make_cmd(address, uid);      
      }      
      InventoryProfileAdmin(      
        const std::string& address,      
        const char* uid      
      )      
      {      
        make_cmd(address.c_str(), uid);      
      }      
      InventoryProfileAdmin(      
        const char* address,      
        const std::string& uid      
      )      
      {      
        make_cmd(address, uid.c_str());      
      }      
      InventoryProfileAdmin(      
        const std::string& address,      
        const std::string& uid      
      )      
      {      
        make_cmd(address.c_str(), uid.c_str());      
      }      
      const char* user_id (unsigned int i = 0);      
      const char* imp_count (unsigned int i = 0);      
      const char* total_channels (unsigned int i = 0);      
      const char* last_request_time (unsigned int i = 0);      
      bool check(const Expected& expected, bool exist = true)      
        /*throw(eh::Exception)*/;      
      bool check(const std::string& expected, bool exist = true)      
        /*throw(eh::Exception)*/;      
  };  
    
}
namespace AutoTest
{
  inline  
  InventoryProfileAdmin::Expected&  
  InventoryProfileAdmin::Expected::user_id(const std::string& val)  
  {  
     values_.insert(std::make_pair(USER_ID, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  InventoryProfileAdmin::user_id (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::USER_ID].c_str();  
  }  
  inline  
  InventoryProfileAdmin::Expected&  
  InventoryProfileAdmin::Expected::imp_count(const std::string& val)  
  {  
     values_.insert(std::make_pair(IMP_COUNT, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  InventoryProfileAdmin::imp_count (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::IMP_COUNT].c_str();  
  }  
  inline  
  InventoryProfileAdmin::Expected&  
  InventoryProfileAdmin::Expected::imp_count(unsigned long val)  
  {  
     values_.insert(std::make_pair(IMP_COUNT, ComparableRegExp(strof(val))));  
     return *this;  
  }  
  inline  
  InventoryProfileAdmin::Expected&  
  InventoryProfileAdmin::Expected::total_channels(const std::string& val)  
  {  
     values_.insert(std::make_pair(TOTAL_CHANNELS, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  InventoryProfileAdmin::total_channels (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::TOTAL_CHANNELS].c_str();  
  }  
  inline  
  InventoryProfileAdmin::Expected&  
  InventoryProfileAdmin::Expected::total_channels (const ComparableStringList& val)  
  {  
     values_.insert(std::make_pair(TOTAL_CHANNELS, val));  
     return *this;  
  }  
  inline  
  InventoryProfileAdmin::Expected&  
  InventoryProfileAdmin::Expected::last_request_time(const std::string& val)  
  {  
     values_.insert(std::make_pair(LAST_REQUEST_TIME, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  InventoryProfileAdmin::last_request_time (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::LAST_REQUEST_TIME].c_str();  
  }  
}
#endif  // __AUTOTESTS_COMMONS_ADMINS_INVENTORYPROFILEADMIN_HPP

