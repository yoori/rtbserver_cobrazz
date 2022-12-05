
#ifndef __AUTOTESTS_COMMONS_ADMINS_FRAUDPROFILEADMIN_HPP
#define __AUTOTESTS_COMMONS_ADMINS_FRAUDPROFILEADMIN_HPP

#include "Admins.hpp"

namespace AutoTest
{
  class FraudProfileAdmin:  
    public BaseAdminCmd<FraudProfileAdmin, 1>  
  {  
    public:    
      class Expected      
      {      
        friend class FraudProfileAdmin;        
        public:        
          Expected() {}          
          Expected& fraud_time (const std::string& val);          
                
          enum FieldName          
          {          
            FRAUD_TIME = 0            
          };          
        protected:        
          FieldIndexMap values_;          
      };      
        
    public:    
      typedef BaseAdminCmd<FraudProfileAdmin, FIELDS_COUNT> Base;    
        
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
        make_admin_cmd(*this, "print-fraud", address, params, static_cast<size_t>(service));      
        add_cmd_i("--align");      
      }      
      FraudProfileAdmin(      
        const char* address,      
        const char* uuid,      
        RequestInfoSrv service = RequestInfoManager      
      )      
      {      
        make_cmd(address, uuid, service);      
      }      
      FraudProfileAdmin(      
        const std::string& address,      
        const char* uuid,      
        RequestInfoSrv service = RequestInfoManager      
      )      
      {      
        make_cmd(address.c_str(), uuid, service);      
      }      
      FraudProfileAdmin(      
        const char* address,      
        const std::string& uuid,      
        RequestInfoSrv service = RequestInfoManager      
      )      
      {      
        make_cmd(address, uuid.c_str(), service);      
      }      
      FraudProfileAdmin(      
        const std::string& address,      
        const std::string& uuid,      
        RequestInfoSrv service = RequestInfoManager      
      )      
      {      
        make_cmd(address.c_str(), uuid.c_str(), service);      
      }      
      const char* fraud_time (unsigned int i = 0);      
      bool check(const Expected& expected, bool exist = true)      
        /*throw(eh::Exception)*/;      
      bool check(const std::string& expected, bool exist = true)      
        /*throw(eh::Exception)*/;      
  };  
    
}
namespace AutoTest
{
  inline  
  FraudProfileAdmin::Expected&  
  FraudProfileAdmin::Expected::fraud_time(const std::string& val)  
  {  
     values_.insert(std::make_pair(FRAUD_TIME, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  FraudProfileAdmin::fraud_time (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::FRAUD_TIME].c_str();  
  }  
}
#endif  // __AUTOTESTS_COMMONS_ADMINS_FRAUDPROFILEADMIN_HPP

