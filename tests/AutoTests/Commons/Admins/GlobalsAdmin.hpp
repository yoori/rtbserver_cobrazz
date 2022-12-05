
#ifndef __AUTOTESTS_COMMONS_ADMINS_GLOBALSADMIN_HPP
#define __AUTOTESTS_COMMONS_ADMINS_GLOBALSADMIN_HPP

#include "Admins.hpp"

namespace AutoTest
{
  class GlobalsAdmin:  
    public BaseAdminCmd<GlobalsAdmin, 5>  
  {  
    public:    
      class Expected      
      {      
        friend class GlobalsAdmin;        
        public:        
          Expected() {}          
          Expected& currency_exchange_id (const std::string& val);          
          Expected& currency_exchange_id(unsigned long val);          
          Expected& foros_min_margin (const std::string& val);          
          bool has_foros_min_margin () const;          
          std::string foros_min_margin () const;          
          Expected& foros_min_margin(unsigned long val);          
          Expected& foros_min_fixed_margin (const std::string& val);          
          bool has_foros_min_fixed_margin () const;          
          std::string foros_min_fixed_margin () const;          
          Expected& foros_min_fixed_margin(unsigned long val);          
          Expected& global_params_timestamp (const std::string& val);          
          bool has_global_params_timestamp () const;          
          std::string global_params_timestamp () const;          
          Expected& master_stamp (const std::string& val);          
          bool has_master_stamp () const;          
          std::string master_stamp () const;          
                
          enum FieldName          
          {          
            CURRENCY_EXCHANGE_ID = 0,            
            OIX_MIN_MARGIN,            
            OIX_MIN_FIXED_MARGIN,            
            GLOBAL_PARAMS_TIMESTAMP,            
            MASTER_STAMP            
          };          
        protected:        
          FieldIndexMap values_;          
      };      
        
    public:    
      typedef BaseAdminCmd<GlobalsAdmin, FIELDS_COUNT> Base;    
        
      void make_cmd (const char* address      
      )      
      {      
        address_ = address;        
        AdminParams params;        
        make_admin_cmd(*this, "globals", address, params, static_cast<size_t>(CampaignManager));      
      }      
      GlobalsAdmin(      
        const char* address      
      )      
      {      
        make_cmd(address);      
      }      
      GlobalsAdmin(      
        const std::string& address      
      )      
      {      
        make_cmd(address.c_str());      
      }      
      const char* currency_exchange_id (unsigned int i = 0);      
      const char* foros_min_margin (unsigned int i = 0);      
      const char* foros_min_fixed_margin (unsigned int i = 0);      
      const char* global_params_timestamp (unsigned int i = 0);      
      const char* master_stamp (unsigned int i = 0);      
      bool check(const Expected& expected, bool exist = true)      
        /*throw(eh::Exception)*/;      
      bool check(const std::string& expected, bool exist = true)      
        /*throw(eh::Exception)*/;      
  };  
    
}
namespace AutoTest
{
  inline  
  GlobalsAdmin::Expected&  
  GlobalsAdmin::Expected::currency_exchange_id(const std::string& val)  
  {  
     values_.insert(std::make_pair(CURRENCY_EXCHANGE_ID, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  GlobalsAdmin::currency_exchange_id (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::CURRENCY_EXCHANGE_ID].c_str();  
  }  
  inline  
  GlobalsAdmin::Expected&  
  GlobalsAdmin::Expected::currency_exchange_id(unsigned long val)  
  {  
     values_.insert(std::make_pair(CURRENCY_EXCHANGE_ID, ComparableRegExp(strof(val))));  
     return *this;  
  }  
  inline  
  GlobalsAdmin::Expected&  
  GlobalsAdmin::Expected::foros_min_margin(const std::string& val)  
  {  
     values_.insert(std::make_pair(OIX_MIN_MARGIN, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  bool  
  GlobalsAdmin::Expected::has_foros_min_margin() const  
  {  
    FieldIndexMap::const_iterator it = values_.find(OIX_MIN_MARGIN);  
    return it != values_.end();  
  }  
  inline  
  std::string  
  GlobalsAdmin::Expected::foros_min_margin() const  
  {  
    FieldIndexMap::const_iterator it = values_.find(OIX_MIN_MARGIN);  
    return it->second->str();  
  }  
  inline  
  const char*  
  GlobalsAdmin::foros_min_margin (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::OIX_MIN_MARGIN].c_str();  
  }  
  inline  
  GlobalsAdmin::Expected&  
  GlobalsAdmin::Expected::foros_min_margin(unsigned long val)  
  {  
     values_.insert(std::make_pair(OIX_MIN_MARGIN, ComparableRegExp(strof(val))));  
     return *this;  
  }  
  inline  
  GlobalsAdmin::Expected&  
  GlobalsAdmin::Expected::foros_min_fixed_margin(const std::string& val)  
  {  
     values_.insert(std::make_pair(OIX_MIN_FIXED_MARGIN, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  bool  
  GlobalsAdmin::Expected::has_foros_min_fixed_margin() const  
  {  
    FieldIndexMap::const_iterator it = values_.find(OIX_MIN_FIXED_MARGIN);  
    return it != values_.end();  
  }  
  inline  
  std::string  
  GlobalsAdmin::Expected::foros_min_fixed_margin() const  
  {  
    FieldIndexMap::const_iterator it = values_.find(OIX_MIN_FIXED_MARGIN);  
    return it->second->str();  
  }  
  inline  
  const char*  
  GlobalsAdmin::foros_min_fixed_margin (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::OIX_MIN_FIXED_MARGIN].c_str();  
  }  
  inline  
  GlobalsAdmin::Expected&  
  GlobalsAdmin::Expected::foros_min_fixed_margin(unsigned long val)  
  {  
     values_.insert(std::make_pair(OIX_MIN_FIXED_MARGIN, ComparableRegExp(strof(val))));  
     return *this;  
  }  
  inline  
  GlobalsAdmin::Expected&  
  GlobalsAdmin::Expected::global_params_timestamp(const std::string& val)  
  {  
     values_.insert(std::make_pair(GLOBAL_PARAMS_TIMESTAMP, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  bool  
  GlobalsAdmin::Expected::has_global_params_timestamp() const  
  {  
    FieldIndexMap::const_iterator it = values_.find(GLOBAL_PARAMS_TIMESTAMP);  
    return it != values_.end();  
  }  
  inline  
  std::string  
  GlobalsAdmin::Expected::global_params_timestamp() const  
  {  
    FieldIndexMap::const_iterator it = values_.find(GLOBAL_PARAMS_TIMESTAMP);  
    return it->second->str();  
  }  
  inline  
  const char*  
  GlobalsAdmin::global_params_timestamp (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::GLOBAL_PARAMS_TIMESTAMP].c_str();  
  }  
  inline  
  GlobalsAdmin::Expected&  
  GlobalsAdmin::Expected::master_stamp(const std::string& val)  
  {  
     values_.insert(std::make_pair(MASTER_STAMP, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  bool  
  GlobalsAdmin::Expected::has_master_stamp() const  
  {  
    FieldIndexMap::const_iterator it = values_.find(MASTER_STAMP);  
    return it != values_.end();  
  }  
  inline  
  std::string  
  GlobalsAdmin::Expected::master_stamp() const  
  {  
    FieldIndexMap::const_iterator it = values_.find(MASTER_STAMP);  
    return it->second->str();  
  }  
  inline  
  const char*  
  GlobalsAdmin::master_stamp (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::MASTER_STAMP].c_str();  
  }  
}
#endif  // __AUTOTESTS_COMMONS_ADMINS_GLOBALSADMIN_HPP

