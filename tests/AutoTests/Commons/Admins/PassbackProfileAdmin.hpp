
#ifndef __AUTOTESTS_COMMONS_ADMINS_PASSBACKPROFILEADMIN_HPP
#define __AUTOTESTS_COMMONS_ADMINS_PASSBACKPROFILEADMIN_HPP

#include "Admins.hpp"

namespace AutoTest
{
  class PassbackProfileAdmin:  
    public BaseAdminCmd<PassbackProfileAdmin, 6>  
  {  
    public:    
      class Expected      
      {      
        friend class PassbackProfileAdmin;        
        public:        
          Expected() {}          
          Expected& tag_id (const std::string& val);          
          Expected& tag_id(unsigned long val);          
          Expected& colo_id (const std::string& val);          
          Expected& colo_id(unsigned long val);          
          Expected& user_status (const std::string& val);          
          Expected& time (const std::string& val);          
          Expected& done (const std::string& val);          
          Expected& verified (const std::string& val);          
                
          enum FieldName          
          {          
            TAG_ID = 0,            
            COLO_ID,            
            USER_STATUS,            
            TIME,            
            DONE,            
            VERIFIED            
          };          
        protected:        
          FieldIndexMap values_;          
      };      
        
    public:    
      typedef BaseAdminCmd<PassbackProfileAdmin, FIELDS_COUNT> Base;    
        
      void make_cmd (const char* address,      
        const char* requestid,      
        RequestInfoSrv service      
      )      
      {      
        address_ = address;        
        AdminParams params;        
        {        
          params.push_back(AdminParamPair("", requestid ? requestid : ""));          
        }        
        make_admin_cmd(*this, "print-passback", address, params, static_cast<size_t>(service));      
        add_cmd_i("--align");      
      }      
      PassbackProfileAdmin(      
        const char* address,      
        const char* requestid,      
        RequestInfoSrv service = RequestInfoManager      
      )      
      {      
        make_cmd(address, requestid, service);      
      }      
      PassbackProfileAdmin(      
        const std::string& address,      
        const char* requestid,      
        RequestInfoSrv service = RequestInfoManager      
      )      
      {      
        make_cmd(address.c_str(), requestid, service);      
      }      
      PassbackProfileAdmin(      
        const char* address,      
        const std::string& requestid,      
        RequestInfoSrv service = RequestInfoManager      
      )      
      {      
        make_cmd(address, requestid.c_str(), service);      
      }      
      PassbackProfileAdmin(      
        const std::string& address,      
        const std::string& requestid,      
        RequestInfoSrv service = RequestInfoManager      
      )      
      {      
        make_cmd(address.c_str(), requestid.c_str(), service);      
      }      
      const char* tag_id (unsigned int i = 0);      
      const char* colo_id (unsigned int i = 0);      
      const char* user_status (unsigned int i = 0);      
      const char* time (unsigned int i = 0);      
      const char* done (unsigned int i = 0);      
      const char* verified (unsigned int i = 0);      
      bool check(const Expected& expected, bool exist = true)      
        /*throw(eh::Exception)*/;      
      bool check(const std::string& expected, bool exist = true)      
        /*throw(eh::Exception)*/;      
  };  
    
}
namespace AutoTest
{
  inline  
  PassbackProfileAdmin::Expected&  
  PassbackProfileAdmin::Expected::tag_id(const std::string& val)  
  {  
     values_.insert(std::make_pair(TAG_ID, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  PassbackProfileAdmin::tag_id (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::TAG_ID].c_str();  
  }  
  inline  
  PassbackProfileAdmin::Expected&  
  PassbackProfileAdmin::Expected::tag_id(unsigned long val)  
  {  
     values_.insert(std::make_pair(TAG_ID, ComparableRegExp(strof(val))));  
     return *this;  
  }  
  inline  
  PassbackProfileAdmin::Expected&  
  PassbackProfileAdmin::Expected::colo_id(const std::string& val)  
  {  
     values_.insert(std::make_pair(COLO_ID, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  PassbackProfileAdmin::colo_id (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::COLO_ID].c_str();  
  }  
  inline  
  PassbackProfileAdmin::Expected&  
  PassbackProfileAdmin::Expected::colo_id(unsigned long val)  
  {  
     values_.insert(std::make_pair(COLO_ID, ComparableRegExp(strof(val))));  
     return *this;  
  }  
  inline  
  PassbackProfileAdmin::Expected&  
  PassbackProfileAdmin::Expected::user_status(const std::string& val)  
  {  
     values_.insert(std::make_pair(USER_STATUS, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  PassbackProfileAdmin::user_status (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::USER_STATUS].c_str();  
  }  
  inline  
  PassbackProfileAdmin::Expected&  
  PassbackProfileAdmin::Expected::time(const std::string& val)  
  {  
     values_.insert(std::make_pair(TIME, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  PassbackProfileAdmin::time (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::TIME].c_str();  
  }  
  inline  
  PassbackProfileAdmin::Expected&  
  PassbackProfileAdmin::Expected::done(const std::string& val)  
  {  
     values_.insert(std::make_pair(DONE, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  PassbackProfileAdmin::done (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::DONE].c_str();  
  }  
  inline  
  PassbackProfileAdmin::Expected&  
  PassbackProfileAdmin::Expected::verified(const std::string& val)  
  {  
     values_.insert(std::make_pair(VERIFIED, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  PassbackProfileAdmin::verified (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::VERIFIED].c_str();  
  }  
}
#endif  // __AUTOTESTS_COMMONS_ADMINS_PASSBACKPROFILEADMIN_HPP

