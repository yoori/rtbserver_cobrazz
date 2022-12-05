
#ifndef __AUTOTESTS_COMMONS_ADMINS_SITEADMIN_HPP
#define __AUTOTESTS_COMMONS_ADMINS_SITEADMIN_HPP

#include "Admins.hpp"

namespace AutoTest
{
  class SiteAdmin:  
    public BaseAdminCmd<SiteAdmin, 7>  
  {  
    public:    
      class Expected      
      {      
        friend class SiteAdmin;        
        public:        
          Expected() {}          
          Expected& site_id (const std::string& val);          
          Expected& site_id(unsigned long val);          
          Expected& status (const std::string& val);          
          Expected& approved_creative_categories (const std::string& val);          
          Expected& rejected_creative_categories (const std::string& val);          
          Expected& approved_creatives (const std::string& val);          
          Expected& account_id (const std::string& val);          
          Expected& noads_timeout (const std::string& val);          
                
          enum FieldName          
          {          
            SITE_ID = 0,            
            STATUS,            
            APPROVED_CREATIVE_CATEGORIES,            
            REJECTED_CREATIVE_CATEGORIES,            
            APPROVED_CREATIVES,            
            ACCOUNT_ID,            
            NOADS_TIMEOUT            
          };          
        protected:        
          FieldIndexMap values_;          
      };      
        
    public:    
      typedef BaseAdminCmd<SiteAdmin, FIELDS_COUNT> Base;    
        
      void make_cmd (const char* address,      
        const char* site      
      )      
      {      
        address_ = address;        
        AdminParams params;        
        if (site)        
        {        
          params.push_back(AdminParamPair("site_id=", site));          
        }        
        make_admin_cmd(*this, "sites", address, params, static_cast<size_t>(CampaignManager));      
      }      
      SiteAdmin(      
        const char* address,      
        const char* site      
      )      
      {      
        make_cmd(address, site);      
      }      
      SiteAdmin(      
        const std::string& address,      
        const char* site      
      )      
      {      
        make_cmd(address.c_str(), site);      
      }      
      SiteAdmin(      
        const char* address,      
        const std::string& site      
      )      
      {      
        make_cmd(address, site.c_str());      
      }      
      SiteAdmin(      
        const std::string& address,      
        const std::string& site      
      )      
      {      
        make_cmd(address.c_str(), site.c_str());      
      }      
      SiteAdmin(      
        const char* address,      
        unsigned long site      
      )      
      {      
        make_cmd(address, strof(site).c_str());      
      }      
      SiteAdmin(      
        const std::string& address,      
        unsigned long site      
      )      
      {      
        make_cmd(address.c_str(), strof(site).c_str());      
      }      
      const char* site_id (unsigned int i = 0);      
      const char* status (unsigned int i = 0);      
      const char* approved_creative_categories (unsigned int i = 0);      
      const char* rejected_creative_categories (unsigned int i = 0);      
      const char* approved_creatives (unsigned int i = 0);      
      const char* account_id (unsigned int i = 0);      
      const char* noads_timeout (unsigned int i = 0);      
      bool check(const Expected& expected, bool exist = true)      
        /*throw(eh::Exception)*/;      
      bool check(const std::string& expected, bool exist = true)      
        /*throw(eh::Exception)*/;      
  };  
    
}
namespace AutoTest
{
  inline  
  SiteAdmin::Expected&  
  SiteAdmin::Expected::site_id(const std::string& val)  
  {  
     values_.insert(std::make_pair(SITE_ID, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  SiteAdmin::site_id (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::SITE_ID].c_str();  
  }  
  inline  
  SiteAdmin::Expected&  
  SiteAdmin::Expected::site_id(unsigned long val)  
  {  
     values_.insert(std::make_pair(SITE_ID, ComparableRegExp(strof(val))));  
     return *this;  
  }  
  inline  
  SiteAdmin::Expected&  
  SiteAdmin::Expected::status(const std::string& val)  
  {  
     values_.insert(std::make_pair(STATUS, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  SiteAdmin::status (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::STATUS].c_str();  
  }  
  inline  
  SiteAdmin::Expected&  
  SiteAdmin::Expected::approved_creative_categories(const std::string& val)  
  {  
     values_.insert(std::make_pair(APPROVED_CREATIVE_CATEGORIES, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  SiteAdmin::approved_creative_categories (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::APPROVED_CREATIVE_CATEGORIES].c_str();  
  }  
  inline  
  SiteAdmin::Expected&  
  SiteAdmin::Expected::rejected_creative_categories(const std::string& val)  
  {  
     values_.insert(std::make_pair(REJECTED_CREATIVE_CATEGORIES, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  SiteAdmin::rejected_creative_categories (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::REJECTED_CREATIVE_CATEGORIES].c_str();  
  }  
  inline  
  SiteAdmin::Expected&  
  SiteAdmin::Expected::approved_creatives(const std::string& val)  
  {  
     values_.insert(std::make_pair(APPROVED_CREATIVES, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  SiteAdmin::approved_creatives (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::APPROVED_CREATIVES].c_str();  
  }  
  inline  
  SiteAdmin::Expected&  
  SiteAdmin::Expected::account_id(const std::string& val)  
  {  
     values_.insert(std::make_pair(ACCOUNT_ID, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  SiteAdmin::account_id (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::ACCOUNT_ID].c_str();  
  }  
  inline  
  SiteAdmin::Expected&  
  SiteAdmin::Expected::noads_timeout(const std::string& val)  
  {  
     values_.insert(std::make_pair(NOADS_TIMEOUT, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  SiteAdmin::noads_timeout (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::NOADS_TIMEOUT].c_str();  
  }  
}
#endif  // __AUTOTESTS_COMMONS_ADMINS_SITEADMIN_HPP

