
#ifndef __AUTOTESTS_COMMONS_ADMINS_TAGADMIN_HPP
#define __AUTOTESTS_COMMONS_ADMINS_TAGADMIN_HPP

#include "Admins.hpp"

namespace AutoTest
{
  class TagAdmin:  
    public BaseAdminCmd<TagAdmin, 6>  
  {  
    public:    
      class Expected      
      {      
        friend class TagAdmin;        
        public:        
          Expected() {}          
          Expected& tag_id (const std::string& val);          
          Expected& tag_id(unsigned long val);          
          Expected& site_id (const std::string& val);          
          Expected& site_id(unsigned long val);          
          Expected& sizes (const std::string& val);          
          Expected& accepted_categories (const std::string& val);          
          Expected& rejected_categories (const std::string& val);          
          Expected& imp_track_pixel (const std::string& val);          
                
          enum FieldName          
          {          
            TAG_ID = 0,            
            SITE_ID,            
            SIZES,            
            ACCEPTED_CATEGORIES,            
            REJECTED_CATEGORIES,            
            IMP_TRACK_PIXEL            
          };          
        protected:        
          FieldIndexMap values_;          
      };      
        
    public:    
      typedef BaseAdminCmd<TagAdmin, FIELDS_COUNT> Base;    
        
      void make_cmd (const char* address,      
        const char* tag      
      )      
      {      
        address_ = address;        
        AdminParams params;        
        if (tag)        
        {        
          params.push_back(AdminParamPair("tag_id=", tag));          
        }        
        make_admin_cmd(*this, "tags", address, params, static_cast<size_t>(CampaignManager));      
      }      
      TagAdmin(      
        const char* address,      
        const char* tag      
      )      
      {      
        make_cmd(address, tag);      
      }      
      TagAdmin(      
        const std::string& address,      
        const char* tag      
      )      
      {      
        make_cmd(address.c_str(), tag);      
      }      
      TagAdmin(      
        const char* address,      
        const std::string& tag      
      )      
      {      
        make_cmd(address, tag.c_str());      
      }      
      TagAdmin(      
        const std::string& address,      
        const std::string& tag      
      )      
      {      
        make_cmd(address.c_str(), tag.c_str());      
      }      
      TagAdmin(      
        const char* address,      
        unsigned long tag      
      )      
      {      
        make_cmd(address, strof(tag).c_str());      
      }      
      TagAdmin(      
        const std::string& address,      
        unsigned long tag      
      )      
      {      
        make_cmd(address.c_str(), strof(tag).c_str());      
      }      
      const char* tag_id (unsigned int i = 0);      
      const char* site_id (unsigned int i = 0);      
      const char* sizes (unsigned int i = 0);      
      const char* accepted_categories (unsigned int i = 0);      
      const char* rejected_categories (unsigned int i = 0);      
      const char* imp_track_pixel (unsigned int i = 0);      
      bool check(const Expected& expected, bool exist = true)      
        /*throw(eh::Exception)*/;      
      bool check(const std::string& expected, bool exist = true)      
        /*throw(eh::Exception)*/;      
  };  
    
}
namespace AutoTest
{
  inline  
  TagAdmin::Expected&  
  TagAdmin::Expected::tag_id(const std::string& val)  
  {  
     values_.insert(std::make_pair(TAG_ID, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  TagAdmin::tag_id (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::TAG_ID].c_str();  
  }  
  inline  
  TagAdmin::Expected&  
  TagAdmin::Expected::tag_id(unsigned long val)  
  {  
     values_.insert(std::make_pair(TAG_ID, ComparableRegExp(strof(val))));  
     return *this;  
  }  
  inline  
  TagAdmin::Expected&  
  TagAdmin::Expected::site_id(const std::string& val)  
  {  
     values_.insert(std::make_pair(SITE_ID, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  TagAdmin::site_id (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::SITE_ID].c_str();  
  }  
  inline  
  TagAdmin::Expected&  
  TagAdmin::Expected::site_id(unsigned long val)  
  {  
     values_.insert(std::make_pair(SITE_ID, ComparableRegExp(strof(val))));  
     return *this;  
  }  
  inline  
  TagAdmin::Expected&  
  TagAdmin::Expected::sizes(const std::string& val)  
  {  
     values_.insert(std::make_pair(SIZES, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  TagAdmin::sizes (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::SIZES].c_str();  
  }  
  inline  
  TagAdmin::Expected&  
  TagAdmin::Expected::accepted_categories(const std::string& val)  
  {  
     values_.insert(std::make_pair(ACCEPTED_CATEGORIES, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  TagAdmin::accepted_categories (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::ACCEPTED_CATEGORIES].c_str();  
  }  
  inline  
  TagAdmin::Expected&  
  TagAdmin::Expected::rejected_categories(const std::string& val)  
  {  
     values_.insert(std::make_pair(REJECTED_CATEGORIES, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  TagAdmin::rejected_categories (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::REJECTED_CATEGORIES].c_str();  
  }  
  inline  
  TagAdmin::Expected&  
  TagAdmin::Expected::imp_track_pixel(const std::string& val)  
  {  
     values_.insert(std::make_pair(IMP_TRACK_PIXEL, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  TagAdmin::imp_track_pixel (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::IMP_TRACK_PIXEL].c_str();  
  }  
}
#endif  // __AUTOTESTS_COMMONS_ADMINS_TAGADMIN_HPP

