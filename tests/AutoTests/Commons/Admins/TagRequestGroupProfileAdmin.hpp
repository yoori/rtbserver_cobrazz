
#ifndef __AUTOTESTS_COMMONS_ADMINS_TAGREQUESTGROUPPROFILEADMIN_HPP
#define __AUTOTESTS_COMMONS_ADMINS_TAGREQUESTGROUPPROFILEADMIN_HPP

#include "Admins.hpp"

namespace AutoTest
{
  class TagRequestGroupProfileAdmin:  
    public BaseAdminCmd<TagRequestGroupProfileAdmin, 9>  
  {  
    public:    
      class Expected      
      {      
        friend class TagRequestGroupProfileAdmin;        
        public:        
          Expected() {}          
          Expected& country (const std::string& val);          
          Expected& colo_id (const std::string& val);          
          Expected& site_id (const std::string& val);          
          Expected& page_load_id (const std::string& val);          
          Expected& referer_hash (const std::string& val);          
          Expected& min_time (const std::string& val);          
          Expected& max_time (const std::string& val);          
          Expected& tags (const std::string& val);          
          Expected& ad_shown (const std::string& val);          
                
          enum FieldName          
          {          
            COUNTRY = 0,            
            COLO_ID,            
            SITE_ID,            
            PAGE_LOAD_ID,            
            REFERER_HASH,            
            MIN_TIME,            
            MAX_TIME,            
            TAGS,            
            AD_SHOWN            
          };          
        protected:        
          FieldIndexMap values_;          
      };      
        
    public:    
      typedef BaseAdminCmd<TagRequestGroupProfileAdmin, FIELDS_COUNT> Base;    
        
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
        make_admin_cmd(*this, "print-tag-request-groups", address, params, static_cast<size_t>(service));      
        add_cmd_i("--align");      
      }      
      TagRequestGroupProfileAdmin(      
        const char* address,      
        const char* uuid,      
        RequestInfoSrv service = RequestInfoManager      
      )      
      {      
        make_cmd(address, uuid, service);      
      }      
      TagRequestGroupProfileAdmin(      
        const std::string& address,      
        const char* uuid,      
        RequestInfoSrv service = RequestInfoManager      
      )      
      {      
        make_cmd(address.c_str(), uuid, service);      
      }      
      TagRequestGroupProfileAdmin(      
        const char* address,      
        const std::string& uuid,      
        RequestInfoSrv service = RequestInfoManager      
      )      
      {      
        make_cmd(address, uuid.c_str(), service);      
      }      
      TagRequestGroupProfileAdmin(      
        const std::string& address,      
        const std::string& uuid,      
        RequestInfoSrv service = RequestInfoManager      
      )      
      {      
        make_cmd(address.c_str(), uuid.c_str(), service);      
      }      
      const char* country (unsigned int i = 0);      
      const char* colo_id (unsigned int i = 0);      
      const char* site_id (unsigned int i = 0);      
      const char* page_load_id (unsigned int i = 0);      
      const char* referer_hash (unsigned int i = 0);      
      const char* min_time (unsigned int i = 0);      
      const char* max_time (unsigned int i = 0);      
      const char* tags (unsigned int i = 0);      
      const char* ad_shown (unsigned int i = 0);      
      bool check(const Expected& expected, bool exist = true)      
        /*throw(eh::Exception)*/;      
      bool check(const std::string& expected, bool exist = true)      
        /*throw(eh::Exception)*/;      
  };  
    
}
namespace AutoTest
{
  inline  
  TagRequestGroupProfileAdmin::Expected&  
  TagRequestGroupProfileAdmin::Expected::country(const std::string& val)  
  {  
     values_.insert(std::make_pair(COUNTRY, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  TagRequestGroupProfileAdmin::country (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::COUNTRY].c_str();  
  }  
  inline  
  TagRequestGroupProfileAdmin::Expected&  
  TagRequestGroupProfileAdmin::Expected::colo_id(const std::string& val)  
  {  
     values_.insert(std::make_pair(COLO_ID, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  TagRequestGroupProfileAdmin::colo_id (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::COLO_ID].c_str();  
  }  
  inline  
  TagRequestGroupProfileAdmin::Expected&  
  TagRequestGroupProfileAdmin::Expected::site_id(const std::string& val)  
  {  
     values_.insert(std::make_pair(SITE_ID, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  TagRequestGroupProfileAdmin::site_id (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::SITE_ID].c_str();  
  }  
  inline  
  TagRequestGroupProfileAdmin::Expected&  
  TagRequestGroupProfileAdmin::Expected::page_load_id(const std::string& val)  
  {  
     values_.insert(std::make_pair(PAGE_LOAD_ID, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  TagRequestGroupProfileAdmin::page_load_id (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::PAGE_LOAD_ID].c_str();  
  }  
  inline  
  TagRequestGroupProfileAdmin::Expected&  
  TagRequestGroupProfileAdmin::Expected::referer_hash(const std::string& val)  
  {  
     values_.insert(std::make_pair(REFERER_HASH, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  TagRequestGroupProfileAdmin::referer_hash (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::REFERER_HASH].c_str();  
  }  
  inline  
  TagRequestGroupProfileAdmin::Expected&  
  TagRequestGroupProfileAdmin::Expected::min_time(const std::string& val)  
  {  
     values_.insert(std::make_pair(MIN_TIME, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  TagRequestGroupProfileAdmin::min_time (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::MIN_TIME].c_str();  
  }  
  inline  
  TagRequestGroupProfileAdmin::Expected&  
  TagRequestGroupProfileAdmin::Expected::max_time(const std::string& val)  
  {  
     values_.insert(std::make_pair(MAX_TIME, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  TagRequestGroupProfileAdmin::max_time (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::MAX_TIME].c_str();  
  }  
  inline  
  TagRequestGroupProfileAdmin::Expected&  
  TagRequestGroupProfileAdmin::Expected::tags(const std::string& val)  
  {  
     values_.insert(std::make_pair(TAGS, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  TagRequestGroupProfileAdmin::tags (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::TAGS].c_str();  
  }  
  inline  
  TagRequestGroupProfileAdmin::Expected&  
  TagRequestGroupProfileAdmin::Expected::ad_shown(const std::string& val)  
  {  
     values_.insert(std::make_pair(AD_SHOWN, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  TagRequestGroupProfileAdmin::ad_shown (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::AD_SHOWN].c_str();  
  }  
}
#endif  // __AUTOTESTS_COMMONS_ADMINS_TAGREQUESTGROUPPROFILEADMIN_HPP

