
#ifndef __AUTOTESTS_COMMONS_ADMINS_CCGKEYWORDADMIN_HPP
#define __AUTOTESTS_COMMONS_ADMINS_CCGKEYWORDADMIN_HPP

#include "Admins.hpp"

namespace AutoTest
{
  class CCGKeywordAdmin:  
    public BaseAdminCmd<CCGKeywordAdmin, 7>  
  {  
    public:    
      class Expected      
      {      
        friend class CCGKeywordAdmin;        
        public:        
          Expected() {}          
          Expected& ccg_keyword_id (const std::string& val);          
          Expected& ccg_id (const std::string& val);          
          Expected& channel_id (const std::string& val);          
          Expected& max_cpc (const std::string& val);          
          Expected& max_cpc (const Money& val);          
          Expected& ctr (const std::string& val);          
          Expected& ctr (const Money& val);          
          Expected& click_url (const std::string& val);          
          Expected& original_keyword (const std::string& val);          
                
          enum FieldName          
          {          
            CCG_KEYWORD_ID = 0,            
            CCG_ID,            
            CHANNEL_ID,            
            MAX_CPC,            
            CTR,            
            CLICK_URL,            
            ORIGINAL_KEYWORD            
          };          
        protected:        
          FieldIndexMap values_;          
      };      
        
    public:    
      typedef BaseAdminCmd<CCGKeywordAdmin, FIELDS_COUNT> Base;    
        
      void make_cmd (const char* address,      
        const char* ccg_keyword_id,      
        ChannelSrv service      
      )      
      {      
        address_ = address;        
        AdminParams params;        
        if (ccg_keyword_id)        
        {        
          params.push_back(AdminParamPair("--show=ccg_keyword_id=", ccg_keyword_id));          
        }        
        make_admin_cmd(*this, "all_ccg", address, params, static_cast<size_t>(service));      
      }      
      CCGKeywordAdmin(      
        const char* address,      
        const char* ccg_keyword_id,      
        ChannelSrv service = ChannelManagerController      
      )      
      {      
        make_cmd(address, ccg_keyword_id, service);      
      }      
      CCGKeywordAdmin(      
        const std::string& address,      
        const char* ccg_keyword_id,      
        ChannelSrv service = ChannelManagerController      
      )      
      {      
        make_cmd(address.c_str(), ccg_keyword_id, service);      
      }      
      CCGKeywordAdmin(      
        const char* address,      
        const std::string& ccg_keyword_id,      
        ChannelSrv service = ChannelManagerController      
      )      
      {      
        make_cmd(address, ccg_keyword_id.c_str(), service);      
      }      
      CCGKeywordAdmin(      
        const std::string& address,      
        const std::string& ccg_keyword_id,      
        ChannelSrv service = ChannelManagerController      
      )      
      {      
        make_cmd(address.c_str(), ccg_keyword_id.c_str(), service);      
      }      
      CCGKeywordAdmin(      
        const char* address,      
        unsigned long ccg_keyword_id,      
        ChannelSrv service = ChannelManagerController      
      )      
      {      
        make_cmd(address, strof(ccg_keyword_id).c_str(), service);      
      }      
      CCGKeywordAdmin(      
        const std::string& address,      
        unsigned long ccg_keyword_id,      
        ChannelSrv service = ChannelManagerController      
      )      
      {      
        make_cmd(address.c_str(), strof(ccg_keyword_id).c_str(), service);      
      }      
      const char* ccg_keyword_id (unsigned int i = 0);      
      const char* ccg_id (unsigned int i = 0);      
      const char* channel_id (unsigned int i = 0);      
      const char* max_cpc (unsigned int i = 0);      
      const char* ctr (unsigned int i = 0);      
      const char* click_url (unsigned int i = 0);      
      const char* original_keyword (unsigned int i = 0);      
      bool check(const Expected& expected, bool exist = true)      
        /*throw(eh::Exception)*/;      
      bool check(const std::string& expected, bool exist = true)      
        /*throw(eh::Exception)*/;      
  };  
    
}
namespace AutoTest
{
  inline  
  CCGKeywordAdmin::Expected&  
  CCGKeywordAdmin::Expected::ccg_keyword_id(const std::string& val)  
  {  
     values_.insert(std::make_pair(CCG_KEYWORD_ID, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  CCGKeywordAdmin::ccg_keyword_id (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::CCG_KEYWORD_ID].c_str();  
  }  
  inline  
  CCGKeywordAdmin::Expected&  
  CCGKeywordAdmin::Expected::ccg_id(const std::string& val)  
  {  
     values_.insert(std::make_pair(CCG_ID, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  CCGKeywordAdmin::ccg_id (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::CCG_ID].c_str();  
  }  
  inline  
  CCGKeywordAdmin::Expected&  
  CCGKeywordAdmin::Expected::channel_id(const std::string& val)  
  {  
     values_.insert(std::make_pair(CHANNEL_ID, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  CCGKeywordAdmin::channel_id (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::CHANNEL_ID].c_str();  
  }  
  inline  
  CCGKeywordAdmin::Expected&  
  CCGKeywordAdmin::Expected::max_cpc(const std::string& val)  
  {  
     values_.insert(std::make_pair(MAX_CPC, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  CCGKeywordAdmin::max_cpc (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::MAX_CPC].c_str();  
  }  
  inline  
  CCGKeywordAdmin::Expected&  
  CCGKeywordAdmin::Expected::max_cpc (const Money& val)  
  {  
     values_.insert(std::make_pair(MAX_CPC, val));  
     return *this;  
  }  
  inline  
  CCGKeywordAdmin::Expected&  
  CCGKeywordAdmin::Expected::ctr(const std::string& val)  
  {  
     values_.insert(std::make_pair(CTR, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  CCGKeywordAdmin::ctr (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::CTR].c_str();  
  }  
  inline  
  CCGKeywordAdmin::Expected&  
  CCGKeywordAdmin::Expected::ctr (const Money& val)  
  {  
     values_.insert(std::make_pair(CTR, val));  
     return *this;  
  }  
  inline  
  CCGKeywordAdmin::Expected&  
  CCGKeywordAdmin::Expected::click_url(const std::string& val)  
  {  
     values_.insert(std::make_pair(CLICK_URL, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  CCGKeywordAdmin::click_url (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::CLICK_URL].c_str();  
  }  
  inline  
  CCGKeywordAdmin::Expected&  
  CCGKeywordAdmin::Expected::original_keyword(const std::string& val)  
  {  
     values_.insert(std::make_pair(ORIGINAL_KEYWORD, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  CCGKeywordAdmin::original_keyword (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::ORIGINAL_KEYWORD].c_str();  
  }  
}
#endif  // __AUTOTESTS_COMMONS_ADMINS_CCGKEYWORDADMIN_HPP

