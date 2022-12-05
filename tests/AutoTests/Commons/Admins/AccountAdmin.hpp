
#ifndef __AUTOTESTS_COMMONS_ADMINS_ACCOUNTADMIN_HPP
#define __AUTOTESTS_COMMONS_ADMINS_ACCOUNTADMIN_HPP

#include "Admins.hpp"

namespace AutoTest
{
  class AccountAdmin:  
    public BaseAdminCmd<AccountAdmin, 20>  
  {  
    public:    
      class Expected      
      {      
        friend class AccountAdmin;        
        public:        
          Expected(): force_remote_present_(false) {}          
          Expected& account_id (const std::string& val);          
          Expected& account_id(unsigned long val);          
          Expected& agency_account_id (const std::string& val);          
          Expected& agency_account_id(unsigned long val);          
          Expected& internal_account_id (const std::string& val);          
          Expected& internal_account_id(unsigned long val);          
          Expected& status (const std::string& val);          
          bool has_status () const;          
          std::string status () const;          
          Expected& eval_status (const std::string& val);          
          bool has_eval_status () const;          
          std::string eval_status () const;          
          Expected& flags (const std::string& val);          
          Expected& at_flags (const std::string& val);          
          Expected& text_adserving (const std::string& val);          
          Expected& currency_id (const std::string& val);          
          Expected& currency_id(unsigned long val);          
          Expected& country (const std::string& val);          
          Expected& commision (const std::string& val);          
          Expected& commision (const Money& val);          
          Expected& budget (const std::string& val);          
          Expected& budget (const Money& val);          
          Expected& paid_amount (const std::string& val);          
          Expected& paid_amount (const Money& val);          
          Expected& time_offset (const std::string& val);          
          Expected& walled_garden_accounts (const std::string& val);          
          Expected& auction_rate (const std::string& val);          
          Expected& use_pub_pixels (const std::string& val);          
          Expected& pub_pixel_optin (const std::string& val);          
          Expected& pub_pixel_optout (const std::string& val);          
          Expected& timestamp (const std::string& val);          
          Expected& force_remote_present(bool val);          
          bool force_remote_present() const;          
                
          enum FieldName          
          {          
            ACCOUNT_ID = 0,            
            AGENCY_ACCOUNT_ID,            
            INTERNAL_ACCOUNT_ID,            
            STATUS,            
            EVAL_STATUS,            
            FLAGS,            
            AT_FLAGS,            
            TEXT_ADSERVING,            
            CURRENCY_ID,            
            COUNTRY,            
            COMMISION,            
            BUDGET,            
            PAID_AMOUNT,            
            TIME_OFFSET,            
            WALLED_GARDEN_ACCOUNTS,            
            AUCTION_RATE,            
            USE_PUB_PIXELS,            
            PUB_PIXEL_OPTIN,            
            PUB_PIXEL_OPTOUT,            
            TIMESTAMP            
          };          
        protected:        
          FieldIndexMap values_;          
          bool force_remote_present_;          
      };      
        
    public:    
      typedef BaseAdminCmd<AccountAdmin, FIELDS_COUNT> Base;    
        
      void make_cmd (const char* address,      
        const char* account      
      )      
      {      
        address_ = address;        
        AdminParams params;        
        if (account)        
        {        
          params.push_back(AdminParamPair("account_id=", account));          
        }        
        make_admin_cmd(*this, "account", address, params, static_cast<size_t>(CampaignManager));      
      }      
      AccountAdmin(      
        const char* address,      
        const char* account      
      )      
      {      
        make_cmd(address, account);      
      }      
      AccountAdmin(      
        const std::string& address,      
        const char* account      
      )      
      {      
        make_cmd(address.c_str(), account);      
      }      
      AccountAdmin(      
        const char* address,      
        const std::string& account      
      )      
      {      
        make_cmd(address, account.c_str());      
      }      
      AccountAdmin(      
        const std::string& address,      
        const std::string& account      
      )      
      {      
        make_cmd(address.c_str(), account.c_str());      
      }      
      AccountAdmin(      
        const char* address,      
        unsigned long account      
      )      
      {      
        make_cmd(address, strof(account).c_str());      
      }      
      AccountAdmin(      
        const std::string& address,      
        unsigned long account      
      )      
      {      
        make_cmd(address.c_str(), strof(account).c_str());      
      }      
      const char* account_id (unsigned int i = 0);      
      const char* agency_account_id (unsigned int i = 0);      
      const char* internal_account_id (unsigned int i = 0);      
      const char* status (unsigned int i = 0);      
      const char* eval_status (unsigned int i = 0);      
      const char* flags (unsigned int i = 0);      
      const char* at_flags (unsigned int i = 0);      
      const char* text_adserving (unsigned int i = 0);      
      const char* currency_id (unsigned int i = 0);      
      const char* country (unsigned int i = 0);      
      const char* commision (unsigned int i = 0);      
      const char* budget (unsigned int i = 0);      
      const char* paid_amount (unsigned int i = 0);      
      const char* time_offset (unsigned int i = 0);      
      const char* walled_garden_accounts (unsigned int i = 0);      
      const char* auction_rate (unsigned int i = 0);      
      const char* use_pub_pixels (unsigned int i = 0);      
      const char* pub_pixel_optin (unsigned int i = 0);      
      const char* pub_pixel_optout (unsigned int i = 0);      
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
  AccountAdmin::Expected&  
  AccountAdmin::Expected::account_id(const std::string& val)  
  {  
     values_.insert(std::make_pair(ACCOUNT_ID, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  AccountAdmin::account_id (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::ACCOUNT_ID].c_str();  
  }  
  inline  
  AccountAdmin::Expected&  
  AccountAdmin::Expected::account_id(unsigned long val)  
  {  
     values_.insert(std::make_pair(ACCOUNT_ID, ComparableRegExp(strof(val))));  
     return *this;  
  }  
  inline  
  AccountAdmin::Expected&  
  AccountAdmin::Expected::agency_account_id(const std::string& val)  
  {  
     values_.insert(std::make_pair(AGENCY_ACCOUNT_ID, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  AccountAdmin::agency_account_id (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::AGENCY_ACCOUNT_ID].c_str();  
  }  
  inline  
  AccountAdmin::Expected&  
  AccountAdmin::Expected::agency_account_id(unsigned long val)  
  {  
     values_.insert(std::make_pair(AGENCY_ACCOUNT_ID, ComparableRegExp(strof(val))));  
     return *this;  
  }  
  inline  
  AccountAdmin::Expected&  
  AccountAdmin::Expected::internal_account_id(const std::string& val)  
  {  
     values_.insert(std::make_pair(INTERNAL_ACCOUNT_ID, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  AccountAdmin::internal_account_id (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::INTERNAL_ACCOUNT_ID].c_str();  
  }  
  inline  
  AccountAdmin::Expected&  
  AccountAdmin::Expected::internal_account_id(unsigned long val)  
  {  
     values_.insert(std::make_pair(INTERNAL_ACCOUNT_ID, ComparableRegExp(strof(val))));  
     return *this;  
  }  
  inline  
  AccountAdmin::Expected&  
  AccountAdmin::Expected::status(const std::string& val)  
  {  
     values_.insert(std::make_pair(STATUS, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  bool  
  AccountAdmin::Expected::has_status() const  
  {  
    FieldIndexMap::const_iterator it = values_.find(STATUS);  
    return it != values_.end();  
  }  
  inline  
  std::string  
  AccountAdmin::Expected::status() const  
  {  
    FieldIndexMap::const_iterator it = values_.find(STATUS);  
    return it->second->str();  
  }  
  inline  
  const char*  
  AccountAdmin::status (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::STATUS].c_str();  
  }  
  inline  
  AccountAdmin::Expected&  
  AccountAdmin::Expected::eval_status(const std::string& val)  
  {  
     values_.insert(std::make_pair(EVAL_STATUS, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  bool  
  AccountAdmin::Expected::has_eval_status() const  
  {  
    FieldIndexMap::const_iterator it = values_.find(EVAL_STATUS);  
    return it != values_.end();  
  }  
  inline  
  std::string  
  AccountAdmin::Expected::eval_status() const  
  {  
    FieldIndexMap::const_iterator it = values_.find(EVAL_STATUS);  
    return it->second->str();  
  }  
  inline  
  const char*  
  AccountAdmin::eval_status (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::EVAL_STATUS].c_str();  
  }  
  inline  
  AccountAdmin::Expected&  
  AccountAdmin::Expected::flags(const std::string& val)  
  {  
     values_.insert(std::make_pair(FLAGS, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  AccountAdmin::flags (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::FLAGS].c_str();  
  }  
  inline  
  AccountAdmin::Expected&  
  AccountAdmin::Expected::at_flags(const std::string& val)  
  {  
     values_.insert(std::make_pair(AT_FLAGS, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  AccountAdmin::at_flags (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::AT_FLAGS].c_str();  
  }  
  inline  
  AccountAdmin::Expected&  
  AccountAdmin::Expected::text_adserving(const std::string& val)  
  {  
     values_.insert(std::make_pair(TEXT_ADSERVING, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  AccountAdmin::text_adserving (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::TEXT_ADSERVING].c_str();  
  }  
  inline  
  AccountAdmin::Expected&  
  AccountAdmin::Expected::currency_id(const std::string& val)  
  {  
     values_.insert(std::make_pair(CURRENCY_ID, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  AccountAdmin::currency_id (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::CURRENCY_ID].c_str();  
  }  
  inline  
  AccountAdmin::Expected&  
  AccountAdmin::Expected::currency_id(unsigned long val)  
  {  
     values_.insert(std::make_pair(CURRENCY_ID, ComparableRegExp(strof(val))));  
     return *this;  
  }  
  inline  
  AccountAdmin::Expected&  
  AccountAdmin::Expected::country(const std::string& val)  
  {  
     values_.insert(std::make_pair(COUNTRY, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  AccountAdmin::country (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::COUNTRY].c_str();  
  }  
  inline  
  AccountAdmin::Expected&  
  AccountAdmin::Expected::commision(const std::string& val)  
  {  
     values_.insert(std::make_pair(COMMISION, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  AccountAdmin::commision (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::COMMISION].c_str();  
  }  
  inline  
  AccountAdmin::Expected&  
  AccountAdmin::Expected::commision (const Money& val)  
  {  
     values_.insert(std::make_pair(COMMISION, val));  
     return *this;  
  }  
  inline  
  AccountAdmin::Expected&  
  AccountAdmin::Expected::budget(const std::string& val)  
  {  
     values_.insert(std::make_pair(BUDGET, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  AccountAdmin::budget (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::BUDGET].c_str();  
  }  
  inline  
  AccountAdmin::Expected&  
  AccountAdmin::Expected::budget (const Money& val)  
  {  
     values_.insert(std::make_pair(BUDGET, val));  
     return *this;  
  }  
  inline  
  AccountAdmin::Expected&  
  AccountAdmin::Expected::paid_amount(const std::string& val)  
  {  
     values_.insert(std::make_pair(PAID_AMOUNT, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  AccountAdmin::paid_amount (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::PAID_AMOUNT].c_str();  
  }  
  inline  
  AccountAdmin::Expected&  
  AccountAdmin::Expected::paid_amount (const Money& val)  
  {  
     values_.insert(std::make_pair(PAID_AMOUNT, val));  
     return *this;  
  }  
  inline  
  AccountAdmin::Expected&  
  AccountAdmin::Expected::time_offset(const std::string& val)  
  {  
     values_.insert(std::make_pair(TIME_OFFSET, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  AccountAdmin::time_offset (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::TIME_OFFSET].c_str();  
  }  
  inline  
  AccountAdmin::Expected&  
  AccountAdmin::Expected::walled_garden_accounts(const std::string& val)  
  {  
     values_.insert(std::make_pair(WALLED_GARDEN_ACCOUNTS, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  AccountAdmin::walled_garden_accounts (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::WALLED_GARDEN_ACCOUNTS].c_str();  
  }  
  inline  
  AccountAdmin::Expected&  
  AccountAdmin::Expected::auction_rate(const std::string& val)  
  {  
     values_.insert(std::make_pair(AUCTION_RATE, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  AccountAdmin::auction_rate (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::AUCTION_RATE].c_str();  
  }  
  inline  
  AccountAdmin::Expected&  
  AccountAdmin::Expected::use_pub_pixels(const std::string& val)  
  {  
     values_.insert(std::make_pair(USE_PUB_PIXELS, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  AccountAdmin::use_pub_pixels (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::USE_PUB_PIXELS].c_str();  
  }  
  inline  
  AccountAdmin::Expected&  
  AccountAdmin::Expected::pub_pixel_optin(const std::string& val)  
  {  
     values_.insert(std::make_pair(PUB_PIXEL_OPTIN, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  AccountAdmin::pub_pixel_optin (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::PUB_PIXEL_OPTIN].c_str();  
  }  
  inline  
  AccountAdmin::Expected&  
  AccountAdmin::Expected::pub_pixel_optout(const std::string& val)  
  {  
     values_.insert(std::make_pair(PUB_PIXEL_OPTOUT, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  AccountAdmin::pub_pixel_optout (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::PUB_PIXEL_OPTOUT].c_str();  
  }  
  inline  
  AccountAdmin::Expected&  
  AccountAdmin::Expected::timestamp(const std::string& val)  
  {  
     values_.insert(std::make_pair(TIMESTAMP, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  AccountAdmin::timestamp (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::TIMESTAMP].c_str();  
  }  
  inline  
  AccountAdmin::Expected&  
  AccountAdmin::Expected::force_remote_present(bool val)  
  {  
     force_remote_present_ = val;  
     return *this;  
  }  
  inline  
  bool  
  AccountAdmin::Expected::force_remote_present() const  
  {  
    return force_remote_present_;  
  }  
}
#endif  // __AUTOTESTS_COMMONS_ADMINS_ACCOUNTADMIN_HPP

