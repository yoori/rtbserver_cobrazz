
#ifndef __AUTOTESTS_COMMONS_ADMINS_CHANNELCHECKADMIN_HPP
#define __AUTOTESTS_COMMONS_ADMINS_CHANNELCHECKADMIN_HPP

#include "Admins.hpp"

namespace AutoTest
{
  class ChannelCheckAdmin:  
    public BaseAdminCmd<ChannelCheckAdmin, 2>  
  {  
    static const unsigned long DEFAULT_SKIP = 3;  
    public:    
      class Expected      
      {      
        friend class ChannelCheckAdmin;        
        public:        
          Expected() {}          
          Expected& id (const std::string& val);          
          Expected& version (const std::string& val);          
                
          enum FieldName          
          {          
            ID = 0,            
            VERSION            
          };          
        protected:        
          FieldIndexMap values_;          
      };      
        
    public:    
      typedef BaseAdminCmd<ChannelCheckAdmin, FIELDS_COUNT> Base;    
        
      void make_cmd (const char* address,      
        ChannelSrv service      
      )      
      {      
        address_ = address;        
        AdminParams params;        
        make_admin_cmd(*this, "check", address, params, static_cast<size_t>(service));      
      }      
      ChannelCheckAdmin(      
        const char* address,      
        ChannelSrv service = ChannelManagerController      
      )      
        :Base(DEFAULT_SKIP)      
      {      
        make_cmd(address, service);      
      }      
      ChannelCheckAdmin(      
        const std::string& address,      
        ChannelSrv service = ChannelManagerController      
      )      
        :Base(DEFAULT_SKIP)      
      {      
        make_cmd(address.c_str(), service);      
      }      
      const char* id (unsigned int i = 0);      
      const char* version (unsigned int i = 0);      
      bool check(const Expected& expected, bool exist = true)      
        /*throw(eh::Exception)*/;      
      bool check(const std::string& expected, bool exist = true)      
        /*throw(eh::Exception)*/;      
  };  
    
}
namespace AutoTest
{
  inline  
  ChannelCheckAdmin::Expected&  
  ChannelCheckAdmin::Expected::id(const std::string& val)  
  {  
     values_.insert(std::make_pair(ID, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  ChannelCheckAdmin::id (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::ID].c_str();  
  }  
  inline  
  ChannelCheckAdmin::Expected&  
  ChannelCheckAdmin::Expected::version(const std::string& val)  
  {  
     values_.insert(std::make_pair(VERSION, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  ChannelCheckAdmin::version (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::VERSION].c_str();  
  }  
}
#endif  // __AUTOTESTS_COMMONS_ADMINS_CHANNELCHECKADMIN_HPP

