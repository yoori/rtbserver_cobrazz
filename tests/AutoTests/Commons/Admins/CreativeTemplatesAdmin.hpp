
#ifndef __AUTOTESTS_COMMONS_ADMINS_CREATIVETEMPLATESADMIN_HPP
#define __AUTOTESTS_COMMONS_ADMINS_CREATIVETEMPLATESADMIN_HPP

#include "Admins.hpp"

namespace AutoTest
{
  class CreativeTemplatesAdmin:  
    public BaseAdminCmd<CreativeTemplatesAdmin, 8>  
  {  
    public:    
      class Expected      
      {      
        friend class CreativeTemplatesAdmin;        
        public:        
          Expected() {}          
          Expected& creative_format (const std::string& val);          
          Expected& creative_size (const std::string& val);          
          Expected& app_format (const std::string& val);          
          Expected& track_impression (const std::string& val);          
          Expected& template_file (const std::string& val);          
          Expected& type (const std::string& val);          
          Expected& timestamp (const std::string& val);          
          Expected& status (const std::string& val);          
                
          enum FieldName          
          {          
            CREATIVE_FORMAT = 0,            
            CREATIVE_SIZE,            
            APP_FORMAT,            
            TRACK_IMPRESSION,            
            TEMPLATE_FILE,            
            TYPE,            
            TIMESTAMP,            
            STATUS            
          };          
        protected:        
          FieldIndexMap values_;          
      };      
        
    public:    
      typedef BaseAdminCmd<CreativeTemplatesAdmin, FIELDS_COUNT> Base;    
        
      void make_cmd (const char* address      
      )      
      {      
        address_ = address;        
        AdminParams params;        
        make_admin_cmd(*this, "creative_templates", address, params, static_cast<size_t>(CampaignManager));      
      }      
      CreativeTemplatesAdmin(      
        const char* address      
      )      
      {      
        make_cmd(address);      
      }      
      CreativeTemplatesAdmin(      
        const std::string& address      
      )      
      {      
        make_cmd(address.c_str());      
      }      
      const char* creative_format (unsigned int i = 0);      
      const char* creative_size (unsigned int i = 0);      
      const char* app_format (unsigned int i = 0);      
      const char* track_impression (unsigned int i = 0);      
      const char* template_file (unsigned int i = 0);      
      const char* type (unsigned int i = 0);      
      const char* timestamp (unsigned int i = 0);      
      const char* status (unsigned int i = 0);      
      bool check(const Expected& expected, bool exist = true)      
        /*throw(eh::Exception)*/;      
      bool check(const std::string& expected, bool exist = true)      
        /*throw(eh::Exception)*/;      
  };  
    
}
namespace AutoTest
{
  inline  
  CreativeTemplatesAdmin::Expected&  
  CreativeTemplatesAdmin::Expected::creative_format(const std::string& val)  
  {  
     values_.insert(std::make_pair(CREATIVE_FORMAT, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  CreativeTemplatesAdmin::creative_format (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::CREATIVE_FORMAT].c_str();  
  }  
  inline  
  CreativeTemplatesAdmin::Expected&  
  CreativeTemplatesAdmin::Expected::creative_size(const std::string& val)  
  {  
     values_.insert(std::make_pair(CREATIVE_SIZE, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  CreativeTemplatesAdmin::creative_size (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::CREATIVE_SIZE].c_str();  
  }  
  inline  
  CreativeTemplatesAdmin::Expected&  
  CreativeTemplatesAdmin::Expected::app_format(const std::string& val)  
  {  
     values_.insert(std::make_pair(APP_FORMAT, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  CreativeTemplatesAdmin::app_format (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::APP_FORMAT].c_str();  
  }  
  inline  
  CreativeTemplatesAdmin::Expected&  
  CreativeTemplatesAdmin::Expected::track_impression(const std::string& val)  
  {  
     values_.insert(std::make_pair(TRACK_IMPRESSION, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  CreativeTemplatesAdmin::track_impression (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::TRACK_IMPRESSION].c_str();  
  }  
  inline  
  CreativeTemplatesAdmin::Expected&  
  CreativeTemplatesAdmin::Expected::template_file(const std::string& val)  
  {  
     values_.insert(std::make_pair(TEMPLATE_FILE, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  CreativeTemplatesAdmin::template_file (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::TEMPLATE_FILE].c_str();  
  }  
  inline  
  CreativeTemplatesAdmin::Expected&  
  CreativeTemplatesAdmin::Expected::type(const std::string& val)  
  {  
     values_.insert(std::make_pair(TYPE, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  CreativeTemplatesAdmin::type (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::TYPE].c_str();  
  }  
  inline  
  CreativeTemplatesAdmin::Expected&  
  CreativeTemplatesAdmin::Expected::timestamp(const std::string& val)  
  {  
     values_.insert(std::make_pair(TIMESTAMP, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  CreativeTemplatesAdmin::timestamp (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::TIMESTAMP].c_str();  
  }  
  inline  
  CreativeTemplatesAdmin::Expected&  
  CreativeTemplatesAdmin::Expected::status(const std::string& val)  
  {  
     values_.insert(std::make_pair(STATUS, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  CreativeTemplatesAdmin::status (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::STATUS].c_str();  
  }  
}
#endif  // __AUTOTESTS_COMMONS_ADMINS_CREATIVETEMPLATESADMIN_HPP

