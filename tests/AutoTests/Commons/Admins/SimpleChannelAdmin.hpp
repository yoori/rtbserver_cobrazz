
#ifndef __AUTOTESTS_COMMONS_ADMINS_SIMPLECHANNELADMIN_HPP
#define __AUTOTESTS_COMMONS_ADMINS_SIMPLECHANNELADMIN_HPP

#include "Admins.hpp"

namespace AutoTest
{
  class SimpleChannelAdmin:  
    public BaseAdminCmd<SimpleChannelAdmin, 7>  
  {  
    public:    
      class Expected      
      {      
        friend class SimpleChannelAdmin;        
        public:        
          Expected() {}          
          Expected& channel_id (const std::string& val);          
          Expected& channel_id(unsigned long val);          
          Expected& country_code (const std::string& val);          
          Expected& status (const std::string& val);          
          bool has_status () const;          
          std::string status () const;          
          Expected& behav_param_list_id (const std::string& val);          
          Expected& categories (const std::string& val);          
          Expected& threshold (const std::string& val);          
          Expected& timestamp (const std::string& val);          
                
          enum FieldName          
          {          
            CHANNEL_ID = 0,            
            COUNTRY_CODE,            
            STATUS,            
            BEHAV_PARAM_LIST_ID,            
            CATEGORIES,            
            THRESHOLD,            
            TIMESTAMP            
          };          
        protected:        
          FieldIndexMap values_;          
      };      
        
    public:    
      typedef BaseAdminCmd<SimpleChannelAdmin, FIELDS_COUNT> Base;    
        
      void make_cmd (const char* address,      
        const char* id      
      )      
      {      
        address_ = address;        
        AdminParams params;        
        if (id)        
        {        
          params.push_back(AdminParamPair("channel_id=", id));          
        }        
        make_admin_cmd(*this, "simple_channel", address, params, static_cast<size_t>(CampaignServer));      
      }      
      SimpleChannelAdmin(      
        const char* address,      
        const char* id      
      )      
      {      
        make_cmd(address, id);      
      }      
      SimpleChannelAdmin(      
        const std::string& address,      
        const char* id      
      )      
      {      
        make_cmd(address.c_str(), id);      
      }      
      SimpleChannelAdmin(      
        const char* address,      
        const std::string& id      
      )      
      {      
        make_cmd(address, id.c_str());      
      }      
      SimpleChannelAdmin(      
        const std::string& address,      
        const std::string& id      
      )      
      {      
        make_cmd(address.c_str(), id.c_str());      
      }      
      SimpleChannelAdmin(      
        const char* address,      
        unsigned long id      
      )      
      {      
        make_cmd(address, strof(id).c_str());      
      }      
      SimpleChannelAdmin(      
        const std::string& address,      
        unsigned long id      
      )      
      {      
        make_cmd(address.c_str(), strof(id).c_str());      
      }      
      const char* channel_id (unsigned int i = 0);      
      const char* country_code (unsigned int i = 0);      
      const char* status (unsigned int i = 0);      
      const char* behav_param_list_id (unsigned int i = 0);      
      const char* categories (unsigned int i = 0);      
      const char* threshold (unsigned int i = 0);      
      const char* timestamp (unsigned int i = 0);      
      bool check(const Expected& expected, bool exist = true)      
        /*throw(eh::Exception)*/;      
      bool check(const std::string& expected, bool exist = true)      
        /*throw(eh::Exception)*/;      
  };  
    
}
namespace AutoTest
{
  inline  
  SimpleChannelAdmin::Expected&  
  SimpleChannelAdmin::Expected::channel_id(const std::string& val)  
  {  
     values_.insert(std::make_pair(CHANNEL_ID, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  SimpleChannelAdmin::channel_id (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::CHANNEL_ID].c_str();  
  }  
  inline  
  SimpleChannelAdmin::Expected&  
  SimpleChannelAdmin::Expected::channel_id(unsigned long val)  
  {  
     values_.insert(std::make_pair(CHANNEL_ID, ComparableRegExp(strof(val))));  
     return *this;  
  }  
  inline  
  SimpleChannelAdmin::Expected&  
  SimpleChannelAdmin::Expected::country_code(const std::string& val)  
  {  
     values_.insert(std::make_pair(COUNTRY_CODE, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  SimpleChannelAdmin::country_code (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::COUNTRY_CODE].c_str();  
  }  
  inline  
  SimpleChannelAdmin::Expected&  
  SimpleChannelAdmin::Expected::status(const std::string& val)  
  {  
     values_.insert(std::make_pair(STATUS, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  bool  
  SimpleChannelAdmin::Expected::has_status() const  
  {  
    FieldIndexMap::const_iterator it = values_.find(STATUS);  
    return it != values_.end();  
  }  
  inline  
  std::string  
  SimpleChannelAdmin::Expected::status() const  
  {  
    FieldIndexMap::const_iterator it = values_.find(STATUS);  
    return it->second->str();  
  }  
  inline  
  const char*  
  SimpleChannelAdmin::status (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::STATUS].c_str();  
  }  
  inline  
  SimpleChannelAdmin::Expected&  
  SimpleChannelAdmin::Expected::behav_param_list_id(const std::string& val)  
  {  
     values_.insert(std::make_pair(BEHAV_PARAM_LIST_ID, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  SimpleChannelAdmin::behav_param_list_id (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::BEHAV_PARAM_LIST_ID].c_str();  
  }  
  inline  
  SimpleChannelAdmin::Expected&  
  SimpleChannelAdmin::Expected::categories(const std::string& val)  
  {  
     values_.insert(std::make_pair(CATEGORIES, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  SimpleChannelAdmin::categories (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::CATEGORIES].c_str();  
  }  
  inline  
  SimpleChannelAdmin::Expected&  
  SimpleChannelAdmin::Expected::threshold(const std::string& val)  
  {  
     values_.insert(std::make_pair(THRESHOLD, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  SimpleChannelAdmin::threshold (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::THRESHOLD].c_str();  
  }  
  inline  
  SimpleChannelAdmin::Expected&  
  SimpleChannelAdmin::Expected::timestamp(const std::string& val)  
  {  
     values_.insert(std::make_pair(TIMESTAMP, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  SimpleChannelAdmin::timestamp (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::TIMESTAMP].c_str();  
  }  
}
#endif  // __AUTOTESTS_COMMONS_ADMINS_SIMPLECHANNELADMIN_HPP

