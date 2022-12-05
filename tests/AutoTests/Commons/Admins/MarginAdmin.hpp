
#ifndef __AUTOTESTS_COMMONS_ADMINS_MARGINADMIN_HPP
#define __AUTOTESTS_COMMONS_ADMINS_MARGINADMIN_HPP

#include "Admins.hpp"

namespace AutoTest
{
  class MarginAdmin:  
    public BaseAdminCmd<MarginAdmin, 9>  
  {  
    public:    
      class Expected      
      {      
        friend class MarginAdmin;        
        public:        
          Expected() {}          
          Expected& margin_rule_id (const std::string& val);          
          Expected& margin_rule_id(unsigned long val);          
          Expected& account_id (const std::string& val);          
          Expected& account_id(unsigned long val);          
          Expected& type (const std::string& val);          
          Expected& sort_order (const std::string& val);          
          Expected& fixed_margin (const std::string& val);          
          Expected& relative_margin (const std::string& val);          
          Expected& isp_accounts (const std::string& val);          
          Expected& publisher_accounts (const std::string& val);          
          Expected& advertiser_accounts (const std::string& val);          
                
          enum FieldName          
          {          
            MARGIN_RULE_ID = 0,            
            ACCOUNT_ID,            
            TYPE,            
            SORT_ORDER,            
            FIXED_MARGIN,            
            RELATIVE_MARGIN,            
            ISP_ACCOUNTS,            
            PUBLISHER_ACCOUNTS,            
            ADVERTISER_ACCOUNTS            
          };          
        protected:        
          FieldIndexMap values_;          
      };      
        
    public:    
      typedef BaseAdminCmd<MarginAdmin, FIELDS_COUNT> Base;    
        
      void make_cmd (const char* address,      
        const char* margin      
      )      
      {      
        address_ = address;        
        AdminParams params;        
        if (margin)        
        {        
          params.push_back(AdminParamPair("margin_rule_id=", margin));          
        }        
        make_admin_cmd(*this, "margin", address, params, static_cast<size_t>(CampaignManager));      
      }      
      MarginAdmin(      
        const char* address,      
        const char* margin      
      )      
      {      
        make_cmd(address, margin);      
      }      
      MarginAdmin(      
        const std::string& address,      
        const char* margin      
      )      
      {      
        make_cmd(address.c_str(), margin);      
      }      
      MarginAdmin(      
        const char* address,      
        const std::string& margin      
      )      
      {      
        make_cmd(address, margin.c_str());      
      }      
      MarginAdmin(      
        const std::string& address,      
        const std::string& margin      
      )      
      {      
        make_cmd(address.c_str(), margin.c_str());      
      }      
      MarginAdmin(      
        const char* address,      
        unsigned long margin      
      )      
      {      
        make_cmd(address, strof(margin).c_str());      
      }      
      MarginAdmin(      
        const std::string& address,      
        unsigned long margin      
      )      
      {      
        make_cmd(address.c_str(), strof(margin).c_str());      
      }      
      const char* margin_rule_id (unsigned int i = 0);      
      const char* account_id (unsigned int i = 0);      
      const char* type (unsigned int i = 0);      
      const char* sort_order (unsigned int i = 0);      
      const char* fixed_margin (unsigned int i = 0);      
      const char* relative_margin (unsigned int i = 0);      
      const char* isp_accounts (unsigned int i = 0);      
      const char* publisher_accounts (unsigned int i = 0);      
      const char* advertiser_accounts (unsigned int i = 0);      
      bool check(const Expected& expected, bool exist = true)      
        /*throw(eh::Exception)*/;      
      bool check(const std::string& expected, bool exist = true)      
        /*throw(eh::Exception)*/;      
  };  
    
}
namespace AutoTest
{
  inline  
  MarginAdmin::Expected&  
  MarginAdmin::Expected::margin_rule_id(const std::string& val)  
  {  
     values_.insert(std::make_pair(MARGIN_RULE_ID, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  MarginAdmin::margin_rule_id (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::MARGIN_RULE_ID].c_str();  
  }  
  inline  
  MarginAdmin::Expected&  
  MarginAdmin::Expected::margin_rule_id(unsigned long val)  
  {  
     values_.insert(std::make_pair(MARGIN_RULE_ID, ComparableRegExp(strof(val))));  
     return *this;  
  }  
  inline  
  MarginAdmin::Expected&  
  MarginAdmin::Expected::account_id(const std::string& val)  
  {  
     values_.insert(std::make_pair(ACCOUNT_ID, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  MarginAdmin::account_id (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::ACCOUNT_ID].c_str();  
  }  
  inline  
  MarginAdmin::Expected&  
  MarginAdmin::Expected::account_id(unsigned long val)  
  {  
     values_.insert(std::make_pair(ACCOUNT_ID, ComparableRegExp(strof(val))));  
     return *this;  
  }  
  inline  
  MarginAdmin::Expected&  
  MarginAdmin::Expected::type(const std::string& val)  
  {  
     values_.insert(std::make_pair(TYPE, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  MarginAdmin::type (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::TYPE].c_str();  
  }  
  inline  
  MarginAdmin::Expected&  
  MarginAdmin::Expected::sort_order(const std::string& val)  
  {  
     values_.insert(std::make_pair(SORT_ORDER, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  MarginAdmin::sort_order (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::SORT_ORDER].c_str();  
  }  
  inline  
  MarginAdmin::Expected&  
  MarginAdmin::Expected::fixed_margin(const std::string& val)  
  {  
     values_.insert(std::make_pair(FIXED_MARGIN, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  MarginAdmin::fixed_margin (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::FIXED_MARGIN].c_str();  
  }  
  inline  
  MarginAdmin::Expected&  
  MarginAdmin::Expected::relative_margin(const std::string& val)  
  {  
     values_.insert(std::make_pair(RELATIVE_MARGIN, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  MarginAdmin::relative_margin (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::RELATIVE_MARGIN].c_str();  
  }  
  inline  
  MarginAdmin::Expected&  
  MarginAdmin::Expected::isp_accounts(const std::string& val)  
  {  
     values_.insert(std::make_pair(ISP_ACCOUNTS, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  MarginAdmin::isp_accounts (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::ISP_ACCOUNTS].c_str();  
  }  
  inline  
  MarginAdmin::Expected&  
  MarginAdmin::Expected::publisher_accounts(const std::string& val)  
  {  
     values_.insert(std::make_pair(PUBLISHER_ACCOUNTS, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  MarginAdmin::publisher_accounts (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::PUBLISHER_ACCOUNTS].c_str();  
  }  
  inline  
  MarginAdmin::Expected&  
  MarginAdmin::Expected::advertiser_accounts(const std::string& val)  
  {  
     values_.insert(std::make_pair(ADVERTISER_ACCOUNTS, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  MarginAdmin::advertiser_accounts (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::ADVERTISER_ACCOUNTS].c_str();  
  }  
}
#endif  // __AUTOTESTS_COMMONS_ADMINS_MARGINADMIN_HPP

