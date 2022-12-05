
#ifndef __AUTOTESTS_COMMONS_ADMINS_ACTIONADMIN_HPP
#define __AUTOTESTS_COMMONS_ADMINS_ACTIONADMIN_HPP

#include "Admins.hpp"

namespace AutoTest
{
  class ActionAdmin:  
    public BaseAdminCmd<ActionAdmin, 3>  
  {  
    public:    
      class Expected      
      {      
        friend class ActionAdmin;        
        public:        
          Expected() {}          
          Expected& action_id (const std::string& val);          
          Expected& action_id(unsigned long val);          
          Expected& timestamp (const std::string& val);          
          Expected& ccg_ids (const std::string& val);          
                
          enum FieldName          
          {          
            ACTION_ID = 0,            
            TIMESTAMP,            
            CCG_IDS            
          };          
        protected:        
          FieldIndexMap values_;          
      };      
        
    public:    
      typedef BaseAdminCmd<ActionAdmin, FIELDS_COUNT> Base;    
        
      void make_cmd (const char* address,      
        const char* action      
      )      
      {      
        address_ = address;        
        AdminParams params;        
        if (action)        
        {        
          params.push_back(AdminParamPair("action_id=", action));          
        }        
        make_admin_cmd(*this, "adv_action", address, params, static_cast<size_t>(CampaignManager));      
      }      
      ActionAdmin(      
        const char* address,      
        const char* action      
      )      
      {      
        make_cmd(address, action);      
      }      
      ActionAdmin(      
        const std::string& address,      
        const char* action      
      )      
      {      
        make_cmd(address.c_str(), action);      
      }      
      ActionAdmin(      
        const char* address,      
        const std::string& action      
      )      
      {      
        make_cmd(address, action.c_str());      
      }      
      ActionAdmin(      
        const std::string& address,      
        const std::string& action      
      )      
      {      
        make_cmd(address.c_str(), action.c_str());      
      }      
      ActionAdmin(      
        const char* address,      
        unsigned long action      
      )      
      {      
        make_cmd(address, strof(action).c_str());      
      }      
      ActionAdmin(      
        const std::string& address,      
        unsigned long action      
      )      
      {      
        make_cmd(address.c_str(), strof(action).c_str());      
      }      
      const char* action_id (unsigned int i = 0);      
      const char* timestamp (unsigned int i = 0);      
      const char* ccg_ids (unsigned int i = 0);      
      bool check(const Expected& expected, bool exist = true)      
        /*throw(eh::Exception)*/;      
      bool check(const std::string& expected, bool exist = true)      
        /*throw(eh::Exception)*/;      
  };  
    
}
namespace AutoTest
{
  inline  
  ActionAdmin::Expected&  
  ActionAdmin::Expected::action_id(const std::string& val)  
  {  
     values_.insert(std::make_pair(ACTION_ID, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  ActionAdmin::action_id (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::ACTION_ID].c_str();  
  }  
  inline  
  ActionAdmin::Expected&  
  ActionAdmin::Expected::action_id(unsigned long val)  
  {  
     values_.insert(std::make_pair(ACTION_ID, ComparableRegExp(strof(val))));  
     return *this;  
  }  
  inline  
  ActionAdmin::Expected&  
  ActionAdmin::Expected::timestamp(const std::string& val)  
  {  
     values_.insert(std::make_pair(TIMESTAMP, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  ActionAdmin::timestamp (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::TIMESTAMP].c_str();  
  }  
  inline  
  ActionAdmin::Expected&  
  ActionAdmin::Expected::ccg_ids(const std::string& val)  
  {  
     values_.insert(std::make_pair(CCG_IDS, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  ActionAdmin::ccg_ids (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::CCG_IDS].c_str();  
  }  
}
#endif  // __AUTOTESTS_COMMONS_ADMINS_ACTIONADMIN_HPP

