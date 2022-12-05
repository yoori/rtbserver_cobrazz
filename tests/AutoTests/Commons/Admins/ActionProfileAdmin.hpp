
#ifndef __AUTOTESTS_COMMONS_ADMINS_ACTIONPROFILEADMIN_HPP
#define __AUTOTESTS_COMMONS_ADMINS_ACTIONPROFILEADMIN_HPP

#include "Admins.hpp"

namespace AutoTest
{
  class ActionProfileAdmin:  
    public BaseAdminCmd<ActionProfileAdmin, 5>  
  {  
    public:    
      class Expected      
      {      
        friend class ActionProfileAdmin;        
        public:        
          Expected() {}          
          Expected& action_markers (const std::string& val);          
          Expected& wait_markers (const std::string& val);          
          Expected& custom_action_markers (const std::string& val);          
          Expected& custom_wait_actions (const std::string& val);          
          Expected& custom_done_actions (const std::string& val);          
                
          enum FieldName          
          {          
            ACTION_MARKERS = 0,            
            WAIT_MARKERS,            
            CUSTOM_ACTION_MARKERS,            
            CUSTOM_WAIT_ACTIONS,            
            CUSTOM_DONE_ACTIONS            
          };          
        protected:        
          FieldIndexMap values_;          
      };      
        
    public:    
      typedef BaseAdminCmd<ActionProfileAdmin, FIELDS_COUNT> Base;    
        
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
        make_admin_cmd(*this, "print-action", address, params, static_cast<size_t>(service));      
        add_cmd_i("--align");      
      }      
      ActionProfileAdmin(      
        const char* address,      
        const char* uuid,      
        RequestInfoSrv service = RequestInfoManager      
      )      
      {      
        make_cmd(address, uuid, service);      
      }      
      ActionProfileAdmin(      
        const std::string& address,      
        const char* uuid,      
        RequestInfoSrv service = RequestInfoManager      
      )      
      {      
        make_cmd(address.c_str(), uuid, service);      
      }      
      ActionProfileAdmin(      
        const char* address,      
        const std::string& uuid,      
        RequestInfoSrv service = RequestInfoManager      
      )      
      {      
        make_cmd(address, uuid.c_str(), service);      
      }      
      ActionProfileAdmin(      
        const std::string& address,      
        const std::string& uuid,      
        RequestInfoSrv service = RequestInfoManager      
      )      
      {      
        make_cmd(address.c_str(), uuid.c_str(), service);      
      }      
      const char* action_markers (unsigned int i = 0);      
      const char* wait_markers (unsigned int i = 0);      
      const char* custom_action_markers (unsigned int i = 0);      
      const char* custom_wait_actions (unsigned int i = 0);      
      const char* custom_done_actions (unsigned int i = 0);      
      bool check(const Expected& expected, bool exist = true)      
        /*throw(eh::Exception)*/;      
      bool check(const std::string& expected, bool exist = true)      
        /*throw(eh::Exception)*/;      
  };  
    
}
namespace AutoTest
{
  inline  
  ActionProfileAdmin::Expected&  
  ActionProfileAdmin::Expected::action_markers(const std::string& val)  
  {  
     values_.insert(std::make_pair(ACTION_MARKERS, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  ActionProfileAdmin::action_markers (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::ACTION_MARKERS].c_str();  
  }  
  inline  
  ActionProfileAdmin::Expected&  
  ActionProfileAdmin::Expected::wait_markers(const std::string& val)  
  {  
     values_.insert(std::make_pair(WAIT_MARKERS, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  ActionProfileAdmin::wait_markers (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::WAIT_MARKERS].c_str();  
  }  
  inline  
  ActionProfileAdmin::Expected&  
  ActionProfileAdmin::Expected::custom_action_markers(const std::string& val)  
  {  
     values_.insert(std::make_pair(CUSTOM_ACTION_MARKERS, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  ActionProfileAdmin::custom_action_markers (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::CUSTOM_ACTION_MARKERS].c_str();  
  }  
  inline  
  ActionProfileAdmin::Expected&  
  ActionProfileAdmin::Expected::custom_wait_actions(const std::string& val)  
  {  
     values_.insert(std::make_pair(CUSTOM_WAIT_ACTIONS, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  ActionProfileAdmin::custom_wait_actions (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::CUSTOM_WAIT_ACTIONS].c_str();  
  }  
  inline  
  ActionProfileAdmin::Expected&  
  ActionProfileAdmin::Expected::custom_done_actions(const std::string& val)  
  {  
     values_.insert(std::make_pair(CUSTOM_DONE_ACTIONS, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  ActionProfileAdmin::custom_done_actions (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::CUSTOM_DONE_ACTIONS].c_str();  
  }  
}
#endif  // __AUTOTESTS_COMMONS_ADMINS_ACTIONPROFILEADMIN_HPP

