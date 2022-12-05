
#ifndef __AUTOTESTS_COMMONS_ADMINS_STATACCOUNTADMIN_HPP
#define __AUTOTESTS_COMMONS_ADMINS_STATACCOUNTADMIN_HPP

#include "Admins.hpp"

namespace AutoTest
{
  class StatAccountAdmin:  
    public BaseAdminCmd<StatAccountAdmin, 5>  
  {  
    public:    
      class Expected      
      {      
        friend class StatAccountAdmin;        
        public:        
          Expected() {}          
          Expected& account_id (const std::string& val);          
          Expected& account_id(unsigned long val);          
          Expected& amount (const std::string& val);          
          Expected& amount (const Money& val);          
          Expected& comm_amount (const std::string& val);          
          Expected& comm_amount (const Money& val);          
          Expected& daily_amount (const std::string& val);          
          Expected& daily_amount (const Money& val);          
          Expected& daily_comm_amount (const std::string& val);          
          Expected& daily_comm_amount (const Money& val);          
                
          enum FieldName          
          {          
            ACCOUNT_ID = 0,            
            AMOUNT,            
            COMM_AMOUNT,            
            DAILY_AMOUNT,            
            DAILY_COMM_AMOUNT            
          };          
        protected:        
          FieldIndexMap values_;          
      };      
        
    public:    
      typedef BaseAdminCmd<StatAccountAdmin, FIELDS_COUNT> Base;    
        
      void make_cmd (const char* address,      
        const char* id      
      )      
      {      
        address_ = address;        
        AdminParams params;        
        if (id)        
        {        
          params.push_back(AdminParamPair("id=", id));          
        }        
        make_admin_cmd(*this, "stat_account", address, params, static_cast<size_t>(CampaignServer));      
      }      
      StatAccountAdmin(      
        const char* address,      
        const char* id      
      )      
      {      
        make_cmd(address, id);      
      }      
      StatAccountAdmin(      
        const std::string& address,      
        const char* id      
      )      
      {      
        make_cmd(address.c_str(), id);      
      }      
      StatAccountAdmin(      
        const char* address,      
        const std::string& id      
      )      
      {      
        make_cmd(address, id.c_str());      
      }      
      StatAccountAdmin(      
        const std::string& address,      
        const std::string& id      
      )      
      {      
        make_cmd(address.c_str(), id.c_str());      
      }      
      StatAccountAdmin(      
        const char* address,      
        unsigned long id      
      )      
      {      
        make_cmd(address, strof(id).c_str());      
      }      
      StatAccountAdmin(      
        const std::string& address,      
        unsigned long id      
      )      
      {      
        make_cmd(address.c_str(), strof(id).c_str());      
      }      
      const char* account_id (unsigned int i = 0);      
      const char* amount (unsigned int i = 0);      
      const char* comm_amount (unsigned int i = 0);      
      const char* daily_amount (unsigned int i = 0);      
      const char* daily_comm_amount (unsigned int i = 0);      
      bool check(const Expected& expected, bool exist = true)      
        /*throw(eh::Exception)*/;      
      bool check(const std::string& expected, bool exist = true)      
        /*throw(eh::Exception)*/;      
  };  
    
}
namespace AutoTest
{
  inline  
  StatAccountAdmin::Expected&  
  StatAccountAdmin::Expected::account_id(const std::string& val)  
  {  
     values_.insert(std::make_pair(ACCOUNT_ID, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  StatAccountAdmin::account_id (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::ACCOUNT_ID].c_str();  
  }  
  inline  
  StatAccountAdmin::Expected&  
  StatAccountAdmin::Expected::account_id(unsigned long val)  
  {  
     values_.insert(std::make_pair(ACCOUNT_ID, ComparableRegExp(strof(val))));  
     return *this;  
  }  
  inline  
  StatAccountAdmin::Expected&  
  StatAccountAdmin::Expected::amount(const std::string& val)  
  {  
     values_.insert(std::make_pair(AMOUNT, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  StatAccountAdmin::amount (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::AMOUNT].c_str();  
  }  
  inline  
  StatAccountAdmin::Expected&  
  StatAccountAdmin::Expected::amount (const Money& val)  
  {  
     values_.insert(std::make_pair(AMOUNT, val));  
     return *this;  
  }  
  inline  
  StatAccountAdmin::Expected&  
  StatAccountAdmin::Expected::comm_amount(const std::string& val)  
  {  
     values_.insert(std::make_pair(COMM_AMOUNT, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  StatAccountAdmin::comm_amount (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::COMM_AMOUNT].c_str();  
  }  
  inline  
  StatAccountAdmin::Expected&  
  StatAccountAdmin::Expected::comm_amount (const Money& val)  
  {  
     values_.insert(std::make_pair(COMM_AMOUNT, val));  
     return *this;  
  }  
  inline  
  StatAccountAdmin::Expected&  
  StatAccountAdmin::Expected::daily_amount(const std::string& val)  
  {  
     values_.insert(std::make_pair(DAILY_AMOUNT, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  StatAccountAdmin::daily_amount (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::DAILY_AMOUNT].c_str();  
  }  
  inline  
  StatAccountAdmin::Expected&  
  StatAccountAdmin::Expected::daily_amount (const Money& val)  
  {  
     values_.insert(std::make_pair(DAILY_AMOUNT, val));  
     return *this;  
  }  
  inline  
  StatAccountAdmin::Expected&  
  StatAccountAdmin::Expected::daily_comm_amount(const std::string& val)  
  {  
     values_.insert(std::make_pair(DAILY_COMM_AMOUNT, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  StatAccountAdmin::daily_comm_amount (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::DAILY_COMM_AMOUNT].c_str();  
  }  
  inline  
  StatAccountAdmin::Expected&  
  StatAccountAdmin::Expected::daily_comm_amount (const Money& val)  
  {  
     values_.insert(std::make_pair(DAILY_COMM_AMOUNT, val));  
     return *this;  
  }  
}
#endif  // __AUTOTESTS_COMMONS_ADMINS_STATACCOUNTADMIN_HPP

